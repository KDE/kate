/*
    SPDX-FileCopyrightText: 2005 Christoph Cullmann <cullmann@kde.org>
    SPDX-FileCopyrightText: 2002, 2003 Joseph Wenninger <jowenn@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef __KATE_MDI_H__
#define __KATE_MDI_H__

#include <KTextEditor/Plugin>

#include <KParts/MainWindow>

#include <KMultiTabBar>
#include <KToggleAction>
#include <KXMLGUIClient>

#include <QChildEvent>
#include <QEvent>
#include <QFrame>
#include <QPointer>
#include <QSplitter>

#include <map>
#include <unordered_map>
#include <vector>

class KActionMenu;
class QAction;
class QLabel;
class QPixmap;
class QStackedWidget;
class KConfigBase;

namespace KTextEditor
{
class ConfigPageInterface;
}

namespace KateMDI
{
class ToolView;

class ToggleToolViewAction : public KToggleAction
{
    Q_OBJECT

public:
    ToggleToolViewAction(const QString &text, ToolView *tv, QObject *parent);

protected Q_SLOTS:
    void slotToggled(bool) override;
    void toolVisibleChanged(bool);

private:
    ToolView *m_tv;
};

class GUIClient : public QObject, public KXMLGUIClient
{
    Q_OBJECT

public:
    GUIClient(class MainWindow *mw);

    void registerToolView(ToolView *tv);
    void unregisterToolView(ToolView *tv);
    void updateSidebarsVisibleAction();

private Q_SLOTS:
    void clientAdded(KXMLGUIClient *client);
    void updateActions();

private:
    MainWindow *m_mw;
    KToggleAction *m_showSidebarsAction;
    std::unordered_map<ToolView *, std::vector<QAction *>> m_toolToActions;
    KActionMenu *m_toolMenu;
    QAction *m_hideToolViews;
    KActionMenu *m_sidebarButtonsMenu;
    KActionMenu *m_focusToolviewMenu;
};

class ToolView : public QFrame
{
    Q_OBJECT

    friend class Sidebar;
    friend class MultiTabBar;
    friend class MainWindow;
    friend class GUIClient;
    friend class ToggleToolViewAction;

protected:
    /**
     * ToolView
     * Objects of this clas represent a toolview in the mainwindow
     * you should only add one widget as child to this toolview, it will
     * be automatically set to be the focus proxy of the toolview
     * @param mainwin main window for this toolview
     * @param sidebar sidebar of this toolview
     * @param parent parent widget, e.g. the splitter of one of the sidebars
     * @param identifier unique id
     */
    ToolView(class MainWindow *mainwin, class Sidebar *sidebar, QWidget *parent, const QString &identifier);

public:
    /**
     * destruct me, this is allowed for all, will care itself that the toolview is removed
     * from the mainwindow and sidebar
     */
    ~ToolView() override;

Q_SIGNALS:
    /**
     * toolview hidden or shown
     * @param visible is this toolview made visible?
     */
    void toolVisibleChanged(bool visible);

    void tabButtonVisibleChanged(bool visible);

    /**
     * some internal methodes needed by the main window and the sidebars
     */
protected:
    MainWindow *mainWindow()
    {
        return m_mainWin;
    }

    Sidebar *sidebar()
    {
        return m_sidebar;
    }

    void setToolVisible(bool vis);

public:
    bool toolVisible() const;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    /**
     * Whether the tab button for this toolview is visible
     * in the sidebar or hidden
     */
    bool tabButtonVisible() const;
    void setTabButtonVisible(bool visible);

protected:
    void childEvent(QChildEvent *ev) override;
    void actionEvent(QActionEvent *event) override;

private:
    MainWindow *m_mainWin;
    Sidebar *m_sidebar;
    KToolBar *m_toolbar;

    /// plugin this view belongs to, may be 0
    QPointer<KTextEditor::Plugin> plugin;

    /**
     * unique id
     */
    const QString id;

    /**
     * is visible in sidebar
     */
    bool m_toolVisible;

    /**
     * Is the button visible in sidebar
     */
    bool isTabButtonVisible = true;

    /**
     * is this view persistent?
     */
    bool persistent;

    QIcon icon;
    QString text;
};

class MultiTabBar : public QWidget
{
    Q_OBJECT

public:
    MultiTabBar(KMultiTabBar::KMultiTabBarPosition pos, Sidebar *sb, int idx);
    ~MultiTabBar();

    KMultiTabBarTab *addTab(int id, ToolView *tv);
    int addBlankTab();
    void removeBlankTab(int id);
    void removeTab(int id);

    void showToolView(int id);
    void hideToolView(int id);

    void setTabActive(int id, bool state);

    bool isToolActive() const;
    void collapseToolView() const;
    bool expandToolView() const;

    KMultiTabBar *tabBar() const
    {
        return m_multiTabBar;
    }

    QList<int> tabList() const
    {
        return m_tabList;
    }

    int tabCount() const
    {
        return m_tabList.size();
    }

    int sectionSize() const
    {
        return m_sectionSize;
    }

    void setSectionSize(int size)
    {
        m_sectionSize = size;
    }

Q_SIGNALS:
    void lastTabRemoved(MultiTabBar *);

private Q_SLOTS:
    void tabClicked(int);

private:
    Sidebar *m_sb;
    QStackedWidget *m_stack;
    KMultiTabBar *m_multiTabBar;
    QList<int> m_tabList;
    int m_activeTab = 0;
    int m_sectionSize = 0;
};

class Sidebar : public QSplitter
{
    Q_OBJECT

    friend class MultiTabBar;

public:
    Sidebar(KMultiTabBar::KMultiTabBarPosition pos, QSplitter *sp, class MainWindow *mainwin, QWidget *parent);

    QSize sizeHint() const override;

    ToolView *addToolView(const QIcon &icon, const QString &text, const QString &identifier, ToolView *widget);
    bool removeToolView(ToolView *widget);

    bool showToolView(ToolView *widget);
    bool hideToolView(ToolView *widget);

    void showToolviewTab(ToolView *widget, bool show);

    bool isCollapsed();

    QWidget *tabButtonForToolview(ToolView *widget) const
    {
        auto it = m_widgetToId.find(widget);
        if (it == m_widgetToId.end()) {
            return nullptr;
        }
        if (auto tabbar = kmTabBar(widget)) {
            return tabbar->tab(it->second);
        }
        return nullptr;
    }

    /**
     * Will the sidebar expand when some tool has to be visible in any section,
     * or calling collapseSidebar() if non such tool is found
     */
    void updateSidebar();
    void collapseSidebar();

    KMultiTabBar::KMultiTabBarPosition position() const
    {
        return m_tabBarPosition;
    }

    bool isVertical() const
    {
        return m_tabBarPosition == KMultiTabBar::Right || m_tabBarPosition == KMultiTabBar::Left;
    }

    void setStyle(KMultiTabBar::KMultiTabBarStyle style);

    KMultiTabBar::KMultiTabBarStyle tabStyle() const
    {
        return m_tabBarStyle;
    }

    void startRestoreSession(KConfigGroup &config);

    /**
     * restore the current session config from given object, use current group
     * @param config config object to use
     */
    void restoreSession(KConfigGroup &config);

    /**
     * save the current session config to given object, use current group
     * @param config config object to use
     */
    void saveSession(KConfigGroup &config);

public Q_SLOTS:
    // reimplemented, to block a show() call if all sidebars are forced hidden
    void setVisible(bool visible) override;

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;

private Q_SLOTS:
    void buttonPopupActivate(QAction *);
    void readConfig();
    void handleCollapse(int pos, int index);
    void ownSplitMoved(int pos, int index);
    void barSplitMoved(int pos, int index);
    bool tabBarIsEmpty(MultiTabBar *bar);

private:
    void updateLastSize();
    int nextId();
    bool adjustSplitterSections();

    /**
     * Append a tab with our styling & needed connections/event filter.
     */
    void appendStyledTab(int id, MultiTabBar *bar, ToolView *widget);

    /**
     * Update style of button to our style.
     */
    void updateButtonStyle(KMultiTabBarTab *button);

    /**
     * Monitor resizes using the mouse and update the last size accordingly.
     */
    void updateLastSizeOnResize();

    MultiTabBar *insertTabBar(int idx = -1);

    MultiTabBar *tabBar(int idx) const
    {
        return static_cast<MultiTabBar *>(widget(idx));
    }

    MultiTabBar *tabBar(ToolView *tv) const
    {
        return m_widgetToTabBar.at(tv);
    }

    KMultiTabBar *kmTabBar(ToolView *widget) const
    {
        return m_widgetToTabBar.at(widget)->tabBar();
    }

    int tabBarCount() const
    {
        return count();
    }

private:
    enum ActionIds {
        HideButtonAction = 11,
        ConfigureAction = 20,
        ToOwnSectAction = 30,
        UpLeftAction = 31,
        DownRightAction = 32,
    };

    MainWindow *m_mainWin;

    KMultiTabBar::KMultiTabBarPosition m_tabBarPosition{};
    KMultiTabBar::KMultiTabBarStyle m_tabBarStyle{};
    QSplitter *m_splitter;
    QSplitter *m_ownSplit;
    const int m_ownSplitIndex;

    std::map<int, ToolView *> m_idToWidget;
    std::map<ToolView *, int> m_widgetToId;
    std::map<ToolView *, MultiTabBar *> m_widgetToTabBar;

    // Session restore only
    std::map<QString, int> m_tvIdToTabId;
    std::map<QString, int> m_tvIdToTabBar;
    bool m_sessionRestoreRunning = false;

    int m_lastSize;
    int m_popupButton = 0;
    QPointer<QLabel> m_resizePlaceholder;
    bool m_isPreviouslyCollapsed = false;
    bool m_syncWithTabs = false;
    bool m_showTextForLeftRight = false;
    int m_leftRightSidebarIconSize = 32;

Q_SIGNALS:
    void sigShowPluginConfigPage(KTextEditor::Plugin *configpageinterface, int id);
};

class MainWindow : public KParts::MainWindow
{
    Q_OBJECT

    friend class ToolView;

    //
    // Constructor area
    //
public:
    /**
     * Constructor
     */
    MainWindow(QWidget *parentWidget = nullptr);

    /**
     * Destructor
     */
    ~MainWindow() override;

    //
    // public interfaces
    //

    /**
     * add a given widget to the given sidebar if possible, name is very important
     * @param plugin pointer to the plugin
     * @param identifier unique identifier for this toolview
     * @param pos position for the toolview, if we are in session restore, this is only a preference
     * @param icon icon to use for the toolview
     * @param text text to use in addition to icon
     * @return created toolview on success or 0
     */
    ToolView *
    createToolView(KTextEditor::Plugin *plugin, const QString &identifier, KMultiTabBar::KMultiTabBarPosition pos, const QIcon &icon, const QString &text);

    /**
     * give you handle to toolview for the given name, 0 if no toolview around
     * @param identifier toolview name
     * @return toolview if existing, else 0
     */
    ToolView *toolView(const QString &identifier) const;

    /**
     * get the sidebars' visibility.
     * @return false, if the sidebars' visibility is forced hidden, otherwise true
     */
    bool sidebarsVisible() const;

    /**
     * set the sidebars' visibility to @p visible. If false, the sidebars
     * are @e always hidden. Usually you do not have to call this because
     * the user can set this in the menu.
     * @param visible sidebars visibility
     * @param noWarning avoid to warn about hiding the stuff
     */
    void setSidebarsVisibleInternal(bool visible, bool noWarning);

public Q_SLOTS:
    /**
     * set the sidebars' visibility to @p visible. If false, the sidebars
     * are @e always hidden. Usually you do not have to call this because
     * the user can set this in the menu.
     * @param visible sidebars visibility
     */
    void setSidebarsVisible(bool visible)
    {
        setSidebarsVisibleInternal(visible, false);
    }

    /**
     * hide all tool views
     */
    void hideToolViews();

protected:
    /**
     * called by toolview destructor
     * @param widget toolview which is destroyed
     */
    void toolViewDeleted(ToolView *widget);

    /**
     * set the toolview's tabbar style.
     * @param style the tabbar style.
     */
    void setToolViewStyle(KMultiTabBar::KMultiTabBarStyle style);

    /**
     * get the toolview's tabbar style. Call this before @p startRestore(),
     * otherwise you overwrite the usersettings.
     * @return toolview's tabbar style
     */
    KMultiTabBar::KMultiTabBarStyle toolViewStyle() const;

public:
    /**
     * central widget ;)
     * use this as parent for your content
     * @return central widget
     */
    QWidget *centralWidget() const;

    /**
     * Called if focus should go to the important part of the central widget.
     * Default doesn't do a thing, if we are called in destructor by accident.
     */
    virtual void triggerFocusForCentralWidget()
    {
    }

protected:
    /**
     * Status bar area stacked widget.
     * We plug in our status bars from the KTextEditor::Views here
     */
    QStackedWidget *statusBarStackedWidget() const
    {
        return m_statusBarStackedWidget;
    }

    /**
     * modifiers for existing toolviews
     */
public:
    /**
     * move a toolview around
     * @param widget toolview to move
     * @param pos position to move too, during session restore, only preference
     * @return success
     */
    bool moveToolView(ToolView *widget, KMultiTabBar::KMultiTabBarPosition pos);

    /**
     * show given toolview, discarded while session restore
     * @param widget toolview to show
     * @return success
     */
    bool showToolView(ToolView *widget);

    /**
     * hide given toolview, discarded while session restore
     * @param widget toolview to hide
     * @return success
     */
    bool hideToolView(ToolView *widget);

    QWidget *toolviewToggleButton(ToolView *tv);

    /**
     * session saving and restore stuff
     */
public:
    /**
     * start the restore
     * @param config config object to use
     * @param group config group to use
     */
    void startRestore(KConfigBase *config, const QString &group);

    /**
     * finish the restore
     */
    void finishRestore();

    /**
     * save the current session config to given object and group
     * @param group config group to use
     */
    void saveSession(KConfigGroup &group);

    /**
     * internal data ;)
     */
private:
    /**
     * all existing tool views
     * mapped by their constant identifier, to have some stable order
     * tool views de-register them self on destruction
     */
    std::map<QString, ToolView *> m_toolviews;

    /**
     * widget, which is the central part of the
     * main window ;)
     */
    QWidget *m_centralWidget;

    /**
     * horizontal splitter
     */
    QSplitter *m_hSplitter;

    /**
     * vertical splitter
     */
    QSplitter *m_vSplitter;

    /**
     * sidebars for the four sides
     */
    std::unique_ptr<Sidebar> m_sidebars[4];

    /**
     * sidebars state.
     */
    bool m_sidebarsVisible = true;

    /**
     * config object for session restore, only valid between
     * start and finish restore calls
     */
    KConfigBase *m_restoreConfig = nullptr;

    /**
     * restore group
     */
    QString m_restoreGroup;

    /**
     * out guiclient
     */
    GUIClient *m_guiClient;

    /**
     * stacked widget for status bars
     */
    QStackedWidget *m_statusBarStackedWidget;

Q_SIGNALS:
    void sigShowPluginConfigPage(KTextEditor::Plugin *configpageinterface, int id);
};

}

#endif
