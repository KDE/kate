/* This file is part of the KDE libraries
   Copyright (C) 2001 Anders Lund <anders@alweb.dk>
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>

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

#include "kateiconborder.h"
#include "kateview.h"
#include "kateviewinternal.h"
#include "katedocument.h"
#include "kateiconborder.moc"
#include "katecodefoldinghelpers.h"

#include <kdebug.h>
#include <qpainter.h>
#include <qpopupmenu.h>
#include <qcursor.h>

const char * plus_xpm[] = {
"12 16 3 1",
"       c None",
".      c #000000",
"+      c #FFFFFF",
"      .     ",
"      .     ",
" .........  ",
" .+++++++.  ",
" .+++++++.  ",
" .+++++++.  ",
" .+++.+++.  ",
" .+++.+++.  ",
" .+.....+.  ",
" .+++.+++.  ",
" .+++.+++.  ",
" .+++++++.  ",
" .+++++++.  ",
" .........  ",
"      .     ",
"      .     "};

const  char * minus_xpm[] = {
"12 16 3 1",
"       c None",
".      c #000000",
"+      c #FFFFFF",
"      .     ",
"      .     ",
" .........  ",
" .+++++++.  ",
" .+++++++.  ",
" .+++++++.  ",
" .+++++++.  ",
" .+++++++.  ",
" .+.....+.  ",
" .+++++++.  ",
" .+++++++.  ",
" .+++++++.  ",
" .+++++++.  ",
" .........  ",
"      .     ",
"      .     "};


const char*bookmark_xpm[]={
"12 16 4 1",
"b c #808080",
"a c #000080",
"# c #0000ff",
". c None",
"............",
"............",
"........###.",
".......#...a",
"......#.##.a",
".....#.#..aa",
"....#.#...a.",
"...#.#.a.a..",
"..#.#.a.a...",
".#.#.a.a....",
"#.#.a.a.....",
"#.#a.a...bbb",
"#...a..bbb..",
".aaa.bbb....",
"............",
"............"};

const char* breakpoint_xpm[]={
"11 16 6 1",
"c c #c6c6c6",
". c None",
"# c #000000",
"d c #840000",
"a c #ffffff",
"b c #ff0000",
"...........",
"...........",
"...#####...",
"..#aaaaa#..",
".#abbbbbb#.",
"#abbbbbbbb#",
"#abcacacbd#",
"#abbbbbbbb#",
"#abcacacbd#",
"#abbbbbbbb#",
".#bbbbbbb#.",
"..#bdbdb#..",
"...#####...",
"...........",
"...........",
"..........."};

const char*breakpoint_bl_xpm[]={
"11 16 7 1",
"a c #c0c0ff",
"# c #000000",
"c c #0000c0",
"e c #0000ff",
"b c #dcdcdc",
"d c #ffffff",
". c None",
"...........",
"...........",
"...#####...",
"..#ababa#..",
".#bcccccc#.",
"#acccccccc#",
"#bcadadace#",
"#acccccccc#",
"#bcadadace#",
"#acccccccc#",
".#ccccccc#.",
"..#cecec#..",
"...#####...",
"...........",
"...........",
"..........."};

const char*breakpoint_gr_xpm[]={
"11 16 6 1",
"c c #c6c6c6",
"d c #2c2c2c",
"# c #000000",
". c None",
"a c #ffffff",
"b c #555555",
"...........",
"...........",
"...#####...",
"..#aaaaa#..",
".#abbbbbb#.",
"#abbbbbbbb#",
"#abcacacbd#",
"#abbbbbbbb#",
"#abcacacbd#",
"#abbbbbbbb#",
".#bbbbbbb#.",
"..#bdbdb#..",
"...#####...",
"...........",
"...........",
"..........."};

const char*exec_xpm[]={
"11 16 4 1",
"a c #00ff00",
"b c #000000",
". c None",
"# c #00c000",
"...........",
"...........",
"...........",
"#a.........",
"#aaa.......",
"#aaaaa.....",
"#aaaaaaa...",
"#aaaaaaaaa.",
"#aaaaaaa#b.",
"#aaaaa#b...",
"#aaa#b.....",
"#a#b.......",
"#b.........",
"...........",
"...........",
"..........."};

const int iconPaneWidth = 16;

KateIconBorder::KateIconBorder ( KateViewInternal* internalView )
  : QWidget(internalView, "", Qt::WStaticContents | Qt::WRepaintNoErase | Qt::WResizeNoErase )
  , m_view( internalView->m_view )
  , m_doc( internalView->m_doc )
  , m_viewInternal( internalView )
  , m_markMenu(0)
  , m_iconBorderOn( false )
  , m_lineNumbersOn( false )
  , m_foldingMarkersOn( false )
  , m_lmbSetsBreakpoints( true )
  , m_oldEditableMarks(0)
{                                        
  setBackgroundMode( NoBackground );
  setFont( m_doc->getFont(KateDocument::ViewFont) ); // for line numbers
}

void KateIconBorder::setIconBorderOn( bool enable )
{
  if( enable == m_iconBorderOn )
    return;

  m_iconBorderOn = enable;
  
  updateGeometry();
}

void KateIconBorder::setLineNumbersOn( bool enable )
{
  if( enable == m_lineNumbersOn )
    return;

  m_lineNumbersOn = enable;
  
  updateGeometry();
}

void KateIconBorder::setFoldingMarkersOn( bool enable )
{
  if( enable == m_foldingMarkersOn )
    return;
  
  m_foldingMarkersOn = enable;
  
  updateGeometry();
}

QSize KateIconBorder::sizeHint() const
{
  int w = 0;
  
  if (m_lineNumbersOn) {
    w += fontMetrics().width( QString().setNum(m_view->doc()->numLines()) );
  }

  if (m_iconBorderOn)
    w += iconPaneWidth;

  if (m_foldingMarkersOn)
    w += iconPaneWidth;

  // A little extra makes selecting at the beginning easier
  w = QMAX( w, 4 );

  return QSize( w, 0 );
}

QSize KateIconBorder::minimumSizeHint() const
{
  return sizeHint();
}

void KateIconBorder::paintEvent(QPaintEvent* e)
{      
  QRect rect = e->rect();
  
  uint startline = m_viewInternal->contentsYToLine( m_viewInternal->yPosition() + rect.y() );
  uint endline   = m_viewInternal->contentsYToLine( m_viewInternal->yPosition() + rect.y() + rect.height() - 1 );
                                    
  QPainter p (this);   
  p.translate (0, -m_viewInternal->yPosition());
   
  int fontHeight = m_doc->viewFont.fontHeight;          
  int lnWidth = fontMetrics().width( QString().setNum(m_view->doc()->numLines()) );
   
   //kdDebug(13030)<<"iconborder repaint !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"<<endl;
                 
  for( uint line = startline; line <= endline; line++ )
  {
    uint realLine = m_doc->getRealLine( line );    
    
    int y = line * fontHeight;
    int lnX = 0;
  
    p.fillRect( 0, y, lnWidth+2*iconPaneWidth, fontHeight, colorGroup().light() );

    // line number
    if( m_lineNumbersOn )
    {
      p.setPen(QColor(colorGroup().background()).dark());
      p.drawLine( lnWidth-1, y, lnWidth-1, y+fontHeight );
      if( realLine <= m_doc->lastLine() )
        p.drawText( lnX + 1, y, lnWidth-4, fontHeight, Qt::AlignRight|Qt::AlignVCenter,
          QString("%1").arg( realLine + 1 ) );

      lnX += lnWidth;
    }

    // icon pane
    if( m_iconBorderOn ) {
      p.setPen(QColor(colorGroup().background()).dark());
      p.drawLine(lnX+iconPaneWidth-1, y, lnX+iconPaneWidth-1, y+fontHeight);

      if( realLine <= m_doc->lastLine() )
      {
        uint mark = m_doc->mark (realLine);
        switch (mark)
        {
        case KateDocument::markType01:
          p.drawPixmap(lnX+2, y, QPixmap(bookmark_xpm));
          break;
        case KateDocument::markType02:
          p.drawPixmap(lnX+2, y, QPixmap(breakpoint_xpm));
          break;
        case KateDocument::markType03:
          p.drawPixmap(lnX+2, y, QPixmap(breakpoint_gr_xpm));
          break;
        case KateDocument::markType04:
          p.drawPixmap(lnX+2, y, QPixmap(breakpoint_bl_xpm));
          break;
        case KateDocument::markType05:
          p.drawPixmap(lnX+2, y, QPixmap(exec_xpm));
          break;
        default:
          break;
        }
      }

      lnX += iconPaneWidth;
    }

    // folding markers
    if( m_foldingMarkersOn )
    {
      if( realLine <= m_doc->lastLine() )
      {
        p.setPen(black);
        KateLineInfo info;
        m_doc->regionTree->getLineInfo(&info,realLine);
        if (!info.topLevel)
        {
          if (info.startsVisibleBlock)
            p.drawPixmap(lnX+2,y,QPixmap(minus_xpm));
          else if (info.startsInVisibleBlock)
            p.drawPixmap(lnX+2,y,QPixmap(plus_xpm));
          else if (info.endsBlock)
          {
            p.drawLine(lnX+iconPaneWidth/2,y,lnX+iconPaneWidth/2,y+fontHeight-1);
            p.drawLine(lnX+iconPaneWidth/2,y+fontHeight-1,lnX+iconPaneWidth-2,y+fontHeight-1);
          }
          else
            p.drawLine(lnX+iconPaneWidth/2,y,lnX+iconPaneWidth/2,y+fontHeight-1);
        }
      }
      lnX+=iconPaneWidth;
    }   
  }
}

KateIconBorder::BorderArea KateIconBorder::positionToArea( const QPoint& p ) const
{
  int x = 0;
  if( m_lineNumbersOn ) {
    x += fontMetrics().width( QString().setNum(m_view->doc()->numLines()) );
    if( p.x() <= x )
      return LineNumbers;
  }
  if( m_iconBorderOn ) {
    x += iconPaneWidth;
    if( p.x() <= x )
      return IconBorder;
  }
  if( m_foldingMarkersOn ) {
    x += iconPaneWidth;
    if( p.x() <= x )
      return FoldingMarkers;
  }
  return None;
}

void KateIconBorder::mousePressEvent( QMouseEvent* e )
{
  m_lastClickedLine = m_doc->getRealLine(
    (e->y() + m_viewInternal->contentsY()) / m_doc->viewFont.fontHeight );
  
  BorderArea area = positionToArea( e->pos() );
  if( area == FoldingMarkers ||
      area == None )
  {
    QMouseEvent forward( QEvent::MouseButtonPress, 
      QPoint( 0, e->y() + m_viewInternal->contentsY() ), e->button(), e->state() );
    m_viewInternal->contentsMousePressEvent( &forward );
  }
}

void KateIconBorder::mouseMoveEvent( QMouseEvent* e )
{
  BorderArea area = positionToArea( e->pos() );
  if( area == FoldingMarkers ||
      area == None )
  {
    QMouseEvent forward( QEvent::MouseMove, 
      QPoint( 0, e->y() + m_viewInternal->contentsY() ), e->button(), e->state() );
    m_viewInternal->contentsMouseMoveEvent( &forward );
  }
}

void KateIconBorder::mouseReleaseEvent( QMouseEvent* e )
{
  uint cursorOnLine = m_doc->getRealLine(
    (e->y() + m_viewInternal->contentsY()) / m_doc->viewFont.fontHeight );
  
  switch( positionToArea( e->pos() ) ) {
  case LineNumbers:
    break;
  case IconBorder:
    if( e->button() == LeftButton &&
        cursorOnLine == m_lastClickedLine &&
        cursorOnLine <= m_doc->lastLine() )
    {
      uint mark = m_doc->mark (cursorOnLine);
      createMarkMenu();
      if (m_oldEditableMarks) {
        if (m_markMenu) {
          m_markMenu->exec(QCursor::pos());	
        } else {
          if (mark&m_oldEditableMarks)
            m_doc->removeMark (cursorOnLine, m_oldEditableMarks);
          else
            m_doc->addMark (cursorOnLine, m_oldEditableMarks);
        }
      }
    }
    break;
  case FoldingMarkers:
    if( cursorOnLine == m_lastClickedLine &&
        cursorOnLine <= m_doc->lastLine() )
    {
      kdDebug(13000)<<"The click was within a marker range, is it valid though ?"<<endl;
      KateLineInfo info;
      m_doc->regionTree->getLineInfo(&info,cursorOnLine);
      if ((info.startsVisibleBlock) || (info.startsInVisibleBlock))
      {
         kdDebug(13000)<<"Tell whomever it concerns, that we want a region visibility changed"<<endl;
         emit toggleRegionVisibility(cursorOnLine);
      }
    }
    // Fall through
  default:
    QMouseEvent forward( QEvent::MouseButtonRelease, 
      QPoint( 0, e->y() + m_viewInternal->contentsY() ), e->button(), e->state() );
    m_viewInternal->contentsMouseReleaseEvent( &forward );
    break;
  }
}

void KateIconBorder::mouseDoubleClickEvent( QMouseEvent* e )
{
  BorderArea area = positionToArea( e->pos() );
  if( area == FoldingMarkers ||
      area == None )
  {
    QMouseEvent forward( QEvent::MouseButtonDblClick, 
      QPoint( 0, e->y() + m_viewInternal->contentsY() ), e->button(), e->state() );
    m_viewInternal->contentsMouseDoubleClickEvent( &forward );
  }
}

void KateIconBorder::createMarkMenu()
{
  unsigned int tmpMarks;
  if (m_doc->editableMarks()==m_oldEditableMarks) return;	
  m_oldEditableMarks=m_doc->editableMarks();
  if ((m_markMenu) && (!m_oldEditableMarks)) {
    delete m_markMenu;
    m_markMenu=0;
    return;
  }
  else if ((m_markMenu) && m_oldEditableMarks) m_markMenu->clear();
  tmpMarks=m_oldEditableMarks;

  bool first_found=false;
  for(unsigned int tmpMark=1;tmpMark;tmpMark=tmpMark<<1) {

    if (tmpMark && tmpMarks) {
      tmpMarks -=tmpMark;

      if (!first_found) {
        if (!tmpMarks) {
          if (m_markMenu) {
            delete m_markMenu;
            m_markMenu=0;
          } 
          return;
        }
        if (!m_markMenu) m_markMenu=new QPopupMenu(this);
        m_markMenu->insertItem(QString("Mark type %1").arg(tmpMark),tmpMark);
        first_found=true;
      }
      else m_markMenu->insertItem(QString("Mark type %1").arg(tmpMark),tmpMark);

    }
    if (!tmpMarks) return;

  }
}
