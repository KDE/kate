/***************************************************************************
                          kateiconborder.cpp  -  description
                             -------------------
    begin                : Mon Jan 15 2001
    copyright            : (C) 2001 by Christoph Cullmann
    email                : cullmann@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

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
  lmbSetsBreakpoints = true;
}

KateIconBorder::~KateIconBorder()
{
}

void KateIconBorder::paintLine(int i)
{
  if (!myView->myIconBorder) return;

  QPainter p(this);

    int fontHeight = myView->myDoc->viewFont.fontHeight;
    int y = i*fontHeight - myInternalView->yPos;
    p.fillRect(0, y, myInternalView->iconBorderWidth-2, fontHeight, colorGroup().background());
    p.setPen(white);
    p.drawLine(myInternalView->iconBorderWidth-2, y, myInternalView->iconBorderWidth-2, y + fontHeight);
    p.setPen(QColor(colorGroup().background()).dark());
    p.drawLine(myInternalView->iconBorderWidth-1, y, myInternalView->iconBorderWidth-1, y + fontHeight);

    if (i > myView->myDoc->lastLine())
      return;

    uint mark = myView->myDoc->mark (i);
    if (mark == 0)
      return;

    if (mark&KateDocument::markType01)
        p.drawPixmap(2, y, QPixmap(bookmark_xpm));

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
  if (!myView->myIconBorder)
    return;

  int lineStart = 0;
  int lineEnd = 0;

  QRect updateR = e->rect();

  // anders: drawing the background pr line is PAINFULLY slooow!!
  QPainter p(this);

  p.fillRect(0, updateR.y(), myInternalView->iconBorderWidth-2, updateR.height(), colorGroup().background());
  p.setPen(white);
  p.drawLine(myInternalView->iconBorderWidth-2, updateR.y(), myInternalView->iconBorderWidth-2, updateR.height());
  p.setPen(QColor(colorGroup().background()).dark());
  p.drawLine(myInternalView->iconBorderWidth-1, updateR.y(), myInternalView->iconBorderWidth-1, updateR.height());



  KateDocument *doc = myView->doc();
  int h = doc->viewFont.fontHeight;
  int yPos = myInternalView->yPos;

  if (h)
  {
    lineStart = (yPos + updateR.y()) / h;
    // anders: why paint below what is visible???
    int vl = (myView->myViewInternal->height()-4)+1; // number of lines the view can display +1 (to compensate for half lines)
    lineEnd = QMAX(((yPos + updateR.y() + updateR.height()) / h), QMIN(vl,(int)doc->numLines()));
  }

  for(int i = lineStart; i <= lineEnd; ++i)
    //paintLine(i);
    // anders: the paintLine function is SLOW!!!
    if ( myView->myDoc->mark(i) & KateDocument::markType01 )
      p.drawPixmap(2, (i - lineStart)*h, QPixmap(bookmark_xpm));
}


void KateIconBorder::mousePressEvent(QMouseEvent* e)
{
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
