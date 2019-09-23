/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>
   Copyright (C) 2007 Flavio Castelli <flavio.castelli@gmail.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

// BEGIN Includes
#include "katemainwindow.h"

#include "kateconfigdialog.h"
#include "katedocmanager.h"
#include "katepluginmanager.h"
#include "kateconfigplugindialogpage.h"
#include "kateviewmanager.h"
#include "kateapp.h"
#include "katesavemodifieddialog.h"
#include "katemwmodonhddialog.h"
#include "katesessionsaction.h"
#include "katesessionmanager.h"
#include "kateviewspace.h"
#include "katequickopen.h"
#include "kateupdatedisabler.h"
#include "katedebug.h"
#include "katecolorschemechooser.h"
#include "katefileactions.h"
#include "katequickopenmodel.h"

#include <KActionMenu>
#include <KAboutApplicationDialog>
#include <KEditToolBar>
#include <KShortcutsDialog>
#include <KMessageBox>
#include <KOpenWithDialog>
#include <KStandardAction>
#include <KMimeTypeTrader>
#include <KMultiTabBar>
#include <khelpclient.h>
#include <KRun>
#include <KRecentFilesAction>
#include <KToggleFullScreenAction>
#include <KActionCollection>
#include <KAboutData>
#include <KWindowSystem>
#include <KToolBar>
#include <KLocalizedString>
#include <KConfigGroup>
#include <KSharedConfig>
#include <kwindowconfig.h>
#include <KXMLGUIFactory>

#include <KIO/Job>
#include <KFileItem>

#include <QDesktopWidget>
#include <QEvent>
#include <QList>
#include <QMimeData>
#include <QMimeDatabase>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QApplication>
#include <QMenu>
#include <QMenuBar>
#include <QToolButton>
#include <QTimer>
#include <QFontDatabase>
#include <QDir>

#include <ktexteditor/sessionconfiginterface.h>

// END

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

KateMainWindow::KateMainWindow(KConfig *sconfig, const QString &sgroup)
    : KateMDI::MainWindow(nullptr)
    , m_modignore(false)
    , m_wrapper(new KTextEditor::MainWindow(this))
{
    /**
     * we don't want any flicker here
     */
    KateUpdateDisabler disableUpdates(this);

    /**
     * get and set config revision
     */
    static const int currentConfigRevision = 10;
    const int readConfigRevision = KConfigGroup(KSharedConfig::openConfig(), "General").readEntry("Config Revision", 0);
    KConfigGroup(KSharedConfig::openConfig(), "General").writeEntry("Config Revision", currentConfigRevision);
    const bool firstStart = readConfigRevision < currentConfigRevision;

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

    // qCDebug(LOG_KATE) << "****************************************************************************" << sconfig;

    // register mainwindow in app
    KateApp::self()->addMainWindow(this);

    // enable plugin guis
    KateApp::self()->pluginManager()->enableAllPluginsGUI(this, sconfig);

    // caption update
    Q_FOREACH (auto doc, KateApp::self()->documentManager()->documentList()) {
        slotDocumentCreated(doc);
    }

    connect(KateApp::self()->documentManager(), &KateDocManager::documentCreated, this, &KateMainWindow::slotDocumentCreated);

    readOptions();

    if (sconfig) {
        m_viewManager->restoreViewConfiguration(KConfigGroup(sconfig, sgroup));
    }

    finishRestore();

    m_fileOpenRecent->loadEntries(KConfigGroup(sconfig, "Recent Files"));

    setAcceptDrops(true);

    connect(KateApp::self()->sessionManager(), SIGNAL(sessionChanged()), this, SLOT(updateCaption()));

    connect(this, &KateMDI::MainWindow::sigShowPluginConfigPage, this, &KateMainWindow::showPluginConfigPage);

    // prior to this there was (possibly) no view, therefore not context menu.
    // Hence, we have to take care of the menu bar here
    toggleShowMenuBar(false);

    // on first start: deactivate toolbar
    if (firstStart)
        toolBar(QStringLiteral("mainToolBar"))->hide();

    // in all cases: avoid that arbitrary plugin toolviews get focus, like terminal, bug 412227
    // we need to delay this a bit due to lazy view creation (and lazy e.g. terminal widget creation)
    QTimer::singleShot(0, centralWidget(), SLOT(setFocus()));
}

KateMainWindow::~KateMainWindow()
{
    // first, save our fallback window size ;)
    KConfigGroup cfg(KSharedConfig::openConfig(), "MainWindow");
    KWindowConfig::saveWindowSize(windowHandle(), cfg);

    // save other options ;=)
    saveOptions();

    // unregister mainwindow in app
    KateApp::self()->removeMainWindow(this);

    // disable all plugin guis, delete all pluginViews
    KateApp::self()->pluginManager()->disableAllPluginsGUI(this);

    // manually delete quick open, it's event filters will cause crash otherwise later
    delete m_quickOpen;
    m_quickOpen = nullptr;

    // delete the view manager, before KateMainWindow's wrapper is dead
    delete m_viewManager;
    m_viewManager = nullptr;

    // kill the wrapper object, now that all views are dead
    delete m_wrapper;
    m_wrapper = nullptr;
}

QSize KateMainWindow::sizeHint() const
{
    /**
     * have some useful size hint, else we have mini windows per default
     */
    return (QSize(640, 480).expandedTo(minimumSizeHint()));
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

    // Load themes
    actionCollection()->addAction(QStringLiteral("colorscheme_menu"), new KateColorSchemeChooser(actionCollection()));

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
    actionCollection()->setDefaultShortcut(a, QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_O));
    connect(a, &QAction::triggered, this, &KateMainWindow::slotQuickOpen);
    a->setWhatsThis(i18n("Open a form to quick open documents."));
}

void KateMainWindow::setupMainWindow()
{
    setToolViewStyle(KMultiTabBar::KDEV3ICON);

    /**
     * create central stacked widget with its children
     */
    m_mainStackedWidget = new QStackedWidget(centralWidget());
    centralWidget()->layout()->addWidget(m_mainStackedWidget);
    (static_cast<QBoxLayout *>(centralWidget()->layout()))->setStretchFactor(m_mainStackedWidget, 100);

    m_quickOpen = new KateQuickOpen(m_mainStackedWidget, this);
    m_mainStackedWidget->addWidget(m_quickOpen);

    m_viewManager = new KateViewManager(m_mainStackedWidget, this);
    m_mainStackedWidget->addWidget(m_viewManager);

    // make view manager default visible!
    m_mainStackedWidget->setCurrentWidget(m_viewManager);

    m_bottomViewBarContainer = new QWidget(centralWidget());
    centralWidget()->layout()->addWidget(m_bottomViewBarContainer);
    m_bottomContainerStack = new KateContainerStackedLayout(m_bottomViewBarContainer);
}

void KateMainWindow::setupActions()
{
    QAction *a;

    actionCollection()->addAction(KStandardAction::New, QStringLiteral("file_new"), m_viewManager, SLOT(slotDocumentNew()))->setWhatsThis(i18n("Create a new document"));
    actionCollection()->addAction(KStandardAction::Open, QStringLiteral("file_open"), m_viewManager, SLOT(slotDocumentOpen()))->setWhatsThis(i18n("Open an existing document for editing"));

    m_fileOpenRecent = KStandardAction::openRecent(m_viewManager, SLOT(openUrl(QUrl)), this);
    m_fileOpenRecent->setMaxItems(KateConfigDialog::recentFilesMaxCount());
    actionCollection()->addAction(m_fileOpenRecent->objectName(), m_fileOpenRecent);
    m_fileOpenRecent->setWhatsThis(i18n("This lists files which you have opened recently, and allows you to easily open them again."));

    a = actionCollection()->addAction(QStringLiteral("file_save_all"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("document-save-all")));
    a->setText(i18n("Save A&ll"));
    actionCollection()->setDefaultShortcut(a, QKeySequence(Qt::CTRL + Qt::Key_L));
    connect(a, &QAction::triggered, KateApp::self()->documentManager(), &KateDocManager::saveAll);
    a->setWhatsThis(i18n("Save all open, modified documents to disk."));

    a = actionCollection()->addAction(QStringLiteral("file_reload_all"));
    a->setText(i18n("&Reload All"));
    connect(a, &QAction::triggered, KateApp::self()->documentManager(), &KateDocManager::reloadAll);
    a->setWhatsThis(i18n("Reload all open documents."));

    a = actionCollection()->addAction(QStringLiteral("file_copy_filepath"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("edit-copy")));
    a->setText(i18n("Copy File &Path"));
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
        KateFileActions::openFilePropertiesDialog(view->document());
    });
    a->setWhatsThis(i18n("Deletes the file belonging to the current document."));

    a = actionCollection()->addAction(QStringLiteral("file_compare"));
    a->setText(i18n("Compare"));
    connect(a, &QAction::triggered, KateApp::self()->documentManager(), [this]() { QMessageBox::information(this, i18n("Compare"), i18n("Use the Tabbar context menu to compare two documents")); });
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
    connect(a, &QAction::triggered, this, &KateMainWindow::newWindow);
    a->setWhatsThis(i18n("Create a new Kate view (a new window with the same document list)."));

    m_showFullScreenAction = KStandardAction::fullScreen(nullptr, nullptr, this, this);
    actionCollection()->addAction(m_showFullScreenAction->objectName(), m_showFullScreenAction);
    connect(m_showFullScreenAction, &QAction::toggled, this, &KateMainWindow::slotFullScreen);

    documentOpenWith = new KActionMenu(i18n("Open W&ith"), this);
    actionCollection()->addAction(QStringLiteral("file_open_with"), documentOpenWith);
    documentOpenWith->setWhatsThis(i18n("Open the current document using another application registered for its file type, or an application of your choice."));
    connect(documentOpenWith->menu(), &QMenu::aboutToShow, this, &KateMainWindow::mSlotFixOpenWithMenu);
    connect(documentOpenWith->menu(), &QMenu::triggered, this, &KateMainWindow::slotOpenWithMenuAction);

    a = KStandardAction::keyBindings(this, SLOT(editKeys()), actionCollection());
    a->setWhatsThis(i18n("Configure the application's keyboard shortcut assignments."));

    a = KStandardAction::configureToolbars(this, SLOT(slotEditToolbars()), actionCollection());
    a->setWhatsThis(i18n("Configure which items should appear in the toolbar(s)."));

    QAction *settingsConfigure = KStandardAction::preferences(this, SLOT(slotConfigure()), actionCollection());
    settingsConfigure->setWhatsThis(i18n("Configure various aspects of this application and the editing component."));

    if (KateApp::self()->pluginManager()->pluginList().count() > 0) {
        a = actionCollection()->addAction(QStringLiteral("help_plugins_contents"));
        a->setText(i18n("&Plugins Handbook"));
        connect(a, &QAction::triggered, this, &KateMainWindow::pluginHelp);
        a->setWhatsThis(i18n("This shows help files for various available plugins."));
    }

    a = actionCollection()->addAction(QStringLiteral("help_about_editor"));
    a->setText(i18n("&About Editor Component"));
    connect(a, &QAction::triggered, this, &KateMainWindow::aboutEditor);

    connect(m_viewManager, &KateViewManager::viewChanged, this, &KateMainWindow::slotWindowActivated);
    connect(m_viewManager, &KateViewManager::viewChanged, this, &KateMainWindow::slotUpdateOpenWith);
    connect(m_viewManager, &KateViewManager::viewChanged, this, &KateMainWindow::slotUpdateActionsNeedingUrl);
    connect(m_viewManager, &KateViewManager::viewChanged, this, &KateMainWindow::slotUpdateBottomViewBar);

    // re-route signals to our wrapper
    connect(m_viewManager, &KateViewManager::viewChanged, m_wrapper, &KTextEditor::MainWindow::viewChanged);
    connect(m_viewManager, &KateViewManager::viewCreated, m_wrapper, &KTextEditor::MainWindow::viewCreated);
    connect(this, &KateMainWindow::unhandledShortcutOverride, m_wrapper, &KTextEditor::MainWindow::unhandledShortcutOverride);

    slotWindowActivated();

    // session actions
    a = actionCollection()->addAction(QStringLiteral("sessions_new"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("document-new")));
    a->setText(i18nc("Menu entry Session->New", "&New"));
    // Qt::QueuedConnection to avoid deletion of code that is executed when reducing the amount of mainwindows. (bug #227008)
    connect(a, &QAction::triggered, KateApp::self()->sessionManager(), &KateSessionManager::sessionNew, Qt::QueuedConnection);
    a = actionCollection()->addAction(QStringLiteral("sessions_save"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("document-save")));
    a->setText(i18n("&Save Session"));
    connect(a, &QAction::triggered, KateApp::self()->sessionManager(), &KateSessionManager::sessionSave);
    a = actionCollection()->addAction(QStringLiteral("sessions_save_as"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("document-save-as")));
    a->setText(i18n("Save Session &As..."));
    connect(a, &QAction::triggered, KateApp::self()->sessionManager(), &KateSessionManager::sessionSaveAs);
    a = actionCollection()->addAction(QStringLiteral("sessions_manage"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("view-choose")));
    a->setText(i18n("&Manage Sessions..."));
    // Qt::QueuedConnection to avoid deletion of code that is executed when reducing the amount of mainwindows. (bug #227008)
    connect(a, &QAction::triggered, KateApp::self()->sessionManager(), &KateSessionManager::sessionManage, Qt::QueuedConnection);

    // quick open menu ;)
    a = new KateSessionsAction(i18n("&Quick Open Session"), this);
    actionCollection()->addAction(QStringLiteral("sessions_list"), a);
}

void KateMainWindow::slotDocumentCloseAll()
{
    if (!KateApp::self()->documentManager()->documentList().empty() &&
        KMessageBox::warningContinueCancel(
            this, i18n("This will close all open documents. Are you sure you want to continue?"), i18n("Close all documents"), KStandardGuiItem::cont(), KStandardGuiItem::cancel(), QStringLiteral("closeAll")) != KMessageBox::Cancel) {
        if (queryClose_internal()) {
            KateApp::self()->documentManager()->closeAllDocuments(false);
        }
    }
}

void KateMainWindow::slotDocumentCloseOther(KTextEditor::Document *document)
{
    if (KateApp::self()->documentManager()->documentList().size() > 1 &&
        KMessageBox::warningContinueCancel(this,
                                           i18n("This will close all open documents beside the current one. Are you sure you want to continue?"),
                                           i18n("Close all documents beside current one"),
                                           KStandardGuiItem::cont(),
                                           KStandardGuiItem::cancel(),
                                           QStringLiteral("closeOther")) != KMessageBox::Cancel) {
        if (queryClose_internal(document)) {
            KateApp::self()->documentManager()->closeOtherDocuments(document);
        }
    }
}

void KateMainWindow::slotDocumentCloseSelected(const QList<KTextEditor::Document *> &docList)
{
    QList<KTextEditor::Document *> documents;
    foreach (KTextEditor::Document *doc, docList) {
        if (queryClose_internal(doc)) {
            documents.append(doc);
        }
    }

    KateApp::self()->documentManager()->closeDocuments(documents);
}

void KateMainWindow::slotDocumentCloseOther()
{
    slotDocumentCloseOther(m_viewManager->activeView()->document());
}

bool KateMainWindow::queryClose_internal(KTextEditor::Document *doc)
{
    int documentCount = KateApp::self()->documentManager()->documentList().size();

    if (!showModOnDiskPrompt()) {
        return false;
    }

    QList<KTextEditor::Document *> modifiedDocuments = KateApp::self()->documentManager()->modifiedDocumentList();
    modifiedDocuments.removeAll(doc);
    bool shutdown = (modifiedDocuments.count() == 0);

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
    // session saving, can we close all views ?
    // just test, not close them actually
    if (qApp->isSavingSession()) {
        return queryClose_internal();
    }

    // normal closing of window
    // allow to close all windows until the last without restrictions
    if (KateApp::self()->mainWindowsCount() > 1) {
        return true;
    }

    // last one: check if we can close all documents, try run
    // and save docs if we really close down !
    if (queryClose_internal()) {
        KateApp::self()->sessionManager()->saveActiveSession(true);
        return true;
    }

    return false;
}

void KateMainWindow::newWindow()
{
    KateApp::self()->newMainWindow(KateApp::self()->sessionManager()->activeSession()->config());
}

void KateMainWindow::slotEditToolbars()
{
    KConfigGroup cfg(KSharedConfig::openConfig(), "MainWindow");
    saveMainWindowSettings(cfg);

    KEditToolBar dlg(factory());

    connect(&dlg, &KEditToolBar::newToolBarConfig, this, &KateMainWindow::slotNewToolbarConfig);
    dlg.exec();
}

void KateMainWindow::reloadXmlGui()
{
    for (KTextEditor::Document *doc : KateApp::self()->documentManager()->documentList()) {
        doc->reloadXML();
        for (KTextEditor::View *view : doc->views()) {
            view->reloadXML();
        }
    }
}

void KateMainWindow::slotNewToolbarConfig()
{
    applyMainWindowSettings(KConfigGroup(KSharedConfig::openConfig(), "MainWindow"));

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
    m_viewManager->openUrl(url, QString(), true, false);
}

void KateMainWindow::readOptions()
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig();

    const KConfigGroup generalGroup(config, "General");
    m_modNotification = generalGroup.readEntry("Modified Notification", false);
    m_modCloseAfterLast = generalGroup.readEntry("Close After Last", false);
    KateApp::self()->documentManager()->setSaveMetaInfos(generalGroup.readEntry("Save Meta Infos", true));
    KateApp::self()->documentManager()->setDaysMetaInfos(generalGroup.readEntry("Days Meta Infos", 30));

    m_paShowPath->setChecked(generalGroup.readEntry("Show Full Path in Title", false));
    m_paShowStatusBar->setChecked(generalGroup.readEntry("Show Status Bar", true));
    m_paShowMenuBar->setChecked(generalGroup.readEntry("Show Menu Bar", true));
    m_paShowTabBar->setChecked(generalGroup.readEntry("Show Tab Bar", true));

    m_quickOpen->setMatchMode(generalGroup.readEntry("Quick Open Search Mode", (int)KateQuickOpenModel::Columns::FileName));
    int listMode = generalGroup.readEntry("Quick Open List Mode", (int)KateQuickOpenModel::List::CurrentProject);
    m_quickOpen->setListMode(static_cast<KateQuickOpenModel::List>(listMode));

    // emit signal to hide/show statusbars
    toggleShowStatusBar();
    toggleShowTabBar();
}

void KateMainWindow::saveOptions()
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig();

    KConfigGroup generalGroup(config, "General");

    generalGroup.writeEntry("Save Meta Infos", KateApp::self()->documentManager()->getSaveMetaInfos());

    generalGroup.writeEntry("Days Meta Infos", KateApp::self()->documentManager()->getDaysMetaInfos());

    generalGroup.writeEntry("Show Full Path in Title", m_paShowPath->isChecked());
    generalGroup.writeEntry("Show Status Bar", m_paShowStatusBar->isChecked());
    generalGroup.writeEntry("Show Menu Bar", m_paShowMenuBar->isChecked());
    generalGroup.writeEntry("Show Tab Bar", m_paShowTabBar->isChecked());
}

void KateMainWindow::toggleShowMenuBar(bool showMessage)
{
    if (m_paShowMenuBar->isChecked()) {
        menuBar()->show();
        removeMenuBarActionFromContextMenu();
    } else {
        if (showMessage) {
            const QString accel = m_paShowMenuBar->shortcut().toString();
            KMessageBox::information(this,
                                     i18n("This will hide the menu bar completely."
                                          " You can show it again by typing %1.",
                                          accel),
                                     i18n("Hide menu bar"),
                                     QStringLiteral("HideMenuBarWarning"));
        }
        menuBar()->hide();
        addMenuBarActionToContextMenu();
    }
}

void KateMainWindow::addMenuBarActionToContextMenu()
{
    if (m_viewManager->activeView()) {
        m_viewManager->activeView()->contextMenu()->addAction(m_paShowMenuBar);
    }
}

void KateMainWindow::removeMenuBarActionFromContextMenu()
{
    if (m_viewManager->activeView()) {
        m_viewManager->activeView()->contextMenu()->removeAction(m_paShowMenuBar);
    }
}

void KateMainWindow::toggleShowStatusBar()
{
    emit statusBarToggled();
}

bool KateMainWindow::showStatusBar()
{
    return m_paShowStatusBar->isChecked();
}

void KateMainWindow::toggleShowTabBar()
{
    emit tabBarToggled();
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

    // show view manager in any case
    if (m_mainStackedWidget->currentWidget() != m_viewManager) {
        m_mainStackedWidget->setCurrentWidget(m_viewManager);
    }

    // update proxy
    centralWidget()->setFocusProxy(m_viewManager->activeView());
}

void KateMainWindow::slotUpdateOpenWith()
{
    if (m_viewManager->activeView()) {
        documentOpenWith->setEnabled(!m_viewManager->activeView()->document()->url().isEmpty());
    } else {
        documentOpenWith->setEnabled(false);
    }
}

void KateMainWindow::slotUpdateActionsNeedingUrl()
{
    auto &&view = viewManager()->activeView();
    const bool hasUrl = view && !view->document()->url().isEmpty();

    action("file_copy_filepath")->setEnabled(hasUrl);
    action("file_open_containing_folder")->setEnabled(hasUrl);
    action("file_rename")->setEnabled(hasUrl);
    action("file_delete")->setEnabled(hasUrl);
    action("file_properties")->setEnabled(hasUrl);
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
            QWidget *parent = kVsender->parentWidget();
            if (parent != nullptr) {
                KateViewSpace *vs = qobject_cast<KateViewSpace *>(parent->parentWidget());
                if (vs != nullptr) {
                    m_viewManager->setActiveSpace(vs);
                }
            }
        }

        foreach (const QUrl &url, textlist) {
            // if url has no file component, try and recursively scan dir
            KFileItem kitem(url);
            kitem.setDelayedMimeTypes(true);
            if (kitem.isDir()) {
                if (KMessageBox::questionYesNo(this,
                                               i18n("You dropped the directory %1 into Kate. "
                                                    "Do you want to load all files contained in it ?",
                                                    url.url()),
                                               i18n("Load files recursively?")) == KMessageBox::Yes) {
                    KIO::ListJob *list_job = KIO::listRecursive(url, KIO::DefaultFlags, false);
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
    foreach (const KIO::UDSEntry &entry, list) {
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

    QList<KXMLGUIClient *> clients = guiFactory()->clients();

    foreach (KXMLGUIClient *client, clients) {
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

void KateMainWindow::showPluginConfigPage(KTextEditor::Plugin *configpageinterface, uint id)
{
    if (!m_viewManager->activeView()) {
        return;
    }

    KateConfigDialog *dlg = new KateConfigDialog(this, m_viewManager->activeView());
    if (configpageinterface) {
        dlg->showAppPluginPage(configpageinterface, id);
    }

    if (dlg->exec() == QDialog::Accepted) {
        m_fileOpenRecent->setMaxItems(KateConfigDialog::recentFilesMaxCount());
    }

    delete dlg;

    m_viewManager->reactivateActiveView(); // gui (toolbars...) needs to be updated, because
    // of possible changes that the configure dialog
    // could have done on it, specially for plugins.
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
    // dh: in bug #307699, this slot is called when launching the Kate application
    // unfortunately, no one ever could reproduce except users.
    KTextEditor::View *activeView = m_viewManager->activeView();
    if (!activeView) {
        return;
    }

    // cleanup menu
    QMenu *menu = documentOpenWith->menu();
    menu->clear();

    // get a list of appropriate services.
    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForName(activeView->document()->mimeType());
    // qCDebug(LOG_KATE) << "mime type: " << mime.name();

    QAction *a = nullptr;
    const KService::List offers = KMimeTypeTrader::self()->query(mime.name(), QStringLiteral("Application"));
    // add all default open-with-actions except "Kate"
    for (const auto &service : offers) {
        if (service->name() == QLatin1String("Kate")) {
            continue;
        }
        a = menu->addAction(QIcon::fromTheme(service->icon()), service->name());
        a->setData(service->entryPath());
    }
    // append "Other..." to call the KDE "open with" dialog.
    a = documentOpenWith->menu()->addAction(i18n("&Other..."));
    a->setData(QString());
}

void KateMainWindow::slotOpenWithMenuAction(QAction *a)
{
    QList<QUrl> list;
    list.append(m_viewManager->activeView()->document()->url());

    const QString openWith = a->data().toString();
    if (openWith.isEmpty()) {
        // display "open with" dialog
        KOpenWithDialog dlg(list);
        if (dlg.exec()) {
            KRun::runService(*dlg.service(), list, this);
        }
        return;
    }

    KService::Ptr app = KService::serviceByDesktopPath(openWith);
    if (app) {
        KRun::runService(*app, list, this);
    } else {
        KMessageBox::error(this, i18n("Application '%1' not found.", openWith), i18n("Application not found"));
    }
}

void KateMainWindow::pluginHelp()
{
    KHelpClient::invokeHelp(QString(), QStringLiteral("kate-plugins"));
}

void KateMainWindow::aboutEditor()
{
    KAboutApplicationDialog ad(KTextEditor::Editor::instance()->aboutData(), this);
    ad.exec();
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
        if (w)
            w->deleteLater();
    }
}

bool KateMainWindow::showModOnDiskPrompt()
{
    KTextEditor::Document *doc;

    DocVector list;
    list.reserve(KateApp::self()->documentManager()->documentList().size());
    foreach (doc, KateApp::self()->documentManager()->documentList()) {
        if (KateApp::self()->documentManager()->documentInfo(doc)->modifiedOnDisc && doc->isModified()) {
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
    connect(doc, SIGNAL(modifiedChanged(KTextEditor::Document *)), this, SLOT(updateCaption(KTextEditor::Document *)));
    connect(doc, SIGNAL(readWriteChanged(KTextEditor::Document *)), this, SLOT(updateCaption(KTextEditor::Document *)));
    connect(doc, SIGNAL(documentNameChanged(KTextEditor::Document *)), this, SLOT(updateCaption(KTextEditor::Document *)));
    connect(doc, SIGNAL(documentUrlChanged(KTextEditor::Document *)), this, SLOT(updateCaption(KTextEditor::Document *)));
    connect(doc, &KTextEditor::Document::documentNameChanged, this, &KateMainWindow::slotUpdateOpenWith);

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
        return;
    }

    // block signals from inactive docs
    if (!((KTextEditor::Document *)m_viewManager->activeView()->document() == doc)) {
        return;
    }

    QString c;
    if (m_viewManager->activeView()->document()->url().isEmpty() || (!m_paShowPath || !m_paShowPath->isChecked())) {
        c = ((KTextEditor::Document *)m_viewManager->activeView()->document())->documentName();
    } else {
        c = m_viewManager->activeView()->document()->url().toString(QUrl::PreferLocalFile);

        const QString homePath = QDir::homePath();
        if (c.startsWith(homePath)) {
            c = QLatin1String("~") + c.right(c.length() - homePath.length());
        }
    }

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

void KateMainWindow::saveProperties(KConfigGroup &config)
{
    saveSession(config);

    // store all plugin view states
    int id = KateApp::self()->mainWindowID(this);
    foreach (const KatePluginInfo &item, KateApp::self()->pluginManager()->pluginList()) {
        if (item.plugin && pluginViews().contains(item.plugin)) {
            if (auto interface = qobject_cast<KTextEditor::SessionConfigInterface *>(pluginViews().value(item.plugin))) {
                KConfigGroup group(config.config(), QStringLiteral("Plugin:%1:MainWindow:%2").arg(item.saveName()).arg(id));
                interface->writeSessionConfig(group);
            }
        }
    }

    saveOpenRecent(config.config());
    m_viewManager->saveViewConfiguration(config);
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
    m_fileOpenRecent->saveEntries(KConfigGroup(config, "Recent Files"));
}

void KateMainWindow::loadOpenRecent(const KConfig *config)
{
    m_fileOpenRecent->loadEntries(KConfigGroup(config, "Recent Files"));
}

void KateMainWindow::saveGlobalProperties(KConfig *sessionConfig)
{
    KateApp::self()->documentManager()->saveDocumentList(sessionConfig);

    KConfigGroup cg(sessionConfig, "General");
    cg.writeEntry("Last Session", KateApp::self()->sessionManager()->activeSession()->name());

    // save plugin config !!
    KateApp::self()->pluginManager()->writeConfig(sessionConfig);
}

void KateMainWindow::saveWindowConfig(const KConfigGroup &_config)
{
    KConfigGroup config(_config);
    saveMainWindowSettings(config);
    KWindowConfig::saveWindowSize(windowHandle(), config);
    config.writeEntry("WindowState", int(((KParts::MainWindow *)this)->windowState()));
    config.sync();
}

void KateMainWindow::restoreWindowConfig(const KConfigGroup &config)
{
    setWindowState(Qt::WindowNoState);
    applyMainWindowSettings(config);
    KWindowConfig::restoreWindowSize(windowHandle(), config);
    setWindowState(QFlags<Qt::WindowState>(config.readEntry("WindowState", int(Qt::WindowActive))));
}

void KateMainWindow::slotUpdateBottomViewBar()
{
    // qCDebug(LOG_KATE)<<"slotUpdateHorizontalViewBar()"<<endl;
    KTextEditor::View *view = m_viewManager->activeView();
    BarState bs = m_bottomViewBarMapping[view];
    if (bs.bar() && bs.state()) {
        m_bottomContainerStack->setCurrentWidget(bs.bar());
        m_bottomContainerStack->currentWidget()->show();
        m_bottomViewBarContainer->show();
    } else {
        QWidget *wid = m_bottomContainerStack->currentWidget();
        if (wid) {
            wid->hide();
        }
        // qCDebug(LOG_KATE)<<wid<<"hiding container"<<endl;
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
    bool modOnDisk = (uint)docInfo->modifiedOnDisc;

    if (s_modOnHdDialog == nullptr && modOnDisk) {
        DocVector list;
        list.append(doc);

        s_modOnHdDialog = new KateMwModOnHdDialog(list, this);
        m_modignore = true;
        KWindowSystem::setOnAllDesktops(s_modOnHdDialog->winId(), true);
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
        emit unhandledShortcutOverride(k);
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

void KateMainWindow::mousePressEvent(QMouseEvent *e)
{
    switch (e->button()) {
        case Qt::ForwardButton:
            slotFocusNextTab();
            break;
        case Qt::BackButton:
            slotFocusPrevTab();
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

void KateMainWindow::slotQuickOpen()
{
    /**
     * toggle back to view manager when when quick open is already shown
     */
    if (m_mainStackedWidget->currentWidget() == m_quickOpen) {
        m_mainStackedWidget->setCurrentWidget(m_viewManager);
        centralWidget()->setFocusProxy(m_viewManager);
        return;
    }

    /**
     * show quick open and pass focus to it
     */
    m_quickOpen->update();
    m_mainStackedWidget->setCurrentWidget(m_quickOpen);
    centralWidget()->setFocusProxy(m_quickOpen);
    m_quickOpen->setFocus();
}

QWidget *KateMainWindow::createToolView(KTextEditor::Plugin *plugin, const QString &identifier, KTextEditor::MainWindow::ToolViewPosition pos, const QIcon &icon, const QString &text)
{
    // FIXME KF5
    return KateMDI::MainWindow::createToolView(plugin, identifier, (KMultiTabBar::KMultiTabBarPosition)(pos), icon.pixmap(QSize(16, 16)), text);
}

bool KateMainWindow::moveToolView(QWidget *widget, KTextEditor::MainWindow::ToolViewPosition pos)
{
    if (!qobject_cast<KateMDI::ToolView *>(widget)) {
        return false;
    }

    // FIXME KF5
    return KateMDI::MainWindow::moveToolView(qobject_cast<KateMDI::ToolView *>(widget), (KMultiTabBar::KMultiTabBarPosition)(pos));
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

void KateMainWindow::setQuickOpenMatchMode(int mode)
{
    m_quickOpen->setMatchMode(mode);
}

int KateMainWindow::quickOpenMatchMode()
{
    return m_quickOpen->matchMode();
}

void KateMainWindow::setQuickOpenListMode(KateQuickOpenModel::List mode)
{
    m_quickOpen->setListMode(mode);
}

KateQuickOpenModel::List KateMainWindow::quickOpenListMode() const
{
    return m_quickOpen->listMode();
}
