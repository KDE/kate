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

  maxLen = 0;
  startLine = 0;
  startLineReal = 0;
  endLine = 0;

  xPos = 0;

  xCoord = 0;
  yCoord = 0;

  scrollTimer = 0;

  cursor.col = 0;
  cursor.line = 0;
  cursorOn = false;
  cursorTimer = 0;
  cXPos = 0;
  cOldXPos = 0;

  possibleTripleClick = false;
  exposeCursor = false;
  updateState = 0;
  newXPos = -1;
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
      VConfig c;
      getVConfig(c);
      if (c.flags & KateDocument::cfDelOnInput) myDoc->removeSelectedText();
      myDoc->newLine(c);
      updateCursor(c.cursor);
      updateView (0);
}

void KateViewInternal::doDelete()
{
      VConfig c;
       getVConfig(c);
     if ((c.flags & KateDocument::cfDelOnInput) && myDoc->hasSelection())
        myDoc->removeSelectedText();
      else myDoc->del(c);
}

void KateViewInternal::doBackspace()
{
      VConfig c;
      getVConfig(c);
      if ((c.flags & KateDocument::cfDelOnInput) && myDoc->hasSelection())
        myDoc->removeSelectedText();
      else
        myDoc->backspace(c.cursor.line, c.cursor.col);
    //  if ( (uint)c.cursor.line >= myDoc->lastLine() )
     //   leftBorder->repaint();
}

void KateViewInternal::doPaste()
{
       VConfig c;
     getVConfig(c);
      if (c.flags & KateDocument::cfDelOnInput) myDoc->removeSelectedText();
      myDoc->paste(c);
}

void KateViewInternal::doTranspose()
{
       VConfig c;
     getVConfig(c);
      myDoc->transpose(c.cursor);
      cursorRight();
      cursorRight();
}

void KateViewInternal::cursorLeft (bool sel)
{
  KateTextCursor tmpCur = cursor;

  tmpCur.col--;
  
  if (myDoc->_configFlags & KateDocument::cfWrapCursor && tmpCur.col < 0 && tmpCur.line > 0)
  {
    tmpCur.line=myDoc->getRealLine(displayCursor.line-1);
    tmpCur.col = myDoc->textLength(tmpCur.line);
  }

  updateCursor (tmpCur, sel);
}

void KateViewInternal::cursorRight(bool sel)
{
  KateTextCursor tmpCur = cursor;
  
  if (myDoc->_configFlags & KateDocument::cfWrapCursor)
  {
    if (tmpCur.col >= (int) myDoc->textLength(tmpCur.line)) {
      if (tmpCur.line == (int)myDoc->lastLine()) return;
      tmpCur.line=myDoc->getRealLine(displayCursor.line+1);
      tmpCur.col = -1;
    }
  }
  
  tmpCur.col++;
  
  updateCursor (tmpCur, sel);
}

void KateViewInternal::wordLeft(bool sel)
{
  KateTextCursor tmpCur = cursor;
  Highlight *highlight = myDoc->highlight();
  TextLine::Ptr textLine = myDoc->kateTextLine(cursor.line);
  
  if (tmpCur.col > 0) {
    do {
      tmpCur.col--;
    } while (tmpCur.col > 0 && !highlight->isInWord(textLine->getChar(tmpCur.col)));
    while (tmpCur.col > 0 && highlight->isInWord(textLine->getChar(tmpCur.col -1)))
      tmpCur.col--;
  } else {
    if (tmpCur.line > 0) {
      tmpCur.line--;
      textLine = myDoc->kateTextLine(tmpCur.line);
      tmpCur.col = textLine->length();
    }
  }
  
  updateCursor (tmpCur, sel);
}

void KateViewInternal::wordRight(bool sel)
{
  KateTextCursor tmpCur = cursor;
  Highlight *highlight = myDoc->highlight();
  int len;

  TextLine::Ptr textLine = myDoc->kateTextLine(cursor.line);
  len = textLine->length();

  if (tmpCur.col < len) {
    do {
      tmpCur.col++;
    } while (tmpCur.col < len && highlight->isInWord(textLine->getChar(tmpCur.col)));
    while (tmpCur.col < len && !highlight->isInWord(textLine->getChar(tmpCur.col)))
      tmpCur.col++;
  } else {
    if (tmpCur.line < (int) myDoc->lastLine()) {
      tmpCur.line++;
      textLine = myDoc->kateTextLine(tmpCur.line);
      tmpCur.col = 0;
    }
  }

  updateCursor (tmpCur, sel);
}

void KateViewInternal::home(bool sel)
{
  KateTextCursor tmpCur = cursor;
  int lc = (myDoc->_configFlags & KateDocument::cfSmartHome) ? myDoc->kateTextLine(tmpCur.line)->firstChar() : 0;
  
  if (lc <= 0 || tmpCur.col == lc) {
    tmpCur.col = 0;
  } else {
    tmpCur.col = lc;
  }

  updateCursor (tmpCur, sel);
}

void KateViewInternal::end(bool sel)
{
  KateTextCursor tmpCur = cursor;
  
  tmpCur.col = myDoc->textLength(cursor.line);
  
  updateCursor (tmpCur, sel);
}


void KateViewInternal::cursorUp(bool sel)
{
  KateTextCursor tmpCur = cursor;
  
  if (displayCursor.line>0)
  {
    tmpCur.line=myDoc->getRealLine(displayCursor.line-1);
    myDoc->textWidth(myDoc->_configFlags & KateDocument::cfWrapCursor,tmpCur,cOldXPos);
    updateCursor(tmpCur, sel);
  }
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
  
  updateCursor (tmpCur, sel);
}

void KateViewInternal::scrollUp(bool sel)
{
#if 0
  if (! yPos) return;

  newYPos = yPos - myDoc->viewFont.fontHeight;
  if (cursor.line == (yPos + height())/myDoc->viewFont.fontHeight -1) {
    cursor.line--;
    cXPos = myDoc->textWidth(c.flags & KateDocument::cfWrapCursor,cursor,cOldXPos);

    changeState(c);
  }
#endif
}

void KateViewInternal::scrollDown(bool sel) {

  if (endLine >= (int)myDoc->lastLine()) return;

  newStartLine = startLine + myDoc->viewFont.fontHeight;
  if (cursor.line == startLine) {
    KateTextCursor tmpCur = cursor;
    tmpCur.line++;
    myDoc->textWidth(myDoc->_configFlags & KateDocument::cfWrapCursor,tmpCur,cOldXPos);
    updateCursor (tmpCur, sel);
  }
}

void KateViewInternal::topOfView(bool sel)
{
  KateTextCursor tmpCur = cursor;
  tmpCur.line = startLineReal;
  tmpCur.col = 0;
  updateCursor (tmpCur, sel);
}

void KateViewInternal::bottomOfView(bool sel)
{
  KateTextCursor tmpCur = cursor;
  
  tmpCur.line = startLineReal + (height()/myDoc->viewFont.fontHeight) - 1;
  if (tmpCur.line < 0) tmpCur.line = 0;
  if (tmpCur.line > (int)myDoc->lastLine()) tmpCur.line = myDoc->lastLine();
  tmpCur.col = 0;

  updateCursor (tmpCur, sel);
}

void KateViewInternal::pageUp(bool sel)
{
  KateTextCursor tmpCur = cursor;
  int lines = (endLine - startLine - 1);

  if (lines <= 0) lines = 1;

  if (startLine > 0) {
    newStartLine = startLine - lines;
    if (newStartLine < 0) newStartLine = 0;
  }

  int displayCursorline = displayCursor.line;
  
  displayCursorline -= lines;
  if (displayCursorline<0) displayCursorline=0;
  tmpCur.line=myDoc->getRealLine(displayCursorline);
  myDoc->textWidth(myDoc->_configFlags & KateDocument::cfWrapCursor, tmpCur, cOldXPos);
  
  updateCursor (tmpCur, sel);
}

void KateViewInternal::pageDown(bool sel)
{
  KateTextCursor tmpCur = cursor;
  int lines = (endLine - startLine - 1);

  if (endLine < (int)myDoc->lastLine()) {
    if (lines < (int)myDoc->lastLine() - endLine)
      newStartLine = startLine + lines;
    else
      newStartLine = startLine + (myDoc->lastLine() - endLine);
  }
  
  int displayCursorline = displayCursor.line;
  displayCursorline += lines;
  
  if (displayCursorline>myDoc->numVisLines()) displayCursorline=myDoc->numVisLines();
  tmpCur.line=myDoc->getRealLine(displayCursorline);
  myDoc->textWidth(myDoc->_configFlags & KateDocument::cfWrapCursor,tmpCur,cOldXPos);

  updateCursor (tmpCur, sel);
}

// go to the top, same X position
void KateViewInternal::top(bool sel)
{
  KateTextCursor tmpCur = cursor;
  tmpCur.line = 0;
  
  myDoc->textWidth(myDoc->_configFlags & KateDocument::cfWrapCursor,tmpCur,cOldXPos);
  updateCursor (tmpCur, sel);
}

// go to the bottom, same X position
void KateViewInternal::bottom(bool sel)
{
  KateTextCursor tmpCur = cursor;
  tmpCur.line = myDoc->lastLine();
  
  myDoc->textWidth(myDoc->_configFlags & KateDocument::cfWrapCursor,tmpCur,cOldXPos);
  updateCursor (tmpCur, sel);
}

// go to the top left corner
void KateViewInternal::top_home(bool sel)
{
  KateTextCursor tmpCur = cursor;
  
  tmpCur.line = 0;
  tmpCur.col = 0;

  updateCursor (tmpCur, sel);
}

// go to the bottom right corner
void KateViewInternal::bottom_end(bool sel)
{
  KateTextCursor tmpCur = cursor;
  
  tmpCur.line = myDoc->lastLine();
  tmpCur.col = myDoc->textLength(tmpCur.line);
  
  updateCursor (tmpCur, sel);
}


void KateViewInternal::changeXPos(int p) {
  int dx;

  dx = xPos - p;
  xPos = p;
  if (QABS(dx) < width()) scroll(dx, 0); else update();
}

void KateViewInternal::changeYPos(int p)
{
  int dy;

  newStartLine = p  / myDoc->viewFont.fontHeight;

  dy = (startLine - newStartLine)  * myDoc->viewFont.fontHeight;

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


void KateViewInternal::getVConfig(VConfig &c) {

  c.view = myView;
  c.cursor = cursor;
  c.displayCursor=displayCursor;
  c.cXPos = cXPos;
  c.flags = myView->myDoc->_configFlags;
}

void KateViewInternal::updateCursor()
{
  cOldXPos = cXPos = myDoc->textWidth(cursor);
}


void KateViewInternal::updateCursor(KateTextCursor &newCursor,bool keepSel, int updateViewFlags)
{
  VConfig oldC;
  oldC.cursor = cursor;

  bool nullMove = (cursor.col == newCursor.col && cursor.line == newCursor.line);

  if (cursorOn)
  {
    tagLines(displayCursor.line, displayCursor.line);
    cursorOn = false;
  }

  if (bm.sXPos < bm.eXPos) {
    tagLines(bm.cursor.line, bm.cursor.line);
  }

  myDoc->newBracketMark(newCursor, bm);

  cursor = newCursor;
  displayCursor.line = myDoc->getVirtualLine(cursor.line);
  displayCursor.col = cursor.col;
  cOldXPos = cXPos = myDoc->textWidth(cursor);

  xCoord = cXPos-xPos;
  yCoord = (displayCursor.line-startLine+1)*myDoc->viewFont.fontHeight;

 if (keepSel) {
    if (! nullMove)
      myDoc->selectTo(oldC, cursor, cXPos);
  } else {
    if (!(myDoc->_configFlags & KateDocument::cfPersistent))
      myDoc->clearSelection();
  }

  updateView (updateViewFlags | KateViewInternal::ufExposeCursor);
  
  if (!nullMove)
    emit myView->cursorPositionChanged();
}

// init the line dirty cache
void KateViewInternal::updateLineRanges()
{
  int lines = 0;
  int oldStartLine = startLine;
  int oldLines = lineRanges.size();
  uint height = this->height();
  bool slchanged = false;
  int lastLine = myDoc->visibleLines ()-1;

  if (newStartLineReal != startLineReal)
  {
    startLine = myDoc->getVirtualLine(newStartLineReal);
    startLineReal = newStartLineReal;
    slchanged = true;
  }
  else if (newStartLine != startLine)
  {
    startLine = newStartLine;
    startLineReal = myDoc->getRealLine(startLine);
    slchanged = true;
  }

  uint tmpEnd = startLine + height/myDoc->viewFont.fontHeight - 1;
  if (tmpEnd > lastLine)
    endLine = lastLine;
  else
    endLine = tmpEnd;

  endLineReal = myDoc->getRealLine(endLine);

  if (endLine < 0) endLine = 0;

  lines = height/myDoc->viewFont.fontHeight;

  if ((lines * myDoc->viewFont.fontHeight) < height)
    lines++;

  if (lines > oldLines)
    lineRanges.resize (lines);

  for (uint z = 0; z < lines; z++)
  {
    uint newLine = myDoc->getRealLine (startLine+z);

	  if (z < oldLines)
		{
		if (newLine != lineRanges[z].line)
		{
		  lineRanges[z].line = newLine;
      lineRanges[z].dirty = true;
		}

    lineRanges[z].startCol = 0;
    lineRanges[z].endCol = -1;
    lineRanges[z].wrapped = false;
		}
		else
		{
	  	lineRanges[z].line = newLine;
      lineRanges[z].startCol = 0;
      lineRanges[z].endCol = -1;
      lineRanges[z].wrapped = false;
			lineRanges[z].dirty = true;
		}

    if ((startLine+z) > lastLine)
      lineRanges[z].empty = true;
    else
      lineRanges[z].empty = false;
  }

  if (lines < oldLines)
    lineRanges.resize (lines);

  newXPos = -1;
  newStartLine = startLine;
  newStartLineReal = startLineReal;
}

void KateViewInternal::tagRealLines(int start, int end)
{
  for (uint z = 0; z < lineRanges.size(); z++)
  {
    if (lineRanges[z].line > end)
      break;

    if (lineRanges[z].line >= start)
    {
      lineRanges[z].dirty = true;
      updateState |= 1;
    }
  }
}

void KateViewInternal::tagLines(int start, int end) {
  KateLineRange *r;
  int z;

  start -= startLine;
  if (start < 0) start = 0;
  end -= startLine;
  if (end > endLine - startLine) end = endLine - startLine;

  if (start < lineRanges.size())
  {
    r = &lineRanges[start];
    uint rpos = start;

    for (z = start; z <= end; z++)
    {
      if (rpos >= lineRanges.size()) break;

      r->dirty = true;

      r++;
      rpos++;

      updateState |= 1;
    }
  }
}

void KateViewInternal::tagAll() {
  updateState = 3;
}

void KateViewInternal::setPos(int x, int y) {
  newXPos = x;
//  newYPos = y;
}

void KateViewInternal::center() {
  newXPos = 0;
  int tmpnewStartLine = displayCursor.line - lineRanges.size()/2;
  if (tmpnewStartLine < 0) newStartLine = 0;
  else newStartLine = tmpnewStartLine;
}


void KateViewInternal::singleShotUpdateView()
{
	updateView(0);
}

void KateViewInternal::updateView(int flags)
{
	unsigned int oldXPos=xPos;
	unsigned int oldYPos=startLine * myDoc->viewFont.fontHeight;
  int oldU = updateState;
  
	int fontHeight = myDoc->viewFont.fontHeight;
	bool needLineRangesUpdate=false;
	bool reUpdate;
  int scrollbarWidth = style().scrollBarExtent().width();

  int w = myView->width();
  int h = myView->height();
  
  int bw = leftBorder->width();
  w -= bw;
  
  //
  //  update yScrollbar (first that, as we need if it should be shown for the width() of the view)
  //
  if (!(flags & KateViewInternal::ufExposeCursor) || (flags & KateViewInternal::ufFoldingChanged) || (flags & KateViewInternal::ufDocGeometry))
  {
    uint contentLines=myDoc->visibleLines();
    int viewLines=height()/fontHeight;
    int yMax=(contentLines-viewLines)*fontHeight;
    
    if (yMax>0)
    {
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
    }
    else
      yScroll->hide();
      
    if (yScroll->isVisible())
      w -= scrollbarWidth;
      
    if (w < 0)
      w = 0;

    if (w != width() || h != height())
    {
      needLineRangesUpdate=true;
      resize(w,h);
    }
  }

   if (updateState==3)
     needLineRangesUpdate=true;

  int tmpYPos;

 
	if (flags & KateViewInternal::ufExposeCursor)
	{
    int he  = height();
	
		if (displayCursor.line>= (startLine+(he/fontHeight)))
		{
			tmpYPos=(displayCursor.line*fontHeight)-he+fontHeight;
			if ((tmpYPos % fontHeight)!=0) tmpYPos=tmpYPos+fontHeight;
			
      
       newStartLine = tmpYPos  / myDoc->viewFont.fontHeight;
      
      yScroll->blockSignals(true);
      yScroll->setValue(tmpYPos);
      yScroll->blockSignals(false);
      
             int dy;
  dy = (startLine - newStartLine)  * myDoc->viewFont.fontHeight;

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
		else
		if (displayCursor.line<startLine)
		{
			tmpYPos=(displayCursor.line*fontHeight);
   
      newStartLine = tmpYPos  / myDoc->viewFont.fontHeight;
      
      yScroll->blockSignals(true);
      yScroll->setValue(tmpYPos);
      yScroll->blockSignals(false);
      
       int dy;
  dy = (startLine - newStartLine)  * myDoc->viewFont.fontHeight;

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
	}

  //
  // update the lineRanges (IMPORTANT)
  //
  if (flags & KateViewInternal::ufFoldingChanged)
  {
	  needLineRangesUpdate=true;
	  updateLineRanges();
  }
  else
  {
	  if (needLineRangesUpdate && !(flags && KateViewInternal::ufDocGeometry)) updateLineRanges();
  }

  //
  // update xScrollbar
  //
  maxLen = 0;
  uint len = 0;
  for (uint z = 0; z < lineRanges.size(); z++)
  {
      if (lineRanges[z].dirty)
      {
        len = myDoc->textWidth (myDoc->kateTextLine (lineRanges[z].line), -1);
        lineRanges[z].lengthPixel = len;
      }
      else
        len = lineRanges[z].lengthPixel;

      if (len > maxLen)
        maxLen = len;
  }

  maxLen += 8;

  bool xScrollVis=xScroll->isVisible();

  if (maxLen > w)
  {
    if (!xScrollVis)
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
  }
  else
  {
    if (xScrollVis)
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
  else if (oldU > 0)
   {
     paintTextLines(xPos, oldYPos);
   //  kdDebug()<<"repaint lines"<<endl;
   }

  //
  // updateView done, reset the update flag + repaint flags
  //
  updateState = 0;
  exposeCursor=false;

  // blank repaint attribs
  for (uint z = 0; z < lineRanges.size(); z++)
  {
    lineRanges[z].dirty = false;
  }

#if 0
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

//kdDebug()<<"widest line to draw is "<<maxLen<<" px"<<endl;
  if (exposeCursor || flags & KateViewInternal::ufDocGeometry) {
    emit myView->cursorPositionChanged();
  } else {
    // anders: I stay for KateViewInternal::ufLeftBorder, to get xcroll updated when border elements
    // display change.
    if ( updateState == 0 && newXPos < 0 && newYPos < 0 && !( flags&KateViewInternal::ufLeftBorder ) ) return;
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
//    if (flags & KateViewInternal::ufNoScroll) break;
/*
    if (flags & KateViewInternal::ufCenter) {
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
    updateLineRanges(h);
    resize(w,h);
  } else {
    dx = oldXPos - xPos;
    dy = oldYPos - yPos;

    b = updateState == 3;
    if (flags & KateViewInternal::ufUpdateOnScroll) {
      b |= dx || dy;
    } else {
      b |= QABS(dx)*3 > w*2 || QABS(dy)*3 > h*2;
    }

    if (b) {
      updateLineRanges(h);
      update();
    } else {
      if (dy)
        leftBorder->scroll(0, dy);
      if (updateState > 0) paintTextLines(oldXPos, oldYPos);
      updateLineRanges(h);

      if (dx || dy) {
        scroll(dx,dy);
      }
      if (cursorOn) paintCursor();
      if (bm.eXPos > bm.sXPos) paintBracketMark();
    }
  }

  exposeCursor = false;
#endif
}


void KateViewInternal::paintTextLines(int xPos, int yPos)
{
  if (drawBuffer->isNull()) return;

  QPainter paint;
  paint.begin(drawBuffer);

  uint h = myDoc->viewFont.fontHeight;
  KateLineRange *r = lineRanges.data();

  uint rpos = 0;

	bool b = myView->isOverwriteMode();

  bool again = true;

  for ( uint line = startLine; rpos < lineRanges.size(); line++)
  {
    if (r->dirty && !r->empty)
    {
      myDoc->paintTextLine ( paint, r->line, r->startCol, r->endCol, 0, xPos, xPos + this->width(),
                                          (cursorOn && (r->line == cursor.line)) ? cursor.col : -1, b, true,
                                          myView->myDoc->_configFlags & KateDocument::cfShowTabs, KateDocument::ViewFont, again && (r->line == cursor.line));

      if (cXPos > r->lengthPixel)
      
      if (cursorOn && (r->line == cursor.line))
     {     
        if (b)
	{
	  int cursorMaxWidth = myDoc->viewFont.myFontMetrics.width(QChar (' '));
          paint.fillRect(cXPos-xPos, 0, cursorMaxWidth, myDoc->viewFont.fontHeight, myDoc->myAttribs[0].col);
	}
       else
         paint.fillRect(cXPos-xPos, 0, 2, myDoc->viewFont.fontHeight, myDoc->myAttribs[0].col);
      }
					  
      bitBlt(this, 0, (line-startLine)*h, drawBuffer, 0, 0, this->width(), h);

      leftBorder->paintLine(line, r);
    }
    else if (r->empty)
    {
      paint.fillRect(0, 0, this->width(), h, myDoc->colors[0]);
      bitBlt(this, 0, (line-startLine)*h, drawBuffer, 0, 0, this->width(), h);

      leftBorder->paintLine(line, r);
    }

    if (r->line == cursor.line)
      again = false;

    r++;
    rpos++;
  }

  paint.end();
}

void KateViewInternal::paintCursor() {
  int h, w,w2,y, x;
  static int cx = 0, cy = 0, ch = 0;

  h = myDoc->viewFont.fontHeight;
  y = h*(displayCursor.line - startLine);
  x = cXPos - xPos;

  if(myDoc->viewFont.myFont != font()) setFont(myDoc->viewFont.myFont);
  if(cx != x || cy != y || ch != h){
    cx = x;
    cy = y;
    ch = h;
    setMicroFocusHint(cx, cy, 0, ch);
  }


  tagRealLines( cursor.line, cursor.line);
  paintTextLines (xPos, 0);
}

void KateViewInternal::paintBracketMark() {
  int y;

  y = myDoc->viewFont.fontHeight*(bm.cursor.line +1 - startLine) -1;

  QPainter paint;
  paint.begin(this);
  paint.setPen(myDoc->cursorCol(bm.cursor.col, bm.cursor.line));

  paint.drawLine(bm.sXPos - xPos, y, bm.eXPos - xPos -1, y);
  paint.end();
}

void KateViewInternal::placeCursor(int x, int y, int flags)
{
  KateTextCursor tmpCur;
  int newDisplayLine=startLine + y/myDoc->viewFont.fontHeight;

  if (newDisplayLine>=myDoc->numVisLines()) return;
  if (((int)(newDisplayLine-startLine) < 0) || ((newDisplayLine-startLine) >= lineRanges.size())) return;// not sure yet, if this is ther correct way;

  tmpCur.line = lineRanges[newDisplayLine-startLine].line;
  myDoc->textWidth(myDoc->_configFlags & KateDocument::cfWrapCursor, tmpCur, xPos + x);

  updateCursor(tmpCur, flags & KateDocument::cfMark);
}

// given physical coordinates, report whether the text there is selected
bool KateViewInternal::isTargetSelected(int x, int y) {
  //y = (yPos + y) / myDoc->viewFont.fontHeight;

#warning "This needs changing for dynWW"
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

void KateViewInternal::keyPressEvent(QKeyEvent *e) {
  VConfig c;
  getVConfig(c);

  KKey key(e);
  if (myView->doc()->isReadWrite()) {
    if (c.flags & KateDocument::cfTabIndents && myDoc->hasSelection()) {
      if (key == Qt::Key_Tab) {
        myDoc->indent(c.cursor.line);
        return;
      }
      if (key == SHIFT+Qt::Key_Backtab || key == Qt::Key_Backtab) {
        myDoc->unIndent(c.cursor.line);
        return;
      }
    }
    if ( !(e->state() & ControlButton) && !(e->state() & AltButton)
         && myDoc->insertChars(c.cursor.line, c.cursor.col, e->text(), this->myView) )
    {
      e->accept();
      return;
    }
  }
  e->ignore();
}

void KateViewInternal::mousePressEvent(QMouseEvent *e) {

  if (e->button() == LeftButton) {
    if (possibleTripleClick) {
      possibleTripleClick = false;
      VConfig c;
      getVConfig(c);
      myDoc->selectLine(c.cursor, c.flags);
      cursor.col = 0;
      cursor.line = cursor.line;
      updateCursor( cursor, true );
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
      updateView();
    }
  }

  if (myView->popup() && e->button() == RightButton) {
    myView->popup()->popup(mapToGlobal(e->pos()));
  }
  myView->mousePressEvent(e); // this doesn't do anything, does it?
  // it does :-), we need this for KDevelop, so please don't uncomment it again -Sandy
}

void KateViewInternal::mouseDoubleClickEvent(QMouseEvent *e) {

  if (e->button() == LeftButton) {
    VConfig c;
    getVConfig(c);
    myDoc->selectWord(c.cursor, c.flags);
    // anders: move cursor to end of selected word
    if (myDoc->hasSelection())
    {
      cursor.col = myDoc->selectEnd.col;
      cursor.line = myDoc->selectEnd.line;
      updateCursor( cursor, true );
    }

    possibleTripleClick=true;
    QTimer::singleShot( QApplication::doubleClickInterval(),this,
            SLOT(tripleClickTimeout()) );
  }
}

void KateViewInternal::tripleClickTimeout()
{
    possibleTripleClick=false;
}


void KateViewInternal::mouseReleaseEvent(QMouseEvent *e) {

  if (e->button() == LeftButton) {
    if (dragInfo.state == diPending) {
      // we had a mouse down in selected area, but never started a drag
      // so now we kill the selection
      placeCursor(e->x(), e->y(), 0);
//      myDoc->updateViews();
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

  if (e->button() == MidButton) {
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
    updateView();
  }
}



void KateViewInternal::wheelEvent( QWheelEvent *e )
{
  if( yScroll->isVisible() == true )
  {
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

  QPainter paint;
  paint.begin(drawBuffer);

  if (rpos <= lineRanges.size())
    r += rpos;
  else
    return;

  bool b = myView->isOverwriteMode();
  bool again = true;
  for ( uint line = startline; (line <= endline) && (rpos < lineRanges.size()); line++)
  {
    if (r->empty)
    {
      paint.fillRect(0, 0, updateR.width(), h, myDoc->colors[0]);
//      bitBlt(this, 0, (line-startLine)*h, drawBuffer, 0, 0, this->width(), h);

//      paint.fillRect(xStart, 0, xEnd-xStart, h, myDoc->colors[0]);
    }
    else
    {
      myDoc->paintTextLine ( paint, r->line, r->startCol, r->endCol, 0, xStart, xEnd,
                                          (cursorOn && (r->line == cursor.line)) ? cursor.col : -1, b, true,
                                          myView->myDoc->_configFlags & KateDocument::cfShowTabs, KateDocument::ViewFont, again && (r->line == cursor.line));
   
     if ((cXPos > r->lengthPixel) && (cXPos>=xStart) && (cXPos<=xEnd))
      if (cursorOn && (r->line == cursor.line))
     {     
        if (b)
	{
	  int cursorMaxWidth = myDoc->viewFont.myFontMetrics.width(QChar (' '));
          paint.fillRect(cXPos-xPos, 0, cursorMaxWidth, myDoc->viewFont.fontHeight, myDoc->myAttribs[0].col);
	}
       else
         paint.fillRect(cXPos-xPos, 0, 2, myDoc->viewFont.fontHeight, myDoc->myAttribs[0].col);
      }
   }
  

    bitBlt(this, updateR.x(), (line-startLine)*h, drawBuffer, 0, 0, updateR.width(), h);

    if (r->line == cursor.line)
      again = false;

    r++;
    rpos++;
  }

  paint.end();

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
    yScroll->setValue(startLine * myDoc->viewFont.fontHeight + scrollY);

    placeCursor(mouseX, mouseY, KateDocument::cfMark);
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

      updateView();
    }
  }
}

void KateViewInternal::clear()
{
	cursor.col=cursor.line=0;
	displayCursor.col=displayCursor.line=0;
}
