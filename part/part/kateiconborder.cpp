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


KateIconBorder::KateIconBorder(KateView *view, KateViewInternal *internalView)
  : QWidget(view, ""),
    myView(view), myInternalView(internalView),oldEditableMarks(0),markMenu(0)
{                                                  
  myDoc = myInternalView->myDoc;

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
  
  if (myView->iconBorderStatus() & LineNumbers) {
    if ( linesAtLastCheck != myView->doc()->numLines() ) {
      cachedLNWidth = 7 + fontMetrics().width( QString().setNum(myView->doc()->numLines()) );
      linesAtLastCheck = myView->myDoc->numLines();
    }
    w += cachedLNWidth;
  }

  if (myView->iconBorderStatus() & Icons)
    w += iconPaneWidth;

  if (myView->iconBorderStatus() & FoldingMarkers)
    w+=iconPaneWidth;

  return w;
}

void KateIconBorder::paintEvent(QPaintEvent* e)
{      
  QRect rect = e->rect();
  
  uint startline = myInternalView->contentsYToLine( myInternalView->yPosition() + rect.y() );
  uint endline   = myInternalView->contentsYToLine( myInternalView->yPosition() + rect.y() + rect.height() - 1 );
                                    
  QPainter p (this);   
  p.translate (0, -myInternalView->yPosition());
   
   int fontHeight = myDoc->viewFont.fontHeight;          
   int w = width ();                            
   
   //kdDebug(13030)<<"iconborder repaint !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"<<endl;
                 
  for( uint line = startline; line <= endline; line++ )
  {
    int realLine = myDoc->getRealLine( line );    
    
    int y = line * fontHeight;
    int lnX = 0;
  
  p.fillRect( 0, y, cachedLNWidth+2*iconPaneWidth, fontHeight, colorGroup().light() );

  // line number
  if ( (myView->iconBorderStatus() & LineNumbers) )
  {
    p.setPen(QColor(colorGroup().background()).dark());
    p.drawLine( cachedLNWidth-1, y, cachedLNWidth-1, y+fontHeight );
      if ((realLine >= 0) && (realLine <= myDoc->lastLine()))
        p.drawText( lnX + 1, y, cachedLNWidth-4, fontHeight, Qt::AlignRight|Qt::AlignVCenter,
          QString("%1").arg(realLine + 1 ));

      lnX+=cachedLNWidth;
  }

  // icon pane
  if ( (myView->iconBorderStatus() & Icons) ) {
    p.setPen(QColor(colorGroup().background()).dark());
    p.drawLine(lnX+iconPaneWidth-1, y, lnX+iconPaneWidth-1, y+fontHeight);

    if ((realLine >= 0) && (realLine <= myDoc->lastLine()))
    {
    uint mark = myView->myDoc->mark (realLine);
    switch (mark)
    {
       case KateDocument::markType01:	p.drawPixmap(lnX+2, y, QPixmap(bookmark_xpm));
					break;
       case KateDocument::markType02: p.drawPixmap(lnX+2, y, QPixmap(breakpoint_xpm));
					break;
       case KateDocument::markType03: p.drawPixmap(lnX+2, y, QPixmap(breakpoint_gr_xpm));
		                        break;
       case KateDocument::markType04: p.drawPixmap(lnX+2, y, QPixmap(breakpoint_bl_xpm));
		                        break;
       case KateDocument::markType05: p.drawPixmap(lnX+2, y, QPixmap(exec_xpm));
		                        break;
       default: break;

    }
    }

    lnX += iconPaneWidth;
  }

  // folding markers
  if  (myView->iconBorderStatus() & FoldingMarkers)
  {
    if ((realLine >= 0) && (realLine <= myDoc->lastLine()))
    {
    p.setPen(black);
    KateLineInfo info;
    myView->myDoc->regionTree->getLineInfo(&info,realLine);
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
    }

    lnX+=iconPaneWidth;
  }   
   }
 }


void KateIconBorder::mousePressEvent(QMouseEvent* e)
{
    // return if the event is in linenumbers pane
    if ( (!myView->iconBorderStatus() & Icons) && (!myView->iconBorderStatus() & FoldingMarkers) )
      return;
    int xwidth=0;
    if (myView->iconBorderStatus() & Icons) xwidth+=iconPaneWidth;
    if (myView->iconBorderStatus() & FoldingMarkers) xwidth+=iconPaneWidth;
    if (myView->iconBorderStatus() & LineNumbers) xwidth+=cachedLNWidth;
    if (e->x()>xwidth) return;
    //myInternalView->placeCursor( 0, e->y(), 0 );

    uint cursorOnLine = myDoc->getRealLine(  ((e->y() + myInternalView->contentsY()) / myView->myDoc->viewFont.fontHeight)  );

    if (cursorOnLine > myView->myDoc->lastLine())
      return;

    uint mark = myView->myDoc->mark (cursorOnLine);

    if (myView->iconBorderStatus() & Icons)
    {
      int xMin=(myView->iconBorderStatus() & LineNumbers)?cachedLNWidth:0;

      int xMax=xMin+iconPaneWidth;

      if ((e->x()>=xMin) && (e->x()<xMax))
      {
        switch (e->button()) {
        case LeftButton:
		createMarkMenu();
		if (oldEditableMarks) {
			if (markMenu) {
				markMenu->exec(QCursor::pos());	
			}
			else {
		                if (mark&oldEditableMarks)
                			myView->myDoc->removeMark (cursorOnLine, oldEditableMarks);
		                else
                			  myView->myDoc->addMark (cursorOnLine, oldEditableMarks);
			}


		}
            break;

        default:
            break;
        }
      }
    }

    if (myView->iconBorderStatus() & FoldingMarkers)
    {
	kdDebug(13000)<<"checking if a folding marker has been clicked"<<endl;

        int xMin=(myView->iconBorderStatus() & Icons)?iconPaneWidth:0;

        if (myView->iconBorderStatus() & LineNumbers)
          xMin += cachedLNWidth;

        int xMax=xMin+iconPaneWidth;
	if ((e->x()>=xMin) && (e->x()<xMax))
        {
	    kdDebug(13000)<<"The click was within a marker range, is it valid though ?"<<endl;
            KateLineInfo info;
            myView->myDoc->regionTree->getLineInfo(&info,cursorOnLine);
	    if ((info.startsVisibleBlock) || (info.startsInVisibleBlock))
            {
               kdDebug(13000)<<"Tell whomever it concerns, that we want a region visibility changed"<<endl;
	        emit toggleRegionVisibility(cursorOnLine);

            }
        }
    }
}

void KateIconBorder::createMarkMenu()
{
	unsigned int tmpMarks;
	if (myView->myDoc->editableMarks()==oldEditableMarks) return;	
	oldEditableMarks=myView->myDoc->editableMarks();
	if ((markMenu) && (!oldEditableMarks)) {
		delete markMenu;
		markMenu=0;
		return;
	}
	else if ((markMenu) && oldEditableMarks) markMenu->clear();
	tmpMarks=oldEditableMarks;
	
	bool first_found=false;
	for(unsigned int tmpMark=1;tmpMark;tmpMark=tmpMark<<1) {

		if (tmpMark && tmpMarks) {
			tmpMarks -=tmpMark;

			if (!first_found) {
				if (!tmpMarks) {
					if (markMenu) {
						delete markMenu;
						markMenu=0;
					} 
					return;
				}
				if (!markMenu) markMenu=new QPopupMenu(this);
				markMenu->insertItem(QString("Mark type %1").arg(tmpMark),tmpMark);
				first_found=true;
			}
			else markMenu->insertItem(QString("Mark type %1").arg(tmpMark),tmpMark);
			
		}
		if (!tmpMarks) return;
		
	}

}
