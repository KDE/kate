/* This file is part of the KDE libraries
   Copyright (C) 2002 John Firebaugh <jfirebaugh@kde.org>
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2002 Christoph Cullmann <cullmann@kde.org> 
   Copyright (C) 2002 Hamish Rodda <meddie@yoyo.its.monash.edu.au>
   
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
#include <qscrollbar.h>

KateViewInternal::KateViewInternal(KateView *view, KateDocument *doc)
  : QWidget (view, "", Qt::WStaticContents | Qt::WRepaintNoErase | Qt::WResizeNoErase )    
  , m_view (view)
  , m_doc (doc)
  , m_currentRange(0)
  , m_preserveMaxX(false)
  , m_currentMaxX(0)
  , m_dragScrollTimer(this)
  , m_suppressColumnScrollBar(false)
{
  setMinimumSize (0,0);

  //
  // scrollbar for lines
  //
  m_lineScrollWidget = new QWidget (m_view);
  m_lineScrollWidget->setFixedWidth (style().scrollBarExtent().width());
  m_lineScrollWidget->hide ();
  QGridLayout *grid = new QGridLayout( m_lineScrollWidget, 2, 1 );
  m_view->m_grid->addMultiCellWidget (m_lineScrollWidget, 0, 1, 2, 2);
  
  m_lineScroll = new QScrollBar(QScrollBar::Vertical,m_lineScrollWidget);
  m_lineScroll->hide ();
  m_lineScroll->setTracking (true);
  grid->addWidget (m_lineScroll, 0, 0);
  
  QWidget *dummy = new QWidget (m_lineScrollWidget);
  dummy->setFixedSize( style().scrollBarExtent().width(),
                                style().scrollBarExtent().width() );
  grid->addWidget (dummy, 1, 0);
  dummy->show();
  
  m_startLine = 0;
  m_oldStartLine = 0;
  
  connect( m_lineScroll, SIGNAL( valueChanged (int) ),
           this, SLOT( scrollLines (int) ) );
  
  //
  // scrollbar for columns
  //
  m_columnScroll = new QScrollBar(QScrollBar::Horizontal,m_view);
  m_columnScroll->hide ();
  m_columnScroll->setTracking (true);
  m_view->m_grid->addMultiCellWidget (m_columnScroll, 1, 1, 0, 1);
  m_startX = 0;
  m_oldStartX = 0;
  
  connect( m_columnScroll, SIGNAL( valueChanged (int) ),
           this, SLOT( scrollColumns (int) ) );
                  
  //                         
  // iconborder ;)
  //
  leftBorder = new KateIconBorder( this, m_view );
  m_view->m_grid->addWidget (leftBorder, 0, 0);
  leftBorder->show ();

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

  possibleTripleClick = false;
  
  setAcceptDrops( true );
  setBackgroundMode( NoBackground );
  
  setCursor( KCursor::ibeamCursor() );
  KCursor::setAutoHideCursor( this, true, true );

  dragInfo.state = diNone;

  installEventFilter(this);
  
#ifndef QT_NO_DRAGANDDROP
  // Drag & scroll
  connect( &m_dragScrollTimer, SIGNAL( timeout() ),
             this, SLOT( doDragScroll() ) );
#endif
  
  updateView ();
}

KateViewInternal::~KateViewInternal ()
{
}

uint KateViewInternal::endLine () const
{
  int lines = (height() / m_doc->viewFont.fontHeight) - 1;
  if (lines < 0)
    lines = 0;

   //kdDebug() << k_funcinfo << lines << " -> " << lineRanges[lines].visibleLine - lineRanges[0].visibleLine << "(count " << lineRanges.count() << ")" << endl;
   
  if (m_view->dynWordWrap())
    lines = lineRanges[lines].visibleLine - lineRanges[0].visibleLine;

  if ((startLine() + lines) >= m_doc->visibleLines())
    return m_doc->visibleLines() - 1;
    
  return startLine() + lines;
}

/**
 * Line is the real line number to scroll to.
 */
void KateViewInternal::scrollLines ( int line )
{
  if (line == m_startLine)
    return;

  if (line < 0)
    line = 0;
    
  if (line >= (int)m_doc->visibleLines() - (height() / m_doc->viewFont.fontHeight)) {
    if (m_view->dynWordWrap()) {
      // Word-wrap can cause some lines to be below this limit.  Calculate the new limit...
      KateTextCursor c(m_doc->numVisLines(), 0);
      int temp = m_doc->numVisLines() - (uint)(-linesForOffset(c, -linesDisplayed()));
      
      if (false) {//temp < line) {
        kdDebug() << "Adjusting incorrect assumption from " << line << " to " << temp << endl;
        line = temp;
      }
      
    } else {
      line = m_doc->visibleLines() - (height() / m_doc->viewFont.fontHeight) - 1;
    }
  }
        
  int dx = m_startLine - line;
  m_oldStartLine = m_startLine;
  m_startLine = line;

  updateView();

  if (!m_view->dynWordWrap())
  {
    if (QABS(dx * m_doc->viewFont.fontHeight) < (m_view->height()-style().scrollBarExtent().width()))
    {
      //
      // trick to avoid artefact of xScrollbar ;)
      //
      
      scroll(0, dx * m_doc->viewFont.fontHeight, QRect (0,0,m_view->width(), m_view->height()-style().scrollBarExtent().width()));
      update (0, m_view->height()-style().scrollBarExtent().width(), m_view->width(), style().scrollBarExtent().width());
      
      leftBorder->scroll(0, dx * m_doc->viewFont.fontHeight, QRect (0,0,leftBorder->width(), m_view->height()-style().scrollBarExtent().width()));
      leftBorder->update (0, m_view->height()-style().scrollBarExtent().width(), leftBorder->width(), style().scrollBarExtent().width());
    }
    else
    {
      update();
      leftBorder->update ();
    }
  }
  else
  {
    update ();
    leftBorder->update ();
  }
}

void KateViewInternal::scrollColumns ( int x )
{
  if (x == m_startX)
    return;
    
  if (x < 0)
    x = 0;

  int dx = m_startX - x;
  m_oldStartX = m_startX;
  m_startX = x;

  if (QABS(dx) < width())
    scroll(dx, 0);
  else
    update();
    
  m_columnScroll->blockSignals(true);
  m_columnScroll->setValue(m_startX);
  m_columnScroll->blockSignals(false);
}

void KateViewInternal::updateView(bool changed)
{
  int maxLen = 0;
  
  int scrollbarWidth = style().scrollBarExtent().width();

  int w = m_view->width();
  int h = m_view->height();
  
  uint contentLines = m_doc->visibleLines();
  
  m_lineScroll->blockSignals(true);
  m_lineScroll->setRange(0,contentLines - (height() / m_doc->viewFont.fontHeight));
  m_lineScroll->setValue(m_startLine);
  m_lineScroll->setSteps(1, height() / m_doc->viewFont.fontHeight);
  m_lineScroll->blockSignals(false);
  m_lineScrollWidget->show();
  m_lineScroll->show();
  
  w -= leftBorder->width() + scrollbarWidth;
      
  uint oldSize = lineRanges.size ();
  lineRanges.resize ((m_view->height() / m_doc->viewFont.fontHeight) + 1);
  
  if (oldSize < lineRanges.size ())
  {
    for (uint i=oldSize; i < lineRanges.size(); i++)
      lineRanges[i].dirty = true;
  }
  
  if (m_view->dynWordWrap())
  {
    uint line = startLine ();
    int startCol = 0;
    int startX = 0, endX = 0;
    bool wrap = false;
  
    for (uint z=0; z < lineRanges.size(); z++)
    {
      if (line >= contentLines)
      {
        if (lineRanges[z].line != -1)
          lineRanges[z].dirty = true;        
      
        lineRanges[z].line = -1;
        lineRanges[z].visibleLine = -1;
        lineRanges[z].startCol = -1;
        lineRanges[z].endCol = -1;
        lineRanges[z].startX = -1;
        lineRanges[z].endX = -1;
        
        line++;
      }
      else
      {
        if (lineRanges[z].line != (int)m_doc->getRealLine (line))
          lineRanges[z].dirty = true;    
      
      lineRanges[z].visibleLine = line;
      lineRanges[z].line = m_doc->getRealLine (line);
   
      int tempEndX = 0;
      int endCol = m_doc->textWidth ( m_doc->kateTextLine (m_doc->getRealLine (line)),
                                  startCol, (uint) w, (uint)0, KateDocument::ViewFont, &wrap, &tempEndX);
      
      endX += tempEndX;
                                                           
      if (wrap)
      {
        if ((lineRanges[z].startCol != startCol) || (lineRanges[z].endCol != endCol))
          lineRanges[z].dirty = true;
      
        lineRanges[z].startCol = startCol;
        lineRanges[z].endCol = endCol;
        lineRanges[z].startX = startX;
        lineRanges[z].endX = endX;
        
        startCol = endCol;
        startX = endX;
      }
      else
      {
        if ((lineRanges[z].startCol != startCol) || (lineRanges[z].endCol != -1))
          lineRanges[z].dirty = true;
      
        lineRanges[z].startCol = startCol;
        lineRanges[z].endCol = -1;
        lineRanges[z].startX = startX;
        lineRanges[z].endX = -1;
        lineRanges[z].dirty = true;
        line++;
        startCol = 0;
        startX = 0;
        endX = 0;
      }
      
      }
    }
    
    if (m_columnScroll->isVisible ())
    {
      m_columnScroll->hide();
    }
  }
  else
  {  
    uint last = 0;
    
    for( uint line = startLine(); (line < contentLines) && ((line-startLine()) < lineRanges.size()); line++ )
    {
      if (lineRanges[line-startLine()].line != (int)m_doc->getRealLine (line))
          lineRanges[line-startLine()].dirty = true;        
    
      lineRanges[line-startLine()].line = m_doc->getRealLine( line );
      lineRanges[line-startLine()].visibleLine = line;
      lineRanges[line-startLine()].startCol = 0;
      lineRanges[line-startLine()].endCol = -1;
      lineRanges[line-startLine()].startX = 0;
      lineRanges[line-startLine()].endX = -1;
      
      maxLen = QMAX( maxLen, (int)m_doc->textWidth( m_doc->kateTextLine( lineRanges[line-startLine()].line ), -1 ) );
    
      last = line-startLine();
    }
    
    for (uint z = last+1; z < lineRanges.size(); z++)
    {
      if (lineRanges[z].line != -1)
          lineRanges[z].dirty = true;        
    
      lineRanges[z].line = -1;
      lineRanges[z].visibleLine = -1;
      lineRanges[z].startCol = -1;
      lineRanges[z].endCol = -1;
      lineRanges[z].startX = -1;
      lineRanges[z].endX = -1;
    }

    // Nice bit of extra space
    maxLen += 8;
    
    if (maxLen > w)
    {
      m_columnScroll->blockSignals(true);
      m_columnScroll->setRange(0,maxLen - w);
      m_columnScroll->setValue(m_startX);
      // Approximate linescroll
      m_columnScroll->setSteps(m_doc->viewFont.width('a', false, false), w);
      m_columnScroll->blockSignals(false);
      
      if (!m_columnScroll->isVisible ()  && !m_suppressColumnScrollBar)
      {
        m_columnScroll->show();
      }
  
      h -= scrollbarWidth;
    }
    else if (m_columnScroll->isVisible () && !m_suppressColumnScrollBar)
    {
      m_columnScroll->hide();
    }
  }
  
  if (changed)
    paintText (0,0,width(), height(), true);
}

void KateViewInternal::paintText (int x, int y, int width, int height, bool paintOnlyDirty)
{
  int xStart = startX() + x;
  int xEnd = xStart + width;
  uint h = m_doc->viewFont.fontHeight;
  uint startz = (y / h);
  uint endz = startz + 1 + (height / h);
  uint lineRangesSize = lineRanges.size();
  
  QPainter paint ( this );
  
  for (uint z=startz; z <= endz; z++)
  {
    if ( (z >= lineRangesSize) || ((lineRanges[z].line == -1) && (!paintOnlyDirty || lineRanges[z].dirty)) )
    {
      if (!(z >= lineRangesSize))
        lineRanges[z].dirty = false;
    
      paint.fillRect( x, z * h, width, h, m_doc->colors[0] );
    }
    else if (!paintOnlyDirty || lineRanges[z].dirty)
    {
      lineRanges[z].dirty = false;
    
      m_doc->paintTextLine
           ( paint,
             lineRanges[z].line,
             lineRanges[z].startCol,
             lineRanges[z].endCol,
             x,
             z * h,
             xStart,
             xEnd,
             ( ( cursorOn && ( hasFocus() || m_view->m_codeCompletion->codeCompletionVisible() ) && ( lineRanges[z].line == cursor.line ) && ( cursor.col >= lineRanges[z].startCol ) && ( ( lineRanges[z].endCol < 0 ) || ( cursor.col < lineRanges[z].endCol ) ) ) ? cursor.col : -1 ),
             m_view->isOverwriteMode(),
             cXPos,
             true,
             ( m_doc->configFlags() & KateDocument::cfShowTabs ),
             KateDocument::ViewFont,
             ( lineRanges[z].line == cursor.line ),
             false,
             bm,
             lineRanges[z].startX + m_startX );
    }
  }
}

/**
 * this function ensures a certain location is visible on the screen.
 */
void KateViewInternal::makeVisible (uint line, uint startCol, uint endCol)
{
  bool scrollbarHidden = m_columnScroll->isHidden();
  kdDebug() << "MakeVisible end: " << endLine() << " start: " << startLine() << " request: " << line << " lines " << height() / m_doc->viewFont.fontHeight <<endl;
  if ( line > endLine() )
  {
    int delta = 0;
    
    if (!m_view->dynWordWrap()) {
      delta = endLine() - startLine();
    
    } else {
      int offset = - (height() / m_doc->viewFont.fontHeight);
      //if (line == endLine() + 1)
        offset += 1;
/*      else
        offset -= 1;*/
      
      KateTextCursor c(line, startCol);
      delta = -linesForOffset(c, offset);
    }
    
    int linesToScroll = line;
    int startingLine = startLine();
    
    if (linesToScroll - delta >= 0)
      linesToScroll -= delta;
    
    kdDebug() << "Delta: " << delta << ", linesToScroll: " << linesToScroll << endl;
  
    scrollLines(linesToScroll);
    
    if (scrollbarHidden && !m_columnScroll->isHidden()) {
      // Scrollbar has appeared, might need to scroll back a line as the cursor is hidden
      if (line <= endLine()) {
        scrollLines(linesToScroll + 1);
      }
    }

  }
  else if ( line < startLine() )
  {
    scrollLines (line);
  }
  
  
  if (!m_view->dynWordWrap())
  {
    int sX = (int)m_doc->textWidth (m_doc->kateTextLine( m_doc->getRealLine( line ) ), startCol );
    int eX = (int)m_doc->textWidth (m_doc->kateTextLine( m_doc->getRealLine( line ) ), endCol );
  
    int sXborder = sX-8;
    if (sXborder < 0)
      sXborder = 0;
  
    if (sX < m_startX)
      scrollColumns (sXborder);
    else if  (sX > m_startX + width())
      scrollColumns (sX - width() + 8);
    
  /*else
    if (eX > m_startX + width())
      m_columnScroll->setValue (eX + 8);*/
  }
}

void KateViewInternal::slotRegionVisibilityChangedAt(unsigned int)
{
  kdDebug(13030) << "slotRegionVisibilityChangedAt()" << endl;
  updateView();
  update();
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

void KateViewInternal::showEvent ( QShowEvent *e )
{
  updateView ();
  
  QWidget::showEvent (e);
}

uint KateViewInternal::linesDisplayed() const
{
  int h = height();
  int fh = m_doc->viewFont.fontHeight;
  int lines = (h % fh) == 0 ? h / fh : h / fh + 1;
  
  return lines;
}

QPoint KateViewInternal::cursorCoordinates()
{
   return QPoint( cXPos, lineToY( displayCursor.line ) );
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

void KateViewInternal::findCurrentRange()
{
  // only use this function if we're dynamic word-wrapping...!
  // FIXME Possible speedup from using the result of oldDisplayCursor.line - displayCursor.line ?
  // Probably not, as would need another assumption...
  Q_ASSERT(m_view->dynWordWrap());
  enum {
    None,
    Forwards,
    Backwards
  } direction = None;
  
  if (m_currentRange >= (int)lineRanges.count()) {
    if (m_currentRange > INT_MAX / 2)
      m_currentRange = 0;
    else
      m_currentRange = lineRanges.count() - 1;
  }
  
  // Make sure the cursor is valid
  Q_ASSERT(displayCursor.col >= 0);
  if (displayCursor.col < 0) displayCursor.col = 0;
  
  Q_ASSERT(displayCursor.col <= m_doc->lineLength(displayCursor.line));
  if (displayCursor.col > m_doc->lineLength(displayCursor.line)) displayCursor.col = m_doc->lineLength(displayCursor.line);

  while (m_currentRange < (int)lineRanges.count()) {    
    LineRange& thisRange = lineRanges[m_currentRange];
    if (displayCursor.line == thisRange.line) {
      if (displayCursor.col >= thisRange.startCol) {
        if (thisRange.endCol == -1 || displayCursor.col < thisRange.endCol) {
          // found it...
          return;
        } else {
          Q_ASSERT(direction == Forwards || direction == None);
          direction = Forwards;
        }
      } else {
        Q_ASSERT(direction == Backwards || direction == None);
        direction = Backwards;
      }
    } else if (displayCursor.line > thisRange.line) {
      Q_ASSERT(direction == Forwards || direction == None);
      direction = Forwards;
    } else {
      Q_ASSERT(direction == Backwards || direction == None);
      direction = Backwards;
    }
    
    switch (direction) {
      case Forwards:
        m_currentRange++;
        break;
      case Backwards:
        m_currentRange--;
        break;
      case None:
      default:
        Q_ASSERT(false);
        return;
    }
  }
  
  m_currentRange = -1;
  return;
}

LineRange KateViewInternal::getRange(int visibleLine, int startCol, int startX, int width)
{
  //kdDebug() << k_funcinfo << " startCol " << startCol << " startX " << startX << endl;
  int realLine = 0;
  
  if (visibleLine < 0) {
    findCurrentRange();
    if (m_currentRange + 1 + visibleLine >= 0)
      return lineRanges[m_currentRange + 1 + visibleLine];
    
    realLine = displayCursor.line + 1 + visibleLine;
  } else {
    realLine = m_doc->getRealLine(visibleLine);
  }
  
  // look at the cache first
  if (realLine >= lineRanges[0].line && realLine <= lineRanges[lineRanges.count() - 1].line)
    for(uint i = 0; i < lineRanges.count(); i++)
      if (realLine == lineRanges[i].line)
        if (lineRanges[i].startCol == startCol)
          return lineRanges[i];
  
  if (width == -1) width = m_view->width() - (leftBorder->width() + style().scrollBarExtent().width());
  
  int endX = 0;
  bool wrap = false;
    
  LineRange ret;
  
  ret.endCol = (int)m_doc->textWidth (m_doc->kateTextLine(realLine), startCol, (uint)width, (uint)0, KateDocument::ViewFont, &wrap, &endX);

  ret.line = realLine;
  ret.visibleLine = visibleLine;
  ret.startCol = startCol;
  if (!wrap) ret.endCol = -1;
  ret.startX = startX;
  ret.endX = wrap ? startX + endX : -1;
  
  return ret;
}

/**
 * This returns the visible line upon which c is situated.
 */
int KateViewInternal::viewLine(const KateTextCursor& c)
{
  kdDebug() << k_funcinfo << " requesting [" << c.line << "," << c.col << "]" << endl;
  
  Q_ASSERT(m_view->dynWordWrap());
  
  // Check in the official cache
  if (c.line >= (int)startLine() && c.line < (int)(lineRanges.count() + startLine())) {
    for (uint i = 0; i < lineRanges.count(); i++) {
      if (c.line == lineRanges[i].line && c.col >= lineRanges[i].startCol && (lineRanges[i].endCol == -1 || c.col < lineRanges[i].endCol)) {
        kdDebug() << "Returning " << i << endl;
        return i;
      }
    }
  }
   
  // Ok, we have to do the hard slog...
  // FIXME optimise - somehow combine with updateView routine?
  enum {
    Forwards,
    Backwards
  } direction;
  
  direction = c.line < (int)startLine() ? Backwards : Forwards;
  //kdDebug() << "Direction: line " << c.line << " < startline " << startLine() << " != forwards? " << (direction == Forwards) << endl;
  
  int currentViewLine = 0;
  int line = 0;
  
  if (direction == Forwards) {
    currentViewLine = lineRanges.count() - 1;
    line = lineRanges[lineRanges.count() - 1].line;
  } else {
    currentViewLine = 0;
    line = startLine();
  }
   
  kdDebug() << "starting currentViewLine is " << currentViewLine << ", starting line is " << line << ", going forwards? " << (direction==Forwards) << endl;
  
  uint width = m_view->width() - (leftBorder->width() + style().scrollBarExtent().width());
  
  while (line >= 0 && line < (int)m_doc->numLines())
  {
    //kdDebug() << "hard slog... line: " << line << ", currentViewLine: " << currentViewLine << endl;
    int found = 0;
    int visibleLine = 0;
    LineRange range;
    
    do {
      range = getRange(line, visibleLine ? range.endCol : 0, visibleLine ? range.endX :0, width);
      
      visibleLine++;
      
      //kdDebug() << "iteration startCol " << range.startCol << ", endCol " << range.endCol << ", currentViewLine: " << currentViewLine << endl;
          
      if (c.line == range.line && c.col >= range.startCol && (range.endCol == -1 || c.col < range.endCol)) {
        // This is it.
        found = visibleLine;
        if (direction == Forwards) {
          //currentViewLine += visibleLine;
          kdDebug() << "8 Returning " << currentViewLine << endl;
          return currentViewLine;
        }
      }
      
    } while (range.endCol != -1);
    
    if (found) {
      currentViewLine += visibleLine - found;
      kdDebug() << k_funcinfo << "9 Returning " << currentViewLine << endl;
      return currentViewLine;
    }
    
    if (direction == Forwards) {
      currentViewLine += visibleLine;
      line++;
    } else {
      currentViewLine -= visibleLine;
      line--;
    }
    
    line = m_doc->getRealLine(line);
  }
  
  // Can't believe we still couldn't find it!!!
  Q_ASSERT(false);
  return 0;
}

/*
 * This returns the number of visible lines required to move to reach
 * as close to the virtual line offset required without going beyond.
 * For word-wrapping only. A "virtual line" is defined as one line of a complete
 * ("visible") line. (Visible would be a better name for it, but that's taken by
 * the folding nomenclature.
 *
 */
int KateViewInternal::linesForOffset(const KateTextCursor& c, int offset)
{ 
  kdDebug() << k_funcinfo << " requesting offset " << offset << " from line " << c.line << endl;
  
  Q_ASSERT(m_view->dynWordWrap());
 

  /*int visOffset = visLine + offset;
  
  kdDebug() << "Cursor [" << c.line << "," << c.col << "] -> virtual line " << visLine << " + offset " << offset << " = " << visOffset << endl;
  
  // First check in the official cache
  if (visOffset >= 0 && visOffset < (int)lineRanges.count()) {
    int offsetLine = lineRanges[visOffset].line;
    if (offset < 0) {
      //LineRange range = getRange(offsetLine);
      kdDebug() << "Checking range to make sure it's the start of a line: startCol " << lineRanges[visOffset].startCol << endl;
      if (lineRanges[visOffset].startCol != 0) {
        offsetLine++;
        kdDebug() << "Incrementing result." << endl;
      }
    }
    kdDebug() << "0 Returning (line " << offsetLine << ") offset " << offsetLine - c.line << endl;
    return offsetLine - c.line;
  }*/
  
  // Ok, we have to do the hard slog...
  // FIXME optimise - somehow combine with updateView routine?
  enum {
    Forwards,
    Backwards
  } direction;
  
  direction = offset > 0 ? Forwards : Backwards;
  
  int currentOffset = 0;
  int line = 0;
  
  if (direction == Forwards) {
    currentOffset = 0;//lineRanges.count() - 1 - visLine;
    line = m_doc->getVirtualLine(c.line);//lineRanges.count() - 1;
    if (c.col != 0) {
      KateTextCursor c2(c.line, 0);
      currentOffset /*int visLine */ = viewLine(c) - viewLine(c2);
      kdDebug() << "Altered currentOffset to " << currentOffset << endl;
      if (offset >= currentOffset) {
        return offset;
      }
    }
/*    while (lineRanges[line].startCol != 0) {
      line--;
      currentOffset--;
    }
    line = lineRanges[line].visibleLine;*/
  } else {
    currentOffset = 0;
    if (c.col != 0) {
      KateTextCursor c2(c.line, 0);
      currentOffset /*int visLine */ = viewLine(c) - viewLine(c2);
      kdDebug() << "Altered currentOffset to " << currentOffset << endl;
      if (offset >= currentOffset) {
        return offset;
      }
    }
    //currentOffset = virtualLineOffset;//-visLine;
    line = m_doc->getVirtualLine(c.line);//startLine();
  }
   
  kdDebug() << "starting currentOffset is " << currentOffset << ", starting line is " << line << ", forwards? " << (direction == Forwards) << endl;
  
  int width = m_view->width() - (leftBorder->width() + style().scrollBarExtent().width());
  
  while (line >= 0 && line < (int)m_doc->numVisLines())
  {
    //kdDebug() << "slog: line: " << line << ", currentOffset: " << currentOffset << endl;
    LineRange range;
    bool first = true;
        
    do {
      range = getRange(line, first ? 0 : range.endCol, first ? 0 : range.endX, width);
      first = false;
      
      //kdDebug() << "Iteration startCol " << range.startCol << " endCol " << range.endCol << endl;
      
      if (direction == Forwards)
        currentOffset++;
      else
        currentOffset--;

    } while (range.endCol != -1);
    
    if ((direction == Forwards && currentOffset == offset)) {
      kdDebug() << "1 Reached visible offset " << currentOffset << ", returning real offset " << (line + 1 - c.line) << endl;
      return (line + 1) - c.line;
    } else if ((direction == Forwards && currentOffset > offset)) {
      kdDebug() << "2 Reached visible offset " << currentOffset << ", returning real offset " << (line - c.line) << endl;
      return line - c.line;
    } else if (direction == Backwards && currentOffset == offset) {
      kdDebug() << "3 Reached visible offset " << currentOffset << ", returning real offset " << (line - 1 - c.line) << endl;
      return (line - 1) - c.line;
    } else if (direction == Backwards && currentOffset < offset) {
      kdDebug() << "4 Reached visible offset " << currentOffset << ", returning real offset " << (line - c.line) << endl;
      return line - c.line;
    }
    
    if (direction == Forwards)
      line++;
    else
      line--;
  }
  
  // Can't believe we still couldn't find it!!!
  // Return the max/min valid line.
  if (direction == Forwards)
    return m_doc->numVisLines() - c.line;
  else
    return 0 - c.line;
}

void KateViewInternal::cursorUp(bool sel)
{
  if( displayCursor.line == 0 )
    return;
    
  int newLine = displayCursor.line, newCol = 0, xOffset = 0, startCol = 0;
  m_preserveMaxX = !sel;

  if (m_view->dynWordWrap()) {
    // Dynamic word wrapping - navigate on visual lines rather than real lines
    LineRange thisRange = getRange();
    
    // Ensure we're in the right spot
    Q_ASSERT((displayCursor.line == thisRange.line) &&
             (displayCursor.col >= thisRange.startCol) &&
             (thisRange.endCol == -1 || displayCursor.col < thisRange.endCol));
    
    KateTextCursor cursorVisibleX = displayCursor;
    int visibleX = m_doc->textWidth(cursorVisibleX) - thisRange.startX;
    
    //kdDebug() << "thisrange line " << thisRange.line << " startcol " << thisRange.startCol << " endcol " << thisRange.endCol << endl;
    //kdDebug() << "thisrange startX " << thisRange.startX << " endX " << thisRange.endX << " visibleX " << visibleX << " m_currentMaxX " << m_currentMaxX << endl;
    
    // This is not the first line because that is already simplified out above
    
/*    if (m_currentRange == 0) {
      // eek :( we actually have to do some work...
      newLine = m_doc->getRealLine(newLine - 1);
      uint width = m_view->width() - (leftBorder->width() + style().scrollBarExtent().width());
      bool wrap = true;
      int endX = 0, endCol = 0;
      while (wrap)
      {
        // Remember the _last_ settings
        startCol = endCol;
        xOffset += endX;
        // Perform the lookup
        endCol = m_doc->textWidth(m_doc->kateTextLine(newLine), startCol, width, (uint)0, KateDocument::ViewFont, &wrap, &endX);
      }
      // We're out here when the last visible line was found; the variables are right...
      
    } else {*/
      LineRange& previousRange = getRange(-2);//lineRanges.at(m_currentRange - 1);

      startCol = previousRange.startCol;
      xOffset = previousRange.startX;
      newLine = previousRange.line;
    //}
    
    if (!sel && m_currentMaxX > visibleX)
      visibleX = m_currentMaxX;
    
    cXPos = xOffset + visibleX;
    
    newCol = m_doc->textPos(newLine, visibleX, KateDocument::ViewFont, startCol);
  
  } else {
    newLine = m_doc->getRealLine(displayCursor.line - 1);
    
    if (!sel && m_currentMaxX > cXPos)
      cXPos = m_currentMaxX;
  }
  
  KateTextCursor c(newLine, newCol);
  m_doc->textWidth(c, cXPos);

  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::cursorDown(bool sel)
{
  if( displayCursor.line >= (int)m_doc->lastLine() )
    return;

  int newLine = displayCursor.line, newCol = 0, xOffset = 0, startCol = 0;
  m_preserveMaxX = !sel;

  if (m_view->dynWordWrap()) {
    // Dynamic word wrapping - navigate on visual lines rather than real lines
    LineRange thisRange = getRange();//lineRanges[m_currentRange];

    // Ensure we're in the right spot
    Q_ASSERT((displayCursor.line == thisRange.line) &&
             (displayCursor.col >= thisRange.startCol) &&
             (thisRange.endCol == -1 || displayCursor.col < thisRange.endCol));
     
    KateTextCursor cursorVisibleX = displayCursor;
    int visibleX = m_doc->textWidth(cursorVisibleX) - thisRange.startX;

    //kdDebug() << "thisrange line " << thisRange.line << " startcol " << thisRange.startCol << " endcol " << thisRange.endCol << " cXPos " << cXPos << endl;
    //kdDebug() << "thisrange startX " << thisRange.startX << " thisRange.endX " << thisRange.endX << " visibleX " << visibleX << " m_currentMaxX " << m_currentMaxX << endl;
    
    if (thisRange.endCol == -1) {
      newLine = m_doc->getRealLine(displayCursor.line + 1);
    } else {
      startCol = thisRange.endCol;
      xOffset = thisRange.endX;
    }
    
    if (!sel && m_currentMaxX > visibleX)
      visibleX = m_currentMaxX;
  
    cXPos = xOffset + visibleX;
    
    newCol = m_doc->textPos(newLine, visibleX, KateDocument::ViewFont, startCol);
    
    //kdDebug() << "result -> cXPos " << cXPos << ", newLine " << newLine << ", newCol " << newCol << endl;
  
  } else {
    newLine = m_doc->getRealLine(displayCursor.line + 1);
    
    if (!sel && m_currentMaxX > cXPos)
      cXPos = m_currentMaxX;
  }
  
  KateTextCursor c(newLine, newCol);
  m_doc->textWidth(c, cXPos);
  
  //kdDebug() << "adjusted to " << c.line << "," << c.col << ", cXPos " << cXPos << endl;
  
  updateSelection(c, sel);
  updateCursor(c);
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
  WrappingCursor c( *m_doc, m_doc->getRealLine( startLine() ), 0 );
  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::bottomOfView( bool sel )
{
  WrappingCursor c( *m_doc, m_doc->getRealLine( endLine() ), 0 );
  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::scrollLines( int lines, bool sel )
{
  //scrollBy( 0, lines * m_doc->viewFont.fontHeight );
  
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
  int linesToScroll = -QMAX( linesDisplayed() - 1, 0 );
  if (m_view->dynWordWrap())
    linesToScroll = linesForOffset(displayCursor, linesToScroll);
  
  scrollLines( linesToScroll, sel );
}

void KateViewInternal::pageDown( bool sel )
{
  int linesToScroll = QMAX( linesDisplayed() - 1, 0 );
  if (m_view->dynWordWrap())
    linesToScroll = linesForOffset(displayCursor, linesToScroll);

  scrollLines( linesToScroll, sel );
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
  makeVisible ( displayCursor.line, displayCursor.col, displayCursor.col );
  
//  ensureVisible( cXPos, lineToContentsY( displayCursor.line+1 ), 0, 0 );

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
  tagLines( displayCursor.line, displayCursor.line );

  QPoint cursorP = cursorCoordinates();
  setMicroFocusHint( cursorP.x(), cursorP.y(), 0, m_doc->viewFont.fontHeight );
  
  if (cursorTimer) {
    killTimer(cursorTimer);
    cursorTimer = startTimer( KApplication::cursorFlashTime() / 2 );
    cursorOn = true;
  }
  
  // Remember the largest real & visible column number
  if (m_view->dynWordWrap()) {
    // Update the current range... first just a quick optimisation
    if (oldDisplayCursor.line == displayCursor.line - 1)
      m_currentRange--;
    else if (oldDisplayCursor.line == displayCursor.line - 1)
      m_currentRange++;
    
    // Now confirm we're in the right spot
    findCurrentRange();

    // Remember the maximum X position if requested
    if (m_preserveMaxX)
      m_preserveMaxX = false;
    else
      m_currentMaxX = m_doc->textWidth(displayCursor) - getRange().startX;
  } else {
    // Remember the maximum X position if requested
    if (m_preserveMaxX)
      m_preserveMaxX = false;
    else
      m_currentMaxX = cXPos;
  }
  //kdDebug() << "m_currentMaxX: " << m_currentMaxX << ", cXPos: " << cXPos << endl;
  
  paintText (0,0,width(), height(), true);
  
  emit m_view->cursorPositionChanged();
}

void KateViewInternal::tagLines( int start, int end, bool realLines )
{
  if (realLines)
  {
    start = m_doc->getVirtualLine( start );
    end = m_doc->getVirtualLine( end );
  }

  if (end < (int)startLine())
    return;
    
  if (start > (int)endLine())
    return;
  
  //kdDebug(13030) << "tagLines( " << start << ", " << end << " )\n";
  
  for (uint z = 0; z < lineRanges.size(); z++)
  {
    if ((lineRanges[z].visibleLine >= start) && (lineRanges[z].visibleLine <= end))
      lineRanges[z].dirty = true;
  }
  
  if (!m_view->dynWordWrap())
  {
    int y = lineToY( start );
    int h = (end - start + 1) * m_doc->viewFont.fontHeight;
 
  //  update ( 0, y, width(), h );
    leftBorder->update (0, y, leftBorder->width(), h);
  }
  else
  {
    for (uint z = 0; z < lineRanges.size(); z++)
    {
      if ((lineRanges[z].visibleLine >= start) && (lineRanges[z].visibleLine <= end))
      {
      //  update ( 0, z * m_doc->viewFont.fontHeight, width(), m_doc->viewFont.fontHeight );
        leftBorder->update (0, z * m_doc->viewFont.fontHeight, leftBorder->width(), m_doc->viewFont.fontHeight);
      }
    }
  }
}

void KateViewInternal::tagAll()
{
  //kdDebug(13030) << "tagAll()" << endl;
  for (uint z = 0; z < lineRanges.size(); z++)
  {
      lineRanges[z].dirty = true;;
  }
  
  leftBorder->updateFont();
  leftBorder->update ();
}

void KateViewInternal::centerCursor()
{
 // center( cXPos, lineToContentsY( displayCursor.line ) );
}

void KateViewInternal::paintCursor()
{
  tagLines( displayCursor.line, displayCursor.line );
  paintText (0,0,width(), height(), true);
}

// Point in content coordinates
void KateViewInternal::placeCursor( const QPoint& p, bool keepSelection, bool updateSelection )
{
  int line = lineRanges[p.y() / m_doc->viewFont.fontHeight].visibleLine;
  uint startCol = lineRanges[p.y() / m_doc->viewFont.fontHeight].startCol;

  line = QMAX( 0, QMIN( line, int(m_doc->numVisLines()) - 1 ) );

  KateTextCursor c;
  c.line = m_doc->getRealLine( line );
  m_doc->textWidth( c, startX() + p.x(),  KateDocument::ViewFont, startCol);

  if (updateSelection)
    KateViewInternal::updateSelection( c, keepSelection );
  updateCursor( c );
}

// Point in content coordinates
bool KateViewInternal::isTargetSelected( const QPoint& p )
{
  int line = lineRanges[p.y() / m_doc->viewFont.fontHeight].visibleLine;
  uint startCol = lineRanges[p.y() / m_doc->viewFont.fontHeight].startCol;
 
  TextLine::Ptr textLine = m_doc->kateTextLine( m_doc->getRealLine( line ) );
  if( !textLine )
    return false;

  int col = m_doc->textPos( textLine, p.x(), KateDocument::ViewFont, startCol );

  return m_doc->lineColSelected( line, col );
}

//
// START EVENT HANDLING STUFF !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//

bool KateViewInternal::eventFilter( QObject *obj, QEvent *e )
{
  //KCursor::autoHideEventFilter( obj, e );
  
  switch( e->type() ) {
  case QEvent::KeyPress: {
    QKeyEvent *k = (QKeyEvent *)e;
    if ( !(k->state() & ControlButton || k->state() & AltButton) ) {
      keyPressEvent( k );
      return k->isAccepted();
    }
  } break;
#ifndef QT_NO_DRAGANDDROP
  case QEvent::DragMove:
    {
      QPoint currentPoint = ((QDragMoveEvent*) e)->pos();
      QRect doNotScrollRegion( scrollMargin, scrollMargin,
                          width() - scrollMargin * 2,
                          height() - scrollMargin * 2 );
      if ( !doNotScrollRegion.contains( currentPoint ) ) {
          startDragScroll();
          // Keep sending move events
          ( (QDragMoveEvent*)e )->accept( QRect(0,0,0,0) );
      }
      dragMoveEvent((QDragMoveEvent*)e);
    }
    break;
  case QEvent::DragLeave:
    stopDragScroll();
    break;
#endif
  default:
    break;
  }
        
  return QWidget::eventFilter( obj, e );
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

void KateViewInternal::mousePressEvent( QMouseEvent* e )
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
      dragInfo.start = e->pos();
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
    m_view->popup()->popup( mapToGlobal( e->pos() ) );
  }
}

void KateViewInternal::mouseDoubleClickEvent(QMouseEvent *e)
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

void KateViewInternal::mouseReleaseEvent( QMouseEvent* e )
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

void KateViewInternal::mouseMoveEvent( QMouseEvent* e )
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
   
    mouseX = e->x();
    mouseY = e->y();
    
    scrollX = 0;
    scrollY = 0;
    int d = m_doc->viewFont.fontHeight;
    if (mouseX < 0) {
      scrollX = -d;
    }
    if (mouseX > width()) {
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
    placeCursor( QPoint( mouseX, mouseY ), true );
  }
}

void KateViewInternal::paintEvent(QPaintEvent *e)
{
  QRect updateR = e->rect();
  paintText (updateR.x(), updateR.y(), updateR.width(), updateR.height());
}

void KateViewInternal::resizeEvent( QResizeEvent *e)
{ 
  updateView();
  
  if (m_view->dynWordWrap())
  {
    leftBorder->update ();
    update ();
  }
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
    scrollLines (m_startLine + (scrollY / m_doc->viewFont.fontHeight));
    //scrollLines (m_startLine + (scrollY / m_doc->viewFont.fontHeight));
    
    placeCursor( QPoint( mouseX, mouseY ), true );
    kdDebug()<<"scroll timer: X: "<<mouseX<<" Y: "<<mouseY<<endl;
  }
}

void KateViewInternal::focusInEvent (QFocusEvent *)
{
  cursorTimer = startTimer( KApplication::cursorFlashTime() / 2 );
  paintCursor();
  emit m_view->gotFocus( m_view );
}

void KateViewInternal::focusOutEvent (QFocusEvent *)
{
  if( ! m_view->m_codeCompletion->codeCompletionVisible() )
  {
    if( cursorTimer )
    {
      killTimer( cursorTimer );
      cursorTimer = 0;
    }
    paintCursor();
    emit m_view->lostFocus( m_view );
  }
}

void KateViewInternal::doDrag()
{
  dragInfo.state = diDragging;
  dragInfo.dragObject = new QTextDrag(m_doc->selection(), this);
  dragInfo.dragObject->dragCopy();
}

void KateViewInternal::dragEnterEvent( QDragEnterEvent* event )
{
  event->accept( (QTextDrag::canDecode(event) && m_doc->isReadWrite()) ||
                  QUriDrag::canDecode(event) );
}

void KateViewInternal::dragMoveEvent( QDragMoveEvent* event )
{
  // track the cursor to the current drop location
  placeCursor( event->pos(), true, false );
}

void KateViewInternal::dropEvent( QDropEvent* event )
{
  if ( QUriDrag::canDecode(event) ) {

      emit dropEventPass(event);

  } else if ( QTextDrag::canDecode(event) && m_doc->isReadWrite() ) {

    QString text;

    if (!QTextDrag::decode(event, text))
      return;
    
    // is the source our own document?
    bool priv = false;
    if (event->source() && event->source()->inherits("KateViewInternal"))
      priv = m_doc->ownedView( ((KateViewInternal*)(event->source()))->m_view );
      
    // dropped on a text selection area?
    bool selected = isTargetSelected( event->pos() );

    if( priv && selected ) {
      // this is a drag that we started and dropped on our selection
      // ignore this case
      return;
    } else if( priv ) {
      // this is one of mine (this document), not dropped on the selection
      m_doc->removeSelectedText();
    }
    placeCursor( event->pos() );
    m_doc->insertText( cursor.line, cursor.col, text );

    updateView();
  }
}

//
// END EVENT HANDLING STUFF !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//

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
    if (tagLinesFrom > -1)
    {
      int startTagging = QMIN( tagLinesFrom, editTagLineStart );
      int endTagging = m_doc->getRealLine( endLine() );
      tagLines (startTagging, endTagging, true);
    }
    else
      tagLines (editTagLineStart, editTagLineEnd, true);

    tagLinesFrom = -1;
    
    updateView (true);
    
    if (cursorCacheChanged)
    {
      cursorCacheChanged = false;
      updateCursor( cursorCache );
    }
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
  if (line >= (int)m_doc->getRealLine(startLine()))
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

void KateViewInternal::wheelEvent(QWheelEvent* e)
{
  if (m_lineScroll->minValue() != m_lineScroll->maxValue()) {
    QWheelEvent copy = *e;
    QApplication::sendEvent(m_lineScroll, &copy);
  } else if (!m_columnScroll->isHidden()) {
    QWheelEvent copy = *e;
    QApplication::sendEvent(m_columnScroll, &copy);
  } else {
    e->ignore();
  }
}

void KateViewInternal::startDragScroll()
{
  if ( !m_dragScrollTimer.isActive() ) {
    m_suppressColumnScrollBar = true;
    m_dragScrollTimer.start( scrollTime );
  }
}

void KateViewInternal::stopDragScroll()
{
  m_suppressColumnScrollBar = false;
  m_dragScrollTimer.stop();
  updateView();
}

void KateViewInternal::doDragScroll()
{
  QPoint p = this->mapFromGlobal( QCursor::pos() );

  int dx = 0, dy = 0;
  if ( p.y() < scrollMargin ) {
    dy = p.y() - scrollMargin;
  } else if ( p.y() > height() - scrollMargin ) {
    dy = scrollMargin - (height() - p.y());
  }
  if ( p.x() < scrollMargin ) {
    dx = p.x() - scrollMargin;
  } else if ( p.x() > width() - scrollMargin ) {
    dx = scrollMargin - (width() - p.x());
  }
  dy /= 4;
  
  if (dy)
    scrollLines(m_startLine + dy);
  if (dx)
    scrollColumns(m_startX + dx);
  if (!dy && !dx)
    stopDragScroll();
}
