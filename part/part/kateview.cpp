/* This file is part of the KDE libraries
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
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

#include "kateview.h"
#include "kateview.moc"

#include "katedocument.h"
#include "katecmd.h"
#include "katefactory.h"
#include "katehighlight.h"
#include "kateviewdialog.h"
#include "katedialogs.h"
#include "katefiledialog.h"

#include <kurldrag.h>
#include <qfocusdata.h>
#include <kdebug.h>
#include <kapp.h>
#include <kiconloader.h>
#include <qscrollbar.h>
#include <qiodevice.h>
#include <qclipboard.h>
#include <qpopupmenu.h>
#include <kpopupmenu.h>
#include <qkeycode.h>
#include <qintdict.h>
#include <klineeditdlg.h>
#include <qdropsite.h>
#include <qdragobject.h>
#include <kconfig.h>
#include <ksconfig.h>
#include <qfont.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qfileinfo.h>
#include <qfile.h>
#include <qevent.h>
#include <qdir.h>
#include <qvbox.h>
#include <qpaintdevicemetrics.h>

#include <qstyle.h>
#include <kcursor.h>
#include <klocale.h>
#include <kglobal.h>
#include <kcharsets.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <kstringhandler.h>
#include <kaction.h>
#include <kstdaction.h>
#include <kparts/event.h>
#include <kxmlguifactory.h>
#include <dcopclient.h>
#include <qregexp.h>

#include <kaccel.h>

#include "katetextline.h"
#include "kateiconborder.h"
#include "kateexportaction.h"

class KateLineRange
{
  public:
    int line;
    int startCol;
    int endCol;
    bool wrapped;
    int start;
    int end;
};

KateViewInternal::KateViewInternal(KateView *view, KateDocument *doc)
 : QWidget(view, "", Qt::WRepaintNoErase | Qt::WResizeNoErase)
{
  setBackgroundMode(NoBackground);

  myView = view;
  myDoc = doc;

//  iconBorderWidth  = 16;
//  iconBorderHeight = 800;

  numLines = 64;
  lineRanges = new KateLineRange[numLines];

  for (uint z = 0; z < numLines; z++)
  {
    lineRanges[z].start = 0xffffff;
    lineRanges[z].end = 0;
  }

  maxLen = 0;
  startLine = 0;
  endLine = -1;

  QWidget::setCursor(ibeamCursor);
  KCursor::setAutoHideCursor( this, true, true );

  setFocusPolicy(StrongFocus);

  xScroll = new QScrollBar(QScrollBar::Horizontal,myView);
  yScroll = new QScrollBar(QScrollBar::Vertical,myView);

  xPos = 0;
  yPos = 0;

  xCoord = 0;
  yCoord = 0;

  scrollTimer = 0;

  cursor.col = 0;
  cursor.line = 0;
  cursorOn = false;
  cursorTimer = 0;
  cXPos = 0;
  cOldXPos = 0;

  exposeCursor = false;
  updateState = 0;
  newXPos = -1;
  newYPos = -1;

  drawBuffer = new QPixmap ();
  drawBuffer->setOptimization (QPixmap::BestOptim);

  bm.sXPos = 0;
  bm.eXPos = -1;

  setAcceptDrops(true);
  dragInfo.state = diNone;

  connect(xScroll,SIGNAL(valueChanged(int)),SLOT(changeXPos(int)));
  connect(yScroll,SIGNAL(valueChanged(int)),SLOT(changeYPos(int)));
}

KateViewInternal::~KateViewInternal()
{
  delete [] lineRanges;
  delete drawBuffer;
}

void KateViewInternal::doCursorCommand(VConfig &c, int cmdNum)
{
  switch (cmdNum) {
    case KateView::cmLeft:
      cursorLeft(c);
      break;
    case KateView::cmRight:
      cursorRight(c);
      break;
    case KateView::cmWordLeft:
      wordLeft(c);
      break;
    case KateView::cmWordRight:
      wordRight(c);
      break;
    case KateView::cmHome:
      home(c);
      break;
    case KateView::cmEnd:
      end(c);
      break;
    case KateView::cmUp:
      cursorUp(c);
      break;
    case KateView::cmDown:
      cursorDown(c);
      break;
    case KateView::cmScrollUp:
      scrollUp(c);
      break;
    case KateView::cmScrollDown:
      scrollDown(c);
      break;
    case KateView::cmTopOfView:
      topOfView(c);
      break;
    case KateView::cmBottomOfView:
      bottomOfView(c);
      break;
    case KateView::cmPageUp:
      pageUp(c);
      break;
    case KateView::cmPageDown:
      pageDown(c);
      break;
    case KateView::cmTop:
      top_home(c);
      break;
    case KateView::cmBottom:
      bottom_end(c);
      break;
  }
}

void KateViewInternal::doEditCommand(VConfig &c, int cmdNum)
{
  switch (cmdNum) {
    case KateView::cmCopy:
      myDoc->copy(c.flags);
      return;
  }

  if (!myView->doc()->isReadWrite()) return;

  switch (cmdNum) {
    case KateView::cmReturn:
      if (c.flags & KateDocument::cfDelOnInput) myDoc->removeSelectedText();
      getVConfig(c);
      myDoc->newLine(c);
      return;
    case KateView::cmDelete:
      if ((c.flags & KateDocument::cfDelOnInput) && myDoc->hasSelection())
        myDoc->removeSelectedText();
      else myDoc->del(c);
      return;
    case KateView::cmBackspace:
      if ((c.flags & KateDocument::cfDelOnInput) && myDoc->hasSelection())
        myDoc->removeSelectedText();
      else
        myDoc->backspace(c.cursor.line, c.cursor.col);
      if ( (uint)c.cursor.line >= myDoc->lastLine() )
        leftBorder->update();
      return;
    case KateView::cmKillLine:
      myDoc->killLine(c);
      return;
    case KateView::cmCut:
      myDoc->cut(c);
      return;
    case KateView::cmPaste:
      if (c.flags & KateDocument::cfDelOnInput) myDoc->removeSelectedText();
      getVConfig(c);
      myDoc->paste(c);
      return;
    case KateView::cmIndent:
      myDoc->indent(c);
      return;
    case KateView::cmUnindent:
      myDoc->unIndent(c);
      return;
    case KateView::cmCleanIndent:
      myDoc->cleanIndent(c);
      return;
    case KateView::cmComment:
      myDoc->comment(c);
      return;
    case KateView::cmUncomment:
      myDoc->unComment(c);
      return;
  }
}

void KateViewInternal::cursorLeft(VConfig &c) {

  cursor.col--;
  if (c.flags & KateDocument::cfWrapCursor && cursor.col < 0 && cursor.line > 0) {
    cursor.line--;
    cursor.col = myDoc->textLength(cursor.line);
  }
  cOldXPos = cXPos = myDoc->textWidth(cursor);
  changeState(c);
}

void KateViewInternal::cursorRight(VConfig &c) {

  if (c.flags & KateDocument::cfWrapCursor) {
    if (cursor.col >= (int) myDoc->textLength(cursor.line)) {
      if (cursor.line == (int)myDoc->lastLine()) return;
      cursor.line++;
      cursor.col = -1;
    }
  }
  cursor.col++;
  cOldXPos = cXPos = myDoc->textWidth(cursor);
  changeState(c);
}

void KateViewInternal::wordLeft(VConfig &c) {
  Highlight *highlight;

  highlight = myDoc->highlight();
  TextLine::Ptr textLine = myDoc->getTextLine(cursor.line);

  if (cursor.col > 0) {
    do {
      cursor.col--;
    } while (cursor.col > 0 && !highlight->isInWord(textLine->getChar(cursor.col)));
    while (cursor.col > 0 && highlight->isInWord(textLine->getChar(cursor.col -1)))
      cursor.col--;
  } else {
    if (cursor.line > 0) {
      cursor.line--;
      textLine = myDoc->getTextLine(cursor.line);
      cursor.col = textLine->length();
    }
  }

  cOldXPos = cXPos = myDoc->textWidth(cursor);
  changeState(c);
}

void KateViewInternal::wordRight(VConfig &c) {
  Highlight *highlight;
  int len;

  highlight = myDoc->highlight();
  TextLine::Ptr textLine = myDoc->getTextLine(cursor.line);
  len = textLine->length();

  if (cursor.col < len) {
    do {
      cursor.col++;
    } while (cursor.col < len && highlight->isInWord(textLine->getChar(cursor.col)));
    while (cursor.col < len && !highlight->isInWord(textLine->getChar(cursor.col)))
      cursor.col++;
  } else {
    if (cursor.line < (int) myDoc->lastLine()) {
      cursor.line++;
      textLine = myDoc->getTextLine(cursor.line);
      cursor.col = 0;
    }
  }

  cOldXPos = cXPos = myDoc->textWidth(cursor);
  changeState(c);
}

void KateViewInternal::home(VConfig &c) {
  int lc;

  lc = (c.flags & KateDocument::cfSmartHome) ? myDoc->getTextLine(cursor.line)->firstChar() : 0;
  if (lc <= 0 || cursor.col == lc) {
    cursor.col = 0;
    cOldXPos = cXPos = 0;
  } else {
    cursor.col = lc;
    cOldXPos = cXPos = myDoc->textWidth(cursor);
  }

  changeState(c);
}

void KateViewInternal::end(VConfig &c) {
  cursor.col = myDoc->textLength(cursor.line);
  cOldXPos = cXPos = myDoc->textWidth(cursor);
  changeState(c);
}


void KateViewInternal::cursorUp(VConfig &c) {

  cursor.line--;
  cXPos = myDoc->textWidth(c.flags & KateDocument::cfWrapCursor,cursor,cOldXPos);
  changeState(c);
}


void KateViewInternal::cursorDown(VConfig &c) {
  int x;

  if (cursor.line == (int)myDoc->lastLine()) {
    x = myDoc->textLength(cursor.line);
    if (cursor.col >= x) return;
    cursor.col = x;
    cXPos = myDoc->textWidth(cursor);
  } else {
    cursor.line++;
    cXPos = myDoc->textWidth(c.flags & KateDocument::cfWrapCursor, cursor, cOldXPos);
  }
  changeState(c);
}

void KateViewInternal::scrollUp(VConfig &c) {

  if (! yPos) return;

  newYPos = yPos - myDoc->viewFont.fontHeight;
  if (cursor.line == (yPos + height())/myDoc->viewFont.fontHeight -1) {
    cursor.line--;
    cXPos = myDoc->textWidth(c.flags & KateDocument::cfWrapCursor,cursor,cOldXPos);

    changeState(c);
  }
}

void KateViewInternal::scrollDown(VConfig &c) {

  if (endLine >= (int)myDoc->lastLine()) return;

  newYPos = yPos + myDoc->viewFont.fontHeight;
  if (cursor.line == (yPos + myDoc->viewFont.fontHeight -1)/myDoc->viewFont.fontHeight) {
    cursor.line++;
    cXPos = myDoc->textWidth(c.flags & KateDocument::cfWrapCursor,cursor,cOldXPos);
    changeState(c);
  }
}

void KateViewInternal::topOfView(VConfig &c) {

  cursor.line = (yPos + myDoc->viewFont.fontHeight -1)/myDoc->viewFont.fontHeight;
  cursor.col = 0;
  cOldXPos = cXPos = 0;
  changeState(c);
}

void KateViewInternal::bottomOfView(VConfig &c) {

  cursor.line = (yPos + height())/myDoc->viewFont.fontHeight -1;
  if (cursor.line < 0) cursor.line = 0;
  if (cursor.line > (int)myDoc->lastLine()) cursor.line = myDoc->lastLine();
  cursor.col = 0;
  cOldXPos = cXPos = 0;
  changeState(c);
}

void KateViewInternal::pageUp(VConfig &c) {
  int lines = (endLine - startLine - 1);

  if (lines <= 0) lines = 1;

  if (yPos > 0) {
    newYPos = yPos - lines * myDoc->viewFont.fontHeight;
    if (newYPos < 0) newYPos = 0;
  }
  cursor.line -= lines;
  cXPos = myDoc->textWidth(c.flags & KateDocument::cfWrapCursor, cursor, cOldXPos);
  changeState(c);
//  cursorPageUp(c);
}

void KateViewInternal::pageDown(VConfig &c) {

  int lines = (endLine - startLine - 1);

  if (endLine < (int)myDoc->lastLine()) {
    if (lines < (int)myDoc->lastLine() - endLine)
      newYPos = yPos + lines * myDoc->viewFont.fontHeight;
    else
      newYPos = yPos + (myDoc->lastLine() - endLine) * myDoc->viewFont.fontHeight;
  }
  cursor.line += lines;
  cXPos = myDoc->textWidth(c.flags & KateDocument::cfWrapCursor,cursor,cOldXPos);
  changeState(c);
//  cursorPageDown(c);
}

// go to the top, same X position
void KateViewInternal::top(VConfig &c) {

//  cursor.col = 0;
  cursor.line = 0;
  cXPos = myDoc->textWidth(c.flags & KateDocument::cfWrapCursor,cursor,cOldXPos);
//  cOldXPos = cXPos = 0;
  changeState(c);
}

// go to the bottom, same X position
void KateViewInternal::bottom(VConfig &c) {

//  cursor.col = 0;
  cursor.line = myDoc->lastLine();
  cXPos = myDoc->textWidth(c.flags & KateDocument::cfWrapCursor,cursor,cOldXPos);
//  cOldXPos = cXPos = 0;
  changeState(c);
}

// go to the top left corner
void KateViewInternal::top_home(VConfig &c)
{
  cursor.line = 0;
  cursor.col = 0;
  cOldXPos = cXPos = 0;
  changeState(c);
}

// go to the bottom right corner
void KateViewInternal::bottom_end(VConfig &c) {

  cursor.line = myDoc->lastLine();
  cursor.col = myDoc->textLength(cursor.line);
  cOldXPos = cXPos = myDoc->textWidth(cursor);
  changeState(c);
}


void KateViewInternal::changeXPos(int p) {
  int dx;

  dx = xPos - p;
  xPos = p;
  if (QABS(dx) < width()) scroll(dx, 0); else update();
}

void KateViewInternal::changeYPos(int p) {
  int dy;

  dy = yPos - p;
  yPos = p;
  clearDirtyCache(height());

  if (QABS(dy) < height())
  {
    scroll(0, dy);
    leftBorder->repaint();//scroll(0, dy);
  }
  else
    update();

  updateView(KateView::ufDocGeometry);
}


void KateViewInternal::getVConfig(VConfig &c) {

  c.view = myView;
  c.cursor = cursor;
  c.cXPos = cXPos;
  c.flags = myView->myDoc->_configFlags;
}

void KateViewInternal::changeState(VConfig &c) {
  /*
   * we need to be sure to kill the selection on an attempted cursor
   * movement even if the cursor doesn't physically move,
   * but we need to be careful not to do some other things in this case,
   * like we don't want to expose the cursor
   */

//  if (cursor.col == c.cursor.col && cursor.line == c.cursor.line) return;
  bool nullMove = (cursor.col == c.cursor.col && cursor.line == c.cursor.line);

//  if (cursor.line != c.cursor.line || c.flags & KateDocument::cfMark) myDoc->recordReset();

  if (! nullMove) {

    exposeCursor = true;

    // mark old position of cursor as dirty
    if (cursorOn) {
      tagLines(c.cursor.line, c.cursor.line, c.cXPos, c.cXPos + myDoc->charWidth(c.cursor));
      cursorOn = false;
    }

    // mark old bracket mark position as dirty
    if (bm.sXPos < bm.eXPos) {
      tagLines(bm.cursor.line, bm.cursor.line, bm.sXPos, bm.eXPos);
    }
    // make new bracket mark
    myDoc->newBracketMark(cursor, bm);

    // remove trailing spaces when leaving a line
    if (c.flags & KateDocument::cfRemoveSpaces && cursor.line != c.cursor.line) {
      TextLine::Ptr textLine = myDoc->getTextLine(c.cursor.line);
      unsigned int newLen = textLine->lastChar();
      if (newLen != textLine->length()) {
        textLine->truncate(newLen);
        // if some spaces are removed, tag the line as dirty
        myDoc->tagLines(c.cursor.line, c.cursor.line);
      }
    }
  }

  if (c.flags & KateDocument::cfMark) {
    if (! nullMove)
      myDoc->selectTo(c, cursor, cXPos);
  } else {
    if (!(c.flags & KateDocument::cfPersistent))
      myDoc->clearSelection();
  }
}

void KateViewInternal::insLine(int line) {

  if (line <= cursor.line) {
    cursor.line++;
  }
  if (line < startLine) {
    startLine++;
    endLine++;
    yPos += myDoc->viewFont.fontHeight;
  } else if (line <= endLine) {
    tagAll();
  }
}

void KateViewInternal::delLine(int line) {

  if (line <= cursor.line && cursor.line > 0) {
    cursor.line--;
  }
  if (line < startLine) {
    startLine--;
    endLine--;
    yPos -= myDoc->viewFont.fontHeight;
  } else if (line <= endLine) {
    tagAll();
  }
}

void KateViewInternal::updateCursor()
{
  cOldXPos = cXPos = myDoc->textWidth(cursor);
}


void KateViewInternal::updateCursor(KateTextCursor &newCursor)
{
  if (!(myDoc->_configFlags & KateDocument::cfPersistent)) myDoc->clearSelection();

  exposeCursor = true;
  if (cursorOn) {
    tagLines(cursor.line, cursor.line, cXPos, cXPos +myDoc->charWidth(cursor));
    cursorOn = false;
  }

  if (bm.sXPos < bm.eXPos) {
    tagLines(bm.cursor.line, bm.cursor.line, bm.sXPos, bm.eXPos);
  }
  myDoc->newBracketMark(newCursor, bm);

  cursor = newCursor;
  cOldXPos = cXPos = myDoc->textWidth(cursor);
}

// init the line dirty cache
void KateViewInternal::clearDirtyCache(int height) {
  int lines, z;

  // calc start and end line of visible part
  startLine = yPos/myDoc->viewFont.fontHeight;
  endLine = (yPos + height -1)/myDoc->viewFont.fontHeight;

  updateState = 0;

  lines = endLine - startLine +1;
  if (lines > numLines) { // resize the dirty cache
    numLines = lines*2;
    delete [] lineRanges;
    lineRanges = new KateLineRange[numLines];
  }

  for (z = 0; z < lines; z++) { // clear all lines
    lineRanges[z].start = 0xffffff;
    lineRanges[z].end = 0;
  }
  newXPos = newYPos = -1;
}

void KateViewInternal::tagLines(int start, int end, int x1, int x2) {
  KateLineRange *r;
  int z;

  start -= startLine;
  if (start < 0) start = 0;
  end -= startLine;
  if (end > endLine - startLine) end = endLine - startLine;

  if (x1 <= 0) x1 = 0;
  if (x1 < xPos-2) x1 = xPos;
  if (x2 > width() + xPos) x2 = width() + xPos;
  if (x1 >= x2) return;

  r = &lineRanges[start];
  for (z = start; z <= end; z++) {
    if (x1 < r->start) r->start = x1;
    if (x2 > r->end) r->end = x2;
    r++;
    updateState |= 1;
  }
}

void KateViewInternal::tagAll() {
  updateState = 3;
}

void KateViewInternal::setPos(int x, int y) {
  newXPos = x;
  newYPos = y;
}

void KateViewInternal::center() {
  newXPos = 0;
  newYPos = cursor.line*myDoc->viewFont.fontHeight - height()/2;
  if (newYPos < 0) newYPos = 0;
}

void KateViewInternal::updateView(int flags) {
  int fontHeight;
  int oldXPos, oldYPos;
  int w, h;
  int z;
  bool b;
  int xMax, yMax;
  int cYPos;
  int cXPosMin, cXPosMax, cYPosMin, cYPosMax;
  int dx, dy;
  int pageScroll;
  int scrollbarWidth = style().scrollBarExtent().width();
  int bw = 0; // width of borders

  if (flags & KateView::ufDocGeometry || ! maxLen )
  {
    maxLen = 0;

    if (!myView->_hasWrap)
    {
      for (int tline = startLine; (tline <= endLine) && (tline <= myDoc->lastLine ()); tline++)
      {
        uint len = myDoc->textWidth (myDoc->getTextLine (tline), myDoc->getTextLine (tline)->length());

        if (len > maxLen)
          maxLen = len;
      }

      maxLen = maxLen + 8;
    }
  }
//kdDebug()<<"widest line to draw is "<<maxLen<<" px"<<endl;
  if (exposeCursor || flags & KateView::ufDocGeometry) {
    emit myView->cursorPositionChanged();
  } else {
    // anders: I stay for KateView::ufLeftBorder, to get xcroll updated when border elements
    // display change.
    if ( updateState == 0 && newXPos < 0 && newYPos < 0 && !( flags&KateView::ufLeftBorder ) ) return;
  }

  if (cursorTimer) {
    killTimer(cursorTimer);
    cursorTimer = startTimer(KApplication::cursorFlashTime() / 2);
    cursorOn = true;
  }

  oldXPos = xPos;
  oldYPos = yPos;

  if (newXPos >= 0) xPos = newXPos;
  if (newYPos >= 0) yPos = newYPos;

  fontHeight = myDoc->viewFont.fontHeight;
  cYPos = cursor.line*fontHeight;

  bw = leftBorder->width();
  z = 0;
  do {
    w = myView->width();
    h = myView->height();

    xMax = maxLen - (w - bw);
    b = (xPos > 0 || xMax > 0);
    if (b) h -= scrollbarWidth;
    yMax = myDoc->textHeight() - h;
    if (yPos > 0 || yMax > 0) {
      w -= scrollbarWidth;
      xMax += scrollbarWidth;
      if (!b && xMax > 0) {
        h -= scrollbarWidth;
        yMax += scrollbarWidth;
      }
    }

    if (!exposeCursor) break;
//    if (flags & KateView::ufNoScroll) break;
/*
    if (flags & KateView::ufCenter) {
      cXPosMin = xPos + w/3;
      cXPosMax = xPos + (w*2)/3;
      cYPosMin = yPos + h/3;
      cYPosMax = yPos + ((h - fontHeight)*2)/3;
    } else {*/
      cXPosMin = xPos+ 4;
      cXPosMax = xPos + w - 8 - bw;
      cYPosMin = yPos;
      cYPosMax = yPos + (h - fontHeight);
//    }

    if (cXPos < cXPosMin) {
      xPos -= cXPosMin - cXPos;
    }
    if (xPos < 0) xPos = 0;
    if (cXPos > cXPosMax) {
      xPos += cXPos - cXPosMax;
    }
    if (cYPos < cYPosMin) {
      yPos -= cYPosMin - cYPos;
    }
    if (yPos < 0) yPos = 0;
    if (cYPos > cYPosMax) {
      yPos += cYPos - cYPosMax;
    }

    z++;
  } while (z < 2);
//kdDebug()<<"x scroll, afaik: "<<xMax<<endl;
  if (xMax < xPos) xMax = xPos;
  if (yMax < yPos) yMax = yPos;

  if ((!myView->_hasWrap) && (xMax > 0)) {
    pageScroll = w - (w % fontHeight) - fontHeight;
    if (pageScroll <= 0)
      pageScroll = fontHeight;

    xScroll->blockSignals(true);
    xScroll->setGeometry(0,h,w,scrollbarWidth);
    xScroll->setRange(0,xMax);
    xScroll->setValue(xPos);
    xScroll->setSteps(fontHeight,pageScroll);
    xScroll->blockSignals(false);
    xScroll->show();
  }
  else xScroll->hide();

  if (yMax > 0) {
    pageScroll = h - (h % fontHeight) - fontHeight;
    if (pageScroll <= 0)
      pageScroll = fontHeight;

    yScroll->blockSignals(true);
    yScroll->setGeometry(w,0,scrollbarWidth,myView->height()-scrollbarWidth);
    yScroll->setRange(0,yMax);
    yScroll->setValue(yPos);
    yScroll->setSteps(fontHeight,pageScroll);
    yScroll->blockSignals(false);
    yScroll->show();
  } else yScroll->hide();

  w -= bw;
  if (w != width() || h != height()) {
    clearDirtyCache(h);
    resize(w,h);
  } else {
    dx = oldXPos - xPos;
    dy = oldYPos - yPos;

    b = updateState == 3;
    if (flags & KateView::ufUpdateOnScroll) {
      b |= dx || dy;
    } else {
      b |= QABS(dx)*3 > w*2 || QABS(dy)*3 > h*2;
    }

    if (b) {
      clearDirtyCache(h);
      update();
    } else {
      if (dy)
        leftBorder->scroll(0, dy);
      if (updateState > 0) paintTextLines(oldXPos, oldYPos);
      clearDirtyCache(h);

      if (dx || dy) {
        scroll(dx,dy);
      }
      if (cursorOn) paintCursor();
      if (bm.eXPos > bm.sXPos) paintBracketMark();
    }
  }

  exposeCursor = false;
}


void KateViewInternal::paintTextLines(int xPos, int yPos)
{
  if (!drawBuffer) return;
  if (drawBuffer->isNull()) return;

  QPainter paint;
  paint.begin(drawBuffer);

  uint h = myDoc->viewFont.fontHeight;
  KateLineRange *r = lineRanges;

  for (uint line = startLine; line <= endLine; line++)
  {
    if (r->start < r->end)
    {
      myDoc->paintTextLine(paint, line, r->start, r->end, myView->myDoc->_configFlags & KateDocument::cfShowTabs);
      bitBlt(this, r->start - xPos, line*h - yPos, drawBuffer, 0, 0, r->end - r->start, h);
      leftBorder->paintLine(line);
    }
    
    r++;
  }

  paint.end();
}

void KateViewInternal::paintCursor() {
  int h, w,w2,y, x;
  static int cx = 0, cy = 0, ch = 0;

  h = myDoc->viewFont.fontHeight;
  y = h*cursor.line - yPos;
  x = cXPos - xPos;

  if(myDoc->viewFont.myFont != font()) setFont(myDoc->viewFont.myFont);
  if(cx != x || cy != y || ch != h){
    cx = x;
    cy = y;
    ch = h;
    setMicroFocusHint(cx, cy, 0, ch);
  }

  w2 = myDoc->charWidth(cursor);
  w = myView->isOverwriteMode() ? w2 : 2;

  xCoord = x;
  yCoord = y+h;

  QPainter paint;
  if (cursorOn)
  {
    QColor &fg = myDoc->cursorCol(cursor.col,cursor.line);
    QColor &bg = myDoc->backCol(cursor.col, cursor.line);
    QColor xor_fg (qRgb(fg.red()^bg.red(), fg.green()^bg.green(), fg.blue()^bg.blue()),
                   fg.pixel()^bg.pixel());

    paint.begin(this);
    paint.setClipping(false);
    paint.setPen(myDoc->cursorCol(cursor.col,cursor.line));
    paint.setRasterOp(XorROP);

    //h += y - 1;
    paint.fillRect(x, y, w, h, xor_fg);
    paint.end();
   }
   else
   {
     tagLines( cursor.line, cursor.line, 0, 0xffff);
     paintTextLines (xPos, yPos);
  }
}

void KateViewInternal::paintBracketMark() {
  int y;

  y = myDoc->viewFont.fontHeight*(bm.cursor.line +1) - yPos -1;

  QPainter paint;
  paint.begin(this);
  paint.setPen(myDoc->cursorCol(bm.cursor.col, bm.cursor.line));

  paint.drawLine(bm.sXPos - xPos, y, bm.eXPos - xPos -1, y);
  paint.end();
}

void KateViewInternal::placeCursor(int x, int y, int flags) {
  VConfig c;

  getVConfig(c);
  c.flags |= flags;
  cursor.line = (yPos + y)/myDoc->viewFont.fontHeight;
  cXPos = cOldXPos = myDoc->textWidth(c.flags & KateDocument::cfWrapCursor, cursor, xPos + x);
  changeState(c);
}

// given physical coordinates, report whether the text there is selected
bool KateViewInternal::isTargetSelected(int x, int y) {

  y = (yPos + y) / myDoc->viewFont.fontHeight;

  TextLine::Ptr line = myDoc->getTextLine(y);
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

void KateViewInternal::keyPressEvent(QKeyEvent *e) {
  VConfig c;
  getVConfig(c);

  if (myView->doc()->isReadWrite()) {
    if (c.flags & KateDocument::cfTabIndents && myDoc->hasSelection()) {
      if (e->key() == Qt::Key_Tab) {
        myDoc->indent(c);
        myDoc->updateViews();
        return;
      }
      if (e->key() == Qt::Key_Backtab) {
        myDoc->unIndent(c);
        myDoc->updateViews();
        return;
      }
    }
    if ( !(e->state() & ControlButton ) && myDoc->insertChars (c.cursor.line, c.cursor.col, e->text(), this->myView) )
    {
      myDoc->updateViews();
      e->accept();
      return;
    }
  }
  e->ignore();
}

void KateViewInternal::mousePressEvent(QMouseEvent *e) {

  if (e->button() == LeftButton) {

    if (isTargetSelected(e->x(), e->y())) {
      // we have a mousedown on selected text
      // we initialize the drag info thingy as pending from this position

      dragInfo.state = diPending;
      dragInfo.start.col = e->x();
      dragInfo.start.line = e->y();
    } else {
      // we have no reason to ever start a drag from here
      dragInfo.state = diNone;

      int flags;

      flags = 0;
      if (e->state() & ShiftButton) {
        flags |= KateDocument::cfMark;
        if (e->state() & ControlButton) flags |= KateDocument::cfMark | KateDocument::cfKeepSelection;
      }
      placeCursor(e->x(), e->y(), flags);
      scrollX = 0;
      scrollY = 0;
      if (!scrollTimer) scrollTimer = startTimer(50);
      myDoc->updateViews();
    }
  }
  if (e->button() == MidButton) {
    placeCursor(e->x(), e->y());
    if (myView->doc()->isReadWrite())
    {
      QApplication::clipboard()->setSelectionMode( true );
      myView->paste();
      QApplication::clipboard()->setSelectionMode( false );
    }
  }
  if (myView->rmbMenu && e->button() == RightButton) {
    myView->rmbMenu->popup(mapToGlobal(e->pos()));
  }
  myView->mousePressEvent(e); // this doesn't do anything, does it?
  // it does :-), we need this for KDevelop, so please don't uncomment it again -Sandy
}

void KateViewInternal::mouseDoubleClickEvent(QMouseEvent *e) {

  if (e->button() == LeftButton) {
    VConfig c;
    getVConfig(c);
    myDoc->selectWord(c.cursor, c.flags);
    myDoc->updateViews();
  }
}

void KateViewInternal::mouseReleaseEvent(QMouseEvent *e) {

  if (e->button() == LeftButton) {
    if (dragInfo.state == diPending) {
      // we had a mouse down in selected area, but never started a drag
      // so now we kill the selection
      placeCursor(e->x(), e->y(), 0);
      myDoc->updateViews();
    } else if (dragInfo.state == diNone)
    {
      QApplication::clipboard()->setSelectionMode( true );
      myView->copy();
      QApplication::clipboard()->setSelectionMode( false );

      killTimer(scrollTimer);
      scrollTimer = 0;
    }
    dragInfo.state = diNone;
  }
}

void KateViewInternal::mouseMoveEvent(QMouseEvent *e) {

  if (e->state() & LeftButton) {
    int flags;
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
//debug("modifiers %d", ((KGuiCmdApp *) kapp)->getModifiers());
    flags = KateDocument::cfMark;
    if (e->state() & ControlButton) flags |= KateDocument::cfKeepSelection;
    placeCursor(mouseX, mouseY, flags);
    myDoc->updateViews();
  }
}



void KateViewInternal::wheelEvent( QWheelEvent *e )
{
  if( yScroll->isVisible() == true )
  {
    QApplication::sendEvent( yScroll, e );
  }
}



void KateViewInternal::paintEvent(QPaintEvent *e) {
  int xStart, xEnd;
  int h;
  int line, y, yEnd;

  QRect updateR = e->rect();

  if (!drawBuffer) return;
  if (drawBuffer->isNull()) return;

  QPainter paint;
  paint.begin(drawBuffer);

  xStart = xPos + updateR.x();
  xEnd = xStart + updateR.width();

  h = myDoc->viewFont.fontHeight;
  line = (yPos + updateR.y()) / h;
  y = line*h - yPos;
  yEnd = updateR.y() + updateR.height();

  while (y < yEnd)
  {
    myDoc->paintTextLine(paint, line, xStart, xEnd, myView->myDoc->_configFlags & KateDocument::cfShowTabs);
    bitBlt(this, updateR.x(), y, drawBuffer, 0, 0, updateR.width(), h);
    leftBorder->paintLine(line);
    line++;
    y += h;
  }
  paint.end();
  if (cursorOn) paintCursor();
  if (bm.eXPos > bm.sXPos) paintBracketMark();
}

void KateViewInternal::resizeEvent(QResizeEvent *)
{
  drawBuffer->resize (width(), myDoc->viewFont.fontHeight);
  leftBorder->resize(leftBorder->width(), height());
}

void KateViewInternal::timerEvent(QTimerEvent *e) {
  if (e->timerId() == cursorTimer) {
    cursorOn = !cursorOn;
    paintCursor();
  }
  if (e->timerId() == scrollTimer && (scrollX | scrollY)) {
    xScroll->setValue(xPos + scrollX);
    yScroll->setValue(yPos + scrollY);

    placeCursor(mouseX, mouseY, KateDocument::cfMark);
    myDoc->updateViews();
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

    if (QTextDrag::decode(event, text)) {
      bool      priv, selected;

      // is the source our own document?
      priv = myDoc->ownedView((KateView*)(event->source()));
      // dropped on a text selection area?
      selected = isTargetSelected(event->pos().x(), event->pos().y());

      if (priv && selected) {
        // this is a drag that we started and dropped on our selection
        // ignore this case
        return;
      }

      VConfig c;
      KateTextCursor cursor;

      getVConfig(c);
      cursor = c.cursor;

      if (priv) {
        // this is one of mine (this document), not dropped on the selection
        if (event->action() == QDropEvent::Move) {
          myDoc->removeSelectedText();
          getVConfig(c);
          cursor = c.cursor;
        } else {
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

      myDoc->updateViews();
    }
  }
}

KateView::KateView(KateDocument *doc, QWidget *parent, const char * name) : Kate::View (doc, parent, name)
    , extension( 0 )
{
  m_editAccels=0;
  setInstance( KateFactory::instance() );

  initCodeCompletionImplementation();

  active = false;
  iconBorderStatus = KateIconBorder::None;
  _hasWrap = false;

  myDoc = doc;
  myViewInternal = new KateViewInternal (this,doc);
  myViewInternal->leftBorder = new KateIconBorder(this, myViewInternal);
  myViewInternal->leftBorder->setGeometry(0, 0, myViewInternal->leftBorder->width(), myViewInternal->iconBorderHeight);
  myViewInternal->leftBorder->hide();
  myViewInternal->leftBorder->installEventFilter( this );
  doc->addView( this );

  connect(myViewInternal,SIGNAL(dropEventPass(QDropEvent *)),this,SLOT(dropEventPassEmited(QDropEvent *)));

  replacePrompt = 0L;
  rmbMenu = 0L;

  setFocusProxy( myViewInternal );
  myViewInternal->setFocus();
  resize(parent->width() -4, parent->height() -4);

  myViewInternal->installEventFilter( this );

  if (!doc->m_bSingleViewMode)
  {
    setXMLFile( "katepartui.rc" );
  }
  else
  {
    if (doc->m_bReadOnly)
      myDoc->setXMLFile( "katepartreadonlyui.rc" );
    else
      myDoc->setXMLFile( "katepartui.rc" );

    if (doc->m_bBrowserView)
    {
      extension = new KateBrowserExtension( myDoc, this );
    }
  }

  setupActions();
  setupEditKeys();

  connect( this, SIGNAL( newStatus() ), this, SLOT( slotUpdate() ) );
  connect( doc, SIGNAL( undoChanged() ), this, SLOT( slotNewUndo() ) );
  connect( doc, SIGNAL( fileNameChanged() ), this, SLOT( slotFileStatusChanged() ) );

  if ( doc->m_bBrowserView )
  {
    connect( this, SIGNAL( dropEventPass(QDropEvent*) ), this, SLOT( slotDropEventPass(QDropEvent*) ) );
  }

  slotUpdate();
  myViewInternal->updateView (KateView::ufDocGeometry);
}


KateView::~KateView()
{
  if (myDoc && !myDoc->m_bSingleViewMode)
    myDoc->removeView( this );

  delete myViewInternal;
  delete myCC_impl;
}

void KateView::initCodeCompletionImplementation()
{
  myCC_impl=new CodeCompletion_Impl(this);
  connect(myCC_impl,SIGNAL(completionAborted()),this,SIGNAL(completionAborted()));
  connect(myCC_impl,SIGNAL(completionDone()),this,SIGNAL(completionDone()));
  connect(myCC_impl,SIGNAL(argHintHidden()),this,SIGNAL(argHintHidden()));
  connect(myCC_impl,SIGNAL(completionDone(KTextEditor::CompletionEntry)),this,SIGNAL(completionDone(KTextEditor::CompletionEntry)));
  connect(myCC_impl,SIGNAL(filterInsertString(KTextEditor::CompletionEntry*,QString *)),this,SIGNAL(filterInsertString(KTextEditor::CompletionEntry*,QString *)));
}

QPoint KateView::cursorCoordinates()
{
  return QPoint(myViewInternal->xCoord, myViewInternal->yCoord);
}

void KateView::copy () const
{
  myDoc->copy(myDoc->_configFlags);
}

void KateView::setupEditKeys()
{

  if (m_editAccels) delete m_editAccels;
  m_editAccels=new KAccel(this,this);
  m_editAccels->insert("KATE_CURSOR_LEFT",i18n("Cursor left"),"","Left",this,SLOT(cursorLeft()));
  m_editAccels->insert("KATE_WORD_LEFT",i18n("One word left"),"","Ctrl+Left",this,SLOT(wordLeft()));
  m_editAccels->insert("KATE_CURSOR_LEFT_SELECT",i18n("Cursor left + SELECT"),"","Shift+Left",this,SLOT(shiftCursorLeft()));
  m_editAccels->insert("KATE_WORD_LEFT_SELECT",i18n("One word left + SELECT"),"","Shift+Ctrl+Left",this,SLOT(shiftWordLeft()));


  m_editAccels->insert("KATE_CURSOR_RIGHT",i18n("Cursor right"),"","Right",this,SLOT(cursorRight()));
  m_editAccels->insert("KATE_WORD_RIGHT",i18n("One word right"),"","Ctrl+Right",this,SLOT(wordRight()));
  m_editAccels->insert("KATE_CURSOR_RIGHT_SELECT",i18n("Cursor right + SELECT"),"","Shift+Right",this,SLOT(shiftCursorRight()));
  m_editAccels->insert("KATE_WORD_RIGHT_SELECT",i18n("One word right + SELECT"),"","Shift+Ctrl+Right",this,SLOT(shiftWordRight()));

  m_editAccels->insert("KATE_CURSOR_HOME",i18n("Home"),"","Home",this,SLOT(home()));
  m_editAccels->insert("KATE_CURSOR_TOP",i18n("Top"),"","Ctrl+Home",this,SLOT(top()));
  m_editAccels->insert("KATE_CURSOR_HOME_SELECT",i18n("Home + SELECT"),"","Shift+Home",this,SLOT(shiftHome()));
  m_editAccels->insert("KATE_CURSOR_TOP_SELECT",i18n("Top + SELECT"),"","Shift+Ctrl+Home",this,SLOT(shiftTop()));

  m_editAccels->insert("KATE_CURSOR_END",i18n("End"),"","End",this,SLOT(end()));
  m_editAccels->insert("KATE_CURSOR_BOTTOM",i18n("Bottom"),"","Ctrl+End",this,SLOT(bottom()));
  m_editAccels->insert("KATE_CURSOR_END_SELECT",i18n("End + SELECT"),"","Shift+End",this,SLOT(shiftEnd()));
  m_editAccels->insert("KATE_CURSOR_BOTTOM_SELECT",i18n("Bottom + SELECT"),"","Shift+Ctrl+End",this,SLOT(shiftBottom()));

  m_editAccels->insert("KATE_CURSOR_UP",i18n("Cursor up"),"","Up",this,SLOT(up()));
  m_editAccels->insert("KATE_CURSOR_UP_SELECT",i18n("Cursor up + SELECT"),"","Shift+Up",this,SLOT(shiftUp()));
  m_editAccels->insert("KATE_SCROLL_UP",i18n("Scroll one line up"),"","Ctrl+Up",this,SLOT(scrollUp()));

  m_editAccels->insert("KATE_CURSOR_DOWN",i18n("Cursor down"),"","Down",this,SLOT(down()));
  m_editAccels->insert("KATE_CURSOR_DOWN_SELECT",i18n("Cursor down + SELECT"),"","Shift+Down",this,SLOT(shiftDown()));
  m_editAccels->insert("KATE_SCROLL_DOWN",i18n("Scroll one line down"),"","Ctrl+Down",this,SLOT(scrollDown()));

  KConfig config("kateeditkeysrc");
  m_editAccels->readSettings(&config);

  if (!(myViewInternal->hasFocus())) m_editAccels->setEnabled(false);
}

void KateView::setupActions()
{
  KActionCollection *ac = this->actionCollection ();

  if (myDoc->m_bSingleViewMode)
  {
    ac = myDoc->actionCollection ();
  }

  if (!myDoc->m_bReadOnly)
  {
    KStdAction::save(this, SLOT(save()), ac);
    editUndo = KStdAction::undo(myDoc, SLOT(undo()), ac);
    editRedo = KStdAction::redo(myDoc, SLOT(redo()), ac);
    KStdAction::cut(this, SLOT(cut()), ac);
    KStdAction::paste(this, SLOT(paste()), ac);
    new KAction(i18n("Apply Word Wrap"), "", 0, myDoc, SLOT(applyWordWrap()), ac, "edit_apply_wordwrap");
    KStdAction::replace(this, SLOT(replace()), ac);
    new KAction(i18n("Editing Co&mmand"), Qt::CTRL+Qt::Key_M, this, SLOT(slotEditCommand()), ac, "edit_cmd");

    // setup Tools menu
    KStdAction::spelling(myDoc, SLOT(spellcheck()), ac);
    new KAction(i18n("&Indent"), "indent", Qt::CTRL+Qt::Key_I, this, SLOT(indent()),
                              ac, "tools_indent");
    new KAction(i18n("&Unindent"), "unindent", Qt::CTRL+Qt::Key_U, this, SLOT(unIndent()),
                                ac, "tools_unindent");
    new KAction(i18n("&Clean Indentation"), 0, this, SLOT(cleanIndent()),
                                   ac, "tools_cleanIndent");
    new KAction(i18n("C&omment"), CTRL+Qt::Key_NumberSign, this, SLOT(comment()),
                               ac, "tools_comment");
    new KAction(i18n("Unco&mment"), CTRL+SHIFT+Qt::Key_NumberSign, this, SLOT(uncomment()),
                                 ac, "tools_uncomment");
  }

  KStdAction::close( this, SLOT(flush()), ac, "file_close" );
  KStdAction::copy(this, SLOT(copy()), ac);

  KStdAction::saveAs(this, SLOT(saveAs()), ac);
  KStdAction::find(this, SLOT(find()), ac);
  KStdAction::findNext(this, SLOT(findAgain()), ac);
  KStdAction::findPrev(this, SLOT(findPrev()), ac, "edit_find_prev");
  KStdAction::gotoLine(this, SLOT(gotoLine()), ac);
  new KAction(i18n("&Configure Editor..."), 0, myDoc, SLOT(configDialog()),ac, "set_confdlg");
  setHighlight = myDoc->hlActionMenu (i18n("&Highlight Mode"),ac,"set_highlight");
  myDoc->exportActionMenu (i18n("&Export"),ac,"file_export");
  KStdAction::selectAll(myDoc, SLOT(selectAll()), ac);
  KStdAction::deselect(myDoc, SLOT(clearSelection()), ac);
  new KAction(i18n("Increase Font Sizes"), "viewmag+", 0, this, SLOT(slotIncFontSizes()), ac, "incFontSizes");
  new KAction(i18n("Decrease Font Sizes"), "viewmag-", 0, this, SLOT(slotDecFontSizes()), ac, "decFontSizes");
  new KAction(i18n("&Toggle Block Selection"), Key_F4, myDoc, SLOT(toggleBlockSelectionMode()), ac, "set_verticalSelect");
  new KToggleAction(i18n("Show &Icon Border"), Key_F6, this, SLOT(toggleIconBorder()), ac, "view_border");
  new KToggleAction(i18n("Show &Line Numbers"), Key_F11, this, SLOT(toggleLineNumbersOn()), ac, "view_line_numbers");
  bookmarkMenu = new KActionMenu(i18n("&Bookmarks"), ac, "bookmarks");

  // setup bookmark menu
  bookmarkToggle = new KAction(i18n("Toggle &Bookmark"), Qt::CTRL+Qt::Key_B, this, SLOT(toggleBookmark()), ac, "edit_bookmarkToggle");
  bookmarkClear = new KAction(i18n("Clear Bookmarks"), 0, myDoc, SLOT(clearMarks()), ac, "edit_bookmarksClear");

  // connect bookmarks menu aboutToshow
  connect(bookmarkMenu->popupMenu(), SIGNAL(aboutToShow()), this, SLOT(bookmarkMenuAboutToShow()));

  QStringList list;
  setEndOfLine = new KSelectAction(i18n("&End of Line"), 0, ac, "set_eol");
  connect(setEndOfLine, SIGNAL(activated(int)), this, SLOT(setEol(int)));
  list.clear();
  list.append("&Unix");
  list.append("&Windows/Dos");
  list.append("&Macintosh");
  setEndOfLine->setItems(list);
}

void KateView::slotUpdate()
{
  slotNewUndo();
}

void KateView::slotFileStatusChanged()
{
  int eol = getEol();
  eol = eol>=1 ? eol : 0;

    setEndOfLine->setCurrentItem(eol);
}

void KateView::slotNewUndo()
{
  if (myDoc->m_bReadOnly)
    return;

  if (doc()->undoCount() == 0)
  {
    editUndo->setEnabled(false);
  }
  else
  {
    editUndo->setEnabled(true);
  }

  if (doc()->redoCount() == 0)
  {
    editRedo->setEnabled(false);
  }
  else
  {
    editRedo->setEnabled(true);
  }
}

void KateView::slotDropEventPass( QDropEvent * ev )
{
    KURL::List lstDragURLs;
    bool ok = KURLDrag::decode( ev, lstDragURLs );

    KParts::BrowserExtension * ext = KParts::BrowserExtension::childObject( doc() );
    if ( ok && ext )
        emit ext->openURLRequest( lstDragURLs.first() );
}

void KateView::keyPressEvent( QKeyEvent *ev )
{
  int key=ev->key();


  switch(key)
  {
        case Key_PageUp:
            if ( ev->state() & ShiftButton )
                shiftPageUp();
            else if ( ev->state() & ControlButton )
                topOfView();
            else
                pageUp();
            break;
        case Key_PageDown:
            if ( ev->state() & ShiftButton )
                shiftPageDown();
            else if ( ev->state() & ControlButton )
                bottomOfView();
            else
                pageDown();
            break;
        case Key_Return:
        case Key_Enter:
            doEditCommand(KateView::cmReturn);
            break;
        case Key_Delete:
            if ( ev->state() & ControlButton )
            {
               VConfig c;
               shiftWordRight();
               myViewInternal->getVConfig(c);
               myDoc->removeSelectedText();
               myViewInternal->update();
            }
            else keyDelete();
            break;
        case Key_Backspace:
            if ( ev->state() & ControlButton )
            {
               VConfig c;
               shiftWordLeft();
               myViewInternal->getVConfig(c);
               myDoc->removeSelectedText();
               myViewInternal->update();
            }
            else backspace();
            break;
        case Key_Insert:
            toggleInsert();
            break;
        case Key_K:
            if ( ev->state() & ControlButton )
            {
                killLine();
                break;
            }
        default:
            KTextEditor::View::keyPressEvent( ev );
            return;
            break; // never reached ;)
    }
    ev->accept();
}

void KateView::customEvent( QCustomEvent *ev )
{
    if ( KParts::GUIActivateEvent::test( ev ) && static_cast<KParts::GUIActivateEvent *>( ev )->activated() )
    {
        installPopup(static_cast<QPopupMenu *>(factory()->container("rb_popup", this) ) );
        return;
    }

    KTextEditor::View::customEvent( ev );
    return;
}

void KateView::contextMenuEvent( QContextMenuEvent *ev )
{
    if ( !extension || !myDoc )
        return;
    
    emit extension->popupMenu( ev->globalPos(), myDoc->url(),
                               QString::fromLatin1( "text/plain" ) );
    ev->accept();
}

bool KateView::setCursorPosition( uint line, uint col )
{
  setCursorPositionInternal( line, col, tabWidth() );
  return true;
}

bool KateView::setCursorPositionReal( uint line, uint col)
{
  setCursorPositionInternal( line, col, 1 );
  return true;
}

void KateView::cursorPosition( uint *line, uint *col )
{
  if ( line )
    *line = cursorLine();

  if ( col )
    *col = cursorColumn();
}

void KateView::cursorPositionReal( uint *line, uint *col )
{
  if ( line )
    *line = cursorLine();

  if ( col )
    *col = cursorColumnReal();
}

uint KateView::cursorLine() {
  return myViewInternal->cursor.line;
}

uint KateView::cursorColumn() {
  return myDoc->currentColumn(myViewInternal->cursor);
}

uint KateView::cursorColumnReal() {
  return myViewInternal->cursor.col;
}

void KateView::setCursorPositionInternal(int line, int col, int tabwidth)
{
  if ((uint)line > myDoc->lastLine())
    return;

  KateTextCursor cursor;

  TextLine::Ptr textLine = myDoc->getTextLine(line);
  QString line_str = QString(textLine->getText(), textLine->length());

  int z;
  int x = 0;
  for (z = 0; z < (int)line_str.length() && z <= col; z++) {
    if (line_str[z] == QChar('\t')) x += tabwidth - (x % tabwidth); else x++;
  }

  cursor.col = x;
  cursor.line = line;
  myViewInternal->updateCursor(cursor);
  myViewInternal->center();
  myDoc->updateViews();
}

int KateView::tabWidth() {
  return myDoc->tabChars;
}

void KateView::setTabWidth(int w) {
  myDoc->setTabWidth(w);
  myDoc->updateViews();
}

void KateView::setEncoding (QString e) {
  myDoc->setEncoding (e);
  myDoc->updateViews();
}

bool KateView::isLastView() {
  return myDoc->isLastView(1);
}

KateDocument *KateView::doc() {
  return myDoc;
}

bool KateView::isOverwriteMode() const
{
  return ( myDoc->_configFlags & KateDocument::cfOvr );
}

void KateView::setOverwriteMode( bool b )
{
  if ( isOverwriteMode() && !b )
    myDoc->setConfigFlags( myDoc->_configFlags ^ KateDocument::cfOvr );
  else
    myDoc->setConfigFlags( myDoc->_configFlags | KateDocument::cfOvr );
}

void KateView::setDynWordWrap (bool b)
{
  if (_hasWrap != b)
    myViewInternal->updateView(KateView::ufDocGeometry);

  _hasWrap = b;
}

bool KateView::dynWordWrap () const
{
  return _hasWrap;
}

void KateView::toggleInsert() {
  myDoc->setConfigFlags(myDoc->_configFlags ^ KateDocument::cfOvr);
  emit newStatus();
}

QString KateView::currentTextLine() {
  TextLine::Ptr textLine = myDoc->getTextLine(myViewInternal->cursor.line);
  return QString(textLine->getText(), textLine->length());
}

QString KateView::currentWord() {
  return myDoc->getWord(myViewInternal->cursor);
}

QString KateView::word(int x, int y) {
  KateTextCursor cursor;
  cursor.line = (myViewInternal->yPos + y)/myDoc->viewFont.fontHeight;
  if (cursor.line < 0 || cursor.line > (int)myDoc->lastLine()) return QString();
  cursor.col = myDoc->textPos(myDoc->getTextLine(cursor.line), myViewInternal->xPos + x);
  return myDoc->getWord(cursor);
}

void KateView::insertText(const QString &s)
{
  VConfig c;
  myViewInternal->getVConfig(c);
  myDoc->insertText(c.cursor.line, c.cursor.col, s);
  myDoc->updateViews();
//JW BEGIN
      for (int i=0;i<s.length();i++)
	{
		cursorRight();
	}
//JW END
     // updateCursor(c.cursor);

}

bool KateView::canDiscard() {
  int query;

  if (doc()->isModified()) {
    query = KMessageBox::warningYesNoCancel(this,
      i18n("The current Document has been modified.\nWould you like to save it?"));
    switch (query) {
      case KMessageBox::Yes: //yes
        if (save() == SAVE_CANCEL) return false;
        if (doc()->isModified()) {
            query = KMessageBox::warningContinueCancel(this,
               i18n("Could not save the document.\nDiscard it and continue?"),
           QString::null, i18n("&Discard"));
          if (query == KMessageBox::Cancel) return false;
        }
        break;
      case KMessageBox::Cancel: //cancel
        return false;
    }
  }
  return true;
}

void KateView::flush()
{
  if (canDiscard()) myDoc->flush();
}

KateView::saveResult KateView::save() {
  int query = KMessageBox::Yes;
  if (doc()->isModified()) {
    if (!myDoc->url().fileName().isEmpty() && doc()->isReadWrite()) {
      // If document is new but has a name, check if saving it would
      // overwrite a file that has been created since the new doc
      // was created:
      if( myDoc->isNewDoc() )
      {
        query = checkOverwrite( myDoc->url() );
        if( query == KMessageBox::Cancel )
          return SAVE_CANCEL;
      }
      if( query == KMessageBox::Yes )
      myDoc->saveAs(myDoc->url());
      else  // Do not overwrite already existing document:
        return saveAs();
    } // New, unnamed document:
    else
      return saveAs();
  }
  return SAVE_OK;
}

/*
 * Check if the given URL already exists. Currently used by both save() and saveAs()
 *
 * Asks the user for permission and returns the message box result and defaults to
 * KMessageBox::Yes in case of doubt
 */
int KateView::checkOverwrite( KURL u )
{
  int query = KMessageBox::Yes;

  if( u.isLocalFile() )
  {
    QFileInfo info;
    QString name( u.path() );
    info.setFile( name );
    if( info.exists() )
      query = KMessageBox::warningYesNoCancel( this,
        i18n( "A Document with this Name already exists.\nDo you want to overwrite it?" ) );
  }
  return query;
}

KateView::saveResult KateView::saveAs() {
  int query;
  KateFileDialog *dialog;
  KateFileDialogData data;

  do {
    query = KMessageBox::Yes;

    dialog = new KateFileDialog (myDoc->url().url(),doc()->encoding(), this, i18n ("Save File"), KateFileDialog::saveDialog);
    data = dialog->exec ();
    delete dialog;
    if (data.url.isEmpty())
      return SAVE_CANCEL;
    query = checkOverwrite( data.url );
  }
  while (query != KMessageBox::Yes);

  if( query == KMessageBox::Cancel )
    return SAVE_CANCEL;

  myDoc->setEncoding (data.encoding);
  if( !myDoc->saveAs(data.url) ) {
    KMessageBox::sorry(this,
      i18n("The file could not be saved. Please check if you have write permission."));
    return SAVE_ERROR;
  }

  return SAVE_OK;
}

void KateView::doCursorCommand(int cmdNum) {
  VConfig c;
  myViewInternal->getVConfig(c);
  if (cmdNum & selectFlag) c.flags |= KateDocument::cfMark;
  cmdNum &= ~selectFlag;
  myViewInternal->doCursorCommand(c, cmdNum);
  myDoc->updateViews();
}

void KateView::doEditCommand(int cmdNum) {
  VConfig c;
  myViewInternal->getVConfig(c);
  myViewInternal->doEditCommand(c, cmdNum);
}

static void kwview_addToStrList(QStringList &list, const QString &str) {
  if (list.count() > 0) {
    if (list.first() == str) return;
    QStringList::Iterator it;
    it = list.find(str);
    if (*it != 0L) list.remove(it);
    if (list.count() >= 16) list.remove(list.fromLast());
  }
  list.prepend(str);
}

void KateView::find() {
  SearchDialog *searchDialog;

  if (!myDoc->hasSelection()) myDoc->_searchFlags &= ~KateDocument::sfSelected;

  searchDialog = new SearchDialog(this, myDoc->searchForList, myDoc->replaceWithList,
  myDoc->_searchFlags & ~KateDocument::sfReplace);

  // If the user has marked some text we use that otherwise
  // use the word under the cursor.
   QString str;
   if (myDoc->hasSelection())
     str = myDoc->selection();

   if (str.isEmpty())
     str = currentWord();

   if (!str.isEmpty())
   {
     str.replace(QRegExp("^\n"), "");
     int pos=str.find("\n");
     if (pos>-1)
       str=str.left(pos);
     searchDialog->setSearchText( str );
   }

  myViewInternal->focusOutEvent(0L);// QT bug ?
  if (searchDialog->exec() == QDialog::Accepted) {
    kwview_addToStrList(myDoc->searchForList, searchDialog->getSearchFor());
    myDoc->_searchFlags = searchDialog->getFlags() | (myDoc->_searchFlags & KateDocument::sfPrompt);
    initSearch(myDoc->s, myDoc->_searchFlags);
    findAgain(myDoc->s);
  }
  delete searchDialog;
}

void KateView::replace() {
  SearchDialog *searchDialog;

  if (!doc()->isReadWrite()) return;

  if (!myDoc->hasSelection()) myDoc->_searchFlags &= ~KateDocument::sfSelected;
  searchDialog = new SearchDialog(this, myDoc->searchForList, myDoc->replaceWithList,
    myDoc->_searchFlags | KateDocument::sfReplace);

  // If the user has marked some text we use that otherwise
  // use the word under the cursor.
   QString str;
   if (myDoc->hasSelection())
     str = myDoc->selection();

   if (str.isEmpty())
     str = currentWord();

   if (!str.isEmpty())
   {
     str.replace(QRegExp("^\n"), "");
     int pos=str.find("\n");
     if (pos>-1)
       str=str.left(pos);
     searchDialog->setSearchText( str );
   }

  myViewInternal->focusOutEvent(0L);// QT bug ?
  if (searchDialog->exec() == QDialog::Accepted) {
//    myDoc->recordReset();
    kwview_addToStrList(myDoc->searchForList, searchDialog->getSearchFor());
    kwview_addToStrList(myDoc->replaceWithList, searchDialog->getReplaceWith());
    myDoc->_searchFlags = searchDialog->getFlags();
    initSearch(myDoc->s, myDoc->_searchFlags);
    replaceAgain();
  }
  delete searchDialog;
}

void KateView::gotoLine()
{
  GotoLineDialog *dlg;

  dlg = new GotoLineDialog(this, myViewInternal->cursor.line + 1, myDoc->numLines());

  if (dlg->exec() == QDialog::Accepted)
    gotoLineNumber( dlg->getLine() - 1 );

  delete dlg;
}

void KateView::gotoLineNumber( int linenumber )
{
  KateTextCursor cursor;

  cursor.col = 0;
  cursor.line = linenumber;
  myViewInternal->updateCursor(cursor);
  myViewInternal->center();
  myViewInternal->updateView(KateView::ufUpdateOnScroll);
  myDoc->updateViews();
 }

void KateView::initSearch(SConfig &, int flags) {

  myDoc->s.flags = flags;
  myDoc->s.setPattern(myDoc->searchForList.first());

  if (!(myDoc->s.flags & KateDocument::sfFromBeginning)) {
    // If we are continuing a backward search, make sure we do not get stuck
    // at an existing match.
    myDoc->s.cursor = myViewInternal->cursor;
    TextLine::Ptr textLine = myDoc->getTextLine(myDoc->s.cursor.line);
    QString const txt(textLine->getText(),textLine->length());
    const QString searchFor= myDoc->searchForList.first();
    int pos = myDoc->s.cursor.col-searchFor.length()-1;
    if ( pos < 0 ) pos = 0;
    pos= txt.find(searchFor, pos, myDoc->s.flags & KateDocument::sfCaseSensitive);
    if ( myDoc->s.flags & KateDocument::sfBackward )
    {
      if ( pos <= myDoc->s.cursor.col )  myDoc->s.cursor.col= pos-1;
    }
    else
      if ( pos == myDoc->s.cursor.col )  myDoc->s.cursor.col++;
  } else {
    if (!(myDoc->s.flags & KateDocument::sfBackward)) {
      myDoc->s.cursor.col = 0;
      myDoc->s.cursor.line = 0;
    } else {
      myDoc->s.cursor.col = -1;
      myDoc->s.cursor.line = myDoc->lastLine();
    }
    myDoc->s.flags |= KateDocument::sfFinished;
  }
  if (!(myDoc->s.flags & KateDocument::sfBackward)) {
    if (!(myDoc->s.cursor.col || myDoc->s.cursor.line))
      myDoc->s.flags |= KateDocument::sfFinished;
  }
}

void KateView::continueSearch(SConfig &) {

  if (!(myDoc->s.flags & KateDocument::sfBackward)) {
    myDoc->s.cursor.col = 0;
    myDoc->s.cursor.line = 0;
  } else {
    myDoc->s.cursor.col = -1;
    myDoc->s.cursor.line = myDoc->lastLine();
  }
  myDoc->s.flags |= KateDocument::sfFinished;
  myDoc->s.flags &= ~KateDocument::sfAgain;
}

void KateView::findAgain(SConfig &s) {
  int query;
  KateTextCursor cursor;
  QString str;

  QString searchFor = myDoc->searchForList.first();

  if( searchFor.isEmpty() ) {
    find();
    return;
  }

  do {
    query = KMessageBox::Cancel;
    if (myDoc->doSearch(myDoc->s,searchFor)) {
      cursor = myDoc->s.cursor;
      if (!(myDoc->s.flags & KateDocument::sfBackward))
        myDoc->s.cursor.col += myDoc->s.matchedLength;
      myViewInternal->updateCursor(myDoc->s.cursor); //does deselectAll()
      exposeFound(cursor,myDoc->s.matchedLength,(myDoc->s.flags & KateDocument::sfAgain) ? 0 : KateView::ufUpdateOnScroll,false);
    } else {
      if (!(myDoc->s.flags & KateDocument::sfFinished)) {
        // ask for continue
        if (!(myDoc->s.flags & KateDocument::sfBackward)) {
          // forward search
          str = i18n("End of document reached.\n"
                "Continue from the beginning?");
          query = KMessageBox::warningContinueCancel(this,
                           str, i18n("Find"), i18n("Continue"));
        } else {
          // backward search
          str = i18n("Beginning of document reached.\n"
                "Continue from the end?");
          query = KMessageBox::warningContinueCancel(this,
                           str, i18n("Find"), i18n("Continue"));
        }
        continueSearch(s);
      } else {
        // wrapped
        KMessageBox::sorry(this,
          i18n("Search string '%1' not found!").arg(KStringHandler::csqueeze(searchFor)),
          i18n("Find"));
      }
    }
  } while (query == KMessageBox::Continue);
}

void KateView::replaceAgain() {
  if (!doc()->isReadWrite())
    return;

  replaces = 0;
  if (myDoc->s.flags & KateDocument::sfPrompt) {
    doReplaceAction(-1);
  } else {
    doReplaceAction(KateView::srAll);
  }
}

void KateView::doReplaceAction(int result, bool found) {
  int rlen;
  KateTextCursor cursor;
  bool started;

  QString searchFor = myDoc->searchForList.first();
  QString replaceWith = myDoc->replaceWithList.first();
  rlen = replaceWith.length();

  switch (result) {
    case KateView::srYes: //yes
      myDoc->removeText (myDoc->s.cursor.line, myDoc->s.cursor.col, myDoc->s.cursor.line, myDoc->s.cursor.col + myDoc->s.matchedLength);
      myDoc->insertText (myDoc->s.cursor.line, myDoc->s.cursor.col, replaceWith);
      replaces++;

      if (!(myDoc->s.flags & KateDocument::sfBackward))
            myDoc->s.cursor.col += rlen;
          else
          {
            if (myDoc->s.cursor.col > 0)
              myDoc->s.cursor.col--;
            else
            {
              myDoc->s.cursor.line--;

              if (myDoc->s.cursor.line >= 0)
              {
                myDoc->s.cursor.col = myDoc->getTextLine(myDoc->s.cursor.line)->length();
              }
            }
          }

      break;
    case KateView::srNo: //no
      if (!(myDoc->s.flags & KateDocument::sfBackward))
            myDoc->s.cursor.col += myDoc->s.matchedLength;
          else
          {
            if (myDoc->s.cursor.col > 0)
              myDoc->s.cursor.col--;
            else
            {
              myDoc->s.cursor.line--;

              if (myDoc->s.cursor.line >= 0)
              {
                myDoc->s.cursor.col = myDoc->getTextLine(myDoc->s.cursor.line)->length();
              }
            }
          }
     
      break;
    case KateView::srAll: //replace all
      deleteReplacePrompt();
      do {
        started = false;
        while (found || myDoc->doSearch(myDoc->s,searchFor)) {
          if (!started) {
            found = false;
            started = true;
          }
          myDoc->removeText (myDoc->s.cursor.line, myDoc->s.cursor.col, myDoc->s.cursor.line, myDoc->s.cursor.col + myDoc->s.matchedLength);
          myDoc->insertText (myDoc->s.cursor.line, myDoc->s.cursor.col, replaceWith);
          replaces++;

          if (!(myDoc->s.flags & KateDocument::sfBackward))
            myDoc->s.cursor.col += rlen;
          else
          {
            if (myDoc->s.cursor.col > 0)
              myDoc->s.cursor.col--;
            else
            {
              myDoc->s.cursor.line--;

              if (myDoc->s.cursor.line >= 0)
              {
                myDoc->s.cursor.col = myDoc->getTextLine(myDoc->s.cursor.line)->length();
              }
            }
          }

        }
      } while (!askReplaceEnd());
      return;
    case KateView::srCancel: //cancel
      deleteReplacePrompt();
      return;
    default:
      replacePrompt = 0L;
  }

  do {
    if (myDoc->doSearch(myDoc->s,searchFor)) {
      //text found: highlight it, show replace prompt if needed and exit
      cursor = myDoc->s.cursor;
      if (!(myDoc->s.flags & KateDocument::sfBackward)) cursor.col += myDoc->s.matchedLength;
      myViewInternal->updateCursor(cursor); //does deselectAll()
      exposeFound(myDoc->s.cursor,myDoc->s.matchedLength,(myDoc->s.flags & KateDocument::sfAgain) ? 0 : KateView::ufUpdateOnScroll,true);
      if (replacePrompt == 0L) {
        replacePrompt = new ReplacePrompt(this);
        myDoc->setPseudoModal(replacePrompt);//disable();
        connect(replacePrompt,SIGNAL(clicked()),this,SLOT(replaceSlot()));
        replacePrompt->show(); //this is not modal
      }
      return; //exit if text found
    }
    //nothing found: repeat until user cancels "repeat from beginning" dialog
  } while (!askReplaceEnd());
  deleteReplacePrompt();
}

void KateView::exposeFound(KateTextCursor &cursor, int slen, int flags, bool replace) {
  int x1, x2, y1, y2, xPos, yPos;

  VConfig c;
  myViewInternal->getVConfig(c);
  myDoc->selectLength(cursor,slen,c.flags);

  TextLine::Ptr textLine = myDoc->getTextLine(cursor.line);
  x1 = myDoc->textWidth(textLine,cursor.col)        -10;
  x2 = myDoc->textWidth(textLine,cursor.col + slen) +20;
  y1 = myDoc->viewFont.fontHeight*cursor.line                 -10;
  y2 = y1 + myDoc->viewFont.fontHeight                     +30;

  xPos = myViewInternal->xPos;
  yPos = myViewInternal->yPos;

  if (x1 < 0) x1 = 0;
  if (replace) y2 += 90;

  if (x1 < xPos || x2 > xPos + myViewInternal->width()) {
    xPos = x2 - myViewInternal->width();
  }
  if (y1 < yPos || y2 > yPos + myViewInternal->height()) {
    xPos = x2 - myViewInternal->width();
    yPos = myDoc->viewFont.fontHeight*cursor.line - height()/3;
  }
  myViewInternal->setPos(xPos, yPos);
  myViewInternal->updateView(flags);// | ufPos,xPos,yPos);
  myDoc->updateViews();
}

void KateView::deleteReplacePrompt() {
  myDoc->setPseudoModal(0L);
}

bool KateView::askReplaceEnd() {
  QString str;
  int query;

  myDoc->updateViews();
  if (myDoc->s.flags & KateDocument::sfFinished) {
    // replace finished
    str = i18n("%1 replacement(s) made").arg(replaces);
    KMessageBox::information(this, str, i18n("Replace"));
    return true;
  }

  // ask for continue
  if (!(myDoc->s.flags & KateDocument::sfBackward)) {
    // forward search
    str = i18n("%1 replacement(s) made.\n"
               "End of document reached.\n"
               "Continue from the beginning?").arg(replaces);
    query = KMessageBox::questionYesNo(this, str, i18n("Replace"),
        i18n("Continue"), i18n("Stop"));
  } else {
    // backward search
    str = i18n("%1 replacement(s) made.\n"
                "Beginning of document reached.\n"
                "Continue from the end?").arg(replaces);
    query = KMessageBox::questionYesNo(this, str, i18n("Replace"),
                i18n("Continue"), i18n("Stop"));
  }
  replaces = 0;
  continueSearch(myDoc->s);
  return (query == KMessageBox::No);
}

void KateView::replaceSlot() {
  doReplaceAction(replacePrompt->result(),true);
}

void KateView::installPopup(QPopupMenu *rmb_Menu)
{
  rmbMenu = rmb_Menu;
}

void KateView::readSessionConfig(KConfig *config)
{
  KateTextCursor cursor;

  myViewInternal->xPos = config->readNumEntry("XPos");
  myViewInternal->yPos = config->readNumEntry("YPos");
  cursor.col = config->readNumEntry("CursorX");
  cursor.line = config->readNumEntry("CursorY");
  myViewInternal->updateCursor(cursor);
  iconBorderStatus = config->readNumEntry("IconBorderStatus");
  setIconBorder( iconBorderStatus & KateIconBorder::Icons );
  setLineNumbersOn( iconBorderStatus & KateIconBorder::LineNumbers );
}

void KateView::writeSessionConfig(KConfig *config)
{
  config->writeEntry("XPos",myViewInternal->xPos);
  config->writeEntry("YPos",myViewInternal->yPos);
  config->writeEntry("CursorX",myViewInternal->cursor.col);
  config->writeEntry("CursorY",myViewInternal->cursor.line);
  config->writeEntry("IconBorderStatus", iconBorderStatus );
}

int KateView::getEol() {
  return myDoc->eolMode;
}

void KateView::setEol(int eol) {
  if (!doc()->isReadWrite())
    return;

  myDoc->eolMode = eol;
  myDoc->setModified(true);
}

void KateView::resizeEvent(QResizeEvent *)
{
  myViewInternal->tagAll();
  myViewInternal->updateView(0/*ufNoScroll*/);
}

void KateView::dropEventPassEmited (QDropEvent* e)
{
  emit dropEventPass(e);
}

// Applies a new pattern to the search context.
void SConfig::setPattern(QString &newPattern) {
  bool regExp = (flags & KateDocument::sfRegularExpression);

  m_pattern = newPattern;
  if (regExp) {
    m_regExp.setCaseSensitive(flags & KateDocument::sfCaseSensitive);
    m_regExp.setPattern(m_pattern);
  }
}

void KateView::setActive (bool b)
{
  active = b;
}

bool KateView::isActive ()
{
  return active;
}

void KateView::setFocus ()
{
  QWidget::setFocus ();

  emit gotFocus ((Kate::View *) this);
}

bool KateView::eventFilter (QObject *object, QEvent *event)
{
  if ( object == myViewInternal )
    KCursor::autoHideEventFilter( object, event );

  if ( (event->type() == QEvent::FocusIn) )
  {
    m_editAccels->setEnabled(true);
    emit gotFocus (this);
  }

  if ( (event->type() == QEvent::FocusOut) )
  {
  	m_editAccels->setEnabled(false);
  }

  if ( (event->type() == QEvent::KeyPress) )
    {
        QKeyEvent * ke=(QKeyEvent *)event;

        if ((ke->key()==Qt::Key_Tab) || (ke->key()==Qt::Key_BackTab))
          {
            myViewInternal->keyPressEvent(ke);
            return true;
          }
    }
  if (object == myViewInternal->leftBorder && event->type() == QEvent::Resize)
    updateIconBorder();
  return QWidget::eventFilter (object, event);
}

void KateView::findAgain (bool back)
{
  bool b= (myDoc->_searchFlags & KateDocument::sfBackward) > 0;
  initSearch(myDoc->s, (myDoc->_searchFlags & ((b==back)?~KateDocument::sfBackward:~0) & ~KateDocument::sfFromBeginning)
                | KateDocument::sfPrompt | KateDocument::sfAgain | ((b!=back)?KateDocument::sfBackward : 0) );
  if (myDoc->s.flags & KateDocument::sfReplace)
    replaceAgain();
  else
    KateView::findAgain(myDoc->s);
}

void KateView::slotEditCommand ()
{
  bool ok;
  QString cmd = KLineEditDlg::getText(i18n("Editing Command"), "", &ok, this);

  if (ok)
    myDoc->cmd()->execCmd (cmd, this);
}

void KateView::setIconBorder (bool enable)
{
  if ( enable == iconBorderStatus & KateIconBorder::Icons )
    return; // no change
  if ( enable )
    iconBorderStatus |= KateIconBorder::Icons;
  else
    iconBorderStatus &= ~KateIconBorder::Icons;

  updateIconBorder();
}

void KateView::toggleIconBorder ()
{
  setIconBorder ( ! (iconBorderStatus & KateIconBorder::Icons) );
}

void KateView::setLineNumbersOn(bool enable)
{
  if (enable == iconBorderStatus & KateIconBorder::LineNumbers)
    return; // no change

  if (enable)
    iconBorderStatus |= KateIconBorder::LineNumbers;
  else
    iconBorderStatus &= ~KateIconBorder::LineNumbers;

  updateIconBorder();
}

void KateView::toggleLineNumbersOn()
{
  setLineNumbersOn( ! (iconBorderStatus & KateIconBorder::LineNumbers) );
}

// FIXME anders: move into KateIconBorder class
void KateView::updateIconBorder()
{
  if ( iconBorderStatus != KateIconBorder::None )
  {
    myViewInternal->leftBorder->show();
  }
  else
  {
    myViewInternal->leftBorder->hide();
  }
  myViewInternal->leftBorder->resize(myViewInternal->leftBorder->width(),myViewInternal->leftBorder->height());
  myViewInternal->resize(width()-myViewInternal->leftBorder->width(), myViewInternal->height());
  myViewInternal->move(myViewInternal->leftBorder->width(), 0);
  myViewInternal->updateView(ufLeftBorder);
}

void KateView::gotoMark (KTextEditor::Mark *mark)
{
  KateTextCursor cursor;

  cursor.col = 0;
  cursor.line = mark->line;
  myViewInternal->updateCursor(cursor);
  myViewInternal->center();
  myViewInternal->updateView(KateView::ufUpdateOnScroll);
  myDoc->updateViews();
}

void KateView::toggleBookmark ()
{
  uint mark = myDoc->mark (cursorLine());

  if (mark&KateDocument::markType01)
    myDoc->removeMark (cursorLine(), KateDocument::markType01);
  else
    myDoc->addMark (cursorLine(), KateDocument::markType01);
}

void KateView::bookmarkMenuAboutToShow()
{
  bookmarkMenu->popupMenu()->clear ();
  bookmarkToggle->plug (bookmarkMenu->popupMenu());
  bookmarkClear->plug (bookmarkMenu->popupMenu());
  bookmarkMenu->popupMenu()->insertSeparator ();

  list = myDoc->marks();
  for (int i=0; (uint) i < list.count(); i++)
  {
    if (list.at(i)->type&KateDocument::markType01)
    {
      QString bText = myDoc->textLine(list.at(i)->line);
      bText.truncate(32);
      bText.append ("...");
      bookmarkMenu->popupMenu()->insertItem ( QString("%1 - \"%2\"").arg(list.at(i)->line).arg(bText), this, SLOT (gotoBookmark(int)), 0, i );
    }
  }
}

void KateView::gotoBookmark (int n)
{
  gotoMark (list.at(n));
}

void KateView::slotIncFontSizes ()
{
  QFont font = myDoc->getFont(KateDocument::ViewFont);
  font.setPointSize (font.pointSize()+1);
  myDoc->setFont (KateDocument::ViewFont,font);
}

void KateView::slotDecFontSizes ()
{
  QFont font = myDoc->getFont(KateDocument::ViewFont);
  font.setPointSize (font.pointSize()-1);
  myDoc->setFont (KateDocument::ViewFont,font);
}

KateDocument *KateView::document()
{
  return myDoc;
}

KateBrowserExtension::KateBrowserExtension( KateDocument *doc, KateView *view )
: KParts::BrowserExtension( doc, "katepartbrowserextension" )
{
  m_doc = doc;
  m_view = view;
  connect( m_doc, SIGNAL( selectionChanged() ), this, SLOT( slotSelectionChanged() ) );
  emit enableAction( "print", true );
}

void KateBrowserExtension::copy()
{
  m_doc->copy( 0 );
}

void KateBrowserExtension::print()
{
  m_doc->printDialog ();
}

void KateBrowserExtension::slotSelectionChanged()
{
  emit enableAction( "copy", m_doc->hasSelection() );
}
