/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2001 Christoph Cullmann <cullmann@kde.org>
   SPDX-FileCopyrightText: 2001 Joseph Wenninger <jowenn@kde.org>
   SPDX-FileCopyrightText: 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>
   SPDX-FileCopyrightText: 2007 Flavio Castelli <flavio.castelli@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

// BEGIN Includes
#include "katemainwindow.h"
#include <KColorSchemeMenu>

#include "diagnostics/diagnosticview.h"
#include "filehistorywidget.h"
#include "kateapp.h"
#include "kateconfigdialog.h"
#include "katedocmanager.h"
#include "katefileactions.h"
#include "katemwmodonhddialog.h"
#include "kateoutputview.h"
#include "katepluginmanager.h"
#include "katequickopen.h"
#include "katesavemodifieddialog.h"
#include "katesessionmanager.h"
#include "katesessionsaction.h"
#include "katestashmanager.h"
#include "kateupdatedisabler.h"
#include "kateviewspace.h"
#include "ktexteditor_utils.h"
#include "texthint/KateTextHintManager.h"

#include <KActionCollection>
#include <KActionMenu>
#include <KColorSchemeManager>
#include <KConfigGroup>
#include <KEditToolBar>
#include <KFileItem>
#include <KHelpClient>
#include <KIO/ListJob>
#include <KLocalizedString>
#include <KMessageBox>
#include <KMultiTabBar>
#include <KOpenWithDialog>
#include <KRecentDocument>
#include <KRecentFilesAction>
#include <KSharedConfig>
#include <KShortcutsDialog>
#include <KStandardAction>
#include <KToggleFullScreenAction>
#include <KToolBar>
#include <KWindowConfig>
#include <KXMLGUIFactory>
#include <kconfigwidgets_version.h>
#include <kwidgetsaddons_version.h>

#include <QApplication>
#include <QDir>
#include <QFontDatabase>
#include <QKeySequence>
#include <QList>
#include <QMenu>
#include <QMenuBar>
#include <QMimeData>
#include <QMimeDatabase>
#include <QScreen>
#include <QStackedWidget>
#include <QTimer>
#include <QToolButton>

#include <ktexteditor/sessionconfiginterface.h>

// END

// shall windows close the documents only visible inside them if the are closed?
static bool winClosesDocuments()
{
    const auto config = KSharedConfig::openConfig();
    const KConfigGroup cgGeneral(config, QStringLiteral("General"));
    return cgGeneral.readEntry("Close documents with window", true);
}

KateMwModOnHdDialog *KateMainWindow::s_modOnHdDialog = nullptr;

KateContainerStackedLayout::KateContainerStackedLayout(QWidget *parent)
    : QStackedLayout(parent)
{
}

QSize KateContainerStackedLayout::sizeHint() const
{
    if (currentWidget()) {
        return currentWidget()->sizeHint();
    }
    return QStackedLayout::sizeHint();
}

QSize KateContainerStackedLayout::minimumSize() const
{
    if (currentWidget()) {
        return currentWidget()->minimumSize();
    }
    return QStackedLayout::minimumSize();
}

KateMainWindow::KateMainWindow(KConfig *sconfig, const QString &sgroup, bool userTriggered)
    : KateMDI::MainWindow(nullptr)
    , m_wrapper(new KTextEditor::MainWindow(this))
{
    /**
     * we don't want any flicker here
     */
    KateUpdateDisabler disableUpdates(this);

    // start session restore if needed
    startRestore(sconfig, sgroup);

    // setup most important actions first, needed by setupMainWindow
    setupImportantActions();

    // setup the most important widgets
    setupMainWindow();

    // setup the actions
    setupActions();

    setStandardToolBarMenuEnabled(true);
    setXMLFile(QStringLiteral("kateui.rc"));
    createShellGUI(true);

    // Has to be after the setXMLFile() call above, so that m_diagView's actions are
    // merged after the Tools menu has been populated with the default actions
    setupDiagnosticsView(sconfig);

    // qCDebug(LOG_KATE) << "****************************************************************************" << sconfig;

    // register mainwindow in app
    KateApp::self()->addMainWindow(this);

    // enable plugin guis
    KateApp::self()->pluginManager()->enableAllPluginsGUI(this, sconfig);

    // caption update
    const auto documents = KateApp::self()->documentManager()->documentList();
    for (auto doc : documents) {
        slotDocumentCreated(doc);
    }

    connect(KateApp::self()->documentManager(), &KateDocManager::documentCreated, this, &KateMainWindow::slotDocumentCreated);
    connect(KateApp::self(), &KateApp::configurationChanged, this, &KateMainWindow::readOptions);

    readOptions();

    if (sconfig && !userTriggered) {
        m_viewManager->restoreViewConfiguration(KConfigGroup(sconfig, sgroup));
    }

    // unstash
    // KateStashManager().popStash(m_viewManager);

    finishRestore();

    m_fileOpenRecent->loadEntries(KConfigGroup(sconfig, QStringLiteral("Recent Files")));

    setAcceptDrops(true);

    connect(KateApp::self()->sessionManager(), SIGNAL(sessionChanged()), this, SLOT(updateCaption()));

    connect(this, &KateMDI::MainWindow::sigShowPluginConfigPage, this, &KateMainWindow::showPluginConfigPage);

    connect(qApp, &QApplication::applicationStateChanged, this, &KateMainWindow::onApplicationStateChanged);

    // prior to this there was (possibly) no view, therefore not context menu.
    // Hence, we have to take care of the menu bar here
    toggleShowMenuBar(false);

    ensureHamburgerBarSize();

    // trigger proper focus restore
    m_viewManager->triggerActiveViewFocus();
}

KateMainWindow::~KateMainWindow()
{
    // first, save our fallback window size ;)
    KConfigGroup cfg(KSharedConfig::openConfig(), QStringLiteral("MainWindow"));
    KWindowConfig::saveWindowSize(windowHandle(), cfg);

    // save other options ;=)
    saveOptions();

    // close all documents not visible in other windows, we did ask for permission in queryClose
    if (winClosesDocuments()) {
        auto docs = KateApp::self()->documentManager()->documentList();
        const bool canStash = KateApp::self()->stashManager()->canStash();
        docs.erase(std::remove_if(docs.begin(),
                                  docs.end(),
                                  [this, canStash](auto doc) {
                                      if (canStash && (doc->isModified() || doc->url().isEmpty())) {
                                          return true;
                                      }
                                      return KateApp::self()->documentVisibleInOtherWindows(doc, this);
                                  }),
                   docs.end());
        KateApp::self()->documentManager()->closeDocuments(docs, false);
    }

    // Delete diagnostics view earlier so that destruction is faster
    // If we delay it then each provider will get unregisted one by one
    // and slow down the destruction as we will be clearing diagnostics
    // for each provider individually
    delete m_diagView;

    // unregister mainwindow in app
    KateApp::self()->removeMainWindow(this);

    // disable all plugin guis, delete all pluginViews
    KateApp::self()->pluginManager()->disableAllPluginsGUI(this);

    // delete the view manager, before KateMainWindow's wrapper is dead
    delete m_viewManager;
    m_viewManager = nullptr;

    // kill the wrapper object, now that all views are dead
    delete m_wrapper;
    m_wrapper = nullptr;
}

QSize KateMainWindow::sizeHint() const
{
    // ensure some proper sizing per default
    return (QSize(800, 600).expandedTo(minimumSizeHint())).expandedTo(screen()->availableSize() * 0.6).boundedTo(screen()->availableSize());
}

void KateMainWindow::setupImportantActions()
{
    m_paShowStatusBar = KStandardAction::showStatusbar(this, SLOT(toggleShowStatusBar()), actionCollection());
    m_paShowStatusBar->setWhatsThis(i18n("Use this command to show or hide the view's statusbar"));
    m_paShowMenuBar = KStandardAction::showMenubar(this, SLOT(toggleShowMenuBar()), actionCollection());

    m_paShowTabBar = new KToggleAction(i18n("Show &Tabs"), this);
    actionCollection()->addAction(QStringLiteral("settings_show_tab_bar"), m_paShowTabBar);
    connect(m_paShowTabBar, &QAction::toggled, this, &KateMainWindow::toggleShowTabBar);
    m_paShowTabBar->setWhatsThis(i18n("Use this command to show or hide the tabs for the views"));

    m_paShowPath = new KToggleAction(i18n("Sho&w Path in Titlebar"), this);
    actionCollection()->addAction(QStringLiteral("settings_show_full_path"), m_paShowPath);
    connect(m_paShowPath, SIGNAL(toggled(bool)), this, SLOT(updateCaption()));
    m_paShowPath->setWhatsThis(i18n("Show the complete document path in the window caption"));

    m_paShowUrlNavBar = new KToggleAction(i18n("Show Navigation Bar"), this);
    actionCollection()->addAction(QStringLiteral("settings_show_url_nav_bar"), m_paShowUrlNavBar);
    connect(m_paShowUrlNavBar, &QAction::toggled, this, [this](bool v) {
        m_viewManager->setShowUrlNavBar(v);
    });

    // Load themes
    KColorSchemeManager *manager = new KColorSchemeManager(this);
    auto *colorSelectionMenu = KColorSchemeMenu::createMenu(manager, this);
    colorSelectionMenu->menu()->setTitle(i18n("&Window Color Scheme"));
    actionCollection()->addAction(QStringLiteral("colorscheme_menu"), colorSelectionMenu);

    QAction *a = actionCollection()->addAction(KStandardAction::Back, QStringLiteral("view_prev_tab"));
    a->setText(i18n("&Previous Tab"));
    a->setWhatsThis(i18n("Focus the previous tab."));
    actionCollection()->setDefaultShortcuts(a, a->shortcuts() << KStandardShortcut::tabPrev());
    connect(a, &QAction::triggered, this, &KateMainWindow::slotFocusPrevTab);

    a = actionCollection()->addAction(KStandardAction::Forward, QStringLiteral("view_next_tab"));
    a->setText(i18n("&Next Tab"));
    a->setWhatsThis(i18n("Focus the next tab."));
    actionCollection()->setDefaultShortcuts(a, a->shortcuts() << KStandardShortcut::tabNext());
    connect(a, &QAction::triggered, this, &KateMainWindow::slotFocusNextTab);

    // the quick open action is used by the KateViewSpace "quick open button"
    a = actionCollection()->addAction(QStringLiteral("view_quick_open"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("quickopen")));
    a->setText(i18n("&Quick Open"));
    actionCollection()->setDefaultShortcut(a, QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_O));
    connect(a, &QAction::triggered, this, &KateMainWindow::slotQuickOpen);
    a->setWhatsThis(i18n("Open a form to quick open documents."));

    // enable hamburger menu
    auto hamburgerMenu = static_cast<KHamburgerMenu *>(actionCollection()->addAction(KStandardAction::HamburgerMenu, QStringLiteral("hamburger_menu")));
    hamburgerMenu->setMenuBar(menuBar());
    hamburgerMenu->setShowMenuBarAction(m_paShowMenuBar);
}

void KateMainWindow::setupMainWindow()
{
    m_viewManager = new KateViewManager(centralWidget(), this);
    centralWidget()->layout()->addWidget(m_viewManager);
    (static_cast<QBoxLayout *>(centralWidget()->layout()))->setStretchFactor(m_viewManager, 100);

    m_bottomViewBarContainer = new QWidget(centralWidget());
    centralWidget()->layout()->addWidget(m_bottomViewBarContainer);
    m_bottomContainerStack = new KateContainerStackedLayout(m_bottomViewBarContainer);

    if (KateApp::isKWrite()) {
        // Kwrite has nothing other than the view manager
        return;
    }

    /**
     * create generic output tool view
     * is used to display output of e.g. plugins
     */
    m_toolViewOutput = createToolView(nullptr /* toolview has no plugin it belongs to */,
                                      QStringLiteral("output"),
                                      KTextEditor::MainWindow::Bottom,
                                      QIcon::fromTheme(QStringLiteral("output_win")),
                                      i18n("Output"));
    m_outputView = new KateOutputView(this, m_toolViewOutput);
}

void KateMainWindow::setupActions()
{
    QAction *a;

    actionCollection()
        ->addAction(KStandardAction::New, QStringLiteral("file_new"), m_viewManager, SLOT(slotDocumentNew()))
        ->setWhatsThis(i18n("Create a new document"));
    actionCollection()
        ->addAction(KStandardAction::Open, QStringLiteral("file_open"), m_viewManager, SLOT(slotDocumentOpen()))
        ->setWhatsThis(i18n("Open an existing document for editing"));

    m_fileOpenRecent = KStandardAction::openRecent(
        m_viewManager,
        [this](const QUrl &url) {
            viewManager()->openUrlOrProject(url);
        },
        this);
    m_fileOpenRecent->setMaxItems(KateConfigDialog::recentFilesMaxCount());
    actionCollection()->addAction(m_fileOpenRecent->objectName(), m_fileOpenRecent);
    m_fileOpenRecent->setWhatsThis(i18n("This lists files which you have opened recently, and allows you to easily open them again."));

    a = actionCollection()->addAction(QStringLiteral("file_save_all"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("document-save-all")));
    a->setText(i18n("Save A&ll"));
    actionCollection()->setDefaultShortcut(a, QKeySequence(Qt::CTRL | Qt::Key_L));
    connect(a, &QAction::triggered, KateApp::self()->documentManager(), &KateDocManager::saveAll);
    a->setWhatsThis(i18n("Save all open, modified documents to disk."));

    a = actionCollection()->addAction(QStringLiteral("file_reload_all"));
    a->setText(i18n("&Reload All"));
    connect(a, &QAction::triggered, KateApp::self()->documentManager(), &KateDocManager::reloadAll);
    a->setWhatsThis(i18n("Reload all open documents."));

    a = actionCollection()->addAction(QStringLiteral("file_copy_filepath"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("edit-copy-path")));
    a->setText(i18n("Copy Location"));
    connect(a, &QAction::triggered, KateApp::self()->documentManager(), [this]() {
        auto &&view = viewManager()->activeView();
        KateFileActions::copyFilePathToClipboard(view->document());
    });
    a->setWhatsThis(i18n("Copies the file path of the current file to clipboard."));

    a = actionCollection()->addAction(QStringLiteral("file_open_containing_folder"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("document-open-folder")));
    a->setText(i18n("&Open Containing Folder"));
    connect(a, &QAction::triggered, KateApp::self()->documentManager(), [this]() {
        auto &&view = viewManager()->activeView();
        KateFileActions::openContainingFolder(view->document());
    });
    a->setWhatsThis(i18n("Copies the file path of the current file to clipboard."));

    a = actionCollection()->addAction(QStringLiteral("file_rename"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("edit-rename")));
    a->setText(i18nc("@action:inmenu", "Rename..."));
    connect(a, &QAction::triggered, KateApp::self()->documentManager(), [this]() {
        auto &&view = viewManager()->activeView();
        KateFileActions::renameDocumentFile(this, view->document());
    });
    a->setWhatsThis(i18n("Renames the file belonging to the current document."));

    a = actionCollection()->addAction(QStringLiteral("file_delete"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("edit-delete")));
    a->setText(i18nc("@action:inmenu", "Delete"));
    connect(a, &QAction::triggered, KateApp::self()->documentManager(), [this]() {
        auto &&view = viewManager()->activeView();
        KateFileActions::deleteDocumentFile(this, view->document());
    });
    a->setWhatsThis(i18n("Deletes the file belonging to the current document."));

    a = actionCollection()->addAction(QStringLiteral("file_properties"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("dialog-object-properties")));
    a->setText(i18n("Properties"));
    connect(a, &QAction::triggered, KateApp::self()->documentManager(), [this]() {
        auto &&view = viewManager()->activeView();
        KateFileActions::openFilePropertiesDialog(this, view->document());
    });
    a->setWhatsThis(i18n("Deletes the file belonging to the current document."));

    a = actionCollection()->addAction(QStringLiteral("file_compare"));
    a->setText(i18n("Compare"));
    connect(a, &QAction::triggered, KateApp::self()->documentManager(), [this]() {
        QMessageBox::information(this, i18n("Compare"), i18n("Use the Tabbar context menu to compare two documents"));
    });
    a->setWhatsThis(i18n("Shows a hint how to compare documents."));

    a = actionCollection()->addAction(QStringLiteral("file_close_orphaned"));
    a->setText(i18n("Close Orphaned"));
    connect(a, &QAction::triggered, KateApp::self()->documentManager(), &KateDocManager::closeOrphaned);
    a->setWhatsThis(i18n("Close all documents in the file list that could not be reopened, because they are not accessible anymore."));

    a = actionCollection()->addAction(KStandardAction::Close, QStringLiteral("file_close"), m_viewManager, SLOT(slotDocumentClose()));
    a->setIcon(QIcon::fromTheme(QStringLiteral("document-close")));
    a->setWhatsThis(i18n("Close the current document."));

    a = actionCollection()->addAction(QStringLiteral("file_close_other"));
    a->setText(i18n("Close Other"));
    connect(a, SIGNAL(triggered()), this, SLOT(slotDocumentCloseOther()));
    a->setWhatsThis(i18n("Close other open documents."));

    a = actionCollection()->addAction(QStringLiteral("file_close_all"));
    a->setText(i18n("Clos&e All"));
    connect(a, &QAction::triggered, this, &KateMainWindow::slotDocumentCloseAll);
    a->setWhatsThis(i18n("Close all open documents."));

    a = actionCollection()->addAction(KStandardAction::Quit, QStringLiteral("file_quit"));
    // Qt::QueuedConnection: delay real shutdown, as we are inside menu action handling (bug #185708)
    connect(a, &QAction::triggered, this, &KateMainWindow::slotFileQuit, Qt::QueuedConnection);
    a->setWhatsThis(i18n("Close this window"));

    a = actionCollection()->addAction(QStringLiteral("view_new_view"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("window-new")));
    a->setText(i18n("&New Window"));
    actionCollection()->setDefaultShortcut(a, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N));
    connect(a, &QAction::triggered, this, &KateMainWindow::newWindow);
    a->setWhatsThis(i18n("Create a new window."));

    m_showFullScreenAction = KStandardAction::fullScreen(nullptr, nullptr, this, this);
    actionCollection()->addAction(m_showFullScreenAction->objectName(), m_showFullScreenAction);
    actionCollection()->setDefaultShortcut(m_showFullScreenAction, Qt::Key_F11);
    connect(m_showFullScreenAction, &QAction::toggled, this, &KateMainWindow::slotFullScreen);

    documentOpenWith = new KActionMenu(i18n("Open W&ith"), this);
    actionCollection()->addAction(QStringLiteral("file_open_with"), documentOpenWith);
    documentOpenWith->setWhatsThis(i18n("Open the current document using another application registered for its file type, or an application of your choice."));
    connect(documentOpenWith->menu(), &QMenu::aboutToShow, this, &KateMainWindow::mSlotFixOpenWithMenu);
    connect(documentOpenWith->menu(), &QMenu::triggered, this, &KateMainWindow::slotOpenWithMenuAction);

    // no open with for KWrite ATM
    documentOpenWith->setVisible(KateApp::isKate());

    a = KStandardAction::keyBindings(this, SLOT(editKeys()), actionCollection());
    a->setWhatsThis(i18n("Configure the application's keyboard shortcut assignments."));

    a = KStandardAction::configureToolbars(this, SLOT(slotEditToolbars()), actionCollection());
    a->setWhatsThis(i18n("Configure which items should appear in the toolbar(s)."));

    QAction *settingsConfigure = KStandardAction::preferences(this, SLOT(slotConfigure()), actionCollection());
    settingsConfigure->setWhatsThis(i18n("Configure various aspects of this application and the editing component."));

    if (KateApp::self()->pluginManager()->pluginList().size() > 0) {
        a = actionCollection()->addAction(QStringLiteral("help_plugins_contents"));
        a->setText(i18n("&Plugins Handbook"));
        connect(a, &QAction::triggered, this, &KateMainWindow::pluginHelp);
        a->setWhatsThis(i18n("This shows help files for various available plugins."));
    }

    connect(m_viewManager, &KateViewManager::viewChanged, this, &KateMainWindow::slotWindowActivated);
    connect(m_viewManager, &KateViewManager::viewChanged, this, &KateMainWindow::slotUpdateActionsNeedingUrl);
    connect(m_viewManager, &KateViewManager::viewChanged, this, &KateMainWindow::slotUpdateBottomViewBar);

    // re-route signals to our wrapper
    connect(m_viewManager, &KateViewManager::viewChanged, m_wrapper, &KTextEditor::MainWindow::viewChanged);
    connect(m_viewManager, &KateViewManager::viewCreated, m_wrapper, &KTextEditor::MainWindow::viewCreated);
    connect(this, &KateMainWindow::unhandledShortcutOverride, m_wrapper, &KTextEditor::MainWindow::unhandledShortcutOverride);

    slotWindowActivated();

    // session actions, not for KWrite, create the full menu to be able to properly hide it
    if (KateApp::isKate()) {
        auto sessionsMenu = actionCollection()->addAction(QStringLiteral("sessions"));
        sessionsMenu->setText(i18n("Sess&ions"));
        sessionsMenu->setMenu(new QMenu(this));

        a = actionCollection()->addAction(QStringLiteral("sessions_new"));
        sessionsMenu->menu()->addAction(a);
        a->setIcon(QIcon::fromTheme(QStringLiteral("document-new")));
        a->setText(i18nc("Menu entry Session->New Session", "&New Session"));
        // Qt::QueuedConnection to avoid deletion of code that is executed when reducing the amount of mainwindows. (bug #227008)
        connect(a, &QAction::triggered, KateApp::self()->sessionManager(), &KateSessionManager::sessionNew, Qt::QueuedConnection);

        // recent sessions menu
        a = new KateSessionsAction(i18n("&Recent Sessions"), this, KateApp::self()->sessionManager(), false);
        sessionsMenu->menu()->addAction(a);
        actionCollection()->addAction(QStringLiteral("session_open_recent"), a);

        // session menu
        a = new KateSessionsAction(i18n("&All Sessions"), this, KateApp::self()->sessionManager(), true);
        sessionsMenu->menu()->addAction(a);
        actionCollection()->addAction(QStringLiteral("session_open_session"), a);

        a = actionCollection()->addAction(QStringLiteral("sessions_manage"));
        sessionsMenu->menu()->addAction(a);
        a->setIcon(QIcon::fromTheme(QStringLiteral("view-choose")));
        a->setText(i18n("&Manage Sessions..."));
        // Qt::QueuedConnection to avoid deletion of code that is executed when reducing the amount of mainwindows. (bug #227008)
        connect(a, &QAction::triggered, KateApp::self()->sessionManager(), &KateSessionManager::sessionManage, Qt::QueuedConnection);

        sessionsMenu->menu()->addSeparator();

        a = actionCollection()->addAction(QStringLiteral("sessions_save"));
        sessionsMenu->menu()->addAction(a);
        a->setIcon(QIcon::fromTheme(QStringLiteral("document-save")));
        a->setText(i18n("&Save Session"));
        connect(a, &QAction::triggered, KateApp::self()->sessionManager(), &KateSessionManager::sessionSave);

        a = actionCollection()->addAction(QStringLiteral("sessions_save_as"));
        sessionsMenu->menu()->addAction(a);
        a->setIcon(QIcon::fromTheme(QStringLiteral("document-save-as")));
        a->setText(i18n("Save Session &As..."));
        connect(a, &QAction::triggered, KateApp::self()->sessionManager(), &KateSessionManager::sessionSaveAs);
    }

    // location history actions
    a = actionCollection()->addAction(QStringLiteral("view_history_back"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("arrow-left")));
    a->setText(i18n("Go to Previous Location"));
    actionCollection()->setDefaultShortcut(a, QKeySequence(Qt::CTRL | Qt::Key_1));
    connect(a, &QAction::triggered, this, [this] {
        m_viewManager->activeViewSpace()->goBack();
    });
    connect(this->m_viewManager, &KateViewManager::historyBackEnabled, a, &QAction::setEnabled);

    a = actionCollection()->addAction(QStringLiteral("view_history_forward"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("arrow-right")));
    a->setText(i18n("Go to Next Location"));
    actionCollection()->setDefaultShortcut(a, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_1));
    connect(a, &QAction::triggered, this, [this] {
        m_viewManager->activeViewSpace()->goForward();
    });
    connect(this->m_viewManager, &KateViewManager::historyForwardEnabled, a, &QAction::setEnabled);

    a = actionCollection()->addAction(QStringLiteral("git_show_file_history"));
    a->setText(i18n("Show File Git History"));
    connect(a, &QAction::triggered, this, [this] {
        if (activeView()) {
            auto url = activeView()->document()->url();
            if (url.isValid() && url.isLocalFile()) {
                FileHistory::showFileHistory(url.toLocalFile(), m_wrapper);
            }
        }
    });
}

void KateMainWindow::setupDiagnosticsView(KConfig *sconfig)
{
    if (KateApp::isKWrite()) {
        return;
    }

    m_diagView = DiagnosticsView::instance(wrapper());
    m_diagView->readSessionConfig(KConfigGroup(sconfig, QStringLiteral("Kate Diagnostics")));
    // See comment in DiagnosticsView::DiagnosticsView()
    m_diagView->actionCollection()->addAssociatedWidget(m_viewManager);
}

void KateMainWindow::ensureHamburgerBarSize()
{
    // Ensure the hamburger menu never gets pushed into the toolbar overflow
    // by setting a minimum size on the bar based on the button's size.
    if (auto *hamburgerBar = toolBar(QStringLiteral("hamburgerBar"))) {
        auto *hamburgerMenu = actionCollection()->action(QStringLiteral("hamburger_menu"));
        if (auto *hamburgerButton = hamburgerBar->widgetForAction(hamburgerMenu)) {
            int neededButtonWidth = hamburgerButton->minimumSizeHint().width();
            // Add toolbar margins.
            const QMargins combinedMargins = hamburgerBar->contentsMargins() + hamburgerBar->layout()->contentsMargins();
            neededButtonWidth += combinedMargins.left();
            neededButtonWidth += combinedMargins.right();

            // The dynamic spacer is also an action leading to spacing being added.
            // Not observable with Breeze but e.g. Fusion style has toolbar button spacing.
            if (hamburgerBar->actions().count() > 1) {
                neededButtonWidth += hamburgerBar->layout()->spacing();
            }

            QSize minimumSize = hamburgerBar->minimumSize();
            minimumSize.setWidth(std::max(minimumSize.width(), neededButtonWidth));
            hamburgerBar->setMinimumSize(minimumSize);
        }
    }
}

void KateMainWindow::slotDocumentCloseAll()
{
    if (!KateApp::self()->documentManager()->documentList().empty()
        && KMessageBox::warningContinueCancel(this,
                                              i18n("This will close all open documents. Are you sure you want to continue?"),
                                              i18n("Close all documents"),
                                              KStandardGuiItem::cont(),
                                              KStandardGuiItem::cancel(),
                                              QStringLiteral("closeAll"))
            != KMessageBox::Cancel) {
        if (queryClose_internal()) {
            KateApp::self()->documentManager()->closeAllDocuments(false);
        }
    }
}

void KateMainWindow::slotDocumentCloseOther(KTextEditor::Document *document)
{
    if (KateApp::self()->documentManager()->documentList().size() > 1
        && KMessageBox::warningContinueCancel(this,
                                              i18n("This will close all open documents beside the current one. Are you sure you want to continue?"),
                                              i18n("Close all documents beside current one"),
                                              KStandardGuiItem::cont(),
                                              KStandardGuiItem::cancel(),
                                              QStringLiteral("closeOther"))
            != KMessageBox::Cancel) {
        if (queryClose_internal(document)) {
            KateApp::self()->documentManager()->closeOtherDocuments(document);
        }
    }
}

void KateMainWindow::slotDocumentCloseSelected(const QList<KTextEditor::Document *> &docList)
{
    QList<KTextEditor::Document *> documents;
    for (KTextEditor::Document *doc : docList) {
        if (queryClose_internal(doc)) {
            documents.push_back(doc);
        }
    }

    KateApp::self()->documentManager()->closeDocuments(documents);
}

void KateMainWindow::slotDocumentCloseOther()
{
    if (auto v = m_viewManager->activeView()) {
        slotDocumentCloseOther(v->document());
    }
}

bool KateMainWindow::queryClose_internal(KTextEditor::Document *doc, KateMainWindow *win)
{
    // we want no auto saving during windows closing, we handle that explicitly
    KateSessionManager::AutoSaveBlocker blocker(KateApp::self()->sessionManager());

    const auto documentCount = KateApp::self()->documentManager()->documentList().size();

    if (!showModOnDiskPrompt(PromptEdited)) {
        return false;
    }

    std::vector<KTextEditor::Document *> modifiedDocuments = KateApp::self()->documentManager()->modifiedDocumentList();

    // filter out what the stashManager will itself stash
    const bool canStash = KateApp::self()->stashManager()->canStash();
    if (canStash) {
        modifiedDocuments.erase(std::remove_if(modifiedDocuments.begin(),
                                               modifiedDocuments.end(),
                                               [](auto doc) {
                                                   return KateApp::self()->stashManager()->willStashDoc(doc);
                                               }),
                                modifiedDocuments.end());
    }

    // do we want to ignore some document?
    if (doc) {
        modifiedDocuments.erase(std::remove(modifiedDocuments.begin(), modifiedDocuments.end(), doc), modifiedDocuments.end());
    }

    // do we want to ignore all documents visible in other windows?
    if (win) {
        modifiedDocuments.erase(std::remove_if(modifiedDocuments.begin(),
                                               modifiedDocuments.end(),
                                               [win, canStash](auto doc) {
                                                   if (canStash && (doc->isModified() || doc->url().isEmpty())) {
                                                       return true;
                                                   }
                                                   return KateApp::self()->documentVisibleInOtherWindows(doc, win);
                                               }),
                                modifiedDocuments.end());
    }

    // Remove all documents that can be closed without user confirmation
    modifiedDocuments.erase(std::remove_if(modifiedDocuments.begin(),
                                           modifiedDocuments.end(),
                                           [](KTextEditor::Document *d) {
                                               return !d->isModified() || (d->isEmpty() && d->url().isEmpty());
                                           }),
                            modifiedDocuments.end());

    bool shutdown = modifiedDocuments.empty();
    if (!shutdown) {
        shutdown = KateSaveModifiedDialog::queryClose(this, modifiedDocuments);
    }

    if (KateApp::self()->documentManager()->documentList().size() > documentCount) {
        KMessageBox::information(this, i18n("New file opened while trying to close Kate, closing aborted."), i18n("Closing Aborted"));
        shutdown = false;
    }

    return shutdown;
}

/**
 * queryClose(), take care that after the last mainwindow the stuff is closed
 */
bool KateMainWindow::queryClose()
{
    // we want no auto saving during windows closing, we handle that explicitly
    KateSessionManager::AutoSaveBlocker blocker(KateApp::self()->sessionManager());

    // session saving, can we close all views ?
    // just test, not close them actually
    if (qApp->isSavingSession()) {
        return queryClose_internal();
    }

    // normal closing of window
    // if we are not the last window, just close the documents we own
    if (KateApp::self()->mainWindowsCount() > 1) {
        return winClosesDocuments() ? queryClose_internal(nullptr, this) : true;
    }

    // last one: check if we can close all documents, try run
    // and save docs if we really close down !
    if (queryClose_internal()) {
        KateApp::self()->sessionManager()->saveActiveSession(true);
        KateApp::self()->stashManager()->stashDocuments(KateApp::self()->sessionManager()->activeSession()->config(),
                                                        KateApp::self()->documentManager()->documentList());
        return true;
    }

    return false;
}

KateMainWindow *KateMainWindow::newWindow() const
{
    // create new window with current session
    // derive size from current one
    auto win = KateApp::self()->newMainWindow(KateApp::self()->sessionManager()->activeSession()->config(), {}, true);
    win->resize(size());
    return win;
}

void KateMainWindow::slotEditToolbars()
{
    KConfigGroup cfg(KSharedConfig::openConfig(), QStringLiteral("MainWindow"));
    saveMainWindowSettings(cfg);

    KEditToolBar dlg(factory(), this);

    connect(&dlg, &KEditToolBar::newToolBarConfig, this, &KateMainWindow::slotNewToolbarConfig);
    dlg.exec();
}

void KateMainWindow::reloadXmlGui()
{
    for (KTextEditor::Document *doc : KateApp::self()->documentManager()->documentList()) {
        doc->reloadXML();
        const auto views = doc->views();
        for (KTextEditor::View *view : views) {
            view->reloadXML();
        }
    }
}

void KateMainWindow::slotNewToolbarConfig()
{
    applyMainWindowSettings(KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("MainWindow")));

    // we need to reload all View's XML Gui from disk to ensure toolbar
    // changes are applied to all views.
    reloadXmlGui();
}

void KateMainWindow::slotFileQuit()
{
    KateApp::self()->shutdownKate(this);
}

void KateMainWindow::slotFileClose()
{
    m_viewManager->slotDocumentClose();
}

void KateMainWindow::slotOpenDocument(const QUrl &url)
{
    m_viewManager->openUrl(url, QString(), true);
}

void KateMainWindow::readOptions()
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig();

    const KConfigGroup generalGroup(config, QStringLiteral("General"));
    m_modNotification = generalGroup.readEntry("Modified Notification", false);
    m_modCloseAfterLast = generalGroup.readEntry("Close After Last", KateApp::isKWrite());
    KateApp::self()->documentManager()->setSaveMetaInfos(generalGroup.readEntry("Save Meta Infos", true));
    KateApp::self()->documentManager()->setDaysMetaInfos(generalGroup.readEntry("Days Meta Infos", 30));

    KateApp::self()->stashManager()->setStashUnsavedChanges(generalGroup.readEntry("Stash unsaved file changes", false));
    KateApp::self()->stashManager()->setStashNewUnsavedFiles(generalGroup.readEntry("Stash new unsaved files", true));

    m_paShowPath->setChecked(generalGroup.readEntry("Show Full Path in Title", false));
    m_paShowStatusBar->setChecked(generalGroup.readEntry("Show Status Bar", true));
    m_paShowMenuBar->setChecked(generalGroup.readEntry("Show Menu Bar", true));
    m_paShowTabBar->setChecked(generalGroup.readEntry("Show Tab Bar", true));
    m_paShowUrlNavBar->setChecked(generalGroup.readEntry("Show Url Nav Bar", KateApp::isKate()));

    for (auto a : {m_paShowMenuBar, m_paShowTabBar, m_paShowPath, m_paShowUrlNavBar, m_paShowStatusBar}) {
        connect(a, &QAction::toggled, this, &KateMainWindow::saveOptions);
    }

    m_mouseButtonBackAction = (MouseBackButtonAction)generalGroup.readEntry("Mouse back button action", 0);
    m_mouseButtonForwardAction = (MouseForwardButtonAction)generalGroup.readEntry("Mouse forward button action", 0);

    // emit signal to hide/show statusbars
    toggleShowStatusBar();
    toggleShowTabBar();
    m_viewManager->setShowUrlNavBar(m_paShowUrlNavBar->isChecked());
}

void KateMainWindow::saveOptions()
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig();

    KConfigGroup generalGroup(config, QStringLiteral("General"));

    generalGroup.writeEntry("Save Meta Infos", KateApp::self()->documentManager()->getSaveMetaInfos());

    generalGroup.writeEntry("Days Meta Infos", KateApp::self()->documentManager()->getDaysMetaInfos());

    generalGroup.writeEntry("Show Full Path in Title", m_paShowPath->isChecked());
    generalGroup.writeEntry("Show Status Bar", m_paShowStatusBar->isChecked());
    generalGroup.writeEntry("Show Menu Bar", m_paShowMenuBar->isChecked());
    generalGroup.writeEntry("Show Tab Bar", m_paShowTabBar->isChecked());
    generalGroup.writeEntry("Show Url Nav Bar", m_paShowUrlNavBar->isChecked());
}

void KateMainWindow::toggleShowMenuBar(bool showMessage)
{
    if (m_paShowMenuBar->isChecked()) {
        menuBar()->show();
        if (m_viewManager->activeView() && m_viewManager->activeView()->contextMenu()) {
            m_viewManager->activeView()->contextMenu()->removeAction(m_paShowMenuBar);
        }
    } else {
        // we have a hamburger button in the toolbar, we can avoid the message if that is still visible
        if (showMessage && toolBar()->isHidden()) {
            const QString accel = m_paShowMenuBar->shortcut().toString();
            KMessageBox::information(this,
                                     i18n("This will hide the menu bar completely."
                                          " You can show it again by typing %1.",
                                          accel),
                                     i18n("Hide menu bar"),
                                     QStringLiteral("HideMenuBarWarning"));
        }
        menuBar()->hide();
        if (m_viewManager->activeView() && m_viewManager->activeView()->contextMenu()) {
            m_viewManager->activeView()->contextMenu()->addAction(m_paShowMenuBar);
        }
    }
}

void KateMainWindow::toggleShowStatusBar()
{
    // just hide or show the status bar stack
    statusBarStackedWidget()->setVisible(showStatusBar());
}

bool KateMainWindow::showStatusBar()
{
    return m_paShowStatusBar->isChecked();
}

void KateMainWindow::toggleShowTabBar()
{
    Q_EMIT tabBarToggled();
}

bool KateMainWindow::showTabBar()
{
    return m_paShowTabBar->isChecked();
}

void KateMainWindow::slotWindowActivated()
{
    if (m_viewManager->activeView()) {
        updateCaption(m_viewManager->activeView()->document());
    }

    // update proxy
    centralWidget()->setFocusProxy(m_viewManager->activeView());
}

void KateMainWindow::slotUpdateActionsNeedingUrl()
{
    auto &&view = viewManager()->activeView();
    const bool hasUrl = view && !view->document()->url().isEmpty();

    action(QStringLiteral("file_copy_filepath"))->setEnabled(hasUrl);
    action(QStringLiteral("file_open_containing_folder"))->setEnabled(hasUrl);
    action(QStringLiteral("file_rename"))->setEnabled(hasUrl);
    action(QStringLiteral("file_delete"))->setEnabled(hasUrl);
    action(QStringLiteral("file_properties"))->setEnabled(hasUrl);
    documentOpenWith->setEnabled(hasUrl);
}

void KateMainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (!event->mimeData()) {
        return;
    }
    const bool accept = event->mimeData()->hasUrls() || event->mimeData()->hasText();
    event->setAccepted(accept);
}

void KateMainWindow::dropEvent(QDropEvent *event)
{
    slotDropEvent(event);
}

void KateMainWindow::slotDropEvent(QDropEvent *event)
{
    if (event->mimeData() == nullptr) {
        return;
    }

    //
    // are we dropping files?
    //

    if (event->mimeData()->hasUrls()) {
        QList<QUrl> textlist = event->mimeData()->urls();

        // Try to get the KTextEditor::View that sent this, and activate it, so that the file opens in the
        // view where it was dropped
        KTextEditor::View *kVsender = qobject_cast<KTextEditor::View *>(QObject::sender());
        if (kVsender != nullptr) {
            if (auto parent = kVsender->parent()) {
                KateViewSpace *vs = qobject_cast<KateViewSpace *>(parent->parent());
                if (vs != nullptr) {
                    m_viewManager->setActiveSpace(vs);
                }
            }
        }

        for (const QUrl &url : qAsConst(textlist)) {
            // if url has no file component, try and recursively scan dir
            KFileItem kitem(url);
            kitem.setDelayedMimeTypes(true);
            if (kitem.isDir()) {
                if (KMessageBox::questionTwoActions(this,
                                                    i18n("You dropped the directory %1 into Kate. "
                                                         "Do you want to open all files contained in it?",
                                                         url.url()),
                                                    i18nc("@title:window", "Open Files Recursively"),
                                                    KGuiItem(i18nc("@action:button", "Open All Files"), QStringLiteral("document-open")),
                                                    KStandardGuiItem::cancel())
                    == KMessageBox::PrimaryAction) {
                    KIO::ListJob *list_job = KIO::listRecursive(url, KIO::DefaultFlags, KIO::ListJob::ListFlags{});
                    connect(list_job, &KIO::ListJob::entries, this, &KateMainWindow::slotListRecursiveEntries);
                }
            } else {
                m_viewManager->openUrl(url);
            }
        }
    }
    //
    // or are we dropping text?
    //
    else if (event->mimeData()->hasText()) {
        KTextEditor::Document *doc = KateApp::self()->documentManager()->createDoc();
        doc->setText(event->mimeData()->text());
        m_viewManager->activateView(doc);
    }
}

void KateMainWindow::slotListRecursiveEntries(KIO::Job *job, const KIO::UDSEntryList &list)
{
    const QUrl dir = static_cast<KIO::SimpleJob *>(job)->url();
    for (const KIO::UDSEntry &entry : list) {
        if (!entry.isDir()) {
            QUrl url(dir);
            url = url.adjusted(QUrl::StripTrailingSlash);
            url.setPath(url.path() + QLatin1Char('/') + entry.stringValue(KIO::UDSEntry::UDS_NAME));
            m_viewManager->openUrl(url);
        }
    }
}

void KateMainWindow::editKeys()
{
    KShortcutsDialog dlg(KShortcutsEditor::AllActions, KShortcutsEditor::LetterShortcutsAllowed, this);

    const QList<KXMLGUIClient *> clients = guiFactory()->clients();

    for (KXMLGUIClient *client : clients) {
        // FIXME there appear to be invalid clients after session switching
        //     qCDebug(LOG_KATE)<<"adding client to shortcut editor";
        //     qCDebug(LOG_KATE)<<client;
        //     qCDebug(LOG_KATE)<<client->actionCollection();
        //     qCDebug(LOG_KATE)<<client->componentData().aboutData();
        //     qCDebug(LOG_KATE)<<client->componentData().aboutData()->programName();
        dlg.addCollection(client->actionCollection(), client->componentName());
    }
    dlg.configure();

    // reloadXML gui clients, to ensure all clients are up-to-date
    reloadXmlGui();
}

void KateMainWindow::openUrl(const QString &name)
{
    m_viewManager->openUrl(QUrl(name));
}

void KateMainWindow::slotConfigure()
{
    showPluginConfigPage(nullptr, 0);
}

bool KateMainWindow::showPluginConfigPage(KTextEditor::Plugin *configpageinterface, int id)
{
    KateConfigDialog *dlg = new KateConfigDialog(this);

    if (configpageinterface) {
        dlg->showAppPluginPage(configpageinterface, id);
    }

    if (dlg->exec() == QDialog::Accepted) {
        m_fileOpenRecent->setMaxItems(KateConfigDialog::recentFilesMaxCount());
    }
    delete dlg;

    m_viewManager->replugActiveView(); // gui (toolbars...) needs to be updated, because
    // of possible changes that the configure dialog
    // could have done on it, specially for plugins.

    return true;
}

QUrl KateMainWindow::activeDocumentUrl()
{
    // anders: i make this one safe, as it may be called during
    // startup (by the file selector)
    KTextEditor::View *v = m_viewManager->activeView();
    if (v) {
        return v->document()->url();
    }
    return QUrl();
}

void KateMainWindow::mSlotFixOpenWithMenu()
{
    KTextEditor::View *activeView = m_viewManager->activeView();
    if (!activeView) {
        return;
    }

    KTextEditor::Document *doc = activeView->document();
    if (!doc) {
        return;
    }

    KateFileActions::prepareOpenWithMenu(doc->url(), documentOpenWith->menu());
}

void KateMainWindow::slotOpenWithMenuAction(QAction *a)
{
    auto activeView = m_viewManager->activeView();
    if (!activeView) {
        return;
    }

    auto doc = activeView->document();
    if (!doc) {
        return;
    }

    KateFileActions::showOpenWithMenu(this, doc->url(), a);
}

void KateMainWindow::pluginHelp()
{
    KHelpClient::invokeHelp(QString(), QStringLiteral("kate-plugins"));
}

void KateMainWindow::slotFullScreen(bool t)
{
    KToggleFullScreenAction::setFullScreen(this, t);
    QMenuBar *mb = menuBar();
    if (t) {
        QToolButton *b = new QToolButton(mb);
        b->setDefaultAction(m_showFullScreenAction);
        b->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Ignored));
        b->setFont(QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont));
        mb->setCornerWidget(b, Qt::TopRightCorner);
        b->setVisible(true);
        b->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    } else {
        QWidget *w = mb->cornerWidget(Qt::TopRightCorner);
        if (w) {
            w->deleteLater();
        }
    }
}

bool KateMainWindow::showModOnDiskPrompt(ModOnDiskMode mode)
{
    const auto documents = KateApp::self()->documentManager()->documentList();
    QList<KTextEditor::Document *> list;
    list.reserve(documents.size());
    for (auto doc : documents) {
        if (KateApp::self()->documentManager()->documentInfo(doc)->modifiedOnDisc && (doc->isModified() || mode == PromptAll)) {
            list.append(doc);
        }
    }

    if (!list.isEmpty() && !m_modignore) {
        KateMwModOnHdDialog mhdlg(list, this);
        m_modignore = true;
        bool res = mhdlg.exec();
        m_modignore = false;

        return res;
    }
    return true;
}

void KateMainWindow::slotDocumentCreated(KTextEditor::Document *doc)
{
    connect(doc, &KTextEditor::Document::modifiedChanged, this, QOverload<KTextEditor::Document *>::of(&KateMainWindow::updateCaption));
    connect(doc, &KTextEditor::Document::readWriteChanged, this, QOverload<KTextEditor::Document *>::of(&KateMainWindow::updateCaption));
    connect(doc, &KTextEditor::Document::documentNameChanged, this, QOverload<KTextEditor::Document *>::of(&KateMainWindow::updateCaption));
    connect(doc, &KTextEditor::Document::documentUrlChanged, this, QOverload<KTextEditor::Document *>::of(&KateMainWindow::updateCaption));
    connect(doc, &KTextEditor::Document::documentUrlChanged, this, &KateMainWindow::slotUpdateActionsNeedingUrl);

    updateCaption(doc);
}

void KateMainWindow::updateCaption()
{
    if (m_viewManager->activeView()) {
        updateCaption(m_viewManager->activeView()->document());
    }
}

void KateMainWindow::updateCaption(KTextEditor::Document *doc)
{
    if (!m_viewManager->activeView()) {
        setCaption(QString(), false);
        setWindowFilePath(QString());
        return;
    }

    // block signals from inactive docs
    if (m_viewManager->activeView()->document() != doc) {
        return;
    }

    QString c;
    const auto url = m_viewManager->activeView()->document()->url();
    if (m_viewManager->activeView()->document()->url().isEmpty() || (!m_paShowPath || !m_paShowPath->isChecked())) {
        c = m_viewManager->activeView()->document()->documentName();
    } else {
        // we want some filename @ folder output to have chance to keep important stuff even on elide
        c = Utils::niceFileNameWithPath(url);
    }

    setWindowFilePath(url.toString(QUrl::PreferLocalFile));

    QString sessName = KateApp::self()->sessionManager()->activeSession()->name();
    if (!sessName.isEmpty()) {
        sessName = QStringLiteral("%1: ").arg(sessName);
    }

    QString readOnlyCaption;
    if (!m_viewManager->activeView()->document()->isReadWrite()) {
        readOnlyCaption = i18n(" [read only]");
    }

    setCaption(sessName + c + readOnlyCaption + QStringLiteral(" [*]"), m_viewManager->activeView()->document()->isModified());
}

void KateMainWindow::saveProperties(KConfigGroup &config, bool includeViewConfig)
{
    saveSession(config);

    // store all plugin view states
    int id = KateApp::self()->mainWindowID(this);
    const auto plugins = KateApp::self()->pluginManager()->pluginList();
    for (const KatePluginInfo &item : plugins) {
        if (item.plugin && pluginViews().contains(item.plugin)) {
            if (auto interface = qobject_cast<KTextEditor::SessionConfigInterface *>(pluginViews().value(item.plugin))) {
                KConfigGroup group(config.config(), QStringLiteral("Plugin:%1:MainWindow:%2").arg(item.saveName()).arg(id));
                interface->writeSessionConfig(group);
            }
        }
    }

    saveOpenRecent(config.config());

    // allow to skip the view manager config, this is needed for KWrite, see bug 463139
    if (includeViewConfig) {
        m_viewManager->saveViewConfiguration(config);
    }
}

void KateMainWindow::readProperties(const KConfigGroup &config)
{
    // KDE5: TODO startRestore should take a const KConfigBase*, or even just a const KConfigGroup&,
    // but this propagates down to interfaces/kate/plugin.h so all plugins have to be ported
    KConfigBase *configBase = const_cast<KConfig *>(config.config());
    startRestore(configBase, config.name());

    // perhaps enable plugin guis
    KateApp::self()->pluginManager()->enableAllPluginsGUI(this, configBase);

    finishRestore();

    loadOpenRecent(config.config());
    m_viewManager->restoreViewConfiguration(config);
}

void KateMainWindow::saveOpenRecent(KConfig *config)
{
    m_fileOpenRecent->saveEntries(KConfigGroup(config, QStringLiteral("Recent Files")));
}

void KateMainWindow::loadOpenRecent(const KConfig *config)
{
    m_fileOpenRecent->loadEntries(KConfigGroup(config, QStringLiteral("Recent Files")));
}

void KateMainWindow::saveGlobalProperties(KConfig *sessionConfig)
{
    KateApp::self()->documentManager()->saveDocumentList(sessionConfig);

    KConfigGroup cg(sessionConfig, QStringLiteral("General"));
    cg.writeEntry("Last Session", KateApp::self()->sessionManager()->activeSession()->name());

    // save plugin config !!
    KateApp::self()->pluginManager()->writeConfig(sessionConfig);

    if (m_diagView) {
        KConfigGroup cg(sessionConfig, QStringLiteral("Kate Diagnostics"));
        m_diagView->writeSessionConfig(cg);
    }
}

void KateMainWindow::saveWindowConfig(const KConfigGroup &_config)
{
    KConfigGroup config(_config);
    saveMainWindowSettings(config);
    config.writeEntry("WindowState", static_cast<int>(windowState()));
    config.sync();
}

void KateMainWindow::restoreWindowConfig(const KConfigGroup &config)
{
    setWindowState(Qt::WindowNoState);
    applyMainWindowSettings(config);
    setWindowState(QFlags<Qt::WindowState>(config.readEntry("WindowState", int(Qt::WindowActive))));
}

void KateMainWindow::slotUpdateBottomViewBar()
{
    // get active view if any, if none => just hide bar
    KTextEditor::View *view = m_viewManager->activeView();
    if (!view) {
        if (auto wid = m_bottomContainerStack->currentWidget()) {
            wid->hide();
        }
        m_bottomViewBarContainer->hide();
        return;
    }

    // get bar state, we must have a bar widget here, KateView always creates one!
    BarState &bs = m_bottomViewBarMapping[view];
    Q_ASSERT(bs.bar());

    // extract statusbar if not already done
    if (!bs.statusBar()) {
        // we search for the status bar by class, this MUST work, we ensure that it is enabled in the view
        const auto widgets = bs.bar()->findChildren<QWidget *>(QString(), Qt::FindChildrenRecursively);
        for (auto *w : widgets) {
            if (w && w->metaObject()->className() == QByteArrayLiteral("KateStatusBar")) {
                bs.setStatusBar(w);
            }
        }
        Q_ASSERT(bs.statusBar());

        // ensure we don't mess up the vertical sizing
        bs.statusBar()->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        // add the status bar to our status bar stack, there we will show/hide it
        statusBarStackedWidget()->addWidget(bs.statusBar());
    }

    // always activate the current statusbar
    Q_ASSERT(bs.statusBar()->parent() == statusBarStackedWidget());
    statusBarStackedWidget()->setCurrentWidget(bs.statusBar());

    // hide or show the bar
    if (bs.state()) {
        m_bottomContainerStack->setCurrentWidget(bs.bar());
        m_bottomContainerStack->currentWidget()->show();
        m_bottomViewBarContainer->show();
    } else {
        if (auto wid = m_bottomContainerStack->currentWidget()) {
            wid->hide();
        }
        m_bottomViewBarContainer->hide();
    }
}

void KateMainWindow::queueModifiedOnDisc(KTextEditor::Document *doc)
{
    if (!m_modNotification) {
        return;
    }

    KateDocumentInfo *docInfo = KateApp::self()->documentManager()->documentInfo(doc);
    if (!docInfo) {
        return;
    }
    bool modOnDisk = static_cast<uint>(docInfo->modifiedOnDisc);

    if (s_modOnHdDialog == nullptr && modOnDisk) {
        QList<KTextEditor::Document *> list;
        list.append(doc);

        s_modOnHdDialog = new KateMwModOnHdDialog(list, this);
        m_modignore = true;
        connect(s_modOnHdDialog, &KateMwModOnHdDialog::requestOpenDiffDocument, KateApp::self(), [](const QUrl &url) {
            // use open with isTempFile == true
            KateApp::self()->openDocUrl(url, QString(), true);
        });

        // Someone modified a doc outside and now we are here
        // but Kate isn't the active app. Delay the dialog exec
        // otherwise it will bring us front interrupting user's
        // work.
        if (qApp->applicationState() != Qt::ApplicationActive) {
            m_modignore = false;
            s_modOnHdDialog->setShowOnWindowActivation(true);
            // hopefully this shows an alert to the user in task bar
            // that something changed in Kate
            qApp->alert(this, 3000);
            return;
        }

        s_modOnHdDialog->exec();
        delete s_modOnHdDialog; // s_modOnHdDialog is set to 0 in destructor of KateMwModOnHdDialog (jowenn!!!)
        m_modignore = false;
    } else if (s_modOnHdDialog != nullptr) {
        s_modOnHdDialog->addDocument(doc);
    }
}

bool KateMainWindow::event(QEvent *e)
{
    if (e->type() == QEvent::ShortcutOverride) {
        QKeyEvent *k = static_cast<QKeyEvent *>(e);
        Q_EMIT unhandledShortcutOverride(k);

        if (KateApp::isKate() && k->key() == Qt::Key_Escape && k->modifiers() == Qt::NoModifier) {
            if (!m_toolViewOutput->isHidden()) {
                hideToolView(m_toolViewOutput);
            }
        }
    }

    return KateMDI::MainWindow::event(e);
}

QObject *KateMainWindow::pluginView(const QString &name)
{
    KTextEditor::Plugin *plugin = KateApp::self()->pluginManager()->plugin(name);
    if (!plugin) {
        return nullptr;
    }

    return m_pluginViews.contains(plugin) ? m_pluginViews.value(plugin) : nullptr;
}

bool KateMainWindow::addWidget(QWidget *widget)
{
    if (!widget) {
        qWarning() << Q_FUNC_INFO << "Unexpected null widget!";
        return false;
    }

    auto vs = m_viewManager->activeViewSpace();
    vs->addWidgetAsTab(widget);
    Q_EMIT widgetAdded(widget);
    m_viewManager->activateView(widget);
    return true;
}

bool KateMainWindow::removeWidget(QWidget *widget)
{
    return m_viewManager->removeWidget(widget);
}

QWidget *KateMainWindow::activeWidget()
{
    auto vs = m_viewManager->activeViewSpace();
    if (auto w = vs->currentWidget()) {
        return w;
    }
    return activeView();
}

void KateMainWindow::activateWidget(QWidget *widget)
{
    if (!m_viewManager->activateWidget(widget)) {
        addWidget(widget);
    }
}

void KateMainWindow::showMessage(const QVariantMap &map)
{
    if (!m_outputView) {
        return;
    }
    m_outputView->slotMessage(map);
}

void KateMainWindow::addPositionToHistory(const QUrl &url, KTextEditor::Cursor c)
{
    m_viewManager->addPositionToHistory(url, c);
}

QWidgetList KateMainWindow::widgets() const
{
    return m_viewManager->widgets();
}

void KateMainWindow::mousePressEvent(QMouseEvent *e)
{
    switch (e->button()) {
    case Qt::ForwardButton:
        handleForwardButtonAction();
        break;
    case Qt::BackButton:
        handleBackButtonAction();
        break;
    default:;
    }
}

void KateMainWindow::slotFocusPrevTab()
{
    if (m_viewManager->activeViewSpace()) {
        m_viewManager->activeViewSpace()->focusPrevTab();
    }
}

void KateMainWindow::slotFocusNextTab()
{
    if (m_viewManager->activeViewSpace()) {
        m_viewManager->activeViewSpace()->focusNextTab();
    }
}

void KateMainWindow::handleBackButtonAction()
{
    if (m_viewManager->activeViewSpace()) {
        switch (m_mouseButtonBackAction) {
        case PreviousTab:
            m_viewManager->activeViewSpace()->focusPrevTab();
            break;
        case HistoryBack:
            m_viewManager->activeViewSpace()->goBack();
            break;
        default:;
        }
    }
}

void KateMainWindow::handleForwardButtonAction()
{
    if (m_viewManager->activeViewSpace()) {
        switch (m_mouseButtonForwardAction) {
        case NextTab:
            m_viewManager->activeViewSpace()->focusNextTab();
            break;
        case HistoryForward:
            m_viewManager->activeViewSpace()->goForward();
            break;
        default:;
        }
    }
}

void KateMainWindow::slotQuickOpen()
{
    /**
     * show quick open and pass focus to it
     */
    KateQuickOpen *quickOpen = new KateQuickOpen(this);
    centralWidget()->setFocusProxy(quickOpen);
    quickOpen->raise();
    quickOpen->show();
}

QWidget *KateMainWindow::createToolView(KTextEditor::Plugin *plugin,
                                        const QString &identifier,
                                        KTextEditor::MainWindow::ToolViewPosition pos,
                                        const QIcon &icon,
                                        const QString &text)
{
    return KateMDI::MainWindow::createToolView(plugin, identifier, static_cast<KMultiTabBar::KMultiTabBarPosition>(pos), icon, text);
}

bool KateMainWindow::moveToolView(QWidget *widget, KTextEditor::MainWindow::ToolViewPosition pos)
{
    if (!qobject_cast<KateMDI::ToolView *>(widget)) {
        return false;
    }

    return KateMDI::MainWindow::moveToolView(qobject_cast<KateMDI::ToolView *>(widget), static_cast<KMultiTabBar::KMultiTabBarPosition>(pos));
}

bool KateMainWindow::showToolView(QWidget *widget)
{
    if (!qobject_cast<KateMDI::ToolView *>(widget)) {
        return false;
    }

    return KateMDI::MainWindow::showToolView(qobject_cast<KateMDI::ToolView *>(widget));
}

bool KateMainWindow::hideToolView(QWidget *widget)
{
    if (!qobject_cast<KateMDI::ToolView *>(widget)) {
        return false;
    }

    return KateMDI::MainWindow::hideToolView(qobject_cast<KateMDI::ToolView *>(widget));
}

void KateMainWindow::addRecentOpenedFile(const QUrl &url)
{
    // skip non-existing urls for untitled documents
    if (url.isEmpty()) {
        return;
    }
    // skip files in /subltmp
    if (url.path().startsWith(QDir::tempPath())) {
        return;
    }

    // to our local list, aka menu
    m_fileOpenRecent->addUrl(url);

    /** FIXME Disabled because this can be too slow 100ms/doc sometimes,
     renable when it is 0/ms again*/
    // to the global "Recent Document Menu", see bug 420504
    // KRecentDocument::add(url);
}

void KateMainWindow::onApplicationStateChanged(Qt::ApplicationState)
{
    // After queueModifiedOnDisc, the app wasn't active but now it got
    // active. Show the dialog.
    if (s_modOnHdDialog && s_modOnHdDialog->showOnWindowActivation() && qApp->applicationState() == Qt::ApplicationActive) {
        // Must set this to false, we want only one exec to happen.
        s_modOnHdDialog->setShowOnWindowActivation(false);
        // do it on next event loop iteration to avoid blocking here
        QTimer::singleShot(1, this, [] {
            s_modOnHdDialog->exec();
            delete s_modOnHdDialog;
        });
    }
}

#include "moc_katemainwindow.cpp"
