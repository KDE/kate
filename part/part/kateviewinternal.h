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

#include <qscrollview.h>
#include <qpoint.h>

class KateDocument;
class KateView;
class KateIconBorder;

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

  public:
    KateViewInternal( KateView* view, KateDocument* doc );
    ~KateViewInternal() {};
    
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
    
    inline const KateTextCursor& getCursor() { return cursor; }
    QPoint cursorCoordinates();        
    
    inline int yPosition () const { return yPos; }     

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
             
  signals:
    // emitted when KateViewInternal is not handling its own URI drops
    void dropEventPass(QDropEvent*);
  
  private:
    void drawContents( QPainter*, int cx, int cy, int cw, int ch );

    bool eventFilter( QObject*, QEvent* );
    void keyPressEvent( QKeyEvent* );
    void contentsMousePressEvent(       QMouseEvent* );
    void contentsMouseDoubleClickEvent( QMouseEvent* );
    void contentsMouseReleaseEvent(     QMouseEvent* );
    void contentsMouseMoveEvent(        QMouseEvent* );
    void viewportResizeEvent( QResizeEvent* );    
    void timerEvent( QTimerEvent* );
    void contentsDragEnterEvent( QDragEnterEvent* );
    void contentsDropEvent( QDropEvent* );   

  private slots:
    void slotContentsMoving (int x, int y);                
    void tripleClickTimeout();
    void updateView();    
    void updateIconBorder();  
    
    void slotRegionVisibilityChangedAt(unsigned int);
    void slotRegionBeginEndAddedRemoved(unsigned int);
    void slotCodeFoldingChanged();

  private:
    void moveChar( Bias bias, bool sel );
    void moveWord( Bias bias, bool sel );
    void moveEdge( Bias bias, bool sel );
    void scrollLines( int lines, bool sel );
    
    uint linesDisplayed() const;
    uint contentsYToLine( int y ) const;
    int  lineToContentsY( uint line ) const;
    inline uint firstLine() const { return contentsYToLine( yPosition() ); }
    inline uint lastLine()  const { return contentsYToLine( yPosition() + visibleHeight() ); }
    inline uint lastLineCalc()  const { return contentsYToLine( yPosition() + height() ); }

    void centerCursor();

    void updateSelection( const KateTextCursor&, bool keepSel );
    void updateCursor( const KateTextCursor& );
    
    void tagLines(int start, int end);
    void tagRealLines(int start, int end);
    void tagAll();

    void paintCursor();

    void placeCursor( const QPoint& p, bool keepSelection = false );
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
                
    // that yPos is allready set while scrolling before the contentsY() is right
    int yPos;

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
};

#endif
