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

// $Id$

#include "kateviewinternal.h"
#include "kateview.h"
#include "kateviewinternal.moc"
#include "katedocument.h"   
#include "katecodefoldinghelpers.h"
#include "kateiconborder.h"
#include "katehighlight.h"

#include <kcursor.h>
#include <kapplication.h>
#include <kglobalsettings.h>

#include <qstyle.h>
#include <qdragobject.h>
#include <qdropsite.h>
#include <qtimer.h>
#include <qpainter.h>
#include <qclipboard.h>

KateViewInternal::KateViewInternal(KateView *view, KateDocument *doc)
  : QScrollView(view, "", Qt::WStaticContents | Qt::WRepaintNoErase | Qt::WResizeNoErase )    
  , m_view (view)
  , m_doc (doc)
{
  // this will prevent the yScrollbar from jumping around on appear of the xScrollbar 
  /*setCornerWidget( new QWidget( this ) );
  cornerWidget()->hide();
  cornerWidget()->setFixedSize( style().scrollBarExtent().width(),
                                style().scrollBarExtent().width() ); */
                                
  setVScrollBarMode (QScrollView::AlwaysOn);
  setHScrollBarMode (QScrollView::AlwaysOn);
                                
  // iconborder ;)
  leftBorder = new KateIconBorder( this );
  connect( leftBorder, SIGNAL(sizeHintChanged()),
           this, SLOT(updateIconBorder()) );
  updateIconBorder();                            
  
  connect( leftBorder, SIGNAL(toggleRegionVisibility(unsigned int)),
           m_doc->foldingTree(), SLOT(toggleRegionVisibility(unsigned int)));
           
  connect( doc->foldingTree(), SIGNAL(regionVisibilityChangedAt(unsigned int)),
           this, SLOT(slotRegionVisibilityChangedAt(unsigned int)));
//  connect( doc->foldingTree(), SIGNAL(regionBeginEndAddedRemoved(unsigned int)),
//           this, SLOT(slotRegionBeginEndAddedRemoved(unsigned int)) );
  connect( doc, SIGNAL(codeFoldingUpdated()),
           this, SLOT(slotCodeFoldingChanged()) );
  
  displayCursor.line=0;
  displayCursor.col=0;

  scrollTimer = 0;

  cursor.col = 0;
  cursor.line = 0;
  cursorOn = true;
  cursorTimer = 0;
  cXPos = 0;         
  yPos = 0;

  possibleTripleClick = false;
  
  setFocusProxy( viewport() );
  viewport()->setAcceptDrops( true );
  viewport()->setBackgroundMode( NoBackground );
  setDragAutoScroll( true );
  setFrameStyle( NoFrame );
  
  viewport()->setCursor( KCursor::ibeamCursor() );
  // QScrollView installs a custom event filter on the
  // viewport, so we must override it and pass events to
  // KCursor::autoHideEventFilter().
  KCursor::setAutoHideCursor( viewport(), true, true );

  dragInfo.state = diNone;
    
  verticalScrollBar()->setLineStep( m_doc->viewFont.fontHeight );

  connect( this, SIGNAL( contentsMoving(int, int) ),
           this, SLOT( slotContentsMoving(int, int) ) );
}

void KateViewInternal::updateIconBorder()
{
  int width = leftBorder->sizeHint().width();
  setMargins( width, 0, 0, 0 );
  leftBorder->resize( width, visibleHeight() );
  leftBorder->update();
}       

void KateViewInternal::slotRegionVisibilityChangedAt(unsigned int)
{
  kdDebug(13030) << "slotRegionVisibilityChangedAt()" << endl;
  updateView();
  updateContents();
  leftBorder->update();
}

void KateViewInternal::slotCodeFoldingChanged()
{
  leftBorder->update();
}

void KateViewInternal::slotRegionBeginEndAddedRemoved(unsigned int)
{
  kdDebug(13030) << "slotRegionBeginEndAddedRemoved()" << endl;
//  m_viewInternal->repaint();   
  // FIXME: performance problem
//  if (m_doc->getVirtualLine(line)<=m_viewInternal->endLine)
  leftBorder->update();
}

uint KateViewInternal::contentsYToLine( int y ) const
{
  return y / m_doc->viewFont.fontHeight;
}

int KateViewInternal::lineToContentsY( uint line ) const
{
  return line * m_doc->viewFont.fontHeight;
}

uint KateViewInternal::linesDisplayed() const
{
  int h = height();
  int fh = m_doc->viewFont.fontHeight;
  return (h % fh) == 0 ? h / fh : h / fh + 1;
}

QPoint KateViewInternal::cursorCoordinates()
{
   return contentsToViewport( QPoint( cXPos, lineToContentsY( displayCursor.line ) ) );
}

void KateViewInternal::doReturn()
{
  KateTextCursor c = cursor;
  m_doc->newLine( c );
  updateCursor( c );
  updateView();
}

void KateViewInternal::doDelete()
{
  m_doc->del( cursor );
}

void KateViewInternal::doBackspace()
{
  m_doc->backspace( cursor );
}

void KateViewInternal::doPaste()
{
  m_doc->paste( cursor, m_view );
}

void KateViewInternal::doTranspose()
{
  m_doc->transpose( cursor );
  if (cursor.col + 2 <  m_doc->lineLength(cursor.line))
    cursorRight();
  cursorRight();
}

void KateViewInternal::doDeleteWordLeft()
{
  wordLeft( true );
  m_doc->removeSelectedText();
  update();
}
    
void KateViewInternal::doDeleteWordRight()
{
  wordRight( true );
  m_doc->removeSelectedText();
  update();
}

class CalculatingCursor : public KateTextCursor {
public:
  CalculatingCursor( const Kate::Document& doc )
    : KateTextCursor(), m_doc( doc )
    { Q_ASSERT( valid() ); }
  CalculatingCursor( const Kate::Document& doc, const KateTextCursor& c )          
    : KateTextCursor( c ), m_doc( doc )
    { Q_ASSERT( valid() ); }
  // This one constrains its arguments to valid positions
  CalculatingCursor( const Kate::Document& doc, uint line, uint col )
    : KateTextCursor( line, col ), m_doc( doc )
    { makeValid(); }
  virtual CalculatingCursor& operator+=( int n ) = 0;
  virtual CalculatingCursor& operator-=( int n ) = 0;
  CalculatingCursor& operator++() { return operator+=( 1 ); }
  CalculatingCursor& operator--() { return operator-=( 1 ); }
  void makeValid() {
    line = QMAX( 0, QMIN( int( m_doc.numLines() - 1 ), line ) );
    col  = QMAX( 0, QMIN( m_doc.lineLength( line ), col ) );
    Q_ASSERT( valid() );
  }
  void toEdge( Bias bias ) {
    if( bias == left ) col = 0;
    else if( bias == right ) col = m_doc.lineLength( line );
  }
  bool atEdge() const { return atEdge( left ) || atEdge( right ); }
  bool atEdge( Bias bias ) const {
    switch( bias ) {
    case left:  return col == 0;
    case none:  return atEdge();
    case right: return col == m_doc.lineLength( line );
    default: Q_ASSERT(false); return false;
    }
  }
protected:
  bool valid() const { return line >= 0 && uint( line ) < m_doc.numLines() &&
                              col  >= 0 && col <= m_doc.lineLength( line ); }
  const Kate::Document& m_doc;
};

class BoundedCursor : public CalculatingCursor {
public:
  BoundedCursor( const Kate::Document& doc )
    : CalculatingCursor( doc ) {};
  BoundedCursor( const Kate::Document& doc, const KateTextCursor& c )          
    : CalculatingCursor( doc, c ) {};
  BoundedCursor( const Kate::Document& doc, uint line, uint col )
    : CalculatingCursor( doc, line, col ) {};
  virtual CalculatingCursor& operator+=( int n ) {
    col += n;
    col = QMAX( 0, col );
    col = QMIN( col, m_doc.lineLength( line ) );
    Q_ASSERT( valid() );
    return *this;
  }
  virtual CalculatingCursor& operator-=( int n ) {
    return operator+=( -n );
  }
};

class WrappingCursor : public CalculatingCursor {
public:
  WrappingCursor( const Kate::Document& doc )
    : CalculatingCursor( doc ) {};
  WrappingCursor( const Kate::Document& doc, const KateTextCursor& c )          
    : CalculatingCursor( doc, c ) {};
  WrappingCursor( const Kate::Document& doc, uint line, uint col )
    : CalculatingCursor( doc, line, col ) {};
  virtual CalculatingCursor& operator+=( int n ) {
    if( n < 0 ) return operator-=( -n );
    int len = m_doc.lineLength( line );
    if( col + n <= len ) {
      col += n;
    } else if( uint( line ) < m_doc.numLines() - 1 ) {
      n -= len - col + 1;
      col = 0;
      line++;
      operator+=( n );
    } else {
      col = len;
    }
    Q_ASSERT( valid() );
    return *this;
  }
  virtual CalculatingCursor& operator-=( int n ) {
    if( n < 0 ) return operator+=( -n );
    if( col - n >= 0 ) {
      col -= n;
    } else if( line > 0 ) {
      n -= col + 1;
      line--;
      col = m_doc.lineLength( line );
      operator-=( n );
    } else {
      col = 0;
    }
    Q_ASSERT( valid() );
    return *this;
  }
};

void KateViewInternal::moveChar( Bias bias, bool sel )
{
  KateTextCursor c;
  if( m_doc->configFlags() & KateDocument::cfWrapCursor ) {
    c = WrappingCursor( *m_doc, cursor ) += bias;
  } else {
    c = BoundedCursor( *m_doc, cursor ) += bias;
  }
  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::cursorLeft(  bool sel ) { moveChar( left,  sel ); }
void KateViewInternal::cursorRight( bool sel ) { moveChar( right, sel ); }

void KateViewInternal::moveWord( Bias bias, bool sel )
{
  WrappingCursor c( *m_doc, cursor );
  if( !c.atEdge( bias ) ) {
    Highlight* h = m_doc->highlight();   
    c += bias;
    while( !c.atEdge( bias ) && !h->isInWord( m_doc->textLine( c.line )[ c.col - (bias == left ? 1 : 0) ] ) )
      c += bias;
    while( !c.atEdge( bias ) &&  h->isInWord( m_doc->textLine( c.line )[ c.col - (bias == left ? 1 : 0) ] ) )
      c += bias;
  } else {
    c += bias;
  }
  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::wordLeft ( bool sel ) { moveWord( left,  sel ); }
void KateViewInternal::wordRight( bool sel ) { moveWord( right, sel ); }

void KateViewInternal::moveEdge( Bias bias, bool sel )
{
  BoundedCursor c( *m_doc, cursor );
  c.toEdge( bias );
  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::home( bool sel )
{      
  if( !(m_doc->configFlags() & KateDocument::cfSmartHome) ) {
    moveEdge( left, sel );
    return;
  }

  KateTextCursor c = cursor;
  int lc = m_doc->kateTextLine( c.line )->firstChar();
  
  if( lc < 0 || c.col == lc ) {
    c.col = 0;
  } else {
    c.col = lc;
  }

  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::end( bool sel ) { moveEdge( right, sel ); }

void KateViewInternal::cursorUp(bool sel)
{
  if( displayCursor.line == 0 )
    return;
  
  KateTextCursor c( m_doc->getRealLine(displayCursor.line-1), cursor.col );
  m_doc->textWidth( c, cXPos );

  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::cursorDown(bool sel)
{
  KateTextCursor c = cursor;

  int x;
  
  if( c.line >= (int)m_doc->lastLine() ) {
    x = m_doc->lineLength( c.line );
    if( c.col >= x )
      return;
    c.col = x;
  } else {
    c.line = m_doc->getRealLine( displayCursor.line + 1 );
    x = m_doc->lineLength( c.line );
    if( c.col > x )
      c.col = x;
  }
  
  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::cursorToMatchingBracket( bool sel )
{
  KateTextCursor start( cursor ), end;

  if( !m_doc->findMatchingBracket( start, end ) )
    return;
  
  // The cursor is now placed just to the left of the matching bracket.
  // If it's an ending bracket, put it to the right (so we can easily
  // get back to the original bracket).
  if( end > start )
    end.col++;
    
  updateSelection( end, sel );
  updateCursor( end );
}

void KateViewInternal::topOfView( bool sel )
{
  WrappingCursor c( *m_doc, m_doc->getRealLine( firstLine() ), 0 );
  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::bottomOfView( bool sel )
{
  WrappingCursor c( *m_doc, m_doc->getRealLine( lastLine() ), 0 );
  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::scrollLines( int lines, bool sel )
{
  scrollBy( 0, lines * m_doc->viewFont.fontHeight );
  
  int line = int( displayCursor.line ) + lines;
  line = QMAX( 0, QMIN( line, int(m_doc->numVisLines()) ) );
  
  KateTextCursor c( m_doc->getRealLine( line ), cursor.col );
  m_doc->textWidth( c, cXPos );

  updateSelection( c, sel );
  updateCursor( c );
}

// Maybe not the right thing to do for these actions, but
// better than what was here before!
void KateViewInternal::scrollUp()   { scrollLines( -1, false ); }
void KateViewInternal::scrollDown() { scrollLines(  1, false ); }

void KateViewInternal::pageUp( bool sel )
{
  scrollLines( -QMAX( linesDisplayed() - 1, 0 ), sel );
}

void KateViewInternal::pageDown( bool sel )
{
  scrollLines( QMAX( linesDisplayed() - 1, 0 ), sel );
}

void KateViewInternal::top( bool sel )
{
  KateTextCursor c( 0, cursor.col );  
  m_doc->textWidth( c, cXPos );
  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::bottom( bool sel )
{
  KateTextCursor c( m_doc->lastLine(), cursor.col );
  m_doc->textWidth( c, cXPos );
  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::top_home( bool sel )
{
  KateTextCursor c( 0, 0 );
  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::bottom_end( bool sel )
{
  KateTextCursor c( m_doc->lastLine(), m_doc->lineLength( m_doc->lastLine() ) );
  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::slotContentsMoving( int, int y)
{                         
  yPos = y;

  updateView();              
  
  //kdDebug ()<< "now Y: "<<contentsY() << " new Y: "<<y<<endl;

  if (contentsY() != yPos)  
    leftBorder->scroll (0, contentsY()-yPos);
}

void KateViewInternal::updateSelection( const KateTextCursor& newCursor, bool keepSel )
{
  if( keepSel ) {
    m_doc->selectTo( cursor, newCursor );
  } else if( !(m_doc->configFlags() & KateDocument::cfPersistent) ) {
    m_doc->clearSelection();
  }
}

void KateViewInternal::updateCursor( const KateTextCursor& newCursor )
{
  if( cursor == newCursor )
    return;

  KateTextCursor oldDisplayCursor = displayCursor;

  cursor = newCursor;
  displayCursor.line = m_doc->getVirtualLine(cursor.line);
  displayCursor.col = cursor.col;

  //kdDebug(13030) << "updateCursor():" << endl;
  //kdDebug(13030) << "Virtual: " << displayCursor.line << endl;
  //kdDebug(13030) << "Real: " << cursor.line << endl;

  cXPos = m_doc->textWidth( cursor );
  ensureVisible( cXPos, lineToContentsY( displayCursor.line   ), 0, 0 );
  ensureVisible( cXPos, lineToContentsY( displayCursor.line+1 ), 0, 0 );

  if( bm.valid ) {
    tagLines( bm.startLine, bm.startLine );
    tagLines( bm.endLine, bm.endLine );
  }
  
  m_doc->newBracketMark( cursor, bm );
  
  if ( bm.valid ) {
    tagLines( bm.startLine, bm.startLine );
    tagLines( bm.endLine, bm.endLine );
  }

  bool b = oldDisplayCursor.line != displayCursor.line;
  if ( b )
    tagLines( oldDisplayCursor.line, oldDisplayCursor.line );
  tagLines( displayCursor.line, displayCursor.line, b );

  QPoint cursorP = cursorCoordinates();
  setMicroFocusHint( cursorP.x(), cursorP.y(), 0, m_doc->viewFont.fontHeight );
  
  if (cursorTimer) {
    killTimer(cursorTimer);
    cursorTimer = startTimer( KApplication::cursorFlashTime() / 2 );
    cursorOn = true;
  }
  
  emit m_view->cursorPositionChanged();
}

void KateViewInternal::tagRealLines( int start, int end )
{
  //kdDebug(13030) << "tagRealLines( " << start << ", " << end << " )\n";
  tagLines( m_doc->getVirtualLine( start ), m_doc->getVirtualLine( end ) );
}

void KateViewInternal::tagLines( int start, int end, bool updateLeftBorder )
{
  //kdDebug(13030) << "tagLines( " << start << ", " << end << " )\n";
  int y = lineToContentsY( start );
  int h = (end - start + 1) * m_doc->viewFont.fontHeight;
  updateContents( contentsX(), y, visibleWidth(), h );  
  if ( updateLeftBorder )
    leftBorder->update (0, y-contentsY(), leftBorder->width(), h);
}

void KateViewInternal::tagAll()
{
  //kdDebug(13030) << "tagAll()" << endl;
  updateContents();      
  leftBorder->update ();
}

void KateViewInternal::centerCursor()
{
  center( cXPos, lineToContentsY( displayCursor.line ) );
}

void KateViewInternal::updateView()
{
  uint maxLen = 0;
  uint endLine = lastLine();                                      
  uint first = firstLine ();
 
  // try to avoid flicker if the last line is too long and the scrollbar appears
  // -> last line not in view -> scrollbar disappear -> last line in view -> 
  // whole cycles endless, need some better adjustment, as one line could be
  // not enough in all cases and we perhaps should only do it if there is allready
  // a visible scrollbar
  if (first > 0)
    first--;
    
  if (lastLineCalc() < m_doc->visibleLines ())
    endLine = lastLineCalc();  
      
  for( uint line = firstLine(); line <= endLine; line++ ) {
    maxLen = QMAX( maxLen, m_doc->textWidth( m_doc->kateTextLine( m_doc->getRealLine( line ) ), -1 ) );
  }

  // Nice bit of extra space
  maxLen += 8;

  resizeContents( maxLen, m_doc->visibleLines() * m_doc->viewFont.fontHeight );
}

void KateViewInternal::paintCursor()
{
  tagLines( displayCursor.line, displayCursor.line );
}

// Point in content coordinates
void KateViewInternal::placeCursor( const QPoint& p, bool keepSelection )
{
  int line = contentsYToLine( p.y() );

  line = QMAX( 0, QMIN( line, int(m_doc->numVisLines()) - 1 ) );

  KateTextCursor c;
  c.line = m_doc->getRealLine( line );
  m_doc->textWidth( c, p.x() );

  updateSelection( c, keepSelection );
  updateCursor( c );
}

// Point in content coordinates
bool KateViewInternal::isTargetSelected( const QPoint& p )
{
  int line = contentsYToLine( p.y() );

  TextLine::Ptr textLine = m_doc->kateTextLine( m_doc->getRealLine( line ) );
  if( !textLine )
    return false;

  int col = m_doc->textPos( textLine, p.x() );

  return m_doc->lineColSelected( line, col );
}

bool KateViewInternal::eventFilter( QObject *obj, QEvent *e )
{
  if( obj != viewport() )
    return QScrollView::eventFilter( obj, e );

  KCursor::autoHideEventFilter( obj, e );
  
  switch( e->type() ) {
  case QEvent::FocusIn:
    cursorTimer = startTimer( KApplication::cursorFlashTime() / 2 );
    paintCursor();
    emit m_view->gotFocus( m_view );
    break;
  case QEvent::FocusOut:
    if( ! m_view->m_codeCompletion->codeCompletionVisible() ) {
      if( cursorTimer ) {
        killTimer( cursorTimer );
        cursorTimer = 0;
      }
      paintCursor();
      emit m_view->lostFocus( m_view );
    }
    break;
  case QEvent::KeyPress: {
    QKeyEvent *k = (QKeyEvent *)e;
    if ( !(k->state() & ControlButton || k->state() & AltButton) ) {
      keyPressEvent( k );
      return k->isAccepted();
    }
  } break;
  default:
    break;
  }
        
  return QScrollView::eventFilter( obj, e );
}

void KateViewInternal::keyPressEvent( QKeyEvent* e )
{
  if( !m_doc->isReadWrite() ) {
    e->ignore();
    return;
  }

  if( m_doc->configFlags() & KateDocument::cfTabIndents && m_doc->hasSelection() ) {
    KKey key(e);
    if( key == Qt::Key_Tab ) {
      m_doc->indent( cursor.line );
      return;
    } else if (key == SHIFT+Qt::Key_Backtab || key == Qt::Key_Backtab) {
      m_doc->unIndent( cursor.line );
      return;
    }
  }
  if ( !(e->state() & ControlButton) && !(e->state() & AltButton)
         && m_doc->insertChars( cursor.line, cursor.col, e->text(), m_view ) )
  {
    e->accept();
    return;
  }
  
  e->ignore();
}

void KateViewInternal::contentsMousePressEvent( QMouseEvent* e )
{
  if (e->button() == LeftButton) {
    if (possibleTripleClick) {
      possibleTripleClick = false;
      m_doc->selectLine( cursor );
      cursor.col = 0;
      updateCursor( cursor );
      return;
    }

    if( isTargetSelected( e->pos() ) ) {
      dragInfo.state = diPending;
      dragInfo.start = contentsToViewport( e->pos() );
    } else {
      dragInfo.state = diNone;

      placeCursor( e->pos(), e->state() & ShiftButton );
      scrollX = 0;
      scrollY = 0;
      if( !scrollTimer )
        scrollTimer = startTimer(50);
    }
  }

  if( m_view->popup() && e->button() == RightButton ) {
    if( !isTargetSelected( e->pos() ) )
      placeCursor( e->pos() );
    m_view->popup()->popup( mapToGlobal( contentsToViewport( e->pos() ) ) );
  }
}

void KateViewInternal::contentsMouseDoubleClickEvent(QMouseEvent *e)
{
  if (e->button() == LeftButton) {

    m_doc->selectWord( cursor );

    // Move cursor to end of selected word
    if (m_doc->hasSelection())
    {
      cursor.col = m_doc->selectEnd.col;
      cursor.line = m_doc->selectEnd.line;
      updateCursor( cursor );
    }

    possibleTripleClick = true;
    QTimer::singleShot( QApplication::doubleClickInterval(),
      this, SLOT(tripleClickTimeout()) );
  }
}

void KateViewInternal::tripleClickTimeout()
{
  possibleTripleClick = false;
}

void KateViewInternal::contentsMouseReleaseEvent( QMouseEvent* e )
{
  if (e->button() == LeftButton) {
    if (dragInfo.state == diPending) {
      // we had a mouse down in selected area, but never started a drag
      // so now we kill the selection
      placeCursor( e->pos() );
    } else if (dragInfo.state == diNone) {
      QApplication::clipboard()->setSelectionMode( true );
      m_doc->copy();
      QApplication::clipboard()->setSelectionMode( false );

      killTimer(scrollTimer);
      scrollTimer = 0;
    }
    dragInfo.state = diNone;
  } else if (e->button() == MidButton) {
    placeCursor( e->pos() );
    if( m_doc->isReadWrite() )
    {
      QApplication::clipboard()->setSelectionMode( true );
      doPaste();
      QApplication::clipboard()->setSelectionMode( false );
    }
  }
}

void KateViewInternal::contentsMouseMoveEvent( QMouseEvent* e )
{
  if( e->state() & LeftButton ) {
    
    if (dragInfo.state == diPending) {
      // we had a mouse down, but haven't confirmed a drag yet
      // if the mouse has moved sufficiently, we will confirm
      QPoint p( e->pos() - dragInfo.start );
      if( p.manhattanLength() > KGlobalSettings::dndEventDelay() ) {
        // we've left the drag square, we can start a real drag operation now
        doDrag();
      }
      return;
    }
   
    contentsToViewport (e->x(), e->y(), mouseX, mouseY);
    
    scrollX = 0;
    scrollY = 0;
    int d = m_doc->viewFont.fontHeight;
    if (mouseX < 0) {
      mouseX = 0;
      scrollX = -d;
    }
    if (mouseX > visibleWidth()) {
      mouseX = visibleWidth();
      scrollX = d;
    }
    if (mouseY < 0) {
      mouseY = 0;
      scrollY = -d;
    }
    if (mouseY > visibleHeight()) {
      mouseY = visibleHeight();
      scrollY = d;
    }
    placeCursor( viewportToContents( QPoint( mouseX, mouseY ) ), true );
  }
}

void KateViewInternal::drawContents( QPainter *paint, int cx, int cy, int cw, int ch )
{
  uint h = m_doc->viewFont.fontHeight;

  uint startline = contentsYToLine( cy );
  uint endline   = contentsYToLine( cy + ch - 1 );
  
  //kdDebug(13030) << "drawContents(): y = " << cy << " h = " << ch << endl; 
  //kdDebug(13030) << "drawContents(): x = " << cx << " w = " << cw << endl;
  //kdDebug(13030) << "Repainting " << startline << " - " << endline << endl;

  for( uint line = startline; line <= endline; line++ ) {
    uint realLine = m_doc->getRealLine( line );
    if( realLine > m_doc->lastLine() ) {
      paint->fillRect( cx, line*h, cw, h, m_doc->colors[0] );
    } else {
      m_doc->paintTextLine( *paint, realLine, 0, -1,
                            cx, line*h, cx, cx+cw,
                            (cursorOn && (hasFocus()||m_view->m_codeCompletion->codeCompletionVisible()) && (realLine == uint(cursor.line))) ? cursor.col : -1,
                            m_view->isOverwriteMode(), cXPos, true,
                            m_doc->configFlags() & KateDocument::cfShowTabs,
                            KateDocument::ViewFont, realLine == uint(cursor.line),
                            false, bm );
    }
  }
}

void KateViewInternal::viewportResizeEvent( QResizeEvent* )
{ 
  updateIconBorder();
  updateView();
}

void KateViewInternal::timerEvent( QTimerEvent* e )
{
  if (e->timerId() == cursorTimer)
  {
    cursorOn = !cursorOn;
    paintCursor();
  }
  else if (e->timerId() == scrollTimer && (scrollX | scrollY))
  {   
     scrollBy( scrollX, scrollY );
    placeCursor( viewportToContents( QPoint( mouseX, mouseY ) ), true );
  }
}

void KateViewInternal::doDrag()
{
  dragInfo.state = diDragging;
  dragInfo.dragObject = new QTextDrag(m_doc->selection(), this);
  dragInfo.dragObject->dragCopy();
}

void KateViewInternal::contentsDragEnterEvent( QDragEnterEvent* event )
{
  event->accept( (QTextDrag::canDecode(event) && m_doc->isReadWrite()) ||
                  QUriDrag::canDecode(event) );
}

void KateViewInternal::contentsDropEvent( QDropEvent* event )
{
  if ( QUriDrag::canDecode(event) ) {

      emit dropEventPass(event);

  } else if ( QTextDrag::canDecode(event) && m_doc->isReadWrite() ) {

    QString text;

    if (!QTextDrag::decode(event, text))
      return;
    
    // is the source our own document?
    bool priv = m_doc->ownedView( (KateView*)(event->source()) );
    // dropped on a text selection area?
    bool selected = isTargetSelected( event->pos() );

    if( priv && selected ) {
      // this is a drag that we started and dropped on our selection
      // ignore this case
      return;
    }

    if( priv && event->action() == QDropEvent::Move ) {
      // this is one of mine (this document), not dropped on the selection
      m_doc->removeSelectedText();
    }
    placeCursor( event->pos() );
    m_doc->insertText( cursor.line, cursor.col, text );

    updateView();
  }
}

void KateViewInternal::clear()
{
  cursor.setPos(0, 0);
  displayCursor.setPos(0, 0);
}

void KateViewInternal::setTagLinesFrom(int line)
{
  if ( tagLinesFrom > line || tagLinesFrom == -1)
    tagLinesFrom = line;
}

void KateViewInternal::editStart()
{
  cursorCacheChanged = false;
  tagLinesFrom = -1;
  cursorCache = getCursor();
}

void KateViewInternal::editEnd(int editTagLineStart, int editTagLineEnd)
{
  if (cursorCacheChanged)
  {
    cursorCacheChanged = false;
    updateCursor( cursorCache );
  }
  
  updateView();

    if (tagLinesFrom > -1)
    {
      int startTagging = QMIN( tagLinesFrom, editTagLineStart );
      int endTagging = m_doc->getRealLine( lastLine() );
      tagRealLines (startTagging, endTagging);
    }
    else
      tagRealLines (editTagLineStart, editTagLineEnd);

    tagLinesFrom = -1;
}

void KateViewInternal::editRemoveText(int line, int col, int len)
{
  int cLine = cursorCache.line;
  int cCol = cursorCache.col;

  if ( (cLine == line) && (cCol > col) )
  {
    if ((cCol - len) >= col)
    {
      if ((cCol - len) > 0)
	cCol = cCol-len;
      else
	cCol = 0;
    }
    else
      cCol = col;

    cursorCache.setPos(line, cCol);
    cursorCacheChanged = true;
  }
}

void KateViewInternal::removeSelectedText(KateTextCursor & start)
{
  if (m_doc->lineHasSelected(cursorCache.line))
  {
    cursorCache.setPos(start);
    cursorCacheChanged = true;
  }
}


void KateViewInternal::setViewTagLinesFrom(int line)
{
  if (line >= (int)m_doc->getRealLine(firstLine()))
    setTagLinesFrom(line);
}

void KateViewInternal::editWrapLine(int line, int col, int len)
{
  setViewTagLinesFrom(line);

  // correct cursor position
  if (cursorCache.line > line)
  {
    cursorCache.line++;
    cursorCacheChanged = true;
  }
  else if ( cursorCache.line == line && cursorCache.col >= col )
  {
    cursorCache.setPos(line + 1, len);
    cursorCacheChanged = true;
  }
}

void KateViewInternal::editUnWrapLine(int line, int col)
{
  setViewTagLinesFrom(line);

  int cLine = cursorCache.line;
  int cCol = cursorCache.col;

  if ( (cLine == (line+1)) || ((cLine == line) && (cCol >= col)) )
  {
    cursorCache.setPos(line, col);
    cursorCacheChanged = true;
  }
}

void KateViewInternal::editRemoveLine(int line)
{
  setViewTagLinesFrom(line);

  if ( (cursorCache.line == line) )
  {
    int newLine = (line < (int)m_doc->lastLine()) ? line : (line - 1);

    cursorCache.setPos(newLine, 0);
    cursorCacheChanged = true;
  }
}
