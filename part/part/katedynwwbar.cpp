/* This file is part of the KDE libraries
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>

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


#include "katedynwwbar.h"
#include "katedynwwbar.moc"

#include "kateview.h"
#include "kateviewinternal.h"
#include "katedocument.h"

#include <kdebug.h>
#include <kglobalsettings.h>
#include <klocale.h>

#include <qpainter.h>
#include <qpopupmenu.h>
#include <qcursor.h>
#include <qstyle.h>
#include <qpainter.h>

using namespace KTextEditor;


KateDynWWBar::KateDynWWBar ( KateViewInternal* internalView, QWidget *parent )
  : QWidget(parent, "", Qt::WStaticContents | Qt::WRepaintNoErase | Qt::WResizeNoErase )
  , m_view( internalView->m_view )
  , m_doc( internalView->m_doc )
  , m_viewInternal( internalView )
{                            
  setSizePolicy( QSizePolicy(  QSizePolicy::Fixed, QSizePolicy::Minimum ) );
            
  setBackgroundMode( NoBackground );
  
  updateFont();
}


QSize KateDynWWBar::sizeHint() const
{
  return QSize(style().scrollBarExtent().width(),0);
}

void KateDynWWBar::updateFont()
{
  int w=style().scrollBarExtent().width();
  int h=m_doc->viewFont.fontHeight;
  arrow.resize(w,h);
  QPainter p(&arrow);
  p.fillRect( 0, 0, w, h, m_doc->colors[0] );

  h=m_doc->viewFont.fontAscent;

  p.setPen(m_doc->colors[4]);
  p.drawLine(w/2, h/2, w/2, 0);
  p.lineTo(w*3/4, h/4);
  p.lineTo(w-1,0);
  p.lineTo(w-1, h/2);
  p.lineTo(w/2, h-1);
  p.lineTo(w/4,h-1);
  p.lineTo(0, h*3/4);
  p.lineTo(w/4, h/2);
  p.lineTo(w-1, h/2);
/*
  const KateFontMetrics& fm = m_doc->getFontMetrics(KateDocument::ViewFont);
  m_maxCharWidth = 0;
  // Loop to determine the widest numeric character in the current font.
  // 48 is ascii '0'
  for (int i = 48; i < 58; i++) {
    int charWidth = fm.width( QChar(i) );
    if (charWidth > m_maxCharWidth) m_maxCharWidth = charWidth;
  }*/
}


void KateDynWWBar::paintEvent(QPaintEvent* e)
{      
  QRect updateR = e->rect();
  paintBar (updateR.x(), updateR.y(), updateR.width(), updateR.height());
}

void KateDynWWBar::paintBar (int x, int y, int width, int height)
{
  int xStart = x;
  int xEnd = xStart + width;
  uint h = m_doc->viewFont.fontHeight;
  uint startz = (y / h);
  uint endz = startz + 1 + (height / h);
  uint lineRangesSize = m_viewInternal->lineRanges.size();

  int w( this->width() );                     // sane value/calc only once

  w=100;

  QPainter p ( this );
  p.setFont ( m_doc->getFont(KateDocument::ViewFont) ); // for line numbers
  p.setPen ( m_doc->myAttribs[0].col );
  
  for (uint z=startz; z <= endz; z++)
  {
    int y = h * z;
    int realLine = -1;
    
    LineRange *r;

    if (z < lineRangesSize)
    {
       r=&m_viewInternal->lineRanges[z];
       realLine = r->line;
    }

    if (realLine==-1)
    {
        p.fillRect(0,y,w,h,m_doc->colors[0]);
        continue;
    }
    
    if (r->lineEnd)
        p.fillRect(0,y,w,h,m_doc->colors[0]);
    else
        p.drawPixmap(0,y,arrow);
//         p.fillRect(0,y,w,h,Qt::black);

    
  }
}

