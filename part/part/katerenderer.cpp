/* This file is part of the KDE libraries
   Copyright (C) 2003 Hamish Rodda <rodda@kde.org>
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

#include "katerenderer.h"

#include "katelinerange.h"
#include "katedocument.h"
#include "katearbitraryhighlight.h"
#include "kateconfig.h"
#include "kateview.h"

#include <kdebug.h>

#include <qpainter.h>

static const QChar tabChar('\t');
static const QChar spaceChar(' ');

class KateRendererSettings
{
public:
  KateRendererSettings()
    : drawCaret(true)
    , caretStyle(KateRenderer::Insert)
    , showSelections(true)
    , showTabs(true)
    , font(KateRendererConfig::ViewFont)
    , printerFriendly(false)
  {
  }

  bool drawCaret;
  int caretStyle;
  bool showSelections;
  bool showTabs;
  int font;
  bool printerFriendly;
};

KateRenderer::KateRenderer(KateDocument* doc, KateView *view)
  : m_doc(doc), m_view (view)
{
  m_config = new KateRendererConfig (this);
  m_currentSettings = new KateRendererSettings();
}

KateRenderer::~KateRenderer()
{
  delete m_config;
}

bool KateRenderer::drawCaret() const
{
  return m_currentSettings->drawCaret;
}

void KateRenderer::setDrawCaret(bool drawCaret)
{
  m_currentSettings->drawCaret = drawCaret;
}

bool KateRenderer::caretStyle() const
{
  return m_currentSettings->caretStyle;
}

void KateRenderer::setCaretStyle(int style)
{
  m_currentSettings->caretStyle = style;
}

bool KateRenderer::showTabs() const
{
  return m_currentSettings->showTabs;
}

void KateRenderer::setShowTabs(bool showTabs)
{
  m_currentSettings->showTabs = showTabs;
}

bool KateRenderer::showSelections() const
{
  return m_currentSettings->showSelections;
}

void KateRenderer::setShowSelections(bool showSelections)
{
  m_currentSettings->showSelections = showSelections;
}

int KateRenderer::font() const
{
  return m_currentSettings->font;
}

void KateRenderer::setFont(int whichFont)
{
  m_currentSettings->font = whichFont;
}

void KateRenderer::increaseFontSizes()
{
  // FIXME broken!!
  //m_currentSettings->font.setPointSize(font.pointSize()+1);
}

void KateRenderer::decreaseFontSizes()
{
  // FIXME broken!!
  // m_currentSettings->font.setPointSize(m_currentSettings->font.pointSize()-1);
}

bool KateRenderer::isPrinterFriendly() const
{
  return m_currentSettings->printerFriendly;
}

void KateRenderer::setPrinterFriendly(bool printerFriendly)
{
  m_currentSettings->printerFriendly = printerFriendly;
  setFont(KateRendererConfig::PrintFont);
  setShowTabs(false);
  setShowSelections(false);
  setDrawCaret(false);
}

void KateRenderer::paintTextLine(QPainter& paint, const LineRange* range, int xStart, int xEnd, const KateTextCursor* cursor, const KateTextRange* bracketmark)
{
  static QTime timer;

  static const bool timeDebug = false;

  int showCursor = (drawCaret() && cursor && range->includesCursor(*cursor)) ? cursor->col() : -1;

  KateSuperRangeList& superRanges = m_doc->arbitraryHL()->rangesIncluding(range->line, 0);

  // A bit too verbose for my tastes
  // Re-write a bracketmark class? put into its own function? add more helper constructors to the range stuff?
  // Also, need a light-weight arbitraryhighlightrange class for static stuff
  ArbitraryHighlightRange* bracketStartRange = 0L;
  ArbitraryHighlightRange* bracketEndRange = 0L;
  ArbitraryHighlightRange* bracketRange = 0L;
  if (bracketmark && bracketmark->isValid()) {
    if (range->includesCursor(bracketmark->start())) {
      KateTextCursor startend = bracketmark->start();
      startend.setCol(startend.col()+1);
      bracketStartRange = new ArbitraryHighlightRange(m_doc, bracketmark->start(), startend);
      bracketStartRange->setBGColor(m_doc->colors[3]);
      superRanges.append(bracketStartRange);
    }

    if (range->includesCursor(bracketmark->end())) {
      KateTextCursor endend = bracketmark->end();
      endend.setCol(endend.col()+1);
      bracketEndRange = new ArbitraryHighlightRange(m_doc, bracketmark->end(), endend);
      bracketEndRange->setBGColor(m_doc->colors[3]);
      superRanges.append(bracketEndRange);
    }

    /*if (*range >= bracketmark->start() && *range <= bracketmark->end()) {
      if (bracketmark->start() < bracketmark->end())
        bracketRange = new ArbitraryHighlightRange(m_doc, bracketmark->start(), bracketmark->end());
      else
        bracketRange = new ArbitraryHighlightRange(m_doc, bracketmark->end(), bracketmark->start());

      bracketRange->setOutline(m_doc->colors[3]);
      superRanges.append(bracketRange);
    }*/
  }

  // font data
  const FontStruct * fs = config()->fontStruct(font());

  int line = range->line;
  bool currentLine = false;

  if (cursor && range->includesCursor(*cursor))
    currentLine = true;

  int startcol = range->startCol;
  int endcol = range->wrap ? range->endCol : -1;

  // text attribs font/style data
  KateAttribute* at = m_doc->attribs()->data();
  uint atLen = m_doc->attribs()->size();

  // textline
  TextLine::Ptr textLine = m_doc->kateTextLine(line);

  if (!textLine)
    // no success
    return;

  // length, chars + raw attribs
  uint len = textLine->length();
  uint oldLen = len;
  //const QChar *s;
  const uchar *a;

   // selection startcol/endcol calc
  bool hasSel = false;
  uint startSel = 0;
  uint endSel = 0;

  // was the selection background allready completly painted ?
  bool selectionPainted = false;

  // should the cursor be painted (if it is in the current xstart - xend range)
  bool cursorVisible = false;
  int cursorXPos = 0, cursorXPos2 = 0;
  int cursorMaxWidth = 0;
  const QColor *cursorColor = 0;

  // Normal background color
  QColor backgroundColor = m_doc->colors[0];

  // Paint selection background as the whole line is selected
  if (!isPrinterFriendly())
  {
    if (showSelections() && m_doc->lineSelected(line))
    {
      backgroundColor = m_doc->colors[1];
      selectionPainted = true;
      hasSel = true;
      startSel = 0;
      endSel = len + 1;
    }
    else
    {
      // paint the current line background if we're on the current line
      if (currentLine)
        backgroundColor = m_doc->colors[2];

      // Check for mark background
      int markRed = 0, markGreen = 0, markBlue = 0, markCount = 0;

      // Retrieve marks for this line
      uint mrk = m_doc->mark( line );

      if (mrk)
      {
        for (uint bit = 0; bit < 32; bit++)
        {
          KTextEditor::MarkInterface::MarkTypes markType = (KTextEditor::MarkInterface::MarkTypes)(1<<bit);
          if (mrk & markType)
          {
            QColor markColor = m_doc->markColor( markType );

            if (markColor.isValid()) {
              markCount++;
              markRed += markColor.red();
              markGreen += markColor.green();
              markBlue += markColor.blue();
            }
          }
        }
      }

      if (markCount) {
        markRed /= markCount;
        markGreen /= markCount;
        markBlue /= markCount;
        backgroundColor.setRgb(
          int((backgroundColor.red() * 0.9) + (markRed * 0.1)),
          int((backgroundColor.green() * 0.9) + (markGreen * 0.1)),
          int((backgroundColor.blue() * 0.9) + (markBlue * 0.1))
        );
      }
    }

    // Draw line background
    paint.fillRect(0, 0, xEnd - xStart, fs->fontHeight, backgroundColor);
  }

  // show word wrap marker if desirable
  if ( !isPrinterFriendly() && m_doc->m_wordWrapMarker && fs->myFont.fixedPitch() ) {
    paint.setPen( m_doc->colors[4] );
    int _x = m_doc->myWordWrapAt * fs->myFontMetrics.width('x') - xStart;
    paint.drawLine( _x,0,_x,fs->fontHeight );
  }

  if (startcol > (int)len)
    startcol = len;

  if (startcol < 0)
    startcol = 0;

  if (endcol < 0)
    len = len - startcol;
  else
    len = endcol - startcol;

  // text + attrib data from line
  a = textLine->attributes ();

  // adjust to startcol ;)
  a = a + startcol;

  uint curCol = startcol;

  // or we will see no text ;)
  int y = fs->fontAscent;

  // painting loop
  uint xPos = range->xOffset();
  uint xPosAfter = xPos;

  KateAttribute* oldAt = 0;

  const QColor *curColor = 0;
  const QColor *oldColor = 0;

  // Start arbitrary highlighting
  KateTextCursor currentPos(line, curCol);
  superRanges.firstBoundary(&currentPos);
  KateAttribute currentHL;

  if (showSelections() && !selectionPainted)
  {
    hasSel = selectBounds(line, startSel, endSel, oldLen);
  }

  uint oldCol = startcol;
  uint oldXPos = xPos;

  bool isSel = false;

  // Draws the dashed underline at the start of a folded block of text.
  if (range->startsInvisibleBlock) {
    paint.setPen(QPen(m_doc->colors[4], 1, Qt::DashLine));
    paint.drawLine(0, fs->fontHeight - 1, xEnd - xStart, fs->fontHeight - 1);
  }

  KateAttribute customHL;

  // Optimisation to quickly draw an empty line of text
  if (len < 1)
  {
    if ((showCursor > -1) && (showCursor >= (int)curCol))
    {
      cursorVisible = true;
      cursorXPos = xPos + (showCursor - (int) curCol) * fs->myFontMetrics.width(QChar (' '));
      cursorMaxWidth = xPosAfter - xPos;
      cursorColor = &at[0].textColor();
    }

  }
  else
  {
    // draw word-wrap-honour-indent filling
    if (range->xOffset() && range->xOffset() > xStart)
      paint.fillRect(0, 0, range->xOffset() - xStart, fs->fontHeight, QBrush(m_doc->colors[4], QBrush::DiagCrossPattern));

    // loop each character (tmp goes backwards, but curCol doesn't)
    for (uint tmp = len; (tmp > 0); tmp--)
    {
      // Determine cursor position
      if (showCursor > -1 && cursor->col() == (int)curCol)
        cursorXPos2 = xPos;

      // Decide if this character is a tab - we treat the spacing differently
      // TODO: move tab width calculation elsewhere?
      bool isTab = textLine->string()[curCol] == tabChar;

      // Determine current syntax highlighting attribute
      // A bit legacy but doesn't need to change
      KateAttribute* curAt = ((*a) >= atLen) ? &at[0] : &at[*a];

      // X position calculation. Incorrect for fonts with non-zero leftBearing() and rightBearing() results.
      // TODO: make internal charWidth() function, use QFontMetrics::charWidth().
      xPosAfter += curAt->width(*fs, textLine->string(), curCol, m_doc->config()->tabWidth());

      // Tab special treatment, move to charWidth().
      if (isTab)
        xPosAfter -= (xPosAfter % curAt->width(*fs, textLine->string(), curCol, m_doc->config()->tabWidth()));

      // Only draw after the starting X value
      // Haha, this was always wrong, due to the use of individual char width calculations...?? :(
      if ((int)xPosAfter >= xStart)
      {
        // Determine if we're in a selection and should be drawing it
        isSel = (showSelections() && hasSel && (curCol >= startSel) && (curCol < endSel));

        // Determine current color, taking into account selection
        curColor = isSel ? &(curAt->selectedTextColor()) : &(curAt->textColor());

        // Incorporate in arbitrary highlighting
        if (curAt != oldAt || curColor != oldColor || (superRanges.count() && superRanges.currentBoundary() && *(superRanges.currentBoundary()) == currentPos)) {
          if (superRanges.count() && superRanges.currentBoundary() && *(superRanges.currentBoundary()) == currentPos)
            customHL = ArbitraryHighlightRange::merge(superRanges.rangesIncluding(currentPos));

          KateAttribute hl = customHL;

          hl += *curAt;

          // use default highlighting color if we haven't defined one above.
          if (!hl.itemSet(KateAttribute::TextColor))
            hl.setTextColor(*curColor);

          if (!isSel)
            paint.setPen(hl.textColor());
          else
            paint.setPen(hl.selectedTextColor());

          paint.setFont(hl.font(*currentFont()));

          if (superRanges.currentBoundary() && *(superRanges.currentBoundary()) == currentPos)
            superRanges.nextBoundary();

          currentHL = hl;
        }

        // make sure we redraw the right character groups on attrib/selection changes
        // Special case... de-special case some of it
        if (isTab)
        {
          if (!isPrinterFriendly() && !selectionPainted) {
            if (isSel)
              paint.fillRect(oldXPos - xStart, 0, xPosAfter - oldXPos, fs->fontHeight, m_doc->colors[1]);
            else if (currentHL.itemSet(KateAttribute::BGColor))
              paint.fillRect(oldXPos - xStart, 0, xPosAfter - oldXPos, fs->fontHeight, currentHL.bgColor());
          }

          if (showTabs())
          {
            paint.drawPoint(xPos - xStart, y);
            paint.drawPoint(xPos - xStart + 1, y);
            paint.drawPoint(xPos - xStart, y - 1);
          }

          // variable advancement
          oldCol = curCol+1;
          oldXPos = xPosAfter;
        }
        // Reasons for NOT delaying the drawing until the next character
        // You have to detect the change one character in advance.
        // TODO: KateAttribute::canBatchRender()
        else if (
            // formatting has changed OR
            (superRanges.count() && superRanges.currentBoundary() && *(superRanges.currentBoundary()) == KateTextCursor(line, curCol+1)) ||

            // it is the end of the line OR
            (tmp < 2) ||

            // the x position is past the end OR
            ((int)xPos > xEnd) ||

            // it is a different attribute OR
            (curAt != &at[*(a+1)]) ||

            // the selection boundary was crossed OR
            (isSel != (hasSel && ((curCol+1) >= startSel) && ((curCol+1) < endSel))) ||

            // the next char is a tab (removed the "and this isn't" because that's dealt with above)
            // i.e. we have to draw the current text so the tab can be rendered as above.
            (textLine->string()[curCol+1] == tabChar)
          )
        {
          // TODO: genericise background painting
          if (!isPrinterFriendly() && !selectionPainted) {
            if (isSel)
              paint.fillRect(oldXPos - xStart, 0, xPosAfter - oldXPos, fs->fontHeight, m_doc->colors[1]);
            else if (currentHL.itemSet(KateAttribute::BGColor))
              paint.fillRect(oldXPos - xStart, 0, xPosAfter - oldXPos, fs->fontHeight, currentHL.bgColor());
          }

          // paint outline
          /*if (currentHL.itemSet(KateAttribute::Outline)) {
            paint.save();
            paint.setPen(currentHL.outline());
            paint.drawLine(oldXPos-xStart, 0, xPosAfter, 0);
            paint.drawLine(oldXPos-xStart, fs->fontHeight - 1, xPosAfter, fs->fontHeight - 1);
            paint.restore();
          }*/

          // Here's where the money is...
          paint.drawText(oldXPos-xStart, y, textLine->string(), oldCol, curCol+1-oldCol);

          // We're done drawing?
          if ((int)xPos > xEnd)
            break;

          // variable advancement
          oldCol = curCol+1;
          oldXPos = xPosAfter;
          //oldS = s+1;
        }

        // determine cursor X position
        if ((showCursor > -1) && (showCursor == (int)curCol))
        {
          cursorVisible = true;
          cursorXPos = xPos;
          cursorMaxWidth = xPosAfter - xPos;
          cursorColor = &curAt->textColor();
        }
      }
      else
      {
        // variable advancement
        oldCol = curCol+1;
        oldXPos = xPosAfter;
      }

      // increase xPos
      xPos = xPosAfter;

      // increase attribs pos
      a++;

      // to only switch font/color if needed
      oldAt = curAt;
      oldColor = curColor;

      // col move
      curCol++;
      currentPos.setCol(currentPos.col() + 1);
    }

    // Determine cursor position (if it is not within the range being drawn)
    if ((showCursor > -1) && (showCursor >= (int)curCol))
    {
      cursorVisible = true;
      cursorXPos = xPos + (showCursor - (int) curCol) * fs->myFontMetrics.width(QChar (' '));
      cursorMaxWidth = xPosAfter - xPos;
      cursorColor = &oldAt->textColor();
    }
  }

  // Draw dregs of the selection
  // TODO: genericise background painting
  if (!isPrinterFriendly() && showSelections() && !selectionPainted && m_doc->lineEndSelected (line, endcol))
  {
    paint.fillRect(xPos-xStart, 0, xEnd - xStart, fs->fontHeight, m_doc->colors[1]);
    selectionPainted = true;
  }

  // Paint cursor
  if (cursorVisible)
  {
    if (caretStyle() == Replace && (cursorMaxWidth > 2))
      paint.fillRect(cursorXPos-xStart, 0, cursorMaxWidth, fs->fontHeight, *cursorColor);
    else
      paint.fillRect(cursorXPos-xStart, 0, 2, fs->fontHeight, *cursorColor);
  }
  // Draw the cursor at the function user's specified position.
  // TODO: Why?????
  else if (showCursor > -1)
  {
    if ((cursorXPos2>=xStart) && (cursorXPos2<=xEnd))
    {
      cursorMaxWidth = fs->myFontMetrics.width(spaceChar);

      if (caretStyle() == Replace && (cursorMaxWidth > 2))
        paint.fillRect(cursorXPos2-xStart, 0, cursorMaxWidth, fs->fontHeight, m_doc->myAttribs[0].textColor());
      else
        paint.fillRect(cursorXPos2-xStart, 0, 2, fs->fontHeight, m_doc->myAttribs[0].textColor());
    }
  }

  //range->debugOutput();
  if (timeDebug /*&& timer.elapsed() > 10*/) {
    kdDebug() << k_funcinfo << "Elapsed: " << timer.elapsed() << "ms" << endl;
    //kdDebug() << textLine->string() << endl;
  }

  // unneeded?
  if (bracketStartRange) {
    Q_ASSERT(superRanges.removeRef(bracketStartRange));
    delete bracketStartRange;
  }

  if (bracketEndRange) {
    Q_ASSERT(superRanges.removeRef(bracketEndRange));
    delete bracketEndRange;
  }

  if (bracketRange) {
    Q_ASSERT(superRanges.removeRef(bracketRange));
    delete bracketRange;
  }
}

uint KateRenderer::textWidth(const TextLine::Ptr &textLine, int cursorCol)
{
  if (!textLine)
    return 0;

  int len = textLine->length();

  if (cursorCol < 0)
    cursorCol = len;

  const FontStruct *fs = config()->fontStruct(font());

  int x = 0;
  int width;
  for (int z = 0; z < cursorCol; z++) {
    KateAttribute* a = m_doc->attribute(textLine->attribute(z));

    if (z < len) {
      width = a->width(*fs, textLine->string(), z, m_doc->config()->tabWidth());
    } else {
      Q_ASSERT(!(m_doc->configFlags() & KateDocument::cfWrapCursor));
      width = a->width(*fs, spaceChar, m_doc->config()->tabWidth());
    }

    x += width;

    if (textLine->getChar(z) == tabChar)
      x -= x % width;
  }

  return x;
}

uint KateRenderer::textWidth(const TextLine::Ptr &textLine, uint startcol, uint maxwidth, bool *needWrap, int *endX)
{
  const FontStruct *fs = config()->fontStruct(font());
  uint x = 0;
  uint endcol = startcol;
  int endX2 = 0;
  int lastWhiteSpace = -1;
  int lastWhiteSpaceX = -1;

  // used to not wrap a solitary word off the first line, ie. the
  // first line should not wrap until some characters have been displayed if possible
  bool foundNonWhitespace = startcol != 0;
  bool foundWhitespaceAfterNonWhitespace = startcol != 0;

  *needWrap = false;

  uint z = startcol;
  for (; z < textLine->length(); z++)
  {
    KateAttribute* a = m_doc->attribute(textLine->attribute(z));
    int width = a->width(*fs, textLine->string(), z, m_doc->config()->tabWidth());
    Q_ASSERT(width);
    x += width;

    if (textLine->getChar(z).isSpace())
    {
      lastWhiteSpace = z+1;
      lastWhiteSpaceX = x;

      if (foundNonWhitespace)
        foundWhitespaceAfterNonWhitespace = true;
    }
    else
    {
      if (!foundWhitespaceAfterNonWhitespace) {
        foundNonWhitespace = true;

        lastWhiteSpace = z+1;
        lastWhiteSpaceX = x;
      }
    }

    // How should tabs be treated when they word-wrap on a print-out?
    // if startcol != 0, this messes up (then again, word wrapping messes up anyway)
    if (textLine->getChar(z) == '\t')
      x -= x % width;

    if (x <= maxwidth)
    {
      if (lastWhiteSpace > -1)
      {
        endcol = lastWhiteSpace;
        endX2 = lastWhiteSpaceX;
      }
      else
      {
        endcol = z+1;
        endX2 = x;
      }
    }
    else if (z == startcol)
    {
      // require a minimum of 1 character advancement per call, even if it means drawing gets cut off
      // (geez gideon causes troubles with starting the views very small)
      endcol = z+1;
      endX2 = x;
    }

    if (x >= maxwidth)
    {
      *needWrap = true;
      break;
    }
  }

  if (*needWrap)
  {
    if (endX)
      *endX = endX2;

    return endcol;
  }
  else
  {
    if (endX)
      *endX = x;

    return z+1;
  }
}

uint KateRenderer::textWidth(const KateTextCursor &cursor)
{
  int line = QMIN(QMAX(0, cursor.line()), (int)m_doc->numLines() - 1);
  int col = QMAX(0, cursor.col());

  return textWidth(m_doc->kateTextLine(line), col);
}

uint KateRenderer::textWidth( KateTextCursor &cursor, int xPos, uint startCol)
{
  bool wrapCursor = m_doc->configFlags() & KateDocument::cfWrapCursor;
  int len;
  int x, oldX;

  const FontStruct *fs = config()->fontStruct(font());

  if (cursor.line() < 0) cursor.setLine(0);
  if (cursor.line() > (int)m_doc->lastLine()) cursor.setLine(m_doc->lastLine());
  TextLine::Ptr textLine = m_doc->kateTextLine(cursor.line());

  if (!textLine) return 0;

  len = textLine->length();

  x = oldX = 0;
  int z = startCol;
  while (x < xPos && (!wrapCursor || z < len)) {
    oldX = x;

    KateAttribute* a = m_doc->attribute(textLine->attribute(z));

    int width = 0;

    if (z < len)
      width = a->width(*fs, textLine->string(), z, m_doc->config()->tabWidth());
    else
      width = a->width(*fs, spaceChar, m_doc->config()->tabWidth());

    x += width;

    if (textLine->getChar(z) == '\t')
      x -= x % width;

    z++;
  }
  if (xPos - oldX < x - xPos && z > 0) {
    z--;
    x = oldX;
  }
  cursor.setCol(z);
  return x;
}

const QFont *KateRenderer::currentFont()
{
  return config()->font(font());
}

const QFontMetrics* KateRenderer::currentFontMetrics()
{
  return config()->fontMetrics(font());
}

uint KateRenderer::textPos(uint line, int xPos, uint startCol)
{
  return textPos(m_doc->kateTextLine(line), xPos, startCol);
}

uint KateRenderer::textPos(const TextLine::Ptr &textLine, int xPos, uint startCol)
{
  Q_ASSERT(textLine);
  if (!textLine)
    return 0;

  const FontStruct *fs = config()->fontStruct(font());

  int x, oldX;
  x = oldX = 0;

  uint z = startCol;
  uint len= textLine->length();
  while ( (x < xPos)  && (z < len)) {
    oldX = x;

    KateAttribute* a = m_doc->attribute(textLine->attribute(z));
    x += a->width(*fs, textLine->string(), z, m_doc->config()->tabWidth());

    z++;
  }
  if (xPos - oldX < x - xPos && z > 0) {
    z--;
   // newXPos = oldX;
  }// else newXPos = x;
  return z;
}

uint KateRenderer::fontHeight()
{
  return config()->fontStruct (font())->fontHeight;
}

uint KateRenderer::documentHeight()
{
  return m_doc->numLines() * fontHeight();
}


bool KateRenderer::selectBounds(uint line, uint &start, uint &end, uint lineLength)
{
  bool hasSel = false;

  if (m_doc->hasSelection() && !m_doc->blockSelect)
  {
    if (m_doc->lineIsSelection(line))
    {
      start = m_doc->selectStart.col();
      end = m_doc->selectEnd.col();
      hasSel = true;
    }
    else if ((int)line == m_doc->selectStart.line())
    {
      start = m_doc->selectStart.col();
      end = lineLength;
      hasSel = true;
    }
    else if ((int)line == m_doc->selectEnd.line())
    {
      start = 0;
      end = m_doc->selectEnd.col();
      hasSel = true;
    }
  }
  else if (m_doc->lineHasSelected(line))
  {
    start = m_doc->selectStart.col();
    end = m_doc->selectEnd.col();
    hasSel = true;
  }

  if (start > end) {
    int temp = end;
    end = start;
    start = temp;
  }

  return hasSel;
}

void KateRenderer::updateConfig ()
{
  if (m_view)
  {
    m_view->m_viewInternal->updateView (true);
    m_view->m_viewInternal->repaint ();
  }
}

