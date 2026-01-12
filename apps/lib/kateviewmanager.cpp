/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2001 Christoph Cullmann <cullmann@kde.org>
   // SPDX-FileCopyrightText: 2001 Joseph Wenninger <jowenn@kde.org>
   SPDX-FileCopyrightText: 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>

   SPDX-License-Identifier: LGPL-2.0-only
*/

// BEGIN Includes
#include "kateviewmanager.h"

#include "kateapp.h"
#include "katedocmanager.h"
#include "katemainwindow.h"
#include "kateupdatedisabler.h"
#include "kateviewspace.h"
#include "welcomeview/welcomeview.h"

#include <KTextEditor/Attribute>
#include <KTextEditor/Document>
#include <KTextEditor/View>

#include <KAboutData>
#include <KActionCollection>
#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KMessageBox>
#include <KSharedConfig>
#include <KToolBar>
#include <KXMLGUIFactory>

#include <QFileDialog>
#include <QScrollBar>
#include <QTimer>

// END Includes

static constexpr qint64 FileSizeAboveToAskUserIfProceedWithOpen = 10L * 1024 * 1024; // 10MB should suffice

KateViewManager::KateViewManager(QWidget *parentW, KateMainWindow *parent)
    : KateSplitter(parentW)
    , m_mainWindow(parent)
    , m_blockViewCreationAndActivation(false)
    , m_minAge(0)
    , m_guiMergedView(nullptr)
{
    setObjectName(QStringLiteral("KateViewManager"));

    // start in home if we are not started from a terminal
    m_lastOpenDialogUrl =
        QUrl::fromLocalFile(KateApp::isInsideTerminal() ? QDir::currentPath() : QStandardPaths::writableLocation(QStandardPaths::HomeLocation));

    // we don't allow full collapse, see bug 366014
    setChildrenCollapsible(false);

    // important, set them up, as we use them in other methods
    setupActions();

    auto *vs = new KateViewSpace(this, nullptr);
    addWidget(vs);

    vs->setActive(true);
    m_viewSpaceList.push_back(vs);

    connect(this, &KateViewManager::viewChanged, this, &KateViewManager::slotViewChanged);

    /**
     * before document is really deleted: cleanup all views!
     */
    connect(KateApp::self()->documentManager(), &KateDocManager::documentWillBeDeleted, this, &KateViewManager::documentWillBeDeleted);

    /**
     * handle document deletion transactions
     * disable view creation in between
     * afterwards ensure we have views ;)
     */
    connect(KateApp::self()->documentManager(), &KateDocManager::aboutToDeleteDocuments, this, &KateViewManager::aboutToDeleteDocuments);
    connect(KateApp::self()->documentManager(), &KateDocManager::documentsDeleted, this, &KateViewManager::documentsDeleted);

    // we want to trigger showing of the welcome view or a new document
    showWelcomeViewOrNewDocumentIfNeeded();

    // enforce configured limit
    readConfig();

    // handle config changes
    connect(KateApp::self(), &KateApp::configurationChanged, this, &KateViewManager::readConfig);
}

KateViewManager::~KateViewManager()
{
    /**
     * remove the single client that is registered at the factory, if any
     */
    if (m_guiMergedView) {
        mainWindow()->guiFactory()->removeClient(m_guiMergedView);
        m_guiMergedView = nullptr;
    }
}

void KateViewManager::readConfig()
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig();
    KConfigGroup cgGeneral = KConfigGroup(config, QStringLiteral("General"));
    m_sdiMode = cgGeneral.readEntry("SDI Mode", false);
}

void KateViewManager::slotScrollSynchedViews(KTextEditor::View *view)
{
    //  Match current view
    QHash<KTextEditor::View *, ScrollSynchronisation::ScrollBarInfo>::const_iterator currentViewInfo = m_scrollSynchronisation.viewScrollInfo.constFind(view);
    //  Delta scroll in reference scroll value
    m_scrollSynchronisation.referenceScrollValue = currentViewInfo.value().scrollBar->value() - currentViewInfo.value().initScrollValue;
    //  Delta scroll in all other views
    for (auto otherViewInfo = m_scrollSynchronisation.viewScrollInfo.constBegin(), end = m_scrollSynchronisation.viewScrollInfo.constEnd();
         otherViewInfo != end;
         ++otherViewInfo) {
        if (otherViewInfo != currentViewInfo) {
            otherViewInfo.value().scrollBar->setValue(m_scrollSynchronisation.referenceScrollValue + otherViewInfo.value().initScrollValue);
        }
    }
}

bool KateViewManager::hasScrollSync() const
{
    return !m_scrollSynchronisation.synchedViewSpaces.isEmpty();
}

void KateViewManager::updateScrollSyncIndicatorVisibility()
{
    const bool syncActive = hasScrollSync();
    for (KateViewSpace *viewSpace : m_viewSpaceList) {
        viewSpace->setScrollSyncToolButtonVisible(syncActive);
    }
}

void KateViewManager::setupActions()
{
    auto mac = m_mainWindow->actionCollection();
    /**
     * view splitting
     */
    m_splitViewVert = mac->addAction(QStringLiteral("view_split_vert"));
    m_splitViewVert->setIcon(QIcon::fromTheme(QStringLiteral("view-split-left-right")));
    m_splitViewVert->setText(i18n("Split Ve&rtical"));
    KActionCollection::setDefaultShortcut(m_splitViewVert, Qt::CTRL | Qt::SHIFT | Qt::Key_L);
    connect(m_splitViewVert, &QAction::triggered, this, &KateViewManager::slotSplitViewSpaceVert);

    m_splitViewVert->setWhatsThis(i18n("Split the currently active view vertically into two views."));

    m_splitViewHoriz = mac->addAction(QStringLiteral("view_split_horiz"));
    m_splitViewHoriz->setIcon(QIcon::fromTheme(QStringLiteral("view-split-top-bottom")));
    m_splitViewHoriz->setText(i18n("Split &Horizontal"));
    KActionCollection::setDefaultShortcut(m_splitViewHoriz, Qt::CTRL | Qt::SHIFT | Qt::Key_T);
    connect(m_splitViewHoriz, &QAction::triggered, this, &KateViewManager::slotSplitViewSpaceHoriz);

    m_splitViewHoriz->setWhatsThis(i18n("Split the currently active view horizontally into two views."));

    m_splitViewVertMove = mac->addAction(QStringLiteral("view_split_vert_move_doc"));
    m_splitViewVertMove->setIcon(QIcon::fromTheme(QStringLiteral("view-split-left-right")));
    m_splitViewVertMove->setText(i18n("Move Document to New Vertical Split"));
    connect(m_splitViewVertMove, &QAction::triggered, this, &KateViewManager::slotSplitViewSpaceVertMoveDoc);

    m_splitViewVertMove->setWhatsThis(
        i18n("Split the currently active view vertically into two views "
             "and move the currently active document to right view."));

    m_splitViewHorizMove = mac->addAction(QStringLiteral("view_split_horiz_move_doc"));
    m_splitViewHorizMove->setIcon(QIcon::fromTheme(QStringLiteral("view-split-top-bottom")));
    m_splitViewHorizMove->setText(i18n("Move Document to New Horizontal Split"));
    connect(m_splitViewHorizMove, &QAction::triggered, this, &KateViewManager::slotSplitViewSpaceHorizMoveDoc);

    m_splitViewHorizMove->setWhatsThis(
        i18n("Split the currently active view horizontally into two views "
             "and move the currently active document to view below."));

    m_closeView = mac->addAction(QStringLiteral("view_close_current_space"));
    m_closeView->setIcon(QIcon::fromTheme(QStringLiteral("view-close")));
    m_closeView->setText(i18n("Cl&ose Current View"));
    KActionCollection::setDefaultShortcut(m_closeView, Qt::CTRL | Qt::SHIFT | Qt::Key_R);
    connect(m_closeView, &QAction::triggered, this, &KateViewManager::slotCloseCurrentViewSpace, Qt::QueuedConnection);

    m_closeView->setWhatsThis(i18n("Close the currently active split view"));

    m_closeOtherViews = mac->addAction(QStringLiteral("view_close_others"));
    m_closeOtherViews->setIcon(QIcon::fromTheme(QStringLiteral("view-close")));
    m_closeOtherViews->setText(i18n("Close Inactive Views"));
    connect(m_closeOtherViews, &QAction::triggered, this, &KateViewManager::slotCloseOtherViews, Qt::QueuedConnection);

    m_closeOtherViews->setWhatsThis(i18n("Close every view but the active one"));

    m_hideOtherViews = mac->addAction(QStringLiteral("view_hide_others"));
    m_hideOtherViews->setIcon(QIcon::fromTheme(QStringLiteral("view-fullscreen")));
    m_hideOtherViews->setText(i18n("Hide Inactive Views"));
    m_hideOtherViews->setCheckable(true);
    connect(m_hideOtherViews, &QAction::triggered, this, &KateViewManager::slotHideOtherViews, Qt::QueuedConnection);

    m_hideOtherViews->setWhatsThis(i18n("Hide every view but the active one"));

    m_toggleSplitterOrientation = mac->addAction(QStringLiteral("view_split_toggle"));
    m_toggleSplitterOrientation->setText(i18n("Toggle Orientation"));
    connect(m_toggleSplitterOrientation, &QAction::triggered, this, &KateViewManager::toggleSplitterOrientation, Qt::QueuedConnection);

    m_toggleSplitterOrientation->setWhatsThis(i18n("Toggles the orientation of the current split view"));

    goNext = mac->addAction(QStringLiteral("go_next_split_view"));
    goNext->setText(i18n("Next Split View"));
    goNext->setIcon(QIcon::fromTheme(QStringLiteral("go-next-view")));
    KActionCollection::setDefaultShortcut(goNext, Qt::Key_F8);
    connect(goNext, &QAction::triggered, this, &KateViewManager::activateNextView);

    goNext->setWhatsThis(i18n("Make the next split view the active one."));

    goPrev = mac->addAction(QStringLiteral("go_prev_split_view"));
    goPrev->setText(i18n("Previous Split View"));
    goPrev->setIcon(QIcon::fromTheme(QStringLiteral("go-previous-view")));
    KActionCollection::setDefaultShortcut(goPrev, Qt::SHIFT | Qt::Key_F8);
    connect(goPrev, &QAction::triggered, this, &KateViewManager::activatePrevView);

    goPrev->setWhatsThis(i18n("Make the previous split view the active one."));

    goLeft = mac->addAction(QStringLiteral("go_left_split_view"));
    goLeft->setText(i18n("Left Split View"));
    goLeft->setIcon(QIcon::fromTheme(QStringLiteral("arrow-left")));
    // KActionCollection::setDefaultShortcut(goLeft, Qt::ALT | Qt::Key_Left);
    connect(goLeft, &QAction::triggered, this, &KateViewManager::activateLeftView);

    goLeft->setWhatsThis(i18n("Make the split view intuitively on the left the active one."));

    goRight = mac->addAction(QStringLiteral("go_right_split_view"));
    goRight->setText(i18n("Right Split View"));
    goRight->setIcon(QIcon::fromTheme(QStringLiteral("arrow-right")));
    // KActionCollection::setDefaultShortcut(goRight, Qt::ALT | Qt::Key_Right);
    connect(goRight, &QAction::triggered, this, &KateViewManager::activateRightView);

    goRight->setWhatsThis(i18n("Make the split view intuitively on the right the active one."));

    goUp = mac->addAction(QStringLiteral("go_upward_split_view"));
    goUp->setText(i18n("Upward Split View"));
    goUp->setIcon(QIcon::fromTheme(QStringLiteral("arrow-up")));
    // KActionCollection::setDefaultShortcut(goUp, Qt::ALT | Qt::Key_Up);
    connect(goUp, &QAction::triggered, this, &KateViewManager::activateUpwardView);

    goUp->setWhatsThis(i18n("Make the split view intuitively upward the active one."));

    goDown = mac->addAction(QStringLiteral("go_downward_split_view"));
    goDown->setText(i18n("Downward Split View"));
    goDown->setIcon(QIcon::fromTheme(QStringLiteral("arrow-down")));
    // KActionCollection::setDefaultShortcut(goDown, Qt::ALT | Qt::Key_Down);
    connect(goDown, &QAction::triggered, this, &KateViewManager::activateDownwardView);

    goDown->setWhatsThis(i18n("Make the split view intuitively downward the active one."));

    QAction *a = mac->addAction(QStringLiteral("view_split_move_right"));
    a->setText(i18n("Move Splitter Right"));
    connect(a, &QAction::triggered, this, &KateViewManager::moveSplitterRight);

    a->setWhatsThis(i18n("Move the splitter of the current view to the right"));

    a = mac->addAction(QStringLiteral("view_split_move_left"));
    a->setText(i18n("Move Splitter Left"));
    connect(a, &QAction::triggered, this, &KateViewManager::moveSplitterLeft);

    a->setWhatsThis(i18n("Move the splitter of the current view to the left"));

    a = mac->addAction(QStringLiteral("view_split_move_up"));
    a->setText(i18n("Move Splitter Up"));
    connect(a, &QAction::triggered, this, &KateViewManager::moveSplitterUp);

    a->setWhatsThis(i18n("Move the splitter of the current view up"));

    a = mac->addAction(QStringLiteral("view_split_move_down"));
    a->setText(i18n("Move Splitter Down"));
    connect(a, &QAction::triggered, this, &KateViewManager::moveSplitterDown);

    a->setWhatsThis(i18n("Move the splitter of the current view down"));

    a = mac->addAction(QStringLiteral("viewspace_focus_nav_bar"));
    a->setText(i18n("Focus Navigation Bar"));
    a->setToolTip(i18n("Focus the navigation bar"));
    KActionCollection::setDefaultShortcut(a, Qt::CTRL | Qt::SHIFT | Qt::Key_Period);
    connect(a, &QAction::triggered, this, [this] {
        activeViewSpace()->focusNavigationBar();
    });

    a = mac->addAction(QStringLiteral("help_welcome_page"));
    a->setText(i18n("Welcome Page"));
    a->setIcon(qApp->windowIcon());
    connect(a, &QAction::triggered, this, [this]() {
        showWelcomeView();
    });
    a->setWhatsThis(i18n("Show the welcome page"));

    /**
     * Synchronized scrolling shortcut
     */
    a = mac->addAction(QStringLiteral("toggle_active_viewspace_scroll_sync"));
    a->setText(i18n("Toggle Scroll Synchronization"));
    a->setToolTip(i18n("Toggle scroll synchronization for the current view"));
    connect(a, &QAction::triggered, this, [this]() {
        activeViewSpace()->toggleScrollSync();
    });
    a->setIcon(QIcon::fromTheme(QStringLiteral("link")));

    /**
     * Synchronized scrolling shortcut
     */
    a = mac->addAction(QStringLiteral("detach_active_view_doc"));
    a->setText(i18n("Detach Active Document"));
    a->setToolTip(i18n("Opens the active document in a new window and closes it in the current window"));
    connect(a, &QAction::triggered, this, [this]() {
        if (auto view = activeView()) {
            activeViewSpace()->detachDocument(view->document());
        }
    });
    a->setIcon(QIcon::fromTheme(QStringLiteral("tab-detach")));
}

void KateViewManager::updateViewSpaceActions()
{
    const bool multipleViewSpaces = m_viewSpaceList.size() > 1;
    m_closeView->setEnabled(multipleViewSpaces);
    m_closeOtherViews->setEnabled(multipleViewSpaces);
    m_toggleSplitterOrientation->setEnabled(multipleViewSpaces);
    m_hideOtherViews->setEnabled(multipleViewSpaces);
    goNext->setEnabled(multipleViewSpaces);
    goPrev->setEnabled(multipleViewSpaces);

    // only allow to split if we have a view that we could show in the new view space
    const bool allowSplit = activeViewSpace();
    m_splitViewVert->setEnabled(allowSplit);
    m_splitViewHoriz->setEnabled(allowSplit);

    // only allow move if we have more than one document in the current view space
    const bool allowMove = allowSplit && (activeViewSpace()->numberOfRegisteredDocuments() > 1);
    m_splitViewVertMove->setEnabled(allowMove);
    m_splitViewHorizMove->setEnabled(allowMove);
}

void KateViewManager::slotDocumentNew()
{
    // open new window for SDI case
    if (m_sdiMode && !m_views.empty()) {
        auto mainWindow = m_mainWindow->newWindow();
        mainWindow->viewManager()->createView();
    } else {
        createView();
    }
}

void KateViewManager::slotDocumentOpen()
{
    // try to start dialog in useful dir: either dir of current doc or last used one
    KTextEditor::View *const cv = activeView();
    QUrl startUrl = cv ? cv->document()->url() : QUrl();
    if (startUrl.isValid()) {
        m_lastOpenDialogUrl = startUrl;
    } else {
        startUrl = m_lastOpenDialogUrl;
    }
    // if file is not local, then remove filename from url
    QList<QUrl> urls;
    if (startUrl.isLocalFile()) {
        urls = QFileDialog::getOpenFileUrls(m_mainWindow, i18n("Open File"), startUrl);
    } else {
        urls = QFileDialog::getOpenFileUrls(m_mainWindow, i18n("Open File"), startUrl.adjusted(QUrl::RemoveFilename));
    }

    /**
     * emit size warning, for local files
     */
    QString fileListWithTooLargeFiles;
    for (const QUrl &url : std::as_const(urls)) {
        if (!url.isLocalFile()) {
            continue;
        }

        const qint64 size = QFile(url.toLocalFile()).size();
        if (size > FileSizeAboveToAskUserIfProceedWithOpen) {
            fileListWithTooLargeFiles += QStringLiteral("<li>%1 (%2MB)</li>").arg(url.fileName()).arg(size / 1024 / 1024);
        }
    }
    if (!fileListWithTooLargeFiles.isEmpty()) {
        const QString text = i18n(
            "<p>You are attempting to open one or more large files:</p><ul>%1</ul><p>Do you want to proceed?</p><p><strong>Beware that kate may stop "
            "responding for some time when opening large files.</strong></p>",
            fileListWithTooLargeFiles);
        const auto ret = KMessageBox::warningTwoActions(this, text, i18n("Opening Large File"), KStandardGuiItem::cont(), KStandardGuiItem::stop());
        if (ret == KMessageBox::SecondaryAction) {
            return;
        }
    }

    // activate view of last opened document
    KateDocumentInfo docInfo;
    docInfo.openedByUser = true;
    if (KTextEditor::Document *lastID = openUrls(urls, QString(), &docInfo)) {
        for (const QUrl &url : std::as_const(urls)) {
            m_mainWindow->addRecentOpenedFile(url);
        }
        activateView(lastID);
    }
}

void KateViewManager::slotDocumentClose(KTextEditor::Document *document)
{
    // Close document
    const auto url = document->url();
    if (!KateApp::self()->documentManager()->closeDocument(document)) {
        return;
    }

    // keep recent used files up to date
    mainWindow()->addRecentOpenedFile(url);

    int docOrWidgetCount = 0;
    for (auto vs : m_viewSpaceList) {
        docOrWidgetCount += vs->numberOfRegisteredDocuments();
    }

    // we might need to close the app or show the welcome page
    if (docOrWidgetCount == 0) {
        // Close window if specified
        if (m_mainWindow->modCloseAfterLast()) {
            KateApp::self()->shutdownKate(m_mainWindow);
        }

        KSharedConfig::Ptr config = KSharedConfig::openConfig();
        KConfigGroup cgGeneral = KConfigGroup(config, QStringLiteral("General"));

        // Otherwise, show the welcome view
        if (cgGeneral.readEntry("Show welcome view for new window", true)) {
            showWelcomeView();
        }
    }
}

void KateViewManager::slotDocumentClose()
{
    if (auto vs = activeViewSpace()) {
        if (auto w = vs->currentWidget()) {
            vs->closeTabWithWidget(w);
            return;
        }

        // no active view, do nothing
        auto view = activeView();
        if (!view) {
            return;
        }

        vs->closeDocument(view->document());
    }
}

void KateViewManager::slotRestoreLastClosedDocument()
{
    const auto &documentManager = KateApp::self()->documentManager();
    if (const auto recentlyClosedUrls = documentManager->popRecentlyClosedUrls(); !recentlyClosedUrls.isEmpty()) {
        if (const auto restoredDoc = openUrls(recentlyClosedUrls, QString())) {
            activateView(restoredDoc);
        }
    }
}

void KateViewManager::slotOpenNextPinnedDocument()
{
    const auto &pinnedUrls = KateApp::self()->documentManager()->pinnedDocumentUrls();
    if (pinnedUrls.empty()) {
        return;
    }

    QUrl activeUrl;
    if (auto view = activeView()) {
        activeUrl = view->document()->url();
    }

    auto it = std::find(pinnedUrls.begin(), pinnedUrls.end(), activeUrl);
    if (it == pinnedUrls.end()) {
        openUrlWithView(pinnedUrls.first(), QString());
    } else {
        QUrl url = it + 1 == pinnedUrls.end() ? pinnedUrls.first() : *(it + 1);
        openUrlWithView(url, QString());
    }
}

KTextEditor::Document *
KateViewManager::openUrl(const QUrl &url, const QString &encoding, bool activate, bool ignoreForRecentFiles, const KateDocumentInfo *docInfo)
{
    auto doc = openUrls({&url, 1}, encoding, docInfo);
    if (!doc) {
        return nullptr;
    }

    if (!ignoreForRecentFiles) {
        m_mainWindow->addRecentOpenedFile(doc->url());
    }

    if (activate) {
        activateView(doc);
    }

    return doc;
}

KTextEditor::Document *KateViewManager::openUrls(std::span<const QUrl> urls, const QString &encoding, const KateDocumentInfo *docInfo)
{
    // remember if we have just one view with an unmodified untitled document, if yes, we close that one
    // same heuristics we had before in the document manager for the single untitled doc, but this works for multiple main windows
    KTextEditor::Document *docToClose = nullptr;
    if (m_views.size() == 1) {
        auto singleDoc = m_views.begin()->first->document();
        if (!singleDoc->isModified() && singleDoc->url().isEmpty() && singleDoc->views().size() == 1) {
            docToClose = singleDoc;
        }
    }

    // try to open all given files, early out if nothing done
    auto docs = KateApp::self()->documentManager()->openUrls(urls, encoding, docInfo ? *docInfo : KateDocumentInfo());
    if (docs.empty()) {
        return nullptr;
    }

    bool first = true;
    KTextEditor::Document *lastDocInThisViewManager = nullptr;
    for (auto doc : docs) {
        // it we have a document to close, we can use this window for the first document even in SDI mode
        // try to re-use open views to avoid massive windows spawning, see bug 474775
        if (!m_sdiMode || (first && docToClose) || activeViewSpace()->numberOfRegisteredDocuments() == 0 || activeViewSpace()->hasDocument(doc)) {
            // forward to currently active view space
            const bool wasEmpty = activeViewSpace()->numberOfRegisteredDocuments() == 0;
            activeViewSpace()->registerDocument(doc);
            connect(doc, &KTextEditor::Document::documentSavedOrUploaded, this, &KateViewManager::documentSavedOrUploaded);
            lastDocInThisViewManager = doc;
            first = false;
            if (wasEmpty) {
                activateView(doc);
            }
            continue;
        }

        // open new window for SDI case
        auto mainWindow = m_mainWindow->newWindow();
        mainWindow->viewManager()->openViewForDoc(doc);
        first = false;
    }

    // if we had some single untitled doc around, trigger close
    // do this delayed to avoid havoc
    if (docToClose) {
        QTimer::singleShot(0, docToClose, [docToClose]() {
            KateApp::self()->documentManager()->closeDocument(docToClose);
        });
    }

    // return the last document we opened in this view manager if any
    return lastDocInThisViewManager;
}

KTextEditor::View *KateViewManager::openUrlWithView(const QUrl &url, const QString &encoding)
{
    KTextEditor::Document *doc = openUrls({&url, 1}, encoding);
    if (!doc) {
        return nullptr;
    }

    m_mainWindow->addRecentOpenedFile(doc->url());

    return activateView(doc);
}

void KateViewManager::openUrl(const QUrl &url)
{
    openUrl(url, QString());
}

void KateViewManager::openUrlOrProject(const QUrl &url)
{
    if (!url.isLocalFile()) {
        openUrl(url);
        return;
    }

    const QDir dir(url.toLocalFile());
    if (!dir.exists()) {
        openUrl(url);
        return;
    }

    if (KateApp::isKWrite()) {
        QString error = i18n("%1 cannot open folders", KAboutData::applicationData().displayName());
        KMessageBox::error(mainWindow(), error);
        return;
    }

    // try to open the folder
    static const QString projectPluginId = QStringLiteral("kateprojectplugin");
    QObject *projectPluginView = mainWindow()->pluginView(projectPluginId);
    if (!projectPluginView) {
        // try to find and enable the Projects plugin
        KatePluginList &pluginList = KateApp::self()->pluginManager()->pluginList();
        auto i = std::find_if(pluginList.begin(), pluginList.end(), [](const KatePluginInfo &pluginInfo) {
            return pluginInfo.metaData.pluginId() == projectPluginId;
        });

        if (i == pluginList.end()) {
            QString error = i18n("The plugin required to open folders was not found");
            KMessageBox::error(mainWindow(), error);
            return;
        }

        KatePluginInfo &projectPluginInfo = *i;
        QString questionText = i18n("In order to open folders, the <b>%1</b> plugin must be enabled. Enable it?", projectPluginInfo.metaData.name());
        if (KMessageBox::questionTwoActions(mainWindow(),
                                            questionText,
                                            i18nc("@title:window", "Open Folder"),
                                            KGuiItem(i18nc("@action:button", "Enable"), QStringLiteral("dialog-ok")),
                                            KStandardGuiItem::cancel())
            == KMessageBox::SecondaryAction) {
            return;
        }

        if (!KateApp::self()->pluginManager()->loadPlugin(&projectPluginInfo)) {
            QString error = i18n("Failed to enable <b>%1</b> plugin", projectPluginInfo.metaData.name());
            KMessageBox::error(mainWindow(), error);
            return;
        }

        KatePluginManager::enablePluginGUI(&projectPluginInfo);
        projectPluginView = mainWindow()->pluginView(projectPluginId);
    }

    Q_ASSERT(projectPluginView);
    QMetaObject::invokeMethod(projectPluginView, "openDirectoryOrProject", Q_ARG(const QDir &, dir));
}

void KateViewManager::openRecent(const QUrl &url)
{
    Q_ASSERT(url.isValid());

    // Handle only local files
    if (url.isLocalFile()) {
        const QString localFilePath = url.toLocalFile();
        if (!QFile::exists(localFilePath)) {
            KMessageBox::error(mainWindow(), i18n("The file '%1' does not exist", localFilePath));
            return;
        }
    }

    openUrlOrProject(url);
}

KTextEditor::View *KateViewManager::openViewForDoc(KTextEditor::Document *doc)
{
    // forward to currently active view space
    auto viewspace = activeViewSpace();
    viewspace->registerDocument(doc);
    connect(doc, &KTextEditor::Document::documentSavedOrUploaded, this, &KateViewManager::documentSavedOrUploaded);

    return activateView(doc, viewspace);
}

void KateViewManager::addPositionToHistory(const QUrl &url, KTextEditor::Cursor pos)
{
    if (KateViewSpace *avs = activeViewSpace()) {
        avs->addPositionToHistory(url, pos, /* calledExternally: */ true);
    }
}

KateMainWindow *KateViewManager::mainWindow()
{
    return m_mainWindow;
}

void KateViewManager::aboutToDeleteDocuments()
{
    /**
     * block view creation until the transaction is done
     * this shall not stack!
     */
    Q_ASSERT(!m_blockViewCreationAndActivation);
    m_blockViewCreationAndActivation = true;

    /**
     * disable updates hard (we can't use KateUpdateDisabler here, we have delayed signal
     */
    mainWindow()->setUpdatesEnabled(false);
}

void KateViewManager::documentsDeleted()
{
    /**
     * again allow view creation
     */
    m_blockViewCreationAndActivation = false;

    /**
     * ensure we don't end up with empty tabs in some view spaces
     * we did block view creation, re-trigger it
     */
    auto viewspace = activeViewSpace();
    for (auto vs : m_viewSpaceList) {
        if (vs != viewspace) {
            vs->ensureViewForCurrentTab();
        }
    }
    setActiveSpace(viewspace);
    // Do it last for active view space so it remains active
    activeViewSpace()->ensureViewForCurrentTab();

    /**
     * reactivate will ensure we really merge up the GUI again
     * this might be missed as above we had m_blockViewCreationAndActivation set to true
     * see bug 426605, no view XMLGUI stuff merged after tab close
     */
    replugActiveView();

    // trigger action update
    updateViewSpaceActions();

    /**
     * enable updates hard (we can't use KateUpdateDisabler here, we have delayed signal
     */
    mainWindow()->setUpdatesEnabled(true);
}

void KateViewManager::documentSavedOrUploaded(KTextEditor::Document *doc, bool)
{
    m_mainWindow->addRecentOpenedFile(doc->url());
}

KTextEditor::View *KateViewManager::createView(KTextEditor::Document *doc, KateViewSpace *vs)
{
    if (m_blockViewCreationAndActivation) {
        return nullptr;
    }

    // create doc
    if (!doc) {
        doc = KateApp::self()->documentManager()->createDoc();
    }

    /**
     * ensure the initial welcome view vanishes as soon as we have some real view!
     */
    hideWelcomeView(vs);

    /**
     * create view, registers its XML gui itself
     * pass the view the correct main window
     */
    KTextEditor::View *view = (vs ? vs : activeViewSpace())->createView(doc);

    /**
     * remember this view, active == false, min age set
     */
    m_views[view].lruAge = m_minAge--;

    // disable settings dialog action
    delete view->actionCollection()->action(QStringLiteral("set_confdlg"));

    // clang-format off
    connect(view, SIGNAL(dropEventPass(QDropEvent*)), mainWindow(), SLOT(slotDropEvent(QDropEvent*)));
    // clang-format on
    connect(view, &KTextEditor::View::focusIn, this, &KateViewManager::activateSpace);

    viewCreated(view);

    if (!vs) {
        activateView(view);
    }

    updateViewSpaceActions();

    /**
     * connect to signal here so we can handle post-load
     * set cursor position for this view if we need to
     * do this after the view is properly registered, else we might trigger assertions
     * about unknown views in the connected lambdas
     */
    KateDocumentInfo *docInfo = KateApp::self()->documentManager()->documentInfo(doc);
    if (docInfo->startCursor.isValid()) {
        KTextEditor::Cursor c = docInfo->startCursor; // Was a cursor position requested?
        docInfo->startCursor = KTextEditor::Cursor::invalid(); // do this only once

        if (!doc->url().isLocalFile()) {
            std::shared_ptr<QMetaObject::Connection> conn(new QMetaObject::Connection());
            auto handler = [view, conn, c](KTextEditor::Document *) {
                QObject::disconnect(*conn);
                view->setCursorPosition(c);
            };

            *conn = connect(doc, &KTextEditor::Document::textChanged, view, handler);
        } else if (c.isValid()) {
            view->setCursorPosition(c);
        }
    }

    return view;
}

bool KateViewManager::deleteView(KTextEditor::View *view)
{
    if (!view) {
        return true;
    }

    auto *viewspace = static_cast<KateViewSpace *>(view->parent()->parent());
    viewspace->removeView(view);
    updateViewSpaceActions();

    /**
     * deregister if needed
     */
    if (m_guiMergedView == view) {
        mainWindow()->guiFactory()->removeClient(m_guiMergedView);
        m_guiMergedView = nullptr;
    }

    // remove view from mapping and memory !!
    m_views.erase(view);
    delete view;
    return true;
}

KateViewSpace *KateViewManager::activeViewSpace()
{
    for (auto vs : m_viewSpaceList) {
        if (vs->isActiveSpace()) {
            return vs;
        }
    }

    // none active, so use the first we grab
    if (!m_viewSpaceList.empty()) {
        m_viewSpaceList.front()->setActive(true);
        return m_viewSpaceList.front();
    }

    Q_ASSERT(false);
    return nullptr;
}

QWidgetList KateViewManager::widgets() const
{
    QWidgetList widgets;
    for (auto *vs : m_viewSpaceList) {
        widgets << vs->widgets();
    }
    return widgets;
}

bool KateViewManager::removeWidget(QWidget *w)
{
    for (auto *vs : m_viewSpaceList) {
        if (vs->closeTabWithWidget(w)) {
            return true;
        }
    }
    return false;
}

bool KateViewManager::activateWidget(QWidget *w)
{
    for (auto *vs : m_viewSpaceList) {
        if (vs->activateWidget(w)) {
            return true;
        }
    }
    return false;
}

KTextEditor::View *KateViewManager::activeView()
{
    return m_guiMergedView;
}

void KateViewManager::setActiveSpace(KateViewSpace *vs)
{
    if (auto activeSpace = activeViewSpace()) {
        activeSpace->setActive(false);
    }

    vs->setActive(true);

    // signal update history buttons in mainWindow
    Q_EMIT historyBackEnabled(vs->isHistoryBackEnabled());
    Q_EMIT historyForwardEnabled(vs->isHistoryForwardEnabled());
}

void KateViewManager::activateSpace(KTextEditor::View *v)
{
    if (!v) {
        return;
    }

    auto *vs = static_cast<KateViewSpace *>(v->parent()->parent());

    if (!vs->isActiveSpace()) {
        setActiveSpace(vs);
        activateView(v);
    }
}

void KateViewManager::replugActiveView()
{
    if (m_guiMergedView) {
        mainWindow()->guiFactory()->removeClient(m_guiMergedView);
        mainWindow()->guiFactory()->addClient(m_guiMergedView);
    }
}

void KateViewManager::activateView(KTextEditor::View *view)
{
    if (!view) {
        if (m_guiMergedView) {
            mainWindow()->guiFactory()->removeClient(m_guiMergedView);
            m_guiMergedView = nullptr;
        }
        Q_EMIT viewChanged(nullptr);
        updateViewSpaceActions();
        return;
    }

    if (!m_guiMergedView || m_guiMergedView != view) {
        // avoid flicker
        KateUpdateDisabler disableUpdates(mainWindow());

        if (!activeViewSpace()->showView(view)) {
            // since it wasn't found, give'em a new one
            createView(view->document());
            return;
        }

        bool toolbarVisible = mainWindow()->toolBar()->isVisible();
        if (toolbarVisible) {
            mainWindow()->toolBar()->hide(); // hide to avoid toolbar flickering
        }

        if (m_guiMergedView) {
            mainWindow()->guiFactory()->removeClient(m_guiMergedView);
            m_guiMergedView = nullptr;
        }

        if (!m_blockViewCreationAndActivation) {
            mainWindow()->guiFactory()->addClient(view);
            m_guiMergedView = view;
        }

        if (toolbarVisible) {
            mainWindow()->toolBar()->show();
        }

        auto it = m_views.find(view);
        Q_ASSERT(it != m_views.end());
        ViewData &viewData = it->second;
        // remember age of this view
        viewData.lruAge = m_minAge--;

        Q_EMIT viewChanged(view);

        updateViewSpaceActions();
    }
}

KTextEditor::View *KateViewManager::activateView(DocOrWidget docOrWidget, KateViewSpace *vs)
{
    // activate existing view if possible
    auto viewspace = vs ? vs : activeViewSpace();
    if (viewspace->showView(docOrWidget)) {
        // Only activateView if viewspace is active
        if (viewspace == activeViewSpace()) {
            // This will be null if currentView is not a KTE::View
            activateView(viewspace->currentView());
            return activeView();
        }
        return viewspace->currentView();
    }

    // create new view otherwise
    Q_ASSERT(docOrWidget.doc());
    auto v = createView(docOrWidget.doc(), vs);
    // If the requesting viewspace is the activeViewSpace, we activate the view
    if (vs == activeViewSpace()) {
        activateView(v);
    }
    return activeView();
}

void KateViewManager::slotViewChanged()
{
    auto view = activeView();
    if (view && !view->hasFocus()) {
        view->setFocus();
    }
}

void KateViewManager::activateNextView()
{
    auto it = std::find(m_viewSpaceList.begin(), m_viewSpaceList.end(), activeViewSpace());
    ++it;

    if (it == m_viewSpaceList.end()) {
        it = m_viewSpaceList.begin();
    }

    setActiveSpace(*it);
    activateView((*it)->currentView());
}

void KateViewManager::activatePrevView()
{
    auto it = std::find(m_viewSpaceList.begin(), m_viewSpaceList.end(), activeViewSpace());
    if (it == m_viewSpaceList.begin()) {
        it = --m_viewSpaceList.end();
    } else {
        --it;
    }

    setActiveSpace(*it);
    activateView((*it)->currentView());
}

void KateViewManager::activateLeftView()
{
    activateIntuitiveNeighborView(Qt::Horizontal, 0);
}

void KateViewManager::activateRightView()
{
    activateIntuitiveNeighborView(Qt::Horizontal, 1);
}

void KateViewManager::activateUpwardView()
{
    activateIntuitiveNeighborView(Qt::Vertical, 0);
}

void KateViewManager::activateDownwardView()
{
    activateIntuitiveNeighborView(Qt::Vertical, 1);
}

void KateViewManager::activateIntuitiveNeighborView(Qt::Orientation o, int dir)
{
    KateViewSpace *currentViewSpace = activeViewSpace();
    if (!currentViewSpace) {
        return;
    }
    KateViewSpace *target = findIntuitiveNeighborView(qobject_cast<KateSplitter *>(currentViewSpace->parent()), currentViewSpace, o, dir);
    if (target) {
        setActiveSpace(target);
        activateView(target->currentView());
    }
}

KateViewSpace *KateViewManager::findIntuitiveNeighborView(KateSplitter *splitter, QWidget *widget, Qt::Orientation o, int dir)
{
    Q_ASSERT(dir == 0 || dir == 1); // 0 for right or up ; 1 for left or down
    if (!splitter) {
        return nullptr;
    }
    if (splitter->count() == 1) { // root view space, nothing to do
        return nullptr;
    }
    int widgetIndex = splitter->indexOf(widget);
    // if the orientation is the same as the desired move, maybe we can move in the current splitter
    if (splitter->orientation() == o && ((dir == 1 && widgetIndex == 0) || (dir == 0 && widgetIndex == 1))) {
        // make our move
        QWidget *target = splitter->widget(dir);
        // try to see if we are next to a view space and move to it
        auto *targetViewSpace = qobject_cast<KateViewSpace *>(target);
        if (targetViewSpace) {
            return targetViewSpace;
        }
        // otherwise we found a splitter and need to go down the splitter tree to the desired view space
        auto *targetSplitter = qobject_cast<KateSplitter *>(target);
        // we already made our move so we need to go down to the opposite direction :
        // e.g.: [ active | [ [ x | y ] | z ] ] going right we find a splitter and want to go to x
        int childIndex = 1 - dir;
        QWidget *targetChild = nullptr;
        while (targetSplitter) {
            if (targetSplitter->orientation() == o) { // as long as the orientation is the same we move down in the desired direction
                targetChild = targetSplitter->widget(childIndex);
            } else { // otherwise we need to find in which of the two child to go based on the cursor position
                QPoint cursorCoord = activeView()->mapToGlobal(activeView()->cursorPositionCoordinates());
                QPoint targetSplitterCoord = targetSplitter->widget(0)->mapToGlobal(QPoint(0, 0));
                if ((o == Qt::Horizontal && (targetSplitterCoord.y() + targetSplitter->sizes().constFirst() > cursorCoord.y()))
                    || (o == Qt::Vertical && (targetSplitterCoord.x() + targetSplitter->sizes().constFirst() > cursorCoord.x()))) {
                    targetChild = targetSplitter->widget(0);
                } else {
                    targetChild = targetSplitter->widget(1);
                }
            }
            // if we found a view space we're done!
            targetViewSpace = qobject_cast<KateViewSpace *>(targetChild);
            if (targetViewSpace) {
                return targetViewSpace;
            }
            // otherwise continue
            targetSplitter = qobject_cast<KateSplitter *>(targetChild);
        }
    }
    // otherwise try to go up in the splitter tree
    return findIntuitiveNeighborView(qobject_cast<KateSplitter *>(splitter->parent()), splitter, o, dir);
}

void KateViewManager::documentWillBeDeleted(KTextEditor::Document *doc)
{
    /**
     * collect all views of that document that belong to this manager
     */
    QList<KTextEditor::View *> closeList;
    const auto views = doc->views();
    for (KTextEditor::View *v : views) {
        if (m_views.contains(v)) {
            closeList.append(v);
        }
    }

    while (!closeList.isEmpty()) {
        deleteView(closeList.takeFirst());
    }
}

void KateViewManager::closeView(KTextEditor::View *view)
{
    // we can only close views that are in our viewspaces
    if (!view || !view->parent() || !view->parent()->parent()) {
        return;
    }

    // get the viewspace if possible and close the document there => should close the passed view
    // will close the full document, too, if last
    auto viewSpace = qobject_cast<KateViewSpace *>(view->parent()->parent());
    if (viewSpace) {
        viewSpace->closeDocument(view->document());
    }
}

void KateViewManager::moveViewToViewSpace(KateViewSpace *dest, KateViewSpace *src, DocOrWidget docOrWidget)
{
    // We always have an active view at this point which is what we are moving
    Q_ASSERT(activeView() || src->currentWidget());

    // Are we trying to drop into some other mainWindow of current app session?
    // shouldn't happen, but just a safe guard
    if (src->viewManager() != dest->viewManager()) {
        return;
    }

    QWidget *view = src->takeView(docOrWidget);
    if (!view) {
        qWarning("%s Unexpected null view when trying to drag the view to a different viewspace %p", Q_FUNC_INFO, (void *)docOrWidget.qobject());
        return;
    }

    dest->addView(view);
    setActiveSpace(dest);
    auto kteView = qobject_cast<KTextEditor::View *>(view);
    if (kteView) {
        activateView(kteView->document(), dest);
    } else {
        activateView(view, dest);
    }
}

void KateViewManager::splitViewSpace(KateViewSpace *vs, // = 0
                                     Qt::Orientation o,
                                     bool moveDocument) // = Qt::Horizontal
{
    // emergency: fallback to activeViewSpace, and if still invalid, abort
    if (!vs) {
        vs = activeViewSpace();
    }
    if (!vs) {
        return;
    }
    if (moveDocument && vs->numberOfRegisteredDocuments() <= 1) {
        return;
    }

    // if we have no current view, splitting makes ATM no sense, as we have
    // nothing to put into the new viewspace
    if (!vs->currentView() && !vs->currentWidget()) {
        return;
    }

    // get current splitter, and abort if null
    auto *currentSplitter = qobject_cast<KateSplitter *>(vs->parent());
    if (!currentSplitter) {
        return;
    }

    // avoid flicker
    KateUpdateDisabler disableUpdates(mainWindow());

    // index where to insert new splitter/viewspace
    const int index = currentSplitter->indexOf(vs);

    // create new viewspace
    auto *vsNew = new KateViewSpace(this);

    // only 1 children -> we are the root container. So simply set the orientation
    // and add the new view space, then correct the sizes to 50%:50%
    if (currentSplitter->count() == 1) {
        if (currentSplitter->orientation() != o) {
            currentSplitter->setOrientation(o);
        }
        QList<int> sizes = currentSplitter->sizes();
        sizes << (sizes.first() - currentSplitter->handleWidth()) / 2;
        sizes[0] = sizes[1];
        currentSplitter->insertWidget(index + 1, vsNew);
        currentSplitter->setSizes(sizes);
    } else {
        // create a new KateSplitter and replace vs with the splitter. vs and newVS are
        // the new children of the new KateSplitter
        auto *newContainer = new KateSplitter(o);

        // we don't allow full collapse, see bug 366014
        newContainer->setChildrenCollapsible(false);

        QList<int> currentSizes = currentSplitter->sizes();

        newContainer->addWidget(vs);
        newContainer->addWidget(vsNew);
        currentSplitter->insertWidget(index, newContainer);
        newContainer->show();

        // fix sizes of children of old and new splitter
        currentSplitter->setSizes(currentSizes);
        QList<int> newSizes = newContainer->sizes();
        newSizes[0] = (newSizes[0] + newSizes[1] - newContainer->handleWidth()) / 2;
        newSizes[1] = newSizes[0];
        newContainer->setSizes(newSizes);
    }

    m_viewSpaceList.push_back(vsNew);
    activeViewSpace()->setActive(false);
    vsNew->setActive(true);
    vsNew->show();

    if (moveDocument) {
        if (vs->currentWidget()) {
            moveViewToViewSpace(vsNew, vs, vs->currentWidget());
        } else {
            moveViewToViewSpace(vsNew, vs, vs->currentView()->document());
        }
    } else {
        auto v = vs->currentView();
        createView(v ? v->document() : nullptr);
    }

    updateViewSpaceActions();
}

void KateViewManager::slotSynchroniseScrolling(bool checked, KateViewSpace *correspondingViewSpace)
{
    KTextEditor::View *userSelectedView = correspondingViewSpace->currentView();
    auto newSynchedViewInfo = m_scrollSynchronisation.viewScrollInfo.constFind(userSelectedView);
    // If already added to sync, then desync
    if (newSynchedViewInfo != m_scrollSynchronisation.viewScrollInfo.constEnd()) {
        Q_ASSERT(!checked);
        disconnect(userSelectedView, &KTextEditor::View::verticalScrollPositionChanged, this, &KateViewManager::slotScrollSynchedViews);
        m_scrollSynchronisation.viewScrollInfo.remove(newSynchedViewInfo.key());
        m_scrollSynchronisation.synchedViewSpaces.remove(correspondingViewSpace);
        if (m_scrollSynchronisation.synchedViewSpaces.isEmpty()) {
            disconnect(this, &KateViewManager::viewChanged, this, &KateViewManager::slotChangeSynchedView);
        }
        return;
    }
    if (!checked) {
        // In case removal from sync was requested, but ViewSpace did not exist
        // This is possible if used tries to activate scroll sync on the "Welcome" page twice.
        return;
    }
    // If not already added to sync, then add to sync
    ScrollSynchronisation::ScrollBarInfo scrollBarInfo = m_scrollSynchronisation.getViewScrollBarInfo(userSelectedView);
    if (!scrollBarInfo.scrollBar) {
        // This case should normally not occur
        // but if it is being used, implement:
        // Uncheck m_toggleSynchronisedScrolling
        return;
    }
    // Connect scrolling to slotSynchedView
    connect(userSelectedView, &KTextEditor::View::verticalScrollPositionChanged, this, &KateViewManager::slotScrollSynchedViews, Qt::UniqueConnection);
    m_scrollSynchronisation.viewScrollInfo.insert(userSelectedView, scrollBarInfo);
    // Remove from group of views, when view is deleted
    connect(userSelectedView, &KTextEditor::View::destroyed, this, [this](QObject *obj) {
        m_scrollSynchronisation.viewScrollInfo.remove(static_cast<KTextEditor::View *>(obj));
    });
    // To cater for multiple views per viewspace
    Q_ASSERT_X(!m_scrollSynchronisation.synchedViewSpaces.contains(correspondingViewSpace),
               "void KateViewManager::slotSynchroniseScrolling()",
               "Somehow m_synchedViewSpaces already contains the viewspace currently selected");
    m_scrollSynchronisation.synchedViewSpaces.insert(correspondingViewSpace, correspondingViewSpace->currentView());
    // If view is deleted, clear from synchedViewSpaces
    // This has to run before slotChangeSynchedView, lest problems occur
    // Alternatively, use QPointer of the pointers in the value of synchedViewSpaces
    connect(correspondingViewSpace->currentView(),
            &KTextEditor::View::destroyed,
            this,
            &KateViewManager::clearSyncOfDeletedView,
            static_cast<Qt::ConnectionType>(Qt::UniqueConnection | Qt::DirectConnection));
    // Change stored view whenever user changes tab
    connect(this, &KateViewManager::viewChanged, this, &KateViewManager::slotChangeSynchedView, Qt::UniqueConnection);
    // Remove from group of viewSpaces when the viewSpace is deleted
    connect(correspondingViewSpace,
            &KateViewSpace::destroyed,
            this,
            &KateViewManager::removeSyncedViewSpace,
            static_cast<Qt::ConnectionType>(Qt::UniqueConnection | Qt::QueuedConnection));
}

void KateViewManager::clearSyncOfDeletedView(QObject *view)
{
    // make sure that no deleted view is being depended upon, by removing pointers or replacing them with nullptr
    m_scrollSynchronisation.viewScrollInfo.remove(static_cast<KTextEditor::View *>(view));
    const QList<KateViewSpace *> viewSpacesWithDeletedView = m_scrollSynchronisation.synchedViewSpaces.keys(static_cast<KTextEditor::View *>(view));
    for (KateViewSpace *itr : viewSpacesWithDeletedView) {
        m_scrollSynchronisation.synchedViewSpaces.insert(itr, nullptr);
    }
}

void KateViewManager::removeSyncedViewSpace(QObject *obj)
{
    m_scrollSynchronisation.synchedViewSpaces.remove(static_cast<KateViewSpace *>(obj));
}

void KateViewManager::slotChangeSynchedView(KTextEditor::View *currentView)
{
    if (m_scrollSynchronisation.synchedViewSpaces.constFind(activeViewSpace()) == m_scrollSynchronisation.synchedViewSpaces.constEnd()) {
        // If currently selected view is not a part of scroll-synched viewspace do nothing
        return;
    }
    // Remove previous view from scroll-sync
    KTextEditor::View *previousView = m_scrollSynchronisation.synchedViewSpaces.constFind(activeViewSpace()).value();
    if (previousView == currentView) {
        // If this function has been called just because the activeViewSpace was changed then don't
        // waste time
        return;
    }
    // For this to work, need to make sure, previousView exists, or is nullptr
    if (previousView) {
        disconnect(previousView, &KTextEditor::View::verticalScrollPositionChanged, this, &KateViewManager::slotScrollSynchedViews);
        m_scrollSynchronisation.viewScrollInfo.remove(previousView);
    }
    // Add the currentView to scroll-sync
    ScrollSynchronisation::ScrollBarInfo scrollBarInfo = m_scrollSynchronisation.getViewScrollBarInfo(currentView);
    if (!scrollBarInfo.scrollBar) {
        return;
    }
    // Connect scrolling to slotSynchedView
    connect(currentView, &KTextEditor::View::verticalScrollPositionChanged, this, &KateViewManager::slotScrollSynchedViews, Qt::UniqueConnection);
    m_scrollSynchronisation.viewScrollInfo.insert(currentView, scrollBarInfo);
    // Remove from group of views, when view is deleted
    connect(currentView, &KTextEditor::View::destroyed, this, [this](QObject *obj) {
        m_scrollSynchronisation.viewScrollInfo.remove(static_cast<KTextEditor::View *>(obj));
    });
    // Add the new view alongside the viewspace already in list
    m_scrollSynchronisation.synchedViewSpaces[activeViewSpace()] = currentView;
}

void KateViewManager::closeViewSpace(KTextEditor::View *view)
{
    KateViewSpace *space;

    if (view) {
        space = static_cast<KateViewSpace *>(view->parent()->parent());
    } else {
        space = activeViewSpace();
    }
    removeViewSpace(space);
}

void KateViewManager::toggleSplitterOrientation()
{
    KateViewSpace *vs = activeViewSpace();
    if (!vs) {
        return;
    }

    // get current splitter, and abort if null
    auto *currentSplitter = qobject_cast<KateSplitter *>(vs->parent());
    if (!currentSplitter || (currentSplitter->count() == 1)) {
        return;
    }

    // avoid flicker
    KateUpdateDisabler disableUpdates(mainWindow());

    // toggle orientation
    if (currentSplitter->orientation() == Qt::Horizontal) {
        currentSplitter->setOrientation(Qt::Vertical);
    } else {
        currentSplitter->setOrientation(Qt::Horizontal);
    }
}

QList<KTextEditor::View *> KateViewManager::views() const
{
    struct ViewWithLruAge {
        KTextEditor::View *view;
        qint64 age;
    };

    QVarLengthArray<ViewWithLruAge> sorted;

    // extract into a list
    for (const auto &[view, viewData] : m_views) {
        sorted.push_back(ViewWithLruAge{view, viewData.lruAge});
    }

    // sort the views based on lru
    std::sort(sorted.begin(), sorted.end(), [](const ViewWithLruAge &l, const ViewWithLruAge &r) {
        return l.age < r.age;
    });

    // extract the views only and return
    QList<KTextEditor::View *> ret;
    ret.reserve(sorted.size());
    for (auto [view, _] : sorted) {
        ret.push_back(view);
    }
    return ret;
}

void KateViewManager::onViewSpaceEmptied(KateViewSpace *vs)
{
    // If we have more than one viewspaces and this viewspace
    // got empty, remove it
    if (m_viewSpaceList.size() > 1) {
        removeViewSpace(vs);
        return;
    }

    // we want to trigger showing of the welcome view or a new document
    showWelcomeViewOrNewDocumentIfNeeded();
}

int KateViewManager::viewspaceCountForDoc(KTextEditor::Document *doc) const
{
    return std::count_if(m_viewSpaceList.begin(), m_viewSpaceList.end(), [doc](KateViewSpace *vs) {
        return vs->hasDocument(doc);
    });
}

bool KateViewManager::tabsVisible() const
{
    return std::count_if(m_viewSpaceList.begin(), m_viewSpaceList.end(), [](KateViewSpace *vs) {
        return vs->tabCount() > 1;
    });
}

bool KateViewManager::docOnlyInOneViewspace(KTextEditor::Document *doc) const
{
    return (viewspaceCountForDoc(doc) == 1) && !KateApp::self()->documentVisibleInOtherWindows(doc, m_mainWindow);
}

void KateViewManager::setShowUrlNavBar(bool show)
{
    if (show != m_showUrlNavBar) {
        m_showUrlNavBar = show;
        Q_EMIT showUrlNavBarChanged(show);
    }
}

bool KateViewManager::showUrlNavBar() const
{
    return m_showUrlNavBar;
}

bool KateViewManager::viewsInSameViewSpace(KTextEditor::View *view1, KTextEditor::View *view2)
{
    if (!view1 || !view2) {
        return false;
    }
    if (m_viewSpaceList.size() == 1) {
        return true;
    }

    auto *vs1 = static_cast<KateViewSpace *>(view1->parent()->parent());
    auto *vs2 = static_cast<KateViewSpace *>(view2->parent()->parent());
    return vs1 && (vs1 == vs2);
}

void KateViewManager::removeViewSpace(KateViewSpace *viewspace)
{
    // abort if viewspace is 0
    if (!viewspace) {
        return;
    }

    // abort if this is the last viewspace
    if (m_viewSpaceList.size() < 2) {
        return;
    }

    // check if any widget has "shouldClose" method to block closing
    // otherwise we don't need to prompt the user
    auto viewspaceCanClose = [viewspace] {
        const auto widgets = viewspace->widgets();
        for (auto w : widgets) {
            bool shouldClose = true; // by default all widgets are closable
            QMetaObject::invokeMethod(w, "shouldClose", Q_RETURN_ARG(bool, shouldClose));
            if (!shouldClose) {
                return false;
            }
        }
        return true;
    };

    const bool viewspaceHasWidgets = viewspace->hasWidgets();
    if (viewspaceHasWidgets && !viewspaceCanClose()) {
        return;
    }

    // get current splitter
    auto *currentSplitter = qobject_cast<KateSplitter *>(viewspace->parent());

    // no splitter found, bah
    if (!currentSplitter) {
        return;
    }

    //
    // 1. get LRU document list from current viewspace
    // 2. delete current view space
    // 3. add LRU documents from deleted viewspace to new active viewspace
    //

    // backup list of known documents to have tab buttons
    const QList<DocOrWidget> documentList = viewspace->documentList();

    // avoid flicker
    KateUpdateDisabler disableUpdates(mainWindow());

    // delete views of the viewspace
    while (viewspace->currentView()) {
        deleteView(viewspace->currentView());
    }

    if (viewspaceHasWidgets) {
        // emit widget removed
        const auto widgets = viewspace->widgets();
        for (auto w : widgets) {
            Q_EMIT m_mainWindow->wrapper()->widgetRemoved(w);
        }
    }

    // cu viewspace
    m_viewSpaceList.erase(std::remove(m_viewSpaceList.begin(), m_viewSpaceList.end(), viewspace), m_viewSpaceList.end());
    delete viewspace;

    // at this point, the splitter has exactly 1 child
    Q_ASSERT(currentSplitter->count() == 1);

    // if we are not the root splitter, move the child one level up and delete
    // the splitter then.
    if (currentSplitter != this) {
        // get parent splitter
        auto *parentSplitter = qobject_cast<KateSplitter *>(currentSplitter->parent());

        // only do magic if found ;)
        if (parentSplitter) {
            int index = parentSplitter->indexOf(currentSplitter);

            // save current splitter size, as the removal of currentSplitter looses the info
            QList<int> parentSizes = parentSplitter->sizes();
            parentSplitter->insertWidget(index, currentSplitter->widget(0));
            delete currentSplitter;
            // now restore the sizes again
            parentSplitter->setSizes(parentSizes);
        }
    } else if (auto *splitter = qobject_cast<KateSplitter *>(currentSplitter->widget(0))) {
        // we are the root splitter and have only one child, which is also a splitter
        // -> eliminate the redundant splitter and move both children into the root splitter
        QList<int> sizes = splitter->sizes();
        // adapt splitter orientation to the splitter we are about to delete
        currentSplitter->setOrientation(splitter->orientation());
        currentSplitter->addWidget(splitter->widget(0));
        currentSplitter->addWidget(splitter->widget(0));
        delete splitter;
        currentSplitter->setSizes(sizes);
    }

    // add the known documents to the current view space to not loose tab buttons
    auto avs = activeViewSpace();
    for (auto doc : documentList) {
        // TODO: unify handling
        if (doc.doc()) {
            avs->registerDocument(doc.doc());
        }
        // We skip widgets because they will be destroyed when viewspace closes
        // avs->addWidgetAsTab(doc.widget());
    }

    // find the view that is now active.
    KTextEditor::View *v = avs->currentView();
    if (v) {
        activateView(v);
    }

    updateViewSpaceActions();

    Q_EMIT viewChanged(v);
}

void KateViewManager::slotCloseOtherViews()
{
    // avoid flicker
    KateUpdateDisabler disableUpdates(mainWindow());

    const KateViewSpace *active = activeViewSpace();
    const std::vector<KateViewSpace *> viewSpaces = m_viewSpaceList;
    for (KateViewSpace *v : viewSpaces) {
        if (active != v) {
            removeViewSpace(v);
        }
    }
}

void KateViewManager::slotHideOtherViews(bool hideOthers)
{
    // avoid flicker
    KateUpdateDisabler disableUpdates(mainWindow());

    const KateViewSpace *active = activeViewSpace();
    for (KateViewSpace *v : m_viewSpaceList) {
        if (active != v) {
            v->setVisible(!hideOthers);
        }
    }

    // disable the split actions, if we are in single-view-mode
    m_splitViewVert->setDisabled(hideOthers);
    m_splitViewHoriz->setDisabled(hideOthers);
    m_closeView->setDisabled(hideOthers);
    m_closeOtherViews->setDisabled(hideOthers);
    m_toggleSplitterOrientation->setDisabled(hideOthers);
}

/**
 * session config functions
 */
void KateViewManager::saveViewConfiguration(KConfigGroup &config)
{
    // set Active ViewSpace to 0, just in case there is none active (would be
    // strange) and config somehow has previous value set
    config.writeEntry("Active ViewSpace", 0);

    m_splitterIndex = 0;
    saveSplitterConfig(this, config.config(), config.name());
}

void KateViewManager::restoreViewConfiguration(const KConfigGroup &config)
{
    /**
     * remove the single client that is registered at the factory, if any
     */
    if (m_guiMergedView) {
        mainWindow()->guiFactory()->removeClient(m_guiMergedView);
        m_guiMergedView = nullptr;
    }

    /**
     * delete viewspaces, they will delete the views
     */
    qDeleteAll(m_viewSpaceList);
    m_viewSpaceList.clear();

    /**
     * delete mapping of now deleted views
     */
    m_views.clear();

    /**
     * kill all previous existing sub-splitters, just to be sure
     * e.g. important if one restores a config in an existing window with some splitters
     */
    while (count() > 0) {
        delete widget(0);
    }

    // reset lru history, too!
    m_minAge = 0;

    // start recursion for the root splitter (Splitter 0)
    restoreSplitter(config.config(), config.name() + QStringLiteral("-Splitter 0"), this, config.name());

    // finally, make the correct view from the last session active
    size_t lastViewSpace = config.readEntry("Active ViewSpace", 0);
    if (lastViewSpace > m_viewSpaceList.size()) {
        lastViewSpace = 0;
    }
    if (lastViewSpace < m_viewSpaceList.size()) {
        setActiveSpace(m_viewSpaceList.at(lastViewSpace));
        // activate correct view (wish #195435, #188764)
        activateView(m_viewSpaceList.at(lastViewSpace)->currentView());
        // give view the focus to avoid focus stealing by toolviews / plugins
        m_viewSpaceList.at(lastViewSpace)->setFocus();
    }

    // emergency
    if (m_viewSpaceList.empty()) {
        // kill bad children
        while (count()) {
            delete widget(0);
        }

        auto *vs = new KateViewSpace(this, nullptr);
        addWidget(vs);
        vs->setActive(true);
        m_viewSpaceList.push_back(vs);
    } else {
        // remove any empty viewspaces
        // use a copy, m_viewSpaceList will be modified
        const std::vector<KateViewSpace *> copy = m_viewSpaceList;
        for (KateViewSpace *vs : copy) {
            if (vs->documentList().isEmpty()) {
                onViewSpaceEmptied(vs);
            }
        }
    }

    // Restore pinned documents
    // Do it delayed because if we do it immediately, for some reason if the tabbar is not visible
    // See tabbar hide()/show() in KateViewSpace::restoreConfig. This seems to be related, and the
    // reason seems to be that the tabbar has 0 height for whatever reason.
    QTimer::singleShot(0, this, [this] {
        const QList<QUrl> &pinnedDocuments = KateApp::self()->documentManager()->pinnedDocumentUrls();
        for (const auto &url : pinnedDocuments) {
            openUrl(url, QString(), /*activate=*/false, /*ignoreForRecentFiles=*/true);
        }
    });

    updateViewSpaceActions();

    // we want to trigger showing of the welcome view or a new document
    showWelcomeViewOrNewDocumentIfNeeded();
}

QString KateViewManager::saveSplitterConfig(KateSplitter *s, KConfigBase *configBase, const QString &viewConfGrp)
{
    /**
     * avoid to export invisible view spaces
     * else they will stick around for ever in sessions
     * bug 358266 - code initially done during load
     * bug 381433 - moved code to save
     */

    /**
     * create new splitter name, might be not used
     */
    QString grp = QString(viewConfGrp + QStringLiteral("-Splitter %1")).arg(m_splitterIndex);
    ++m_splitterIndex;

    // a KateSplitter has two children, either KateSplitters and/or KateViewSpaces
    // special case: root splitter might have only one child (just for info)
    QStringList childList;
    for (int idx = 0; idx < s->count(); ++idx) {
        // For KateViewSpaces, ask them to save the file list.
        auto obj = s->widget(idx);
        if (auto kvs = qobject_cast<KateViewSpace *>(obj)) {
            auto it = std::find(m_viewSpaceList.begin(), m_viewSpaceList.end(), kvs);
            int viewspaceIdx = (int)std::distance(m_viewSpaceList.begin(), it);

            childList.append(QString(viewConfGrp + QStringLiteral("-ViewSpace %1")).arg(viewspaceIdx));
            kvs->saveConfig(configBase, viewspaceIdx, viewConfGrp);
            // save active viewspace
            if (kvs->isActiveSpace()) {
                KConfigGroup viewConfGroup(configBase, viewConfGrp);
                viewConfGroup.writeEntry("Active ViewSpace", viewspaceIdx);
            }
        }
        // for KateSplitters, recurse
        else if (auto splitter = qobject_cast<KateSplitter *>(obj)) {
            childList.append(saveSplitterConfig(splitter, configBase, viewConfGrp));
        }
    }

    // if only one thing, skip splitter config export, if not top splitter
    if ((s != this) && (childList.size() == 1)) {
        return childList.at(0);
    }

    // Save sizes, orient, children for this splitter
    KConfigGroup config(configBase, grp);
    config.writeEntry("Sizes", s->sizes());
    config.writeEntry("Orientation", int(s->orientation()));
    config.writeEntry("Children", childList);
    return grp;
}

void KateViewManager::restoreSplitter(const KConfigBase *configBase, const QString &group, KateSplitter *parent, const QString &viewConfGrp)
{
    KConfigGroup config(configBase, group);

    parent->setOrientation(static_cast<Qt::Orientation>(config.readEntry("Orientation", int(Qt::Horizontal))));

    const QStringList children = config.readEntry("Children", QStringList());
    for (const auto &str : children) {
        // for a viewspace, create it and open all documents therein.
        if (str.startsWith(viewConfGrp + QStringLiteral("-ViewSpace"))) {
            auto *vs = new KateViewSpace(this, nullptr);
            m_viewSpaceList.push_back(vs);
            // make active so that the view created in restoreConfig has this
            // new view space as parent.
            setActiveSpace(vs);

            parent->addWidget(vs);
            vs->restoreConfig(this, configBase, str);
            vs->show();
        } else {
            // for a splitter, recurse
            auto newContainer = new KateSplitter(parent);

            // we don't allow full collapse, see bug 366014
            newContainer->setChildrenCollapsible(false);

            restoreSplitter(configBase, str, newContainer, viewConfGrp);
        }
    }

    // set sizes
    parent->setSizes(config.readEntry("Sizes", QList<int>()));
    parent->show();
}

void KateViewManager::moveSplitter(Qt::Key key, int repeats)
{
    if (repeats < 1) {
        return;
    }

    KateViewSpace *vs = activeViewSpace();
    if (!vs) {
        return;
    }

    auto *currentSplitter = qobject_cast<KateSplitter *>(vs->parent());

    if (!currentSplitter) {
        return;
    }
    if (currentSplitter->count() == 1) {
        return;
    }

    int move = 0;
    // try to use font height in pixel to move splitter
    {
        KTextEditor::Attribute::Ptr attrib(vs->currentView()->defaultStyleAttribute(KSyntaxHighlighting::Theme::TextStyle::Normal));
        QFontMetrics fm(attrib->font());
        move = fm.height() * repeats;
    }

    auto *currentWidget = static_cast<QWidget *>(vs);
    bool foundSplitter = false;

    // find correct splitter to be moved
    while (currentSplitter && currentSplitter->count() != 1) {
        if (currentSplitter->orientation() == Qt::Horizontal && (key == Qt::Key_Right || key == Qt::Key_Left)) {
            foundSplitter = true;
        }

        if (currentSplitter->orientation() == Qt::Vertical && (key == Qt::Key_Up || key == Qt::Key_Down)) {
            foundSplitter = true;
        }

        // if the views within the current splitter can be resized, resize them
        if (foundSplitter) {
            QList<int> currentSizes = currentSplitter->sizes();
            int index = currentSplitter->indexOf(currentWidget);

            if ((index == 0 && (key == Qt::Key_Left || key == Qt::Key_Up)) || (index == 1 && (key == Qt::Key_Right || key == Qt::Key_Down))) {
                currentSizes[index] -= move;
            }

            if ((index == 0 && (key == Qt::Key_Right || key == Qt::Key_Down)) || (index == 1 && (key == Qt::Key_Left || key == Qt::Key_Up))) {
                currentSizes[index] += move;
            }

            if (index == 0 && (key == Qt::Key_Right || key == Qt::Key_Down)) {
                currentSizes[index + 1] -= move;
            }

            if (index == 0 && (key == Qt::Key_Left || key == Qt::Key_Up)) {
                currentSizes[index + 1] += move;
            }

            if (index == 1 && (key == Qt::Key_Right || key == Qt::Key_Down)) {
                currentSizes[index - 1] += move;
            }

            if (index == 1 && (key == Qt::Key_Left || key == Qt::Key_Up)) {
                currentSizes[index - 1] -= move;
            }

            currentSplitter->setSizes(currentSizes);
            break;
        }

        currentWidget = static_cast<QWidget *>(currentSplitter);
        // the parent of the current splitter will become the current splitter
        currentSplitter = qobject_cast<KateSplitter *>(currentSplitter->parent());
    }
}

void KateViewManager::hideWelcomeView(KateViewSpace *vs)
{
    if (auto welcomeView = qobject_cast<WelcomeView *>(vs ? vs : activeViewSpace()->currentWidget())) {
        QTimer::singleShot(0, welcomeView, [this, welcomeView]() {
            mainWindow()->removeWidget(welcomeView);
        });
    }
}

void KateViewManager::showWelcomeViewOrNewDocumentIfNeeded()
{
    // delay the action
    QTimer::singleShot(0, this, [this]() {
        // we really want to show up only if nothing is in the current view space
        // this guard versus double invocation of this function, too
        if (activeViewSpace() && (activeViewSpace()->currentView() || activeViewSpace()->currentWidget())) {
            return;
        }

        // the user can decide: welcome page or a new untitled document for a new window?
        KSharedConfig::Ptr config = KSharedConfig::openConfig();
        KConfigGroup cgGeneral = KConfigGroup(config, QStringLiteral("General"));
        if (!m_welcomeViewAlreadyShown && cgGeneral.readEntry("Show welcome view for new window", true)) {
            showWelcomeView();
        } else {
            // don't use slotDocumentNew() to avoid dummy new windows in SDI mode
            createView();
        }
        triggerActiveViewFocus();
    });
}

void KateViewManager::showWelcomeView()
{
    // Find and activate existing welcome view
    if (activeViewSpace()) {
        const auto widgets = activeViewSpace()->widgets();
        for (const auto &widget : widgets) {
            if (qobject_cast<WelcomeView *>(widget)) {
                activateWidget(widget);
                return;
            }
        }
    }

    auto welcomeView = new WelcomeView(this);
    mainWindow()->addWidget(welcomeView);
    m_welcomeViewAlreadyShown = true;
}

void KateViewManager::triggerActiveViewFocus()
{
    // delay the focus, needed e.g. on startup
    QTimer::singleShot(0, this, [this]() {
        // no active view space, bad!
        if (!activeViewSpace()) {
            return;
        }

        // else: try to focus either the current active view or widget
        if (auto v = activeViewSpace()->currentView()) {
            v->setFocus();
            return;
        }
        if (auto v = activeViewSpace()->currentWidget()) {
            v->setFocus();
            return;
        }
    });
}

KateViewManager::ScrollSynchronisation::ScrollBarInfo KateViewManager::ScrollSynchronisation::getViewScrollBarInfo(KTextEditor::View *view) const
{
    if (!view) {
        return ScrollBarInfo{};
    }
    const QList<QScrollBar *> scrollBars = view->findChildren<QScrollBar *>();
    // Cannot use std::find_if because QList<>::last() is inclusive
    ScrollSynchronisation::ScrollBarInfo scrollBarInfo;
    scrollBarInfo.scrollBar = [scrollBars] {
        for (auto scrollBar : scrollBars) {
            if (qstrcmp(scrollBar->metaObject()->className(), "KateScrollBar") == 0) {
                return scrollBar;
            }
        }
        Q_ASSERT_X(false,
                   "void "
                   "KateViewManager::ScrollSynchronisation::getViewScrollBarInfo("
                   "KTextEditor::View *currentView)",
                   "No QScrollBar* named \"KateScrollBar\" found in the selected View");
        return static_cast<QScrollBar *>(nullptr);
    }();
    if (scrollBarInfo.scrollBar) {
        scrollBarInfo.initScrollValue = scrollBarInfo.scrollBar->value() - this->referenceScrollValue;
    }
    return scrollBarInfo;
}

#include "moc_kateviewmanager.cpp"
