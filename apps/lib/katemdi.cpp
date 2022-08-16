/*
    SPDX-FileCopyrightText: 2005 Christoph Cullmann <cullmann@kde.org>
    SPDX-FileCopyrightText: 2002, 2003 Joseph Wenninger <jowenn@kde.org>

    GUIClient partly based on ktoolbarhandler.cpp: SPDX-FileCopyrightText: 2002 Simon Hausmann <hausmann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "katemdi.h"
#include "kateapp.h"

#include "katedebug.h"

#include <KActionCollection>
#include <KActionMenu>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KMessageBox>
#include <KSharedConfig>
#include <KToolBar>
#include <KWindowConfig>
#include <KXMLGUIFactory>

#include <QContextMenuEvent>
#include <QDomDocument>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QSizePolicy>
#include <QStackedWidget>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>

#include <array>

namespace KateMDI
{
// BEGIN TOGGLETOOLVIEWACTION
//
ToggleToolViewAction::ToggleToolViewAction(const QString &text, ToolView *tv, QObject *parent)
    : KToggleAction(text, parent)
    , m_tv(tv)
{
    connect(this, &ToggleToolViewAction::toggled, this, &ToggleToolViewAction::slotToggled);
    connect(m_tv, &ToolView::toolVisibleChanged, this, &ToggleToolViewAction::toolVisibleChanged);

    setChecked(m_tv->toolVisible());
}

void ToggleToolViewAction::toolVisibleChanged(bool v)
{
    if (isChecked() != v) {
        setChecked(v);
    }
}

void ToggleToolViewAction::slotToggled(bool t)
{
    if (m_tv->toolVisible() == t) {
        return;
    }

    if (t) {
        m_tv->mainWindow()->showToolView(m_tv);
        m_tv->setFocus();
    } else {
        m_tv->mainWindow()->hideToolView(m_tv);
    }
}

// END TOGGLETOOLVIEWACTION

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

    m_toolMenu = new KActionMenu(i18n("Tool &Views"), this);
    actionCollection()->addAction(QStringLiteral("kate_mdi_toolview_menu"), m_toolMenu);
    m_showSidebarsAction = new KToggleAction(i18n("Show Side&bars"), this);
    actionCollection()->addAction(QStringLiteral("kate_mdi_sidebar_visibility"), m_showSidebarsAction);
    actionCollection()->setDefaultShortcut(m_showSidebarsAction, Qt::CTRL | Qt::ALT | Qt::SHIFT | Qt::Key_F);

    m_showSidebarsAction->setChecked(m_mw->sidebarsVisible());
    connect(m_showSidebarsAction, &KToggleAction::toggled, m_mw, &MainWindow::setSidebarsVisible);

    m_hideToolViews = actionCollection()->addAction(QStringLiteral("kate_mdi_hide_toolviews"), m_mw, &MainWindow::hideToolViews);
    m_hideToolViews->setText(i18n("Hide All Tool Views"));

    m_toolMenu->addAction(m_showSidebarsAction);
    m_toolMenu->addAction(m_hideToolViews);
    QAction *sep_act = new QAction(this);
    sep_act->setSeparator(true);
    m_toolMenu->addAction(sep_act);

    // read shortcuts
    actionCollection()->setConfigGroup(QStringLiteral("Shortcuts"));
    actionCollection()->readSettings();

    actionCollection()->addAssociatedWidget(m_mw);
    const auto actions = actionCollection()->actions();
    for (QAction *action : actions) {
        action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    }

    // hide tool views menu for KWrite mode
    if (KateApp::isKWrite()) {
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

    auto shortcutsForActionName = [](const QString &aname) {
        QList<QKeySequence> shortcuts;
        KSharedConfigPtr cfg = KSharedConfig::openConfig();
        const QString shortcutString = cfg->group("Shortcuts").readEntry(aname, QString());
        const auto shortcutStrings = shortcutString.split(QLatin1Char(';'));
        for (const QString &shortcut : shortcutStrings) {
            shortcuts << QKeySequence::fromString(shortcut);
        }
        return shortcuts;
    };

    /** Show ToolView Action **/
    KToggleAction *a = new ToggleToolViewAction(i18n("Show %1", tv->text), tv, this);
    actionCollection()->setDefaultShortcuts(a, shortcutsForActionName(aname));
    actionCollection()->addAction(aname, a);

    m_toolMenu->addAction(a);

    auto &actionsForTool = m_toolToActions[tv];
    actionsForTool.push_back(a);

    /** Show Tab button in sidebar action **/
    aname = QStringLiteral("kate_mdi_show_toolview_button_") + tv->id;
    a = new KToggleAction(i18n("Show %1 Button", tv->text), this);
    a->setChecked(true);
    actionCollection()->setDefaultShortcuts(a, shortcutsForActionName(aname));
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

    updateActions();
}

void GUIClient::unregisterToolView(ToolView *tv)
{
    auto &actionsForTool = m_toolToActions[tv];
    if (actionsForTool.empty())
        return;

    for (auto *a : actionsForTool) {
        delete a;
    }
    m_toolToActions.erase(tv);

    updateActions();
}

void GUIClient::clientAdded(KXMLGUIClient *client)
{
    if (client == this) {
        updateActions();
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

    plugActionList(actionListName, addList);
}

// END GUICLIENT

// BEGIN TOOLVIEW

ToolView::ToolView(MainWindow *mainwin, Sidebar *sidebar, QWidget *parent)
    : QFrame(parent)
    , m_mainWin(mainwin)
    , m_sidebar(sidebar)
    , m_toolbar(nullptr)
    , m_toolVisible(false)
{
    // try to fix resize policy
    QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    policy.setRetainSizeWhenHidden(true);
    setSizePolicy(policy);

    // per default vbox layout
    QVBoxLayout *layout = new QVBoxLayout(this);
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
    QVBoxLayout *layout = new QVBoxLayout(this);
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

    if (m_tabList.contains(id)) {
        // We are in session restore
        newTab = m_multiTabBar->tab(id);
        newTab->setIcon(tv->icon);
        newTab->setText(tv->text);
    } else {
        m_tabList.append(id);
        m_multiTabBar->appendTab(tv->icon, id, tv->text);
        newTab = m_multiTabBar->tab(id);
    }

    connect(newTab, &KMultiTabBarTab::clicked, this, &MultiTabBar::tabClicked);

    m_sb->m_widgetToTabBar.emplace(tv, this);

    return newTab;
}

int MultiTabBar::addBlankTab()
{
    int id = m_sb->nextId();
    m_tabList.append(id);
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
    m_tabList.removeAll(id);
    m_multiTabBar->removeTab(id);
    if (tabCount() == 0) {
        m_activeTab = 0;
        Q_EMIT lastTabRemoved(this);
    }
}

void MultiTabBar::removeTab(int id)
{
    m_tabList.removeAll(id);
    m_multiTabBar->removeTab(id);
    ToolView *tv = m_sb->m_idToWidget.at(id);
    m_sb->m_widgetToTabBar.erase(tv);

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
        hideToolView(m_sb->m_widgetToId.at(tv));
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
        m_sb->m_idToWidget.at(id)->setToolVisible(state);
        m_activeTab = state ? id : 0;
        return;
    }

    if (m_activeTab && state) {
        // Obviously the active tool is changed, disable the old one
        m_multiTabBar->setTab(m_activeTab, false);
        m_sb->m_idToWidget.at(m_activeTab)->setToolVisible(false);
    }

    m_multiTabBar->setTab(id, state);
    m_sb->m_idToWidget.at(id)->setToolVisible(state);
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

    ToolView *tv = m_sb->m_idToWidget.at(m_activeTab);
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
{
    setChildrenCollapsible(false);

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

    // shall we show text for the left and right bars?
    KSharedConfig::Ptr config = KSharedConfig::openConfig();
    KConfigGroup cgGeneral = KConfigGroup(config, "General");
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

    for (const auto &[id, wid] : m_idToWidget) {
        updateButtonStyle(kmTabBar(wid)->tab(id));
    }
}

void Sidebar::appendStyledTab(int id, MultiTabBar *bar, ToolView *widget)
{
    auto newTab = bar->addTab(id, widget);

    Q_ASSERT(newTab);
    newTab->installEventFilter(this);

    // remember original text, we will need it again if we update the style in updateButtonStyle
    newTab->setProperty("kate_original_text", widget->text);

    // fixup styling
    updateButtonStyle(newTab);
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

MultiTabBar *Sidebar::insertTabBar(int idx /* = -1*/)
{
    auto *newBar = new MultiTabBar(m_tabBarPosition, this, idx);
    newBar->installEventFilter(this);
    newBar->tabBar()->setStyle(tabStyle());
    insertWidget(idx, newBar);
    auto sections = sizes();
    // For whatever reason need the splitter some help for halfway nice new
    // section sizes. We only support with this math to insert below some section,
    // not above like at first place, but that's ok atm
    idx = idx < 0 ? sections.count() - 1 : idx;
    if (idx) {
        sections[idx] = sections.at(idx - 1) / 2;
        sections[idx - 1] = sections.at(idx);
        setSizes(sections);
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
        widget = new ToolView(m_mainWin, this, nullptr);
        widget->icon = icon;
        widget->text = text;
        widget->id = identifier;
    }

    widget->m_sidebar = this;

    auto blankTabId = m_tvIdToTabId.find(identifier);
    if (blankTabId != m_tvIdToTabId.end()) {
        int newId = blankTabId->second;
        m_idToWidget.emplace(newId, widget);
        m_widgetToId.emplace(widget, newId);
        appendStyledTab(newId, tabBar(m_tvIdToTabBar.at(identifier)), widget);
        // Indicate the blank tab is re-used
        m_tvIdToTabId.erase(identifier);
        m_tvIdToTabBar.erase(identifier);
    } else {
        int newId = nextId();
        m_idToWidget.emplace(newId, widget);
        m_widgetToId.emplace(widget, newId);
        appendStyledTab(newId, tabBar(0), widget);
    }

    show();

    return widget;
}

bool Sidebar::removeToolView(ToolView *widget)
{
    if (m_widgetToId.find(widget) == m_widgetToId.end()) {
        return false;
    }

    int id = m_widgetToId.at(widget);

    auto tbar = m_widgetToTabBar.at(widget);
    tbar->removeTab(id);

    m_idToWidget.erase(id);
    m_widgetToId.erase(widget);

    updateSidebar();

    return true;
}

bool Sidebar::showToolView(ToolView *widget)
{
    if (m_widgetToId.find(widget) == m_widgetToId.end()) {
        return false;
    }

    tabBar(widget)->showToolView(m_widgetToId.at(widget));
    updateSidebar();

    return true;
}

bool Sidebar::hideToolView(ToolView *widget)
{
    if (m_widgetToId.find(widget) == m_widgetToId.end()) {
        return false;
    }

    updateLastSize();
    tabBar(widget)->hideToolView(m_widgetToId.at(widget));
    updateSidebar();

    return true;
}

void Sidebar::showToolviewTab(ToolView *widget, bool show)
{
    auto it = m_widgetToId.find(widget);
    if (it == m_widgetToId.end()) {
        qWarning() << Q_FUNC_INFO << "Unexpected no id for widget " << widget;
        return;
    }
    auto *tab = kmTabBar(widget)->tab(it->second);
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
    moveSplitter(pos, index);
}

void Sidebar::barSplitMoved(int pos, int index)
{
    Q_UNUSED(pos);
    Q_UNUSED(index);
    adjustSplitterSections();
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
    m_mainWin->centralWidget()->setFocus();
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

    // Ensure we are visible
    m_ownSplit->show();
}

bool Sidebar::eventFilter(QObject *obj, QEvent *ev)
{
    if (ev->type() == QEvent::ContextMenu) {
        QContextMenuEvent *e = static_cast<QContextMenuEvent *>(ev);
        KMultiTabBarTab *bt = dynamic_cast<KMultiTabBarTab *>(obj);
        if (bt) {
            // qCDebug(LOG_KATE) << "Request for popup";

            m_popupButton = bt->id();

            ToolView *w = m_idToWidget[m_popupButton];

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

                int tabBarId = indexOf(m_widgetToTabBar.at(w));

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
        QMouseEvent *e = static_cast<QMouseEvent *>(ev);
        if (e->button() == Qt::LeftButton) {
            updateLastSize();
        }
    } else if (ev->type() == QEvent::MouseButtonPress) {
        QMouseEvent *e = static_cast<QMouseEvent *>(ev);
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
    }

    return false;
}

void Sidebar::setVisible(bool visible)
{
    // visible==true means show-request
    if (visible && (m_idToWidget.empty() || !m_mainWin->sidebarsVisible())) {
        return;
    }

    QWidget::setVisible(visible);
}

void Sidebar::buttonPopupActivate(QAction *a)
{
    const int id = a->data().toInt();
    ToolView *w = m_idToWidget[m_popupButton];

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
        auto newBar = insertTabBar(indexOf(m_widgetToTabBar.at(w)) + 1);
        tabBar(w)->removeTab(m_widgetToId.at(w));
        appendStyledTab(m_widgetToId.at(w), newBar, w);
        showToolView(w);
    }
    if (id == UpLeftAction) {
        auto newBar = tabBar(indexOf(tabBar(w)) - 1);
        tabBar(w)->removeTab(m_widgetToId.at(w));
        appendStyledTab(m_widgetToId.at(w), newBar, w);
    }
    if (id == DownRightAction) {
        auto newBar = tabBar(indexOf(tabBar(w)) + 1);
        tabBar(w)->removeTab(m_widgetToId.at(w));
        appendStyledTab(m_widgetToId.at(w), newBar, w);
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
    // Using splitter data avoid to store tabBarCount explicit ;-)
    QList<int> s = config.readEntry(QStringLiteral("Kate-MDI-Sidebar-%1-Splitter").arg(position()), QList<int>());
    // Notice the start value of 1, only add extra tab bars
    for (int i = 1; i < s.size(); ++i) {
        insertTabBar();
    }
    // Create for each tool we expect, in the correct order, a tab in advance, a blank one
    for (int i = 0; i < s.size(); ++i) {
        QStringList tvList = config.readEntry(QStringLiteral("Kate-MDI-Sidebar-%1-Bar-%2-TvList").arg(position()).arg(i), QStringList());
        for (int j = 0; j < tvList.size(); ++j) {
            // Don't add in case of a config mismatch blank tabs for some stuff twice
            auto search = m_tvIdToTabId.find(tvList.at(j));
            if (search == m_tvIdToTabId.end()) {
                int id = tabBar(i)->addBlankTab();
                m_tvIdToTabId.emplace(tvList.at(j), id);
                m_tvIdToTabBar.emplace(tvList.at(j), i);
            }
        }
    }
}

void Sidebar::restoreSession(KConfigGroup &config)
{
    // show only correct toolviews ;)
    for (const auto &[id, tv] : m_idToWidget) {
        tabBar(tv)->setTabActive(id, config.readEntry(QStringLiteral("Kate-MDI-ToolView-%1-Visible").arg(tv->id), false));
        showToolviewTab(tv, config.readEntry(QStringLiteral("Kate-MDI-ToolView-%1-Show-Button-In-Sidebar").arg(tv->id), true));
    }

    // In case of some config mismatch, we should remove left over blank tabs
    for (const auto &[tv, id] : m_tvIdToTabId) {
        tabBar(m_tvIdToTabBar.at(tv))->removeBlankTab(id);
    }
    m_tvIdToTabId.clear();
    m_tvIdToTabBar.clear();

    // In case of some config mismatch, we may have some ugly empty bars
    for (int i = 0; i < tabBarCount(); ++i) {
        // Don't panic! Function take care not to remove non empty bar
        if (tabBarIsEmpty(tabBar(i))) {
            // Now that the bar is gone, we need to adjust our index or we would skip some bar
            --i;
        }
    }

    // Collapse now, and before! we restore m_lastSize, will hide (all) m_stack(s) so that
    // expanding will work fine...
    collapseSidebar();
    m_lastSize = config.readEntry(QStringLiteral("Kate-MDI-Sidebar-%1-LastSize").arg(position()), 160);
    setSizes(config.readEntry(QStringLiteral("Kate-MDI-Sidebar-%1-Splitter").arg(position()), QList<int>()));
    // ...now we are ready to get the final splitter sizes by MainWindow::finishRestore
    updateSidebar();
}

void Sidebar::saveSession(KConfigGroup &config)
{
    config.writeEntry(QStringLiteral("Kate-MDI-Sidebar-%1-Splitter").arg(position()), sizes());
    config.writeEntry(QStringLiteral("Kate-MDI-Sidebar-%1-LastSize").arg(position()), m_lastSize);

    // store the data about all toolviews in this sidebar ;)
    for (const auto &[id, tv] : m_idToWidget) {
        config.writeEntry(QStringLiteral("Kate-MDI-ToolView-%1-Position").arg(tv->id), int(tv->sidebar()->position()));
        config.writeEntry(QStringLiteral("Kate-MDI-ToolView-%1-Visible").arg(tv->id), tv->toolVisible());
        config.writeEntry(QStringLiteral("Kate-MDI-ToolView-%1-Show-Button-In-Sidebar").arg(tv->id), tv->tabButtonVisible());
    }

    for (int i = 0; i < tabBarCount(); ++i) {
        QStringList tvList;
        for (int j : tabBar(i)->tabList()) {
            tvList << m_idToWidget.at(j)->id;
        }
        config.writeEntry(QStringLiteral("Kate-MDI-Sidebar-%1-Bar-%2-TvList").arg(position()).arg(i), tvList);
    }
}

// END SIDEBAR

// BEGIN MAIN WINDOW

MainWindow::MainWindow(QWidget *parentWidget)
    : KParts::MainWindow(parentWidget, Qt::Window)
    , m_guiClient(new GUIClient(this))
{
    // central frame for all stuff
    QFrame *hb = new QFrame(this);
    setCentralWidget(hb);

    // top level vbox for all stuff + bottom bar
    QVBoxLayout *toplevelVBox = new QVBoxLayout(hb);
    toplevelVBox->setContentsMargins(0, 0, 0, 0);
    toplevelVBox->setSpacing(0);

    // hbox for all splitters and other side bars
    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->setContentsMargins(0, 0, 0, 0);
    hlayout->setSpacing(0);
    toplevelVBox->addLayout(hlayout);

    m_hSplitter = new QSplitter(Qt::Horizontal, hb);
    m_sidebars[KMultiTabBar::Left] = std::make_unique<Sidebar>(KMultiTabBar::Left, m_hSplitter, this, hb);
    hlayout->addWidget(m_sidebars[KMultiTabBar::Left].get());
    hlayout->addWidget(m_hSplitter);

    QFrame *vb = new QFrame(m_hSplitter);
    QVBoxLayout *vlayout = new QVBoxLayout(vb);
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

    // bottom side bar spans full windows, include status bar, too
    m_sidebars[KMultiTabBar::Bottom] = std::make_unique<Sidebar>(KMultiTabBar::Bottom, m_vSplitter, this, vb);
    auto bottomHBoxLaout = new QHBoxLayout;
    bottomHBoxLaout->addWidget(m_sidebars[KMultiTabBar::Bottom].get());
    m_statusBarStackedWidget = new QStackedWidget(this);
    m_statusBarStackedWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    bottomHBoxLaout->addWidget(m_statusBarStackedWidget);
    bottomHBoxLaout->setStretch(0, 100);
    toplevelVBox->addLayout(bottomHBoxLaout);

    for (const auto &sidebar : m_sidebars) {
        connect(sidebar.get(), &Sidebar::sigShowPluginConfigPage, this, &MainWindow::sigShowPluginConfigPage);
    }
}

MainWindow::~MainWindow()
{
    // cu toolviews
    qDeleteAll(m_toolviews);

    // seems like we really should delete this by hand ;)
    delete m_centralWidget;
}

QWidget *MainWindow::centralWidget() const
{
    return m_centralWidget;
}

ToolView *MainWindow::createToolView(KTextEditor::Plugin *plugin,
                                     const QString &identifier,
                                     KMultiTabBar::KMultiTabBarPosition pos,
                                     const QIcon &icon,
                                     const QString &text)
{
    if (m_idToWidget[identifier]) {
        return nullptr;
    }

    // try the restore config to figure out real pos
    if (m_restoreConfig && m_restoreConfig->hasGroup(m_restoreGroup)) {
        KConfigGroup cg(m_restoreConfig, m_restoreGroup);
        pos = static_cast<KMultiTabBar::KMultiTabBarPosition>(cg.readEntry(QStringLiteral("Kate-MDI-ToolView-%1-Position").arg(identifier), int(pos)));
    }

    ToolView *v = m_sidebars[pos]->addToolView(icon, text, identifier, nullptr);
    v->plugin = plugin;

    m_idToWidget.emplace(identifier, v);
    m_toolviews.push_back(v);

    // register for menu stuff
    m_guiClient->registerToolView(v);

    return v;
}

ToolView *MainWindow::toolView(const QString &identifier) const
{
    auto it = m_idToWidget.find(identifier);
    if (it != m_idToWidget.end()) {
        return it->second;
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

    m_idToWidget.erase(widget->id);

    m_toolviews.erase(std::remove(m_toolviews.begin(), m_toolviews.end(), widget), m_toolviews.end());
}

void MainWindow::setSidebarsVisibleInternal(bool visible, bool noWarning)
{
    bool old_visible = m_sidebarsVisible;
    m_sidebarsVisible = visible;

    for (auto &sidebar : m_sidebars) {
        sidebar->setVisible(visible);
    }

    m_guiClient->updateSidebarsVisibleAction();

    // show information message box, if the users hides the sidebars
    if (!noWarning && old_visible && (!m_sidebarsVisible)) {
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

bool MainWindow::moveToolView(ToolView *widget, KMultiTabBar::KMultiTabBarPosition pos)
{
    if (!widget || widget->mainWindow() != this) {
        return false;
    }

    // try the restore config to figure out real pos
    if (m_restoreConfig && m_restoreConfig->hasGroup(m_restoreGroup)) {
        KConfigGroup cg(m_restoreConfig, m_restoreGroup);
        pos = static_cast<KMultiTabBar::KMultiTabBarPosition>(cg.readEntry(QStringLiteral("Kate-MDI-ToolView-%1-Position").arg(widget->id), int(pos)));
    }

    m_sidebars[pos]->addToolView(widget->icon, widget->text, widget->id, widget);

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
    m_centralWidget->setFocus();
    return ret;
}

void MainWindow::hideToolViews()
{
    for (const auto &tv : m_toolviews) {
        if (tv) {
            tv->sidebar()->hideToolView(tv);
        }
    }
    m_centralWidget->setFocus();
}

void MainWindow::startRestore(KConfigBase *config, const QString &group)
{
    // first save this stuff
    m_restoreConfig = config;
    m_restoreGroup = group;

    if (!m_restoreConfig || !m_restoreConfig->hasGroup(m_restoreGroup)) {
        return;
    }

    // apply size once, to get sizes ready ;)
    KConfigGroup cg(m_restoreConfig, m_restoreGroup);
    KWindowConfig::restoreWindowSize(windowHandle(), cg);

    // restore the sidebars
    for (auto &sidebar : qAsConst(m_sidebars)) {
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
        for (const auto tv : m_toolviews) {
            KMultiTabBar::KMultiTabBarPosition newPos = static_cast<KMultiTabBar::KMultiTabBarPosition>(
                cg.readEntry(QStringLiteral("Kate-MDI-ToolView-%1-Position").arg(tv->id), int(tv->sidebar()->position())));

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
            for (auto &sidebar : m_sidebars) {
                sidebar->updateSidebar();
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

// END MAIN WINDOW

} // namespace KateMDI
