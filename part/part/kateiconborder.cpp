/* This file is part of the KDE libraries
   Copyright (C) 2002 John Firebaugh <jfirebaugh@kde.org>
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
#include "kateiconborder.moc"

#include "kateview.h"
#include "kateviewinternal.h"
#include "katedocument.h"
#include "katecodefoldinghelpers.h"
#include "katerenderer.h"
#include "kateattribute.h"
#include "kateconfig.h"

#include <kglobalsettings.h>
#include <klocale.h>

#include <qpainter.h>
#include <qpopupmenu.h>
#include <qcursor.h>
#include <qstyle.h>

#include <math.h>

using namespace KTextEditor;

static const char* const plus_xpm[] = {
"11 11 3 1",
"       c None",
".      c #000000",
"+      c #FFFFFF",
"...........",
".+++++++++.",
".+++++++++.",
".++++.++++.",
".++++.++++.",
".++.....++.",
".++++.++++.",
".++++.++++.",
".+++++++++.",
".+++++++++.",
"..........."};

static const char* const minus_xpm[] = {
"11 11 3 1",
"       c None",
".      c #000000",
"+      c #FFFFFF",
"...........",
".+++++++++.",
".+++++++++.",
".+++++++++.",
".+++++++++.",
".++.....++.",
".+++++++++.",
".+++++++++.",
".+++++++++.",
".+++++++++.",
"..........."};


static const char* const bookmark_xpm[]={
"12 12 4 1",
"b c #808080",
"a c #000080",
"# c #0000ff",
". c None",
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
".aaa.bbb...."};

const int iconPaneWidth = 16;
const int halfIPW = 8;

static QPixmap minus_px ((const char**)minus_xpm);
static QPixmap plus_px ((const char**)plus_xpm);

KateIconBorder::KateIconBorder ( KateViewInternal* internalView, QWidget *parent )
  : QWidget(parent, "", Qt::WStaticContents | Qt::WRepaintNoErase | Qt::WResizeNoErase )
  , m_view( internalView->m_view )
  , m_doc( internalView->m_doc )
  , m_viewInternal( internalView )
  , m_iconBorderOn( false )
  , m_lineNumbersOn( false )
  , m_foldingMarkersOn( false )
  , m_dynWrapIndicatorsOn( false )
  , m_dynWrapIndicators( 0 )
  , m_cachedLNWidth( 0 )
  , m_maxCharWidth( 0 )
  , m_defaultMarkType(MarkInterface::markType01)
{
  setSizePolicy( QSizePolicy(  QSizePolicy::Fixed, QSizePolicy::Minimum ) );

  setBackgroundMode( NoBackground );

  m_doc->setDescription( MarkInterface::markType01, i18n("Bookmark") );
  m_doc->setPixmap( MarkInterface::markType01, QPixmap((const char**)bookmark_xpm) );

  updateFont();
}

void KateIconBorder::setIconBorderOn( bool enable )
{
  if( enable == m_iconBorderOn )
    return;

  m_iconBorderOn = enable;

  updateGeometry();
  update ();
}

void KateIconBorder::setLineNumbersOn( bool enable )
{
  if( enable == m_lineNumbersOn )
    return;

  m_lineNumbersOn = enable;
  m_dynWrapIndicatorsOn = (m_dynWrapIndicators == 1) ? enable : m_dynWrapIndicators;

  updateGeometry();
  update ();
}

void KateIconBorder::setDynWrapIndicators( int state )
{
  if (state == m_dynWrapIndicators )
    return;

  m_dynWrapIndicators = state;
  m_dynWrapIndicatorsOn = (state == 1) ? m_lineNumbersOn : state;

  updateGeometry ();
  update ();
}

void KateIconBorder::setFoldingMarkersOn( bool enable )
{
  if( enable == m_foldingMarkersOn )
    return;

  m_foldingMarkersOn = enable;

  updateGeometry();
  update ();
}

QSize KateIconBorder::sizeHint() const
{
  int w = 0;

  if (m_iconBorderOn)
    w += iconPaneWidth + 1;

  if (m_lineNumbersOn || (m_view->dynWordWrap() && m_dynWrapIndicatorsOn)) {
    w += lineNumberWidth();
  }

  if (m_foldingMarkersOn)
    w += iconPaneWidth;

  w += 4;

  return QSize( w, 0 );
}

// This function (re)calculates the maximum width of any of the digit characters (0 -> 9)
// for graceful handling of variable-width fonts as the linenumber font.
void KateIconBorder::updateFont()
{
  const QFontMetrics *fm = m_view->renderer()->config()->fontMetrics();
  m_maxCharWidth = 0;
  // Loop to determine the widest numeric character in the current font.
  // 48 is ascii '0'
  for (int i = 48; i < 58; i++) {
    int charWidth = fm->width( QChar(i) );
    m_maxCharWidth = QMAX(m_maxCharWidth, charWidth);
  }
}

int KateIconBorder::lineNumberWidth() const
{
  int width = m_lineNumbersOn ? ((int)log10((double)(m_view->doc()->numLines())) + 1) * m_maxCharWidth + 4 : 0;

  if (m_view->dynWordWrap() && m_dynWrapIndicatorsOn) {
    width = QMAX(style().scrollBarExtent().width() + 4, width);

    if (m_cachedLNWidth != width || m_oldBackgroundColor != *m_view->renderer()->config()->iconBarColor()) {
      int w = style().scrollBarExtent().width();
      int h = m_view->renderer()->config()->fontMetrics()->height();

      QSize newSize(w, h);
      if ((m_arrow.size() != newSize || m_oldBackgroundColor != *m_view->renderer()->config()->iconBarColor()) && !newSize.isEmpty()) {
        m_arrow.resize(newSize);

        QPainter p(&m_arrow);
        p.fillRect( 0, 0, w, h, *m_view->renderer()->config()->iconBarColor() );

        h = m_view->renderer()->config()->fontMetrics()->ascent();

        p.setPen(m_doc->attribs()->at(0).textColor());
        p.drawLine(w/2, h/2, w/2, 0);
#if 1
        p.lineTo(w/4, h/4);
        p.lineTo(0, 0);
        p.lineTo(0, h/2);
        p.lineTo(w/2, h-1);
        p.lineTo(w*3/4, h-1);
        p.lineTo(w-1, h*3/4);
        p.lineTo(w*3/4, h/2);
        p.lineTo(0, h/2);
#else
        p.lineTo(w*3/4, h/4);
        p.lineTo(w-1,0);
        p.lineTo(w-1, h/2);
        p.lineTo(w/2, h-1);
        p.lineTo(w/4,h-1);
        p.lineTo(0, h*3/4);
        p.lineTo(w/4, h/2);
        p.lineTo(w-1, h/2);
#endif
      }
    }
  }

  return width;
}

void KateIconBorder::paintEvent(QPaintEvent* e)
{
  QRect updateR = e->rect();
  paintBorder (updateR.x(), updateR.y(), updateR.width(), updateR.height());
}

void KateIconBorder::paintBorder (int /*x*/, int y, int /*width*/, int height)
{
  uint h = m_view->renderer()->config()->fontStruct()->fontHeight;
  uint startz = (y / h);
  uint endz = startz + 1 + (height / h);
  uint lineRangesSize = m_viewInternal->lineRanges.size();

  // center the folding boxes
  int m_px = (h - 11) / 2;
  if (m_px < 0)
    m_px = 0;

  int lnWidth( 0 );
  if ( m_lineNumbersOn || (m_view->dynWordWrap() && m_dynWrapIndicatorsOn) ) // avoid calculating unless needed ;-)
  {
    lnWidth = lineNumberWidth();
    if ( lnWidth != m_cachedLNWidth || m_oldBackgroundColor != *m_view->renderer()->config()->iconBarColor() )
    {
      // we went from n0 ->n9 lines or vice verca
      // this causes an extra updateGeometry() first time the line numbers
      // are displayed, but sizeHint() is supposed to be const so we can't set
      // the cached value there.
      m_cachedLNWidth = lnWidth;
      m_oldBackgroundColor = *m_view->renderer()->config()->iconBarColor();
      updateGeometry();
      update ();
      return;
    }
  }

  int w( this->width() );                     // sane value/calc only once

  QPainter p ( this );
  p.setFont ( *m_view->renderer()->config()->font() ); // for line numbers
  p.setPen ( m_doc->attribs()->at(0).textColor() );

  KateLineInfo oldInfo;
  if ((m_viewInternal->lineRanges[startz].line-1) < 0)
    oldInfo.topLevel = true;
  else
     m_doc->lineInfo(&oldInfo,m_viewInternal->lineRanges[startz].line-1);

  for (uint z=startz; z <= endz; z++)
  {
    int y = h * z;
    int realLine = -1;

    if (z < lineRangesSize)
     realLine = m_viewInternal->lineRanges[z].line;

    int lnX ( 0 );

    p.fillRect( 0, y, w-4, h, *m_view->renderer()->config()->iconBarColor() );
    p.fillRect( w-4, y, 4, h, *m_view->renderer()->config()->backgroundColor() );

    // icon pane
    if( m_iconBorderOn )
    {
      p.drawLine(lnX+iconPaneWidth, y, lnX+iconPaneWidth, y+h);

      if( (realLine > -1) && (m_viewInternal->lineRanges[z].startCol == 0) )
      {
        uint mrk ( m_doc->mark( realLine ) ); // call only once

        if ( mrk )
        {
          for( uint bit = 0; bit < 32; bit++ )
          {
            MarkInterface::MarkTypes markType = (MarkInterface::MarkTypes)(1<<bit);
            if( mrk & markType )
            {
              QPixmap *px_mark (m_doc->markPixmap( markType ));

              if (px_mark)
              {
                // center the mark pixmap
                int x_px = (iconPaneWidth - px_mark->width()) / 2;
                if (x_px < 0)
                  x_px = 0;

                int y_px = (h - px_mark->height()) / 2;
                if (y_px < 0)
                  y_px = 0;

                p.drawPixmap( lnX+x_px, y+y_px, *px_mark);
              }
            }
          }
        }
      }

      lnX += iconPaneWidth + 1;
    }

    // line number
    if( m_lineNumbersOn || (m_view->dynWordWrap() && m_dynWrapIndicatorsOn) )
    {
      lnX +=2;

      if (realLine > -1)
        if (m_viewInternal->lineRanges[z].startCol == 0) {
          if (m_lineNumbersOn)
            p.drawText( lnX + 1, y, lnWidth-4, h, Qt::AlignRight|Qt::AlignVCenter, QString("%1").arg( realLine + 1 ) );
        } else if (m_view->dynWordWrap() && m_dynWrapIndicatorsOn) {
          p.drawPixmap(lnX + lnWidth - m_arrow.width() - 4, y, m_arrow);
        }

      lnX += lnWidth;
    }

    // folding markers
    if( m_foldingMarkersOn )
    {
      if( realLine > -1 )
      {
        KateLineInfo info;
        m_doc->lineInfo(&info,realLine);

        if (!info.topLevel)
        {
          if (info.startsVisibleBlock && (m_viewInternal->lineRanges[z].startCol == 0))
          {
            if (oldInfo.topLevel)
              p.drawLine(lnX+halfIPW,y+m_px,lnX+halfIPW,y+h-1);
            else
              p.drawLine(lnX+halfIPW,y,lnX+halfIPW,y+h-1);

            p.drawPixmap(lnX+3,y+m_px,minus_px);
          }
          else if (info.startsInVisibleBlock)
          {
            if (m_viewInternal->lineRanges[z].startCol == 0)
            {
              if (oldInfo.topLevel)
                p.drawLine(lnX+halfIPW,y+m_px,lnX+halfIPW,y+h-1);
              else
                p.drawLine(lnX+halfIPW,y,lnX+halfIPW,y+h-1);

              p.drawPixmap(lnX+3,y+m_px,plus_px);
            }
            else
            {
              p.drawLine(lnX+halfIPW,y,lnX+halfIPW,y+h-1);
            }

            if (!m_viewInternal->lineRanges[z].wrap)
              p.drawLine(lnX+halfIPW,y+h-1,lnX+iconPaneWidth-2,y+h-1);
          }
          else
          {
            p.drawLine(lnX+halfIPW,y,lnX+halfIPW,y+h-1);

            if (info.endsBlock && !m_viewInternal->lineRanges[z].wrap)
              p.drawLine(lnX+halfIPW,y+h-1,lnX+iconPaneWidth-2,y+h-1);
          }
        }

        oldInfo = info;
      }

      lnX += iconPaneWidth;
    }
  }
}

KateIconBorder::BorderArea KateIconBorder::positionToArea( const QPoint& p ) const
{
  int x = 0;

  if( m_iconBorderOn ) {
    x += iconPaneWidth;
    if( p.x() <= x )
      return IconBorder;
  }
  if( m_lineNumbersOn || m_dynWrapIndicators ) {
    x += lineNumberWidth();
    if( p.x() <= x )
      return LineNumbers;
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
  m_lastClickedLine = m_viewInternal->yToLineRange(e->y()).line;

  QMouseEvent forward( QEvent::MouseButtonPress,
    QPoint( 0, e->y() ), e->button(), e->state() );
  m_viewInternal->mousePressEvent( &forward );
}

void KateIconBorder::mouseMoveEvent( QMouseEvent* e )
{
  QMouseEvent forward( QEvent::MouseMove,
    QPoint( 0, e->y() ), e->button(), e->state() );
  m_viewInternal->mouseMoveEvent( &forward );
}

void KateIconBorder::mouseReleaseEvent( QMouseEvent* e )
{
  uint cursorOnLine = m_viewInternal->yToLineRange(e->y()).line;

  if (cursorOnLine == m_lastClickedLine &&
      cursorOnLine <= m_doc->lastLine() )
  {
    BorderArea area = positionToArea( e->pos() );
    if( area == IconBorder) {
      if (e->button() == LeftButton) {
        if( m_doc->editableMarks() & m_defaultMarkType ) {
          if( m_doc->mark( cursorOnLine ) & m_defaultMarkType )
            m_doc->removeMark( cursorOnLine, m_defaultMarkType );
          else
            m_doc->addMark( cursorOnLine, m_defaultMarkType );
          } else {
            showMarkMenu( cursorOnLine, QCursor::pos() );
          }
        }
        else
        if (e->button() == RightButton) {
          showMarkMenu( cursorOnLine, QCursor::pos() );
        }
    }

    if ( area == FoldingMarkers) {
      KateLineInfo info;
      m_doc->lineInfo(&info,cursorOnLine);
      if ((info.startsVisibleBlock) || (info.startsInVisibleBlock)) {
        emit toggleRegionVisibility(cursorOnLine);
      }
    }
  }

  QMouseEvent forward( QEvent::MouseButtonRelease,
    QPoint( 0, e->y() ), e->button(), e->state() );
  m_viewInternal->mouseReleaseEvent( &forward );
}

void KateIconBorder::mouseDoubleClickEvent( QMouseEvent* e )
{
  QMouseEvent forward( QEvent::MouseButtonDblClick,
    QPoint( 0, e->y() ), e->button(), e->state() );
  m_viewInternal->mouseDoubleClickEvent( &forward );
}

void KateIconBorder::showMarkMenu( uint line, const QPoint& pos )
{
  QPopupMenu markMenu;
  QPopupMenu selectDefaultMark;

  typedef QValueVector<int> MarkTypeVector;
  MarkTypeVector vec( 33 );
  int i=1;

  for( uint bit = 0; bit < 32; bit++ ) {
    MarkInterface::MarkTypes markType = (MarkInterface::MarkTypes)(1<<bit);
    if( !(m_doc->editableMarks() & markType) )
      continue;

    if( !m_doc->markDescription( markType ).isEmpty() ) {
      markMenu.insertItem( m_doc->markDescription( markType ), i );
      selectDefaultMark.insertItem( m_doc->markDescription( markType ), i+100);
    } else {
      markMenu.insertItem( i18n("Mark Type %1").arg( bit + 1 ), i );
      selectDefaultMark.insertItem( i18n("Mark Type %1").arg( bit + 1 ), i+100);
    }

    if( m_doc->mark( line ) & markType )
      markMenu.setItemChecked( i, true );

    if( markType & m_defaultMarkType )
      selectDefaultMark.setItemChecked( i+100, true );

    vec[i++] = markType;
  }

  if( markMenu.count() == 0 )
    return;

  if( markMenu.count() > 1 )
    markMenu.insertItem( i18n("Set Default Mark Type" ), &selectDefaultMark);

  int result = markMenu.exec( pos );
  if( result <= 0 )
    return;

  if ( result > 100)
     m_defaultMarkType  = vec[result-100];
  else
  {
    MarkInterface::MarkTypes markType = (MarkInterface::MarkTypes) vec[result];
    if( m_doc->mark( line ) & markType ) {
      m_doc->removeMark( line, markType );
    } else {
        m_doc->addMark( line, markType );
    }
  }
}

// kate: space-indent on; indent-width 2; replace-tabs on;
