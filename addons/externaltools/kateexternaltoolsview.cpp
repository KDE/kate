/* This file is part of the KDE project
 *
 *  SPDX-FileCopyrightText: 2019 Dominik Haumann <dhaumann@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */
#include "kateexternaltoolsview.h"
#include "externaltoolsplugin.h"
#include "kateexternaltool.h"
#include "ktexteditor_utils.h"
#include "ui_toolview.h"

#include <KTextEditor/Application>
#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>

#include <KActionCollection>
#include <KAuthorized>
#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>
#include <KXMLGUIFactory>
#include <QMenu>
#include <QStandardPaths>

#include <QFontDatabase>
#include <QKeyEvent>
#include <QTextDocument>
#include <QToolButton>

#include <map>
#include <vector>

// BEGIN KateExternalToolsMenuAction
KateExternalToolsMenuAction::KateExternalToolsMenuAction(const QString &text,
                                                         KActionCollection *collection,
                                                         KateExternalToolsPlugin *plugin,
                                                         KTextEditor::MainWindow *mw)
    : KActionMenu(text, mw)
    , m_plugin(plugin)
    , m_mainwindow(mw)
    , m_actionCollection(collection)
{
    reload();

    // track active view to adapt enabled tool actions
    connect(mw, &KTextEditor::MainWindow::viewChanged, this, &KateExternalToolsMenuAction::slotViewChanged);
}

KateExternalToolsMenuAction::~KateExternalToolsMenuAction() = default;

void KateExternalToolsMenuAction::reload()
{
    // clear action collection
    bool needs_readd = (m_actionCollection->takeAction(this) != nullptr);
    m_actionCollection->clear();
    if (needs_readd) {
        m_actionCollection->addAction(QStringLiteral("tools_external"), this);
    }
    menu()->clear();

    // create tool actions
    std::map<QString, KActionMenu *> categories;
    std::vector<QAction *> uncategorizedActions;

    // first add categorized actions, such that the submenus appear at the top
    for (auto tool : m_plugin->tools()) {
        // we add all tools, even the non-executable ones, they will be disabled in updateActionState
        auto a = new QAction(tool->translatedName().replace(QLatin1Char('&'), QLatin1String("&&")), this);
        a->setIcon(QIcon::fromTheme(tool->icon));
        a->setData(QVariant::fromValue(tool));

        connect(a, &QAction::triggered, a, [this, a]() {
            m_plugin->runTool(*a->data().value<KateExternalTool *>(), m_mainwindow->activeView());
        });

        m_actionCollection->addAction(tool->actionName, a);
        if (!tool->category.isEmpty()) {
            auto categoryMenu = categories[tool->category];
            if (!categoryMenu) {
                categoryMenu = new KActionMenu(tool->translatedCategory(), this);
                categories[tool->category] = categoryMenu;
                addAction(categoryMenu);
            }
            categoryMenu->addAction(a);
        } else {
            uncategorizedActions.push_back(a);
        }
    }

    // now add uncategorized actions below
    for (auto uncategorizedAction : uncategorizedActions) {
        addAction(uncategorizedAction);
    }

    addSeparator();
    auto cfgAction = new QAction(i18n("Configure..."), this);
    addAction(cfgAction);
    connect(cfgAction, &QAction::triggered, this, &KateExternalToolsMenuAction::showConfigPage, Qt::QueuedConnection);

    // load shortcuts
    KSharedConfigPtr pConfig = m_plugin->config();
    KConfigGroup group(pConfig, QStringLiteral("Global"));
    group = KConfigGroup(pConfig, QStringLiteral("Shortcuts"));
    m_actionCollection->readSettings(&group);
    slotViewChanged(m_mainwindow->activeView());
}

void KateExternalToolsMenuAction::slotViewChanged(KTextEditor::View *view)
{
    // no active view, oh oh
    disconnect(m_docUrlChangedConnection);
    if (!view) {
        updateActionState(nullptr);
        return;
    }

    m_docUrlChangedConnection = connect(view->document(), &KTextEditor::Document::documentUrlChanged, this, [this](KTextEditor::Document *doc) {
        updateActionState(doc);
    });

    updateActionState(view->document());
}

void KateExternalToolsMenuAction::updateActionState(KTextEditor::Document *activeDoc)
{
    // try to enable/disable to match current mime type or if we have no doc
    const QString mimeType = activeDoc ? activeDoc->mimeType() : QString();
    const auto actions = m_actionCollection->actions();
    for (QAction *action : actions) {
        if (action && action->data().value<KateExternalTool *>()) {
            auto tool = action->data().value<KateExternalTool *>();
            action->setEnabled(activeDoc && tool->canExecute() && (tool->matchesMimetype(mimeType) || tool->mimetypes.isEmpty()));
        }
    }
}

void KateExternalToolsMenuAction::showConfigPage()
{
    m_mainwindow->showPluginConfigPage(m_plugin, 0);
}
// END KateExternalToolsMenuAction

// BEGIN KateExternalToolsPluginView
KateExternalToolsPluginView::KateExternalToolsPluginView(KTextEditor::MainWindow *mainWindow, KateExternalToolsPlugin *plugin)
    : QObject(mainWindow)
    , m_plugin(plugin)
    , m_mainWindow(mainWindow)
    , m_outputDoc(new QTextDocument(this))
{
    m_plugin->registerPluginView(this);

    KXMLGUIClient::setComponentName(QLatin1String("externaltools"), i18n("External Tools"));
    setXMLFile(QLatin1String("ui.rc"));

    if (KAuthorized::authorizeAction(QStringLiteral("shell_access"))) {
        m_externalToolsMenu = new KateExternalToolsMenuAction(i18n("External Tools"), actionCollection(), plugin, mainWindow);
        actionCollection()->addAction(QStringLiteral("tools_external"), m_externalToolsMenu);
        m_externalToolsMenu->setWhatsThis(i18n("Launch external helper applications"));
    }

    mainWindow->guiFactory()->addClient(this);

    // ESC should close & hide ToolView
    connect(m_mainWindow, &KTextEditor::MainWindow::unhandledShortcutOverride, this, &KateExternalToolsPluginView::handleEsc);
    connect(m_mainWindow, &KTextEditor::MainWindow::viewChanged, this, &KateExternalToolsPluginView::slotViewChanged);
    slotViewChanged(m_mainWindow->activeView());
}

KateExternalToolsPluginView::~KateExternalToolsPluginView()
{
    m_plugin->unregisterPluginView(this);

    m_mainWindow->guiFactory()->removeClient(this);

    deleteToolView();

    delete m_externalToolsMenu;
    m_externalToolsMenu = nullptr;
}

void KateExternalToolsPluginView::rebuildMenu()
{
    if (m_externalToolsMenu) {
        KXMLGUIFactory *f = factory();
        f->removeClient(this);
        reloadXML();
        m_externalToolsMenu->reload();
        f->addClient(this);
    }
}

KTextEditor::MainWindow *KateExternalToolsPluginView::mainWindow() const
{
    return m_mainWindow;
}

void KateExternalToolsPluginView::createToolView()
{
    if (!m_toolView) {
        m_toolView = mainWindow()->createToolView(m_plugin,
                                                  QStringLiteral("ktexteditor_plugin_externaltools"),
                                                  KTextEditor::MainWindow::Bottom,
                                                  QIcon::fromTheme(QStringLiteral("system-run")),
                                                  i18n("External Tools"));

        m_ui = new Ui::ToolView();
        m_ui->setupUi(m_toolView);

        // set the documents
        m_ui->teOutput->setDocument(m_outputDoc);

        // use fixed font for displaying status and output text
        const auto fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
        m_ui->teOutput->setFont(fixedFont);

        // close button to delete tool view
        auto btnClose = new QToolButton();
        btnClose->setAutoRaise(true);
        btnClose->setIcon(QIcon::fromTheme(QStringLiteral("tab-close")));
        connect(btnClose, &QToolButton::clicked, this, &KateExternalToolsPluginView::deleteToolView);
        m_ui->tabWidget->setCornerWidget(btnClose);
    }
}

void KateExternalToolsPluginView::showToolView()
{
    createToolView();
    m_ui->tabWidget->setCurrentWidget(m_ui->tabOutput);
    mainWindow()->showToolView(m_toolView);
}

void KateExternalToolsPluginView::clearToolView()
{
    m_outputDoc->clear();
}

void KateExternalToolsPluginView::setOutputData(const QString &data)
{
    QTextCursor cursor(m_outputDoc);
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(data);
}

void KateExternalToolsPluginView::deleteToolView()
{
    if (m_toolView) {
        delete m_ui;
        m_ui = nullptr;

        delete m_toolView;
        m_toolView = nullptr;
    }
}

void KateExternalToolsPluginView::handleEsc(QEvent *event)
{
    if (event->type() != QEvent::ShortcutOverride)
        return;

    auto keyEvent = static_cast<QKeyEvent *>(event);
    if (keyEvent && keyEvent->key() == Qt::Key_Escape && keyEvent->modifiers() == Qt::NoModifier) {
        deleteToolView();
    }
}

QAction *KateExternalToolsPluginView::externalToolsForDocumentAction(KTextEditor::Document *doc)
{
    if (!doc || doc->views().isEmpty())
        return nullptr;

    auto *ret = new KActionMenu(this);
    ret->setText(i18n("External Tools"));
    auto menu = ret->menu();
    connect(menu, &QMenu::aboutToShow, this, [doc, this, menu] {
        const auto mime = doc->mimeType();
        const QList<KateExternalTool *> &tools = m_plugin->tools();
        QPointer<KTextEditor::View> view = doc->views().first();
        for (auto tool : tools) {
            if (!tool->mimetypes.isEmpty() && !tool->matchesMimetype(mime)) {
                continue;
            }
            auto a = menu->addAction(QIcon::fromTheme(tool->icon), tool->translatedName());
            connect(a, &QAction::triggered, this, [this, tool, view] {
                m_plugin->runTool(*tool, view);
            });
        }
    });
    return ret;
}

void KateExternalToolsPluginView::slotViewChanged(KTextEditor::View *v)
{
    if (m_currentView) {
        disconnect(m_currentView->document(), &KTextEditor::Document::documentSavedOrUploaded, this, &KateExternalToolsPluginView::onDocumentSaved);
        disconnect(m_currentView->document(), &KTextEditor::Document::aboutToSave, this, &KateExternalToolsPluginView::onDocumentAboutToSave);
    }
    m_currentView = v;

    if (!m_currentView) {
        return;
    }

    connect(v->document(), &KTextEditor::Document::documentSavedOrUploaded, this, &KateExternalToolsPluginView::onDocumentSaved, Qt::UniqueConnection);
    connect(v->document(), &KTextEditor::Document::aboutToSave, this, &KateExternalToolsPluginView::onDocumentAboutToSave, Qt::UniqueConnection);
}

void KateExternalToolsPluginView::onDocumentSaved(KTextEditor::Document *doc)
{
    // We only want to run this in the current active mainwindow
    if (KTextEditor::Editor::instance()->application()->activeMainWindow() != m_mainWindow) {
        return;
    }

    const auto tools = m_plugin->tools();
    for (KateExternalTool *tool : tools) {
        const bool hasSaveTrigger = tool->trigger == KateExternalTool::Trigger::AfterSave;
        if (hasSaveTrigger && tool->matchesMimetype(doc->mimeType())) {
            m_plugin->runTool(*tool, m_currentView, /*exec save trigger=*/true);
        }
    }
}

void KateExternalToolsPluginView::onDocumentAboutToSave(KTextEditor::Document *doc)
{
    // We only want to run this in the current active mainwindow
    if (KTextEditor::Editor::instance()->application()->activeMainWindow() != m_mainWindow) {
        return;
    }

    const auto tools = m_plugin->tools();
    for (KateExternalTool *tool : tools) {
        const bool hasSaveTrigger = tool->trigger == KateExternalTool::Trigger::BeforeSave;
        if (hasSaveTrigger && tool->matchesMimetype(doc->mimeType())) {
            m_plugin->blockingRunTool(*tool, m_currentView, /*exec save trigger=*/true);
        }
    }
}

// END KateExternalToolsPluginView

#include "moc_kateexternaltoolsview.cpp"

// kate: space-indent on; indent-width 4; replace-tabs on;
