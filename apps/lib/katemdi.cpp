/*
    SPDX-FileCopyrightText: 2005 Christoph Cullmann <cullmann@kde.org>
    SPDX-FileCopyrightText: 2002, 2003 Joseph Wenninger <jowenn@kde.org>

    GUIClient partly based on ktoolbarhandler.cpp: SPDX-FileCopyrightText: 2002 Simon Hausmann <hausmann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "katemdi.h"
#include "kateapp.h"
#include "katedebug.h"

#include <KAcceleratorManager>
#include <KActionCollection>
#include <KActionMenu>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KMessageBox>
#include <KSharedConfig>
#include <KToggleAction>
#include <KToolBar>
#include <KWindowConfig>
#include <KXMLGUIFactory>

#include <QApplication>
#include <QChildEvent>
#include <QContextMenuEvent>
#include <QDomDocument>
#include <QDrag>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMimeData>
#include <QRubberBand>
#include <QSizePolicy>
#include <QStackedWidget>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>

namespace KateMDI
{

static KToggleAction *createToolViewToggleAction(const QString &text, ToolView *tv, QObject *parent)
{
    auto a = new KToggleAction(text, parent);
    a->setChecked(tv->toolVisible());
    QObject::connect(a, &KToggleAction::toggled, a, [tv](bool t) {
        if (tv->toolVisible() == t) {
            return;
        }
        if (t) {
            tv->mainWindow()->showToolView(tv);
            tv->setFocus();
        } else {
            tv->mainWindow()->hideToolView(tv);
        }
    });
    QObject::connect(tv, &ToolView::toolVisibleChanged, a, [a](bool v) {
        if (a->isChecked() != v) {
            a->setChecked(v);
        }
    });
    return a;
}

// BEGIN GUICLIENT

static const QString actionListName = QStringLiteral("kate_mdi_view_actions");

GUIClient::GUIClient(MainWindow *mw)
    : QObject(mw)
    , KXMLGUIClient(mw)
    , m_mw(mw)
{
    setComponentName(QStringLiteral("toolviewmanager"), i18n("Toolview Manager"));
    connect(m_mw->guiFactory(), &KXMLGUIFactory::clientAdded, this, &GUIClient::clientAdded);
    const QString guiDescription = QStringLiteral(
        ""
        "<!DOCTYPE gui><gui name=\"kate_mdi_view_actions\">"
        "<MenuBar>"
        "    <Menu name=\"view\">"
        "        <ActionList name=\"%1\" />"
        "    </Menu>"
        "</MenuBar>"
        "</gui>");

    if (domDocument().documentElement().isNull()) {
        QString completeDescription = guiDescription.arg(actionListName);

        setXML(completeDescription, false /*merge*/);
    }

    m_sidebarButtonsMenu = new KActionMenu(i18n("Sidebar Buttons"), this);
    actionCollection()->addAction(QStringLiteral("kate_mdi_show_sidebar_buttons"), m_sidebarButtonsMenu);

    m_focusToolviewMenu = new KActionMenu(i18n("Focus Toolview"), this);
    actionCollection()->addAction(QStringLiteral("kate_mdi_focus_toolview"), m_focusToolviewMenu);

    m_toolMenu = new KActionMenu(i18n("Tool &Views"), this);
    actionCollection()->addAction(QStringLiteral("kate_mdi_toolview_menu"), m_toolMenu);
    m_showSidebarsAction = new KToggleAction(i18n("Show Side&bars"), this);
    actionCollection()->addAction(QStringLiteral("kate_mdi_sidebar_visibility"), m_showSidebarsAction);
    KActionCollection::setDefaultShortcut(m_showSidebarsAction, Qt::CTRL | Qt::ALT | Qt::SHIFT | Qt::Key_F);

    m_showSidebarsAction->setChecked(m_mw->sidebarsVisible());
    connect(m_showSidebarsAction, &KToggleAction::toggled, m_mw, &MainWindow::setSidebarsVisible);

    m_hideToolViews = actionCollection()->addAction(QStringLiteral("kate_mdi_hide_toolviews"), m_mw, &MainWindow::hideToolViews);
    m_hideToolViews->setText(i18n("Hide All Tool Views"));

    m_toolMenu->addAction(m_showSidebarsAction);
    m_toolMenu->addAction(m_hideToolViews);
    auto *sep_act = new QAction(this);
    sep_act->setSeparator(true);
    m_toolMenu->addAction(sep_act);

    // Set config group
    actionCollection()->setConfigGroup(QStringLiteral("Shortcuts"));

    actionCollection()->addAssociatedWidget(m_mw);
    const auto actions = actionCollection()->actions();
    for (QAction *action : actions) {
        action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    }

    // hide tool views menu for KWrite mode
    if (KateApp::isKWrite()) {
        m_sidebarButtonsMenu->setVisible(false);
        m_focusToolviewMenu->setVisible(false);
        m_toolMenu->setVisible(false);
    }
}

void GUIClient::updateSidebarsVisibleAction()
{
    m_showSidebarsAction->setChecked(m_mw->sidebarsVisible());
}

void GUIClient::registerToolView(ToolView *tv)
{
    QString aname = QLatin1String("kate_mdi_toolview_") + tv->id;

    // try to read the action shortcut

    auto shortcutsForActionName = [](const QString &actName) {
        QList<QKeySequence> shortcuts;
        KSharedConfigPtr cfg = KSharedConfig::openConfig();
        const QString shortcutString = cfg->group(QStringLiteral("Shortcuts")).readEntry(actName, QString());
        const auto shortcutStrings = shortcutString.split(QStringLiteral("; "));
        for (const QString &shortcut : shortcutStrings) {
            shortcuts << QKeySequence::fromString(shortcut);
        }
        return shortcuts;
    };

    /** Show ToolView Action **/
    KToggleAction *a = createToolViewToggleAction(i18n("Show %1", tv->text), tv, this);
    //
    QString s = QKeySequence::listToString(shortcutsForActionName(aname));
    if (!(s.isEmpty())) {
        a->setShortcuts(shortcutsForActionName(aname));
    }
    actionCollection()->addAction(aname, a);

    m_toolMenu->addAction(a);

    Q_ASSERT(std::find_if(m_toolToActions.begin(),
                          m_toolToActions.end(),
                          [tv](const ToolViewActions &tvActs) {
                              return tvActs.toolview == tv;
                          })
             == m_toolToActions.end());

    m_toolToActions.push_back({tv, {a}});
    auto &actionsForTool = m_toolToActions.back().actions;

    /** Show Tab button in sidebar action **/
    aname = QStringLiteral("kate_mdi_show_toolview_button_") + tv->id;
    a = new KToggleAction(i18n("Show %1 Button", tv->text), this);
    a->setChecked(true);
    s = QKeySequence::listToString(shortcutsForActionName(aname));
    if (!(s.isEmpty())) {
        a->setShortcuts(shortcutsForActionName(aname));
    }
    actionCollection()->addAction(aname, a);
    connect(a, &KToggleAction::toggled, this, [toolview = QPointer<ToolView>(tv)](bool checked) {
        if (toolview) {
            const QSignalBlocker b(toolview);
            toolview->sidebar()->showToolviewTab(toolview, checked);
        }
    });
    connect(tv, &ToolView::tabButtonVisibleChanged, a, &QAction::setChecked);

    m_sidebarButtonsMenu->addAction(a);
    actionsForTool.push_back(a);

    aname = QStringLiteral("kate_mdi_focus_toolview_") + tv->id;
    auto *act = new QAction(i18n("Focus %1", tv->text), this);
    s = QKeySequence::listToString(shortcutsForActionName(aname));
    if (!(s.isEmpty())) {
        act->setShortcuts(shortcutsForActionName(aname));
    }
    actionCollection()->addAction(aname, act);
    connect(act, &QAction::triggered, tv, [tv = QPointer(tv)] {
        if (tv && tv->mainWindow()) {
            if (!tv->isVisible()) {
                tv->mainWindow()->showToolView(tv);
            }
            tv->setFocus();
        }
    });
    m_focusToolviewMenu->addAction(act);
    actionsForTool.push_back(act);

    updateActions();
}

void GUIClient::unregisterToolView(ToolView *tv)
{
    std::erase_if(m_toolToActions, [tv](const ToolViewActions &a) {
        if (a.toolview == tv) {
            qDeleteAll(a.actions);
            return true;
        }
        return false;
    });

    updateActions();
}

void GUIClient::clientAdded(KXMLGUIClient *client)
{
    if (client == this) {
        updateActions();
        // This should be called after createShellGUI in KateMainWindow ctor
        // otherwise when the client is added it restores the default shortcuts!
        actionCollection()->readSettings();
    }
}

void GUIClient::updateActions()
{
    if (!factory()) {
        return;
    }

    unplugActionList(actionListName);

    QList<QAction *> addList;
    addList.append(m_toolMenu);
    addList.append(m_sidebarButtonsMenu);
    addList.append(m_focusToolviewMenu);

    plugActionList(actionListName, addList);
}

// END GUICLIENT

// BEGIN TOOLVIEW

ToolView::ToolView(MainWindow *mainwin, Sidebar *sidebar, QWidget *parent, const QString &identifier)
    : QFrame(parent)
    , m_mainWin(mainwin)
    , m_sidebar(sidebar)
    , m_toolbar(nullptr)
    , id(identifier)
    , m_toolVisible(false)
{
    // try to fix resize policy
    QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    policy.setRetainSizeWhenHidden(true);
    setSizePolicy(policy);

    // per default vbox layout
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    setLayout(layout);

    // toolbar to collect actions
    m_toolbar = new KToolBar(this);
    m_toolbar->setVisible(false);
    m_toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);

    // ensure reasonable icons sizes, like e.g. the quick-open and co. icons
    // the normal toolbar sizes are TOO large, e.g. for scaled stuff even more!
    const int iconSize = style()->pixelMetric(QStyle::PM_ButtonIconSize, nullptr, this);
    m_toolbar->setIconSize(QSize(iconSize, iconSize));
}

QSize ToolView::sizeHint() const
{
    return size();
}

QSize ToolView::minimumSizeHint() const
{
    return QSize(160, 160);
}

bool ToolView::tabButtonVisible() const
{
    return isTabButtonVisible;
}

void ToolView::setTabButtonVisible(bool visible)
{
    isTabButtonVisible = visible;
}

ToolView::~ToolView()
{
    m_mainWin->toolViewDeleted(this);
}

void ToolView::setToolVisible(bool vis)
{
    if (m_toolVisible == vis) {
        return;
    }

    m_toolVisible = vis;
    Q_EMIT toolVisibleChanged(m_toolVisible);
}

bool ToolView::toolVisible() const
{
    return m_toolVisible;
}

void ToolView::childEvent(QChildEvent *ev)
{
    // set the widget to be focus proxy if possible
    if (ev->type() == QEvent::ChildAdded) {
        if (QWidget *widget = qobject_cast<QWidget *>(ev->child())) {
            setFocusProxy(widget);
            layout()->addWidget(widget);
        }
    }

    QFrame::childEvent(ev);
}

void ToolView::actionEvent(QActionEvent *event)
{
    QFrame::actionEvent(event);
    if (event->type() == QEvent::ActionAdded) {
        m_toolbar->addAction(event->action());
    } else if (event->type() == QEvent::ActionRemoved) {
        m_toolbar->removeAction(event->action());
    }
    m_toolbar->setVisible(!m_toolbar->actions().isEmpty());
}

// END TOOLVIEW

// BEGIN SIDEBAR

MultiTabBar::MultiTabBar(KMultiTabBar::KMultiTabBarPosition pos, Sidebar *sb, int idx)
    : m_sb(sb)
    , m_stack(new QStackedWidget())
    , m_multiTabBar(new KMultiTabBar(pos, this))
{
    setProperty("is-multi-tabbar", true);
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_multiTabBar);

    m_sb->m_ownSplit->insertWidget(idx, m_stack);
    m_stack->hide();
}

MultiTabBar::~MultiTabBar()
{
    // Don't forget to remove our stack from the splitter
    m_stack->deleteLater();
}

KMultiTabBarTab *MultiTabBar::addTab(int id, ToolView *tv)
{
    m_stack->addWidget(tv);
    KMultiTabBarTab *newTab;

    if (std::find(m_tabList.begin(), m_tabList.end(), id) != m_tabList.end()) {
        // We are in session restore
        newTab = m_multiTabBar->tab(id);
        newTab->setIcon(tv->icon);
        newTab->setText(tv->text);
    } else {
        m_tabList.push_back(id);
        m_multiTabBar->appendTab(tv->icon, id, tv->text);
        newTab = m_multiTabBar->tab(id);
    }

    connect(newTab, &KMultiTabBarTab::clicked, this, &MultiTabBar::tabClicked);

    return newTab;
}

int MultiTabBar::addBlankTab()
{
    int id = m_sb->nextId();
    m_tabList.push_back(id);
    m_multiTabBar->appendTab(QIcon(), id, QStringLiteral("placeholder"));
    return id;
}

void MultiTabBar::tabClicked(int id)
{
    if (m_multiTabBar->isTabRaised(id) || m_sb->isCollapsed()) {
        showToolView(id);
    } else {
        hideToolView(id);
    }

    m_sb->updateSidebar();
}

void MultiTabBar::removeBlankTab(int id)
{
    m_tabList.erase(std::remove(m_tabList.begin(), m_tabList.end(), id), m_tabList.end());
    m_multiTabBar->removeTab(id);
    if (tabCount() == 0) {
        m_activeTab = 0;
        Q_EMIT lastTabRemoved(this);
    }
}

void MultiTabBar::removeTab(int id, ToolView *tv)
{
    m_tabList.erase(std::remove(m_tabList.begin(), m_tabList.end(), id), m_tabList.end());
    m_multiTabBar->removeTab(id);

    bool hideView = (m_stack->currentWidget() == tv);
    m_stack->removeWidget(tv);

    if (tabCount() == 0) {
        m_activeTab = 0; // Without any tab left, there is no one active
        Q_EMIT lastTabRemoved(this);
        return;
    }

    if (!hideView) {
        // Some other than the active is removed, no more to do
        return;
    }

    m_activeTab = 0; // Ensure we are up to date, reporting nonsense is dangerous
    tv = static_cast<ToolView *>(m_stack->currentWidget());
    if (tv) {
        auto it = std::find_if(m_sb->m_toolviews.begin(), m_sb->m_toolviews.end(), [tv](const Sidebar::ToolViewData &d) {
            return d.toolview == tv;
        });
        if (it != m_sb->m_toolviews.end()) {
            hideToolView(it->id);
        }
    }
}

void MultiTabBar::reorderTab(int id, KMultiTabBarTab *before)
{
    // can't find source id?
    auto it = std::find(m_tabList.begin(), m_tabList.end(), id);
    if (it == m_tabList.end()) {
        return;
    }

    const qsizetype idIdx = std::distance(m_tabList.begin(), it);
    it = before ? std::find(m_tabList.begin(), m_tabList.end(), before->id()) : m_tabList.end();
    if (before && it == m_tabList.end()) {
        // can't find before id?
        return;
    }
    // before == null, idx will be the last idx
    const qsizetype beforeIdx = it == m_tabList.end() ? m_tabList.size() - 1 : std::distance(m_tabList.begin(), it);
    if (idIdx == beforeIdx) {
        return;
    }

    const qsizetype start = std::min(idIdx, beforeIdx);

    // copy because after removeTab `before` will be invalid
    const int beforeId = before ? before->id() : -1;

    // Remove the actual tabs
    for (size_t i = start; i < m_tabList.size(); ++i) {
        KMultiTabBarTab *oldTab = m_multiTabBar->tab(m_tabList[i]);
        oldTab->removeEventFilter(m_sb);
        m_multiTabBar->removeTab(m_tabList[i]);
    }

    // reorder the tab list
    // erase old position
    m_tabList.erase(std::remove(m_tabList.begin(), m_tabList.end(), id), m_tabList.end());
    // find new position and insert
    it = before ? std::find(m_tabList.begin(), m_tabList.end(), beforeId) : m_tabList.end();
    m_tabList.insert(it, id);

    // re-add tabs
    for (size_t i = start; i < m_tabList.size(); ++i) {
        int tabId = m_tabList[i];
        ToolView *tv = m_sb->dataForId(tabId).toolview;
        m_multiTabBar->appendTab(tv->icon, tabId, tv->text);
        m_sb->appendStyledTab(tabId, this, tv);
    }
}

void MultiTabBar::showToolView(int id)
{
    setTabActive(id, true);
    expandToolView();
}

void MultiTabBar::hideToolView(int id)
{
    setTabActive(id, false);
    collapseToolView();
}

void MultiTabBar::setTabActive(int id, bool state)
{
    // Only change m_activeTab when state is true

    if (m_activeTab == id) {
        // Well, normally should be state always==false, but who knows...
        m_multiTabBar->setTab(id, state);
        m_sb->dataForId(id).toolview->setToolVisible(state);
        m_activeTab = state ? id : 0;
        return;
    }

    if (m_activeTab && state) {
        // Obviously the active tool is changed, disable the old one
        m_multiTabBar->setTab(m_activeTab, false);
        m_sb->dataForId(m_activeTab).toolview->setToolVisible(false);
    }

    m_multiTabBar->setTab(id, state);
    m_sb->dataForId(id).toolview->setToolVisible(state);
    m_activeTab = state ? id : m_activeTab;
}

bool MultiTabBar::isToolActive() const
{
    return m_activeTab > 0;
}

void MultiTabBar::collapseToolView() const
{
    m_stack->hide();

    if (m_stack->count() < 1) {
        return;
    }

    static_cast<ToolView *>(m_stack->currentWidget())->setToolVisible(false);
}

bool MultiTabBar::expandToolView() const
{
    if (!m_activeTab) {
        return false;
    }

    if (m_stack->count() < 1) {
        return false;
    }

    ToolView *tv = m_sb->dataForId(m_activeTab).toolview;
    tv->setToolVisible(true);
    tv->setFocus(); // This is for some tools nice, for some other not
    m_stack->setCurrentWidget(tv);
    m_stack->show();

    return true;
}

Sidebar::Sidebar(KMultiTabBar::KMultiTabBarPosition pos, QSplitter *sp, MainWindow *mainwin, QWidget *parent)
    : QSplitter(parent)
    , m_mainWin(mainwin)
    , m_tabBarPosition(pos)
    , m_splitter(sp)
    , m_ownSplit(new QSplitter(sp))
    , m_ownSplitIndex(sp->indexOf(m_ownSplit))
    , m_lastSize(200) // Default used when no session to restore is around
    , m_dropIndicator(new QRubberBand(QRubberBand::Rectangle, mainwin))
    , m_internalDropIndicator(new QRubberBand(QRubberBand::Rectangle, mainwin))
{
    setChildrenCollapsible(false);
    setAcceptDrops(true);
    connect(this, &Sidebar::destroyed, m_dropIndicator, &QObject::deleteLater);

    if (isVertical()) {
        m_ownSplit->setOrientation(Qt::Vertical);
        setOrientation(Qt::Vertical);
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    } else {
        m_ownSplit->setOrientation(Qt::Horizontal);
        setOrientation(Qt::Horizontal);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }

    m_ownSplit->setChildrenCollapsible(false);

    // ensure proper sidebar state, see resizing issues in bug 460160
    m_ownSplit->hide();

    connect(this, &QSplitter::splitterMoved, this, &Sidebar::barSplitMoved);
    connect(m_ownSplit, &QSplitter::splitterMoved, this, &Sidebar::ownSplitMoved);
    connect(m_splitter, &QSplitter::splitterMoved, this, &Sidebar::handleCollapse);

    insertTabBar();

    // handle config changes & apply initial config
    connect(KateApp::self(), &KateApp::configurationChanged, this, &Sidebar::readConfig);
    readConfig();
}

void Sidebar::readConfig()
{
    bool needsUpdate = false;

    KSharedConfig::Ptr config = KSharedConfig::openConfig();
    KConfigGroup cgGeneral = KConfigGroup(config, QStringLiteral("General"));
    const bool syncWithTabs = cgGeneral.readEntry("Sync section size with tab positions", false);
    if (syncWithTabs != m_syncWithTabs) {
        m_syncWithTabs = syncWithTabs;
        needsUpdate = true;
    }
    // Ignore the option for the bottom bar! Due to it's special design would that never looks good
    if (position() == KMultiTabBar::Bottom) {
        m_syncWithTabs = false;
        needsUpdate = false;
    }
    if (m_syncWithTabs && needsUpdate) {
        // Give the user an immediate feedback that the option has an effect, works not perfect
        // when some section is not active, but may better that to do nothing
        QList<int> wsizes = m_ownSplit->sizes();
        for (int i = 0; i < wsizes.count(); ++i) {
            if (wsizes.at(i) == 0) {
                wsizes[i] = tabBar(i)->sectionSize();
            }
        }
        setSizes(wsizes);
        adjustSplitterSections();
        needsUpdate = false;
    }

    // shall we show text for the left and right bars?
    const bool showTextForLeftRight = cgGeneral.readEntry("Show text for left and right sidebar", false);
    if (showTextForLeftRight != m_showTextForLeftRight) {
        m_showTextForLeftRight = showTextForLeftRight;
        needsUpdate = true;
    }

    int size = cgGeneral.readEntry("Icon size for left and right sidebar buttons", 32);
    if (size != m_leftRightSidebarIconSize) {
        m_leftRightSidebarIconSize = size;
        needsUpdate = true;
    }

    if (!needsUpdate) {
        return;
    }

    for (const auto &[id, wid, _] : m_toolviews) {
        updateButtonStyle(kmTabBar(wid)->tab(id));
    }
}

void Sidebar::appendStyledTab(int id, MultiTabBar *bar, ToolView *widget)
{
    auto newTab = bar->addTab(id, widget);
    auto it = std::find_if(m_toolviews.begin(), m_toolviews.end(), [id](const Sidebar::ToolViewData &d) {
        return d.id == id;
    });
    Q_ASSERT(it != m_toolviews.end());
    it->tabbar = bar;

    Q_ASSERT(newTab);
    newTab->installEventFilter(this);

    // remember original text, we will need it again if we update the style in updateButtonStyle
    newTab->setProperty("kate_original_text", widget->text);

    // fixup styling
    updateButtonStyle(newTab);

    // tell that we have a new tab, useful for e.g. overlays
    // do it delayed so that if toolview was constructed before the container widget
    // and the container wants to react to this signal, it can do so. Otherwise, this
    // will fire before the container has connected to this signal
    QMetaObject::invokeMethod(
        m_mainWin,
        [w = m_mainWin, widget, newTab] {
            Q_EMIT w->tabForToolViewAdded(widget, newTab);
        },
        Qt::QueuedConnection);
}

void Sidebar::updateButtonStyle(KMultiTabBarTab *button)
{
    const auto originalText = button->property("kate_original_text").toString();
    if (!m_showTextForLeftRight && (position() == KMultiTabBar::Left || position() == KMultiTabBar::Right)) {
        const int iconSize = m_leftRightSidebarIconSize;
        button->setIconSize(QSize(iconSize, iconSize));
        button->setText(QString());
        button->setToolTip(originalText);
    } else {
        const int iconSize = style()->pixelMetric(QStyle::PM_ButtonIconSize, nullptr, this);
        button->setIconSize(QSize(iconSize, iconSize));
        button->setText(originalText);
        button->setToolTip(QString());
    }
}

void Sidebar::setStyle(KMultiTabBar::KMultiTabBarStyle style)
{
    m_tabBarStyle = style;

    for (int i = 0; i < tabBarCount(); ++i) {
        tabBar(i)->tabBar()->setStyle(style);
    }
}

QSize Sidebar::sizeHint() const
{
    return minimumSizeHint();
}

QSize Sidebar::minimumSizeHint() const
{
    return isVisible() ? QSplitter::minimumSizeHint() : QSize{0, 0};
}

MultiTabBar *Sidebar::insertTabBar(int idx /* = -1*/)
{
    auto *newBar = new MultiTabBar(m_tabBarPosition, this, idx);
    newBar->installEventFilter(this);
    newBar->tabBar()->setStyle(tabStyle());
    // Fetch user set tabBar splitting before the new bar is inserted
    QList<int> sections = sizes();
    insertWidget(idx, newBar);

    // For a halfway nice new section distribution we need to help the splitter.
    // We only support with this math to insert below some section,
    // not above like at first place, but that's ok atm
    idx = idx < 0 ? sections.count() : idx;
    if (idx) {
        if (m_syncWithTabs) {
            // Share the space where the tab came from with the new bar
            sections[idx - 1] = sections.at(idx - 1) / 2;
            sections.insert(idx, sections.at(idx - 1));
            setSizes(sections);
        } else {
            // We try here to keep the user manipulated tabBar splitting, but that works not perfect.
            // For proper calculations we need to ask for sizeHint, otherwise would tabs with text crunched,
            // but because the to be moved tab is still at the old place, we need to delay that.
            QTimer::singleShot(100, this, [this, idx, sections]() {
                if (tabBarCount() - 1 < idx) {
                    // Config mismatch, the add bar was removed in the meanwhile
                    return;
                }
                QList<int> sectionsC(sections); // To manipulate, we need a C-opy
                if (sectionsC.count() == 1) {
                    int oldTabSize = isVertical() ? tabBar(idx - 1)->sizeHint().height() : tabBar(idx - 1)->sizeHint().width();
                    sectionsC[0] -= oldTabSize;
                    sectionsC.insert(0, oldTabSize);
                } else {
                    int newTabSize = isVertical() ? tabBar(idx)->sizeHint().height() : tabBar(idx)->sizeHint().width();
                    for (int i = 0; i < sections.size(); ++i) {
                        sectionsC[i] -= newTabSize;
                    }
                    sectionsC.insert(idx, newTabSize);
                }
                setSizes(sectionsC);
            });
        }
    }

    connect(newBar, &MultiTabBar::lastTabRemoved, this, &Sidebar::tabBarIsEmpty);

    return newBar;
}

void Sidebar::updateLastSizeOnResize()
{
    const int splitHandleIndex = qMin(m_splitter->indexOf(m_ownSplit) + 1, m_splitter->count() - 1);
    // this method requires that a splitter handle for resizing the sidebar exists
    Q_ASSERT(splitHandleIndex > 0);
    m_splitter->handle(splitHandleIndex)->installEventFilter(this);
}

int Sidebar::nextId()
{
    static int id = 0;
    return ++id;
}

ToolView *Sidebar::addToolView(const QIcon &icon, const QString &text, const QString &identifier, ToolView *widget)
{
    if (widget) {
        if (widget->sidebar() == this) {
            return widget;
        }

        widget->sidebar()->removeToolView(widget);

    } else {
        widget = new ToolView(m_mainWin, this, nullptr, identifier);
        widget->icon = icon;
        widget->text = text;
    }

    widget->m_sidebar = this;

    auto blankTabId = std::find_if(m_tvIdToTabId.begin(), m_tvIdToTabId.end(), [&identifier](const ToolViewToTabId &a) {
        return a.toolview == identifier;
    });
    if (blankTabId != m_tvIdToTabId.end()) {
        int newId = blankTabId->tabId;
        m_toolviews.push_back({.id = newId, .toolview = widget, .tabbar = nullptr});
        appendStyledTab(newId, tabBar(blankTabId->tabbarId), widget);
        // Indicate the blank tab is re-used
        m_tvIdToTabId.erase(blankTabId);
    } else {
        int newId = nextId();
        m_toolviews.push_back({.id = newId, .toolview = widget, .tabbar = nullptr});
        appendStyledTab(newId, tabBar(0), widget);
    }

    show();

    return widget;
}

bool Sidebar::removeToolView(ToolView *widget)
{
    auto it = std::find_if(m_toolviews.begin(), m_toolviews.end(), [widget](const Sidebar::ToolViewData &d) {
        return d.toolview == widget;
    });
    if (it == m_toolviews.end()) {
        return false;
    }

    int id = it->id;
    MultiTabBar *tabbar = it->tabbar;
    m_toolviews.erase(it);

    tabbar->removeTab(id, widget);
    updateSidebar();
    return true;
}

bool Sidebar::showToolView(ToolView *widget)
{
    auto it = std::find_if(m_toolviews.begin(), m_toolviews.end(), [widget](const Sidebar::ToolViewData &d) {
        return d.toolview == widget;
    });
    if (it == m_toolviews.end()) {
        return false;
    }

    tabBar(widget)->showToolView(it->id);
    updateSidebar();

    return true;
}

bool Sidebar::hideToolView(ToolView *widget)
{
    auto it = std::find_if(m_toolviews.begin(), m_toolviews.end(), [widget](const Sidebar::ToolViewData &d) {
        return d.toolview == widget;
    });
    if (it == m_toolviews.end()) {
        return false;
    }

    updateLastSize();
    tabBar(widget)->hideToolView(it->id);
    updateSidebar();

    return true;
}

void Sidebar::showToolviewTab(ToolView *widget, bool show)
{
    auto it = std::find_if(m_toolviews.begin(), m_toolviews.end(), [widget](const Sidebar::ToolViewData &d) {
        return d.toolview == widget;
    });
    if (it == m_toolviews.end()) {
        return;
    }
    KMultiTabBarTab *tab = kmTabBar(widget)->tab(it->id);
    if (widget->tabButtonVisible() == show) {
        return;
    } else {
        widget->setTabButtonVisible(show);
        tab->setVisible(show);
        Q_EMIT widget->tabButtonVisibleChanged(show);
    }
}

bool Sidebar::isCollapsed()
{
    return m_splitter->sizes().at(m_ownSplitIndex) == 0;
}

ToolView *Sidebar::firstVisibleToolView()
{
    for (int i = 0; i < tabBarCount(); ++i) {
        MultiTabBar *tabbar = tabBar(i);
        if (tabbar->isToolActive()) {
            Q_ASSERT(tabbar->tabCount() > 0);
            int id = tabbar->activeTab();
            auto it = std::find_if(m_toolviews.begin(), m_toolviews.end(), [id](const Sidebar::ToolViewData &d) {
                return d.id == id;
            });
            return it->toolview;
        }
    }
    return nullptr;
}

void Sidebar::handleCollapse(int pos, int index)
{
    Q_UNUSED(pos);

    // Verify that we are handling the correct/matching sidebar
    // 0 | 1 | 2  <- m_ownSplitIndex (can be 0 or 2)
    //   1   2    <- index (of splitters, represented by |)
    const bool myInterest = ((m_ownSplitIndex == 0) && (1 == index)) || (m_ownSplitIndex == index);
    if (!myInterest) {
        return;
    }

    if (isCollapsed() && !m_isPreviouslyCollapsed) {
        if (!m_resizePlaceholder) {
            m_resizePlaceholder = new QLabel();
            m_ownSplit->addWidget(m_resizePlaceholder);
            m_resizePlaceholder->show();
            m_resizePlaceholder->setMinimumSize(QSize(160, 160)); // Same minimum size set in ToolView::minimumSizeHint
        }
        collapseSidebar();
    } else if (!isCollapsed() && m_isPreviouslyCollapsed) {
        updateSidebar();
    }
}

void Sidebar::ownSplitMoved(int pos, int index)
{
    QList<int> wsizes = m_ownSplit->sizes();
    for (int i = 0; i < tabBarCount(); ++i) {
        if (tabBar(i)->isToolActive()) {
            tabBar(i)->setSectionSize(wsizes.at(i));
        }
    }

    if (m_syncWithTabs) {
        moveSplitter(pos, index);
    }
}

void Sidebar::barSplitMoved(int pos, int index)
{
    Q_UNUSED(pos);
    Q_UNUSED(index);

    if (m_syncWithTabs) {
        adjustSplitterSections();
    }
}

bool Sidebar::tabBarIsEmpty(MultiTabBar *bar)
{
    // Don't remove the last bar!
    if (!bar || bar->tabCount() > 0 || tabBarCount() == 1) {
        return false;
    }

    delete bar;

    QTimer::singleShot(0, this, [this]() {
        // We need to delay the update or m_ownSplit report wrong size count
        updateSidebar();
    });

    return true;
}

void Sidebar::collapseSidebar()
{
    if (m_isPreviouslyCollapsed) {
        return;
    }

    updateLastSize();

    for (int i = 0; i < tabBarCount(); ++i) {
        tabBar(i)->collapseToolView();
    }

    m_isPreviouslyCollapsed = true;

    if (!m_resizePlaceholder) {
        // Hiding m_ownSplit will cause that no resize handle is offered and
        // as side effect the sidebar collapse
        m_ownSplit->hide();
    } else {
        // We need to force collapse the sidebar by set a zero size
        QList<int> wsizes = m_splitter->sizes();
        wsizes[m_ownSplitIndex] = 0;
        m_splitter->setSizes(wsizes);
    }

    // Now that tools are hidden, ensure the doc got the focus
    m_mainWin->triggerFocusForCentralWidget();
}

bool Sidebar::adjustSplitterSections()
{
    // Here we catch two birds with one stone
    // - Report some caller if any tool is in use or not
    // - Adjust the m_ownSplit sizes to fit more sensible the active tools
    //   To do so we run the loop in reverse order for a better result
    bool anyVis = false;
    QList<int> wsizes = sizes();
    int sizeCollector = 0;
    int lastExpandedId = -1;
    for (int i = tabBarCount() - 1; i > -1; --i) {
        sizeCollector += wsizes.at(i);
        if (tabBar(i)->expandToolView()) {
            anyVis = true;
            wsizes[i] = sizeCollector;
            sizeCollector = 0;
            lastExpandedId = i;
        } else {
            wsizes[i] = 0;
        }
    }

    if (!anyVis) {
        // No need to go on
        return false;
    }

    if (!m_syncWithTabs) {
        return true;
    }

    if (sizeCollector && lastExpandedId > -1) {
        wsizes[lastExpandedId] += sizeCollector;
    }

    m_ownSplit->setSizes(wsizes);

    return true;
}

void Sidebar::updateSidebar()
{
    if (!adjustSplitterSections()) {
        // Nothing is shown, don't expand to a blank area
        collapseSidebar();
        return;
    }

    if (m_resizePlaceholder) {
        delete m_resizePlaceholder;
    }

    m_isPreviouslyCollapsed = false;

    if (isCollapsed()) {
        QList<int> wsizes = m_splitter->sizes();
        wsizes[m_ownSplitIndex] = m_lastSize;
        m_splitter->setSizes(wsizes);
    }

    if (!m_syncWithTabs) {
        QList<int> wsizes = m_ownSplit->sizes();
        for (int i = 0; i < tabBarCount(); ++i) {
            wsizes[i] = tabBar(i)->isToolActive() ? tabBar(i)->sectionSize() : 0;
        }
        m_ownSplit->setSizes(wsizes);
    }

    // Ensure we are visible
    m_ownSplit->show();
}

bool Sidebar::eventFilter(QObject *obj, QEvent *ev)
{
    if (ev->type() == QEvent::ContextMenu) {
        auto *e = static_cast<QContextMenuEvent *>(ev);
        auto *bt = qobject_cast<KMultiTabBarTab *>(obj);
        if (bt) {
            // qCDebug(LOG_KATE, "Request for popup");

            m_popupButton = bt->id();

            auto it = std::find_if(m_toolviews.begin(), m_toolviews.end(), [id = m_popupButton](const Sidebar::ToolViewData &d) {
                return d.id == id;
            });
            Q_ASSERT(it != m_toolviews.end());
            ToolView *w = it->toolview;

            if (w) {
                QMenu menu(this);

                menu.addSection(w->icon, w->text);

                if (!w->plugin.isNull()) {
                    if (w->plugin.data()->configPages() > 0) {
                        menu.addAction(QIcon::fromTheme(QStringLiteral("configure")), i18n("Configure..."))->setData(ConfigureAction);
                    }
                }

                menu.addAction(i18n("Hide Button"))->setData(HideButtonAction);

                menu.addSection(QIcon::fromTheme(QStringLiteral("move")), i18n("Move To"));

                int tabBarId = indexOf(it->tabbar);

                if (tabBar(tabBarId)->tabCount() > 1) {
                    menu.addAction(QIcon::fromTheme(QStringLiteral("list-add")), i18n("Own Section"))->setData(ToOwnSectAction);
                }

                if (tabBarCount() > 1) {
                    if (tabBarId < 1) {
                        if (isVertical()) {
                            menu.addAction(QIcon::fromTheme(QStringLiteral("go-down")), i18n("One Down"))->setData(DownRightAction);
                        } else {
                            menu.addAction(QIcon::fromTheme(QStringLiteral("go-next")), i18n("One Right"))->setData(DownRightAction);
                        }
                    } else {
                        if (isVertical()) {
                            menu.addAction(QIcon::fromTheme(QStringLiteral("go-up")), i18n("One Up"))->setData(UpLeftAction);
                            if (tabBarId < tabBarCount() - 1) {
                                menu.addAction(QIcon::fromTheme(QStringLiteral("go-down")), i18n("One Down"))->setData(DownRightAction);
                            }
                        } else {
                            menu.addAction(QIcon::fromTheme(QStringLiteral("go-previous")), i18n("One Left"))->setData(UpLeftAction);
                            if (tabBarId < tabBarCount() - 1) {
                                menu.addAction(QIcon::fromTheme(QStringLiteral("go-next")), i18n("One Right"))->setData(DownRightAction);
                            }
                        }
                    }
                }

                if (position() != 0) {
                    menu.addAction(QIcon::fromTheme(QStringLiteral("go-previous")), i18n("Left Sidebar"))->setData(0);
                }

                if (position() != 1) {
                    menu.addAction(QIcon::fromTheme(QStringLiteral("go-next")), i18n("Right Sidebar"))->setData(1);
                }

                if (position() != 2) {
                    menu.addAction(QIcon::fromTheme(QStringLiteral("go-up")), i18n("Top Sidebar"))->setData(2);
                }

                if (position() != 3) {
                    menu.addAction(QIcon::fromTheme(QStringLiteral("go-down")), i18n("Bottom Sidebar"))->setData(3);
                }

                connect(&menu, &QMenu::triggered, this, &Sidebar::buttonPopupActivate);

                menu.exec(e->globalPos());

                return true;
            }
        }
    } else if (ev->type() == QEvent::MouseButtonRelease) {
        // The sidebar's splitter handle handle was released, so we update the sidebar's size. See Sidebar::updateLastSizeOnResize
        auto *e = static_cast<QMouseEvent *>(ev);
        if (e->button() == Qt::LeftButton) {
            updateLastSize();
        }
    } else if (ev->type() == QEvent::MouseButtonPress) {
        auto *e = static_cast<QMouseEvent *>(ev);
        if (qobject_cast<MultiTabBar *>(obj)) {
            // The non-tab area of the sidebar is clicked (well, pressed) => toggle collapse/expand
            if (e->button() == Qt::LeftButton) {
                if (obj->property("is-multi-tabbar").toBool()) {
                    if (isCollapsed()) {
                        updateSidebar();
                    } else {
                        collapseSidebar();
                    }
                    return true;
                }
            }
        } else if (qobject_cast<KMultiTabBarTab *>(obj) && e->button() == Qt::LeftButton) {
            dragStartPos = e->pos();
        }
    } else if (!dragStartPos.isNull() && ev->type() == QEvent::MouseMove) {
        auto *e = static_cast<QMouseEvent *>(ev);
        auto *tab = qobject_cast<KMultiTabBarTab *>(obj);
        if (tab && (e->pos() - dragStartPos).manhattanLength() >= QApplication::startDragDistance()) {
            // start drag
            QPixmap pixmap = tab->grab();
            auto *drag = new QDrag(this);
            auto md = new QMimeData();
            ToolView *toolView = dataForId(tab->id()).toolview;
            Q_ASSERT(toolView);
            md->setProperty("toolviewToMove", QVariant::fromValue(toolView));
            drag->setMimeData(md);
            drag->setPixmap(pixmap);
            drag->setHotSpot(dragStartPos);
            dragStartPos = {};
            connect(drag, &QObject::destroyed, this, &Sidebar::dragEnded);

            Q_EMIT dragStarted();

            drag->exec(Qt::MoveAction);
            return true;
        }
    }

    return QSplitter::eventFilter(obj, ev);
}

void Sidebar::dragEnterEvent(QDragEnterEvent *e)
{
    if (e->proposedAction() != Qt::MoveAction) {
        return;
    }

    if (!e->mimeData() || !e->mimeData()->property("toolviewToMove").value<ToolView *>()) {
        return;
    }

    if (e->source() == this) {
        if (toolviewCount() == 1) {
            // only 1 toolview? Nothing to reorder then
            return;
        }
        m_internalDropIndicator->raise();
        if (m_internalDropIndicator->geometry() != geometry()) {
            m_internalDropIndicator->setGeometry(geometry());
        }

        if (isVertical()) {
            m_internalDropIndicator->setFixedHeight(4);
        } else {
            m_internalDropIndicator->setFixedWidth(4);
        }

        auto mimeData = e->mimeData();
        auto *toolview = mimeData->property("toolviewToMove").value<ToolView *>();
        QWidget *tab = tabButtonForToolview(toolview);
        if (!tab || !toolview) {
            return;
        }

        const QPoint tabPos = tab->pos();
        auto globalPos = mapToGlobal(tabPos);
        auto pos = m_mainWin->mapFromGlobal(globalPos);
        m_internalDropIndicator->move(pos);
        m_internalDropIndicator->show();
    } else { // Show Drop Indicator
        m_dropIndicator->raise();
        if (m_dropIndicator->geometry() != geometry()) {
            m_dropIndicator->setGeometry(geometry());
        }
        auto globalPos = mapToGlobal(pos());
        auto indicatorPos = m_dropIndicator->mapFromGlobal(globalPos);
        if (indicatorPos != m_dropIndicator->pos()) {
            m_dropIndicator->move(indicatorPos);
        }

        m_dropIndicator->show();
    }
    e->acceptProposedAction();
}

void Sidebar::dropEvent(QDropEvent *e)
{
    auto mimeData = e->mimeData();
    m_dropIndicator->hide();
    m_internalDropIndicator->hide();

    if (!mimeData) {
        return;
    }
    auto *toolview = mimeData->property("toolviewToMove").value<ToolView *>();
    if (!toolview) {
        return;
    }

    if (e->source() == this) {
        // Re-ordering tabs
        auto *sourceTab = qobject_cast<KMultiTabBarTab *>(tabButtonForToolview(toolview));
        if (!sourceTab) {
            return;
        }
        // destTab might be null, which means we will just append to the end
        auto destTab = qobject_cast<KMultiTabBarTab *>(childAt(e->position().toPoint()));
        auto it = std::find_if(m_toolviews.begin(), m_toolviews.end(), [toolview](const Sidebar::ToolViewData &d) {
            return d.toolview == toolview;
        });
        Q_ASSERT(it != m_toolviews.end());
        it->tabbar->reorderTab(sourceTab->id(), destTab);
    } else {
        m_mainWin->moveToolView(toolview, position(), /*isDND=*/true);
        m_mainWin->showToolView(toolview);
    }

    e->accept();
}

void Sidebar::dragMoveEvent(QDragMoveEvent *e)
{
    if (e->source() != this || toolviewCount() == 1) {
        // We only handle drag move if we are moving on source sidebar
        // and if there is only 1 toolview, there is nothing to reorder
        return;
    }

    auto *tab = qobject_cast<KMultiTabBarTab *>(childAt(e->position().toPoint()));
    if (tab) {
        // moving over a tab? show the indicator at tab start
        const QPoint tabPos = tab->pos();
        auto globalPos = mapToGlobal(tabPos);
        auto pos = m_mainWin->mapFromGlobal(globalPos);
        m_internalDropIndicator->move(pos);
    } else {
        // Otherwise show at the end of tab list
        auto mimeData = e->mimeData();
        auto *toolview = mimeData->property("toolviewToMove").value<ToolView *>();
        if (!toolview) {
            return;
        }

        auto it = std::find_if(m_toolviews.begin(), m_toolviews.end(), [toolview](const Sidebar::ToolViewData &d) {
            return d.toolview == toolview;
        });
        Q_ASSERT(it != m_toolviews.end());
        MultiTabBar *tabbar = it->tabbar;
        int lastTabId = tabbar->tabList().back();
        KMultiTabBarTab *lastTab = tabbar->tabBar()->tab(lastTabId);

        QPoint tabPos = lastTab->pos();
        if (isVertical()) {
            tabPos.setY(tabPos.y() + lastTab->height());
        } else {
            tabPos.setX(tabPos.x() + lastTab->width());
        }
        auto globalPos = mapToGlobal(tabPos);
        auto pos = m_mainWin->mapFromGlobal(globalPos);
        m_internalDropIndicator->move(pos);
    }
}

void Sidebar::dragLeaveEvent(QDragLeaveEvent *)
{
    m_internalDropIndicator->hide();
    m_dropIndicator->hide();
}

void Sidebar::setVisible(bool visible)
{
    // visible==true means show-request
    if (visible && (m_toolviews.empty() || !m_mainWin->sidebarsVisible())) {
        return;
    }

    QSplitter::setVisible(visible);
}

void Sidebar::buttonPopupActivate(QAction *a)
{
    const int id = a->data().toInt();
    ToolViewData tvData = dataForId(m_popupButton);
    ToolView *w = tvData.toolview;

    if (!w) {
        return;
    }

    // move to other Sidebar ids
    if (id < 4) {
        // move + show ;)
        m_mainWin->moveToolView(w, static_cast<KMultiTabBar::KMultiTabBarPosition>(id));
        m_mainWin->showToolView(w);
    }

    if (id == ConfigureAction) {
        if (!w->plugin.isNull()) {
            if (w->plugin.data()->configPages() > 0) {
                Q_EMIT sigShowPluginConfigPage(w->plugin.data(), 0);
            }
        }
    }

    if (id == HideButtonAction) {
        showToolviewTab(w, false);
    }

    if (id == ToOwnSectAction) {
        MultiTabBar *newBar = insertTabBar(indexOf(tvData.tabbar) + 1);
        tabBar(w)->removeTab(tvData.id, w);
        appendStyledTab(tvData.id, newBar, w);
        showToolView(w);
    }
    if (id == UpLeftAction) {
        MultiTabBar *newBar = tabBar(indexOf(tabBar(w)) - 1);
        tabBar(w)->removeTab(tvData.id, w);
        appendStyledTab(tvData.id, newBar, w);
    }
    if (id == DownRightAction) {
        MultiTabBar *newBar = tabBar(indexOf(tabBar(w)) + 1);
        tabBar(w)->removeTab(tvData.id, w);
        appendStyledTab(tvData.id, newBar, w);
    }
}

void Sidebar::updateLastSize()
{
    if (isCollapsed()) {
        // We are too late, don't update to stupid value
        return;
    }

    QList<int> s = m_splitter->sizes();

    // Ensure last size is always sensible
    m_lastSize = qMax(s[m_ownSplitIndex], 160);
}

void Sidebar::startRestoreSession(KConfigGroup &config)
{
    // ensure we only run once and we don't start a saveSession in addition
    if (m_sessionRestoreRunning) {
        return;
    }
    m_sessionRestoreRunning = true;

    // Using splitter data avoid to store tabBarCount explicit ;-)
    char key[256]{};
    snprintf(key, sizeof(key), "Kate-MDI-Sidebar-%d-Splitter", (int)position());
    QList<int> s = config.readEntry(key, QList<int>());
    // Notice the start value of 1, only add extra tab bars
    for (int i = 1; i < s.size(); ++i) {
        insertTabBar();
    }
    // Create for each tool we expect, in the correct order, a tab in advance, a blank one
    for (int i = 0; i < s.size(); ++i) {
        snprintf(key, sizeof(key), "Kate-MDI-Sidebar-%d-Bar-%d-TvList", (int)position(), i);
        const QStringList tvList = config.readEntry(key, QStringList());
        for (int j = 0; j < tvList.size(); ++j) {
            // Don't add in case of a config mismatch blank tabs for some stuff twice
            auto it = std::find_if(m_tvIdToTabId.begin(), m_tvIdToTabId.end(), [&tvList, j](const ToolViewToTabId &a) {
                return a.toolview == tvList.at(j);
            });
            if (it == m_tvIdToTabId.end()) {
                int id = tabBar(i)->addBlankTab();
                m_tvIdToTabId.emplace_back(ToolViewToTabId{.toolview = tvList[j], .tabId = id, .tabbarId = i});
            }
        }
    }
}

void Sidebar::restoreSession(KConfigGroup &config)
{
    // Only continue when restore was started properly
    if (!m_sessionRestoreRunning) {
        return;
    }

    // show only correct toolviews ;)
    for (const auto &[id, tv, tabbar] : m_toolviews) {
        tabbar->setTabActive(id, config.readEntry(QStringLiteral("Kate-MDI-ToolView-%1-Visible").arg(tv->id), false));
        showToolviewTab(tv, config.readEntry(QStringLiteral("Kate-MDI-ToolView-%1-Show-Button-In-Sidebar").arg(tv->id), true));
    }

    // In case of some config mismatch, we should remove left over blank tabs
    for (const auto &[tv, id, tabbarId] : m_tvIdToTabId) {
        tabBar(tabbarId)->removeBlankTab(id);
    }
    m_tvIdToTabId.clear();

    // In case of some config mismatch, we may have some ugly empty bars
    for (int i = 0; i < tabBarCount(); ++i) {
        // Don't panic! Function take care not to remove non empty bar
        if (tabBarIsEmpty(tabBar(i))) {
            // Now that the bar is gone, we need to adjust our index or we would skip some bar
            --i;
        }
    }

    char key[256]{};
    snprintf(key, sizeof(key), "Kate-MDI-Sidebar-%d-SectSizes", (int)position());
    QList<int> sectSizes = config.readEntry(key, QList<int>());
    for (int i = 0; i < sectSizes.count(); ++i) {
        if (tabBarCount() - 1 < i) {
            // Config mismatch!
            break;
        }
        tabBar(i)->setSectionSize(sectSizes.at(i));
    }

    // Collapse now, and before! we restore m_lastSize, will hide (all) m_stack(s) so that
    // expanding will work fine...
    collapseSidebar();
    snprintf(key, sizeof(key), "Kate-MDI-Sidebar-%d-LastSize", (int)position());
    m_lastSize = config.readEntry(key, 160);
    // Since we delay in insertTabBar(..) to adjust the sizes, we need it here too or the now set data will overwritten
    snprintf(key, sizeof(key), "Kate-MDI-Sidebar-%d-Splitter", (int)position());
    auto sz = config.readEntry(key, QList<int>());
    QTimer::singleShot(100, this, [this, sz]() {
        setSizes(sz);

        // ensure focus is not stolen
        m_mainWin->triggerFocusForCentralWidget();
    });
    // ...now we are ready to get the final splitter sizes by MainWindow::finishRestore
    updateSidebar();

    // be done, e.g. saveSession is now allowed again
    m_sessionRestoreRunning = false;
}

void Sidebar::saveSession(KConfigGroup &config)
{
    // Don't try to save a session while we are still in "session restore mode" BUG:459108
    if (m_sessionRestoreRunning) {
        return;
    }

    char key[256]{};
    snprintf(key, sizeof(key), "Kate-MDI-Sidebar-%d-Splitter", (int)position());
    config.writeEntry(key, sizes());

    snprintf(key, sizeof(key), "Kate-MDI-Sidebar-%d-LastSize", (int)position());
    config.writeEntry(key, m_lastSize);

    // store the data about all toolviews in this sidebar ;)
    for (const auto &[id, tv, _] : m_toolviews) {
        config.writeEntry(QStringLiteral("Kate-MDI-ToolView-%1-Position").arg(tv->id), int(tv->sidebar()->position()));
        config.writeEntry(QStringLiteral("Kate-MDI-ToolView-%1-Visible").arg(tv->id), tv->toolVisible());
        config.writeEntry(QStringLiteral("Kate-MDI-ToolView-%1-Show-Button-In-Sidebar").arg(tv->id), tv->tabButtonVisible());
    }

    QList<int> sectSizes;
    for (int i = 0; i < tabBarCount(); ++i) {
        sectSizes << tabBar(i)->sectionSize();

        QStringList tvList;
        for (int j : tabBar(i)->tabList()) {
            tvList << dataForId(j).toolview->id;
        }
        snprintf(key, sizeof(key), "Kate-MDI-Sidebar-%d-Bar-%d-TvList", (int)position(), i);
        config.writeEntry(key, tvList);
    }

    snprintf(key, sizeof(key), "Kate-MDI-Sidebar-%d-SectSizes", (int)position());
    config.writeEntry(key, sectSizes);
}

// END SIDEBAR

// BEGIN MAIN WINDOW

MainWindow::MainWindow(QWidget *parent)
    : KParts::MainWindow(parent, Qt::Window)
    , m_guiClient(new GUIClient(this))
{
    // central frame for all stuff
    auto *hb = new QFrame(this);
    hb->setObjectName("KateCentralWidget");
    setCentralWidget(hb);

    // top level vbox for all stuff + bottom bar
    auto *toplevelVBox = new QVBoxLayout(hb);
    toplevelVBox->setContentsMargins(0, 0, 0, 0);
    toplevelVBox->setSpacing(0);

    // hbox for all splitters and other side bars
    auto *hlayout = new QHBoxLayout;
    hlayout->setContentsMargins(0, 0, 0, 0);
    hlayout->setSpacing(0);
    toplevelVBox->addLayout(hlayout);

    m_hSplitter = new QSplitter(Qt::Horizontal, hb);
    m_sidebars[KMultiTabBar::Left] = std::make_unique<Sidebar>(KMultiTabBar::Left, m_hSplitter, this, hb);
    hlayout->addWidget(m_sidebars[KMultiTabBar::Left].get());
    hlayout->addWidget(m_hSplitter);

    auto *vb = new QFrame(m_hSplitter);
    auto *vlayout = new QVBoxLayout(vb);
    vlayout->setContentsMargins(0, 0, 0, 0);
    vlayout->setSpacing(0);

    m_hSplitter->setCollapsible(m_hSplitter->indexOf(vb), false);
    m_hSplitter->setStretchFactor(m_hSplitter->indexOf(vb), 1);

    m_vSplitter = new QSplitter(Qt::Vertical, vb);
    m_sidebars[KMultiTabBar::Top] = std::make_unique<Sidebar>(KMultiTabBar::Top, m_vSplitter, this, vb);
    vlayout->addWidget(m_sidebars[KMultiTabBar::Top].get());
    vlayout->addWidget(m_vSplitter);

    m_centralWidget = new QWidget(m_vSplitter);
    m_centralWidget->setLayout(new QVBoxLayout);
    m_centralWidget->layout()->setSpacing(0);
    m_centralWidget->layout()->setContentsMargins(0, 0, 0, 0);

    m_vSplitter->setCollapsible(m_vSplitter->indexOf(m_centralWidget), false);
    m_vSplitter->setStretchFactor(m_vSplitter->indexOf(m_centralWidget), 1);

    m_sidebars[KMultiTabBar::Right] = std::make_unique<Sidebar>(KMultiTabBar::Right, m_hSplitter, this, hb);
    hlayout->addWidget(m_sidebars[KMultiTabBar::Right].get());

    auto separator = new QFrame(this);
    separator->setFrameShape(QFrame::HLine);
    separator->setFixedHeight(1);
    separator->setEnabled(false);
    toplevelVBox->addWidget(separator);

    // bottom side bar spans full windows, include status bar, too
    m_sidebars[KMultiTabBar::Bottom] = std::make_unique<Sidebar>(KMultiTabBar::Bottom, m_vSplitter, this, vb);
    m_bottomSidebarLayout = new QHBoxLayout;
    m_bottomSidebarLayout->addWidget(m_sidebars[KMultiTabBar::Bottom].get());
    m_statusBarStackedWidget = new QStackedWidget(this);
    m_statusBarStackedWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    // button that shows git branch
    // m_branchLabel = new CurrentGitBranchButton(this);
    // m_bottomSidebarLayout->addWidget(m_branchLabel);
    // widget that hold statusbar of active kte-view
    m_bottomSidebarLayout->addWidget(m_statusBarStackedWidget);
    m_bottomSidebarLayout->setStretch(0, 100);
    toplevelVBox->addLayout(m_bottomSidebarLayout);

    // ensure proper toolview style
    setToolViewStyle(KMultiTabBar::KDEV3ICON);

    for (const auto &sidebar : m_sidebars) {
        connect(sidebar.get(), &Sidebar::sigShowPluginConfigPage, this, &MainWindow::sigShowPluginConfigPage);

        // Drag/Drop
        connect(sidebar.get(), &Sidebar::dragStarted, this, [this]() {
            for (const auto &sb : m_sidebars) {
                m_dragState.m_wasVisible[(int)sb->position()] = sb->isVisible();
                if (!sb->isVisible()) {
                    sb->setMinimumSize({50, 50});
                    sb->QSplitter::setVisible(true);
                }
            }
        });
        connect(sidebar.get(), &Sidebar::dragEnded, this, [this]() {
            for (const auto &sb : m_sidebars) {
                bool wasVisible = m_dragState.m_wasVisible[(int)sb->position()];
                if (!wasVisible) {
                    sb->setMinimumSize({0, 0});
                    sb->QSplitter::setVisible(false);
                }
            }
        });
    }
}

MainWindow::~MainWindow()
{
    // kill all toolviews, they will deregister themself
    while (!m_toolviews.empty()) {
        delete m_toolviews.begin()->toolview;
    }

    // seems like we really should delete this by hand ;)
    delete m_centralWidget;
}

QWidget *MainWindow::centralWidget() const
{
    return m_centralWidget;
}

void MainWindow::insertWidgetBeforeStatusbar(QWidget *widget)
{
    Q_ASSERT(m_bottomSidebarLayout);
    const auto idxOfStatusbar = m_bottomSidebarLayout->indexOf(m_statusBarStackedWidget);
    Q_ASSERT(idxOfStatusbar != -1);
    m_bottomSidebarLayout->insertWidget(idxOfStatusbar, widget);
}

void MainWindow::setStatusBarVisible(bool visible)
{
    statusBarStackedWidget()->setVisible(visible);
    // Hide everything after bottom sidebar
    const auto idxOfItemAfterBottomSidebar = m_bottomSidebarLayout->indexOf(m_sidebars[KMultiTabBar::Bottom].get()) + 1;
    for (int i = idxOfItemAfterBottomSidebar; i < m_bottomSidebarLayout->count(); ++i) {
        auto item = m_bottomSidebarLayout->itemAt(i);
        if (item && item->widget()) {
            item->widget()->setVisible(visible);
        }
    }
}

ToolView *MainWindow::createToolView(KTextEditor::Plugin *plugin,
                                     const QString &identifier,
                                     KMultiTabBar::KMultiTabBarPosition pos,
                                     const QIcon &icon,
                                     const QString &text)
{
    // clashing names are not allowed
    if (toolView(identifier)) {
        return nullptr;
    }

    // try the restore config to figure out real pos
    if (m_restoreConfig && m_restoreConfig->hasGroup(m_restoreGroup)) {
        KConfigGroup cg(m_restoreConfig, m_restoreGroup);
        pos = static_cast<KMultiTabBar::KMultiTabBarPosition>(cg.readEntry(QStringLiteral("Kate-MDI-ToolView-%1-Position").arg(identifier), int(pos)));
    }

    ToolView *v = m_sidebars[pos]->addToolView(icon, text, identifier, nullptr);
    v->plugin = plugin;

    Q_ASSERT(toolView(identifier) == nullptr);
    m_toolviews.push_back({identifier, v});

    // register for menu stuff
    m_guiClient->registerToolView(v);

    return v;
}

ToolView *MainWindow::toolView(const QString &identifier) const
{
    for (const auto &[name, toolview] : m_toolviews) {
        if (name == identifier) {
            return toolview;
        }
    }
    return nullptr;
}

void MainWindow::toolViewDeleted(ToolView *widget)
{
    if (!widget) {
        return;
    }

    if (widget->mainWindow() != this) {
        return;
    }

    // unregister from menu stuff
    m_guiClient->unregisterToolView(widget);

    widget->sidebar()->removeToolView(widget);

    std::erase_if(m_toolviews, [widget](const ToolViewWithId &p) {
        return p.id == widget->id;
    });
}

void MainWindow::setSidebarsVisibleInternal(bool visible, bool hideFullySilent)
{
    bool old_visible = m_sidebarsVisible;
    m_sidebarsVisible = visible;

    for (auto &sidebar : m_sidebars) {
        sidebar->setVisible(visible);

        // fully hide the stuff, see bug 464320
        if (hideFullySilent) {
            sidebar->collapseSidebar();
        }
    }

    m_guiClient->updateSidebarsVisibleAction();

    // show information message box, if the users hides the sidebars
    if (!hideFullySilent && old_visible && (!m_sidebarsVisible)) {
        KMessageBox::information(this,
                                 i18n("<qt>You are about to hide the sidebars. With "
                                      "hidden sidebars it is not possible to directly "
                                      "access the tool views with the mouse anymore, "
                                      "so if you need to access the sidebars again "
                                      "invoke <b>View &gt; Tool Views &gt; Show Sidebars</b> "
                                      "in the menu. It is still possible to show/hide "
                                      "the tool views with the assigned shortcuts.</qt>"),
                                 QString(),
                                 QStringLiteral("Kate hide sidebars notification message"));
    }
}

ToolView *MainWindow::activeViewToolView(KMultiTabBar::KMultiTabBarPosition pos)
{
    const auto side = static_cast<size_t>(pos);
    if (side >= 4) {
        return nullptr;
    }
    return m_sidebars[side]->firstVisibleToolView();
}

bool MainWindow::sidebarsVisible() const
{
    return m_sidebarsVisible;
}

void MainWindow::setToolViewStyle(KMultiTabBar::KMultiTabBarStyle style)
{
    for (auto &sidebar : m_sidebars) {
        sidebar->setStyle(style);
    }
}

KMultiTabBar::KMultiTabBarStyle MainWindow::toolViewStyle() const
{
    // all sidebars have the same style, so just take Top
    return m_sidebars[KMultiTabBar::Top]->tabStyle();
}

bool MainWindow::moveToolView(ToolView *widget, KMultiTabBar::KMultiTabBarPosition pos, bool isDND)
{
    if (!widget || widget->mainWindow() != this) {
        return false;
    }

    // try the restore config to figure out real pos
    if (m_restoreConfig && m_restoreConfig->hasGroup(m_restoreGroup)) {
        KConfigGroup cg(m_restoreConfig, m_restoreGroup);
        pos = static_cast<KMultiTabBar::KMultiTabBarPosition>(cg.readEntry(QStringLiteral("Kate-MDI-ToolView-%1-Position").arg(widget->id), int(pos)));
    }

    if (isDND) {
        // Ensure that after dropping, sidebar becomes visiblee
        // Sidebar might have been hidden because it was empty i.e.,
        // had no toolviews
        m_dragState.m_wasVisible[pos] = true;

        // If source sidebar contains only 1 toolview, then hide it
        // because after the drop it will have nothing.
        auto source = widget->sidebar();
        if (source->toolviewCount() == 1) {
            m_dragState.m_wasVisible[source->position()] = false;
        }
    }

    m_sidebars[pos]->addToolView(widget->icon, widget->text, widget->id, widget);

    if (isDND) {
        // reduce min size so that sidebar can adjust size properly
        m_sidebars[pos]->setMinimumSize({0, 0});
    }

    Q_EMIT toolViewMoved(widget, static_cast<KTextEditor::MainWindow::ToolViewPosition>(pos));

    return true;
}

bool MainWindow::showToolView(ToolView *widget)
{
    if (!widget || widget->mainWindow() != this) {
        return false;
    }

    // skip this if happens during restoring, or we will just see flicker
    if (m_restoreConfig && m_restoreConfig->hasGroup(m_restoreGroup)) {
        return true;
    }

    return widget->sidebar()->showToolView(widget);
}

bool MainWindow::hideToolView(ToolView *widget)
{
    if (!widget || widget->mainWindow() != this) {
        return false;
    }

    // skip this if happens during restoring, or we will just see flicker
    if (m_restoreConfig && m_restoreConfig->hasGroup(m_restoreGroup)) {
        return true;
    }

    const bool ret = widget->sidebar()->hideToolView(widget);
    triggerFocusForCentralWidget();
    return ret;
}

void MainWindow::hideToolViews()
{
    for (const auto &tv : m_toolviews) {
        tv.toolview->sidebar()->hideToolView(tv.toolview);
    }
    triggerFocusForCentralWidget();
}

KTextEditor::MainWindow::ToolViewPosition MainWindow::toolViewPosition(QWidget *toolview)
{
    if (auto tv = qobject_cast<ToolView *>(toolview)) {
        return static_cast<KTextEditor::MainWindow::ToolViewPosition>(tv->sidebar()->position());
    }
    qCWarning(LOG_KATE, "Invalid, not a toolview");
    return KTextEditor::MainWindow::ToolViewPosition::Bottom;
}

void MainWindow::startRestore(KConfigBase *config, const QString &group)
{
    // first save this stuff
    m_restoreConfig = config;
    m_restoreGroup = group;

    if (!m_restoreConfig || !m_restoreConfig->hasGroup(m_restoreGroup)) {
        m_restoreConfig = nullptr;
        m_restoreGroup.clear();
        return;
    }

    // apply size once, to get sizes ready ;)
    KConfigGroup cg(m_restoreConfig, m_restoreGroup);
    KWindowConfig::restoreWindowSize(windowHandle(), cg);

    // KWrite uses no sidebars, avoid all work beside windows sizes restoring above
    if (KateApp::isKWrite()) {
        m_restoreConfig = nullptr;
        m_restoreGroup.clear();
        return;
    }

    // restore the sidebars
    for (auto &sidebar : std::as_const(m_sidebars)) {
        sidebar->startRestoreSession(cg);
    }

    setToolViewStyle(static_cast<KMultiTabBar::KMultiTabBarStyle>(cg.readEntry("Kate-MDI-Sidebar-Style", static_cast<int>(toolViewStyle()))));
    // after reading m_sidebarsVisible, update the GUI toggle action
    m_sidebarsVisible = cg.readEntry("Kate-MDI-Sidebar-Visible", true);
    m_guiClient->updateSidebarsVisibleAction();
}

void MainWindow::finishRestore()
{
    if (!m_restoreConfig) {
        return;
    }

    if (m_restoreConfig->hasGroup(m_restoreGroup)) {
        // apply all settings, like toolbar pos and more ;)
        KConfigGroup cg(m_restoreConfig, m_restoreGroup);
        applyMainWindowSettings(cg);

        // reshuffle toolviews only if needed
        for (const auto &[id, tv] : m_toolviews) {
            KMultiTabBar::KMultiTabBarPosition newPos = static_cast<KMultiTabBar::KMultiTabBarPosition>(
                cg.readEntry(QStringLiteral("Kate-MDI-ToolView-%1-Position").arg(id), int(tv->sidebar()->position())));

            if (tv->sidebar()->position() != newPos) {
                moveToolView(tv, newPos);
            }
        }

        // Restore the sidebars before we restore h/vSplitter..
        for (auto &sidebar : m_sidebars) {
            sidebar->restoreSession(cg);
        }

        // get main splitter sizes ;)
        m_hSplitter->setSizes(cg.readEntry("Kate-MDI-H-Splitter", QList<int>{200, 100, 200}));
        m_vSplitter->setSizes(cg.readEntry("Kate-MDI-V-Splitter", QList<int>{150, 100, 200}));

        // Expand again to trigger splitter sync tabs/tools, but for any reason works this sometimes only after enough delay
        QTimer::singleShot(400, this, [this]() {
            // ensure we don't steal the focus, remember old focus widget
            QPointer<QWidget> oldFocusWidget(QApplication::focusWidget());

            for (auto &sidebar : m_sidebars) {
                sidebar->updateSidebar();
            }

            // ensure focus is not stolen, pass back to widget or at least central area
            if (oldFocusWidget) {
                oldFocusWidget->setFocus();
            } else {
                triggerFocusForCentralWidget();
            }
        });
    }

    // clear this stuff, we are done ;)
    m_restoreConfig = nullptr;
    m_restoreGroup.clear();
}

void MainWindow::saveSession(KConfigGroup &config)
{
    saveMainWindowSettings(config);

    // KWrite uses no sidebars, avoid all work beside windows sizes saving
    if (KateApp::isKWrite()) {
        return;
    }

    // save main splitter sizes ;)
    config.writeEntry("Kate-MDI-H-Splitter", m_hSplitter->sizes());
    config.writeEntry("Kate-MDI-V-Splitter", m_vSplitter->sizes());

    // save sidebar style
    config.writeEntry("Kate-MDI-Sidebar-Style", static_cast<int>(toolViewStyle()));
    config.writeEntry("Kate-MDI-Sidebar-Visible", m_sidebarsVisible);

    // save the sidebars
    for (auto &sidebar : m_sidebars) {
        sidebar->saveSession(config);
    }
}

QWidget *MainWindow::createContainer(QWidget *parent, int index, const QDomElement &element, QAction *&containerAction)
{
    // ensure we don't have toolbar accelerators that clash with other stuff
    QWidget *createdContainer = KParts::MainWindow::createContainer(parent, index, element, containerAction);
    if (element.tagName() == QLatin1String("ToolBar")) {
        KAcceleratorManager::setNoAccel(createdContainer);

        // ensure actions visible in any toolbar are hidden from the hamburger menu
        if (auto hamburgerMenu = static_cast<KHamburgerMenu *>(actionCollection()->action(QStringLiteral("hamburger_menu")))) {
            hamburgerMenu->hideActionsOf(createdContainer);
        }
    }
    return createdContainer;
}

// END MAIN WINDOW

} // namespace KateMDI

#include "moc_katemdi.cpp"
