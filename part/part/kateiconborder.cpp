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

const char*ddd_xpm[]={
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


KateIconBorder::KateIconBorder(KateView *view, KateViewInternal *internalView)
    : QWidget(view), myView(view), myInternalView(internalView)
{
  lmbSetsBreakpoints = true; // anders: does NOTHING ?!
  iconPaneWidth = 16; // FIXME: this should be shared by all instances!
  lmbSetsBreakpoints = true;
  setFont( myView->doc()->getFont(KateDocument::ViewFont) ); // for line numbers
  cachedLNWidth = 7 + fontMetrics().width(QString().setNum(myView->doc()->numLines()));
  linesAtLastCheck = myView->myDoc->numLines();
}

KateIconBorder::~KateIconBorder()
{
}

int KateIconBorder::width()
{
  int w = 0;
  if (myView->iconBorderStatus & Icons)
    w += iconPaneWidth;
  if (myView->iconBorderStatus & LineNumbers) {
    if ( linesAtLastCheck != myView->doc()->numLines() ) {
      cachedLNWidth = 7 + fontMetrics().width( QString().setNum(myView->doc()->numLines()) );
      linesAtLastCheck = myView->myDoc->numLines();
    }
    w += cachedLNWidth;
  }
  if (myView->iconBorderStatus & FoldingMarkers)
    w+=iconPaneWidth;
  return w;
}


void KateIconBorder::paintLine(int i,int pos)
{
  if ( myView->iconBorderStatus == None ) return;
  if ( (uint)i > myView->myDoc->numLines() ) return;

  //kdDebug()<<"KateIconBorder::paintLine( "<<i<<") - line is "<<i+1<<endl;
  QPainter p(this);

  int fontHeight = myView->myDoc->viewFont.fontHeight;
  int y = (pos-myView->myViewInternal->startLine) * fontHeight;
  int lnX = 0;

  // icon pane
  if ( (myView->iconBorderStatus & Icons) ) {
    p.fillRect(0, y, iconPaneWidth-2, fontHeight, colorGroup().background());
    p.setPen(white);
    p.drawLine(iconPaneWidth-2, y, iconPaneWidth-2, y + fontHeight);
    p.setPen(QColor(colorGroup().background()).dark());
    p.drawLine(iconPaneWidth-1, y, iconPaneWidth-1, y + fontHeight);

    uint mark = myView->myDoc->mark (myInternalView->lineRanges[i-myInternalView->startLine].line);
    if (mark&KateDocument::markType01)
        p.drawPixmap(2, y, QPixmap(bookmark_xpm));
    lnX += iconPaneWidth;
  }

  // folding markers
  if  (myView->iconBorderStatus & FoldingMarkers)
  {
    p.fillRect(lnX,y,iconPaneWidth-1,fontHeight,colorGroup().background());
    p.setPen(black);
    KateLineInfo info;
    myView->myDoc->regionTree->getLineInfo(&info,myInternalView->lineRanges[i-myInternalView->startLine].line);
    if (!info.topLevel)
     {
         if (info.startsVisibleBlock)
            p.drawPixmap(lnX+2,y,QPixmap(minus_xpm));
            else
	    if (info.startsInVisibleBlock)
              p.drawPixmap(lnX+2,y,QPixmap(plus_xpm));
	      else
              if (info.endsBlock)
	      {
	       p.drawLine(lnX+iconPaneWidth/2,y,lnX+iconPaneWidth/2,y+fontHeight-1);
	       p.drawLine(lnX+iconPaneWidth/2,y+fontHeight-1,lnX+iconPaneWidth-2,y+fontHeight-1);
              }
               else
	       p.drawLine(lnX+iconPaneWidth/2,y,lnX+iconPaneWidth/2,y+fontHeight-1);

    }
    lnX+=iconPaneWidth;
  }


  // line number
  if ( (myView->iconBorderStatus & LineNumbers) && i < myView->doc()->numLines() ) {
    p.fillRect( lnX, y, width()-2, fontHeight, colorGroup().light() );
    p.setPen(QColor(colorGroup().background()).dark());
    p.drawLine( width()-1, y, width()-1, y + fontHeight );
//    kdDebug()<<"IconBorder::paintLine"<<endl;
      p.drawText( lnX + 1, y, width()-lnX-4, fontHeight, Qt::AlignRight|Qt::AlignVCenter,
          QString("%1").arg(myInternalView->lineRanges[i-myInternalView->startLine].line + 1 ));
  }
         /*
    if ((line->breakpointId() != -1)) {
        if (!line->breakpointEnabled())
            p.drawPixmap(2, y, QPixmap(breakpoint_gr_xpm));
        else if (line->breakpointPending())
            p.drawPixmap(2, y, QPixmap(breakpoint_bl_xpm));
        else
            p.drawPixmap(2, y, QPixmap(breakpoint_xpm));
    }
    if (line->isExecutionPoint())
        p.drawPixmap(2, y, QPixmap(ddd_xpm));    */


}


void KateIconBorder::paintEvent(QPaintEvent* e)
{
//	return;
  if (myView->iconBorderStatus == None)
    return;

  //kdDebug()<<"KateIconBorder::paintEvent()"<<endl;

  KateDocument *doc = myView->doc();
  if ( myView->iconBorderStatus & LineNumbers && linesAtLastCheck != doc->numLines() ) {
    cachedLNWidth = 7 + fontMetrics().width( QString().setNum( doc->numLines()) );
    linesAtLastCheck = doc->numLines();
    resize( width(), height() );
    return; // we get a new paint event at resize
  }


  uint topLine=myInternalView->startLine;
  uint lineStart = 0; // first line to paint
  uint lineEnd = 0;   // last line to paint
  uint lnX = 0;       // line numbers X position

  QRect ur = e->rect();

  int h = fontMetrics().height();
  int yPos = myInternalView->originCoordinates().y();
  lineStart = topLine; //( yPos + ur.top() ) / h;
  // number of lines the rect can display +1 (to compensate for half lines)
  uint vl = ( ur.height() / h ) + 1;
  lineEnd = QMIN( lineStart + vl, doc->numLines() );
  //kdDebug()<<"Painting lines: "<<lineStart<<" - "<<lineEnd<<endl;

  QPainter p(this);
  // paint the background of the icon pane if required
  if ( myView->iconBorderStatus & Icons ) {
    p.fillRect( 0, 0, iconPaneWidth-2, height(), colorGroup().background() );
    p.setPen( white );
    p.drawLine( iconPaneWidth-2, 0, iconPaneWidth-2, height() );
    p.setPen(QColor(colorGroup().background()).dark());
    p.drawLine( iconPaneWidth-1, 0, iconPaneWidth-1, height() );
    lnX += iconPaneWidth;
  }


  // folding markers
  if  (myView->iconBorderStatus & FoldingMarkers)
  {
    p.fillRect(0,0,iconPaneWidth-1,height(),colorGroup().background());
    lnX+=iconPaneWidth;
  }

  // paint the background of the line numbers pane if required
  if ( myView->iconBorderStatus & LineNumbers ) {
    p.fillRect( lnX, 0, width()-2, height(), colorGroup().light() );
    p.setPen(QColor(colorGroup().background()).dark());
    p.drawLine(width()-1, 0, width()-1, height());
  }

  QString s;             // line number
  int adj = yPos%h;      // top line may be obscured

  int mappedLine=1;
  for( uint i = lineStart; i <= lineEnd; ++i ) {
    
    //kdDebug()<<QString("KateIconBorder::paintEvent: line: %1").arg(i)<<endl;
    
    bool mappedLineValid=true;
    if ((i-topLine) > 0) mappedLine=myInternalView->lineRanges[i-topLine].line;
    else mappedLineValid=false;
 
    if (mappedLineValid)
    {
       // paint icon if required
       lnX=0;
       if (myView->iconBorderStatus & Icons) {
         if ( doc->mark(mappedLine) & KateDocument::markType01 )
           p.drawPixmap(2, (i)*h - adj, QPixmap(bookmark_xpm));
         lnX+=iconPaneWidth;
       }

       if (myView->iconBorderStatus & FoldingMarkers) {
         p.setPen(black);
 
   		KateLineInfo info;
        
		myView->myDoc->regionTree->getLineInfo(&info,mappedLine);
		if (!info.topLevel)
		{
			if (info.startsVisibleBlock)
				p.drawPixmap(lnX+2,(i-lineStart)*h-adj,QPixmap(minus_xpm));
			else
			if (info.startsInVisibleBlock)
				p.drawPixmap(lnX+2,(i-lineStart)*h-adj,QPixmap(plus_xpm));
	                else
			if (info.endsBlock)
        		{
	                  p.drawLine(lnX+iconPaneWidth/2,(i-lineStart)*h-adj,lnX+iconPaneWidth/2,(i-lineStart+1)*h-1);
        	          p.drawLine(lnX+iconPaneWidth/2,(i-lineStart+1)*h-1,lnX+iconPaneWidth-2,(i-lineStart+1)*h-1);
	                }
		        else
		           p.drawLine(lnX+iconPaneWidth/2,(i-lineStart)*h-adj,lnX+iconPaneWidth/2,(i-lineStart+1)*h-adj-1);
	
		}
	      lnX+=iconPaneWidth;
	    p.setPen(QColor(colorGroup().background()).dark());
	    }
	}
    // paint line number if required
    if (myView->iconBorderStatus & LineNumbers) {
      s.setNum( mappedLine +1);
      mappedLine++;
      p.drawText( lnX + 1, (i-lineStart/*JWTEST*/)*h - adj, width()-lnX-4, h, Qt::AlignRight|Qt::AlignVCenter, s );
    }
  }
}


void KateIconBorder::mousePressEvent(QMouseEvent* e)
{
    // return if the event is in linenumbers pane    
    if ( (!myView->iconBorderStatus & Icons) && (!myView->iconBorderStatus & FoldingMarkers) )
      return;
    int xwidth=0;
    if (myView->iconBorderStatus & Icons) xwidth+=iconPaneWidth;
    if (myView->iconBorderStatus & FoldingMarkers) xwidth+=iconPaneWidth;
    if (e->x()>xwidth) return;
    myInternalView->placeCursor( 0, e->y(), 0 );

    uint cursorOnLine = (e->y() + myInternalView->originCoordinates().y()) / myView->myDoc->viewFont.fontHeight;
    //if (myInternalView->lineRanges[cursorOnLine-myInternalView->startLine])
    	cursorOnLine=myInternalView->lineRanges[cursorOnLine-myInternalView->startLine].line;

    if (cursorOnLine > myView->myDoc->lastLine())
      return;

    uint mark = myView->myDoc->mark (cursorOnLine);

    if ((myView->iconBorderStatus & Icons) && (e->x()<iconPaneWidth))
    {
        switch (e->button()) {
        case LeftButton:
                if (mark&KateDocument::markType01)
                  myView->myDoc->removeMark (cursorOnLine, KateDocument::markType01);
                else
                  myView->myDoc->addMark (cursorOnLine, KateDocument::markType01);
            break;
 /*       case RightButton:
            {
                if (!line)
                    break;
                KPopupMenu popup;
                popup.setCheckable(true);
                popup.insertTitle(i18n("Breakpoints/Bookmarks"));
                int idToggleBookmark =     popup.insertItem(i18n("Toggle bookmark"));
                popup.insertSeparator();
                int idToggleBreakpoint =   popup.insertItem(i18n("Toggle breakpoint"));
                int idEditBreakpoint   =   popup.insertItem(i18n("Edit breakpoint"));
                int idEnableBreakpoint =   popup.insertItem(i18n("Disable breakpoint"));
                popup.insertSeparator();
                popup.insertSeparator();
                int idLmbSetsBreakpoints = popup.insertItem(i18n("LMB sets breakpoints"));
                int idLmbSetsBookmarks   = popup.insertItem(i18n("LMB sets bookmarks"));

                popup.setItemChecked(idLmbSetsBreakpoints, lmbSetsBreakpoints);
                popup.setItemChecked(idLmbSetsBookmarks, !lmbSetsBreakpoints);

                if (line->breakpointId() == -1) {
                    popup.setItemEnabled(idEditBreakpoint, false);
                    popup.setItemEnabled(idEnableBreakpoint, false);
                    popup.changeItem(idEnableBreakpoint, i18n("Enable breakpoint"));
                }
                int res = popup.exec(mapToGlobal(e->pos()));
                if (res == idToggleBookmark) {
                    line->toggleBookmark();
                    doc->tagLines(cursorOnLine, cursorOnLine);
                    doc->updateViews();
                } else if (res == idToggleBreakpoint)
                    emit myView->toggledBreakpoint(cursorOnLine);
                else if (res == idEditBreakpoint)
                    emit myView->editedBreakpoint(cursorOnLine);
                else if (res == idEnableBreakpoint)
                    emit myView->toggledBreakpointEnabled(cursorOnLine+1);
                else if (res == idLmbSetsBreakpoints || res == idLmbSetsBookmarks)
                    lmbSetsBreakpoints = !lmbSetsBreakpoints;
                break;
            }
        case MidButton:
            line->toggleBookmark();
            doc->tagLines(cursorOnLine, cursorOnLine);
            doc->updateViews();
            break;      */
        default:
            break;
        }
    }

    if (myView->iconBorderStatus & FoldingMarkers)
    {
	kdDebug()<<"checking if a folding marker has been clicked"<<endl;

        int xMin=(myView->iconBorderStatus & Icons)?iconPaneWidth:0;
        int xMax=xMin+iconPaneWidth;
	if ((e->x()>=xMin) && (e->x()<xMax))
        {
	    kdDebug()<<"The click was within a marker range, is it valid though ?"<<endl;
            KateLineInfo info;
            myView->myDoc->regionTree->getLineInfo(&info,cursorOnLine);
	    if ((info.startsVisibleBlock) || (info.startsInVisibleBlock))
            {
               kdDebug()<<"Tell whomever it concerns, that we want a region visibility changed"<<endl;
	        emit toggleRegionVisibility(cursorOnLine);

            }
        }
    }
}
