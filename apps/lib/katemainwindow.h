/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2001 Christoph Cullmann <cullmann@kde.org>
   SPDX-FileCopyrightText: 2001 Joseph Wenninger <jowenn@kde.org>
   SPDX-FileCopyrightText: 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include "katemdi.h"
#include "kateprivate_export.h"
#include "kateviewmanager.h"
#include "mainwindow_commands.h"

#include <ktexteditor/mainwindow.h>

#include <QHash>
#include <QStackedLayout>
#include <QUrl>

// makes no sense on macOS
#ifdef Q_OS_MACOS
#define KATE_ALLOW_MENU_BAR_HIDE 0
#else
#define KATE_ALLOW_MENU_BAR_HIDE 1
#endif

class QMenu;

namespace KIO
{
class UDSEntry;
typedef class QList<UDSEntry> UDSEntryList;
class Job;
}

class KFileItem;
class KRecentFilesAction;

class KateOutputView;
class KateViewManager;
class KateMwModOnHdDialog;
class QDragEnterEvent;
class QDropEvent;
class QEvent;

// Helper layout class to always provide minimum size
class KateContainerStackedLayout : public QStackedLayout
{
public:
    explicit KateContainerStackedLayout(QWidget *parent);
    QSize sizeHint() const override;
    QSize minimumSize() const override;
};

class KATE_PRIVATE_EXPORT KateMainWindow : public KateMDI::MainWindow, virtual public KParts::PartBase
{
    Q_OBJECT

public:
    /**
     * Construct the window and restore its state from given config if any
     * @param sconfig session config for this window, 0 if none
     * @param sgroup session config group to use
     * @param userTriggered is the user creating this window or is it part of session restore / startup
     */
    KateMainWindow(KConfig *sconfig, const QString &sgroup, bool userTriggered);

    /**
     * Destruct the nice window
     */
    ~KateMainWindow() override;

    /**
     * Accessor methodes for interface and child objects
     */
public:
    KateViewManager *viewManager()
    {
        return m_viewManager;
    }

    /**
     * KTextEditor::MainWindow wrapper
     * @return KTextEditor::MainWindow wrapper.
     */
    KTextEditor::MainWindow *wrapper()
    {
        return m_wrapper;
    }

public:
    /** Returns the URL of the current document.
     * anders: I add this for use from the file selector. */
    QUrl activeDocumentUrl();

    /** Enumeration to specify if files modified on disk should show up
     * in the reload dialog even if not edited in this instance. */
    enum ModOnDiskMode {
        PromptEdited, ///< Do not list files that have not been edited
        PromptAll, ///< Include all files modified on disk
    };

    /**
     * Prompts the user for what to do with files that are modified on disk if any.
     * This is optionally run when the window receives focus, and when the last
     * window is closed.
     * @return true if no documents are modified on disk, or all documents were
     * handled by the dialog; otherwise (the dialog was canceled) false.
     */
    bool showModOnDiskPrompt(ModOnDiskMode mode);

public:
    void readProperties(const KConfigGroup &config) override;
    void saveProperties(KConfigGroup &config) override
    {
        saveProperties(config, true);
    }
    void saveGlobalProperties(KConfig *sessionConfig) override;

    void saveProperties(KConfigGroup &config, bool includeViewConfig);

    void saveOpenRecent(KConfig *config);
    void loadOpenRecent(const KConfig *config);

public:
    bool queryClose_internal(KTextEditor::Document *doc = nullptr, KateMainWindow *win = nullptr);

    /**
     * save the settings, size and state of this window in
     * the provided config group
     */
    void saveWindowConfig(const KConfigGroup &);
    /**
     * restore the settings, size and state of this window from
     * the provided config group.
     */
    void restoreWindowConfig(const KConfigGroup &);

    /**
     * save some global options to katerc
     */
    void saveOptions();

    /**
     * Called if focus should go to the important part of the central widget.
     */
    void triggerFocusForCentralWidget() override
    {
        // just dispatch to view manager
        m_viewManager->triggerActiveViewFocus();
    }

private:
    /**
     * Setup actions which pointers are needed already in setupMainWindow
     */
    void setupImportantActions();

    void setupMainWindow();
    void setupActions();
    bool queryClose() override;

    void setupDiagnosticsView(KConfig *);

    /**
     * read some global options from katerc
     */
    void readOptions();

    void dragEnterEvent(QDragEnterEvent *) override;
    void dropEvent(QDropEvent *) override;

public:
    void slotFileQuit();
    void queueModifiedOnDisc(KTextEditor::Document *doc);

    void slotFocusPrevTab();
    void slotFocusNextTab();
    void slotSwitchToTab(const int i);

    /**
     * Show quick open
     */
    void slotQuickOpen();

    /**
     * Overwrite size hint for better default window sizes
     * @return size hint
     */
    QSize sizeHint() const override;

    /**
     * Create a new window.
     * Inherit size from current one.
     * @return new window
     */
    KateMainWindow *newWindow() const;

    /**
     * slots used for actions in the menus/toolbars
     * or internal signal connections
     */
private Q_SLOTS:
    // Connected to KTextEditor::View dropEventPass private signal so marked as Q_SLOT.
    // TODO make the api public as it is apparently needed
    void slotDropEvent(QDropEvent *);

private:
    void slotConfigure();

    void slotOpenWithMenuAction(QAction *a);

    void slotEditToolbars();
    void slotNewToolbarConfig();
    void slotUpdateActionsNeedingUrl();

    void editKeys();
    void mSlotFixOpenWithMenu();
    static void reloadXmlGui();

    /* to update the caption */
    void slotDocumentCreated(KTextEditor::Document *doc);
    void updateCaption(KTextEditor::Document *doc);
    // calls updateCaption(doc) with the current document
    void updateCaption();

    static void pluginHelp();
    void slotFullScreen(bool);

    void slotListRecursiveEntries(KIO::Job *job, const KIO::UDSEntryList &list);

    void onApplicationStateChanged(Qt::ApplicationState);

private:
#if KATE_ALLOW_MENU_BAR_HIDE
    void toggleShowMenuBar(bool showMessage = true);
    void ensureHamburgerBarSize();
    void updateHamburgerMenu();
#endif

    void toggleShowStatusBar();
    void toggleShowTabBar();

public:
    bool showStatusBar();
    bool showTabBar();

Q_SIGNALS:
    void tabBarToggled();
    void unhandledShortcutOverride(QEvent *e);

public:
    void openUrl(const QString &name = QString());

    QHash<KTextEditor::Plugin *, QObject *> &pluginViews()
    {
        return m_pluginViews;
    }

    QWidget *bottomViewBarContainer()
    {
        return m_bottomViewBarContainer;
    }

    void addToBottomViewBarContainer(KTextEditor::View *view, QWidget *bar)
    {
        m_bottomContainerStack->addWidget(bar);
        m_bottomViewBarMapping[view] = BarState(bar);
    }

    void hideBottomViewBarForView(KTextEditor::View *view)
    {
        BarState &state = m_bottomViewBarMapping[view];
        if (state.bar()) {
            m_bottomContainerStack->setCurrentWidget(state.bar());
            state.bar()->hide();
            state.setState(false);
        }
        m_bottomViewBarContainer->hide();
    }

    void showBottomViewBarForView(KTextEditor::View *view)
    {
        BarState &state = m_bottomViewBarMapping[view];
        if (state.bar()) {
            m_bottomContainerStack->setCurrentWidget(state.bar());
            state.bar()->show();
            state.setState(true);
            m_bottomViewBarContainer->show();
        }
    }

    void deleteBottomViewBarForView(KTextEditor::View *view)
    {
        BarState state = m_bottomViewBarMapping.take(view);
        if (state.bar()) {
            if (m_bottomContainerStack->currentWidget() == state.bar()) {
                m_bottomViewBarContainer->hide();
            }
            delete state.bar();
            delete state.statusBar();
        }
    }

    bool modNotificationEnabled() const
    {
        return m_modNotification;
    }

    void setModNotificationEnabled(bool e)
    {
        m_modNotification = e;
    }

    bool modCloseAfterLast() const
    {
        return m_modCloseAfterLast;
    }

    void setModCloseAfterLast(bool e)
    {
        m_modCloseAfterLast = e;
    }

    /**
     * add given url to list of recently opened files
     * @param url url that got opened
     */
    void addRecentOpenedFile(const QUrl &url);

    KRecentFilesAction *recentFilesAction()
    {
        return m_fileOpenRecent;
    }

    /**
     * activate this kate window
     * @param token activation token, needed for X11 or wayland
     */
    void activate(const QString &token = QString());

    //
    // KTextEditor::MainWindow interface, get called by invokeMethod from our wrapper object!
    //
public Q_SLOTS:
    /**
     * get the toplevel widget.
     * \return the real main window widget.
     */
    QWidget *window()
    {
        return this;
    }

    /**
     * Accessor to the XMLGUIFactory.
     * \return the mainwindow's KXMLGUIFactory.
     */
    KXMLGUIFactory *guiFactory() override
    {
        return KateMDI::MainWindow::guiFactory();
    }

    /**
     * Get a list of all views for this main window.
     * @return all views
     */
    QList<KTextEditor::View *> views()
    {
        return viewManager()->views();
    }

    /**
     * Access the active view.
     * \return active view
     */
    KTextEditor::View *activeView()
    {
        return viewManager()->activeView();
    }

    /**
     * Activate the view with the corresponding \p document.
     * If none exist for this document, create one
     * \param document the document
     * \return activated view of this document
     */
    KTextEditor::View *activateView(KTextEditor::Document *document);

    /**
     * Open the document \p url with the given \p encoding.
     * \param url the document's url
     * \param encoding the preferred encoding. If encoding is QString() the
     *        encoding will be guessed or the default encoding will be used.
     * \return a pointer to the created view for the new document, if a document
     *         with this url is already existing, its view will be activated
     */
    KTextEditor::View *openUrl(const QUrl &url, const QString &encoding = QString())
    {
        return viewManager()->openUrlWithView(url, encoding);
    }

    /**
     * Close selected view
     * \param view the view
     * \return true if view was closed
     */
    bool closeView(KTextEditor::View *view)
    {
        m_viewManager->closeView(view);
        return true;
    }

    /**
     * Close the split view where the given view is contained.
     * \param view the view.
     * \return true if the split view was closed.
     */
    bool closeSplitView(KTextEditor::View *view)
    {
        m_viewManager->closeViewSpace(view);
        return true;
    }

    /**
     * @returns true if the two given views share the same split view,
     * false otherwise.
     */
    bool viewsInSameSplitView(KTextEditor::View *view1, KTextEditor::View *view2)
    {
        return m_viewManager->viewsInSameViewSpace(view1, view2);
    }

    /**
     * Split current view space according to \p orientation
     * \param orientation in which line split the view
     */
    void splitView(Qt::Orientation orientation)
    {
        m_viewManager->splitViewSpace(nullptr, orientation);
    }

    /**
     * Try to create a view bar for the given view.
     * Its parameter is the view for which we want a view bar
     * @return suitable widget that can host view bars widgets or nullptr
     */
    QWidget *createViewBar(KTextEditor::View *)
    {
        return bottomViewBarContainer();
    }

    /**
     * Delete the view bar for the given view.
     * @param view view for which we want an view bar
     */
    void deleteViewBar(KTextEditor::View *view)
    {
        deleteBottomViewBarForView(view);
    }

    /**
     * Add a widget to the view bar.
     * @param view view for which the view bar is used
     * @param bar bar widget, shall have the viewBarParent() as parent widget
     */
    void addWidgetToViewBar(KTextEditor::View *view, QWidget *bar)
    {
        addToBottomViewBarContainer(view, bar);
    }

    /**
     * Show the view bar for the given view
     * @param view view for which the view bar is used
     */
    void showViewBar(KTextEditor::View *view)
    {
        showBottomViewBarForView(view);
    }

    /**
     * Hide the view bar for the given view
     * @param view view for which the view bar is used
     */
    void hideViewBar(KTextEditor::View *view)
    {
        hideBottomViewBarForView(view);
    }

    void insertWidgetInStatusbar(QWidget *w)
    {
        insertWidgetBeforeStatusbar(w);
    }

    /**
     * Create a new toolview with unique \p identifier at side \p pos
     * with \p icon and caption \p text. Use the returned widget to embed
     * your widgets.
     * \param plugin which owns this tool view
     * \param identifier unique identifier for this toolview
     * \param pos position for the toolview, if we are in session restore,
     *        this is only a preference
     * \param icon icon to use in the sidebar for the toolview
     * \param text translated text (i18n()) to use in addition to icon
     * \return created toolview on success, otherwise NULL
     */
    QWidget *createToolView(KTextEditor::Plugin *plugin,
                            const QString &identifier,
                            KTextEditor::MainWindow::ToolViewPosition pos,
                            const QIcon &icon,
                            const QString &text);

    /**
     * Find the toolview with the given \p identifier
     * \param identifier identifier as specified in createToolView()
     * \return pointer to the toolview, if found, otherwise NULL
     */
    QWidget *toolviewForName(const QString &identifier)
    {
        return toolView(identifier);
    }

    QWidget *activeViewToolView(int pos)
    {
        return KateMDI::MainWindow::activeViewToolView(static_cast<KMultiTabBar::KMultiTabBarPosition>(pos));
    }

    /**
     * Move the toolview \p widget to position \p pos.
     * \param widget the toolview to move, where the widget was constructed
     *        by createToolView().
     * \param pos new position to move widget to
     * \return \e true on success, otherwise \e false
     */
    bool moveToolView(QWidget *widget, KTextEditor::MainWindow::ToolViewPosition pos);

    /**
     * Show the toolview \p widget.
     * \param widget the toolview to show, where the widget was constructed
     *        by createToolView().
     * \return \e true on success, otherwise \e false
     * \todo add focus parameter: bool showToolView (QWidget *widget, bool giveFocus );
     */
    bool showToolView(QWidget *widget);

    /**
     * Hide the toolview \p widget.
     * \param widget the toolview to hide, where the widget was constructed
     *        by createToolView().
     * \return \e true on success, otherwise \e false
     */
    bool hideToolView(QWidget *widget);

    /**
     * Shows the @p plugin's config page. The @p page specifies which
     * config page will be shown, see KTextEditor::Plugin::configPages().
     *
     * \return \e true on success, otherwise \e false
     * \since 5.63
     */
    bool showPluginConfigPage(KTextEditor::Plugin *configpageinterface, int id);

    /**
     * Get a plugin view for the plugin with with identifier \p name.
     * \param name the plugin's name
     * \return pointer to the plugin view if a plugin with \p name is loaded and has a view for this mainwindow,
     *         otherwise NULL
     */
    QObject *pluginView(const QString &name);

    /**
     * Add a widget to the main window.
     * This is useful to show non-KTextEditor::View widgets in the main window.
     * The host application should try to manage this like some KTextEditor::View (e.g. as a tab) and provide
     * the means to close it.
     * \param widget widget to add
     * \return success, if false, the plugin needs to take care to show the widget itself, otherwise
     *         the main window will take ownership of the widget
     */
    bool addWidget(QWidget *widget);

    /**
     * \brief returns the list of non-KTextEditor::View widgets in this main window.
     * \see addWidget
     */
    QWidgetList widgets() const;

    /**
     * \brief remove this \p widget from this mainwindow. The widget will be deleted afterwards
     * \param widget the widget to be removed
     * \return true on success
     */
    bool removeWidget(QWidget *widget);

    /**
     * \brief returns the currently active widget. It can be a
     * non-KTextEditor::View widget or a KTextEditor::View
     */
    QWidget *activeWidget();

    /**
     * \brief activate @p widget. If the widget is not present in the window, it will be added to the window
     * \param widget the widget to activate
     */
    void activateWidget(QWidget *widget);

    /**
     * @brief show a message in the output view. See KTextEditor::MainWindow::showMessage for details
     */
    bool showMessage(const QVariantMap &map);

    void addPositionToHistory(const QUrl &url, KTextEditor::Cursor c);

private:
    void slotUpdateBottomViewBar();
    void slotDocumentCloseAll();
    void slotDocumentCloseOther();
    void slotDocumentCloseOther(KTextEditor::Document *document);
    void slotDocumentCloseSelected(const QList<KTextEditor::Document *> &);

private:
    /**
     * Wrapper of main window for KTextEditor
     */
    KTextEditor::MainWindow *m_wrapper;

    /**
     * Notify about file modifications from other processes?
     */
    bool m_modNotification = false;

    /**
     * Shutdown Kate after last file is closed
     */
    bool m_modCloseAfterLast = false;

    /**
     * keeps track of views
     */
    KateViewManager *m_viewManager = nullptr;

    KRecentFilesAction *m_fileOpenRecent = nullptr;

    KActionMenu *documentOpenWith = nullptr;

    KToggleAction *settingsShowFileselector = nullptr;

    KToggleAction *m_showFullScreenAction = nullptr;

    bool m_modignore = false;

    // all plugin views for this mainwindow, used by the pluginmanager
    QHash<KTextEditor::Plugin *, QObject *> m_pluginViews;

    // options: show statusbar + show path
    KToggleAction *m_paShowPath = nullptr;
    KToggleAction *m_paShowStatusBar = nullptr;
    KToggleAction *m_paShowTabBar = nullptr;
    KToggleAction *m_paShowUrlNavBar = nullptr;

    // makes no sense on macOS
#if KATE_ALLOW_MENU_BAR_HIDE
    KToggleAction *m_paShowMenuBar = nullptr;
#endif

    QWidget *m_bottomViewBarContainer = nullptr;
    KateContainerStackedLayout *m_bottomContainerStack = nullptr;

    class BarState
    {
    public:
        BarState() = default;
        explicit BarState(QWidget *bar)
            : m_bar(bar)
            , m_state(false)
        {
        }
        ~BarState()
        {
        }
        QWidget *bar()
        {
            return m_bar;
        }
        bool state()
        {
            return m_state;
        }
        void setState(bool state)
        {
            m_state = state;
        }
        QWidget *statusBar()
        {
            return m_statusBar;
        }
        void setStatusBar(QWidget *statusBar)
        {
            Q_ASSERT(statusBar);
            Q_ASSERT(!m_statusBar);
            m_statusBar = statusBar;
        }

    private:
        QWidget *m_bar = nullptr;
        bool m_state = false;

        /**
         * status bar widget
         * if already extracted from the bar itself
         * we can't extract this on creation of the bar widget, but do this lazy
         * later, as the construction order in KateView doesn't allow for that.
         */
        QWidget *m_statusBar = nullptr;
    };
    QHash<KTextEditor::View *, BarState> m_bottomViewBarMapping;

    /**
     * generic output tool view
     */
    QWidget *m_toolViewOutput = nullptr;

    /**
     * output widget contained in above tool view
     */
    KateOutputView *m_outputView = nullptr;

    /**
     * Diagnostics view at the bottom
     */
    class DiagnosticsView *m_diagView = nullptr;

public:
    /**
     * The available actions for the mouse back button, used as indexing
     * for the comboboxes of the config window
     */
    enum MouseBackButtonAction {
        PreviousTab = 0,
        HistoryBack
    };

    /**
     * The available actions for the mouse forward button, used as indexing
     * for the comboboxes of the config window
     */
    enum MouseForwardButtonAction {
        NextTab = 0,
        HistoryForward
    };

private:
    MouseBackButtonAction m_mouseButtonBackAction = PreviousTab;
    MouseForwardButtonAction m_mouseButtonForwardAction = NextTab;
    void handleBackButtonAction();
    void handleForwardButtonAction();

public:
    /**
     * Accessor for unique output view per main window.
     * @return our output view, will always exist!
     */
    KateOutputView *outputView()
    {
        return m_outputView;
    }

public:
    static void unsetModifiedOnDiscDialogIfIf(KateMwModOnHdDialog *diag)
    {
        if (s_modOnHdDialog == diag) {
            s_modOnHdDialog = nullptr;
        }
    }

    void slotWindowActivated();

private:
    static KateMwModOnHdDialog *s_modOnHdDialog;
    const MainWindowCommands m_mainWindowCommands;

protected:
    bool event(QEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
};
