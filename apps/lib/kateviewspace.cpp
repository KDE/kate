/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2001 Christoph Cullmann <cullmann@kde.org>
   SPDX-FileCopyrightText: 2001 Joseph Wenninger <jowenn@kde.org>
   SPDX-FileCopyrightText: 2001, 2005 Anders Lund <anders.lund@lund.tdcadsl.dk>

   SPDX-License-Identifier: LGPL-2.0-only
*/
#include "kateviewspace.h"

#include "diffwidget.h"
#include "kateapp.h"
#include "katedocmanager.h"
#include "katefileactions.h"
#include "katemainwindow.h"
#include "katesessionmanager.h"
#include "kateupdatedisabler.h"
#include "kateurlbar.h"
#include "kateviewmanager.h"
#include "tabmimedata.h"

#include <KAcceleratorManager>
#include <KActionCollection>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>
#include <KTextEditor/Editor>

#include <QHelpEvent>
#include <QMenu>
#include <QMessageBox>
#include <QRubberBand>
#include <QScopedValueRollback>
#include <QStackedWidget>
#include <QTimer>
#include <QToolButton>
#include <QToolTip>
#include <QWhatsThis>

// BEGIN KateViewSpace
KateViewSpace::KateViewSpace(KateViewManager *viewManager, QWidget *parent)
    : QWidget(parent)
    , m_viewManager(viewManager)
    , m_isActiveSpace(false)
{
    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);

    // BEGIN tab bar
    auto *hLayout = new QHBoxLayout();
    hLayout->setSpacing(0);
    hLayout->setContentsMargins(0, 0, 0, 0);

    // add left <-> right history buttons
    m_historyBack = new QToolButton(this);
    m_historyBack->setToolTip(i18n("Go to Previous Location"));
    m_historyBack->setIcon(QIcon::fromTheme(QStringLiteral("arrow-left")));
    m_historyBack->setAutoRaise(true);
    KAcceleratorManager::setNoAccel(m_historyBack);
    hLayout->addWidget(m_historyBack);
    connect(m_historyBack, &QToolButton::clicked, this, [this] {
        goBack();
    });
    m_historyBack->setEnabled(false);

    m_historyForward = new QToolButton(this);
    m_historyForward->setIcon(QIcon::fromTheme(QStringLiteral("arrow-right")));
    m_historyForward->setToolTip(i18n("Go to Next Location"));
    m_historyForward->setAutoRaise(true);
    KAcceleratorManager::setNoAccel(m_historyForward);
    hLayout->addWidget(m_historyForward);
    connect(m_historyForward, &QToolButton::clicked, this, [this] {
        goForward();
    });
    m_historyForward->setEnabled(false);

    // add tab bar
    m_tabBar = new KateTabBar(this);
    connect(m_tabBar, &KateTabBar::currentChanged, this, &KateViewSpace::changeView);
    connect(m_tabBar, &KateTabBar::tabCloseRequested, this, &KateViewSpace::closeTabRequest, Qt::QueuedConnection);
    connect(m_tabBar, &KateTabBar::contextMenuRequest, this, &KateViewSpace::showContextMenu, Qt::QueuedConnection);
    connect(m_tabBar, &KateTabBar::newTabRequested, this, &KateViewSpace::createNewDocument);
    connect(m_tabBar, &KateTabBar::activateViewSpaceRequested, this, [this] {
        makeActive(true);
    });
    hLayout->addWidget(m_tabBar);

    // add Scroll Sync Indicator
    m_scrollSync = new QToolButton(this);
    m_scrollSync->setHidden(!m_viewManager->hasScrollSync());
    m_scrollSync->setCheckable(true);
    m_scrollSync->setAutoRaise(true);
    m_scrollSync->setToolTip(i18n("Synchronize Scrolling"));
    m_scrollSync->setIcon(QIcon::fromTheme(QStringLiteral("link")));
    m_scrollSync->setWhatsThis(i18n("Synchronize scrolling of this split-view with other synchronized split-views"));
    connect(m_scrollSync, &QToolButton::toggled, this, [this](bool checked) {
        if (currentView()) {
            m_viewManager->slotSynchroniseScrolling(checked, this);
        } else {
            // If invalid, undo check
            const QSignalBlocker blockToggle(m_scrollSync);
            m_scrollSync->setChecked(!checked);
        }
        m_viewManager->updateScrollSyncIndicatorVisibility();
    });
    KAcceleratorManager::setNoAccel(m_scrollSync);
    hLayout->addWidget(m_scrollSync);

    // add quick open
    m_quickOpen = new QToolButton(this);
    m_quickOpen->setAutoRaise(true);
    KAcceleratorManager::setNoAccel(m_quickOpen);
    hLayout->addWidget(m_quickOpen);

    auto mwActionCollection = m_viewManager->mainWindow()->actionCollection();
    // forward tab bar quick open action to global quick open action
    auto *bridge = new QAction(QIcon::fromTheme(QStringLiteral("quickopen")), i18nc("indicator for more documents", "+%1", 100), this);
    m_quickOpen->setDefaultAction(bridge);
    QAction *quickOpen = mwActionCollection->action(QStringLiteral("view_quick_open"));
    Q_ASSERT(quickOpen);
    bridge->setToolTip(quickOpen->toolTip());
    bridge->setWhatsThis(i18n("Click here to switch to the Quick Open view."));
    connect(bridge, &QAction::triggered, quickOpen, &QAction::trigger);

    // add vertical split view space
    m_split = new QToolButton(this);
    m_split->setAutoRaise(true);
    m_split->setPopupMode(QToolButton::InstantPopup);
    m_split->setIcon(QIcon::fromTheme(QStringLiteral("view-split-left-right")));
    m_split->setMenu(new QMenu(this));
    m_split->menu()->addAction(mwActionCollection->action(QStringLiteral("view_split_vert")));
    m_split->menu()->addAction(mwActionCollection->action(QStringLiteral("view_split_horiz")));
    m_split->menu()->addAction(mwActionCollection->action(QStringLiteral("view_split_vert_move_doc")));
    m_split->menu()->addAction(mwActionCollection->action(QStringLiteral("view_split_horiz_move_doc")));
    m_split->menu()->addAction(mwActionCollection->action(QStringLiteral("view_close_current_space")));
    m_split->menu()->addAction(mwActionCollection->action(QStringLiteral("view_close_others")));
    m_split->menu()->addAction(mwActionCollection->action(QStringLiteral("view_hide_others")));
    m_split->setToolTip(i18n("Split View"));
    m_split->setWhatsThis(i18n("Control view space splitting"));

    m_scrollSync->installEventFilter(this); // on click, active this view space
    m_split->installEventFilter(this); // on click, active this view space
    hLayout->addWidget(m_split);

    layout->addLayout(hLayout);

    // on click, active this view space, register this late, we need m_quickOpen and Co. inside the filter
    m_historyBack->installEventFilter(this);
    m_historyForward->installEventFilter(this);
    m_quickOpen->installEventFilter(this);
    m_split->installEventFilter(this);
    // END tab bar

    // more explicit enabling of the view space, see bug 485210
    // we need to setMenu above and watch for aboutToShow to get that right
    connect(m_quickOpen, &QAbstractButton::pressed, this, [this]() {
        makeActive(true);
    });
    connect(m_split->menu(), &QMenu::aboutToShow, this, [this]() {
        makeActive(true);
    });

    m_urlBar = new KateUrlBar(this);

    // like other editors, we try to re-use documents, of not modified
    connect(m_urlBar, &KateUrlBar::openUrlRequested, this, [this](const QUrl &url, Qt::KeyboardModifiers mod) {
        // try if reuse of view make sense
        const bool shiftPress = mod == Qt::ShiftModifier;
        if (!shiftPress && !KateApp::self()->documentManager()->findDocument(url)) {
            if (auto activeView = m_viewManager->activeView()) {
                KateDocumentInfo *info = KateApp::self()->documentManager()->documentInfo(activeView->document());
                if (info && !info->wasDocumentEverModified) {
                    activeView->document()->openUrl(url);
                    return;
                }
            }
        }

        // default: open a new document or switch there
        m_viewManager->openUrl(url);
    });
    layout->addWidget(m_urlBar);

    stack = new QStackedWidget(this);
    stack->setFocus();
    stack->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding));
    layout->addWidget(stack);

    m_group.clear();

    // connect signal to hide/show statusbar
    connect(m_viewManager->mainWindow(), &KateMainWindow::tabBarToggled, this, &KateViewSpace::tabBarToggled);
    connect(m_viewManager, &KateViewManager::showUrlNavBarChanged, this, &KateViewSpace::urlBarToggled);

    connect(this, &KateViewSpace::viewSpaceEmptied, m_viewManager, &KateViewManager::onViewSpaceEmptied);

    // we accept drops (tabs from other viewspaces / windows)
    setAcceptDrops(true);

    m_layout.tabBarLayout = hLayout;
    m_layout.mainLayout = layout;

    // apply config, will init tabbar
    readConfig();

    // handle config changes
    connect(KateApp::self(), &KateApp::configurationChanged, this, &KateViewSpace::readConfig);
    // handle document pin changes
    connect(KateApp::self(), &KateApp::documentPinStatusChanged, this, &KateViewSpace::updateDocumentIcon);

    // ensure we show/hide tabbar if needed
    connect(m_viewManager, &KateViewManager::viewCreated, this, &KateViewSpace::updateTabBar);
    connect(KateApp::self()->documentManager(), &KateDocManager::documentsDeleted, this, &KateViewSpace::updateTabBar);
    connect(m_viewManager->mainWindow()->wrapper(), &KTextEditor::MainWindow::widgetAdded, this, &KateViewSpace::updateTabBar);
    connect(m_viewManager->mainWindow()->wrapper(), &KTextEditor::MainWindow::widgetRemoved, this, &KateViewSpace::updateTabBar);
}

KateViewSpace::~KateViewSpace() = default;

void KateViewSpace::readConfig()
{
    // get tab config
    KSharedConfig::Ptr config = KSharedConfig::openConfig();
    KConfigGroup cgGeneral = KConfigGroup(config, QStringLiteral("General"));
    m_autoHideTabBar = cgGeneral.readEntry("Auto Hide Tabs", KateApp::isKWrite());

    // init tab bar
    tabBarToggled();
}

bool KateViewSpace::eventFilter(QObject *obj, QEvent *event)
{
    if (auto button = qobject_cast<QToolButton *>(obj)) {
        // quick open button: show tool tip with shortcut
        if (button == m_quickOpen && event->type() == QEvent::ToolTip) {
            auto *e = static_cast<QHelpEvent *>(event);
            QAction *quickOpen = m_viewManager->mainWindow()->actionCollection()->action(QStringLiteral("view_quick_open"));
            Q_ASSERT(quickOpen);
            QToolTip::showText(e->globalPos(),
                               button->toolTip() + QStringLiteral(" (%1)").arg(quickOpen->shortcut().toString(QKeySequence::NativeText)),
                               button);
            return true;
        }

        // quick open button: What's This
        if (button == m_quickOpen && event->type() == QEvent::WhatsThis) {
            auto *e = static_cast<QHelpEvent *>(event);
            const int hiddenDocs = hiddenDocuments();
            QString helpText = (hiddenDocs == 0)
                ? i18n("Click here to switch to the Quick Open view.")
                : i18np("Currently, there is one more document open. To see all open documents, switch to the Quick Open view by clicking here.",
                        "Currently, there are %1 more documents open. To see all open documents, switch to the Quick Open view by clicking here.",
                        hiddenDocs);
            QWhatsThis::showText(e->globalPos(), helpText, m_quickOpen);
            return true;
        }

        // on mouse press on view space bar tool buttons: activate this space
        if (event->type() == QEvent::MouseButtonPress) {
            makeActive(true);
        }
    }

    // ensure proper view space activation even for widgets
    if (event->type() == QEvent::FocusIn) {
        makeActive(true);
        return false;
    }

    // keep track of new sub-widgets
    if (event->type() == QEvent::ChildAdded || event->type() == QEvent::ChildRemoved) {
        auto *c = static_cast<QChildEvent *>(event);
        if (c->added()) {
            c->child()->installEventFilter(this);
        } else if (c->removed()) {
            c->child()->removeEventFilter(this);
        }
    }

    return false;
}

void KateViewSpace::updateTabBar()
{
    if (!m_autoHideTabBar || !m_viewManager->mainWindow()->showTabBar()) {
        return;
    }

    // toggle hide/show if state changed
    if (m_viewManager->tabsVisible() != m_tabBar->isVisible()) {
        tabBarToggled();
    }
}

void KateViewSpace::tabBarToggled()
{
    KateUpdateDisabler updatesDisabled(m_viewManager->mainWindow());

    bool show = m_viewManager->mainWindow()->showTabBar();

    // we might want to auto hide if just one document is open
    if (show && m_autoHideTabBar) {
        show = tabCount() > 1 || m_viewManager->tabsVisible();
    }

    const bool urlBarVisible = m_viewManager->showUrlNavBar();

    bool showButtons = true;

    m_tabBar->setVisible(show);
    if (show) {
        m_layout.tabBarLayout->removeWidget(m_urlBar);
        int afterTabLayout = m_layout.mainLayout->indexOf(m_layout.tabBarLayout);
        m_layout.mainLayout->insertWidget(afterTabLayout + 1, m_urlBar);
        showButtons = true;
    } else if (!show && !urlBarVisible) {
        // both hidden, hide buttons as well
        showButtons = false;
    } else if (!show && urlBarVisible) {
        // UrlBar is still visible. Move it up to take the place of tabbar
        int insertAt = m_layout.tabBarLayout->indexOf(m_historyForward) + 1;
        m_layout.tabBarLayout->insertWidget(insertAt, m_urlBar);
        showButtons = true;
    }

    m_historyBack->setVisible(showButtons);
    m_historyForward->setVisible(showButtons);
    m_split->setVisible(showButtons);
    m_quickOpen->setVisible(showButtons);
}

void KateViewSpace::urlBarToggled(bool show)
{
    const bool tabBarVisible = m_tabBar->isVisible();
    bool showButtons = true;
    if (!show && !tabBarVisible) {
        // Tab bar was already hidden, now url bar is also hidden
        // so hide the buttons as well
        showButtons = false;
    } else if (show && !tabBarVisible) {
        // Tabbar is hidden, but we now need to show url bar
        // Move it up to take the place of tabbar
        int insertAt = m_layout.tabBarLayout->indexOf(m_historyForward) + 1;
        m_layout.tabBarLayout->insertWidget(insertAt, m_urlBar);
        // make sure buttons are visible
        showButtons = true;
    }

    m_historyBack->setVisible(showButtons);
    m_historyForward->setVisible(showButtons);
    m_split->setVisible(showButtons);
    m_quickOpen->setVisible(showButtons);
}

KTextEditor::View *KateViewSpace::createView(KTextEditor::Document *doc)
{
    // do nothing if we already have some view
    if (const auto it = m_docToView.find(doc); it != m_docToView.end()) {
        return it->second;
    }

    /**
     * Create a fresh view
     */
    KTextEditor::View *v = doc->createView(stack, m_viewManager->mainWindow()->wrapper());

    // our framework relies on the existence of the status bar
    v->setStatusBarEnabled(true);

    // restore the config of this view if possible
    if (!m_group.isEmpty()) {
        if (KateSession::Ptr as = KateApp::self()->sessionManager()->activeSession(); as->config()) {
            // try id first
            QString id = QString::number(KateApp::self()->documentManager()->documentInfo(v->document())->sessionConfigId);
            QString vgroup = QStringLiteral("%1 %2").arg(m_group, id);
            if (!as->config()->hasGroup(vgroup)) {
                // fallback to url, see KateViewSpace::saveViewConfig
                vgroup = QStringLiteral("%1 %2").arg(m_group, v->document()->url().toString());
            }
            if (as->config()->hasGroup(vgroup)) {
                KConfigGroup cg(as->config(), vgroup);
                v->readSessionConfig(cg);
            }
        }
    }

    {
        // If this doc already has a view then the cursor position
        // might be different from what is in config, try to
        // restore position from one of the views
        const auto views = doc->views();
        if (views.size() > 1) {
            for (auto view : views) {
                if (view != v) {
                    v->setCursorPosition(view->cursorPosition());
                    break;
                }
            }
        }
    }

    connect(v, &KTextEditor::View::cursorPositionChanged, this, [this](KTextEditor::View *view, const KTextEditor::Cursor &newPosition) {
        if (!m_blockAddHistory && view && view->document())
            addPositionToHistory(view->document()->url(), newPosition);
    });

    // register document
    registerDocument(doc);

    // view shall still be not registered
    Q_ASSERT(!m_docToView.contains(doc));

    // insert View into stack
    stack->addWidget(v);
    m_docToView[doc] = v;

    return v;
}

void KateViewSpace::removeView(KTextEditor::View *v)
{
    // remove view mappings, if we have none, this document didn't have any view here at all, just ignore it
    const auto it = m_docToView.find(v->document());
    if (it == m_docToView.end()) {
        return;
    }
    m_docToView.erase(it);

    // ...and now: remove from view space
    stack->removeWidget(v);

    // Remove the doc now!
    // Why do this now? Because otherwise it messes up the LRU
    // because we get two "currentChanged" signals
    // - First signal when we "showView" below
    // - Second comes soon after when v->document() is destroyed
    // Handling (blocking) both signals here is necessary
    m_tabBar->blockSignals(true);
    documentDestroyed(v->document());
    m_tabBar->blockSignals(false);

    // switch to most recently used rather than letting stack choose one
    // (last element could well be v->document() being removed here)
    for (auto rit = m_registeredDocuments.rbegin(); rit != m_registeredDocuments.rend(); ++rit) {
        if (auto doc = rit->doc()) {
            m_viewManager->activateView(doc, this);
            break;
        } else if (auto wid = rit->widget()) {
            activateWidget(wid);
            break;
        }
    }

    // Did we just loose our last doc?
    // Send a delayed signal. Delay is important as we want to kill
    // the viewspace after the view transfer was done
    if (m_tabBar->count() == 0 && m_registeredDocuments.empty()) {
        QMetaObject::invokeMethod(
            this,
            [this] {
                Q_EMIT viewSpaceEmptied(this);
            },
            Qt::QueuedConnection);
    }
}

void KateViewSpace::saveViewConfig(KTextEditor::View *v)
{
    if (!v) {
        return;
    }
    const auto url = v->document()->url();
    if (!url.isEmpty() && !m_group.isEmpty()) {
        const QString vgroup = QStringLiteral("%1 %2").arg(m_group, url.toString());
        const KateSession::Ptr as = KateApp::self()->sessionManager()->activeSession();
        if (as->config()) {
            KConfigGroup viewGroup(as->config(), vgroup);
            v->writeSessionConfig(viewGroup);
        }
    }
}

bool KateViewSpace::showView(DocOrWidget docOrWidget)
{
    /**
     * nothing can be done if we have no view ready here
     */
    auto it = m_docToView.find(docOrWidget.doc());
    if (it == m_docToView.end()) {
        if (!docOrWidget.widget()) {
            return false;
        } else if (docOrWidget.widget() && !m_registeredDocuments.contains(docOrWidget)) {
            qWarning("Unexpected unregistered widget %p, please add it first", (void *)docOrWidget.widget());
            return false;
        }
    }

    /**
     * update mru list order
     */
    const int index = m_registeredDocuments.lastIndexOf(docOrWidget);
    // move view to end of list
    if (index < 0) {
        return false;
    }
    // move view to end of list if not already there
    if (index != m_registeredDocuments.size() - 1) {
        m_registeredDocuments.removeAt(index);
        m_registeredDocuments.push_back(docOrWidget);
    }

    /**
     * show the wanted view
     */
    if (it != m_docToView.end()) {
        KTextEditor::View *kv = it->second;
        stack->setCurrentWidget(kv);
        kv->show();
    } else {
        stack->setCurrentWidget(m_registeredDocuments.back().widget());
    }

    /**
     * we need to avoid that below's index changes will mess with current view
     */
    disconnect(m_tabBar, &KateTabBar::currentChanged, this, &KateViewSpace::changeView);

    /**
     * follow current view
     */
    m_tabBar->setCurrentDocument(docOrWidget);

    // track tab changes again
    connect(m_tabBar, &KateTabBar::currentChanged, this, &KateViewSpace::changeView);
    return true;
}

void KateViewSpace::changeView(int idx)
{
    if (idx == -1) {
        return;
    }

    // make sure we open the view in this view space
    if (!isActiveSpace()) {
        m_viewManager->setActiveSpace(this);
    }

    const DocOrWidget docOrWidget = m_tabBar->tabDocument(idx);

    // tell the view manager to show the view
    m_viewManager->activateView(docOrWidget, this);
}

KTextEditor::View *KateViewSpace::currentView()
{
    // might be 0 if the stack contains no view
    return qobject_cast<KTextEditor::View *>(stack->currentWidget());
}

bool KateViewSpace::isActiveSpace() const
{
    return m_isActiveSpace;
}

void KateViewSpace::setActive(bool active)
{
    m_isActiveSpace = active;
    m_tabBar->setActive(active);
}

void KateViewSpace::makeActive(bool focusCurrentView)
{
    if (!isActiveSpace()) {
        m_viewManager->setActiveSpace(this);
        if (focusCurrentView) {
            if (currentView()) {
                m_viewManager->activateView(currentView()->document(), this);
            } else if (currentWidget()) {
                activateWidget(currentWidget());
            }
        }
    }
    Q_ASSERT(isActiveSpace());
}

void KateViewSpace::registerDocument(KTextEditor::Document *doc)
{
    /**
     * ignore request to register something that is already known
     */
    if (m_registeredDocuments.contains(doc)) {
        return;
    }

    /**
     * remember our new document
     */
    m_registeredDocuments.push_back(doc);

    /**
     * ensure we cleanup after document is deleted, e.g. we remove the tab bar button
     */
    connect(doc, &QObject::destroyed, this, &KateViewSpace::documentDestroyed);

    /**
     * register document is used in places that don't like view creation
     * therefore we must ensure the currentChanged doesn't trigger that
     */
    disconnect(m_tabBar, &KateTabBar::currentChanged, this, &KateViewSpace::changeView);

    /**
     * create the tab for this document, if necessary
     */
    m_tabBar->setCurrentDocument(doc);

    /**
     * handle later document state changes
     */
    connect(doc, &KTextEditor::Document::documentNameChanged, this, &KateViewSpace::updateDocumentName);
    connect(doc, &KTextEditor::Document::documentUrlChanged, this, &KateViewSpace::updateDocumentUrl);
    connect(doc, &KTextEditor::Document::modifiedChanged, this, &KateViewSpace::updateDocumentIcon);
    // needed to get mime-type udate right, see bug 489452
    connect(doc, &KTextEditor::Document::reloaded, this, &KateViewSpace::updateDocumentIcon);

    /**
     * allow signals again, now that the tab is there
     */
    connect(m_tabBar, &KateTabBar::currentChanged, this, &KateViewSpace::changeView);
}

void KateViewSpace::closeDocument(KTextEditor::Document *doc)
{
    auto it = m_docToView.find(doc);
    if (it != m_docToView.end() && it->first) {
        saveViewConfig(it->second);
    }

    // If this is the only view of the document,
    // OR the doc has no views yet
    // just close the document and it will take
    // care of removing the view + cleaning up the doc
    if (m_viewManager->docOnlyInOneViewspace(doc)) {
        m_viewManager->slotDocumentClose(doc);
    } else {
        // KTE::view for this tab has been created yet?
        if (it != m_docToView.end()) {
            // - We have a view for this doc in this viewspace
            // - We have other views of this doc in other viewspaces
            // - Just remove the view in this viewspace
            m_viewManager->deleteView(it->second);
        } else {
            // We don't have a view for this doc in this viewspace
            // Just remove the document
            documentDestroyed(doc);
        }
    }

    /**
     * if this was the last doc, let viewManager know we are empty
     */
    if (m_registeredDocuments.empty() && m_tabBar->count() == 0) {
        Q_EMIT viewSpaceEmptied(this);
    }
}

bool KateViewSpace::acceptsDroppedTab(const QMimeData *md)
{
    if (auto tabMimeData = qobject_cast<const TabMimeData *>(md)) {
        return this != tabMimeData->sourceVS && // must not be same viewspace
                                                // for now we don't support dropping into different windows if no document
            ((viewManager() == tabMimeData->sourceVS->viewManager()) || tabMimeData->doc.doc()) && !hasDocument(tabMimeData->doc);
    }
    return TabMimeData::hasValidData(md);
}

void KateViewSpace::dragEnterEvent(QDragEnterEvent *e)
{
    if (acceptsDroppedTab(e->mimeData())) {
        m_dropIndicator.reset(new QRubberBand(QRubberBand::Rectangle, this));
        m_dropIndicator->setGeometry(rect());
        m_dropIndicator->show();
        e->acceptProposedAction();
        return;
    }

    QWidget::dragEnterEvent(e);
}

void KateViewSpace::dragLeaveEvent(QDragLeaveEvent *e)
{
    m_dropIndicator.reset();
    QWidget::dragLeaveEvent(e);
}

void KateViewSpace::dropEvent(QDropEvent *e)
{
    if (auto mimeData = qobject_cast<const TabMimeData *>(e->mimeData())) {
        if (viewManager() == mimeData->sourceVS->viewManager()) {
            m_viewManager->moveViewToViewSpace(this, mimeData->sourceVS, mimeData->doc);
        } else {
            Q_ASSERT(mimeData->doc.doc()); // we ensure that in acceptsDroppedTab
            auto sourceView = mimeData->sourceVS->findViewForDocument(mimeData->doc.doc());
            auto view = m_viewManager->activateView(mimeData->doc.doc(), this);
            if (sourceView && view) {
                view->setCursorPosition(sourceView->cursorPosition());
            }
        }
        m_dropIndicator.reset();
        e->accept();
        return;
    }
    std::optional<TabMimeData::DroppedData> droppedData = TabMimeData::data(e->mimeData());
    if (droppedData.has_value()) {
        auto doc = KateApp::self()->documentManager()->openUrl(droppedData.value().url);
        auto view = m_viewManager->activateView(doc, this);
        if (view) {
            view->setCursorPosition({droppedData.value().line, droppedData.value().col});
            m_dropIndicator.reset();
            e->accept();
            return;
        }
    }

    QWidget::dropEvent(e);
}

bool KateViewSpace::hasDocument(DocOrWidget doc) const
{
    return m_registeredDocuments.contains(doc);
}

QWidget *KateViewSpace::takeView(DocOrWidget docOrWidget)
{
    QWidget *ret;
    if (docOrWidget.doc()) {
        auto it = m_docToView.find(docOrWidget.doc());
        if (it == m_docToView.end()) {
            qWarning("Unexpected unable to find a view for %p", (void *)docOrWidget.qobject());
            return nullptr;
        }
        ret = it->second;
        // remove it from the stack
        stack->removeWidget(ret);
        // remove it from our doc->view mapping
        m_docToView.erase(it);
        documentDestroyed(docOrWidget.doc());
    } else if (docOrWidget.widget()) {
        stack->removeWidget(docOrWidget.widget());
        m_registeredDocuments.removeAll(docOrWidget);
        m_tabBar->removeDocument(docOrWidget);
        ret = docOrWidget.widget();
    } else {
        qWarning("Unexpected docOrWidget: %p", (void *)docOrWidget.qobject());
        ret = nullptr;
        Q_UNREACHABLE();
    }

    // Did we just loose our last doc?
    // Send a delayed signal. Delay is important as we want to kill
    // the viewspace after the view transfer was done
    if (m_tabBar->count() == 0 && m_registeredDocuments.empty()) {
        QMetaObject::invokeMethod(
            this,
            [this] {
                Q_EMIT viewSpaceEmptied(this);
            },
            Qt::QueuedConnection);
    }

    return ret;
}

void KateViewSpace::addView(QWidget *w)
{
    // We must not already have this widget
    Q_ASSERT(stack->indexOf(w) == -1);

    if (auto v = qobject_cast<KTextEditor::View *>(w)) {
        registerDocument(v->document());
        m_docToView[v->document()] = v;
        stack->addWidget(v);
    } else {
        addWidgetAsTab(w);
    }
}

void KateViewSpace::documentDestroyed(QObject *doc)
{
    /**
     * WARNING: this pointer is half destroyed
     * only good enough to check pointer value e.g. for hashes
     */
    auto *invalidDoc = static_cast<KTextEditor::Document *>(doc);
    if (m_registeredDocuments.removeAll(invalidDoc) == 0) {
        // do nothing if this document wasn't registered for this viewspace
        return;
    }

    /**
     * we shall have no views for this document at this point in time!
     */
    Q_ASSERT(!m_docToView.contains(invalidDoc));

    // disconnect entirely
    disconnect(doc, nullptr, this, nullptr);

    /**
     * remove the tab for this document, if existing
     */
    m_tabBar->removeDocument(invalidDoc);
}

void KateViewSpace::updateDocumentName(KTextEditor::Document *doc)
{
    // update tab button if available, might not be the case for tab limit set!wee
    const int buttonId = m_tabBar->documentIdx(doc);
    if (buttonId >= 0) {
        // BUG: 441278 We need to escape the & because it is used for accelerators/shortcut mnemonic by default
        QString tabName = doc->documentName();
        tabName.replace(QLatin1Char('&'), QLatin1String("&&"));
        m_tabBar->setTabText(buttonId, tabName);
    }
}

void KateViewSpace::updateDocumentIcon(KTextEditor::Document *doc)
{
    // update tab button if available, might not be the case for tab limit set!
    const int buttonId = m_tabBar->documentIdx(doc);
    if (buttonId >= 0) {
        m_tabBar->setModifiedStateIcon(buttonId, doc);
    }
}

void KateViewSpace::updateDocumentUrl(KTextEditor::Document *doc)
{
    // update tab button if available, might not be the case for tab limit set!
    const int buttonId = m_tabBar->documentIdx(doc);
    if (buttonId >= 0) {
        m_tabBar->setTabToolTip(buttonId, doc->url().toDisplayString(QUrl::PreferLocalFile));
        m_tabBar->setModifiedStateIcon(buttonId, doc);
    }

    // update recent files for e.g. saveAs, bug 486203
    m_viewManager->mainWindow()->addRecentOpenedFile(doc->url());
}

void KateViewSpace::closeTabRequest(int idx)
{
    const auto docOrWidget = m_tabBar->tabData(idx).value<DocOrWidget>();
    auto *doc = docOrWidget.doc();
    if (!doc) {
        auto widget = docOrWidget.widget();
        if (!widget) {
            Q_ASSERT(false);
            return;
        }
        removeWidget(widget);
        return;
    }

    closeDocument(doc);
}

void KateViewSpace::removeWidget(QWidget *w)
{
    bool shouldClose = true;
    QMetaObject::invokeMethod(w, "shouldClose", Q_RETURN_ARG(bool, shouldClose));
    if (shouldClose) {
        w->removeEventFilter(this);
        for (auto c : w->findChildren<QWidget *>()) {
            c->removeEventFilter(this);
        }

        stack->removeWidget(w);
        m_registeredDocuments.removeOne(w);

        DocOrWidget widget(w);
        const auto idx = m_tabBar->documentIdx(widget);
        m_tabBar->blockSignals(true);
        m_tabBar->removeDocument(widget);
        m_tabBar->blockSignals(false);

        w->deleteLater();
        Q_EMIT m_viewManager->mainWindow()->wrapper()->widgetRemoved(w);

        // If some tab was removed, switch to most recently used doc
        if (idx >= 0) {
            // switch to most recently used doc
            for (auto rit = m_registeredDocuments.rbegin(); rit != m_registeredDocuments.rend(); ++rit) {
                if (auto doc = rit->doc()) {
                    m_viewManager->activateView(doc, this);
                    break;
                } else if (auto wid = rit->widget()) {
                    activateWidget(wid);
                    break;
                }
            }
        }

        // if this was the last doc, let viewManager know we are empty
        if (m_registeredDocuments.empty() && m_tabBar->count() == 0) {
            Q_EMIT viewSpaceEmptied(this);
        }
    }
}

void KateViewSpace::createNewDocument()
{
    // make sure we open the view in this view space
    if (!isActiveSpace()) {
        m_viewManager->setActiveSpace(this);
    }

    // create document
    KTextEditor::Document *doc = KateApp::self()->documentManager()->createDoc();

    // tell the view manager to show the document
    m_viewManager->activateView(doc, this);
}

void KateViewSpace::focusPrevTab()
{
    const int id = m_tabBar->prevTab();
    if (id >= 0) {
        changeView(id);
    }
}

void KateViewSpace::focusNextTab()
{
    const int id = m_tabBar->nextTab();
    if (id >= 0) {
        changeView(id);
    }
}

void KateViewSpace::focusTab(const int id)
{
    if (m_tabBar->containsTab(id)) {
        changeView(id);
    }
}

void KateViewSpace::addWidgetAsTab(QWidget *widget)
{
    // ensure we keep track of focus changes, KTextEditor::View has some extra focusIn signal
    widget->installEventFilter(this);
    for (auto c : widget->findChildren<QWidget *>()) {
        c->installEventFilter(this);
    }

    stack->addWidget(widget);
    // disconnect changeView, we are just adding the widget here
    // and don't want any unnecessary viewChanged signals
    disconnect(m_tabBar, &KateTabBar::currentChanged, this, &KateViewSpace::changeView);
    m_tabBar->setCurrentDocument(widget);
    connect(m_tabBar, &KateTabBar::currentChanged, this, &KateViewSpace::changeView);
    stack->setCurrentWidget(widget);
    m_registeredDocuments.push_back(widget);
    updateTabBar();
}

bool KateViewSpace::hasWidgets() const
{
    return stack->count() > (int)m_docToView.size();
}

QWidget *KateViewSpace::currentWidget()
{
    if (auto w = stack->currentWidget()) {
        return qobject_cast<KTextEditor::View *>(w) ? nullptr : w;
    }
    return nullptr;
}

QWidgetList KateViewSpace::widgets() const
{
    QWidgetList widgets;
    for (int i = 0; i < m_tabBar->count(); ++i) {
        auto widget = m_tabBar->tabData(i).value<DocOrWidget>().widget();
        if (widget) {
            widgets << widget;
        }
    }
    return widgets;
}

bool KateViewSpace::closeTabWithWidget(QWidget *widget)
{
    int index = m_registeredDocuments.indexOf(widget);
    if (index < 0) {
        return false;
    }
    removeWidget(widget);
    return true;
}

bool KateViewSpace::activateWidget(QWidget *widget)
{
    if (stack->indexOf(widget) == -1) {
        return false;
    }

    stack->setCurrentWidget(widget);
    m_registeredDocuments.removeOne(widget);
    m_registeredDocuments.push_back(widget);
    m_tabBar->setCurrentDocument(DocOrWidget(widget));
    widget->setFocus();
    return true;
}

void KateViewSpace::focusNavigationBar()
{
    if (!m_urlBar->isHidden()) {
        m_urlBar->open();
    }
}

void KateViewSpace::toggleScrollSync()
{
    m_scrollSync->toggle();
}

void KateViewSpace::setScrollSyncToolButtonVisible(bool visible)
{
    m_scrollSync->setHidden(!visible);
}

void KateViewSpace::detachDocument(KTextEditor::Document *doc)
{
    auto mainWindow = viewManager()->mainWindow()->newWindow();
    mainWindow->viewManager()->openViewForDoc(doc);

    // use single shot as this action can trigger deletion of this viewspace!
    QTimer::singleShot(0, this, [this, doc]() {
        closeDocument(doc);
    });
}

void KateViewSpace::addPositionToHistory(const QUrl &url, KTextEditor::Cursor c, bool calledExternally)
{
    // We don't care about invalid urls (Fixed Diff View / Untitled docs)
    if (!url.isValid()) {
        return;
    }

    // if same line, remove last entry
    // If new pos is same as "current pos", replace it with new one
    bool currPosIsInSameLine = false;
    if (currentLocation < m_locations.size()) {
        const auto &currentLoc = m_locations.at(currentLocation);
        currPosIsInSameLine = currentLoc.url == url && currentLoc.cursor.line() == c.line();
    }

    // Check if the location is at least "viewLineCount" away from the "current" position in m_locations
    if (const auto view = m_viewManager->activeView();
        view && !calledExternally && currentLocation < m_locations.size() && m_locations.at(currentLocation).url == url) {
        const int currentLine = m_locations.at(currentLocation).cursor.line();
        const int newPosLine = c.line();
        const int viewLineCount = view->lastDisplayedLine() - view->firstDisplayedLine();
        const int lowerBound = currentLine - viewLineCount;
        const int upperBound = currentLine + viewLineCount;
        if (lowerBound <= newPosLine && newPosLine <= upperBound) {
            if (currPosIsInSameLine) {
                m_locations[currentLocation].cursor = c;
            }
            return;
        }
    }

    if (currPosIsInSameLine) {
        m_locations[currentLocation].cursor.setColumn(c.column());
        return;
    }

    // we are in the middle of jumps somewhere?
    if (!m_locations.empty() && currentLocation + 1 < m_locations.size()) {
        // erase all forward history
        m_locations.erase(m_locations.begin() + currentLocation + 1, m_locations.end());
    }

    /** this is our new forward **/

    m_locations.push_back({url, c});
    // set currentLocation as last
    currentLocation = m_locations.size() - 1;
    // disable forward button as we are at the end now
    m_historyForward->setEnabled(false);
    Q_EMIT m_viewManager->historyForwardEnabled(false);

    // renable back
    if (currentLocation > 0) {
        m_historyBack->setEnabled(true);
        Q_EMIT m_viewManager->historyBackEnabled(true);
    }

    // limit size to 50, remove first 10
    int toErase = (int)m_locations.size() - 50;
    if (toErase > 0) {
        m_locations.erase(m_locations.begin(), m_locations.begin() + toErase);
        currentLocation -= toErase;
    }
}

int KateViewSpace::hiddenDocuments() const
{
    const auto hiddenDocs = KateApp::self()->documentManager()->documentList().size() - m_tabBar->count();
    Q_ASSERT(hiddenDocs >= 0);
    return hiddenDocs;
}

void KateViewSpace::showContextMenu(int idx, const QPoint &globalPos)
{
    QMenu menu(this);
    buildContextMenu(idx, menu);
    if (!menu.isEmpty()) {
        menu.exec(globalPos);
    }
}

void KateViewSpace::buildContextMenu(int tabIndex, QMenu &menu)
{
    // right now, show no context menu on empty tab bar space
    if (tabIndex < 0) {
        return;
    }

    auto docOrWidget = m_tabBar->tabDocument(tabIndex);
    auto activeView = KTextEditor::Editor::instance()->application()->activeMainWindow()->activeView();
    auto activeDocument = activeView ? activeView->document() : nullptr; // used for compareUsing which is used with another
    if (!docOrWidget.doc()) {
        // This tab is holding some other widget
        // Show only "close tab" for now
        // maybe later allow adding context menu entries from the widgets
        // if needed
        auto aCloseTab = menu.addAction(QIcon::fromTheme(QStringLiteral("tab-close")), i18n("Close Tab"));
        connect(aCloseTab, &QAction::triggered, this, [this, tabIndex] {
            QTimer::singleShot(0, this, [this, tabIndex]() {
                closeTabRequest(tabIndex);
            });
        });
        return;
    }
    auto *doc = docOrWidget.doc();

    auto addActionFromCollection = [this](QMenu *menu, const char *action_name, Qt::ConnectionType connType = Qt::AutoConnection) {
        QAction *action = m_viewManager->mainWindow()->action(QLatin1StringView(action_name));
        auto newAction = menu->addAction(action->icon(), action->text());
        connect(newAction, &QAction::triggered, action, &QAction::trigger, connType);
        return newAction;
    };

    QAction *aCloseTab = menu.addAction(QIcon::fromTheme(QStringLiteral("tab-close")), i18n("&Close Document"));
    connect(aCloseTab, &QAction::triggered, this, [this, tabIndex] {
        QTimer::singleShot(0, this, [this, tabIndex]() {
            closeTabRequest(tabIndex);
        });
    });

    QAction *aCloseOthers = menu.addAction(QIcon::fromTheme(QStringLiteral("tab-close-other")), i18n("Close Other &Documents"));
    auto onCloseOthers = [doc] {
        KateApp::self()->documentManager()->closeOtherDocuments(doc);
    };
    connect(aCloseOthers, &QAction::triggered, this, onCloseOthers);

    QAction *aCloseAll = menu.addAction(QIcon::fromTheme(QStringLiteral("tab-close-all")), i18n("Close &All Documents"));
    connect(aCloseAll, &QAction::triggered, this, [this] {
        QTimer::singleShot(0, this, []() {
            KateApp::self()->documentManager()->closeAllDocuments();
        });
    });

    menu.addAction(m_viewManager->mainWindow()->actionCollection()->action(QStringLiteral("reopen_latest_closed_document")));
    menu.addSeparator();

    menu.addAction(KStandardAction::open(m_viewManager, &KateViewManager::slotDocumentOpen, this));

    QAction *aDetachTab = menu.addAction(QIcon::fromTheme(QStringLiteral("tab-detach")), i18n("D&etach Document"));
    connect(aDetachTab, &QAction::triggered, this, [this, doc] {
        detachDocument(doc);
    });
    menu.addSeparator();
    aDetachTab->setWhatsThis(i18n("Opens the document in a new window and closes it in the current window"));
    menu.addSeparator();

    QAction *aCopyPath = addActionFromCollection(&menu, "file_copy_filepath");
    QAction *aCopyFilename = addActionFromCollection(&menu, "file_copy_filename");
    QAction *aOpenFolder = addActionFromCollection(&menu, "file_open_containing_folder");
    QAction *aFileProperties = addActionFromCollection(&menu, "file_properties");
    menu.addSeparator();
    QAction *aRenameFile = addActionFromCollection(&menu, "file_rename");

    // use QueuedConnection as this action can trigger deletion of this viewspace!
    QAction *aDeleteFile = addActionFromCollection(&menu, "file_delete", Qt::QueuedConnection);

    menu.addSeparator();
    QAction *compare = menu.addAction(i18n("Compare with Active Document"));
    compare->setIcon(QIcon::fromTheme(QStringLiteral("vcs-diff")));
    connect(compare, &QAction::triggered, this, [this, activeDocument, doc] {
        DiffWidgetManager::diffDocs(activeDocument, doc, m_viewManager->mainWindow()->wrapper());
    });

    auto *compareUsing = new QMenu(i18n("Compare with Active Document Using"), &menu);
    compareUsing->setIcon(QIcon::fromTheme(QStringLiteral("vcs-diff")));
    menu.addMenu(compareUsing);

    if (KateApp::isKate() && !doc->url().isEmpty()) {
        QAction *pinAction;
        auto kateApp = KateApp::self();
        if (kateApp->documentManager()->isDocumentPinned(doc)) {
            pinAction = menu.addAction(QIcon::fromTheme(QStringLiteral("window-unpin")), i18n("Unpin Document"));
        } else {
            pinAction = menu.addAction(QIcon::fromTheme(QStringLiteral("pin")), i18n("Pin Document"));
        }

        connect(pinAction, &QAction::triggered, KateApp::self()->documentManager(), [doc] {
            KateApp::self()->documentManager()->togglePinDocument(doc);
        });
    }

    // if we have other documents, allow to close them
    aCloseOthers->setEnabled(KateApp::self()->documentManager()->documentList().size() > 1);

    // make it feasible to detach tabs if we have more then one
    aDetachTab->setEnabled(m_tabBar->count() > 1);

    if (doc->url().isEmpty()) {
        aCopyPath->setEnabled(false);
        aCopyFilename->setEnabled(false);
        aOpenFolder->setEnabled(false);
        aRenameFile->setEnabled(false);
        aDeleteFile->setEnabled(false);
        aFileProperties->setEnabled(false);
        compareUsing->setEnabled(false);
    }

    // both documents must have urls and must not be the same to have the compare feature enabled
    if (!activeDocument || activeDocument->url().isEmpty() || activeDocument == doc) {
        compare->setEnabled(false);
        compareUsing->setEnabled(false);
    }

    if (compareUsing->isEnabled()) {
        for (KateFileActions::DiffTool &diffTool : KateFileActions::supportedDiffTools()) {
            QAction *compareAction = compareUsing->addAction(diffTool.name);
            connect(compareAction, &QAction::triggered, this, [tool = diffTool.path, doc, activeDocument] {
                KateFileActions::compareWithExternalProgram(activeDocument, doc, tool);
            });

            // we use the full path to safely execute the tool, disable action if no full path => tool not found
            compareAction->setEnabled(!diffTool.path.isEmpty());
        }
    }
}

void KateViewSpace::saveConfig(KConfigBase *config, int myIndex, const QString &viewConfGrp)
{
    const QString groupname = QString(viewConfGrp + QStringLiteral("-ViewSpace %1")).arg(myIndex);
    // Ensure that we store the group name. It is used to save config for views inside
    // the viewspace. See KateViewSpace::saveViewConfig that is called when a view is closed
    m_group = groupname;

    // aggregate all registered documents & views in view space
    // we need even the documents without tabs to avoid that we later have issues with closing
    // documents and not the right ones being used as replacement if you have a limit on tabs
    // we first store all stuff without visible tabs and then the tabs in tab order to proper restore
    // that order
    QVarLengthArray<KTextEditor::View *> views;
    QStringList docList;
    const auto handleDoc = [this, &views, &docList](auto doc) {
        // we can only store stuff about documents that get an id
        const int sessionId = KateApp::self()->documentManager()->documentInfo(doc)->sessionConfigId;
        if (sessionId < 0) {
            return;
        }

        docList << QString::number(sessionId);
        auto it = m_docToView.find(doc);
        if (it != m_docToView.end()) {
            views.push_back(it->second);
        }
    };

    const QList<KTextEditor::Document *> lruDocList = m_tabBar->lruSortedDocuments();
    const QList<DocOrWidget> tabs = m_tabBar->documentList();
    docList.reserve(tabs.size() + lruDocList.size());

    for (auto doc : lruDocList) {
        // skip stuff with visible tabs
        if (std::find(tabs.begin(), tabs.end(), DocOrWidget(doc)) == tabs.end()) {
            handleDoc(doc);
        }
    }
    for (DocOrWidget docOrWidget : tabs) {
        // only docs are of interest
        if (KTextEditor::Document *doc = docOrWidget.doc()) {
            handleDoc(doc);
        }
    }

    KConfigGroup group(config, groupname);
    group.writeEntry("Documents", docList);
    group.writeEntry("Count", static_cast<int>(views.size()));

    if (currentView()) {
        group.writeEntry("Active View", QString::number(KateApp::self()->documentManager()->documentInfo(currentView()->document())->sessionConfigId));
    }

    // Save file list, including cursor position in this instance.
    int idx = 0;
    for (auto view : views) {
        const int sessionId = KateApp::self()->documentManager()->documentInfo(view->document())->sessionConfigId;
        if (sessionId >= 0) {
            char key[128];
            *std::format_to(key, "View {}", idx) = '\0';
            group.writeEntry(key, sessionId);

            // view config, group: "ViewSpace <n> id"
            QString vgroup = QStringLiteral("%1 %2").arg(groupname, QString::number(sessionId));
            KConfigGroup viewGroup(config, vgroup);
            view->writeSessionConfig(viewGroup);
        }

        ++idx;
    }
}

void KateViewSpace::restoreConfig(KateViewManager *viewMan, const KConfigBase *config, const QString &groupname)
{
    KConfigGroup group(config, groupname);

    // workaround for the weird bug where the tabbar sometimes becomes invisible after opening a session via the session chooser dialog or the --start cmd
    // option
    // TODO: Debug the actual reason for the bug. See https://invent.kde.org/utilities/kate/-/merge_requests/189
    m_tabBar->hide();
    m_tabBar->show();

    // set back bar status to configured variant
    tabBarToggled();

    // Disable opening tabs to the right of current during session restore
    // as it can mess up the tab order during session restore and because we
    // open tabs to right as a result of user action
    const auto openTabsToRightOfCurrent = m_tabBar->openTabsToRightOfCurrent();
    m_tabBar->setOpenTabsToRightOfCurrentEnabled(false);

    // restore Document list so that all tabs from the last session reappear
    const QStringList docList = group.readEntry("Documents", QStringList());
    for (const auto &idOrUrl : docList) {
        // ignore stuff with no id or url
        if (idOrUrl.isEmpty()) {
            continue;
        }

        // if id use that, is the new format to allow to differentiate between different untitled documents
        bool isInt = false;
        KTextEditor::Document *doc = nullptr;
        if (const int id = idOrUrl.toInt(&isInt, 10); isInt) {
            doc = KateApp::self()->documentManager()->findDocumentForSessionConfigId(id);
        } else {
            doc = KateApp::self()->documentManager()->findDocument(QUrl(idOrUrl));
        }
        if (doc) {
            registerDocument(doc);
        }
    }

    m_tabBar->setOpenTabsToRightOfCurrentEnabled(openTabsToRightOfCurrent);

    // Fix the tab order. If the number of docs cross the tab limit,
    // then the first tab after the limit will go to position 0, which
    // might be incorrect. Below we fix the tab order by traversing the
    // m_registeredDocuments and modifying the tabs in tab bar.
    // docList: 0 1 2 3
    // m_registeredDocuments: 0 1 2 3
    const QList<DocOrWidget> tabsList = m_tabBar->documentList();
    if (tabsList.size() < docList.size()) {
        int firstDocIndex = docList.size() - tabsList.size();
        auto docIt = m_registeredDocuments.begin() + firstDocIndex;
        for (int i = 0; i < tabsList.size() && docIt != m_registeredDocuments.end(); i++, ++docIt) {
            m_tabBar->setTabDocument(i, *docIt);
        }
    }

    // restore active view properties
    if (const QString idOrUrl = group.readEntry("Active View"); !idOrUrl.isEmpty()) {
        // if id use that, is the new format to allow to differentiate between different untitled documents
        bool isInt = false;
        KTextEditor::Document *doc = nullptr;
        if (const int id = idOrUrl.toInt(&isInt, 10); isInt) {
            doc = KateApp::self()->documentManager()->findDocumentForSessionConfigId(id);
        } else {
            doc = KateApp::self()->documentManager()->findDocument(QUrl(idOrUrl));
        }
        if (doc) {
            if (auto view = viewMan->createView(doc, this)) {
                // view config, group: "ViewSpace <n> url"
                const QString vgroup = QStringLiteral("%1 %2").arg(groupname, idOrUrl);
                view->readSessionConfig(KConfigGroup(config, vgroup));
                m_tabBar->setCurrentDocument(doc);
            }
        }
    }

    // ensure we update the urlbar at least once
    m_urlBar->updateForDocument(currentView() ? currentView()->document() : nullptr);

    m_group = groupname; // used for restoring view configs later
}

void KateViewSpace::goBack()
{
    if (m_locations.empty() || currentLocation <= 0) {
        currentLocation = 0;
        return;
    }

    const auto &location = m_locations.at(currentLocation - 1);
    currentLocation--;

    if (currentLocation <= 0) {
        m_historyBack->setEnabled(false);
        Q_EMIT m_viewManager->historyBackEnabled(false);
    }

    if (auto v = m_viewManager->activeView()) {
        if (v->document()->url() == location.url) {
            QScopedValueRollback blocker(m_blockAddHistory, true);
            v->setCursorPosition(location.cursor);
            // enable forward
            m_historyForward->setEnabled(true);
            Q_EMIT m_viewManager->historyForwardEnabled(true);
            return;
        }
    }

    auto v = m_viewManager->openUrlWithView(location.url, QString());
    QScopedValueRollback blocker(m_blockAddHistory, true);
    v->setCursorPosition(location.cursor);
    // enable forward in viewspace + mainwindow
    m_historyForward->setEnabled(true);
    Q_EMIT m_viewManager->historyForwardEnabled(true);
}

bool KateViewSpace::isHistoryBackEnabled() const
{
    return m_historyBack->isEnabled();
}

bool KateViewSpace::isHistoryForwardEnabled() const
{
    return m_historyForward->isEnabled();
}

void KateViewSpace::goForward()
{
    if (m_locations.empty()) {
        return;
    }

    // We are already at the last position
    if (currentLocation >= m_locations.size() - 1) {
        return;
    }

    const auto &location = m_locations.at(currentLocation + 1);
    currentLocation++;

    if (currentLocation + 1 >= m_locations.size()) {
        Q_EMIT m_viewManager->historyForwardEnabled(false);
        m_historyForward->setEnabled(false);
    }

    if (!location.url.isValid() || !location.cursor.isValid()) {
        m_locations.erase(m_locations.begin() + currentLocation);
        return;
    }

    m_historyBack->setEnabled(true);
    Q_EMIT m_viewManager->historyBackEnabled(true);

    if (auto v = m_viewManager->activeView()) {
        if (v->document()->url() == location.url) {
            QScopedValueRollback blocker(m_blockAddHistory, true);
            v->setCursorPosition(location.cursor);
            return;
        }
    }

    auto v = m_viewManager->openUrlWithView(location.url, QString());
    QScopedValueRollback blocker(m_blockAddHistory, true);
    v->setCursorPosition(location.cursor);
}

// END KateViewSpace

#include "moc_kateviewspace.cpp"
