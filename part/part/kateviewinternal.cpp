/* This file is part of the KDE libraries
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

// $Id$

#include "kateviewinternal.h"
#include "kateview.h"
#include "kateviewinternal.moc"
#include "katedocument.h"
#include "kateiconborder.h"
#include "katehighlight.h"

#include <kcursor.h>
#include <kapplication.h>

#include <qstyle.h>
#include <qdragobject.h>
#include <qdropsite.h>
#include <qtimer.h>
#include <qpainter.h>
#include <qclipboard.h>

KateViewInternal::KateViewInternal(KateView *view, KateDocument *doc)
 : QWidget(view, "", Qt::WRepaintNoErase | Qt::WResizeNoErase)
{
  setBackgroundMode(NoBackground);

  myView = view;
  myDoc = doc;

  displayCursor.line=0;
  displayCursor.col=0;

  startLine = 0;
  startLineReal = 0;
  endLine = 0;

  xPos = 0;

  scrollTimer = 0;

  cursor.col = 0;
  cursor.line = 0;
  cursorOn = false;
  cursorTimer = 0;
  cXPos = 0;

  possibleTripleClick = false;
  updateState = 0;
  newStartLine = 0;
  newStartLineReal = 0;
  
  drawBuffer = new QPixmap ();
  drawBuffer->setOptimization (QPixmap::BestOptim);

  bm.sXPos = 0;
  bm.eXPos = -1;

  // create a one line lineRanges array
  updateLineRanges ();

  QWidget::setCursor(ibeamCursor);
  KCursor::setAutoHideCursor( this, true, true );

  setFocusPolicy(StrongFocus);

  xScroll = new QScrollBar(QScrollBar::Horizontal,myView);
  yScroll = new QScrollBar(QScrollBar::Vertical,myView);

  connect(xScroll,SIGNAL(valueChanged(int)),SLOT(changeXPos(int)));
  connect(yScroll,SIGNAL(valueChanged(int)),SLOT(changeYPos(int)));

  setAcceptDrops(true);
  dragInfo.state = diNone;
}

KateViewInternal::~KateViewInternal()
{
  delete drawBuffer;
}

void KateViewInternal::doReturn()
{
  if( myDoc->configFlags() & KateDocument::cfDelOnInput )
    myDoc->removeSelectedText();
  KateTextCursor c = cursor;
  myDoc->newLine( c );
  updateCursor( c );
  updateView();
}

void KateViewInternal::doDelete()
{
  if ( (myDoc->configFlags() & KateDocument::cfDelOnInput) && myDoc->hasSelection() ) {
    myDoc->removeSelectedText();
  } else {
    myDoc->del( cursor );
  }
}

void KateViewInternal::doBackspace()
{
  if( (myDoc->configFlags() & KateDocument::cfDelOnInput) && myDoc->hasSelection() ) {
    myDoc->removeSelectedText();
  } else {
    myDoc->backspace( cursor );
  }
}

void KateViewInternal::doPaste()
{
   if( myDoc->configFlags() & KateDocument::cfDelOnInput )
     myDoc->removeSelectedText();
   VConfig c;
   getVConfig(c);
   myDoc->paste(c);
}

void KateViewInternal::doTranspose()
{
  myDoc->transpose( cursor );
  cursorRight();
  cursorRight();
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
  if( myDoc->configFlags() & KateDocument::cfWrapCursor ) {
    c = WrappingCursor( *myDoc, cursor ) += bias;
  } else {
    c = BoundedCursor( *myDoc, cursor ) += bias;
  }
  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::cursorLeft(  bool sel ) { moveChar( left,  sel ); }
void KateViewInternal::cursorRight( bool sel ) { moveChar( right, sel ); }

void KateViewInternal::moveWord( Bias bias, bool sel )
{
  WrappingCursor c( *myDoc, cursor );
  if( !c.atEdge( bias ) ) {
    Highlight* h = myDoc->highlight();   
    c += bias;
    while( !c.atEdge( bias ) && !h->isInWord( myDoc->textLine( c.line )[ c.col - (bias == left ? 1 : 0) ] ) )
      c += bias;
    while( !c.atEdge( bias ) &&  h->isInWord( myDoc->textLine( c.line )[ c.col - (bias == left ? 1 : 0) ] ) )                                                
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
  BoundedCursor c( *myDoc, cursor );
  c.toEdge( bias );
  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::home( bool sel )
{
  if( !(myDoc->configFlags() & KateDocument::cfSmartHome) ) {
    moveEdge( left, sel );
    return;
  }

  KateTextCursor tmpCur = cursor;
  int lc = myDoc->kateTextLine(tmpCur.line)->firstChar();
  
  if (lc <= 0 || tmpCur.col == lc) {
    tmpCur.col = 0;
  } else {
    tmpCur.col = lc;
  }

  updateSelection( tmpCur, sel );
  updateCursor( tmpCur );
}

void KateViewInternal::end( bool sel ) { moveEdge( right, sel ); }

void KateViewInternal::cursorUp(bool sel)
{
  if( displayCursor.line == 0 )
    return;
  
  KateTextCursor c( myDoc->getRealLine(displayCursor.line-1), cursor.col );
  myDoc->textWidth( c, cXPos );

  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::cursorDown(bool sel)
{
  KateTextCursor tmpCur = cursor;
  int x;
  
  if (tmpCur.line >= (int)myDoc->lastLine()) {
    x = myDoc->textLength(tmpCur.line);
    if (tmpCur.col >= x) return;
    tmpCur.col = x;
  } else {
    tmpCur.line=myDoc->getRealLine(displayCursor.line+1);
     x = myDoc->textLength(tmpCur.line);
    if (tmpCur.col > x)
      tmpCur.col = x;
  }
  
  updateSelection( tmpCur, sel );
  updateCursor( tmpCur );
}

void KateViewInternal::topOfView( bool sel )
{
  WrappingCursor c( *myDoc, startLineReal, 0 );
  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::bottomOfView( bool sel )
{
  int line = startLineReal + linesDisplayed() - 1;
  WrappingCursor c( *myDoc, line, 0 );
  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::scrollLines( int lines, bool sel )
{
  // This block can probably be optimized
  if( lines > 0 ) { // Down
    if( endLine < myDoc->lastLine() ) {
      if( lines < myDoc->lastLine() - endLine )
        newStartLine = startLine + lines;
      else
        newStartLine = startLine + (myDoc->lastLine() - endLine);
    }
  } else { // Up
    if( startLine > 0 ) {
      newStartLine = QMAX( startLine - lines, 0 );
    }
  }
  
  int line = QMAX( 0, QMIN( int(displayCursor.line) + lines, int(myDoc->numVisLines()) ) );
  
  KateTextCursor c( myDoc->getRealLine( line ), cursor.col );
  myDoc->textWidth( c, cXPos );

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
  myDoc->textWidth( c, cXPos );
  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::bottom( bool sel )
{
  KateTextCursor c( myDoc->lastLine(), cursor.col );
  myDoc->textWidth( c, cXPos );
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
  KateTextCursor c( myDoc->lastLine(), myDoc->textLength( myDoc->lastLine() ) );
  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::changeXPos(int p)
{
  int dx = xPos - p;
  xPos = p;
  if (QABS(dx) < width())
    scroll(dx, 0);
  else
    update();
}

void KateViewInternal::changeYPos(int p)
{

  newStartLine = p  / myDoc->viewFont.fontHeight;

  int dy = (startLine - newStartLine)  * myDoc->viewFont.fontHeight;

  updateLineRanges();

  if (QABS(dy) < height())
  {
    scroll(0, dy);
    leftBorder->scroll(0,dy);
  }
  else
  {
    repaint();
    leftBorder->repaint();
  }

  updateView();
}

void KateViewInternal::getVConfig(VConfig &c)
{
  c.view = myView;
  c.cursor = cursor;
  c.displayCursor=displayCursor;
  c.cXPos = cXPos;
  c.flags = myDoc->configFlags();
}

 QPoint KateViewInternal::cursorCoordinates()
 {
   return QPoint(
     cXPos - xPos,
    (displayCursor.line-startLine+1)*myDoc->viewFont.fontHeight );
} 

void KateViewInternal::updateSelection( const KateTextCursor& newCursor, bool keepSel )
{
  if( keepSel ) {
    myDoc->selectTo( cursor, newCursor );
  } else if( !(myDoc->configFlags() & KateDocument::cfPersistent) ) {
    myDoc->clearSelection();
  }
}

void KateViewInternal::updateCursor( const KateTextCursor& newCursor, int updateViewFlags )
{
  bool nullMove = cursor == newCursor;

  if( cursorOn ) {
    tagLines( displayCursor.line, displayCursor.line );
    cursorOn = false;
  }

  if( bm.sXPos < bm.eXPos ) {
    tagLines( bm.cursor.line, bm.cursor.line );
  }

  myDoc->newBracketMark( newCursor, bm );
  
  cursor = newCursor;
  displayCursor.line = myDoc->getVirtualLine(cursor.line);
  displayCursor.col = cursor.col;
  cXPos = myDoc->textWidth( cursor );
  exposeCursor();
  
  int h = myDoc->viewFont.fontHeight;
  int y = h*(displayCursor.line - startLine);
  int x = cXPos - xPos;

  setMicroFocusHint( x, y, 0, h );
  
  updateView( updateViewFlags );
  
  if( !nullMove )
    emit myView->cursorPositionChanged();
}

void KateViewInternal::updateLineRanges()
{
  if( newStartLineReal != startLineReal ) {
    startLine = myDoc->getVirtualLine( newStartLineReal );
    startLineReal = newStartLineReal;
  } else if( newStartLine != startLine ) {
    startLine = newStartLine;
    startLineReal = myDoc->getRealLine( startLine );
  }

  int fontHeight = myDoc->viewFont.fontHeight;
  int lines = linesDisplayed();
  
  endLine = QMIN( myDoc->visibleLines() - 1, startLine + lines - 1 );
  endLineReal = myDoc->getRealLine( endLine );

  if( lines * fontHeight < height() )
    lines++;

  int oldLines = lineRanges.size();
  lineRanges.resize( lines );

  for( uint z = 0; z < lines; z++ ) {
  
    uint newLine = myDoc->getRealLine( startLine+z );

    if (z < oldLines) {
      if( newLine != lineRanges[z].line ) {
        lineRanges[z].line = newLine;
        lineRanges[z].dirty = true;
      }
      lineRanges[z].startCol = 0;
      lineRanges[z].endCol = -1;
      lineRanges[z].wrapped = false;
    } else {
      lineRanges[z].line = newLine;
      lineRanges[z].startCol = 0;
      lineRanges[z].endCol = -1;
      lineRanges[z].wrapped = false;
      lineRanges[z].dirty = true;
    }

    lineRanges[z].empty = (startLine+z) > endLine;
  }

  newStartLine = startLine;
  newStartLineReal = startLineReal;
}

void KateViewInternal::tagRealLines( int start, int end )
{
  for( uint z = 0; z < lineRanges.size() && lineRanges[z].line <= end; z++) {
    if( lineRanges[z].line >= start ) {
      lineRanges[z].dirty = true;
      updateState |= 1;
    }
  }
}

void KateViewInternal::tagLines( int start, int end )
{
  start = QMAX( start - startLine, 0 );
  end = QMIN( end - startLine, endLine - startLine );

  if( start >= lineRanges.size() )
   return;
    
  KateLineRange* r = &lineRanges[start];
  uint rpos = start;

  for( int z = start; z <= end && rpos < lineRanges.size(); z++ ) {

    r->dirty = true;

    r++;
    rpos++;

    updateState |= 1;
  }
}

void KateViewInternal::tagAll() {
  updateState = 3;
}

void KateViewInternal::center() {
  newStartLine = QMAX( displayCursor.line - linesDisplayed() / 2, 0 );
  updateView();
}

uint KateViewInternal::linesDisplayed() const
{
  return height() / myDoc->viewFont.fontHeight;
}

void KateViewInternal::exposeCursor()
{
  int tmpYPos = 0;
  int fontHeight = myDoc->viewFont.fontHeight;

  if( displayCursor.line >= startLine + linesDisplayed() )
  {
    tmpYPos = displayCursor.line * fontHeight - height() + fontHeight;
    if( tmpYPos % fontHeight != 0 )
      tmpYPos += fontHeight;
  } else if( displayCursor.line < startLine ) {
    tmpYPos = displayCursor.line * fontHeight;
  } else {
    return;
  }
  
  newStartLine = tmpYPos / fontHeight;
      
  yScroll->blockSignals(true);
  yScroll->setValue(tmpYPos);
  yScroll->blockSignals(false);

  int dy = (startLine - newStartLine) * fontHeight;

  updateLineRanges();

  if (QABS(dy) < height())
  {
    scroll(0, dy);
    leftBorder->scroll(0,dy);
  }
  else
  {
    repaint();
    leftBorder->repaint();
  }
}

void KateViewInternal::updateView(int flags)
{
  int oldU = updateState;
  
  int fontHeight = myDoc->viewFont.fontHeight;
  bool needLineRangesUpdate = updateState == 3;
  int scrollbarWidth = style().scrollBarExtent().width();

  int w = myView->width();
  int h = myView->height();
  
  w -= leftBorder->width();
  
  //
  //  update yScrollbar (first that, as we need if it should be shown for the width() of the view)
  //
  if( (flags & KateViewInternal::ufFoldingChanged) || (flags & KateViewInternal::ufDocGeometry) )
  {
    uint contentLines = myDoc->visibleLines();
    int viewLines = linesDisplayed();
    int yMax = (contentLines-viewLines) * fontHeight;
    
    if( yMax > 0 ) {
      int pageScroll = h - (h % fontHeight) - fontHeight;
      if (pageScroll <= 0)
        pageScroll = fontHeight;

      yScroll->blockSignals(true);
      yScroll->setGeometry(myView->width()-scrollbarWidth,0,scrollbarWidth, myView->height()-scrollbarWidth);
      yScroll->setRange(0,yMax);
      yScroll->setValue(startLine * myDoc->viewFont.fontHeight);
      yScroll->setSteps(fontHeight,pageScroll);
      yScroll->blockSignals(false);
      yScroll->show();
    } else {
      yScroll->hide();
    }
      
    if (yScroll->isVisible())
      w -= scrollbarWidth;
      
    w = QMAX( 0, w );

    if (w != width() || h != height())
    {
      needLineRangesUpdate = true;
      resize(w,h);
    }
  }

  //
  // update the lineRanges (IMPORTANT)
  //
  if( flags & KateViewInternal::ufFoldingChanged ) {
    needLineRangesUpdate = true;
    updateLineRanges();
  } else if( needLineRangesUpdate && !(flags && KateViewInternal::ufDocGeometry) ) {
    updateLineRanges();
  }

  //
  // update xScrollbar
  //
  uint maxLen = 0;
  for( uint z = 0; z < lineRanges.size(); z++ ) {
    if (lineRanges[z].dirty)
    {
      lineRanges[z].lengthPixel = myDoc->textWidth( myDoc->kateTextLine( lineRanges[z].line ), -1 );
    }
    maxLen = QMAX( maxLen, lineRanges[z].lengthPixel );
  }

  maxLen += 8;

  if( maxLen > w ) {
    if (!xScroll->isVisible())
      h -= scrollbarWidth;
        
    int pageScroll = w - (w % fontHeight) - fontHeight;
    if (pageScroll <= 0)
      pageScroll = fontHeight;

    xScroll->blockSignals(true);
    xScroll->setGeometry(0,myView->height()-scrollbarWidth,w,scrollbarWidth);
    xScroll->setRange(0,maxLen);
    xScroll->setSteps(fontHeight,pageScroll);
    xScroll->blockSignals(false);
    xScroll->show();
  } else {
    if (xScroll->isVisible())
      h += scrollbarWidth;
  
    xScroll->hide();
  }

  if (h != height())
    resize(w,h);
    
  if ((flags & KateViewInternal::ufRepaint) || (flags & KateViewInternal::ufFoldingChanged))
  {
    repaint();
    leftBorder->repaint();
  }
  else if( oldU > 0 )
  {
    paintTextLines( xPos );
   //  kdDebug(13000)<<"repaint lines"<<endl;
  }

  //
  // updateView done, reset the update flag + repaint flags
  //
  updateState = 0;

  // blank repaint attribs
  for (uint z = 0; z < lineRanges.size(); z++)
  {
    lineRanges[z].dirty = false;
  }
}

void KateViewInternal::paintTextLines( int xPos )
{
  if (drawBuffer->isNull()) return;

  QPainter paint( drawBuffer );

  uint fontHeight = myDoc->viewFont.fontHeight;
  KateLineRange* r = lineRanges.data();

  uint rpos = 0;

  bool isOverwrite = myView->isOverwriteMode();

  bool again = true;

  for( uint line = startLine; rpos < lineRanges.size(); line++ ) {
  
    if (r->dirty && !r->empty) {
    
      myDoc->paintTextLine( paint, r->line, r->startCol, r->endCol,
                            0, xPos, xPos + width(),
                            (cursorOn && (r->line == cursor.line)) ? cursor.col : -1,
                            isOverwrite, true,
                            myDoc->configFlags() & KateDocument::cfShowTabs,
                            KateDocument::ViewFont, again && (r->line == cursor.line) );

      if( cXPos > (int)r->lengthPixel ) {
        if( cursorOn && (r->line == cursor.line) ) {     
          if( isOverwrite ) {
            int cursorMaxWidth = myDoc->viewFont.myFontMetrics.width(QChar (' '));
            paint.fillRect( cXPos-xPos, 0, cursorMaxWidth,
                            fontHeight, myDoc->myAttribs[0].col );
          } else {
            paint.fillRect( cXPos-xPos, 0, 2,
                            fontHeight, myDoc->myAttribs[0].col );
          }
        }
      }
      
      bitBlt( this, 0, (line-startLine)*fontHeight, drawBuffer,
              0, 0, width(), fontHeight );
      leftBorder->paintLine(line, r);
      
    } else if (r->empty) {
    
      paint.fillRect( 0, 0, width(), fontHeight, myDoc->colors[0] );
      bitBlt( this, 0, (line-startLine)*fontHeight, drawBuffer,
              0, 0, width(), fontHeight );
      leftBorder->paintLine(line, r);
      
    }

    if ((int)r->line == cursor.line)
      again = false;

    r++;
    rpos++;
  }
}

void KateViewInternal::paintCursor()
{
  tagRealLines( cursor.line, cursor.line );
  paintTextLines( xPos );
}

void KateViewInternal::paintBracketMark()
{
  int y = myDoc->viewFont.fontHeight*(bm.cursor.line +1 - startLine) -1;

  QPainter paint( this );
  paint.setPen(myDoc->cursorCol(bm.cursor.col, bm.cursor.line));
  paint.drawLine(bm.sXPos - xPos, y, bm.eXPos - xPos -1, y);
}

void KateViewInternal::placeCursor( int x, int y, bool keepSelection )
{
  int newDisplayLine = startLine + y / myDoc->viewFont.fontHeight;

  if( newDisplayLine >= myDoc->numVisLines() )
    newDisplayLine = myDoc->numVisLines() - 1;

  int index = newDisplayLine - startLine;
  if( ( index < 0 ) || ( index >= lineRanges.size() ) )
    return; // not sure yet, if this is ther correct way;

  KateTextCursor c;
  c.line = lineRanges[index].line;
  myDoc->textWidth( c, xPos + x );

  updateSelection( c, keepSelection );
  updateCursor( c );
}

// given physical coordinates, report whether the text there is selected
bool KateViewInternal::isTargetSelected(int x, int y)
{

  y=startLine + y/myDoc->viewFont.fontHeight;

  if (((y-startLine) < 0) || ((y-startLine) >= lineRanges.size())) return false;

  y= lineRanges[y-startLine].line;

  TextLine::Ptr line = myDoc->kateTextLine(y);
  if (!line)
    return false;

  x = myDoc->textPos(line, x);

  return myDoc->lineColSelected(y, x);
}

void KateViewInternal::focusInEvent(QFocusEvent *) {
//  debug("got focus %d",cursorTimer);

  if (!cursorTimer) {
    cursorTimer = startTimer(KApplication::cursorFlashTime() / 2);
    cursorOn = true;
    paintCursor();
  }
}

void KateViewInternal::focusOutEvent(QFocusEvent *) {
//  debug("lost focus %d", cursorTimer);

  if (cursorTimer) {
    killTimer(cursorTimer);
    cursorTimer = 0;
  }

  if (cursorOn) {
    cursorOn = false;
    paintCursor();
  }
}

void KateViewInternal::keyPressEvent( QKeyEvent* e )
{
  if( !myView->doc()->isReadWrite() ) {
    e->ignore();
    return;
  }

  if( myDoc->configFlags() & KateDocument::cfTabIndents && myDoc->hasSelection() ) {
    KKey key(e);
    if( key == Qt::Key_Tab ) {
      myDoc->indent( cursor.line );
      return;
    } else if (key == SHIFT+Qt::Key_Backtab || key == Qt::Key_Backtab) {
      myDoc->unIndent( cursor.line );
      return;
    }
  }
  if ( !(e->state() & ControlButton) && !(e->state() & AltButton)
         && myDoc->insertChars( cursor.line, cursor.col, e->text(), myView ) )
  {
    e->accept();
    return;
  }
  
  e->ignore();
}

void KateViewInternal::mousePressEvent(QMouseEvent *e) {

  if (e->button() == LeftButton) {
    if (possibleTripleClick) {
      possibleTripleClick = false;
      myDoc->selectLine( cursor );
      cursor.col = 0;
      updateCursor( cursor );
      return;
    }

    if (isTargetSelected(e->x(), e->y())) {
      // we have a mousedown on selected text
      // we initialize the drag info thingy as pending from this position

      dragInfo.state = diPending;
      dragInfo.start.col = e->x();
      dragInfo.start.line = e->y();
    } else {
      // we have no reason to ever start a drag from here
      dragInfo.state = diNone;

      placeCursor( e->x(), e->y(), e->state() & ShiftButton );
      scrollX = 0;
      scrollY = 0;
      if (!scrollTimer) scrollTimer = startTimer(50);
      updateView();
    }
  }

  if (myView->popup() && e->button() == RightButton) {
    myView->popup()->popup(mapToGlobal(e->pos()));
  }
  myView->mousePressEvent(e); // this doesn't do anything, does it?
  // it does :-), we need this for KDevelop, so please don't uncomment it again -Sandy
}

void KateViewInternal::mouseDoubleClickEvent(QMouseEvent *e)
{
  if (e->button() == LeftButton) {

    myDoc->selectWord( cursor );

    // Move cursor to end of selected word
    if (myDoc->hasSelection())
    {
      cursor.col = myDoc->selectEnd.col;
      cursor.line = myDoc->selectEnd.line;
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

void KateViewInternal::mouseReleaseEvent(QMouseEvent *e) {
  if (e->button() == LeftButton) {
    if (dragInfo.state == diPending) {
      // we had a mouse down in selected area, but never started a drag
      // so now we kill the selection
      placeCursor(e->x(), e->y());
    } else if (dragInfo.state == diNone) {
      QApplication::clipboard()->setSelectionMode( true );
      myView->copy();
      QApplication::clipboard()->setSelectionMode( false );

      killTimer(scrollTimer);
      scrollTimer = 0;
    }
    dragInfo.state = diNone;
  } else if (e->button() == MidButton) {
    placeCursor(e->x(), e->y());
    if (myView->doc()->isReadWrite())
    {
      QApplication::clipboard()->setSelectionMode( true );
      myView->paste();
      QApplication::clipboard()->setSelectionMode( false );
    }
  }
}

void KateViewInternal::mouseMoveEvent(QMouseEvent *e) {

  if (e->state() & LeftButton) {
    
    int d;
    int x = e->x(),
        y = e->y();

    if (dragInfo.state == diPending) {
      // we had a mouse down, but haven't confirmed a drag yet
      // if the mouse has moved sufficiently, we will confirm

      if (x > dragInfo.start.col + 4 || x < dragInfo.start.col - 4 ||
          y > dragInfo.start.line + 4 || y < dragInfo.start.line - 4) {
        // we've left the drag square, we can start a real drag operation now
        doDrag();
      }
      return;
    } else if (dragInfo.state == diDragging) {
      // this isn't technically needed because mouseMoveEvent is suppressed during
      // Qt drag operations, replaced by dragMoveEvent
      return;
    }

    mouseX = e->x();
    mouseY = e->y();
    scrollX = 0;
    scrollY = 0;
    d = myDoc->viewFont.fontHeight;
    if (mouseX < 0) {
      mouseX = 0;
      scrollX = -d;
    }
    if (mouseX > width()) {
      mouseX = width();
      scrollX = d;
    }
    if (mouseY < 0) {
      mouseY = 0;
      scrollY = -d;
    }
    if (mouseY > height()) {
      mouseY = height();
      scrollY = d;
    }
    placeCursor( mouseX, mouseY, true );
    updateView();
  }
}

void KateViewInternal::wheelEvent( QWheelEvent *e )
{
  if( yScroll->isVisible() ) {
    QApplication::sendEvent( yScroll, e );
  }
}

void KateViewInternal::paintEvent(QPaintEvent *e)
{
  if (drawBuffer->isNull()) return;

  QRect updateR = e->rect();
  int xStart = xPos + updateR.x();
  int xEnd = xStart + updateR.width();
  uint h = myDoc->viewFont.fontHeight;
  uint startline = startLine + (updateR.y() / h);
  uint endline = startline + 1 + (updateR.height() / h);

  KateLineRange *r = lineRanges.data();
  uint rpos = startline-startLine;

  QPainter paint( drawBuffer );

  if (rpos <= lineRanges.size())
    r += rpos;
  else
    return;

  bool b = myView->isOverwriteMode();
  bool again = true;
  for ( uint line = startline; (line <= endline) && (rpos < lineRanges.size()); line++)
  {
    if (r->empty) {
      paint.fillRect(0, 0, updateR.width(), h, myDoc->colors[0]);
    } else {
      myDoc->paintTextLine ( paint, r->line, r->startCol, r->endCol, 0, xStart, xEnd,
                            (cursorOn && (r->line == cursor.line)) ? cursor.col : -1, b,
                            true, myDoc->configFlags() & KateDocument::cfShowTabs,
                            KateDocument::ViewFont, again && (r->line == cursor.line));
      if ((cXPos > r->lengthPixel) && (cXPos>=xStart) && (cXPos<=xEnd)) {
        if (cursorOn && (r->line == cursor.line))
        {     
          if (b)
          {
            int cursorMaxWidth = myDoc->viewFont.myFontMetrics.width(QChar (' '));
            paint.fillRect(cXPos-xPos, 0, cursorMaxWidth, h, myDoc->myAttribs[0].col);
          } else {
            paint.fillRect(cXPos-xPos, 0, 2, h, myDoc->myAttribs[0].col);
          }
        }
      }
   }
   
   bitBlt(this, updateR.x(), (line-startLine)*h, drawBuffer, 0, 0, updateR.width(), h);

   if (r->line == cursor.line)
     again = false;

   r++;
   rpos++;
 }

 if (bm.eXPos > bm.sXPos)
   paintBracketMark();
}

void KateViewInternal::resizeEvent( QResizeEvent* )
{
  drawBuffer->resize( width(), myDoc->viewFont.fontHeight );
  leftBorder->resize( leftBorder->width(), height() );
}

void KateViewInternal::timerEvent( QTimerEvent* e )
{
  if (e->timerId() == cursorTimer) {
    cursorOn = !cursorOn;
    paintCursor();
  }
  if (e->timerId() == scrollTimer && (scrollX | scrollY)) {
    xScroll->setValue(xPos + scrollX);
    yScroll->setValue(startLine * myDoc->viewFont.fontHeight + scrollY);

    placeCursor(mouseX, mouseY);
    updateView();
  }
}

/////////////////////////////////////
// Drag and drop handlers
//

// call this to start a drag from this view
void KateViewInternal::doDrag()
{
  dragInfo.state = diDragging;
  dragInfo.dragObject = new QTextDrag(myDoc->selection(), this);
  dragInfo.dragObject->dragCopy();
}

void KateViewInternal::dragEnterEvent( QDragEnterEvent *event )
{
  event->accept( (QTextDrag::canDecode(event) && myView->doc()->isReadWrite()) || QUriDrag::canDecode(event) );
}

void KateViewInternal::dropEvent( QDropEvent *event )
{
  if ( QUriDrag::canDecode(event) ) {

      emit dropEventPass(event);

  } else if ( QTextDrag::canDecode(event) && myView->doc()->isReadWrite() ) {

    QString   text;

    if (!QTextDrag::decode(event, text))
      return;
    
    // is the source our own document?
    bool priv = myDoc->ownedView((KateView*)(event->source()));
    // dropped on a text selection area?
    bool selected = isTargetSelected(event->pos().x(), event->pos().y());

    if (priv && selected) {
      // this is a drag that we started and dropped on our selection
      // ignore this case
      return;
    }

    VConfig c;
    getVConfig(c);
    KateTextCursor cursor = c.cursor;

    if (priv) {
      // this is one of mine (this document), not dropped on the selection
      if (event->action() == QDropEvent::Move) {
        myDoc->removeSelectedText();
        getVConfig(c);
        cursor = c.cursor;
      }
      placeCursor(event->pos().x(), event->pos().y());
      getVConfig(c);
      cursor = c.cursor;
    } else {
      // this did not come from this document
      if (! selected) {
        placeCursor(event->pos().x(), event->pos().y());
        getVConfig(c);
        cursor = c.cursor;
      }
    }
    myDoc->insertText(c.cursor.line, c.cursor.col, text);

    cursor = c.cursor;
    updateCursor(cursor);

    updateView();
  }
}

void KateViewInternal::clear()
{
  cursor.col        = cursor.line        = 0;
  displayCursor.col = displayCursor.line = 0;
}
