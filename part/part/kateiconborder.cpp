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
#include "katedocument.h"
#include "kateiconborder.moc"
#include <kdebug.h>
#include <qpainter.h>

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
  return w;
}


void KateIconBorder::paintLine(int i)
{
  if ( myView->iconBorderStatus == None ) return;

//kdDebug()<<"KateIconBorder::paintLine( "<<i<<") - line is "<<i+1<<endl;
  QPainter p(this);

  int fontHeight = myView->myDoc->viewFont.fontHeight;
  int y = i*fontHeight - myInternalView->yPos;
  int lnX = 0;

  // icon pane
  if ( (myView->iconBorderStatus & Icons) ) {
    p.fillRect(0, y, iconPaneWidth-2, fontHeight, colorGroup().background());
    p.setPen(white);
    p.drawLine(iconPaneWidth-2, y, iconPaneWidth-2, y + fontHeight);
    p.setPen(QColor(colorGroup().background()).dark());
    p.drawLine(iconPaneWidth-1, y, iconPaneWidth-1, y + fontHeight);

    uint mark = myView->myDoc->mark (i);
    if (mark&KateDocument::markType01)
        p.drawPixmap(2, y, QPixmap(bookmark_xpm));
    lnX += iconPaneWidth;
  }

  // line number
  if ( (myView->iconBorderStatus & LineNumbers) ) {
    p.fillRect( lnX, y, width()-2, fontHeight, colorGroup().light() );
    p.setPen(QColor(colorGroup().background()).dark());
    p.drawLine( width()-1, y, width()-1, y + fontHeight );
    if ( (uint)i < myView->myDoc->numLines() )
      p.drawText( lnX + 1, y, width()-lnX-4, fontHeight, Qt::AlignRight|Qt::AlignVCenter, QString("%1").arg(i+1) );
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

  if (myView->iconBorderStatus == None)
    return;

  KateDocument *doc = myView->doc();
  if ( myView->iconBorderStatus & LineNumbers && linesAtLastCheck != doc->numLines() ) {
    cachedLNWidth = 7 + fontMetrics().width( QString().setNum( doc->numLines()) );
    linesAtLastCheck = doc->numLines();
    resize( width(), height() );
    return; // we get a new paint event at resize
  }

  uint lineStart = 0; // first line to paint
  uint lineEnd = 0;   // last line to paint
  uint lnX = 0;       // line numbers X position

  QRect ur = e->rect();

  int h = fontMetrics().height();
  int yPos = myInternalView->yPos;
  lineStart = ( yPos + ur.top() ) / h;
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
  // paint the background of the line numbers pane if required
  if ( myView->iconBorderStatus & LineNumbers ) {
    p.fillRect( lnX, 0, width()-2, height(), colorGroup().light() );
    p.setPen(QColor(colorGroup().background()).dark());
    p.drawLine(width()-1, 0, width()-1, height());
  }

  QString s;             // line number
  int adj = yPos%h;      // top line may be obscured
  for( uint i = lineStart; i <= lineEnd; ++i ) {
    // paint icon if required
    if (myView->iconBorderStatus & Icons) {
      if ( doc->mark(i) & KateDocument::markType01 )
        p.drawPixmap(2, (i - lineStart)*h - adj, QPixmap(bookmark_xpm));
    }
    // paint line number if required
    if (myView->iconBorderStatus & LineNumbers) {
      s.setNum( i );
      p.drawText( lnX + 1, (i-lineStart-1)*h - adj, width()-lnX-4, h, Qt::AlignRight|Qt::AlignVCenter, s );
    }
  }
}


void KateIconBorder::mousePressEvent(QMouseEvent* e)
{
    // return if the event is in linenumbers pane
    if ( !myView->iconBorderStatus & Icons || e->x() > iconPaneWidth )
      return;
    myInternalView->placeCursor( 0, e->y(), 0 );

    uint cursorOnLine = (e->y() + myInternalView->yPos) / myView->myDoc->viewFont.fontHeight;

    if (cursorOnLine > myView->myDoc->lastLine())
      return;

    uint mark = myView->myDoc->mark (cursorOnLine);

    switch (e->button()) {
    case LeftButton:
            if (mark&KateDocument::markType01)
              myView->myDoc->removeMark (cursorOnLine, KateDocument::markType01);
            else
              myView->myDoc->addMark (cursorOnLine, KateDocument::markType01);
        break;
 /*   case RightButton:
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
