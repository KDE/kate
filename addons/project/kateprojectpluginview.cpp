/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2010 Christoph Cullmann <cullmann@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "kateprojectpluginview.h"

#include <ktexteditor/editor.h>
#include <ktexteditor/application.h>
#include <ktexteditor/view.h>
#include <ktexteditor/document.h>
#include <ktexteditor/codecompletioninterface.h>

#include <kactioncollection.h>
#include <kpluginfactory.h>
#include <kpluginloader.h>
#include <kaboutdata.h>
#include <KLocalizedString>
#include <KXMLGUIFactory>
#include <KIconLoader>

#include <QAction>
#include <QDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>

K_PLUGIN_FACTORY_WITH_JSON(KateProjectPluginFactory, "kateprojectplugin.json", registerPlugin<KateProjectPlugin>();)

KateProjectPluginView::KateProjectPluginView(KateProjectPlugin *plugin, KTextEditor::MainWindow *mainWin)
    : QObject(mainWin)
    , m_plugin(plugin)
    , m_mainWindow(mainWin)
    , m_toolView(nullptr)
    , m_toolInfoView(nullptr)
{
    KXMLGUIClient::setComponentName(QStringLiteral("kateproject"), i18n("Kate Project Manager"));
    setXMLFile(QStringLiteral("ui.rc"));

    /**
     * create views for all already existing projects
     * will create toolviews on demand!
     */
    foreach(KateProject * project, m_plugin->projects())
    viewForProject(project);

    /**
     * connect to important signals, e.g. for auto project view creation
     */
    connect(m_plugin, SIGNAL(projectCreated(KateProject *)), this, SLOT(viewForProject(KateProject *)));
    connect(m_mainWindow, SIGNAL(viewChanged(KTextEditor::View *)), this, SLOT(slotViewChanged()));
    connect(m_mainWindow, SIGNAL(viewCreated(KTextEditor::View *)), this, SLOT(slotViewCreated(KTextEditor::View *)));

    /**
     * connect for all already existing views
     */
    foreach(KTextEditor::View * view, m_mainWindow->views())
    slotViewCreated(view);

    /**
     * trigger once view change, to highlight right document
     */
    slotViewChanged();

    /**
     * back + forward
     */
    actionCollection()->addAction(KStandardAction::Back, QStringLiteral("projects_prev_project"), this, SLOT(slotProjectPrev()))->setShortcut(Qt::CTRL | Qt::ALT | Qt::Key_Left);
    actionCollection()->addAction(KStandardAction::Forward, QStringLiteral("projects_next_project"), this, SLOT(slotProjectNext()))->setShortcut(Qt::CTRL | Qt::ALT | Qt::Key_Right);

    /**
     * add us to gui
     */
    m_mainWindow->guiFactory()->addClient(this);
}

KateProjectPluginView::~KateProjectPluginView()
{
    /**
     * cleanup for all views
     */
    foreach(QObject * view, m_textViews) {
        KTextEditor::CodeCompletionInterface *cci = qobject_cast<KTextEditor::CodeCompletionInterface *>(view);
        if (cci) {
            cci->unregisterCompletionModel(m_plugin->completion());
        }
    }

    /**
     * cu toolviews
     */
    delete m_toolView;
    m_toolView = nullptr;
    delete m_toolInfoView;
    m_toolInfoView = nullptr;

    /**
     * cu gui client
     */
    m_mainWindow->guiFactory()->removeClient(this);
}

QPair<KateProjectView *, KateProjectInfoView *> KateProjectPluginView::viewForProject(KateProject *project)
{
    /**
     * needs valid project
     */
    Q_ASSERT(project);

    /**
     * create toolviews on demand
     */
    if (!m_toolView) {
        /**
         * create toolviews
         */
        m_toolView = m_mainWindow->createToolView(m_plugin, QStringLiteral("kateproject"), KTextEditor::MainWindow::Left, SmallIcon(QStringLiteral("project-open")), i18n("Projects"));
        m_toolInfoView = m_mainWindow->createToolView(m_plugin, QStringLiteral("kateprojectinfo"), KTextEditor::MainWindow::Bottom, SmallIcon(QStringLiteral("view-choose")), i18n("Current Project"));

        /**
         * create the combo + buttons for the toolViews + stacked widgets
         */
        m_projectsCombo = new QComboBox(m_toolView);
        m_reloadButton = new QToolButton(m_toolView);
        m_reloadButton->setIcon(SmallIcon(QStringLiteral("view-refresh")));
        QHBoxLayout *layout = new QHBoxLayout();
        layout->setSpacing(0);
        layout->addWidget(m_projectsCombo);
        layout->addWidget(m_reloadButton);
        m_toolView->layout()->addItem(layout);
        m_toolView->layout()->setSpacing(0);

        m_stackedProjectViews = new QStackedWidget(m_toolView);
        m_stackedProjectInfoViews = new QStackedWidget(m_toolInfoView);

        connect(m_projectsCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(slotCurrentChanged(int)));
        connect(m_reloadButton, SIGNAL(clicked(bool)), this, SLOT(slotProjectReload()));
    }

    /**
     * existing view?
     */
    if (m_project2View.contains(project)) {
        return m_project2View.value(project);
    }

    /**
     * create new views
     */
    KateProjectView *view = new KateProjectView(this, project);
    KateProjectInfoView *infoView = new KateProjectInfoView(this, project);

    /**
     * attach to toolboxes
     * first the views, then the combo, that triggers signals
     */
    m_stackedProjectViews->addWidget(view);
    m_stackedProjectInfoViews->addWidget(infoView);
    m_projectsCombo->addItem(SmallIcon(QStringLiteral("project-open")), project->name(), project->fileName());

    /**
     * remember and return it
     */
    return (m_project2View[project] = QPair<KateProjectView *, KateProjectInfoView *> (view, infoView));
}

QString KateProjectPluginView::projectFileName() const
{
    // nothing there, skip
    if (!m_toolView) {
        return QString();
    }

    QWidget *active = m_stackedProjectViews->currentWidget();
    if (!active) {
        return QString();
    }

    return static_cast<KateProjectView *>(active)->project()->fileName();
}

QString KateProjectPluginView::projectName() const
{
    // nothing there, skip
    if (!m_toolView) {
        return QString();
    }

    QWidget *active = m_stackedProjectViews->currentWidget();
    if (!active) {
        return QString();
    }

    return static_cast<KateProjectView *>(active)->project()->name();
}

QString KateProjectPluginView::projectBaseDir() const
{
    // nothing there, skip
    if (!m_toolView) {
        return QString();
    }

    QWidget *active = m_stackedProjectViews->currentWidget();
    if (!active) {
        return QString();
    }

    return static_cast<KateProjectView *>(active)->project()->baseDir();
}

QVariantMap KateProjectPluginView::projectMap() const
{
    // nothing there, skip
    if (!m_toolView) {
        return QVariantMap();
    }

    QWidget *active = m_stackedProjectViews->currentWidget();
    if (!active) {
        return QVariantMap();
    }

    return static_cast<KateProjectView *>(active)->project()->projectMap();
}

QStringList KateProjectPluginView::projectFiles() const
{
    // nothing there, skip
    if (!m_toolView) {
        return QStringList();
    }

    KateProjectView *active = static_cast<KateProjectView *>(m_stackedProjectViews->currentWidget());
    if (!active) {
        return QStringList();
    }

    return active->project()->files();
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
        m_activeTextEditorView->document()->disconnect(this);
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
    connect(m_activeTextEditorView->document(), SIGNAL(documentUrlChanged(KTextEditor::Document *)), this, SLOT(slotDocumentUrlChanged(KTextEditor::Document *)));

    /**
     * trigger slot once
     */
    slotDocumentUrlChanged(m_activeTextEditorView->document());
}

void KateProjectPluginView::slotCurrentChanged(int index)
{
    // nothing there, skip
    if (!m_toolView) {
        return;
    }

    /**
     * trigger change of stacked widgets
     */
    m_stackedProjectViews->setCurrentIndex(index);
    m_stackedProjectInfoViews->setCurrentIndex(index);

    /**
     * open currently selected document
     */
    if (QWidget *current = m_stackedProjectViews->currentWidget()) {
        static_cast<KateProjectView *>(current)->openSelectedDocument();
    }

    /**
     * project file name might have changed
     */
    emit projectFileNameChanged();
    emit projectMapChanged();
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
    m_project2View.value(project).first->selectFile(document->url().toLocalFile());

    /**
     * get active project view and switch it, if it is for a different project
     * do this AFTER file selection
     */
    KateProjectView *active = static_cast<KateProjectView *>(m_stackedProjectViews->currentWidget());
    if (active != m_project2View.value(project).first) {
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
    connect(view, SIGNAL(destroyed(QObject *)), this, SLOT(slotViewDestroyed(QObject *)));

    /**
     * add completion model if possible
     */
    KTextEditor::CodeCompletionInterface *cci = qobject_cast<KTextEditor::CodeCompletionInterface *>(view);
    if (cci) {
        cci->registerCompletionModel(m_plugin->completion());
    }

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
    // nothing there, skip
    if (!m_toolView) {
        return;
    }

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
    // nothing there, skip
    if (!m_toolView) {
        return;
    }

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
    // nothing there, skip
    if (!m_toolView) {
        return;
    }

    /**
     * force reload if any active project
     */
    if (QWidget *current = m_stackedProjectViews->currentWidget()) {
        static_cast<KateProjectView *>(current)->project()->reload(true);
    }
}

#include "kateprojectpluginview.moc"

