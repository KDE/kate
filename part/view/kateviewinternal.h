/* This file is part of the KDE libraries
   Copyright (C) 2002-2007 Hamish Rodda <rodda@kde.org>
   Copyright (C) 2002 John Firebaugh <jfirebaugh@kde.org>
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2002 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2007 Mirko Stocker <me@misto.ch>

   Based on:
     KWriteView : Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>

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

#ifndef _KATE_VIEW_INTERNAL_
#define _KATE_VIEW_INTERNAL_

#include <ktexteditor/attribute.h>
#include <ktexteditor/smartrangewatcher.h>

#include "katetextcursor.h"
#include "katelinelayout.h"
#include "katetextline.h"
#include "katedocument.h"
#include "kateview.h"

#include <QtCore/QPoint>
#include <QtCore/QTimer>
#include <QtGui/QDrag>
#include <QtGui/QWidget>
#include <QtCore/QSet>
#include <QtCore/QPointer>

namespace KTextEditor {
  class MovingRange;
}

class KateIconBorder;
class KateScrollBar;
class KateSmartRange;
class KateTextLayout;

class KateViInputModeManager;

class QScrollBar;

class KateViewInternal : public QWidget, private KTextEditor::SmartRangeWatcher
{
    Q_OBJECT

    friend class KateView;
    friend class KateIconBorder;
    friend class KateScrollBar;
    friend class CalculatingCursor;
    friend class BoundedCursor;
    friend class WrappingCursor;
    friend class KateViModeBase;

  public:
    enum Bias
    {
        left  = -1,
        none  =  0,
        right =  1
    };

  public:
    KateViewInternal ( KateView *view );
    ~KateViewInternal ();

  //BEGIN EDIT STUFF
  public:
    void editStart ();
    void editEnd (int editTagLineStart, int editTagLineEnd, bool tagFrom);

    void editSetCursor (const KTextEditor::Cursor &cursor);

  private:
    uint editSessionNumber;
    bool editIsRunning;
    KTextEditor::Cursor editOldCursor;
  //END

  //BEGIN TAG & CLEAR & UPDATE STUFF
  public:
    bool tagLine (const KTextEditor::Cursor& virtualCursor);

    bool tagLines (int start, int end, bool realLines = false);
    // cursors not const references as they are manipulated within
    bool tagLines (KTextEditor::Cursor start, KTextEditor::Cursor end, bool realCursors = false);

    bool tagRange(const KTextEditor::Range& range, bool realCursors);

    void tagAll ();

    void relayoutRange(const KTextEditor::Range& range, bool realCursors = true);

    void updateDirty();

    void clear ();

  Q_SIGNALS:
    // Trigger this signal whenever you want to call updateView() and may not be in the same thread.
    // Make sure to set m_smartDirty = false before, else nothing will happen
    void requestViewUpdateIfSmartDirty();
  //END

  private Q_SLOTS:
    // Updates the view and requests a redraw.
    void updateView (bool changed = false, int viewLinesScrolled = 0);
    // This is used to prevent multiple unneeded view updates
    void updateViewIfSmartDirty();

  private:
    // Actually performs the updating, but doesn't call update().
    void doUpdateView(bool changed = false, int viewLinesScrolled = 0);
    void makeVisible (const KTextEditor::Cursor& c, int endCol, bool force = false, bool center = false, bool calledExternally = false);

  public:
    // Start Position is a virtual cursor
    KTextEditor::Cursor startPos() const { return m_startPos; }
    int startLine () const { return m_startPos.line(); }
    int startX () const { return m_startX; }

    KTextEditor::Cursor endPos () const;
    int endLine () const;

    KateTextLayout yToKateTextLayout(int y) const;

    void prepareForDynWrapChange();
    void dynWrapChanged();

  public Q_SLOTS:
    void slotIncFontSizes();
    void slotDecFontSizes();

  private Q_SLOTS:
    void scrollLines(int line); // connected to the sliderMoved of the m_lineScroll
    void scrollViewLines(int offset);
    void scrollAction(int action);
    void scrollNextPage();
    void scrollPrevPage();
    void scrollPrevLine();
    void scrollNextLine();
    void scrollColumns (int x); // connected to the valueChanged of the m_columnScroll
    void viewSelectionChanged ();

  public:
    void doReturn();
    void doSmartNewline();
    void doDelete();
    void doBackspace();
    void doTabulator();
    void doTranspose();
    void doDeleteWordLeft();
    void doDeleteWordRight();

    void cursorLeft(bool sel=false);
    void cursorRight(bool sel=false);
    void wordLeft(bool sel=false);
    void wordRight(bool sel=false);
    void home(bool sel=false);
    void end(bool sel=false);
    void cursorUp(bool sel=false);
    void cursorDown(bool sel=false);
    void cursorToMatchingBracket(bool sel=false);
    void scrollUp();
    void scrollDown();
    void topOfView(bool sel=false);
    void bottomOfView(bool sel=false);
    void pageUp(bool sel=false);
    void pageDown(bool sel=false);
    void top(bool sel=false);
    void bottom(bool sel=false);
    void top_home(bool sel=false);
    void bottom_end(bool sel=false);

    KTextEditor::Cursor getCursor() const { return m_cursor; }
    KTextEditor::Cursor getMouse() const { return m_mouse; }

    QPoint cursorToCoordinate(const KTextEditor::Cursor& cursor, bool realCursor = true, bool includeBorder = true) const;
    //Always works on coordinates of the whole widget, eg. offsetted by the border
    KTextEditor::Cursor coordinatesToCursor(const QPoint& coord) const;
    QPoint cursorCoordinates(bool includeBorder = true) const;
    KTextEditor::Cursor findMatchingBracket();

  // EVENT HANDLING STUFF - IMPORTANT
  private:
    void fixDropEvent(QDropEvent *event);
  protected:
    virtual void hideEvent(QHideEvent* e);
    virtual void paintEvent(QPaintEvent *e);
    virtual bool eventFilter( QObject *obj, QEvent *e );
    virtual void keyPressEvent( QKeyEvent* );
    virtual void keyReleaseEvent( QKeyEvent* );
    virtual void resizeEvent( QResizeEvent* );
    virtual void mousePressEvent(       QMouseEvent* );
    virtual void mouseDoubleClickEvent( QMouseEvent* );
    virtual void mouseReleaseEvent(     QMouseEvent* );
    virtual void mouseMoveEvent(        QMouseEvent* );
    virtual void leaveEvent( QEvent* );
    virtual void dragEnterEvent( QDragEnterEvent* );
    virtual void dragMoveEvent( QDragMoveEvent* );
    virtual void dropEvent( QDropEvent* );
    virtual void showEvent ( QShowEvent *);
    virtual void wheelEvent(QWheelEvent* e);
    virtual void focusInEvent (QFocusEvent *);
    virtual void focusOutEvent (QFocusEvent *);
    virtual void inputMethodEvent(QInputMethodEvent* e);

    void contextMenuEvent ( QContextMenuEvent * e );

  private Q_SLOTS:
    void tripleClickTimeout();

  Q_SIGNALS:
    // emitted when KateViewInternal is not handling its own URI drops
    void dropEventPass(QDropEvent*);

  private Q_SLOTS:
    void slotRegionVisibilityChangedAt(unsigned int,bool clear_cache);
    void slotRegionBeginEndAddedRemoved(unsigned int);
    void slotCodeFoldingChanged();

  private:
    void moveChar( Bias bias, bool sel );
    void moveEdge( Bias bias, bool sel );
    KTextEditor::Cursor maxStartPos(bool changed = false);
    void scrollPos(KTextEditor::Cursor& c, bool force = false, bool calledExternally = false);
    void scrollLines( int lines, bool sel );

    int linesDisplayed() const;

    int lineToY(int viewLine) const;

    void updateSelection( const KTextEditor::Cursor&, bool keepSel );
    void setSelection( const KTextEditor::Range& );
    void moveCursorToSelectionEdge();
    //The smart-lock should not be locked when this is called
    void updateCursor( const KTextEditor::Cursor& newCursor, bool force = false, bool center = false, bool calledExternally = false );
    void updateBracketMarks();

    void paintCursor();

    void placeCursor( const QPoint& p, bool keepSelection = false, bool updateSelection = true );
    bool isTargetSelected( const QPoint& p );
    //Returns whether the given range affects the area currently visible in the view
    bool rangeAffectsView(const KTextEditor::Range& range, bool realCursors) const;

    void doDrag();

    KateRenderer* renderer() const;

    KateView *m_view;
    class KateIconBorder *m_leftBorder;

    int m_mouseX;
    int m_mouseY;
    int m_scrollX;
    int m_scrollY;

    Qt::CursorShape m_mouseCursor;

    Kate::TextCursor m_cursor;
    KTextEditor::Cursor m_mouse;
    KTextEditor::Cursor m_displayCursor;

    bool m_possibleTripleClick;

    //Whether the current completion-item was expanded while the last press of ALT
    bool m_completionItemExpanded;
    QTime m_altDownTime;

    // Bracket mark and corresponding decorative ranges
    KTextEditor::MovingRange *m_bm, *m_bmStart, *m_bmEnd;
    void updateBracketMarkAttributes();

    enum DragState { diNone, diPending, diDragging };

    struct _dragInfo {
      DragState    state;
      QPoint       start;
      QDrag*   dragObject;
    } m_dragInfo;

    uint m_iconBorderHeight;

    //
    // line scrollbar + first visible (virtual) line in the current view
    //
    KateScrollBar *m_lineScroll;
    QWidget* m_dummy;

    // These are now cursors to account for word-wrap.
    // Start Position is a virtual cursor
    Kate::TextCursor m_startPos;
    //Count of lines that are visible behind m_startPos.
    //This does not respect dynamic word wrap, so take it as an approximation.
    uint m_visibleLineCount;

    // This is set to false on resize or scroll (other than that called by makeVisible),
    // so that makeVisible is again called when a key is pressed and the cursor is in the same spot
    bool m_madeVisible;
    bool m_shiftKeyPressed;

    // How many lines to should be kept visible above/below the cursor when possible
    void setAutoCenterLines(int viewLines, bool updateView = true);
    int m_autoCenterLines;
    int m_minLinesVisible;

    //
    // column scrollbar + x position
    //
    QScrollBar *m_columnScroll;
    int m_startX;

    // has selection changed while your mouse or shift key is pressed
    bool m_selChangedByUser;
    KTextEditor::Cursor m_selectAnchor;

    enum SelectionMode { Default=0, Mouse, Word, Line }; ///< for drag selection.
    uint m_selectionMode;
    // when drag selecting after double/triple click, keep the initial selected
    // word/line independent of direction.
    // They get set in the event of a double click, and is used with mouse move + leftbutton
    KTextEditor::Range m_selectionCached;

    // maximal length of textlines visible from given startLine
    int maxLen(int startLine);

    // are we allowed to scroll columns?
    bool columnScrollingPossible ();

    // returns the maximum X value / col value a cursor can take for a specific line range
    int lineMaxCursorX(const KateTextLayout& line);
    int lineMaxCol(const KateTextLayout& line);

    class KateLayoutCache* cache() const;
    KateLayoutCache* m_layoutCache;

    // convenience methods
    KateTextLayout currentLayout() const;
    KateTextLayout previousLayout() const;
    KateTextLayout nextLayout() const;

    // find the cursor offset by (offset) view lines from a cursor.
    // when keepX is true, the column position will be calculated based on the x
    // position of the specified cursor.
    KTextEditor::Cursor viewLineOffset(const KTextEditor::Cursor& virtualCursor, int offset, bool keepX = false);

    KTextEditor::Cursor toRealCursor(const KTextEditor::Cursor& virtualCursor) const;
    KTextEditor::Cursor toVirtualCursor(const KTextEditor::Cursor& realCursor) const;

    // These variable holds the most recent maximum real & visible column number
    bool m_preserveX;
    int m_preservedX;

    bool m_updatingView;
    int m_wrapChangeViewLine;
    KTextEditor::Cursor m_cachedMaxStartPos;

  private Q_SLOTS:
    void doDragScroll();
    void startDragScroll();
    void stopDragScroll();

  private:
    // Timers
    QTimer m_dragScrollTimer;
    QTimer m_scrollTimer;
    QTimer m_cursorTimer;
    QTimer m_textHintTimer;

    static const int s_scrollTime = 30;
    static const int s_scrollMargin = 16;

  private Q_SLOTS:
    void scrollTimeout ();
    void cursorTimeout ();
    void textHintTimeout ();

  //TextHint
 public:
   void enableTextHints(int timeout);
   void disableTextHints();

 private:
   bool m_textHintEnabled;
   int m_textHintTimeout;
   int m_textHintMouseX;
   int m_textHintMouseY;

  /**
   * IM input stuff
   */
  public:
    virtual QVariant inputMethodQuery(Qt::InputMethodQuery query) const;

  private:
    KTextEditor::MovingRange *m_imPreeditRange;
    QList<KTextEditor::MovingRange *> m_imPreeditRangeChildren;

  // Arbitrary highlighting
  public:
    void addHighlightRange(KTextEditor::SmartRange* range);
    void removeHighlightRange(KTextEditor::SmartRange* range);

  private:
    void mouseMoved();
    void cursorMoved();

  private:
    // Overrides for watched highlighting ranges
    void rangePositionChanged(KTextEditor::SmartRange* range);
    void rangeDeleted(KTextEditor::SmartRange* range);
    void childRangeInserted(KTextEditor::SmartRange* range, KTextEditor::SmartRange* child);
    void childRangeRemoved(KTextEditor::SmartRange* range, KTextEditor::SmartRange* child);
    void rangeAttributeChanged(KTextEditor::SmartRange* range, KTextEditor::Attribute::Ptr currentAttribute, KTextEditor::Attribute::Ptr previousAttribute);

  private:
    bool m_smartDirty;

    void removeWatcher(KTextEditor::SmartRange* range, KTextEditor::SmartRangeWatcher* watcher);
    void addWatcher(KTextEditor::SmartRange* range, KTextEditor::SmartRangeWatcher* watcher);

  private:
    inline KateDocument *doc() { return m_view->doc(); }
    inline KateDocument *doc() const { return m_view->doc(); }
    inline KateSmartManager *smartManager() { return doc()->smartManager(); }

  // vi Mode
  private:
    bool m_viInputMode;
    bool m_viInputModeStealKeys;

    /**
     * returns the current vi mode
     */
    ViMode getCurrentViMode();

    /**
     * an instance of KateViInputModeManager. used for interacting with the vi input mode when
     * enabled
     */
    KateViInputModeManager* m_viInputModeManager;

    /**
     * @return a pointer to a KateViInputModeManager
     */
    KateViInputModeManager* getViInputModeManager();
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
