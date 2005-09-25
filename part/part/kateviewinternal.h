/* This file is part of the KDE libraries
   Copyright (C) 2002-2005 Hamish Rodda <rodda@kde.org>
   Copyright (C) 2002 John Firebaugh <jfirebaugh@kde.org>
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2002 Christoph Cullmann <cullmann@kde.org>

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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef _KATE_VIEW_INTERNAL_
#define _KATE_VIEW_INTERNAL_

#include "katesupercursor.h"
#include "katelinerange.h"
#include "katetextline.h"
#include "katedocument.h"

#include <QPoint>
#include <QTimer>
#include <QDrag>
#include <QWidget>

class KateView;
class KateIconBorder;
class KateScrollBar;

class QScrollBar;

class KateViewInternal : public QWidget
{
    Q_OBJECT

    friend class KateView;
    friend class KateIconBorder;
    friend class KateScrollBar;
    friend class CalculatingCursor;
    friend class BoundedCursor;
    friend class WrappingCursor;

  public:
    enum Bias
    {
        left  = -1,
        none  =  0,
        right =  1
    };

  public:
    KateViewInternal ( KateView *view, KateDocument *doc );
    ~KateViewInternal ();
    
    // Return the correct micro focus hint
    virtual QVariant inputMethodQuery(Qt::InputMethodQuery query) const;

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

    void clear ();
  //END

  private:
    void updateView (bool changed = false, int viewLinesScrolled = 0);
    void makeVisible (const KTextEditor::Cursor& c, uint endCol, bool force = false, bool center = false, bool calledExternally = false);

  public:
    inline const KTextEditor::Cursor& startPos() const { return m_startPos; }
    inline int startLine () const { return m_startPos.line(); }
    inline int startX () const { return m_startX; }

    KTextEditor::Cursor endPos () const;
    uint endLine () const;

    KateTextLayout yToKateTextLayout(int y) const;

    void prepareForDynWrapChange();
    void dynWrapChanged();

    KateView *view () { return m_view; }

  public slots:
    void slotIncFontSizes();
    void slotDecFontSizes();

  private slots:
    void scrollLines(int line); // connected to the sliderMoved of the m_lineScroll
    void scrollViewLines(int offset);
    void scrollNextPage();
    void scrollPrevPage();
    void scrollPrevLine();
    void scrollNextLine();
    void scrollColumns (int x); // connected to the valueChanged of the m_columnScroll
    void viewSelectionChanged ();

  public:
    void doReturn();
    void doDelete();
    void doBackspace();
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

    inline const KTextEditor::Cursor& getCursor() const { return m_cursor; }
    QPoint cursorCoordinates() const;

  // EVENT HANDLING STUFF - IMPORTANT
  private:
    void fixDropEvent(QDropEvent *event);
  protected:
    virtual void paintEvent(QPaintEvent *e);
    virtual bool eventFilter( QObject *obj, QEvent *e );
    virtual void keyPressEvent( QKeyEvent* );
    virtual void keyReleaseEvent( QKeyEvent* );
    virtual void resizeEvent( QResizeEvent* );
    virtual void mousePressEvent(       QMouseEvent* );
    virtual void mouseDoubleClickEvent( QMouseEvent* );
    virtual void mouseReleaseEvent(     QMouseEvent* );
    virtual void mouseMoveEvent(        QMouseEvent* );
    virtual void dragEnterEvent( QDragEnterEvent* );
    virtual void dragMoveEvent( QDragMoveEvent* );
    virtual void dropEvent( QDropEvent* );
    virtual void showEvent ( QShowEvent *);
    virtual void wheelEvent(QWheelEvent* e);
    virtual void focusInEvent (QFocusEvent *);
    virtual void focusOutEvent (QFocusEvent *);
    virtual void inputMethodEvent(QInputMethodEvent* e);

    void contextMenuEvent ( QContextMenuEvent * e );

  private slots:
    void tripleClickTimeout();

  signals:
    // emitted when KateViewInternal is not handling its own URI drops
    void dropEventPass(QDropEvent*);

  private slots:
    void slotRegionVisibilityChangedAt(unsigned int);
    void slotRegionBeginEndAddedRemoved(unsigned int);
    void slotCodeFoldingChanged();

  private:
    void moveChar( Bias bias, bool sel );
    void moveWord( Bias bias, bool sel );
    void moveEdge( Bias bias, bool sel );
    KTextEditor::Cursor maxStartPos(bool changed = false);
    void scrollPos(KTextEditor::Cursor& c, bool force = false, bool calledExternally = false);
    void scrollLines( int lines, bool sel );

    uint linesDisplayed() const;

    int lineToY(uint viewLine) const;

    void updateSelection( const KTextEditor::Cursor&, bool keepSel );
    void updateCursor( const KTextEditor::Cursor& newCursor, bool force = false, bool center = false, bool calledExternally = false );
    void updateBracketMarks();

    void paintCursor();

    void placeCursor( const QPoint& p, bool keepSelection = false, bool updateSelection = true );
    bool isTargetSelected( const QPoint& p );

    void doDrag();

    inline KateView* view() const { return m_view; }
    KateRenderer* renderer() const;

    KateView *m_view;
    KateDocument* m_doc;
    class KateIconBorder *m_leftBorder;

    int m_mouseX;
    int m_mouseY;
    int m_scrollX;
    int m_scrollY;

    Qt::CursorShape m_mouseCursor;

    KateSmartCursor m_cursor;
    // FIXME probably a bug here, the mouse position shouldn't change just because text gets changed
    KateSmartCursor m_mouse;
    KTextEditor::Cursor m_displayCursor;
    int m_cursorX;

    bool m_possibleTripleClick;

    // Bracket mark and corresponding decorative ranges
    KateSmartRange m_bm, m_bmStart, m_bmEnd;

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
    KateSmartCursor m_startPos;

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
    bool m_columnScrollDisplayed;
    int m_startX;
    int m_oldStartX;

    // has selection changed while your mouse or shift key is pressed
    bool m_selChangedByUser;
    KTextEditor::Cursor m_selectAnchor;

    enum SelectionMode { Default=0, Word, Line }; ///< for drag selection. @since 2.3
    uint m_selectionMode;
    // when drag selecting after double/triple click, keep the initial selected
    // word/line independant of direction.
    // They get set in the event of a double click, and is used with mouse move + leftbutton
    KTextEditor::Range m_selectionCached;

    // Used to determine if the scrollbar will appear/disappear in non-wrapped mode
    bool scrollbarVisible(uint startLine);
    int maxLen(uint startLine);

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
    bool m_preserveMaxX;
    int m_currentMaxX;

    bool m_usePlainLines; // accept non-highlighted lines if this is set

    inline KateTextLine::Ptr textLine( int realLine ) const
    {
      if (m_usePlainLines)
        return m_doc->plainKateTextLine(realLine);
      else
        return m_doc->kateTextLine(realLine);
    }

    bool m_updatingView;
    int m_wrapChangeViewLine;
    KTextEditor::Cursor m_cachedMaxStartPos;

  private slots:
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

    // needed to stop the column scroll bar from hiding / unhiding during a dragScroll.
    bool m_suppressColumnScrollBar;

  private slots:
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

#if 0
  /**
   * IM input stuff
   */
  protected:
    void imStartEvent( QIMEvent *e );
    void imComposeEvent( QIMEvent *e );
    void imEndEvent( QIMEvent *e );
#endif

  private:
    KTextEditor::Range m_imPreedit;
    KTextEditor::Cursor m_imPreeditSelStart;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
