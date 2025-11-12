/*  This file is part of the Kate project.
 *
 *  SPDX-FileCopyrightText: 2010 Christoph Cullmann <cullmann@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "kateprojectpluginview.h"
#include "branchcheckoutdialog.h"
#include "currentgitbranchbutton.h"
#include "fileutil.h"
#include "gitprocess.h"
#include "gitwidget.h"
#include "kateproject.h"
#include "kateprojectinfoview.h"
#include "kateprojectinfoviewindex.h"
#include "kateprojectplugin.h"
#include "kateprojectview.h"
#include "ktexteditor_utils.h"

#include <KTextEditor/Command>
#include <KTextEditor/Document>
#include <ktexteditor/application.h>
#include <ktexteditor/editor.h>
#include <ktexteditor/mainwindow.h>
#include <ktexteditor/view.h>

#include <kconfigwidgets_version.h>

#include <KActionCollection>
#include <KActionMenu>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KStandardAction>
#include <KStringHandler>
#include <KXMLGUIFactory>
#include <KXmlGuiWindow>

#include <QAction>
#include <QComboBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QMenu>
#include <QStackedWidget>

#define PROJECTCLOSEICON "window-close"

K_PLUGIN_FACTORY_WITH_JSON(KateProjectPluginFactory, "kateprojectplugin.json", registerPlugin<KateProjectPlugin>();)

KateProjectPluginView::KateProjectPluginView(KateProjectPlugin *plugin, KTextEditor::MainWindow *mainWin)
    : QObject(mainWin)
    , m_plugin(plugin)
    , m_mainWindow(mainWin)
    , m_toolView(nullptr)
    , m_toolInfoView(nullptr)
    , m_toolMultiView(nullptr)
    , m_lookupAction(nullptr)
    , m_gotoSymbolAction(nullptr)
    , m_gotoSymbolActionAppMenu(nullptr)
{
    KXMLGUIClient::setComponentName(QStringLiteral("kateproject"), i18n("Project Manager"));
    setXMLFile(QStringLiteral("ui.rc"));

    /**
     * create toolviews
     */
    m_toolView = m_mainWindow->createToolView(m_plugin,
                                              QStringLiteral("kateproject"),
                                              KTextEditor::MainWindow::Left,
                                              QIcon::fromTheme(QStringLiteral("project-open")),
                                              i18n("Projects"));
    m_gitToolView.reset(m_mainWindow->createToolView(m_plugin, QStringLiteral("kateprojectgit"), KTextEditor::MainWindow::Left, gitIcon(), i18n("Git")));
    m_toolInfoView = m_mainWindow->createToolView(m_plugin,
                                                  QStringLiteral("kateprojectinfo"),
                                                  KTextEditor::MainWindow::Bottom,
                                                  QIcon::fromTheme(QStringLiteral("view-choose")),
                                                  i18n("Project"));

    /**
     * create the combo + buttons for the toolViews + stacked widgets
     */
    m_projectsCombo = new QComboBox(m_toolView);
    m_projectsCombo->setToolTip(i18n("Open projects list"));
    m_projectsCombo->setFrame(false);
    m_reloadButton = new QToolButton(m_toolView);
    m_reloadButton->setAutoRaise(true);
    m_reloadButton->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh")));
    m_reloadButton->setToolTip(i18n("Reload project"));
    m_closeProjectButton = new QToolButton(m_toolView);
    m_closeProjectButton->setAutoRaise(true);
    m_closeProjectButton->setToolTip(i18n("Close project"));
    m_closeProjectButton->setIcon(QIcon::fromTheme(QStringLiteral(PROJECTCLOSEICON)));
    auto *layout = new QHBoxLayout();
    layout->setSpacing(0);
    layout->addWidget(m_projectsCombo);
    layout->addWidget(m_reloadButton);
    layout->addWidget(m_closeProjectButton);
    m_toolView->layout()->addItem(layout);
    m_toolView->layout()->setSpacing(0);

    auto separator = new QFrame(m_toolView);
    separator->setFrameShape(QFrame::HLine);
    separator->setEnabled(false);
    m_toolView->layout()->addWidget(separator);

    m_gitToolView->layout()->setSpacing(0);

    m_stackedProjectViews = new QStackedWidget(m_toolView);
    m_stackedProjectInfoViews = new QStackedWidget(m_toolInfoView);
    m_gitWidget = new GitWidget(m_mainWindow, this, m_gitToolView.get());

    connect(m_projectsCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &KateProjectPluginView::slotCurrentChanged);
    connect(m_reloadButton, &QToolButton::clicked, this, &KateProjectPluginView::slotProjectReload);

    connect(m_closeProjectButton, &QToolButton::clicked, this, &KateProjectPluginView::slotCloseProject);
    connect(m_plugin, &KateProjectPlugin::pluginViewProjectClosing, this, &KateProjectPluginView::slotHandleProjectClosing);

    connect(&m_plugin->fileWatcher(), &QFileSystemWatcher::fileChanged, this, [this](const QString &path) {
        if (m_gitChangedWatcherFile == path) {
            slotUpdateStatus(true);
        }
    });

    /**
     * create views for all already existing projects
     * will create toolviews on demand!
     */
    const auto projectList = m_plugin->projects();
    for (KateProject *project : projectList) {
        viewForProject(project);
    }
    // If the list of projects is empty we do not want to restore the tool view from the last session, BUG: 432296
    if (projectList.isEmpty()) {
        // We have to call this in the next iteration of the event loop, after the session is restored
        QTimer::singleShot(0, this, [this]() {
            m_mainWindow->hideToolView(m_toolView);
            m_mainWindow->hideToolView(m_gitToolView.get());
            m_mainWindow->hideToolView(m_toolInfoView);
            if (m_toolMultiView) {
                m_mainWindow->hideToolView(m_toolMultiView);
            }
        });
    }

    /**
     * connect to important signals, e.g. for auto project view creation
     */
    connect(m_plugin, &KateProjectPlugin::projectCreated, this, &KateProjectPluginView::viewForProject);
    connect(m_plugin, &KateProjectPlugin::configUpdated, this, &KateProjectPluginView::slotConfigUpdated);
    connect(m_mainWindow, &KTextEditor::MainWindow::viewChanged, this, &KateProjectPluginView::slotViewChanged);
    connect(m_mainWindow, &KTextEditor::MainWindow::viewCreated, this, &KateProjectPluginView::slotViewCreated);

    /**
     * connect for all already existing views
     */
    const auto views = m_mainWindow->views();
    for (KTextEditor::View *view : views) {
        slotViewCreated(view);
    }

    /**
     * back + forward
     */
    auto a = actionCollection()->addAction(QStringLiteral("projects_open_project"), this, [this] {
        openDirectoryOrProject();
    });
    a->setText(i18n("Open &Folder..."));
    a->setIcon(QIcon::fromTheme(QStringLiteral("document-open-folder")));
    KActionCollection::setDefaultShortcut(a, QKeySequence(QKeySequence(QStringLiteral("Ctrl+T, O"), QKeySequence::PortableText)));

    m_projectTodosAction = a = actionCollection()->addAction(QStringLiteral("projects_todos"));
    connect(a, &QAction::triggered, this, &KateProjectPluginView::showProjectTodos);
    a->setText(i18n("Project TODOs"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("korg-todo")));

    m_projectPrevAction = a = actionCollection()->addAction(QStringLiteral("projects_prev_project"));
    connect(a, &QAction::triggered, this, &KateProjectPluginView::slotProjectPrev);
    a->setText(i18n("Activate Previous Project"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("arrow-left")));
    KActionCollection::setDefaultShortcut(a, QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Left));

    m_projectNextAction = a = actionCollection()->addAction(QStringLiteral("projects_next_project"));
    connect(a, &QAction::triggered, this, &KateProjectPluginView::slotProjectNext);
    a->setText(i18n("Activate Next Project"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("arrow-right")));
    KActionCollection::setDefaultShortcut(a, QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Right));

    m_projectGotoIndexAction = a = actionCollection()->addAction(QStringLiteral("projects_goto_index"));
    connect(a, &QAction::triggered, this, &KateProjectPluginView::slotProjectIndex);
    a->setText(i18n("Lookup"));
    KActionCollection::setDefaultShortcut(a, QKeySequence(Qt::ALT | Qt::Key_1));

    m_projectCloseAction = a = actionCollection()->addAction(QStringLiteral("projects_close"));
    connect(a, &QAction::triggered, this, &KateProjectPluginView::slotCloseProject);
    a->setText(i18n("Close Project"));
    a->setIcon(QIcon::fromTheme(QStringLiteral(PROJECTCLOSEICON)));

    m_projectCloseAllAction = a = actionCollection()->addAction(QStringLiteral("projects_close_all"));
    connect(a, &QAction::triggered, this, &KateProjectPluginView::slotCloseAllProjects);
    a->setText(i18n("Close All Projects"));
    a->setIcon(QIcon::fromTheme(QStringLiteral(PROJECTCLOSEICON)));

    m_projectCloseWithoutDocumentsAction = a = actionCollection()->addAction(QStringLiteral("projects_close_without_open_documents"));
    connect(a, &QAction::triggered, this, &KateProjectPluginView::slotCloseAllProjectsWithoutDocuments);
    a->setText(i18n("Close Orphaned Projects"));
    a->setIcon(QIcon::fromTheme(QStringLiteral(PROJECTCLOSEICON)));

    m_projectReloadAction = a = actionCollection()->addAction(QStringLiteral("project_reload"));
    connect(a, &QAction::triggered, this, &KateProjectPluginView::slotProjectReload);
    a->setText(i18n("Reload Project"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh")));

    m_gotoSymbolActionAppMenu = a = actionCollection()->addAction(KStandardAction::Goto, QStringLiteral("projects_goto_symbol"));
    connect(a, &QAction::triggered, this, &KateProjectPluginView::slotGotoSymbol);

    auto chckbrAct = actionCollection()->addAction(QStringLiteral("checkout_branch"), this, [this] {
        auto *bd = new BranchCheckoutDialog(mainWindow()->window(), projectBaseDir());
        bd->openDialog();
    });
    chckbrAct->setIcon(QIcon::fromTheme(QStringLiteral("vcs-branch")));
    chckbrAct->setText(i18n("Checkout Git Branch"));

    // popup menu
    auto popup = new KActionMenu(i18n("Project"), this);
    actionCollection()->addAction(QStringLiteral("popup_project"), popup);

    m_lookupAction = popup->menu()->addAction(i18n("Lookup: %1", QString()), this, &KateProjectPluginView::slotProjectIndex);
    m_gotoSymbolAction = popup->menu()->addAction(i18n("Goto: %1", QString()), this, &KateProjectPluginView::slotGotoSymbol);

    connect(popup->menu(), &QMenu::aboutToShow, this, &KateProjectPluginView::slotContextMenuAboutToShow);

    connect(m_mainWindow, &KTextEditor::MainWindow::unhandledShortcutOverride, this, &KateProjectPluginView::handleEsc);

    // update git status on toolview visibility change
    connect(m_gitToolView.get(), SIGNAL(toolVisibleChanged(bool)), this, SLOT(slotUpdateStatus(bool)));

    /**
     * add us to gui
     */
    m_mainWindow->guiFactory()->addClient(this);

    /**
     * align to current config
     */
    slotConfigUpdated();

    /**
     * trigger once view change, to highlight right document
     */
    slotViewChanged();

    /**
     * ensure proper action update, to enable/disable stuff
     */
    connect(this, &KateProjectPluginView::projectMapChanged, this, &KateProjectPluginView::updateActions);
    updateActions();
}

KateProjectPluginView::~KateProjectPluginView()
{
    /**
     * cleanup for all views
     */
    for (QObject *view : std::as_const(m_textViews)) {
        auto *v = qobject_cast<KTextEditor::View *>(view);
        if (v) {
            v->unregisterCompletionModel(m_plugin->completion());
        }
    }

    /**
     * cu toolviews
     */
    delete m_toolView;
    m_toolView = nullptr;
    delete m_toolInfoView;
    m_toolInfoView = nullptr;
    delete m_toolMultiView;
    m_toolMultiView = nullptr;

    /**
     * cu gui client
     */
    m_mainWindow->guiFactory()->removeClient(this);

    // Don't watch what nobody use, the old project...
    if (!m_gitChangedWatcherFile.isEmpty()) {
        m_plugin->fileWatcher().removePath(m_gitChangedWatcherFile);
    }
}

void KateProjectPluginView::slotConfigUpdated()
{
    if (!m_plugin->multiProjectGoto()) {
        delete m_toolMultiView;
        m_toolMultiView = nullptr;
    } else if (!m_toolMultiView) {
        m_toolMultiView = m_mainWindow->createToolView(m_plugin,
                                                       QStringLiteral("kateprojectmulti"),
                                                       KTextEditor::MainWindow::Bottom,
                                                       QIcon::fromTheme(QStringLiteral("view-choose")),
                                                       i18n("Projects Index"));
        auto gotoindex = new KateProjectInfoViewIndex(this, nullptr, m_toolMultiView);
        m_toolMultiView->layout()->addWidget(gotoindex);
    }

    // update action state
    updateActions();
}

void KateProjectPluginView::viewForProject(KateProject *project)
{
    /**
     * needs valid project
     */
    Q_ASSERT(project);

    /**
     * existing view?
     */
    if (m_project2View.contains(project)) {
        m_project2View.value(project);
        return;
    }

    /**
     * create new views
     */
    auto *view = new KateProjectView(this, project);
    auto *infoView = new KateProjectInfoView(this, project);

    /**
     * attach to toolboxes
     * first the views, then the combo, that triggers signals
     */
    m_stackedProjectViews->addWidget(view);
    m_stackedProjectInfoViews->addWidget(infoView);
    m_projectsCombo->addItem(QIcon::fromTheme(QStringLiteral("project-open")), project->name(), project->fileName());
    connect(project, &KateProject::projectMapChanged, this, [this] {
        auto widget = m_stackedProjectViews->currentWidget();
        auto project = static_cast<KateProjectView *>(widget)->project();
        if (widget && project == sender()) {
            Q_EMIT projectMapEdited();

            int index = m_projectsCombo->findData(project->fileName());
            Q_ASSERT(index == m_projectsCombo->currentIndex());
            if (index != -1) {
                m_projectsCombo->setItemText(index, project->name());
            }
        }
    });

    /*
     * inform onward
     */
    Q_EMIT pluginProjectAdded(project->baseDir(), project->name());

    /**
     * remember and return it
     */
    m_project2View[project] = ProjectViews{.tree = view, .infoview = infoView};
}

QString KateProjectPluginView::projectFileName() const
{
    QWidget *active = m_stackedProjectViews->currentWidget();
    if (!active) {
        return {};
    }

    return static_cast<KateProjectView *>(active)->project()->fileName();
}

QString KateProjectPluginView::projectName() const
{
    QWidget *active = m_stackedProjectViews->currentWidget();
    if (!active) {
        return {};
    }

    return static_cast<KateProjectView *>(active)->project()->name();
}

QString KateProjectPluginView::projectBaseDir() const
{
    QWidget *active = m_stackedProjectViews->currentWidget();
    if (!active) {
        return {};
    }

    return static_cast<KateProjectView *>(active)->project()->baseDir();
}

QVariantMap KateProjectPluginView::projectMap() const
{
    QWidget *active = m_stackedProjectViews->currentWidget();
    if (!active) {
        return {};
    }

    return static_cast<KateProjectView *>(active)->project()->projectMap();
}

QVariantMap KateProjectPluginView::projectMapFor(const QString &baseDir) const
{
    const auto projects = m_plugin->projects();

    for (const auto proj : projects) {
        if (proj->baseDir() == baseDir) {
            return proj->projectMap();
        }
    }
    return {};
}

QStringList KateProjectPluginView::projectFiles() const
{
    auto *active = static_cast<KateProjectView *>(m_stackedProjectViews->currentWidget());
    if (!active) {
        return {};
    }

    return active->project()->files();
}

QString KateProjectPluginView::allProjectsCommonBaseDir() const
{
    auto projects = m_plugin->projects();

    if (projects.empty()) {
        return {};
    }

    if (projects.size() == 1) {
        return projects[0]->baseDir();
    }

    QString commonParent1 = FileUtil::commonParent(projects[0]->baseDir(), projects[1]->baseDir());

    for (int i = 2; i < projects.size(); i++) {
        commonParent1 = FileUtil::commonParent(commonParent1, projects[i]->baseDir());
    }

    return commonParent1;
}

QStringList KateProjectPluginView::allProjectsFiles() const
{
    QStringList fileList;

    const auto projectList = m_plugin->projects();
    for (auto project : projectList) {
        fileList.append(project->files());
    }

    return fileList;
}

QMap<QString, QString> KateProjectPluginView::allProjects() const
{
    QMap<QString, QString> projectMap;

    const auto projectList = m_plugin->projects();
    for (auto project : projectList) {
        projectMap[project->baseDir()] = project->name();
    }
    return projectMap;
}

void KateProjectPluginView::slotViewChanged()
{
    /**
     * get active view
     */
    KTextEditor::View *activeView = m_mainWindow->activeView();

    /**
     * update pointer, maybe disconnect before
     */
    if (m_activeTextEditorView) {
        // but only url changed
        disconnect(m_activeTextEditorView->document(), &KTextEditor::Document::documentUrlChanged, this, &KateProjectPluginView::slotDocumentUrlChanged);
    }
    m_activeTextEditorView = activeView;

    /**
     * no current active view, return
     */
    if (!m_activeTextEditorView) {
        return;
    }

    /**
     * connect to url changed, for auto load
     */
    connect(m_activeTextEditorView->document(), &KTextEditor::Document::documentUrlChanged, this, &KateProjectPluginView::slotDocumentUrlChanged);

    /**
     * Watch any document, as long as we live, if it's saved
     */
    connect(m_activeTextEditorView->document(),
            &KTextEditor::Document::documentSavedOrUploaded,
            this,
            &KateProjectPluginView::slotDocumentSaved,
            Qt::UniqueConnection);

    /**
     * trigger slot once
     */
    slotDocumentUrlChanged(m_activeTextEditorView->document());
}

void KateProjectPluginView::slotDocumentSaved()
{
    slotUpdateStatus(true);
}

void KateProjectPluginView::slotCurrentChanged(int index)
{
    // trigger change of stacked widgets
    m_stackedProjectViews->setCurrentIndex(index);
    m_stackedProjectInfoViews->setCurrentIndex(index);

    // update focus proxy + open currently selected document
    if (QWidget *current = m_stackedProjectViews->currentWidget()) {
        m_stackedProjectViews->setFocusProxy(current);
        static_cast<KateProjectView *>(current)->openSelectedDocument();
    }

    // update focus proxy
    if (QWidget *current = m_stackedProjectInfoViews->currentWidget()) {
        m_stackedProjectInfoViews->setFocusProxy(current);
    }

    // Don't watch what nobody use, the old project...
    if (!m_gitChangedWatcherFile.isEmpty()) {
        m_plugin->fileWatcher().removePath(m_gitChangedWatcherFile);
        m_gitChangedWatcherFile.clear();
    }

    // ...and start watching the new one
    slotUpdateStatus(true);

    // project file name might have changed
    Q_EMIT projectFileNameChanged();
    Q_EMIT projectMapChanged();

    if (auto widget = gitWidget()) {
        widget->updateGitProjectFolder();
    }
}

void KateProjectPluginView::slotDocumentUrlChanged(KTextEditor::Document *document)
{
    /**
     * abort if empty url or no local path
     */
    if (document->url().isEmpty() || !document->url().isLocalFile()) {
        return;
    }

    /**
     * search matching project
     */
    KateProject *project = m_plugin->projectForUrl(document->url());
    if (!project) {
        return;
    }

    /**
     * select the file FIRST
     */
    m_project2View.value(project).tree->selectFile(document->url().toLocalFile());

    /**
     * get active project view and switch it, if it is for a different project
     * do this AFTER file selection
     */
    auto *active = static_cast<KateProjectView *>(m_stackedProjectViews->currentWidget());
    if (active != m_project2View.value(project).tree) {
        int index = m_projectsCombo->findData(project->fileName());
        if (index >= 0) {
            m_projectsCombo->setCurrentIndex(index);
        }
    }
}

void KateProjectPluginView::switchToProject(const QDir &dir)
{
    /**
     * search matching project
     */
    KateProject *project = m_plugin->projectForDir(dir);
    if (!project) {
        return;
    }

    /**
     * get active project view and switch it, if it is for a different project
     * do this AFTER file selection
     */
    auto *active = static_cast<KateProjectView *>(m_stackedProjectViews->currentWidget());
    if (active != m_project2View.value(project).tree) {
        int index = m_projectsCombo->findData(project->fileName());
        if (index >= 0) {
            m_projectsCombo->setCurrentIndex(index);
        }
    }
}

void KateProjectPluginView::slotViewCreated(KTextEditor::View *view)
{
    /**
     * connect to destroyed
     */
    connect(view, &KTextEditor::View::destroyed, this, &KateProjectPluginView::slotViewDestroyed);

    /**
     * add completion model if possible
     */
    view->registerCompletionModel(m_plugin->completion());

    /**
     * remember for this view we need to cleanup!
     */
    m_textViews.insert(view);
}

void KateProjectPluginView::slotViewDestroyed(QObject *view)
{
    /**
     * remove remembered views for which we need to cleanup on exit!
     */
    m_textViews.remove(view);
}

void KateProjectPluginView::slotProjectPrev()
{
    if (!m_projectsCombo->count()) {
        return;
    }

    if (m_projectsCombo->currentIndex() == 0) {
        m_projectsCombo->setCurrentIndex(m_projectsCombo->count() - 1);
    } else {
        m_projectsCombo->setCurrentIndex(m_projectsCombo->currentIndex() - 1);
    }
}

void KateProjectPluginView::slotProjectNext()
{
    if (!m_projectsCombo->count()) {
        return;
    }

    if (m_projectsCombo->currentIndex() + 1 == m_projectsCombo->count()) {
        m_projectsCombo->setCurrentIndex(0);
    } else {
        m_projectsCombo->setCurrentIndex(m_projectsCombo->currentIndex() + 1);
    }
}

void KateProjectPluginView::slotProjectReload()
{
    /**
     * force reload if any active project
     */
    if (QWidget *current = m_stackedProjectViews->currentWidget()) {
        static_cast<KateProjectView *>(current)->project()->reload(true);
    }
    /**
     * Refresh git status
     */
    if (auto widget = gitWidget()) {
        widget->updateStatus();
    }
}

void KateProjectPluginView::slotCloseProject()
{
    if (QWidget *current = m_stackedProjectViews->currentWidget()) {
        m_plugin->closeProject(static_cast<KateProjectView *>(current)->project());
    }
}

void KateProjectPluginView::slotCloseAllProjects()
{
    // we must close project after project
    // project closing might activate a different one and then would load the files of that project again
    const auto copiedProjects = m_plugin->projects();
    for (auto project : copiedProjects) {
        m_plugin->closeProject(project);
    }
}

void KateProjectPluginView::slotCloseAllProjectsWithoutDocuments()
{
    // we must close project after project
    // project closing might activate a different one and then would load the files of that project again
    const auto copiedProjects = m_plugin->projects();
    for (auto project : copiedProjects) {
        if (!m_plugin->projectHasOpenDocuments(project)) {
            m_plugin->closeProject(project);
        }
    }
}

void KateProjectPluginView::slotHandleProjectClosing(KateProject *project)
{
    const auto viewIt = m_project2View.find(project);
    Q_ASSERT(viewIt != m_project2View.end());

    const int index = m_stackedProjectViews->indexOf(viewIt->tree);

    m_stackedProjectViews->removeWidget(viewIt->tree);
    delete viewIt->tree;

    m_stackedProjectInfoViews->removeWidget(viewIt->infoview);
    delete viewIt->infoview;

    m_project2View.erase(viewIt);

    m_projectsCombo->removeItem(index);

    // Stop watching what no one is interesting anymore
    if (!m_gitChangedWatcherFile.isEmpty()) {
        m_plugin->fileWatcher().removePath(m_gitChangedWatcherFile);
        m_gitChangedWatcherFile.clear();
    }

    // inform onward
    Q_EMIT pluginProjectRemoved(project->baseDir(), project->name());

    // update actions, e.g. the close all stuff needs this
    updateActions();
}

QString KateProjectPluginView::currentWord() const
{
    KTextEditor::View *kv = m_activeTextEditorView;
    if (!kv) {
        return {};
    }

    if (kv->selection() && kv->selectionRange().onSingleLine()) {
        return kv->selectionText();
    }

    return kv->document()->wordAt(kv->cursorPosition());
}

void KateProjectPluginView::slotProjectIndex()
{
    const QString word = currentWord();
    if (!word.isEmpty()) {
        auto tabView = qobject_cast<QTabWidget *>(m_stackedProjectInfoViews->currentWidget());
        if (tabView) {
            if (auto codeIndex = tabView->findChild<KateProjectInfoViewIndex *>()) {
                tabView->setCurrentWidget(codeIndex);
            }
        }
        m_mainWindow->showToolView(m_toolInfoView);
        Q_EMIT projectLookupWord(word);
    }
}

void KateProjectPluginView::slotGotoSymbol()
{
    if (!m_toolMultiView) {
        return;
    }

    const QString word = currentWord();
    if (!word.isEmpty()) {
        int results = 0;
        Q_EMIT gotoSymbol(word, results);
        if (results > 1) {
            m_mainWindow->showToolView(m_toolMultiView);
        }
    }
}

void KateProjectPluginView::slotContextMenuAboutToShow()
{
    const QString word = currentWord();
    if (word.isEmpty()) {
        return;
    }

    const QString squeezed = KStringHandler::csqueeze(word, 30);
    m_lookupAction->setText(i18n("Lookup: %1", squeezed));
    m_gotoSymbolAction->setText(i18n("Goto: %1", squeezed));
}

void KateProjectPluginView::handleEsc(QEvent *e)
{
    if (!m_mainWindow) {
        return;
    }

    auto *k = static_cast<QKeyEvent *>(e);
    if (k->key() == Qt::Key_Escape && k->modifiers() == Qt::NoModifier) {
        const auto infoView = qobject_cast<const KateProjectInfoView *>(m_stackedProjectInfoViews->currentWidget());
        if (m_toolInfoView->isVisible() && (!infoView || !infoView->ignoreEsc())) {
            m_mainWindow->hideToolView(m_toolInfoView);
        }
    }
}

void KateProjectPluginView::slotUpdateStatus(bool visible)
{
    if (!visible) {
        return;
    }

    if (auto widget = gitWidget()) {
        // To support separate-git-dir always use dotGitPath
        // We need to add the path every time again because it's always a different file
        if (!m_gitChangedWatcherFile.isEmpty()) {
            m_plugin->fileWatcher().removePath(m_gitChangedWatcherFile);
        }
        m_gitChangedWatcherFile = widget->indexPath();
        if (!m_gitChangedWatcherFile.isEmpty()) {
            m_plugin->fileWatcher().addPath(m_gitChangedWatcherFile);
        }
        widget->updateStatus();
    }
}

void KateProjectPluginView::openDirectoryOrProject()
{
    // get dir or do nothing
    QFileDialog::Options opts;
    opts.setFlag(QFileDialog::ShowDirsOnly);
    opts.setFlag(QFileDialog::ReadOnly);
    const QString dir = QFileDialog::getExistingDirectory(m_mainWindow->window(), i18n("Choose a directory"), QDir::currentPath(), opts);
    if (dir.isEmpty()) {
        return;
    }

    openDirectoryOrProject(dir);
}

void KateProjectPluginView::openDirectoryOrProject(const QDir &dir)
{
    // switch to this project if there
    if (auto project = m_plugin->projectForDir(dir, true)) {
        openProject(project);
    }
}

void KateProjectPluginView::openProject(KateProject *project)
{
    // just activate the right plugin in the toolview
    slotActivateProject(project);

    // this is a user action, ensure the toolview is visible
    mainWindow()->showToolView(m_toolView);

    // add the project to the recently opened items list
    if (auto *parentClient = qobject_cast<KXmlGuiWindow *>(m_mainWindow->window())) {
        if (auto *openRecentAction = parentClient->action(KStandardAction::name(KStandardAction::StandardAction::OpenRecent))) {
            if (auto *recentFilesAction = qobject_cast<KRecentFilesAction *>(openRecentAction)) {
#if KCONFIGWIDGETS_VERSION >= QT_VERSION_CHECK(6, 9, 0)
                recentFilesAction->addUrl(QUrl::fromLocalFile(project->baseDir()), QString(), QStringLiteral("inode/directory"));
#else
                recentFilesAction->addUrl(QUrl::fromLocalFile(project->baseDir()));
#endif
            }
        }
    }
}

void KateProjectPluginView::showProjectTodos()
{
    KTextEditor::Command *pgrep = KTextEditor::Editor::instance()->queryCommand(QStringLiteral("pgrep"));
    if (!pgrep) {
        return;
    }
    QString msg;
    pgrep->exec(nullptr, QStringLiteral("preg (TODO|FIXME)\\b"), msg);
}

void KateProjectPluginView::openTerminal(const QString &dirPath, KateProject *project)
{
    m_mainWindow->showToolView(m_toolInfoView);

    auto it = m_project2View.constFind(project);
    if (it != m_project2View.cend()) {
        it->infoview->resetTerminal(dirPath);
    }
}

void KateProjectPluginView::updateActions()
{
    const bool hasMultipleProjects = m_projectsCombo->count() > 1;
    // currently some project active?
    const bool projectActive = !projectBaseDir().isEmpty();
    m_projectsCombo->setEnabled(projectActive);
    m_reloadButton->setEnabled(projectActive);
    m_closeProjectButton->setEnabled(projectActive);
    m_projectTodosAction->setEnabled(projectActive);
    m_projectPrevAction->setEnabled(projectActive && hasMultipleProjects);
    m_projectNextAction->setEnabled(projectActive && hasMultipleProjects);
    m_projectCloseAction->setEnabled(projectActive);
    m_projectCloseAllAction->setEnabled(hasMultipleProjects);
    m_projectCloseWithoutDocumentsAction->setEnabled(m_projectsCombo->count() > 0);

    const bool hasIndex = projectActive && m_plugin->getIndexEnabled();
    m_lookupAction->setVisible(hasIndex);
    m_gotoSymbolAction->setVisible(hasIndex);
    m_projectGotoIndexAction->setVisible(hasIndex);
    m_gotoSymbolActionAppMenu->setVisible(hasIndex);
    actionCollection()->action(QStringLiteral("popup_project"))->setVisible(hasIndex);
}

void KateProjectPluginView::slotActivateProject(KateProject *project)
{
    const int index = m_projectsCombo->findData(project->fileName());
    if (index >= 0) {
        m_projectsCombo->setCurrentIndex(index);
    }
}

void KateProjectPluginView::updateGitBranchButton(KateProject *project)
{
    if (!m_branchBtn) {
        m_branchBtn = std::make_unique<CurrentGitBranchButton>(mainWindow(), this, nullptr);
        auto a = actionCollection()->action(QStringLiteral("checkout_branch"));
        Q_ASSERT(a);
        m_branchBtn->setDefaultAction(a);
        Utils::insertWidgetInStatusbar(m_branchBtn.get(), mainWindow());
    }

    if (!project || project->baseDir() != projectBaseDir()) {
        return;
    }

    static_cast<CurrentGitBranchButton *>(m_branchBtn.get())->refresh();
}

GitWidget *KateProjectPluginView::gitWidget()
{
    return m_gitWidget;
}

void KateProjectPluginView::runCmdInTerminal(const QString &cmd)
{
    if (auto widget = qobject_cast<KateProjectInfoView *>(m_stackedProjectInfoViews->currentWidget())) {
        widget->runCmdInTerminal(cmd);
    }
}

#include "kateprojectpluginview.moc"
#include "moc_kateprojectpluginview.cpp"
