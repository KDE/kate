/* This file is part of the KDE libraries
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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef _KATE_VIEW_INTERNAL_
#define _KATE_VIEW_INTERNAL_

#include "katecursor.h"
#include "katedocument.h"

#include <qpoint.h>

class QScrollBar;

class KateDocument;
class KateView;
class KateIconBorder;

  enum Bias {
    left  = -1,
    none  =  0,
    right =  1
  };
  
class LineRange
{
  public:
    int line;
    int visibleLine;
    int startCol;
    int endCol;
    int startX;
    int endX;
    bool dirty;
};

class KateViewInternal : public QWidget
{
    Q_OBJECT
    friend class KateDocument;
    friend class KateUndoGroup;
    friend class KateUndo;
    friend class KateView;
    friend class KateIconBorder;

  public:
    KateViewInternal ( KateView *view, KateDocument *doc );
    ~KateViewInternal ();
    
  public:
    inline uint startLine () const { return m_startLine; }
    inline uint startX () const { return m_startX; }
  
    uint endLine () const;

    inline LineRange yToLineRange ( uint y ) const { return lineRanges[y / m_doc->viewFont.fontHeight]; };
      
  private slots:
    void scrollLines (int line); // connected to the valueChanged of the m_lineScroll  
    void scrollColumns (int x); // connected to the valueChanged of the m_columnScroll
  
  public slots:
    void updateView (bool changed = false);
    void makeVisible (uint line, uint startCol, uint endCol);
    
  public:
    void doReturn();
    void doDelete();
    void doBackspace();
    void doPaste();
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
/*    void cursorPageUp(bool sel=false);
    void cursorPageDown(bool sel=false);*/
    void top(bool sel=false);
    void bottom(bool sel=false);
    void top_home(bool sel=false);
    void bottom_end(bool sel=false);

    void clear();
    
    inline const KateTextCursor& getCursor() { return cursor; }
    QPoint cursorCoordinates();
    
    void setTagLinesFrom(int line);

    void editStart();
    void editEnd(int editTagLineStart, int editTagLineEnd);

    void editRemoveText(int line, int col, int len);
    void removeSelectedText(KateTextCursor & start);

    /**
       Set the tagLinesFrom member if usefull. 
    */
    void setViewTagLinesFrom(int line);

    void editWrapLine(int line, int col, int len);
    void editUnWrapLine(int line, int col);
    void editRemoveLine(int line);
    
    void paintText (int x, int y, int width, int height, bool paintOnlyDirty = false);
 
  // EVENT HANDLING STUFF - IMPORTANT
  protected:
    void paintEvent(QPaintEvent *e);
    bool eventFilter( QObject *obj, QEvent *e );
    void keyPressEvent( QKeyEvent* );
    void resizeEvent( QResizeEvent* );
    void mousePressEvent(       QMouseEvent* );
    void mouseDoubleClickEvent( QMouseEvent* );
    void mouseReleaseEvent(     QMouseEvent* );
    void mouseMoveEvent(        QMouseEvent* );
    void timerEvent( QTimerEvent* );
    void dragEnterEvent( QDragEnterEvent* );
    void dragMoveEvent( QDragMoveEvent* );
    void dropEvent( QDropEvent* );   
    void showEvent ( QShowEvent *);
    void wheelEvent(QWheelEvent* e);
    void focusInEvent (QFocusEvent *);
    void focusOutEvent (QFocusEvent *);
    
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
    void scrollLines( int lines, bool sel );
    
    uint linesDisplayed() const;

    inline int lineToY ( uint line ) const { return (line-startLine()) * m_doc->viewFont.fontHeight; }

    void centerCursor();

    void updateSelection( const KateTextCursor&, bool keepSel );
    void updateCursor( const KateTextCursor& );
    
    void tagLines(int start, int end, bool realLines = false );
    void tagRealLines(int start, int end);
    void tagAll();

    void paintCursor();

    void placeCursor( const QPoint& p, bool keepSelection = false, bool updateSelection = true );
    bool isTargetSelected( const QPoint& p );

    void doDrag();

    KateView *m_view;
    KateDocument *m_doc;
    class KateIconBorder *leftBorder;
    
    int mouseX;
    int mouseY;
    int scrollX;
    int scrollY;
    int scrollTimer;

    KateTextCursor cursor;
    KateTextCursor displayCursor;
    bool cursorOn;
    int cursorTimer;
    int cXPos;

    bool possibleTripleClick;

    // for use from doc: tag lines from here (if larger than -1)
    int tagLinesFrom;

    //
    // cursor cache for document
    // here stores the document the view's cursor pos while editing before update
    //
    KateTextCursor cursorCache;
    bool cursorCacheChanged;

    BracketMark bm;
    
    enum DragState { diNone, diPending, diDragging };

    struct _dragInfo {
      DragState    state;
      QPoint       start;
      QTextDrag*   dragObject;
    } dragInfo;

    uint iconBorderHeight;
    
    //
    // line scrollbar + first visible (virtual) line in the current view
    //
    QScrollBar *m_lineScroll;
    QWidget *m_lineScrollWidget;
    int m_startLine;
    int m_oldStartLine;
    
    //
    // column scrollbar + x position
    //
    QScrollBar *m_columnScroll;
    int m_startX;
    int m_oldStartX;
    
    //
    // lines Ranges, mostly useful to speedup + dyn. word wrap
    //
    QMemArray<LineRange> lineRanges;
    
    // holds the current range
    void findCurrentRange();
    // find which line number in the view contains cursor c
    int viewLine(const KateTextCursor& c) const;
    uint m_currentRange;

    // These variable holds the most recent maximum real & visible column number
    bool m_preserveMaxX;
    int m_currentMaxX;

  private slots:
#ifndef QT_NO_DRAGANDDROP
    void doDragScroll();
    void startDragScroll();
    void stopDragScroll();
#endif

  private:
    // Timer for drag & scroll
    QTimer m_dragScrollTimer;

    static const int scrollTime = 30;
    static const int scrollMargin = 16;
    
    // needed to stop the column scroll bar from hiding / unhiding during a dragScroll.
    bool m_suppressColumnScrollBar;
};

#endif
