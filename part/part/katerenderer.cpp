/* This file is part of the KDE libraries
   Copyright (C) 2003-2005 Hamish Rodda <rodda@kde.org>
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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "katerenderer.h"

#include "katelinerange.h"
#include "katedocument.h"
#include "kateconfig.h"
#include "katehighlight.h"
#include "kateview.h"
#include "katefont.h"
#include "katerenderrange.h"

#include <limits.h>

#include <kdebug.h>

#include <QPainter>
#include <QTextLayout>
#include <QStack>

static const QChar tabChar('\t');
static const QChar spaceChar(' ');

KateRenderer::KateRenderer(KateDocument* doc, KateView *view)
  : m_doc(doc), m_view (view), m_caretStyle(KateRenderer::Insert)
    , m_drawCaret(true)
    , m_showSelections(true)
    , m_showTabs(true)
    , m_printerFriendly(false)
    , m_dynamicRegion(doc)
{
  m_config = new KateRendererConfig (this);

  m_tabWidth = m_doc->config()->tabWidth();
  m_indentWidth = m_tabWidth;
  if (m_doc->config()->configFlags() & KateDocumentConfig::cfSpaceIndent)
  {
    m_indentWidth = m_doc->config()->indentationWidth();
  }

  updateAttributes ();
}

KateRenderer::~KateRenderer()
{
  delete m_config;
}

void KateRenderer::updateAttributes ()
{
  m_schema = config()->schema ();
  m_attributes = m_doc->highlight()->attributes (m_schema);
}

KTextEditor::Attribute* KateRenderer::attribute(uint pos) const
{
  if (pos < (uint)m_attributes->size())
    return &(*m_attributes)[pos];

  return &(*m_attributes)[0];
}

KTextEditor::Attribute * KateRenderer::specificAttribute( int context ) const
{
  if (context >= 0 && context < m_attributes->count())
    return &(*m_attributes)[context];

  return &(*m_attributes)[0];
}

void KateRenderer::setDrawCaret(bool drawCaret)
{
  m_drawCaret = drawCaret;
}

void KateRenderer::setCaretStyle(KateRenderer::caretStyles style)
{
  m_caretStyle = style;
}

void KateRenderer::setShowTabs(bool showTabs)
{
  m_showTabs = showTabs;
}

void KateRenderer::setTabWidth(int tabWidth)
{
  m_tabWidth = tabWidth;
}

bool KateRenderer::showIndentLines() const
{
  return m_config->showIndentationLines();
}

void KateRenderer::setShowIndentLines(bool showIndentLines)
{
  m_config->setShowIndentationLines(showIndentLines);
}

void KateRenderer::setIndentWidth(int indentWidth)
{
  m_indentWidth = m_tabWidth;
  if (m_doc->config()->configFlags() & KateDocumentConfig::cfSpaceIndent)
  {
    m_indentWidth = indentWidth;
  }
}

void KateRenderer::setShowSelections(bool showSelections)
{
  m_showSelections = showSelections;
}

void KateRenderer::increaseFontSizes()
{
  QFont f ( *config()->font () );
  f.setPointSize (f.pointSize ()+1);

  config()->setFont (f);
}

void KateRenderer::decreaseFontSizes()
{
  QFont f ( *config()->font () );

  if ((f.pointSize ()-1) > 0)
    f.setPointSize (f.pointSize ()-1);

  config()->setFont (f);
}

bool KateRenderer::isPrinterFriendly() const
{
  return m_printerFriendly;
}

void KateRenderer::setPrinterFriendly(bool printerFriendly)
{
  m_printerFriendly = printerFriendly;
  setShowTabs(false);
  setShowSelections(false);
  setDrawCaret(false);
}

void KateRenderer::paintTextLineBackground(QPainter& paint, KateLineLayoutPtr layout, int currentViewLine, int xStart, int xEnd)
{
  if (isPrinterFriendly())
    return;

  // font data
  KateFontStruct *fs = config()->fontStruct();

  // Normal background color
  QColor backgroundColor( config()->backgroundColor() );

  // paint the current line background if we're on the current line
  QColor currentLineColor = config()->highlightedLineColor();

  // Check for mark background
  int markRed = 0, markGreen = 0, markBlue = 0, markCount = 0;

  // Retrieve marks for this line
  uint mrk = m_doc->mark( layout->line() );
  if (mrk)
  {
    for (uint bit = 0; bit < 32; bit++)
    {
      KTextEditor::MarkInterface::MarkTypes markType = (KTextEditor::MarkInterface::MarkTypes)(1<<bit);
      if (mrk & markType)
      {
        QColor markColor = config()->lineMarkerColor(markType);

        if (markColor.isValid()) {
          markCount++;
          markRed += markColor.red();
          markGreen += markColor.green();
          markBlue += markColor.blue();
        }
      }
    } // for
  } // Marks

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

  // Draw line background
  paint.fillRect(0, 0, xEnd - xStart, fs->fontHeight * layout->viewLineCount(), backgroundColor);

  // paint the current line background if we're on the current line
  if (currentViewLine != -1) {
    if (markCount) {
      markRed /= markCount;
      markGreen /= markCount;
      markBlue /= markCount;
      currentLineColor.setRgb(
        int((currentLineColor.red() * 0.9) + (markRed * 0.1)),
        int((currentLineColor.green() * 0.9) + (markGreen * 0.1)),
        int((currentLineColor.blue() * 0.9) + (markBlue * 0.1))
      );
    }

    paint.fillRect(0, fs->fontHeight * currentViewLine, xEnd - xStart, fs->fontHeight, currentLineColor);
  }
}

void KateRenderer::paintWhitespaceMarker(QPainter &paint, uint x, uint y)
{
  QPen penBackup( paint.pen() );
  paint.setPen( config()->tabMarkerColor() );
  paint.drawPoint(x,     y);
  paint.drawPoint(x + 1, y);
  paint.drawPoint(x,     y - 1);
  paint.setPen( penBackup );
}

void KateRenderer::paintIndentMarker(QPainter &paint, uint x, uint row)
{
  QPen penBackup( paint.pen() );
  paint.setPen( config()->tabMarkerColor() );

  const int height = config()->fontStruct()->fontHeight;
  const int top = 0;
  const int bottom = height-1;
  const int h = bottom - top + 1;

  // Dot padding.
  int pad = 0;
  if(row & 1 && h & 1) pad = 1;

  for(int i = top; i <= bottom; i++)
  {
    if((i + pad) & 1)
    {
      paint.drawPoint(x + 2, i);
    }
  }

  paint.setPen( penBackup );
}

QList<QTextLayout::FormatRange> KateRenderer::decorationsForLine( const KateTextLine::Ptr& textLine, int line, bool selectionsOnly, KateRenderRange* completionHighlight, bool completionSelected ) const
{
  QList<QTextLayout::FormatRange> newHighlight;

  if (textLine->attributesList().count() || m_view->externalHighlights().count() || m_view->internalHighlights().count() || m_doc->documentHighlights().count()) {
    RenderRangeList renderRanges;

    if (!completionHighlight) {
      renderRanges.appendRanges(m_view->internalHighlights(), selectionsOnly, view());
      renderRanges.appendRanges(m_view->externalHighlights(), selectionsOnly, view());
      renderRanges.appendRanges(m_doc->documentHighlights(), selectionsOnly, view());

    } else {
      renderRanges.append(completionHighlight);
    }

    NormalRenderRange* inbuiltHighlight = new NormalRenderRange();
    const QVector<int> &al = textLine->attributesList();
    for (int i = 0; i+2 < al.count(); i += 3) {
      inbuiltHighlight->addRange(new KTextEditor::Range(KTextEditor::Cursor(line, al[i]), al[i+1]), specificAttribute(al[i+2]));
    }
    renderRanges.append(inbuiltHighlight);

    NormalRenderRange* selectionHighlight = 0L;
    if ((selectionsOnly && showSelections() && m_view->selection()) || (completionHighlight && completionSelected)) {
      selectionHighlight = new NormalRenderRange();
      static KTextEditor::Attribute backgroundAttribute;
      backgroundAttribute.setBackground(config()->selectionColor());

      if (completionHighlight && completionSelected)
        selectionHighlight->addRange(new KTextEditor::Range(line, 0, line + 1, 0), &backgroundAttribute);
      else
        selectionHighlight->addRange(new KTextEditor::Range(m_view->selectionRange()), &backgroundAttribute);

      renderRanges.append(selectionHighlight);
    }

    KTextEditor::Cursor currentPosition, endPosition;

    if (selectionsOnly) {
      KTextEditor::Range rangeNeeded = m_view->selectionRange().encompass(m_dynamicRegion.boundingRange());
      rangeNeeded &= KTextEditor::Range(line, 0, line + 1, 0);

      currentPosition = qMax(KTextEditor::Cursor(line, 0), rangeNeeded.start());
      endPosition = qMin(KTextEditor::Cursor(line + 1, 0), rangeNeeded.end());

    } else {
      currentPosition = KTextEditor::Cursor(line, 0);
      endPosition = KTextEditor::Cursor(line + 1, 0);
    }

    do {
      renderRanges.advanceTo(currentPosition);

      if (!renderRanges.hasAttribute()) {
        currentPosition = renderRanges.nextBoundary();
        continue;
      }

      KTextEditor::Cursor nextPosition = renderRanges.nextBoundary();

      QTextLayout::FormatRange fr;
      fr.start = currentPosition.column();

      if (nextPosition < endPosition) {
        fr.length = nextPosition.column() - currentPosition.column();

      } else {
        if (endPosition.line() > line)
          // +1 to force background drawing at the end of the line when it's warranted
          fr.length = textLine->length() - currentPosition.column() + 1;
        else
          fr.length = endPosition.column() - currentPosition.column();
      }

      KTextEditor::Attribute a = renderRanges.generateAttribute();
      fr.format = a;
      if (m_view->selection() && m_view->selectionRange().contains(currentPosition) && a.hasProperty(KTextEditor::Attribute::SelectedForeground))
        fr.format.setForeground(a.selectedForeground());

      newHighlight.append(fr);

      currentPosition = nextPosition;

    } while (currentPosition < endPosition);

    delete inbuiltHighlight;
    delete selectionHighlight;
  }

  return newHighlight;
}

/*
The ultimate line painting function.
Currently missing features:
- draw indent lines
- draw input method hints
*/
void KateRenderer::paintTextLine(QPainter& paint, KateLineLayoutPtr range, int xStart, int xEnd, const KTextEditor::Cursor* cursor)
{
  Q_ASSERT(range->isValid());

  // font data
  KateFontStruct* fs = config()->fontStruct();

  // Paint selection background as the whole line is selected
  // selection startcol/endcol calc
  KTextEditor::Range selection = m_view->selection() ? m_view->selectionRange() : KTextEditor::Range::invalid();

  int currentViewLine = -1;
  if (cursor && cursor->line() == range->line())
    currentViewLine = range->viewLineForColumn(cursor->column());

  paintTextLineBackground(paint, range, currentViewLine, xStart, xEnd);

  // Draws the dashed underline at the start of a folded block of text.
  if (range->startsInvisibleBlock()) {
    paint.setPen(QPen(config()->wordWrapMarkerColor(), 1, Qt::DashLine));
    paint.drawLine(0, (fs->fontHeight * range->viewLineCount()) - 1, xEnd - xStart, (fs->fontHeight * range->viewLineCount()) - 1);
  }

  if (range->layout()) {
    QVector<QTextLayout::FormatRange> additionalFormats;
    if (range->length() > 0) {
      // Draw the text :)
      if (m_dynamicRegion.boundingRange().isValid() || (m_view->selection() && showSelections() && m_view->selectionRange().overlapsLine(range->line()))) {
        // FIXME toVector() may be a performance issue
        additionalFormats = decorationsForLine(range->textLine(), range->line(), true).toVector();
        range->layout()->draw(&paint, QPoint(-xStart,0), additionalFormats);

      } else {
        range->layout()->draw(&paint, QPoint(-xStart,0));
      }
    }

    // Loop each individual line for additional text decoration etc.
    QListIterator<QTextLayout::FormatRange> it = range->layout()->additionalFormats();
    QVectorIterator<QTextLayout::FormatRange> it2 = additionalFormats;
    for (int i = 0; i < range->viewLineCount(); ++i) {
      KateTextLayout line = range->viewLine(i);

      // Determine the background color to use, if any, for the end of this view line
      QColor backgroundColor;
      while (it2.hasNext()) {
        const QTextLayout::FormatRange& fr = it2.peekNext();
        if (fr.start > line.endCol())
          goto backgroundFound;

        if (fr.start + fr.length > line.endCol()) {
          if (fr.format.hasProperty(QTextFormat::BackgroundBrush))
            backgroundColor = fr.format.background().color();
          goto backgroundFound;
        }

        it2.next();
      }

      while (it.hasNext()) {
        const QTextLayout::FormatRange& fr = it.peekNext();
        if (fr.start > line.endCol())
          break;

        if (fr.start + fr.length > line.endCol()) {
          if (fr.format.hasProperty(QTextFormat::BackgroundBrush))
            backgroundColor = fr.format.background().color();
          break;
        }

        it.next();
      }

      backgroundFound:

      // Draw selection outside of areas where text is rendered
      if (m_view->selection() && m_view->lineEndSelected(line.end())) {
        QRect area(line.endX() + line.xOffset() - xStart, fs->fontHeight * i, xEnd - xStart, fs->fontHeight * (i + 1));
        paint.fillRect(area, config()->selectionColor());

      } else if (backgroundColor.isValid()) {
        // Draw text background outside of areas where text is rendered.
        QRect area(line.endX() + line.xOffset() - xStart, fs->fontHeight * i, xEnd - xStart, fs->fontHeight * (i + 1));
        paint.fillRect(area, backgroundColor);
      }

      // Draw tab stops
      if (showTabs()) {
        const QString& text = range->textLine()->string();
        int tabIndex = text.indexOf(tabChar);
        int y = (fs->fontHeight * i) + fs->fontAscent;
        while (tabIndex != -1) {
          paintWhitespaceMarker(paint, (int)line.lineLayout().cursorToX(tabIndex) - xStart, y);
          tabIndex = text.indexOf(tabChar, tabIndex + 1);
        }

        // FIXME need to add config option for draw trailing spaces
        int spaceIndex = text.count() - 1;
        while (spaceIndex >= 0 && text.at(spaceIndex).isSpace()) {
          paintWhitespaceMarker(paint, (int)line.lineLayout().cursorToX(spaceIndex) - xStart, y);
          --spaceIndex;
        }
      }
    }

    // draw word-wrap-honor-indent filling
    if (range->viewLineCount() > 1 && range->shiftX() && range->shiftX() > xStart)
    {
      paint.fillRect(0, fs->fontHeight, range->shiftX() - xStart, fs->fontHeight * (range->viewLineCount() - 1),
        QBrush(config()->wordWrapMarkerColor(), Qt::DiagCrossPattern));
    }

    // Draw caret
    if (drawCaret() && cursor && range->includesCursor(*cursor)) {
      // Make the caret the desired width
      int caretWidth = 2;
      QTextLine line = range->layout()->lineForTextPosition(cursor->column());
      if (caretStyle() == Replace) {
        caretWidth = int(line.cursorToX(cursor->column() + 1) - line.cursorToX(cursor->column()));
        if (caretWidth < 0)
          caretWidth = -caretWidth;
      }

      QColor c;
      // Could actually use the real highlighting system for this... would be slower but more accurate for corner cases
      foreach (QTextLayout::FormatRange r, range->layout()->additionalFormats())
        if (r.start <= cursor->column() && r.start + r.length > cursor->column()) {
          c = r.format.foreground().color();
          break;
        }
      if (!c.isValid())
        if (range->layout()->additionalFormats().count())
          c = range->layout()->additionalFormats().last().format.foreground().color();
        else
          c = Qt::black;

      paint.save();
      paint.setPen(QPen(c, caretWidth));

      // Clip the caret - Qt's caret has a habit of intruding onto other lines
      paint.setClipRect(0, line.lineNumber() * fs->fontHeight, xEnd - xStart, fs->fontHeight);

      // Draw the cursor, start drawing in the middle as the above sets the width from the centre of the line
      range->layout()->drawCursor(&paint, QPoint(caretWidth/2 - xStart,0), cursor->column());
      paint.restore();
    }
  }

  /* Remnants from the old implementation which haven't yet been integrated into the new
  {
    bool isIMSel  = false;
    bool isIMEdit = false;

    while (curCol - startcol < len)
    {
      // Only draw after the starting X value
      if ((int)xPosAfter >= xStart)
      {
        // input method edit area
        isIMEdit = m_view && m_view->isIMEdit( line, curCol );

        // input method selection
        isIMSel = m_view && m_view->isIMSelection( line, curCol );

          // indentation lines OR
          || (showIndentLines() && curCol < lastIndentColumn)

          // the next char is a tab (removed the "and this isn't" because that's dealt with above)
          // i.e. we have to draw the current text so the tab can be rendered as above.
          || (textLine->string()[nextCol] == tabChar)

          // input method edit area
          || ( m_view && (isIMEdit != m_view->isIMEdit( line, nextCol )) )

          // input method selection
          || ( m_view && (isIMSel !=  m_view->isIMSelection( line, nextCol )) )

            if (isIMSel && !isTab)
            {
              // input method selection
              fillColor = m_view->colorGroup().color(QColorGroup::Foreground);
            }
            else if (isIMEdit && !isTab)
            {
              // XIM support
              // input method edit area
              const QColorGroup& cg = m_view->colorGroup();
              int h1, s1, v1, h2, s2, v2;
              cg.color( QColorGroup::Base ).hsv( &h1, &s1, &v1 );
              cg.color( QColorGroup::Background ).hsv( &h2, &s2, &v2 );
              fillColor.setHsv( h1, s1, ( v1 + v2 ) / 2 );
            }

            if (isIMSel && paintBackground && !isTab)
            {
              paint.save();
              paint.setPen( m_view->colorGroup().color( QColorGroup::BrightText ) );
            }

            // Draw indentation markers.
            if (showIndentLines() && curCol < lastIndentColumn)
            {
              // Draw multiple guides when tab width greater than indent width.
              const int charWidth = isTab ? m_tabWidth - curPos % m_tabWidth : 1;

              // Do not draw indent guides on the first line.
              int i = 0;
              if (curPos == 0 || curPos % m_indentWidth > 0)
                i = m_indentWidth - curPos % m_indentWidth;

              for (; i < charWidth; i += m_indentWidth)
              {
                // In most cases this is done one or zero times.
                paintIndentMarker(paint, xPos - xStart + i * spaceWidth, line);

                // Draw highlighted line.
                if (curPos+i == minIndent)
                {
                  paintIndentMarker(paint, xPos - xStart + 1 + i * spaceWidth, line+1);
                }
              }
            }

            // Draw preedit's underline
            if (isIMEdit) {
              QRect r( oldXPos - xStart, 0, xPosAfter - oldXPos, fs->fontHeight );
              paint.drawLine( r.bottomLeft(), r.bottomRight() );
            }

            // Put pen color back
            if (isIMSel) paint.restore();

      // Update the current indentation pos.
      if (isTab)
      {
        curPos += m_tabWidth - (curPos % m_tabWidth);
      }
      else
      {
        curPos++;
      }
    }
  */

  // show word wrap marker if desirable
  if (!isPrinterFriendly() && config()->wordWrapMarker() && fs->fixedPitch())
  {
    paint.setPen( config()->wordWrapMarkerColor() );
    int _x = m_doc->config()->wordWrapAt() * fs->myFontMetrics.width('x') - xStart;
    paint.drawLine( _x,0,_x,fs->fontHeight );
  }
}

const QFont *KateRenderer::currentFont() const
{
  return config()->font();
}

const QFontMetrics* KateRenderer::currentFontMetrics() const
{
  return config()->fontMetrics();
}

uint KateRenderer::fontHeight()
{
  return config()->fontStruct ()->fontHeight;
}

uint KateRenderer::documentHeight()
{
  return m_doc->lines() * fontHeight();
}

bool KateRenderer::getSelectionBounds(int line, int lineLength, int &start, int &end) const
{
  bool hasSel = false;

  if (m_view->selection() && !m_view->blockSelectionMode())
  {
    if (m_view->lineIsSelection(line))
    {
      start = m_view->selectionRange().start().column();
      end = m_view->selectionRange().end().column();
      hasSel = true;
    }
    else if (line == m_view->selectionRange().start().line())
    {
      start = m_view->selectionRange().start().column();
      end = lineLength;
      hasSel = true;
    }
    else if (m_view->selectionRange().containsLine(line))
    {
      start = 0;
      end = lineLength;
      hasSel = true;
    }
    else if (line == m_view->selectionRange().end().line())
    {
      start = 0;
      end = m_view->selectionRange().end().column();
      hasSel = true;
    }
  }
  else if (m_view->lineHasSelected(line))
  {
    start = m_view->selectionRange().start().column();
    end = m_view->selectionRange().end().column();
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
  // update the attibute list pointer
  updateAttributes ();

  if (m_view)
    m_view->updateRendererConfig();
}

uint KateRenderer::spaceWidth() const
{
  return config()->fontStruct()->myFontMetrics.width(spaceChar);
}

void KateRenderer::layoutLine(KateLineLayoutPtr lineLayout, int maxwidth, bool cacheLayout) const
{
  // if maxwidth == -1 we have no wrap

  Q_ASSERT(lineLayout->textLine());
  Q_ASSERT(currentFont());

  QTextLayout* l = new QTextLayout(lineLayout->textLine()->string(), config()->fontStruct()->font(false, false));
  l->setCacheEnabled(cacheLayout);

  // Initial setup of the QTextLayout.

  // Tab width
  QTextOption opt;
  opt.setFlags(QTextOption::IncludeTrailingSpaces);
  opt.setTabStop(m_tabWidth * config()->fontStruct()->width(spaceChar, false, false, m_tabWidth));
  l->setTextOption(opt);

  // Syntax highlighting, inbuilt and arbitrary
  l->setAdditionalFormats(decorationsForLine(lineLayout->textLine(), lineLayout->line()));

  // Begin layouting
  l->beginLayout();

  int height = 0;
  int shiftX = 0;

  forever {
    QTextLine line = l->createLine();
    if (!line.isValid())
      break;

    if (maxwidth > 0)
      line.setLineWidth(maxwidth);

    line.setPosition(QPoint(line.lineNumber() ? shiftX : 0, height));

    if (!line.lineNumber() && maxwidth != -1) {
      // Determine x offset for subsequent-lines-of-paragraph indenting
      if (m_view->config()->dynWordWrapAlignIndent() > 0)
      {
        if (shiftX == 0)
        {
          int pos = lineLayout->textLine()->nextNonSpaceChar(0);

          if (pos > 0) {
            shiftX = (int)line.cursorToX(pos);
          }

          if (shiftX > ((double)maxwidth / 100 * m_view->config()->dynWordWrapAlignIndent()) || shiftX == -1)
            shiftX = 0;
        }
      }
      lineLayout->setShiftX(shiftX);
    }

    height += config()->fontStruct()->fontHeight;
  }

  l->endLayout();

  lineLayout->setLayout(l);
}

int KateRenderer::cursorToX(const KateTextLayout& range, int col) const
{
  return cursorToX(range, KTextEditor::Cursor(range.line(), col));
}

int KateRenderer::cursorToX(const KateTextLayout& range, const KTextEditor::Cursor & pos) const
{
  Q_ASSERT(range.isValid());

  return (int)range.lineLayout().cursorToX(pos.column());
}

KTextEditor::Cursor KateRenderer::xToCursor(const KateTextLayout & range, int x, bool returnPastLine ) const
{
  Q_ASSERT(range.isValid());

  int adjustedX = x;

  if (adjustedX <  range.xOffset())
    adjustedX = range.xOffset();
  else if (adjustedX > range.width() + range.xOffset())
    adjustedX = range.width() + range.xOffset();

  Q_ASSERT(range.isValid());
  KTextEditor::Cursor ret(range.line(), range.lineLayout().xToCursor(adjustedX));

  if (returnPastLine && x > range.width() + range.xOffset())
    ret.setColumn(ret.column() + ((x - (range.width() + range.xOffset())) / spaceWidth()));

  return ret;
}

// kate: space-indent on; indent-width 2; replace-tabs on;
