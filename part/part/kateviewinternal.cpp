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

#include "kateview.h"
#include "kateviewinternal.h"
#include "kateviewinternal.moc"
#include "katedocument.h"

#include <kcursor.h>
#include <kapplication.h>

#include <qstyle.h>
#include <qdragobject.h>
#include <qdropsite.h>
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
  endLine = 0;

  xPos = 0;

  xCoord = 0;
  yCoord = 0;

  // create a one line lineRanges array
  updateLineRanges (0, false);

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
  newStartLine = -1;

  drawBuffer = new QPixmap ();
  drawBuffer->setOptimization (QPixmap::BestOptim);

  bm.sXPos = 0;
  bm.eXPos = -1;

  QWidget::setCursor(ibeamCursor);
  KCursor::setAutoHideCursor( this, true, true );

  setFocusPolicy(StrongFocus);

  xScroll = new QScrollBar(QScrollBar::Horizontal,myView);
  yScroll = new QScrollBar(QScrollBar::Vertical,myView);
  xScroll->hide(); //TEMPORARY

  connect(xScroll,SIGNAL(valueChanged(int)),SLOT(changeXPos(int)));
  connect(yScroll,SIGNAL(valueChanged(int)),SLOT(changeYPos(int)));
  
  setAcceptDrops(true);
  dragInfo.state = diNone;
}

KateViewInternal::~KateViewInternal()
{
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
      updateCursor(c.cursor);
#warning "OPTIMIZE THE FOLLOWING LINE"
      tagLines( displayCursor.line, endLine, 0, 0xffff);
      updateView(0); //really ?
      return;
#warning "FIXME FIXME FIXME, cursor updates are missing"
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
  bool lineChanged=false;
  cursor.col--;
  if (c.flags & KateDocument::cfWrapCursor && cursor.col < 0 && cursor.line > 0) {
    cursor.line=myDoc->getRealLine(displayCursor.line-1);
    cursor.col = myDoc->textLength(cursor.line);
    lineChanged=true;
  }
  calculateDisplayPositions(displayCursor,cursor,true,lineChanged);
  cOldXPos = cXPos = myDoc->textWidth(cursor);
  changeState(c);
}

void KateViewInternal::cursorRight(VConfig &c) {
  bool updateLine=false;
  if (c.flags & KateDocument::cfWrapCursor) {
    if (cursor.col >= (int) myDoc->textLength(cursor.line)) {
      if (cursor.line == (int)myDoc->lastLine()) return;
      cursor.line=myDoc->getRealLine(displayCursor.line+1);
      cursor.col = -1;
      updateLine=true;
    }
  }
  cursor.col++;
  calculateDisplayPositions(displayCursor,cursor,true,updateLine);
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
  if (displayCursor.line>0)
  {
    displayCursor.line--;
    cursor.line=myDoc->getRealLine(displayCursor.line);
    cXPos = myDoc->textWidth(c.flags & KateDocument::cfWrapCursor,cursor,cOldXPos);
    changeState(c);
  }
}


void KateViewInternal::cursorDown(VConfig &c) {
  int x;
  if (cursor.line == (int)myDoc->lastLine()) {
    x = myDoc->textLength(cursor.line);
    if (cursor.col >= x) return;
    cursor.col = x;
    calculateDisplayPositions(displayCursor,cursor,true,false);
    cXPos = myDoc->textWidth(cursor);
  } else {
    displayCursor.line++;
    cursor.line=myDoc->getRealLine(displayCursor.line);
  }
  cXPos = myDoc->textWidth(c.flags & KateDocument::cfWrapCursor, cursor, cOldXPos);
  changeState(c);
}

void KateViewInternal::scrollUp(VConfig &c) {
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

void KateViewInternal::scrollDown(VConfig &c) {

  if (endLine >= (int)myDoc->lastLine()) return;

  newStartLine = startLine + myDoc->viewFont.fontHeight;
  if (cursor.line == startLine) {
    cursor.line++;
    cXPos = myDoc->textWidth(c.flags & KateDocument::cfWrapCursor,cursor,cOldXPos);
    changeState(c);
  }
}

void KateViewInternal::topOfView(VConfig &c) {

  cursor.line = startLine;
  cursor.col = 0;
  cOldXPos = cXPos = 0;
  changeState(c);
}

void KateViewInternal::bottomOfView(VConfig &c) {

  cursor.line = startLine + (height()/myDoc->viewFont.fontHeight) - 1;
  if (cursor.line < 0) cursor.line = 0;
  if (cursor.line > (int)myDoc->lastLine()) cursor.line = myDoc->lastLine();
  cursor.col = 0;
  cOldXPos = cXPos = 0;
  changeState(c);
}

void KateViewInternal::pageUp(VConfig &c) {
  int lines = (endLine - startLine - 1);

  if (lines <= 0) lines = 1;

  if (startLine > 0) {
    newStartLine = startLine - lines;
    if (newStartLine < 0) newStartLine = 0;
  }

  displayCursor.line -= lines;
  if (displayCursor.line<0) displayCursor.line=0;
  cursor.line=myDoc->getRealLine(displayCursor.line);
  cXPos = myDoc->textWidth(c.flags & KateDocument::cfWrapCursor, cursor, cOldXPos);
  changeState(c);
//  cursorPageUp(c);
}

void KateViewInternal::pageDown(VConfig &c) {
  int lines = (endLine - startLine - 1);

  if (endLine < (int)myDoc->lastLine()) {
    if (lines < (int)myDoc->lastLine() - endLine)
      newStartLine = startLine + lines;
    else
      newStartLine = startLine + (myDoc->lastLine() - endLine);
  }
  displayCursor.line += lines;
  if (displayCursor.line>myDoc->numVisLines()) displayCursor.line=myDoc->numVisLines();
  cursor.line=myDoc->getRealLine(displayCursor.line);
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

void KateViewInternal::changeYPos(int p)
{
  int dy;

  newStartLine = p  / myDoc->viewFont.fontHeight;

  dy = (startLine - newStartLine)  * myDoc->viewFont.fontHeight;

  updateLineRanges(height());

  if (QABS(dy) < height())
  {
    leftBorder->scroll(0,dy);
    scroll(0, dy);
  }
  else
    update();
}


void KateViewInternal::getVConfig(VConfig &c) {

  c.view = myView;
  c.cursor = cursor;
  c.displayCursor=displayCursor;
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
      tagLines(c.displayCursor.line, c.displayCursor.line, c.cXPos, c.cXPos + myDoc->charWidth(c.cursor));
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

  if (!nullMove)
    emit myView->cursorPositionChanged();
}

void KateViewInternal::insLine(int line) {

  if (line <= cursor.line) {
    cursor.line++;
    displayCursor.line++;
  }
  if (line < startLine) {
    startLine++;
    endLine++;
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
  } else if (line <= endLine) {
    tagAll();
  }
}

void KateViewInternal::updateCursor()
{
  cOldXPos = cXPos = myDoc->textWidth(cursor);
}


void KateViewInternal::updateCursor(KateTextCursor &newCursor,bool keepSel)
{
  kdDebug()<<"WARNING: look, if this call is really used only with real cursor positions"<<endl;
  VConfig tmp;
  tmp.cursor=newCursor;
  tmp.displayCursor.col=newCursor.col;
  tmp.displayCursor.line=myDoc->getVirtualLine(newCursor.line);

  kdDebug()<<QString("cursor %1/%2, displayCursor %3/%4").arg(tmp.cursor.col).arg(tmp.cursor.line).arg(tmp.displayCursor.col).arg(tmp.displayCursor.line)<<endl;

  updateCursor(tmp,keepSel);
}

void KateViewInternal::updateCursor(VConfig &c,bool keepSel)//KateTextCursor &newCursor)
{

#warning "FIXME, FIXME, display/document cursor"

  if (!(myDoc->_configFlags & KateDocument::cfPersistent) && !keepSel) myDoc->clearSelection();

  exposeCursor = true;
  if (cursorOn) {
    tagLines(displayCursor.line, displayCursor.line, cXPos, cXPos +myDoc->charWidth(cursor));
//    tagLines(cursor.line, cursor.line, cXPos, cXPos +myDoc->charWidth(cursor));
    cursorOn = false;
  }

  if (bm.sXPos < bm.eXPos) {
    tagLines(bm.cursor.line, bm.cursor.line, bm.sXPos, bm.eXPos);
  }


  myDoc->newBracketMark(c.cursor, bm);

  cursor = c.cursor;//newCursor;
  displayCursor=c.displayCursor;
  cOldXPos = cXPos = myDoc->textWidth(cursor);
}

// init the line dirty cache
void KateViewInternal::updateLineRanges(uint height, bool keepLineData)
{
  int lines = 0;
  int oldStartLine = startLine;
  int oldLines = lineRanges.size();

  // calc start and end line of visible part
  if (newStartLine > -1)
    startLine = newStartLine;

  endLine = startLine + height/myDoc->viewFont.fontHeight - 1;

  if (endLine < 0) endLine = 0;

  kdDebug()<<"endLine: "<<endLine<<endl;

  lines = endLine - startLine + 1;

  if (lines > oldLines)
    lineRanges.resize (lines);

  for (uint z = 0; z < lines; z++)
  {
    lineRanges[z].line = myDoc->getRealLine (startLine+z);
  }

  if (lines < oldLines)
    lineRanges.resize (lines);

  newXPos = -1;
}

void KateViewInternal::tagRealLines(int start, int end, int x1, int x2)
{
  if (x1 <= 0) x1 = 0;
  if (x1 < xPos-2) x1 = xPos;
  if (x2 > width() + xPos) x2 = width() + xPos;
  if (x1 >= x2) return;
  
  for (uint z = 0; z < lineRanges.size(); z++)
  {
    if (lineRanges[z].line > end)
      break;

    if (lineRanges[z].line >= start)
    {
      if (x1 < lineRanges[z].start) lineRanges[z].start = x1;
      if (x2 > lineRanges[z].end) lineRanges[z].end = x2;

      updateState |= 1;
    }
  }
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

  if (start < lineRanges.size())
  {
    r = &lineRanges[start];
    uint rpos = start;

    for (z = start; z <= end; z++)
    {
      if (rpos >= lineRanges.size()) break;

      if (x1 < r->start) r->start = x1;
      if (x2 > r->end) r->end = x2;

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
  newStartLine = displayCursor.line - lineRanges.size()/2;
  if (newStartLine < 0) newStartLine = 0;
}


void KateViewInternal::singleShotUpdateView()
{
	updateView(0);
}

void KateViewInternal::updateView(int flags)
{
	unsigned int oldXPos=xPos;
	unsigned int oldYPos=startLine * myDoc->viewFont.fontHeight;
	bool yScrollVis=yScroll->isVisible();
	bool xScrollVis=xScroll->isVisible();
	int fontHeight = myDoc->viewFont.fontHeight;
	bool needLineRangesUpdate=false;
	uint lineRangesUpdateHeight=0;
	bool reUpdate;

	if (!exposeCursor)
	{
		do
		{
			reUpdate=false;
			int w = myView->width();
			int h = myView->height();
			int scrollbarWidth = style().scrollBarExtent().width();

			if (!flags) { //exposeCursor|| (flags & KateView::ufDocGeometry)) {
				int bw = leftBorder->width();
				w -= bw;
				if (yScrollVis) w -= scrollbarWidth;
		  		if (w != width() || h != height()) {
					needLineRangesUpdate=true;
					lineRangesUpdateHeight=h;
//			    		updateLineRanges(h);
			   	 	resize(w,h);
				}
			}

			unsigned int contentLines=myDoc->visibleLines(); /* temporary */

			unsigned int contentHeight=contentLines*fontHeight;
			int viewLines=height()/fontHeight;
			int yMax=(contentLines-viewLines)*fontHeight;

			if (yMax>0)
			{
				int pageScroll = h - (h % fontHeight) - fontHeight;
				if (pageScroll <= 0)
					pageScroll = fontHeight;

				yScroll->blockSignals(true);
				yScroll->setGeometry(myView->width()-scrollbarWidth,0,scrollbarWidth,
					myView->height()-(xScrollVis?scrollbarWidth : 0));
				yScroll->setRange(0,yMax);
				yScroll->setValue(startLine * myDoc->viewFont.fontHeight);
				yScroll->setSteps(fontHeight,pageScroll);
				yScroll->blockSignals(false);
				reUpdate=reUpdate || (!yScrollVis);
				yScrollVis=true;
				yScroll->show();
				kdDebug()<<"Showing yScroll"<<endl;
			}
			else
			{
				kdDebug()<<"Hiding yScroll"<<endl;
				reUpdate=reUpdate || (yScrollVis);
				yScroll->hide();
				yScrollVis=false;
			}


		} while (reUpdate);
	}

  int oldU = updateState;

   if (updateState > 0)  paintTextLines(oldXPos, oldYPos);

   if (updateState==3)
   {
	if ((!needLineRangesUpdate) ||
	(lineRangesUpdateHeight<height())) lineRangesUpdateHeight=height();
	needLineRangesUpdate=true;
	//updateLineRanges(height());update();}
    }
	int tmpYPos;
	if (exposeCursor)
	{
		exposeCursor=false;
		if (displayCursor.line>=endLine)
		{
			tmpYPos=(displayCursor.line*fontHeight)-height()+fontHeight;
			yScroll->setValue(tmpYPos);

		        if ((!needLineRangesUpdate) ||
		        (lineRangesUpdateHeight<height())) lineRangesUpdateHeight=height();
		        needLineRangesUpdate=true;
//			updateLineRanges(height());
		}
		else
		if (displayCursor.line<startLine)
		{
			tmpYPos=(displayCursor.line*fontHeight);
			yScroll->setValue(tmpYPos);

	                if ((!needLineRangesUpdate) ||
        	        (lineRangesUpdateHeight<height())) lineRangesUpdateHeight=height();
                	needLineRangesUpdate=true;
//			updateLineRanges(height());
		}
	}

  if (flags & KateView::ufFoldingChanged)
  {
	if ((!needLineRangesUpdate) ||
	(lineRangesUpdateHeight<height())) lineRangesUpdateHeight=height();
	needLineRangesUpdate=true;
	updateLineRanges(lineRangesUpdateHeight);
//	updateLineRanges (height());
    repaint ();
  }
  else
  {
	if (needLineRangesUpdate) updateLineRanges(lineRangesUpdateHeight);
  }

  if (oldU > 0)
    leftBorder->update ();
    
  //
  // updateView done, reset the update flag + repaint flags
  //
  updateState = 0;
  
  // blank repaint attribs
  for (uint z = 0; z < lineRanges.size(); z++)
  {
    lineRanges[z].start = 0xffffff;
    lineRanges[z].end = 0;
  }

//   updateLineRanges(height());
//   repaint();

//	update();
#if 0
	int fontHeight = myDoc->viewFont.fontHeight;
	int viewLinesMappingSize=height()/fontHeight+1;
        int line = (yPos ) / fontHeight;

	for (int i=line;i<viewLinesMappingSize+line;i++)
	{
		int *tmp=new int;
		*tmp=myDoc->getRealLine(i);
		m_lineMapping.replace(i-startLine,tmp);
	}
	leftBorder->repaint();
#endif

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
    updateLineRanges(h);
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
//WARNING EXPERIMENTALLY RENABLED
  if (!drawBuffer) return;
  if (drawBuffer->isNull()) return;

  QPainter paint;
  paint.begin(drawBuffer);

  uint h = myDoc->viewFont.fontHeight;
  KateLineRange *r = lineRanges.data();

  uint rpos = 0;
  kdDebug()<<QString("startLine: %1, endLine %2").arg(startLine).arg(endLine)<<endl;

  if (endLine>=startLine)
  {
    for ( uint line = startLine; (line <= endLine) && (rpos < lineRanges.size()); line++)
    {
      if (r->start < r->end)
      {
        myDoc->paintTextLine(paint, r->line, r->start, r->end, myView->myDoc->_configFlags & KateDocument::cfShowTabs);

        bitBlt(this, r->start - xPos, (line-startLine)*h, drawBuffer, 0, 0, r->end - r->start, h);
      }

      r++;
      rpos++;
    }
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
     tagLines( displayCursor.line, displayCursor.line, 0, 0xffff);
     paintTextLines (xPos, 0);
  }
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

void KateViewInternal::placeCursor(int x, int y, int flags) {
//#if 0
 VConfig c;

  getVConfig(c);

  int newDisplayLine;
  newDisplayLine=startLine + y/myDoc->viewFont.fontHeight;
  if (newDisplayLine>=myDoc->numVisLines()) return;
  if (((newDisplayLine-startLine) < 0) || ((newDisplayLine-startLine) >= lineRanges.size())) return;// not sure yet, if this is ther correct way;

  c.flags |= flags;
  displayCursor.line=newDisplayLine;
  cursor.line = lineRanges[displayCursor.line-startLine].line;
  cXPos = cOldXPos = myDoc->textWidth(c.flags & KateDocument::cfWrapCursor, cursor, xPos + x);
  displayCursor.col=cursor.col;
  changeState(c);
//#endif
}

// given physical coordinates, report whether the text there is selected
bool KateViewInternal::isTargetSelected(int x, int y) {
  //y = (yPos + y) / myDoc->viewFont.fontHeight;

#warning "This needs changing for dynWW"
  y=startLine + y/myDoc->viewFont.fontHeight;

  if (((y-startLine) < 0) || ((y-startLine) >= lineRanges.size())) return false;

  y= lineRanges[y-startLine].line;

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

  KKey key(e);
  if (myView->doc()->isReadWrite()) {
    if (c.flags & KateDocument::cfTabIndents && myDoc->hasSelection()) {
      if (key == Qt::Key_Tab) {
        myDoc->indent(c);
        return;
      }
      if (key == SHIFT+Qt::Key_Backtab || key == Qt::Key_Backtab) {
        myDoc->unIndent(c);
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
      myDoc->updateViews();
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
  line = startLine + (updateR.y() / h);
  y = (line-startLine)*h;
  yEnd = updateR.y() + updateR.height();

  int disppos=line-startLine;

  bool isVisible=false;
  while (y < yEnd)
  {
    if (disppos >= lineRanges.size())
      break;

    int realLine;
    isVisible=myDoc->paintTextLine(paint, lineRanges[disppos].line, xStart, xEnd, myView->myDoc->_configFlags & KateDocument::cfShowTabs);
    bitBlt(this, updateR.x(), y, drawBuffer, 0, 0, updateR.width(), h);

//    kdDebug()<<QString("paintevent: line %1, realLine %2").arg(line).arg(realLine)<<endl;

    leftBorder->paintLine(line,line);
    disppos++;
    y += h;
    line++;
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
    yScroll->setValue(startLine * myDoc->viewFont.fontHeight + scrollY);

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




void KateViewInternal::calculateDisplayPositions(KateTextCursor &disp,KateTextCursor cur,bool colChanged, bool lineChanged)
{
	if (lineChanged)
		disp.line=myDoc->getVirtualLine(cur.line);
	if (colChanged)
		disp.col=cur.col;
}

void KateViewInternal::clear()
{
	cursor.col=cursor.line=0;
	displayCursor.col=displayCursor.line=0;
}
