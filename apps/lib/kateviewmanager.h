/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2001 Christoph Cullmann <cullmann@kde.org>
   SPDX-FileCopyrightText: 2001 Joseph Wenninger <jowenn@kde.org>
   SPDX-FileCopyrightText: 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include "kateprivate_export.h"
#include "katesplitter.h"

#include <QList>
#include <QPointer>
#include <QUrl>

#include <span>
#include <unordered_map>

namespace KTextEditor
{
class View;
class Document;
class Cursor;
}

class KateDocumentInfo;
class DocOrWidget;
class KConfigGroup;
class KConfigBase;
class KateMainWindow;
class KateViewSpace;
class QScrollBar;

class KATE_PRIVATE_EXPORT KateViewManager : public KateSplitter
{
    Q_OBJECT

    friend class KateViewManagementTests;

public:
    KateViewManager(QWidget *parentW, KateMainWindow *parent);
    ~KateViewManager() override;

    /**
     * triggered delayed focus set to current active view
     */
    void triggerActiveViewFocus();

private:
    /**
     * create all actions needed for the view manager
     */
    void setupActions();

    void updateViewSpaceActions();

public:
    /* This will save the splitter configuration */
    void saveViewConfiguration(KConfigGroup &group);

    /* restore it */
    void restoreViewConfiguration(const KConfigGroup &group);

    KTextEditor::Document *
    openUrl(const QUrl &url, const QString &encoding, bool activate = true, bool ignoreForRecentFiles = false, const KateDocumentInfo *docInfo = nullptr);

    KTextEditor::Document *openUrls(std::span<const QUrl> url, const QString &encoding, const KateDocumentInfo *docInfo = nullptr);

    KTextEditor::View *openUrlWithView(const QUrl &url, const QString &encoding);

    KTextEditor::View *openViewForDoc(KTextEditor::Document *doc);

public:
    void openUrl(const QUrl &url);
    void openUrlOrProject(const QUrl &url);
    void openRecent(const QUrl &url);
    void addPositionToHistory(const QUrl &url, KTextEditor::Cursor pos);

public:
    void closeView(KTextEditor::View *view);
    KateMainWindow *mainWindow();

    void moveViewToViewSpace(KateViewSpace *dest, KateViewSpace *src, DocOrWidget doc);

private:
    void activateView(KTextEditor::View *view);
    void activateSpace(KTextEditor::View *v);

public:
    void slotDocumentNew();
    void slotDocumentOpen();
    void slotDocumentClose();
    void slotDocumentClose(KTextEditor::Document *document);
    void slotRestoreLastClosedDocument();
    void slotOpenNextPinnedDocument();

    void setActiveSpace(KateViewSpace *vs);

    void activateNextView();
    void activatePrevView();
    void activateLeftView();
    void activateRightView();
    void activateUpwardView();
    void activateDownwardView();

private:
    void activateIntuitiveNeighborView(Qt::Orientation o, int dir);
    KateViewSpace *findIntuitiveNeighborView(KateSplitter *splitter, QWidget *widget, Qt::Orientation o, int dir);

Q_SIGNALS:
    void viewChanged(KTextEditor::View *);
    void viewCreated(KTextEditor::View *);

    void historyBackEnabled(bool e);
    void historyForwardEnabled(bool e);

    void showUrlNavBarChanged(bool);

public:
    /**
     * create and activate a new view for doc, if doc == 0, then
     * create a new document.
     * Can return NULL.
     */
    KTextEditor::View *createView(KTextEditor::Document *doc = nullptr, KateViewSpace *vs = nullptr);

    bool deleteView(KTextEditor::View *view);

private:
    /* Save the configuration of a single splitter.
     * If child splitters are found, it calls it self with those as the argument.
     * If a viewspace child is found, it is asked to save its filelist.
     */
    QString saveSplitterConfig(KateSplitter *s, KConfigBase *config, const QString &viewConfGrp);

    /** Restore a single splitter.
     * This is all the work is done for @see saveSplitterConfig()
     */
    void restoreSplitter(const KConfigBase *config, const QString &group, KateSplitter *parent, const QString &viewConfGrp);

    void removeViewSpace(KateViewSpace *viewspace);

public:
    KTextEditor::View *activeView();
    KateViewSpace *activeViewSpace();

    /// Returns all non-KTE::View widgets in this viewManager
    QWidgetList widgets() const;
    bool removeWidget(QWidget *w);
    bool activateWidget(QWidget *w);

private:
    void slotViewChanged();

    void documentWillBeDeleted(KTextEditor::Document *doc);

    void documentSavedOrUploaded(KTextEditor::Document *document, bool saveAs);

    /**
     * This signal is emitted before the documents batch is going to be deleted
     *
     * note that the batch can be interrupted in the middle and only some
     * of the documents may be actually deleted. See documentsDeleted() signal.
     *
     * @param documents documents we want to delete, may not be deleted
     */
    void aboutToDeleteDocuments();

    /**
     * This signal is emitted after the documents batch was deleted
     *
     * This is the batch closing signal for aboutToDeleteDocuments
     * @param documents the documents that weren't deleted after all
     */
    void documentsDeleted();

    /**
     * Read and apply the config for this view manager.
     */
    void readConfig();

    /**
     * Scroll all synched views according to the delta scroll of the given view
     * @param view  Other synchronised views will be scrolled according to this
     */
    void slotScrollSynchedViews(KTextEditor::View *view);

public:
    /**
     * Splits a KateViewSpace into two in the following steps:
     * 1. create a KateSplitter in the parent of the KateViewSpace to be split
     * 2. move the to-be-split KateViewSpace to the new splitter
     * 3. create new KateViewSpace and added to the new splitter
     * 4. create KateView to populate the new viewspace.
     * 5. The new KateView is made the active one, because createView() does that.
     * If no viewspace is provided, the result of activeViewSpace() is used.
     * The orientation of the new splitter is determined by the value of o.
     * Note: horizontal splitter means vertically aligned views.
     */
    void splitViewSpace(KateViewSpace *vs = nullptr, Qt::Orientation o = Qt::Horizontal, bool moveDocument = false);

    /**
     * Set the synchronisation of scrolling of multiple views according to user input
     */
    void slotSynchroniseScrolling(bool checked, KateViewSpace *correspondingViewSpace);

    /**
     * @brief clearSyncOfDeletedView
     * @param obj KTextEditor::View* that got deleted
     */
    void clearSyncOfDeletedView(QObject *view);

    /**
     * Remove the given KateViewSpace pointer from scroll synced viewspaces
     */
    void removeSyncedViewSpace(QObject *obj);

    /**
     * Will tell you whether or not all Scroll Sync indicators are to be displayed
     */
    bool hasScrollSync() const;

    /**
     * Will iterate over all viewspaces and show/hide the Scroll Sync indicators
     */
    void updateScrollSyncIndicatorVisibility();

    /**
     * Sets the currentView as the scroll-synched view
     * and removes synchronisation for the previous view in the same viewSpace
     * @param synched scroll for the activeViewSpace() will be set to scrollbar found in this view
     */
    void slotChangeSynchedView(KTextEditor::View *currentView);

    /**
     * Close the view space that contains the given view. If no view was
     * given, then the active view space will be closed instead.
     */
    void closeViewSpace(KTextEditor::View *view = nullptr);

    /**
     * @returns true of the two given views share the same view space.
     */
    bool viewsInSameViewSpace(KTextEditor::View *view1, KTextEditor::View *view2);

    /**
     * activate view for given document
     * @param doc document to activate view for
     * @param vs the target viewspace in which the doc should be activated. If it is
     * null, activeViewSpace will be used instead
     */
    KTextEditor::View *activateView(DocOrWidget docOrWidget, KateViewSpace *vs = nullptr);

    /** Splits the active viewspace horizontally */
    void slotSplitViewSpaceHoriz()
    {
        splitViewSpace(nullptr, Qt::Vertical);
    }

    /** Splits the active viewspace vertically */
    void slotSplitViewSpaceVert()
    {
        splitViewSpace();
    }

    /** Splits the active viewspace horizontally and moves the active document to the viewspace below */
    void slotSplitViewSpaceHorizMoveDoc()
    {
        splitViewSpace(nullptr, Qt::Vertical, true);
    }

    /** Splits the active viewspace vertically and moves the active document to the right viewspace */
    void slotSplitViewSpaceVertMoveDoc()
    {
        splitViewSpace(nullptr, Qt::Horizontal, true);
    }

    /**  moves the splitter according to the key that has been pressed */
    void moveSplitter(Qt::Key key, int repeats = 1);

    /** moves the splitter to the right  */
    void moveSplitterRight()
    {
        moveSplitter(Qt::Key_Right);
    }

    /** moves the splitter to the left  */
    void moveSplitterLeft()
    {
        moveSplitter(Qt::Key_Left);
    }

    /** moves the splitter up  */
    void moveSplitterUp()
    {
        moveSplitter(Qt::Key_Up);
    }

    /** moves the splitter down  */
    void moveSplitterDown()
    {
        moveSplitter(Qt::Key_Down);
    }

    /** closes the current view space. */
    void slotCloseCurrentViewSpace()
    {
        closeViewSpace();
    }

    /** closes every view but the active one */
    void slotCloseOtherViews();

    /** hide every view but the active one */
    void slotHideOtherViews(bool hideOthers);

    void replugActiveView();

    /**
     * Toggle the orientation of current split view
     */
    void toggleSplitterOrientation();

    /**
     * Get a list of all views.
     * @return all views sorted by their last use, most recently used first
     */
    QList<KTextEditor::View *> views() const;

    void onViewSpaceEmptied(KateViewSpace *vs);

    // Returns the number of viewspaces this document is registered in
    int viewspaceCountForDoc(KTextEditor::Document *doc) const;
    // returns true if @p doc exists only in one viewspace
    bool docOnlyInOneViewspace(KTextEditor::Document *doc) const;
    // returns true if any viewspace in this viewmanager has its tabbar visible
    bool tabsVisible() const;

    void setShowUrlNavBar(bool show);
    bool showUrlNavBar() const;

    void hideWelcomeView(KateViewSpace *vs);
    void showWelcomeViewOrNewDocumentIfNeeded();
    void showWelcomeView();

private:
    KateMainWindow *m_mainWindow;

    QAction *m_splitViewVert = nullptr;
    QAction *m_splitViewHoriz = nullptr;
    QAction *m_splitViewVertMove = nullptr;
    QAction *m_splitViewHorizMove = nullptr;
    QAction *m_closeView = nullptr;
    QAction *m_closeOtherViews = nullptr;
    QAction *m_toggleSplitterOrientation = nullptr;
    QAction *m_hideOtherViews = nullptr;
    QAction *goNext = nullptr;
    QAction *goPrev = nullptr;
    QAction *goLeft = nullptr;
    QAction *goRight = nullptr;
    QAction *goUp = nullptr;
    QAction *goDown = nullptr;

    std::vector<KateViewSpace *> m_viewSpaceList;

    bool m_blockViewCreationAndActivation;

    bool m_showUrlNavBar = false;

    int m_splitterIndex = 0; // used during saving splitter config.

    /**
     * View meta data
     */
    class ViewData
    {
    public:
        /**
         * lru age of the view
         * important: smallest age ==> latest used view
         */
        qint64 lruAge = 0;
    };

    /**
     * central storage of all views known in the view manager
     * maps the view to meta data
     */
    std::unordered_map<KTextEditor::View *, ViewData> m_views;

    struct ScrollSynchronisation {
        /**
         * stores a reference scroll value according to which,
         * all others will be set
         */
        int referenceScrollValue = 0;
        /**
         * Keeps track of all viewspace having a scroll-synched view
         * to help with finding the required view when user changes the tab
         */
        QHash<KateViewSpace *, KTextEditor::View *> synchedViewSpaces;

        /**
         * @struct ScrollBarInfo
         * @param scrollBar stores the pointer to the vertical scroll bar of the view
         * @param initScrollValue stores the offset position of the scroll bar to the reference scroll value using which the sync will occur
         */
        struct ScrollBarInfo {
            QScrollBar *scrollBar = nullptr;
            int initScrollValue;
        };

        /**
         * stores the information required for synchronisation of scrolling between multiple split views
         * Key - KTextEditor::View* stores the pointer to the corresp view
         * Value - ScrollBarInfo stores relevant information regarding the scrollbar of the synched view
         */
        QHash<KTextEditor::View *, ScrollBarInfo> viewScrollInfo;

        ScrollBarInfo getViewScrollBarInfo(KTextEditor::View *view) const;
    } m_scrollSynchronisation;

    /**
     * current minimal age
     */
    qint64 m_minAge;

    /**
     * the view that is ATM merged to the xml gui factory
     */
    QPointer<KTextEditor::View> m_guiMergedView;

    /**
     * last url of open file dialog, used if current document has no valid url
     */
    QUrl m_lastOpenDialogUrl;

    /**
     * SDI mode: open every new document in a new window in most cases
     */
    bool m_sdiMode = false;

    /**
     * was the welcome view already shown?
     * ensures it doesn't auto popup multiple times
     */
    bool m_welcomeViewAlreadyShown = false;
};
