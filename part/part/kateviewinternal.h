/* This file is part of the KDE libraries
   Copyright (C) 2002 John Firebaugh <jfirebaugh@kde.org>
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2002 Christoph Cullmann <cullmann@kde.org>

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

#include "kateglobal.h"

#include <qscrollview.h>
#include <qpoint.h>

class KateDocument;
class KateView;
class KateIconBorder;

class KateLineRange
{
  public:
    // need this line a update / is it new/changed in the lineranges
    bool dirty;
    
    // is the line empty ? (no textline available)
    bool empty;
    
    // length of the line in pixel, updated by updateView
    uint lengthPixel;
    
    // REAL line number
    uint line;
    
    // start/end column
    int startCol;
    int endCol;
    
    // is this line wrapped ?
    bool wrapped;

};


  enum Bias {
    left  = -1,
    none  =  0,
    right =  1
  };

class KateViewInternal : public QScrollView
{
    Q_OBJECT
    friend class KateDocument;
    friend class KateUndoGroup;
    friend class KateUndo;
    friend class KateView;
    friend class KateIconBorder;
    friend class CodeCompletion_Impl;

  public:
    KateViewInternal( KateView* view, KateDocument* doc );
    ~KateViewInternal();
    
    // update flags
    enum updateFlags
    {
     ufRepaint,
     ufDocGeometry,
     ufFoldingChanged
     };

    void doReturn();
    void doDelete();
    void doBackspace();
    void doPaste();
    void doTranspose();
    
    void cursorLeft(bool sel=false);
    void cursorRight(bool sel=false);
    void wordLeft(bool sel=false);
    void wordRight(bool sel=false);
    void home(bool sel=false);
    void end(bool sel=false);
    void cursorUp(bool sel=false);
    void cursorDown(bool sel=false);
    void scrollUp();
    void scrollDown();
    void topOfView(bool sel=false);
    void bottomOfView(bool sel=false);
    void pageUp(bool sel=false);
    void pageDown(bool sel=false);
    void cursorPageUp(bool sel=false);
    void cursorPageDown(bool sel=false);
    void top(bool sel=false);
    void bottom(bool sel=false);
    void top_home(bool sel=false);
    void bottom_end(bool sel=false);

    void clear();
    const KateTextCursor& getCursor()  { return cursor; }
    QPoint cursorCoordinates();

  signals:
    // emitted when KateViewInternal is not handling its own URI drops
    void dropEventPass(QDropEvent*);
    
  private slots:
    void slotContentsMoving (int x, int y);                
    void tripleClickTimeout();

  private:
    void drawContents( QPainter *paint, int cx, int cy, int cw, int ch );
    void drawContents( QPainter *paint, int cx, int cy, int cw, int ch, bool repaint );
  
    void moveChar( Bias bias, bool sel );
    void moveWord( Bias bias, bool sel );
    void moveEdge( Bias bias, bool sel );
    void scrollLines( int lines, bool sel );
    
    uint linesDisplayed() const;
    
    void exposeCursor();

    void updateSelection( const KateTextCursor&, bool keepSel );
    void updateCursor( const KateTextCursor&, int updateViewFlags = 0 );
    
    void updateLineRanges();
    void tagLines(int start, int end);
    void tagRealLines(int start, int end);
    void tagAll();
    void center();

    void updateView(int flags = 0);

    void paintTextLines( int xPos );
    void paintCursor();
    void paintBracketMark();

    void placeCursor( int x, int y, bool keepSelection = false );
    bool isTargetSelected( int x, int y );

    void doDrag();

    void focusInEvent(QFocusEvent *);
    void focusOutEvent(QFocusEvent *);
    void keyPressEvent(QKeyEvent *e);
    void mousePressEvent(QMouseEvent *);
    void mouseDoubleClickEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void wheelEvent( QWheelEvent *e );
    void resizeEvent(QResizeEvent *);
    void timerEvent(QTimerEvent *);

    void dragEnterEvent( QDragEnterEvent * );
    void dropEvent( QDropEvent * );

    KateView *myView;
    KateDocument *myDoc;
    class KateIconBorder *leftBorder;
    
    int xPos;

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
    int updateState;

    // start line virtual / real
    uint startLine;
    uint startLineReal;

    // end line virtual / real
    uint endLine;
    uint endLineReal;

    // new start line, will be used by updateLineRanges
    uint newStartLine;
    uint newStartLineReal;

    // for use from doc: tag lines from here (if larger than -1)
    int tagLinesFrom;

    // array with the line data
    QMemArray<KateLineRange> lineRanges;
    
    //
    // cursor cache for document
    // here stores the document the view's cursor pos while editing before update
    //
    KateTextCursor cursorCache;
    bool cursorCacheChanged;

    BracketMark bm;

    enum DragState { diNone, diPending, diDragging };

    struct _dragInfo {
      DragState       state;
      KateTextCursor      start;
      QTextDrag       *dragObject;
    } dragInfo;

    //uint iconBorderWidth;
    uint iconBorderHeight;

};

#endif
