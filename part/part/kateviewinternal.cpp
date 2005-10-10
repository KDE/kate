/* This file is part of the KDE libraries
   Copyright (C) 2002 John Firebaugh <jfirebaugh@kde.org>
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2002,2003 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002-2005 Hamish Rodda <rodda@kde.org>
   Copyright (C) 2003 Anakim Border <aborder@sources.sourceforge.net>

   Based on:
     KWriteView : Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>

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

#include "kateviewinternal.h"
#include "kateviewinternal.moc"

#include "kateview.h"
#include "katecodefoldinghelpers.h"
#include "kateviewhelpers.h"
#include "katehighlight.h"
#include "katesmartcursor.h"
#include "katerenderer.h"
#include "katecodecompletion.h"
#include "kateconfig.h"
#include "katelayoutcache.h"
#include "katefont.h"

#include <kcursor.h>
#include <kdebug.h>
#include <kapplication.h>
#include <kglobalsettings.h>

#include <QMimeData>
#include <QPainter>
#include <QLayout>
#include <QClipboard>
#include <QPixmap>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QClipboard>

KateViewInternal::KateViewInternal(KateView *view, KateDocument *doc)
  : QWidget (view)
  , editSessionNumber (0)
  , editIsRunning (false)
  , m_view (view)
  , m_doc (doc)
  , m_cursor(doc)
  , m_mouse(doc)
  , m_possibleTripleClick (false)
  , m_bm(doc)
  , m_bmStart(doc)
  , m_bmEnd(doc)
  , m_dummy (0)
  , m_startPos(doc)
  , m_madeVisible(false)
  , m_shiftKeyPressed (false)
  , m_autoCenterLines (false)
  , m_columnScrollDisplayed(false)
  , m_selChangedByUser (false)
  , m_selectAnchor (-1, -1)
  , m_selectionMode( Default )
  , m_layoutCache(new KateLayoutCache(renderer()))
  , m_preserveMaxX(false)
  , m_currentMaxX(0)
  , m_usePlainLines(false)
  , m_updatingView(true)
  , m_cachedMaxStartPos(-1, -1)
  , m_dragScrollTimer(this)
  , m_scrollTimer (this)
  , m_cursorTimer (this)
  , m_textHintTimer (this)
  , m_suppressColumnScrollBar(false)
  , m_textHintEnabled(false)
  , m_textHintMouseX(-1)
  , m_textHintMouseY(-1)
{
  // Set up bracket marking
  static KTextEditor::Attribute bracketOutline, bracketFill;
  if (!bracketOutline.isSomethingSet())
    bracketOutline.setOutline(m_view->m_renderer->config()->highlightedBracketColor());
  if (!bracketFill.isSomethingSet()) {
    bracketFill.setBGColor(m_view->m_renderer->config()->highlightedBracketColor(), false);
    bracketFill.setBold(true);
  }

  /*m_bm.setAttribute(&bracketOutline, false);
  m_bmStart.setAttribute(&bracketFill, false);
  m_bmEnd.setAttribute(&bracketFill, false);*/

  setMinimumSize (0,0);

  // cursor
  m_cursor.setMoveOnInsert (true);

  // invalidate m_selectionCached.start(), or keyb selection is screwed initially
  m_selectionCached = KTextEditor::Range::invalid();
  //
  // scrollbar for lines
  //
  m_lineScroll = new KateScrollBar(Qt::Vertical, this);
  m_lineScroll->show();
  m_lineScroll->setTracking (true);
  m_lineScroll->setSizePolicy( QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding));

  if (!m_view->dynWordWrap())
  {
    // bottom corner box
    m_dummy = new QWidget(m_view);
    m_dummy->setFixedHeight(m_lineScroll->width());
    m_dummy->setSizePolicy( QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    m_dummy->show();
  }

  // Hijack the line scroller's controls, so we can scroll nicely for word-wrap
  connect(m_lineScroll, SIGNAL(prevPage()), SLOT(scrollPrevPage()));
  connect(m_lineScroll, SIGNAL(nextPage()), SLOT(scrollNextPage()));

  connect(m_lineScroll, SIGNAL(prevLine()), SLOT(scrollPrevLine()));
  connect(m_lineScroll, SIGNAL(nextLine()), SLOT(scrollNextLine()));

  connect(m_lineScroll, SIGNAL(sliderMoved(int)), SLOT(scrollLines(int)));
  connect(m_lineScroll, SIGNAL(sliderMMBMoved(int)), SLOT(scrollLines(int)));

  // catch wheel events, completing the hijack
  m_lineScroll->installEventFilter(this);

  //
  // scrollbar for columns
  //
  m_columnScroll = new QScrollBar(Qt::Horizontal,m_view);
  m_columnScroll->hide();
  m_columnScroll->setTracking(true);
  m_startX = 0;
  m_oldStartX = 0;

  connect( m_columnScroll, SIGNAL( valueChanged (int) ),
           this, SLOT( scrollColumns (int) ) );

  //
  // iconborder ;)
  //
  m_leftBorder = new KateIconBorder( this, m_view );
  m_leftBorder->show ();

  connect( m_leftBorder, SIGNAL(toggleRegionVisibility(unsigned int)),
           m_doc->foldingTree(), SLOT(toggleRegionVisibility(unsigned int)));

  connect( doc->foldingTree(), SIGNAL(regionVisibilityChangedAt(unsigned int)),
           this, SLOT(slotRegionVisibilityChangedAt(unsigned int)));
  connect( doc, SIGNAL(codeFoldingUpdated()),
           this, SLOT(slotCodeFoldingChanged()) );

  m_displayCursor.setPosition(0, 0);
  m_cursor.setMoveOnInsert(true);
  m_cursorX = 0;

  setAcceptDrops( true );

  // event filter
  installEventFilter(this);

  // im
  setInputMethodEnabled(true);

  // set initial cursor
  setCursor( KCursor::ibeamCursor() );
  m_mouseCursor = Qt::IBeamCursor;

  // call mouseMoveEvent also if no mouse button is pressed
  setMouseTracking(true);

  m_dragInfo.state = diNone;

  // timers
  connect( &m_dragScrollTimer, SIGNAL( timeout() ),
             this, SLOT( doDragScroll() ) );

  connect( &m_scrollTimer, SIGNAL( timeout() ),
             this, SLOT( scrollTimeout() ) );

  connect( &m_cursorTimer, SIGNAL( timeout() ),
             this, SLOT( cursorTimeout() ) );

  connect( &m_textHintTimer, SIGNAL( timeout() ),
             this, SLOT( textHintTimeout() ) );

  // selection changed to set anchor
  connect( m_view, SIGNAL( selectionChanged(KTextEditor::View*) ),
             this, SLOT( viewSelectionChanged() ) );

  // update is called in KateView, after construction and layout is over
  // but before any other kateviewinternal call
}

KateViewInternal::~KateViewInternal ()
{
}

void KateViewInternal::prepareForDynWrapChange()
{
  // Which is the current view line?
  m_wrapChangeViewLine = cache()->displayViewLine(m_displayCursor, true);
}

void KateViewInternal::dynWrapChanged()
{
  if (m_view->dynWordWrap())
  {
    m_dummy->hide();
    m_columnScroll->hide();
    m_columnScrollDisplayed = false;

  }
  else
  {
    // bottom corner box
    m_dummy->show();
  }

  tagAll();
  updateView();

  if (m_view->dynWordWrap())
    scrollColumns(0);

  // Determine where the cursor should be to get the cursor on the same view line
  if (m_wrapChangeViewLine != -1) {
    KTextEditor::Cursor newStart = viewLineOffset(m_displayCursor, -m_wrapChangeViewLine);

    // Account for the scrollbar in non-dyn-word-wrap mode
    if (!m_view->dynWordWrap() && scrollbarVisible(newStart.line())) {
      int lines = linesDisplayed() - 1;

      if (m_view->height() != height())
        lines++;

      if (newStart.line() + lines == m_displayCursor.line())
        newStart = viewLineOffset(m_displayCursor, 1 - m_wrapChangeViewLine);
    }

    makeVisible(newStart, newStart.column(), true);

  } else {
    update();
  }
}

KTextEditor::Cursor KateViewInternal::endPos() const
{
  // Hrm, no lines laid out at all??
  if (!cache()->viewCacheLineCount())
    return KTextEditor::Cursor();

  for (int i = cache()->viewCacheLineCount() - 1; i >= 0; i--) {
    const KateTextLayout& thisLine = cache()->viewLine(i);

    if (thisLine.line() == -1) continue;

    if (thisLine.virtualLine() >= (int)m_doc->numVisLines()) {
      // Cache is too out of date
      return KTextEditor::Cursor(m_doc->numVisLines() - 1, m_doc->lineLength(m_doc->getRealLine(m_doc->numVisLines() - 1)));
    }

    return KTextEditor::Cursor(thisLine.virtualLine(), thisLine.wrap() ? thisLine.endCol() - 1 : thisLine.endCol());
  }

  Q_ASSERT(false);
  kdDebug(13030) << "WARNING: could not find a lineRange at all" << endl;
  return KTextEditor::Cursor(-1, -1);
}

uint KateViewInternal::endLine() const
{
  return endPos().line();
}

KateTextLayout KateViewInternal::yToKateTextLayout(int y) const
{
  if (y < 0 || y > size().height())
    return KateTextLayout::invalid();

  int range = y / renderer()->fontHeight();

  // lineRanges is always bigger than 0, after the initial updateView call
  if (range >= 0 && range <= cache()->viewCacheLineCount())
    return cache()->viewLine(range);

  return KateTextLayout::invalid();
}

int KateViewInternal::lineToY(uint viewLine) const
{
  return (viewLine-startLine()) * renderer()->fontHeight();
}

void KateViewInternal::slotIncFontSizes()
{
  renderer()->increaseFontSizes();
}

void KateViewInternal::slotDecFontSizes()
{
  renderer()->decreaseFontSizes();
}

/**
 * Line is the real line number to scroll to.
 */
void KateViewInternal::scrollLines ( int line )
{
  KTextEditor::Cursor newPos(line, 0);
  scrollPos(newPos);
}

// This can scroll less than one true line
void KateViewInternal::scrollViewLines(int offset)
{
  KTextEditor::Cursor c = viewLineOffset(startPos(), offset);
  scrollPos(c);

  m_lineScroll->blockSignals(true);
  m_lineScroll->setValue(startLine());
  m_lineScroll->blockSignals(false);
}

void KateViewInternal::scrollNextPage()
{
  scrollViewLines(QMAX( (int)linesDisplayed() - 1, 0 ));
}

void KateViewInternal::scrollPrevPage()
{
  scrollViewLines(-QMAX( (int)linesDisplayed() - 1, 0 ));
}

void KateViewInternal::scrollPrevLine()
{
  scrollViewLines(-1);
}

void KateViewInternal::scrollNextLine()
{
  scrollViewLines(1);
}

KTextEditor::Cursor KateViewInternal::maxStartPos(bool changed)
{
  m_usePlainLines = true;

  if (m_cachedMaxStartPos.line() == -1 || changed)
  {
    KTextEditor::Cursor end(m_doc->numVisLines() - 1, m_doc->lineLength(m_doc->getRealLine(m_doc->numVisLines() - 1)));

    m_cachedMaxStartPos = viewLineOffset(end, -((int)linesDisplayed() - 1));
  }

  // If we're not dynamic word-wrapping, the horizontal scrollbar is hidden and will appear, increment the maxStart by 1
  if (!m_view->dynWordWrap() && m_columnScroll->isHidden() && scrollbarVisible(m_cachedMaxStartPos.line()))
  {
    KTextEditor::Cursor end(m_doc->numVisLines() - 1, m_doc->lineLength(m_doc->getRealLine(m_doc->numVisLines() - 1)));

    return viewLineOffset(end, -(int)linesDisplayed());
  }

  m_usePlainLines = false;

  return m_cachedMaxStartPos;
}

// c is a virtual cursor
void KateViewInternal::scrollPos(KTextEditor::Cursor& c, bool force, bool calledExternally)
{
  if (!force && ((!m_view->dynWordWrap() && c.line() == (int)startLine()) || c == startPos()))
    return;

  if (c.line() < 0)
    c.setLine(0);

  KTextEditor::Cursor limit = maxStartPos();
  if (c > limit) {
    c = limit;

    // overloading this variable, it's not used in non-word wrap
    // used to set the lineScroll to the max value
    if (m_view->dynWordWrap())
      m_suppressColumnScrollBar = true;

    // Re-check we're not just scrolling to the same place
    if (!force && ((!m_view->dynWordWrap() && c.line() == (int)startLine()) || c == startPos()))
      return;
  }

  int viewLinesScrolled = 0;

  // only calculate if this is really used and usefull, could be wrong here, please recheck
  // for larger scrolls this makes 2-4 seconds difference on my xeon with dyn. word wrap on
  // try to get it really working ;)
  bool viewLinesScrolledUsable = !force
                                 && (c.line() >= (int)startLine()-(int)linesDisplayed()-1)
                                 && (c.line() <= (int)endLine()+(int)linesDisplayed()+1);

  if (viewLinesScrolledUsable)
    viewLinesScrolled = cache()->displayViewLine(c);

  m_startPos.setPosition(c);

  // set false here but reversed if we return to makeVisible
  m_madeVisible = false;

  if (false && viewLinesScrolledUsable)
  {
    int lines = linesDisplayed();
    if ((int)m_doc->numVisLines() < lines) {
      KTextEditor::Cursor end(m_doc->numVisLines() - 1, m_doc->lineLength(m_doc->getRealLine(m_doc->numVisLines() - 1)));
      lines = QMIN((int)linesDisplayed(), cache()->displayViewLine(end) + 1);
    }

    Q_ASSERT(lines >= 0);

    if (!calledExternally && QABS(viewLinesScrolled) < lines)
    {
      updateView(false, viewLinesScrolled);

      int scrollHeight = -(viewLinesScrolled * (int)renderer()->fontHeight());
      int scrollbarWidth = m_lineScroll->width();

      //
      // updates are for working around the scrollbar leaving blocks in the view
      //
      scroll(0, scrollHeight);
      update(0, height()+scrollHeight-scrollbarWidth, width(), 2*scrollbarWidth);

      m_leftBorder->scroll(0, scrollHeight);
      m_leftBorder->update(0, m_leftBorder->height()+scrollHeight-scrollbarWidth, m_leftBorder->width(), 2*scrollbarWidth);

      return;
    }
  }

  updateView();
  update();
  m_leftBorder->update();
}

void KateViewInternal::scrollColumns ( int x )
{
  if (x == m_startX)
    return;

  if (x < 0)
    x = 0;

  int dx = m_startX - x;
  m_oldStartX = m_startX;
  m_startX = x;

  if (QABS(dx) < width())
    scroll(dx, 0);
  else
    update();

  m_columnScroll->blockSignals(true);
  m_columnScroll->setValue(m_startX);
  m_columnScroll->blockSignals(false);
}

// If changed is true, the lines that have been set dirty have been updated.
void KateViewInternal::updateView(bool changed, int viewLinesScrolled)
{
  m_updatingView = true;

  if (changed)
    cache()->clear();

  m_lineScroll->blockSignals(true);

  if (width() != cache()->viewWidth())
    cache()->setViewWidth(width());

  int newSize = (height() / renderer()->fontHeight()) + 1;
  cache()->updateViewCache(startPos(), newSize, viewLinesScrolled);

  KTextEditor::Cursor maxStart = maxStartPos(changed);
  int maxLineScrollRange = maxStart.line();
  if (m_view->dynWordWrap() && maxStart.column() != 0)
    maxLineScrollRange++;
  m_lineScroll->setRange(0, maxLineScrollRange);

  if (m_view->dynWordWrap() && m_suppressColumnScrollBar) {
    m_suppressColumnScrollBar = false;
    m_lineScroll->setValue(maxStart.line());
  } else {
    m_lineScroll->setValue(startPos().line());
  }
  m_lineScroll->setSteps(1, height() / renderer()->fontHeight());
  m_lineScroll->blockSignals(false);

  /*
    if (scrollbarVisible(startLine()))
    {
      m_columnScroll->blockSignals(true);

      int max = maxLen(startLine()) - width();
      if (max < 0)
        max = 0;

      m_columnScroll->setRange(0, max);

      m_columnScroll->setValue(m_startX);

      // Approximate linescroll
      m_columnScroll->setSteps(renderer()->config()->fontMetrics()->width('a'), width());

      m_columnScroll->blockSignals(false);

      if (!m_columnScroll->isVisible ()  && !m_suppressColumnScrollBar)
      {
        m_columnScroll->show();
        m_columnScrollDisplayed = true;
      }
    }
    else if (!m_suppressColumnScrollBar && (startX() == 0))
    {
      m_columnScroll->hide();
      m_columnScrollDisplayed = false;
    }
  }*/

  m_updatingView = false;

  if (changed)
    update(); //paintText(0, 0, width(), height(), true);
}

/**
 * this function ensures a certain location is visible on the screen.
 * if endCol is -1, ignore making the columns visible.
 */
void KateViewInternal::makeVisible (const KTextEditor::Cursor& c, uint endCol, bool force, bool center, bool calledExternally)
{
  //kdDebug(13030) << "MakeVisible start [" << startPos().line << "," << startPos().col << "] end [" << endPos().line << "," << endPos().col << "] -> request: [" << c.line << "," << c.col << "]" <<endl;// , new start [" << scroll.line << "," << scroll.col << "] lines " << (linesDisplayed() - 1) << " height " << height() << endl;
    // if the line is in a folded region, unfold all the way up
    //if ( m_doc->foldingTree()->findNodeForLine( c.line )->visible )
    //  kdDebug(13030)<<"line ("<<c.line<<") should be visible"<<endl;

  if ( force )
  {
    KTextEditor::Cursor scroll = c;
    scrollPos(scroll, force, calledExternally);
  }
  else if (center && (c < startPos() || c > endPos()))
  {
    KTextEditor::Cursor scroll = viewLineOffset(c, -int(linesDisplayed()) / 2);
    scrollPos(scroll, false, calledExternally);
  }
  else if ( c > viewLineOffset(endPos(), -m_minLinesVisible) )
  {
    KTextEditor::Cursor scroll = viewLineOffset(c, -((int)linesDisplayed() - m_minLinesVisible - 1));

    if (!m_view->dynWordWrap() && m_columnScroll->isHidden())
      if (scrollbarVisible(scroll.line()))
        scroll.setLine(scroll.line() + 1);

    scrollPos(scroll, false, calledExternally);
  }
  else if ( c < viewLineOffset(startPos(), m_minLinesVisible) )
  {
    KTextEditor::Cursor scroll = viewLineOffset(c, -m_minLinesVisible);
    scrollPos(scroll, false, calledExternally);
  }
  else
  {
    // Check to see that we're not showing blank lines
    KTextEditor::Cursor max = maxStartPos();
    if (startPos() > max) {
      scrollPos(max, max.column(), calledExternally);
    }
  }

  if (!m_view->dynWordWrap() && endCol != (uint)-1)
  {
    int sX = renderer()->cursorToX(cache()->textLayout(c), c, !view()->wrapCursor());

    int sXborder = sX-8;
    if (sXborder < 0)
      sXborder = 0;

    if (sX < m_startX)
      scrollColumns (sXborder);
    else if  (sX > m_startX + width())
      scrollColumns (sX - width() + 8);
  }

  m_madeVisible = !force;
}

void KateViewInternal::slotRegionVisibilityChangedAt(unsigned int)
{
  kdDebug(13030) << "slotRegionVisibilityChangedAt()" << endl;
  m_cachedMaxStartPos.setLine(-1);
  KTextEditor::Cursor max = maxStartPos();
  if (startPos() > max)
    scrollPos(max);

  updateView();
  update();
  m_leftBorder->update();
}

void KateViewInternal::slotCodeFoldingChanged()
{
  m_leftBorder->update();
}

void KateViewInternal::slotRegionBeginEndAddedRemoved(unsigned int)
{
  kdDebug(13030) << "slotRegionBeginEndAddedRemoved()" << endl;
  // FIXME: performance problem
  m_leftBorder->update();
}

void KateViewInternal::showEvent ( QShowEvent *e )
{
  updateView ();

  QWidget::showEvent (e);
}

uint KateViewInternal::linesDisplayed() const
{
  int h = height();
  int fh = renderer()->fontHeight();

  return (h - (h % fh)) / fh;
}

QPoint KateViewInternal::cursorCoordinates() const
{
  int viewLine = cache()->displayViewLine(m_displayCursor, true);

  if (viewLine < 0 || viewLine >= cache()->viewCacheLineCount())
    return QPoint(-1, -1);

  uint y = viewLine * renderer()->fontHeight();
  uint x = m_cursorX - m_startX - cache()->viewLine(viewLine).startX() + m_leftBorder->width() + cache()->viewLine(viewLine).xOffset();

  return QPoint(x, y);
}

QVariant KateViewInternal::inputMethodQuery ( Qt::InputMethodQuery query ) const
{
  /*switch (query) {
    case Qt::ImMicroFocus: {
      int line = cache()->displayViewLine(m_displayCursor, true);
      if (line < 0 || line >= cache()->viewLineCount)
          return QWidget::inputMethodQuery(query);

      KateRenderer *renderer = renderer();

      // Cursor placement code is changed for Asian input method that
      // shows candidate window. This behavior is same as Qt/E 2.3.7
      // which supports Asian input methods. Asian input methods need
      // start point of IM selection text to place candidate window as
      // adjacent to the selection text.
      uint preeditStrLen = renderer->textWidth(textLine(m_imPreeditStartLine), cursor.column()) - renderer->textWidth(textLine(m_imPreeditStartLine), m_imPreeditSelStart);
      uint x = m_cursorX - m_startX - lineRanges[line].startX() + lineRanges[line].xOffset() - preeditStrLen;
      uint y = line * renderer->fontHeight();

      return QRect(x, y, 0, renderer->fontHeight());
    }
    default:*/
      return QWidget::inputMethodQuery(query);
  //}
}

void KateViewInternal::doReturn()
{
  KTextEditor::Cursor c = m_cursor;
  m_doc->newLine( c, this );
  // TODO Might not be needed
  updateCursor( c );
  updateView();
}

void KateViewInternal::doDelete()
{
  m_doc->del( m_view, m_cursor );
  if (m_view->m_codeCompletion->codeCompletionVisible()) {
    m_view->m_codeCompletion->updateBox();
  }
}

void KateViewInternal::doBackspace()
{
  m_doc->backspace( m_view, m_cursor );
  if (m_view->m_codeCompletion->codeCompletionVisible()) {
    m_view->m_codeCompletion->updateBox();
  }
}

void KateViewInternal::doTranspose()
{
  m_doc->transpose( m_cursor );
}

void KateViewInternal::doDeleteWordLeft()
{
  wordLeft( true );
  m_view->removeSelectedText();
  update();
}

void KateViewInternal::doDeleteWordRight()
{
  wordRight( true );
  m_view->removeSelectedText();
  update();
}

class CalculatingCursor : public KTextEditor::Cursor {
public:
  CalculatingCursor(KateViewInternal* vi)
    : KTextEditor::Cursor()
    , m_vi(vi)
  {
    Q_ASSERT(valid());
  }

  CalculatingCursor(KateViewInternal* vi, const KTextEditor::Cursor& c)
    : KTextEditor::Cursor(c)
    , m_vi(vi)
  {
    Q_ASSERT(valid());
  }

  // This one constrains its arguments to valid positions
  CalculatingCursor(KateViewInternal* vi, uint line, uint col)
    : KTextEditor::Cursor(line, col)
    , m_vi(vi)
  {
    makeValid();
  }


  virtual CalculatingCursor& operator+=( int n ) = 0;

  virtual CalculatingCursor& operator-=( int n ) = 0;

  CalculatingCursor& operator++() { return operator+=( 1 ); }

  CalculatingCursor& operator--() { return operator-=( 1 ); }

  void makeValid() {
    setLine(QMAX( 0, QMIN( int( m_vi->m_doc->lines() - 1 ), line() ) ));
    if (m_vi->m_view->wrapCursor())
      m_column = QMAX( 0, QMIN( m_vi->m_doc->lineLength( line() ), column() ) );
    else
      m_column = QMAX( 0, column() );
    Q_ASSERT( valid() );
  }

  void toEdge( KateViewInternal::Bias bias ) {
    if( bias == KateViewInternal::left ) m_column = 0;
    else if( bias == KateViewInternal::right ) m_column = m_vi->m_doc->lineLength( line() );
  }

  bool atEdge() const { return atEdge( KateViewInternal::left ) || atEdge( KateViewInternal::right ); }

  bool atEdge( KateViewInternal::Bias bias ) const {
    switch( bias ) {
    case KateViewInternal::left:  return column() == 0;
    case KateViewInternal::none:  return atEdge();
    case KateViewInternal::right: return column() == m_vi->m_doc->lineLength( line() );
    default: Q_ASSERT(false); return false;
    }
  }

protected:
  bool valid() const {
    return line() >= 0 &&
            line() < m_vi->m_doc->lines() &&
            column() >= 0 &&
            (!m_vi->m_view->wrapCursor() || column() <= m_vi->m_doc->lineLength( line() ));
  }
  KateViewInternal* m_vi;
};

class BoundedCursor : public CalculatingCursor {
public:
  BoundedCursor(KateViewInternal* vi)
    : CalculatingCursor( vi ) {};
  BoundedCursor(KateViewInternal* vi, const KTextEditor::Cursor& c )
    : CalculatingCursor( vi, c ) {};
  BoundedCursor(KateViewInternal* vi, uint line, uint col )
    : CalculatingCursor( vi, line, col ) {};
  virtual CalculatingCursor& operator+=( int n ) {
    KateLineLayoutPtr thisLine = m_vi->cache()->line(line());
    if (!thisLine->isValid()) {
      kdWarning() << "Did not retrieve valid layout for line " << line() << endl;
      return *this;
    }

    if (n >= 0) {
      for (int i = 0; i < n; i++) {
        if (m_column == thisLine->length())
          break;

        m_column = thisLine->layout()->nextCursorPosition(m_column);
      }
    } else {
      for (int i = 0; i > n; i--) {
        if (m_column == 0)
          break;

        m_column = thisLine->layout()->previousCursorPosition(m_column);
      }
    }

    Q_ASSERT( valid() );
    return *this;
  }
  virtual CalculatingCursor& operator-=( int n ) {
    return operator+=( -n );
  }
};

class WrappingCursor : public CalculatingCursor {
public:
  WrappingCursor(KateViewInternal* vi)
    : CalculatingCursor( vi) {};
  WrappingCursor(KateViewInternal* vi, const KTextEditor::Cursor& c )
    : CalculatingCursor( vi, c ) {};
  WrappingCursor(KateViewInternal* vi, uint line, uint col )
    : CalculatingCursor( vi, line, col ) {};

  virtual CalculatingCursor& operator+=( int n ) {
    KateLineLayoutPtr thisLine = m_vi->cache()->line(line());
    if (!thisLine->isValid()) {
      kdWarning() << "Did not retrieve a valid layout for line " << line() << endl;
      return *this;
    }

    if (n >= 0) {
      for (int i = 0; i < n; i++) {
        if (m_column == thisLine->length()) {
          // Have come to the end of a line
          if (line() >= m_vi->m_doc->lines() - 1)
            // Have come to the end of the document
            break;

          // Advance to the beginning of the next line
          m_column = 0;
          setLine(line() + 1);

          // Retrieve the next text range
          thisLine = m_vi->cache()->line(line());
          if (!thisLine->isValid()) {
            kdWarning() << "Did not retrieve a valid layout for line " << line() << endl;
            return *this;
          }

          continue;
        }

        m_column = thisLine->layout()->nextCursorPosition(m_column);
      }

    } else {
      for (int i = 0; i > n; i--) {
        if (m_column == 0) {
          // Have come to the start of the document
          if (line() == 0)
            break;

          // Start going back to the end of the last line
          setLine(line() - 1);

          // Retrieve the next text range
          thisLine = m_vi->cache()->line(line());
          if (!thisLine->isValid()) {
            kdWarning() << "Did not retrieve a valid layout for line " << line() << endl;
            return *this;
          }

          // Finish going back to the end of the last line
          m_column = thisLine->length();

          continue;
        }

        m_column = thisLine->layout()->previousCursorPosition(m_column);
      }
    }

    Q_ASSERT(valid());
    return *this;
  }
  virtual CalculatingCursor& operator-=( int n ) {
    return operator+=( -n );
  }
};

void KateViewInternal::moveChar( KateViewInternal::Bias bias, bool sel )
{
  KTextEditor::Cursor c;
  if ( m_view->wrapCursor() ) {
    c = WrappingCursor( this, m_cursor ) += bias;
  } else {
    c = BoundedCursor( this, m_cursor ) += bias;
  }

  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::cursorLeft(  bool sel )
{
  if ( ! m_view->wrapCursor() && m_cursor.column() == 0 )
    return;

  moveChar( KateViewInternal::left, sel );
  if (m_view->m_codeCompletion->codeCompletionVisible()) {
    m_view->m_codeCompletion->updateBox();
  }
}

void KateViewInternal::cursorRight( bool sel )
{
  moveChar( KateViewInternal::right, sel );
  if (m_view->m_codeCompletion->codeCompletionVisible()) {
    m_view->m_codeCompletion->updateBox();
  }
}

void KateViewInternal::moveWord( KateViewInternal::Bias bias, bool sel )
{
  // This matches the word-moving in QTextEdit, QLineEdit etc.

  WrappingCursor c( this, m_cursor );
  if( !c.atEdge( bias ) ) {
    KateHighlighting* h = m_doc->highlight();

    bool moved = false;
    while( !c.atEdge( bias ) && !h->isInWord( m_doc->line( c.line() )[ c.column() - (bias == left ? 1 : 0) ] ) )
    {
      c += bias;
      moved = true;
    }

    if ( bias != right || !moved )
    {
      while( !c.atEdge( bias ) &&  h->isInWord( m_doc->line( c.line() )[ c.column() - (bias == left ? 1 : 0) ] ) )
        c += bias;
      if ( bias == right )
      {
        while ( !c.atEdge( bias ) && m_doc->line( c.line() )[ c.column() ].isSpace() )
          c+= bias;
      }
    }

  } else {
    c += bias;
  }

  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::wordLeft ( bool sel ) { moveWord( left,  sel ); }
void KateViewInternal::wordRight( bool sel ) { moveWord( right, sel ); }

void KateViewInternal::moveEdge( KateViewInternal::Bias bias, bool sel )
{
  BoundedCursor c( this, m_cursor );
  c.toEdge( bias );
  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::home( bool sel )
{
  if (m_view->m_codeCompletion->codeCompletionVisible()) {
    QKeyEvent e(QEvent::KeyPress, Qt::Key_Home, 0, 0);
    m_view->m_codeCompletion->handleKey(&e);
    return;
  }

  if (m_view->dynWordWrap() && currentLayout().startCol()) {
    // Allow us to go to the real start if we're already at the start of the view line
    if (m_cursor.column() != currentLayout().startCol()) {
      KTextEditor::Cursor c = currentLayout().start();
      updateSelection( c, sel );
      updateCursor( c );
      return;
    }
  }

  if( !(m_doc->config()->configFlags() & KateDocumentConfig::cfSmartHome) ) {
    moveEdge( left, sel );
    return;
  }

  KTextEditor::Cursor c = m_cursor;
  int lc = textLine( c.line() )->firstChar();

  if( lc < 0 || c.column() == lc ) {
    c.setColumn(0);
  } else {
    c.setColumn(lc);
  }

  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::end( bool sel )
{
  if (m_view->m_codeCompletion->codeCompletionVisible()) {
    QKeyEvent e(QEvent::KeyPress, Qt::Key_End, 0, 0);
    m_view->m_codeCompletion->handleKey(&e);
    return;
  }


  if (m_view->dynWordWrap() && currentLayout().wrap()) {
    // Allow us to go to the real end if we're already at the end of the view line
    if (m_cursor.column() < currentLayout().endCol() - 1) {
      KTextEditor::Cursor c(m_cursor.line(), currentLayout().endCol() - 1);
      updateSelection( c, sel );
      updateCursor( c );
      return;
    }
  }

  moveEdge( right, sel );
}

KateTextLayout KateViewInternal::currentLayout() const
{
  return cache()->textLayout(m_cursor);
}

KateTextLayout KateViewInternal::previousLayout() const
{
  int currentViewLine = cache()->viewLine(m_cursor);

  if (currentViewLine)
    return cache()->textLayout(m_cursor.line(), currentViewLine - 1);
  else
    return cache()->textLayout(m_doc->getRealLine(m_displayCursor.line() - 1), -1);
}

KateTextLayout KateViewInternal::nextLayout() const
{
  int currentViewLine = cache()->viewLine(m_cursor) + 1;

  if (currentViewLine >= cache()->line(m_cursor.line())->viewLineCount()) {
    currentViewLine = 0;
    return cache()->textLayout(m_cursor.line() + 1, currentViewLine);
  } else {
    return cache()->textLayout(m_cursor.line(), currentViewLine);
  }
}

/*
 * This returns the cursor which is offset by (offset) view lines.
 * This is the main function which is called by code not specifically dealing with word-wrap.
 * The opposite conversion (cursor to offset) can be done with cache()->displayViewLine().
 *
 * The cursors involved are virtual cursors (ie. equivalent to m_displayCursor)
 */
KTextEditor::Cursor KateViewInternal::viewLineOffset(const KTextEditor::Cursor& virtualCursor, int offset, bool keepX)
{
  if (!m_view->dynWordWrap()) {
    KTextEditor::Cursor ret(QMIN((int)m_doc->visibleLines() - 1, virtualCursor.line() + offset), 0);

    if (ret.line() < 0)
      ret.setLine(0);

    if (keepX) {
      int realLine = m_doc->getRealLine(ret.line());
      KateTextLayout t = cache()->textLayout(realLine, 0);
      Q_ASSERT(t.isValid());

      if (m_currentMaxX > m_cursorX)
        m_cursorX = m_currentMaxX;

      ret.setColumn(renderer()->xToCursor(t, m_cursorX, !m_view->wrapCursor()).column());
    }

    return ret;
  }

  KTextEditor::Cursor realCursor = virtualCursor;
  realCursor.setLine(m_doc->getRealLine(virtualCursor.line()));

  uint cursorViewLine = cache()->viewLine(realCursor);

  int currentOffset = 0;
  int virtualLine = 0;

  bool forwards = (offset > 0) ? true : false;

  if (forwards) {
    currentOffset = cache()->lastViewLine(realCursor.line()) - cursorViewLine;
    if (offset <= currentOffset) {
      // the answer is on the same line
      KateTextLayout thisLine = cache()->textLayout(realCursor.line(), cursorViewLine + offset);
      Q_ASSERT(thisLine.virtualLine() == virtualCursor.line());
      return KTextEditor::Cursor(virtualCursor.line(), thisLine.startCol());
    }

    virtualLine = virtualCursor.line() + 1;

  } else {
    offset = -offset;
    currentOffset = cursorViewLine;
    if (offset <= currentOffset) {
      // the answer is on the same line
      KateTextLayout thisLine = cache()->textLayout(realCursor.line(), cursorViewLine - offset);
      Q_ASSERT(thisLine.virtualLine() == virtualCursor.line());
      return KTextEditor::Cursor(virtualCursor.line(), thisLine.startCol());
    }

    virtualLine = virtualCursor.line() - 1;
  }

  currentOffset++;

  while (virtualLine >= 0 && virtualLine < (int)m_doc->visibleLines())
  {
    int realLine = m_doc->getRealLine(virtualLine);
    KateLineLayoutPtr thisLine = cache()->line(realLine, virtualLine);
    if (!thisLine)
      break;

    for (int i = 0; i < QMAX(1, thisLine->viewLineCount() - 1); ++i) {
      if (offset == currentOffset) {
        KateTextLayout thisViewLine = cache()->viewLine(i);

        if (!forwards) {
          // We actually want it the other way around
          int requiredViewLine = cache()->lastViewLine(realLine) - thisViewLine.viewLine();
          if (requiredViewLine != thisViewLine.viewLine()) {
            thisViewLine = thisLine->viewLine(requiredViewLine);
          }
        }

        KTextEditor::Cursor ret(virtualLine, thisViewLine.startCol());

        // keep column position
        if (keepX) {
          ret.setColumn(thisViewLine.endCol() - 1);
          KTextEditor::Cursor realCursor = toRealCursor(virtualCursor);
          KateTextLayout t = cache()->textLayout(realCursor);
          m_cursorX = renderer()->cursorToX(t, realCursor);

          if (m_currentMaxX > m_cursorX) {
            m_cursorX = m_currentMaxX;

            realCursor = renderer()->xToCursor(t, m_cursorX, !m_view->wrapCursor());
            ret.setColumn(realCursor.column());
          }
        }

        return ret;
      }

      currentOffset++;
    }

    if (forwards)
      virtualLine++;
    else
      virtualLine--;
  }

  // Looks like we were asked for something a bit exotic.
  // Return the max/min valid position.
  if (forwards)
    return KTextEditor::Cursor(m_doc->visibleLines() - 1, m_doc->lineLength(m_doc->visibleLines() - 1));
  else
    return KTextEditor::Cursor(0, 0);
}

int KateViewInternal::lineMaxCursorX(const KateTextLayout& range)
{
  if (!m_view->wrapCursor() && !range.wrap())
    return INT_MAX;

  int maxX = range.endX();

  if (maxX && range.wrap()) {
    QChar lastCharInLine = textLine(range.line())->getChar(range.endCol() - 1);
    maxX -= renderer()->config()->fontMetrics()->width(lastCharInLine);
  }

  return maxX;
}

int KateViewInternal::lineMaxCol(const KateTextLayout& range)
{
  int maxCol = range.endCol();

  if (maxCol && range.wrap())
    maxCol--;

  return maxCol;
}

void KateViewInternal::cursorUp(bool sel)
{
  if (m_view->m_codeCompletion->codeCompletionVisible()) {
    QKeyEvent e(QEvent::KeyPress, Qt::Key_Up, 0, 0);
    m_view->m_codeCompletion->handleKey(&e);
    return;
  }

  if (m_displayCursor.line() == 0 && (!m_view->dynWordWrap() || cache()->viewLine(m_cursor) == 0))
    return;

  m_preserveMaxX = true;

  KateTextLayout thisLine = currentLayout();
  // This is not the first line because that is already simplified out above
  KateTextLayout pRange = previousLayout();

  // Ensure we're in the right spot
  Q_ASSERT((m_cursor.line() == thisLine.line()) &&
      (m_cursor.column() >= thisLine.startCol()) &&
      (!thisLine.wrap() || m_cursor.column() < thisLine.endCol()));

  // Retrieve current cursor x position
  m_cursorX = renderer()->cursorToX(thisLine, m_cursor);

  if (m_currentMaxX > m_cursorX)
    m_cursorX = m_currentMaxX;

  KTextEditor::Cursor c = renderer()->xToCursor(pRange, m_cursorX, !m_view->wrapCursor());

  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::cursorDown(bool sel)
{
  if (m_view->m_codeCompletion->codeCompletionVisible()) {
    QKeyEvent e(QEvent::KeyPress, Qt::Key_Down, 0, 0);
    m_view->m_codeCompletion->handleKey(&e);
    return;
  }

  if ((m_displayCursor.line() >= (int)m_doc->numVisLines() - 1) && (!m_view->dynWordWrap() || cache()->viewLine(m_cursor) == cache()->lastViewLine(m_cursor.line())))
    return;

  m_preserveMaxX = true;

  KateTextLayout thisLine = currentLayout();
  // This is not the last line because that is already simplified out above
  KateTextLayout nRange = nextLayout();

  // Ensure we're in the right spot
  Q_ASSERT((m_cursor.line() == thisLine.line()) &&
      (m_cursor.column() >= thisLine.startCol()) &&
      (!thisLine.wrap() || m_cursor.column() < thisLine.endCol()));

  // Retrieve current cursor x position
  m_cursorX = renderer()->cursorToX(thisLine, m_cursor);

  if (m_currentMaxX > m_cursorX)
    m_cursorX = m_currentMaxX;

  KTextEditor::Cursor c = renderer()->xToCursor(nRange, m_cursorX, !m_view->wrapCursor());

  updateSelection(c, sel);
  updateCursor(c);
}

void KateViewInternal::cursorToMatchingBracket( bool sel )
{
  KTextEditor::Range range(m_cursor, m_cursor);

  if( !m_doc->findMatchingBracket( range ) )
    return;

  // The cursor is now placed just to the left of the matching bracket.
  // If it's an ending bracket, put it to the right (so we can easily
  // get back to the original bracket).
  if( range.end() > range.start() )
    range.setEndColumn(range.end().column() + 1);

  updateSelection( range.end(), sel );
  updateCursor( range.end() );
}

void KateViewInternal::topOfView( bool sel )
{
  KTextEditor::Cursor c = viewLineOffset(startPos(), m_minLinesVisible);
  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::bottomOfView( bool sel )
{
  // FIXME account for wordwrap
  KTextEditor::Cursor c = viewLineOffset(endPos(), -m_minLinesVisible);
  updateSelection( c, sel );
  updateCursor( c );
}

// lines is the offset to scroll by
void KateViewInternal::scrollLines( int lines, bool sel )
{
  KTextEditor::Cursor c = viewLineOffset(m_displayCursor, lines, true);

  // Fix the virtual cursor -> real cursor
  c.setLine(m_doc->getRealLine(c.line()));

  updateSelection( c, sel );
  updateCursor( c );
}

// This is a bit misleading... it's asking for the view to be scrolled, not the cursor
void KateViewInternal::scrollUp()
{
  KTextEditor::Cursor newPos = viewLineOffset(startPos(), -1);
  scrollPos(newPos);
}

void KateViewInternal::scrollDown()
{
  KTextEditor::Cursor newPos = viewLineOffset(startPos(), 1);
  scrollPos(newPos);
}

void KateViewInternal::setAutoCenterLines(int viewLines, bool updateView)
{
  m_autoCenterLines = viewLines;
  m_minLinesVisible = QMIN(int((linesDisplayed() - 1)/2), m_autoCenterLines);
  if (updateView)
    KateViewInternal::updateView();
}

void KateViewInternal::pageUp( bool sel )
{
  if (m_view->m_codeCompletion->codeCompletionVisible()) {
    QKeyEvent e(QEvent::KeyPress, Qt::Key_PageUp, 0, 0);
    m_view->m_codeCompletion->handleKey(&e);
    return;
  }

  // remember the view line and x pos
  int viewLine = cache()->displayViewLine(m_displayCursor);
  bool atTop = startPos().atStartOfDocument();

  // Adjust for an auto-centering cursor
  int lineadj = 2 * m_minLinesVisible;
  int cursorStart = (linesDisplayed() - 1) - viewLine;
  if (cursorStart < m_minLinesVisible)
    lineadj -= m_minLinesVisible - cursorStart;

  int linesToScroll = -QMAX( ((int)linesDisplayed() - 1) - lineadj, 0 );
  m_preserveMaxX = true;

  // don't scroll the full view in case the scrollbar appears
  if (!m_view->dynWordWrap()) {
    if (scrollbarVisible(startLine() + linesToScroll + viewLine)) {
      if (!m_columnScrollDisplayed) {
        linesToScroll++;
      }
    } else {
      if (m_columnScrollDisplayed) {
        linesToScroll--;
      }
    }
  }

  if (!m_doc->pageUpDownMovesCursor () && !atTop) {
    m_cursorX = renderer()->cursorToX(currentLayout(), m_cursor);

    KTextEditor::Cursor newStartPos = viewLineOffset(startPos(), linesToScroll - 1);
    scrollPos(newStartPos);

    // put the cursor back approximately where it was
    KTextEditor::Cursor newPos = toRealCursor(viewLineOffset(newStartPos, viewLine, true));

    KateTextLayout newLine = cache()->textLayout(newPos);

    if (m_currentMaxX> m_cursorX)
      m_cursorX = m_currentMaxX;

    newPos = renderer()->xToCursor(newLine, m_cursorX, !view()->wrapCursor());

    m_preserveMaxX = true;
    updateSelection( newPos, sel );
    updateCursor(newPos);

  } else {
    scrollLines( linesToScroll, sel );
  }
}

void KateViewInternal::pageDown( bool sel )
{
  if (m_view->m_codeCompletion->codeCompletionVisible()) {
    QKeyEvent e(QEvent::KeyPress, Qt::Key_PageDown, 0, 0);
    m_view->m_codeCompletion->handleKey(&e);
    return;
  }

  // remember the view line
  int viewLine = cache()->displayViewLine(m_displayCursor);
  bool atEnd = startPos() >= m_cachedMaxStartPos;

  // Adjust for an auto-centering cursor
  int lineadj = 2 * m_minLinesVisible;
  int cursorStart = m_minLinesVisible - viewLine;
  if (cursorStart > 0)
    lineadj -= cursorStart;

  int linesToScroll = QMAX( ((int)linesDisplayed() - 1) - lineadj, 0 );
  m_preserveMaxX = true;

  // don't scroll the full view in case the scrollbar appears
  if (!m_view->dynWordWrap()) {
    if (scrollbarVisible(startLine() + linesToScroll + viewLine - (linesDisplayed() - 1))) {
      if (!m_columnScrollDisplayed) {
        linesToScroll--;
      }
    } else {
      if (m_columnScrollDisplayed) {
        linesToScroll--;
      }
    }
  }

  if (!m_doc->pageUpDownMovesCursor () && !atEnd) {
    m_cursorX = renderer()->cursorToX(currentLayout(), m_cursor);

    KTextEditor::Cursor newStartPos = viewLineOffset(startPos(), linesToScroll + 1);
    scrollPos(newStartPos);

    // put the cursor back approximately where it was
    KTextEditor::Cursor newPos = toRealCursor(viewLineOffset(newStartPos, viewLine, true));

    KateTextLayout newLine = cache()->textLayout(newPos);

    if (m_currentMaxX> m_cursorX)
      m_cursorX = m_currentMaxX;

    newPos = renderer()->xToCursor(newLine, m_cursorX, !view()->wrapCursor());

    m_preserveMaxX = true;
    updateSelection( newPos, sel );
    updateCursor(newPos);

  } else {
    scrollLines( linesToScroll, sel );
  }
}

bool KateViewInternal::scrollbarVisible(uint startLine)
{
  return maxLen(startLine) > width() - 8;
}

int KateViewInternal::maxLen(uint startLine)
{
  Q_ASSERT(!m_view->dynWordWrap());

  int displayLines = (m_view->height() / renderer()->fontHeight()) + 1;

  int maxLen = 0;

  for (int z = 0; z < displayLines; z++) {
    int virtualLine = startLine + z;

    if (virtualLine < 0 || virtualLine >= (int)m_doc->visibleLines())
      break;

    maxLen = QMAX(maxLen, cache()->line((int)m_doc->getRealLine(virtualLine))->width());
  }

  return maxLen;
}

void KateViewInternal::top( bool sel )
{
  m_cursorX = renderer()->cursorToX(currentLayout(), m_cursor);

  KTextEditor::Cursor newCursor(0, 0);

  if (m_currentMaxX > m_cursorX)
    m_cursorX = m_currentMaxX;

  newCursor = renderer()->xToCursor(cache()->textLayout(newCursor), m_cursorX, !view()->wrapCursor());

  updateSelection( newCursor, sel );
  updateCursor( newCursor );
}

void KateViewInternal::bottom( bool sel )
{
  KTextEditor::Cursor newCursor(m_doc->lastLine(), 0);

  if (m_currentMaxX > m_cursorX)
    m_cursorX = m_currentMaxX;

  newCursor = renderer()->xToCursor(cache()->textLayout(newCursor), m_cursorX, !view()->wrapCursor());

  updateSelection( newCursor, sel );
  updateCursor( newCursor );
}

void KateViewInternal::top_home( bool sel )
{
  if (m_view->m_codeCompletion->codeCompletionVisible()) {
    QKeyEvent e(QEvent::KeyPress, Qt::Key_Home, 0, 0);
    m_view->m_codeCompletion->handleKey(&e);
    return;
  }
  KTextEditor::Cursor c( 0, 0 );
  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::bottom_end( bool sel )
{
  if (m_view->m_codeCompletion->codeCompletionVisible()) {
    QKeyEvent e(QEvent::KeyPress, Qt::Key_End, 0, 0);
    m_view->m_codeCompletion->handleKey(&e);
    return;
  }
  KTextEditor::Cursor c( m_doc->lastLine(), m_doc->lineLength( m_doc->lastLine() ) );
  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::updateSelection( const KTextEditor::Cursor& _newCursor, bool keepSel )
{
  KTextEditor::Cursor newCursor = _newCursor;
  if( keepSel )
  {
    if ( !m_view->selection() || (m_selectAnchor.line() == -1)
         || (m_view->config()->persistentSelection()
             && !m_view->selectionRange().contains(m_cursor)) )
    {
      m_selectAnchor = m_cursor;
      m_view->setSelection( KTextEditor::Range(m_cursor, newCursor) );
    }
    else
    {
      bool doSelect = true;
      switch (m_selectionMode)
      {
        case Word:
        {
          bool same = ( newCursor.line() == m_selectionCached.start().line() );
          int c;
          if ( newCursor.line() > m_selectionCached.start().line() ||
               ( same && newCursor.column() > m_selectionCached.end().column() ) )
          {
            m_selectAnchor = m_selectionCached.start();

            KateTextLine::Ptr l = m_doc->kateTextLine( newCursor.line() );

            for ( c = newCursor.column(); c < l->length(); c++ )
              if ( !m_doc->highlight()->isInWord( l->getChar( c ) ) )
                break;

            newCursor.setColumn( c );
          }
          else if ( newCursor.line() < m_selectionCached.start().line() ||
               ( same && newCursor.column() < m_selectionCached.start().column() ) )
          {
            m_selectAnchor = m_selectionCached.end();

            KateTextLine::Ptr l = m_doc->kateTextLine( newCursor.line() );

            for ( c = newCursor.column(); c > 0; c-- )
              if ( !m_doc->highlight()->isInWord( l->getChar( c ) ) )
                break;

            newCursor.setColumn( c+1 );
          }
          else
            doSelect = false;

        }
        break;
        case Line:
          if ( newCursor.line() > m_selectionCached.start().line() )
          {
            m_selectAnchor = m_selectionCached.start();
            newCursor.setColumn( m_doc->line( newCursor.line() ).length() );
          }
          else if ( newCursor.line() < m_selectionCached.start().line() )
          {
            m_selectAnchor = m_selectionCached.end();
            newCursor.setColumn( 0 );
          }
          else // same line, ignore
            doSelect = false;
        break;
        default: // *allways* keep original selection for mouse
        {
          if ( m_selectionCached.start().line() < 0 ) // invalid
            break;

          if ( newCursor.line() > m_selectionCached.end().line() ||
               ( newCursor.line() == m_selectionCached.end().line() &&
                 newCursor.column() > m_selectionCached.end().column() ) )
            m_selectAnchor = m_selectionCached.start();

          else if ( newCursor.line() < m_selectionCached.start().line() ||
               ( newCursor.line() == m_selectionCached.start().line() &&
                 newCursor.column() < m_selectionCached.start().column() ) )
            m_selectAnchor = m_selectionCached.end();

          else
            doSelect = false;
        }
//         break;
      }

      if ( doSelect )
        m_view->setSelection( KTextEditor::Range(m_selectAnchor, newCursor) );
      else if ( m_selectionCached.isValid() ) // we have a cached selection, so we restore that
        m_view->setSelection( m_selectionCached );
    }

    m_selChangedByUser = true;
  }
  else if ( !m_view->config()->persistentSelection() )
  {
    m_view->clearSelection();
    m_selectionCached = KTextEditor::Range::invalid();
  }
}

void KateViewInternal::updateCursor( const KTextEditor::Cursor& newCursor, bool force, bool center, bool calledExternally )
{
  KateTextLine::Ptr l = textLine( newCursor.line() );

  if ( !force && (m_cursor == newCursor) )
  {
    if ( !m_madeVisible )
    {
      // unfold if required
      m_doc->foldingTree()->ensureVisible( newCursor.line() );

      makeVisible ( m_displayCursor, m_displayCursor.column(), false, center, calledExternally );
    }

    return;
  }

  // unfold if required
  m_doc->foldingTree()->ensureVisible( newCursor.line() );

  KTextEditor::Cursor oldDisplayCursor = m_displayCursor;

  m_cursor.setPosition (newCursor);
  m_displayCursor = toVirtualCursor(m_cursor);

  m_cursorX = renderer()->cursorToX(cache()->textLayout(m_cursor), m_cursor);
  makeVisible ( m_displayCursor, m_displayCursor.column(), false, center, calledExternally );

  updateBracketMarks();

  // It's efficient enough to just tag them both without checking to see if they're on the same view line
  tagLine(oldDisplayCursor);
  tagLine(m_displayCursor);

  updateMicroFocus();

  if (m_cursorTimer.isActive ())
  {
    if ( KApplication::cursorFlashTime() > 0 )
      m_cursorTimer.start( KApplication::cursorFlashTime() / 2 );
    renderer()->setDrawCaret(true);
  }

  // Remember the maximum X position if requested
  if (m_preserveMaxX)
    m_preserveMaxX = false;
  else
    m_currentMaxX = m_cursorX;

  //kdDebug(13030) << "m_currentMaxX: " << m_currentMaxX << " (was "<< oldmaxx << "), m_cursorX: " << m_cursorX << endl;
  //kdDebug(13030) << "Cursor now located at real " << cursor.line << "," << cursor.col << ", virtual " << m_displayCursor.line << ", " << m_displayCursor.col << "; Top is " << startLine() << ", " << startPos().col <<  endl;

  update(); //paintText(0, 0, width(), height(), true);

  emit m_view->cursorPositionChanged(m_view);
}

void KateViewInternal::updateBracketMarks()
{
  if ( m_bm.isValid() ) {
    tagRange(m_bm, true);
    tagRange(m_bmStart, true);
    tagRange(m_bmEnd, true);
  }

  //m_bmStart.setValid(false);
  //m_bmEnd.setValid(false);

  // add some limit to this, this is really endless on big files without limit
  int maxLines = linesDisplayed () * 3;
  m_doc->newBracketMark( m_cursor, m_bm, maxLines );

  if ( m_bm.isValid() ) {
    m_bmStart.setStart(m_bm.start());
    m_bmStart.setEnd(m_bm.start().line(), m_bm.start().column() + 1);
    //m_bmStart.setValid(true);

    m_bmEnd.setStart(m_bm.end());
    m_bmEnd.setEnd(m_bm.end().line(), m_bmEnd.end().column() + 1);
    //m_bmEnd.setValid(true);

    m_bm.setEndColumn(m_bm.end().column() + 1);

    tagRange(m_bm, true);
    tagRange(m_bmStart, true);
    tagRange(m_bmEnd, true);
  }
}

bool KateViewInternal::tagLine(const KTextEditor::Cursor& virtualCursor)
{
  // FIXME may be a more efficient way for this
  if ((int)m_doc->getRealLine(virtualCursor.line()) > m_doc->lastLine())
    return false;
  // End FIXME

  int viewLine = cache()->displayViewLine(virtualCursor, true);
  if (viewLine >= 0 && viewLine < cache()->viewCacheLineCount()) {
    cache()->viewLine(viewLine).setDirty();
    m_leftBorder->update (0, lineToY(viewLine), m_leftBorder->width(), renderer()->fontHeight());
    return true;
  }
  return false;
}

bool KateViewInternal::tagLines( int start, int end, bool realLines )
{
  return tagLines(KTextEditor::Cursor(start, 0), KTextEditor::Cursor(end, -1), realLines);
}

bool KateViewInternal::tagLines(KTextEditor::Cursor start, KTextEditor::Cursor end, bool realCursors)
{
  if (realCursors)
  {
    //kdDebug(13030)<<"realLines is true"<<endl;
    start = toVirtualCursor(start);
    end = toVirtualCursor(end);
  }

  if (end.line() < (int)startLine())
  {
    //kdDebug(13030)<<"end<startLine"<<endl;
    return false;
  }
  if (start.line() > (int)endLine())
  {
    //kdDebug(13030)<<"start> endLine"<<start<<" "<<((int)endLine())<<endl;
    return false;
  }

  //kdDebug(13030) << "tagLines( [" << start.line << "," << start.col << "], [" << end.line << "," << end.col << "] )\n";

  bool ret = false;

  for (int z = 0; z < cache()->viewCacheLineCount(); z++)
  {
    if ((cache()->viewLine(z).virtualLine() > start.line() || (cache()->viewLine(z).virtualLine() == start.line() && cache()->viewLine(z).endCol() >= start.column() && start.column() != -1)) && (cache()->viewLine(z).virtualLine() < end.line() || (cache()->viewLine(z).virtualLine() == end.line() && (cache()->viewLine(z).startCol() <= end.column() || end.column() == -1)))) {
      ret = true;
      cache()->viewLine(z).setDirty();
      //kdDebug(13030) << "Tagged line " << cache()->viewLine(z).line << endl;
    }
  }

  if (!m_view->dynWordWrap())
  {
    int y = lineToY( start.line() );
    // FIXME is this enough for when multiple lines are deleted
    int h = (end.line() - start.line() + 2) * renderer()->fontHeight();
    if (end.line() == (int)m_doc->numVisLines() - 1)
      h = height();

    m_leftBorder->update (0, y, m_leftBorder->width(), h);
  }
  else
  {
    // FIXME Do we get enough good info in editRemoveText to optimise this more?
    //bool justTagged = false;
    for (int z = 0; z < cache()->viewCacheLineCount(); z++)
    {
      if ((cache()->viewLine(z).virtualLine() > start.line() || (cache()->viewLine(z).virtualLine() == start.line() && cache()->viewLine(z).endCol() >= start.column() && start.column() != -1)) && (cache()->viewLine(z).virtualLine() < end.line() || (cache()->viewLine(z).virtualLine() == end.line() && (cache()->viewLine(z).startCol() <= end.column() || end.column() == -1))))
      {
        //justTagged = true;
        m_leftBorder->update (0, z * renderer()->fontHeight(), m_leftBorder->width(), m_leftBorder->height());
        break;
      }
      /*else if (justTagged)
      {
        justTagged = false;
        leftBorder->update (0, z * m_doc->viewFont.fontHeight, leftBorder->width(), m_doc->viewFont.fontHeight);
        break;
      }*/
    }
  }

  return ret;
}

bool KateViewInternal::tagRange(const KTextEditor::Range& range, bool realCursors)
{
  return tagLines(range.start(), range.end(), realCursors);
}

void KateViewInternal::tagAll()
{
  //kdDebug(13030) << "tagAll()" << endl;
  for (int z = 0; z < cache()->viewCacheLineCount(); z++)
    cache()->viewLine(z).setDirty();

  m_leftBorder->updateFont();
  m_leftBorder->update();
}

void KateViewInternal::paintCursor()
{
  if (tagLine(m_displayCursor))
    update(); //paintText (0,0,width(), height(), true);
}

// Point in content coordinates
void KateViewInternal::placeCursor( const QPoint& p, bool keepSelection, bool updateSelection )
{
  KateTextLayout thisLine = yToKateTextLayout(p.y());
  if (!thisLine.isValid())
    return;

  KTextEditor::Cursor c = renderer()->xToCursor(thisLine, startX() + p.x(), !view()->wrapCursor());

  if (c.line () < 0 || c.line() >= m_doc->lines())
    return;

  if (updateSelection)
    KateViewInternal::updateSelection( c, keepSelection );

  updateCursor( c );
}

// Point in content coordinates
bool KateViewInternal::isTargetSelected( const QPoint& p )
{
  const KateTextLayout& thisLine = yToKateTextLayout(p.y());
  if (!thisLine.isValid())
    return false;

  return m_view->cursorSelected(renderer()->xToCursor(thisLine, p.x(), !view()->wrapCursor()));
}

//BEGIN EVENT HANDLING STUFF

bool KateViewInternal::eventFilter( QObject *obj, QEvent *e )
{
  if (obj == m_lineScroll)
  {
    // the second condition is to make sure a scroll on the vertical bar doesn't cause a horizontal scroll ;)
    if (e->type() == QEvent::Wheel && m_lineScroll->minValue() != m_lineScroll->maxValue())
    {
      wheelEvent((QWheelEvent*)e);
      return true;
    }

    // continue processing
    return QWidget::eventFilter( obj, e );
  }

  switch( e->type() )
  {
    case QEvent::KeyPress:
    {
      QKeyEvent *k = (QKeyEvent *)e;

      if (m_view->m_codeCompletion->codeCompletionVisible ())
      {
        kdDebug (13030) << "hint around" << endl;

        if( k->key() == Qt::Key_Escape )
          m_view->m_codeCompletion->abortCompletion();
      }

      if ((k->key() == Qt::Key_Escape) && !m_view->config()->persistentSelection() )
      {
        m_view->clearSelection();
        return true;
      }
      else if ( !((k->state() & Qt::ControlModifier) || (k->state() & Qt::AltModifier)) )
      {
        keyPressEvent( k );
        return k->isAccepted();
      }

    } break;

    case QEvent::DragMove:
    {
      QPoint currentPoint = ((QDragMoveEvent*) e)->pos();

      QRect doNotScrollRegion( s_scrollMargin, s_scrollMargin,
                          width() - s_scrollMargin * 2,
                          height() - s_scrollMargin * 2 );

      if ( !doNotScrollRegion.contains( currentPoint ) )
      {
          startDragScroll();
          // Keep sending move events
          ( (QDragMoveEvent*)e )->accept( QRect(0,0,0,0) );
      }

      dragMoveEvent((QDragMoveEvent*)e);
    } break;

    case QEvent::DragLeave:
      // happens only when pressing ESC while dragging
      stopDragScroll();
      break;

    case QEvent::WindowBlocked:
      // next focus originates from an internal dialog:
      // don't show the modonhd prompt
      m_doc->m_isasking = -1;
      break;

    default:
      break;
  }

  return QWidget::eventFilter( obj, e );
}

void KateViewInternal::keyPressEvent( QKeyEvent* e )
{
  
  KKey key(e);

  bool codeComp = m_view->m_codeCompletion->codeCompletionVisible ();

  if (codeComp)
  {
    kdDebug (13030) << "hint around" << endl;

    if( e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return  ||
    (key == Qt::SHIFT + Qt::Key_Return) || (key == Qt::SHIFT + Qt::Key_Enter)) {
      m_view->m_codeCompletion->doComplete();
      e->accept();
      return;
    }
  }

  if( !m_doc->isReadWrite() )
  {
    e->ignore();
    return;
  }

  if ((key == Qt::Key_Return) || (key == Qt::Key_Enter))
  {
    doReturn();
    e->accept();
    return;
  }

  if ((key == Qt::SHIFT + Qt::Key_Return) || (key == Qt::SHIFT + Qt::Key_Enter))
  {
    uint ln = m_cursor.line();
    int col = m_cursor.column();
    KateTextLine::Ptr line = m_doc->kateTextLine( ln );
    int pos = line->firstChar();
    if (pos > m_cursor.column()) pos = m_cursor.column();
    if (pos != -1) {
      while ((int)line->length() > pos &&
             !line->getChar(pos).isLetterOrNumber() &&
             pos < m_cursor.column()) ++pos;
    } else {
      pos = line->length(); // stay indented
    }
    m_doc->editStart();
    m_doc->insertText( KTextEditor::Cursor(m_cursor.line(), line->length()), "\n" +  line->string(0, pos)
      + line->string().right( line->length() - m_cursor.column() ) );
    m_cursor.setPosition(KTextEditor::Cursor(ln + 1, pos));
    if (col < int(line->length()))
      m_doc->editRemoveText(ln, col, line->length() - col);
    m_doc->editEnd();
    updateCursor(m_cursor, true);
    updateView();
    e->accept();

    return;
  }

  if (key == Qt::Key_Backspace || key == Qt::SHIFT + Qt::Key_Backspace)
  {
    //m_view->backspace();
    e->accept();

    if (codeComp)
      m_view->m_codeCompletion->updateBox ();

    return;
  }

  if  (key == Qt::Key_Tab || key == Qt::SHIFT+Qt::Key_Backtab || key == Qt::Key_Backtab)
  {
    if (m_doc->invokeTabInterceptor(key)) {
      e->accept();
      return;
    } else
    if (m_doc->config()->configFlags() & KateDocumentConfig::cfTabIndents)
    {
      if( key == Qt::Key_Tab )
      {
        if (m_view->selection() || (m_doc->config()->configFlags() & KateDocumentConfig::cfTabIndentsMode))
          m_doc->indent( m_view, m_cursor.line(), 1 );
        else if (m_doc->config()->configFlags() & KateDocumentConfig::cfTabInsertsTab)
          m_doc->typeChars ( m_view, QString ("\t") );
        else
          m_doc->insertIndentChars ( m_view );

        e->accept();

        if (codeComp)
          m_view->m_codeCompletion->updateBox ();

        return;
      }

      if (key == Qt::SHIFT+Qt::Key_Backtab || key == Qt::Key_Backtab)
      {
        m_doc->indent( m_view, m_cursor.line(), -1 );
        e->accept();

        if (codeComp)
          m_view->m_codeCompletion->updateBox ();

        return;
      }
    }
}
  if ( !(e->state() & Qt::ControlModifier) && !(e->state() & Qt::AltModifier)
       && m_doc->typeChars ( m_view, e->text() ) )
  {
    e->accept();

    if (codeComp)
      m_view->m_codeCompletion->updateBox ();

    return;
  }

  e->ignore();
}

void KateViewInternal::keyReleaseEvent( QKeyEvent* e )
{
  KKey key(e);

  if (key == Qt::SHIFT)
    m_shiftKeyPressed = true;
  else
  {
    if (m_shiftKeyPressed)
    {
      m_shiftKeyPressed = false;

      if (m_selChangedByUser)
      {
        if (m_view->selection())
          QApplication::clipboard()->setText(m_view->selectionText (), QClipboard::Selection);

        m_selChangedByUser = false;
      }
    }
  }

  e->ignore();
  return;
}

void KateViewInternal::contextMenuEvent ( QContextMenuEvent * e )
{
  // try to show popup menu

  QPoint p = e->pos();

  if ( m_view->m_doc->browserView() )
  {
    m_view->contextMenuEvent( e );
    return;
  }

  if ( e->reason() == QContextMenuEvent::Keyboard )
  {
    makeVisible( m_cursor, 0 );
    p = cursorCoordinates();
  }
  else if ( ! m_view->selection() || m_view->config()->persistentSelection() )
    placeCursor( e->pos() );

  // popup is a qguardedptr now
  if (m_view->contextMenu()) {
    m_view->contextMenu()->popup( mapToGlobal( p ) );
    e->accept ();
  }
}

void KateViewInternal::mousePressEvent( QMouseEvent* e )
{
  switch (e->button())
  {
    case Qt::LeftButton:
        m_selChangedByUser = false;

        if (m_possibleTripleClick)
        {
          m_possibleTripleClick = false;

          m_selectionMode = Line;

          if ( e->state() & Qt::ShiftModifier )
          {
            updateSelection( m_cursor, true );
          }
          else
          {
            m_view->selectLine( m_cursor );
          }

          if (m_view->selection())
            QApplication::clipboard()->setText(m_view->selectionText (), QClipboard::Selection);

          m_selectionCached = m_view->selectionRange();

          m_cursor.setColumn(0);
          updateCursor( m_cursor );
          return;
        }

        if ( e->state() & Qt::ShiftModifier )
        {
          m_selectionCached = m_view->selectionRange();
        }
        else
          m_selectionCached.start().setLine( -1 ); // invalidate

        if( isTargetSelected( e->pos() ) )
        {
          m_dragInfo.state = diPending;
          m_dragInfo.start = e->pos();
        }
        else
        {
          m_dragInfo.state = diNone;

          placeCursor( e->pos(), e->state() & Qt::ShiftModifier );

          m_scrollX = 0;
          m_scrollY = 0;

          m_scrollTimer.start (50);
        }

        e->accept ();
        break;

    default:
      e->ignore ();
      break;
  }
}

void KateViewInternal::mouseDoubleClickEvent(QMouseEvent *e)
{
  switch (e->button())
  {
    case Qt::LeftButton:
      m_selectionMode = Word;

      if ( e->state() & Qt::ShiftModifier )
      {
        m_selectionCached = m_view->selectionRange();
        updateSelection( m_cursor, true );
      }
      else
      {
        m_view->selectWord( m_cursor );
        m_selectAnchor = m_view->selectionRange().end();
        m_selectionCached = m_view->selectionRange();
      }

      // Move cursor to end of selected word
      if (m_view->selection())
      {
        QApplication::clipboard()->setText(m_view->selectionText (), QClipboard::Selection);

        m_cursor.setPosition(m_view->selectionRange().end());
        updateCursor( m_cursor );

        m_selectionCached = m_view->selectionRange();
      }

      m_possibleTripleClick = true;
      QTimer::singleShot ( QApplication::doubleClickInterval(), this, SLOT(tripleClickTimeout()) );

      e->accept ();
      break;

    default:
      e->ignore ();
      break;
  }
}

void KateViewInternal::tripleClickTimeout()
{
  m_possibleTripleClick = false;
}

void KateViewInternal::mouseReleaseEvent( QMouseEvent* e )
{
  switch (e->button())
  {
    case Qt::LeftButton:
      m_selectionMode = Default;
//       m_selectionCached.start().setLine( -1 );

      if (m_selChangedByUser)
      {
        if (m_view->selection())
          QApplication::clipboard()->setText(m_view->selectionText (), QClipboard::Selection);

        m_selChangedByUser = false;
      }

      if (m_dragInfo.state == diPending)
        placeCursor( e->pos(), e->state() & Qt::ShiftModifier );
      else if (m_dragInfo.state == diNone)
        m_scrollTimer.stop ();

      m_dragInfo.state = diNone;

      e->accept ();
      break;

    case Qt::MidButton:
      placeCursor( e->pos() );

      if( m_doc->isReadWrite() )
      {
        m_doc->paste( m_view, QClipboard::Selection );
        repaint();
      }

      e->accept ();
      break;

    default:
      e->ignore ();
      break;
  }
}

void KateViewInternal::mouseMoveEvent( QMouseEvent* e )
{
  // FIXME only do this if needing to track mouse movement
  /*const KateTextLayout& thisLine = yToKateTextLayout(e->y());
  if (thisLine.isValid()) {
    m_mouse.setPosition(renderer()->xToCursor(thisLine, e->x(), !view()->wrapCursor()));
  } else {
    m_mouse.setPosition(KTextEditor::Cursor(0,0));
  }*/

  if( e->state() & Qt::LeftButton )
  {
    if (m_dragInfo.state == diPending)
    {
      // we had a mouse down, but haven't confirmed a drag yet
      // if the mouse has moved sufficiently, we will confirm
      QPoint p( e->pos() - m_dragInfo.start );

      // we've left the drag square, we can start a real drag operation now
      if( p.manhattanLength() > KGlobalSettings::dndEventDelay() )
        doDrag();

      return;
    }

    m_mouseX = e->x();
    m_mouseY = e->y();

    m_scrollX = 0;
    m_scrollY = 0;
    int d = renderer()->fontHeight();

    if (m_mouseX < 0)
      m_scrollX = -d;

    if (m_mouseX > width())
      m_scrollX = d;

    if (m_mouseY < 0)
    {
      m_mouseY = 0;
      m_scrollY = -d;
    }

    if (m_mouseY > height())
    {
      m_mouseY = height();
      m_scrollY = d;
    }

    placeCursor( QPoint( m_mouseX, m_mouseY ), true );

  }
  else
  {
    if (isTargetSelected( e->pos() ) ) {
      // mouse is over selected text. indicate that the text is draggable by setting
      // the arrow cursor as other Qt text editing widgets do
      if (m_mouseCursor != Qt::ArrowCursor) {
        setCursor( KCursor::arrowCursor() );
        m_mouseCursor = Qt::ArrowCursor;
      }
    } else {
      // normal text cursor
      if (m_mouseCursor != Qt::IBeamCursor) {
        setCursor( KCursor::ibeamCursor() );
        m_mouseCursor = Qt::IBeamCursor;
      }
    }

    if (m_textHintEnabled)
    {
       m_textHintTimer.start(m_textHintTimeout);
       m_textHintMouseX=e->x();
       m_textHintMouseY=e->y();
    }
  }
}

void KateViewInternal::paintEvent(QPaintEvent *e)
{
  //kdDebug (13030) << "GOT PAINT EVENT: x: " << e->rect().x() << " y: " << e->rect().y()
  //  << " width: " << e->rect().width() << " height: " << e->rect().height() << endl;

  int xStart = startX() + e->rect().x();
  int xEnd = xStart + e->rect().width();
  uint h = renderer()->fontHeight();
  uint startz = (e->rect().y() / h);
  uint endz = startz + 1 + (e->rect().height() / h);
  uint lineRangesSize = cache()->viewCacheLineCount();
  bool paintOnlyDirty = false; // hack atm ;)

  QPainter paint(this);

  // TODO put in the proper places
  renderer()->setCaretStyle(m_view->isOverwriteMode() ? KateRenderer::Replace : KateRenderer::Insert);
  renderer()->setShowTabs(m_doc->config()->configFlags() & KateDocumentConfig::cfShowTabs);

  int sy = startz * h;
  paint.translate(e->rect().x(), startz * h);

  for (uint z=startz; z <= endz; z++)
  {
    if ( (z >= lineRangesSize) || ((cache()->viewLine(z).line() == -1) && (!paintOnlyDirty || cache()->viewLine(z).isDirty())) )
    {
      if (!(z >= lineRangesSize))
        cache()->viewLine(z).setDirty(false);

      paint.fillRect( 0, 0, e->rect().width(), h, renderer()->config()->backgroundColor() );
    }
    else if (!paintOnlyDirty || cache()->viewLine(z).isDirty())
    {
      cache()->viewLine(z).setDirty(false);

      //kdDebug (13030) << "paint text: x: " << e->rect().x() << " y: " << sy
      // << " width: " << xEnd-xStart << " height: " << h << endl;

      if (!cache()->viewLine(z).viewLine() || z == startz) {
        if (cache()->viewLine(z).viewLine())
          paint.translate(0, h * - cache()->viewLine(z).viewLine());

        renderer()->paintTextLine(paint, cache()->viewLine(z).kateLineLayout(), xStart, xEnd, &m_cursor);

        if (cache()->viewLine(z).viewLine())
          paint.translate(0, h * cache()->viewLine(z).viewLine());
      }
    }

    paint.translate(0, h);
    sy += h;
  }
}

void KateViewInternal::resizeEvent(QResizeEvent* e)
{
  bool expandedHorizontally = width() > e->oldSize().width();
  bool expandedVertically = height() > e->oldSize().height();
  bool heightChanged = height() != e->oldSize().height();

  m_madeVisible = false;

  if (heightChanged) {
    setAutoCenterLines(m_autoCenterLines, false);
    m_cachedMaxStartPos.setPosition(-1, -1);
  }

  if (m_view->dynWordWrap()) {
    bool dirtied = false;

    for (int i = 0; i < cache()->viewCacheLineCount(); i++) {
      // find the first dirty line
      // the word wrap updateView algorithm is forced to check all lines after a dirty one
      if (cache()->viewLine(i).wrap() ||
          (!expandedHorizontally && (cache()->viewLine(i).endX() - cache()->viewLine(i).startX()) > width())) {
        dirtied = true;
        cache()->viewLine(i).setDirty();
        break;
      }
    }

    if (dirtied || heightChanged) {
      updateView(true);
      m_leftBorder->update();
    }

    if (width() < e->oldSize().width()) {
      if (!m_view->wrapCursor()) {
        // May have to restrain cursor to new smaller width...
        if (m_cursor.column() > m_doc->lineLength(m_cursor.line())) {
          KateTextLayout thisLine = currentLayout();

          KTextEditor::Cursor newCursor(m_cursor.line(), thisLine.endCol() + ((width() - thisLine.xOffset() - (thisLine.endX() - thisLine.startX())) / renderer()->spaceWidth()) - 1);
          updateCursor(newCursor);
        }
      }
    }

  } else {
    updateView();

    if (expandedHorizontally && startX() > 0)
      scrollColumns(startX() - (width() - e->oldSize().width()));
  }

  if (expandedVertically) {
    KTextEditor::Cursor max = maxStartPos();
    if (startPos() > max)
      scrollPos(max);
  }
}

void KateViewInternal::scrollTimeout ()
{
  if (m_scrollX || m_scrollY)
  {
    scrollLines (startPos().line() + (m_scrollY / (int)renderer()->fontHeight()));
    placeCursor( QPoint( m_mouseX, m_mouseY ), true );
  }
}

void KateViewInternal::cursorTimeout ()
{
  renderer()->setDrawCaret(!renderer()->drawCaret());
  paintCursor();
}

void KateViewInternal::textHintTimeout ()
{
  m_textHintTimer.stop ();

  KateTextLayout thisLine = yToKateTextLayout(m_textHintMouseY);

  if (!thisLine.isValid()) return;

  if (m_textHintMouseX> (lineMaxCursorX(thisLine) - thisLine.startX())) return;

  KTextEditor::Cursor c = thisLine.start();
  c = renderer()->xToCursor(cache()->textLayout(c), m_textHintMouseX, !view()->wrapCursor());

  QString tmp;

  emit m_view->needTextHint(c, tmp);

  if (!tmp.isEmpty()) kdDebug(13030)<<"Hint text: "<<tmp<<endl;
}

void KateViewInternal::focusInEvent (QFocusEvent *)
{
  if (KApplication::cursorFlashTime() > 0)
    m_cursorTimer.start ( KApplication::cursorFlashTime() / 2 );

  if (m_textHintEnabled)
    m_textHintTimer.start( m_textHintTimeout );

  paintCursor();

  m_doc->setActiveView( m_view );

  // this will handle focus stuff in kateview
  m_view->slotGotFocus ();
}

void KateViewInternal::focusOutEvent (QFocusEvent *)
{
  if( ! m_view->m_codeCompletion->codeCompletionVisible() )
  {
    m_cursorTimer.stop();

    renderer()->setDrawCaret(true);

    paintCursor();

    // this will handle focus stuff in kateview
    m_view->slotLostFocus ();
  }

  m_textHintTimer.stop();
}

void KateViewInternal::doDrag()
{
  m_dragInfo.state = diDragging;
  m_dragInfo.dragObject = new QDrag(this);
  QMimeData *mimeData=new QMimeData();
  mimeData->setText(m_view->selectionText());
  m_dragInfo.dragObject->setMimeData(mimeData);
  m_dragInfo.dragObject->start(Qt::MoveAction);
}

void KateViewInternal::dragEnterEvent( QDragEnterEvent* event )
{
  if (event->source()==this) event->setDropAction(Qt::MoveAction);
  event->accept( (event->mimeData()->hasText() && m_doc->isReadWrite()) ||
                  KURL::List::canDecode(event->mimeData()) );
}

void KateViewInternal::fixDropEvent(QDropEvent* event) {
  if (event->source()!=this) event->setDropAction(Qt::CopyAction);
  else {
    Qt::DropAction action=Qt::MoveAction;
#ifdef Q_WS_MAC
    if(event->keyboardModifiers() & Qt::AltModifier)
        action = Qt::CopyAction;
#else
    if (event->keyboardModifiers() & Qt::ControlModifier)
        action = Qt::CopyAction;
#endif
    event->setDropAction(action);
  }
}

void KateViewInternal::dragMoveEvent( QDragMoveEvent* event )
{
  // track the cursor to the current drop location
  placeCursor( event->pos(), true, false );

  // important: accept action to switch between copy and move mode
  // without this, the text will always be copied.
  fixDropEvent(event);
}

void KateViewInternal::dropEvent( QDropEvent* event )
{
  if ( KURL::List::canDecode(event->mimeData()) ) {

      emit dropEventPass(event);

  } else if ( event->mimeData()->hasText() && m_doc->isReadWrite() ) {

    QString text=event->mimeData()->text();

    // is the source our own document?
    bool priv = false;
    if (event->source() && event->source()->inherits("KateViewInternal"))
      priv = m_doc->ownedView( ((KateViewInternal*)(event->source()))->m_view );

    // dropped on a text selection area?
    bool selected = isTargetSelected( event->pos() );

    if( priv && selected ) {
      // this is a drag that we started and dropped on our selection
      // ignore this case
      return;
    }

    fixDropEvent(event);

    // use one transaction
    m_doc->editStart ();

    // on move: remove selected text; on copy: duplicate text
    m_doc->insertText(m_cursor, text );

    if ( event->dropAction() != Qt::CopyAction )
      m_view->removeSelectedText();

    m_doc->editEnd ();

    placeCursor( event->pos() );

    event->acceptAction();
    updateView();
  }

  // finally finish drag and drop mode
  m_dragInfo.state = diNone;
  // important, because the eventFilter`s DragLeave does not occure
  stopDragScroll();
}
//END EVENT HANDLING STUFF

void KateViewInternal::clear()
{
  m_cursor.setPosition(KTextEditor::Cursor(0, 0));
  m_displayCursor.setPosition(0, 0);
}

void KateViewInternal::wheelEvent(QWheelEvent* e)
{
  if (m_lineScroll->minValue() != m_lineScroll->maxValue() && e->orientation() != Qt::Horizontal) {
    // React to this as a vertical event
    if ( ( e->state() & Qt::ControlModifier ) || ( e->state() & Qt::ShiftModifier ) ) {
      if (e->delta() > 0)
        scrollPrevPage();
      else
        scrollNextPage();
    } else {
      scrollViewLines(-((e->delta() / 120) * QApplication::wheelScrollLines()));
      // maybe a menu was opened or a bubbled window title is on us -> we shall erase it
      update();
      m_leftBorder->update();
    }

  } else if (!m_columnScroll->isHidden()) {
    QWheelEvent copy = *e;
    QApplication::sendEvent(m_columnScroll, &copy);

  } else {
    e->ignore();
  }
}

void KateViewInternal::startDragScroll()
{
  if ( !m_dragScrollTimer.isActive() ) {
    m_suppressColumnScrollBar = true;
    m_dragScrollTimer.start( s_scrollTime );
  }
}

void KateViewInternal::stopDragScroll()
{
  m_suppressColumnScrollBar = false;
  m_dragScrollTimer.stop();
  updateView();
}

void KateViewInternal::doDragScroll()
{
  QPoint p = this->mapFromGlobal( QCursor::pos() );

  int dx = 0, dy = 0;
  if ( p.y() < s_scrollMargin ) {
    dy = p.y() - s_scrollMargin;
  } else if ( p.y() > height() - s_scrollMargin ) {
    dy = s_scrollMargin - (height() - p.y());
  }

  if ( p.x() < s_scrollMargin ) {
    dx = p.x() - s_scrollMargin;
  } else if ( p.x() > width() - s_scrollMargin ) {
    dx = s_scrollMargin - (width() - p.x());
  }

  dy /= 4;

  if (dy)
    scrollLines(startPos().line() + dy);

  if (!m_view->dynWordWrap() && m_columnScrollDisplayed && dx)
    scrollColumns(kMin (m_startX + dx, m_columnScroll->maxValue()));

  if (!dy && !dx)
    stopDragScroll();
}

void KateViewInternal::enableTextHints(int timeout)
{
  m_textHintTimeout=timeout;
  m_textHintEnabled=true;
  m_textHintTimer.start(timeout);
}

void KateViewInternal::disableTextHints()
{
  m_textHintEnabled=false;
  m_textHintTimer.stop ();
}

//BEGIN EDIT STUFF
void KateViewInternal::editStart()
{
  editSessionNumber++;

  if (editSessionNumber > 1)
    return;

  editIsRunning = true;
  editOldCursor = m_cursor;
}

void KateViewInternal::editEnd(int editTagLineStart, int editTagLineEnd, bool tagFrom)
{
   if (editSessionNumber == 0)
    return;

  editSessionNumber--;

  if (editSessionNumber > 0)
    return;

  if (tagFrom && (editTagLineStart <= int(m_doc->getRealLine(startLine()))))
    tagAll();
  else
    tagLines (editTagLineStart, tagFrom ? m_doc->lastLine() : editTagLineEnd, true);

  if (editOldCursor == m_cursor)
    updateBracketMarks();

  if (m_imPreedit.start() >= m_imPreedit.end())
    updateView(true);

  if ((editOldCursor != m_cursor) && (m_imPreedit.start() >= m_imPreedit.end()))
  {
    m_madeVisible = false;
    updateCursor ( m_cursor, true );
  }
  else //if ( m_view->isActive() )
  {
#warning fixme, this needs to be fixed app transparent to only be done if this view is the view where the editing did happen

    makeVisible(m_displayCursor, m_displayCursor.column());
  }

  editIsRunning = false;
}

void KateViewInternal::editSetCursor (const KTextEditor::Cursor &_cursor)
{
  if (m_cursor != _cursor)
  {
    m_cursor = _cursor;
  }
}
//END

void KateViewInternal::viewSelectionChanged ()
{
  if (!m_view->selection())
    m_selectAnchor.setPosition (-1, -1);
}

void KateViewInternal::inputMethodEvent(QInputMethodEvent* e)
{
  if ( m_doc->m_bReadOnly ) {
    e->ignore();
    return;
  }

  if ( m_view->selection() )
    m_view->removeSelectedText();

  m_imPreedit.setRange(m_cursor, m_cursor);
  m_imPreeditSelStart = m_cursor;

  m_view->setIMSelectionValue( m_imPreedit, m_imPreedit, true );

  QWidget::inputMethodEvent(e);
}

KateLayoutCache* KateViewInternal::cache( ) const
{
  return m_layoutCache;
}

KTextEditor::Cursor KateViewInternal::toRealCursor( const KTextEditor::Cursor & virtualCursor ) const
{
  return KTextEditor::Cursor(m_doc->getRealLine(virtualCursor.line()), virtualCursor.column());
}

KTextEditor::Cursor KateViewInternal::toVirtualCursor( const KTextEditor::Cursor & realCursor ) const
{
  return KTextEditor::Cursor(m_doc->getVirtualLine(realCursor.line()), realCursor.column());
}

KateRenderer * KateViewInternal::renderer( ) const
{
  return m_view->renderer();
}

#if 0
//BEGIN IM INPUT STUFF
void KateViewInternal::imStartEvent( QIMEvent *e )
{
  if ( m_doc->m_bReadOnly ) {
    e->ignore();
    return;
  }

  if ( m_view->selection() )
    m_view->removeSelectedText();

  m_imPreeditStartLine = m_cursor.line();
  m_imPreeditStart = m_cursor.column();
  m_imPreeditLength = 0;
  m_imPreeditSelStart = m_imPreeditStart;

  m_view->setIMSelectionValue( m_imPreeditStartLine, m_imPreeditStart, 0, 0, 0, true );
}

void KateViewInternal::imComposeEvent( QIMEvent *e )
{
  if ( m_doc->m_bReadOnly ) {
    e->ignore();
    return;
  }

  // remove old preedit
  if ( m_imPreeditLength > 0 ) {
    m_cursor.setPosition( m_imPreeditStartLine, m_imPreeditStart );
    m_doc->removeText( m_imPreeditStartLine, m_imPreeditStart,
                       m_imPreeditStartLine, m_imPreeditStart + m_imPreeditLength );
  }

  m_imPreeditLength = e->text().length();
  m_imPreeditSelStart = m_imPreeditStart + e->cursorPos();

  // update selection
  m_view->setIMSelectionValue( m_imPreeditStartLine, m_imPreeditStart, m_imPreeditStart + m_imPreeditLength,
                              m_imPreeditSelStart, m_imPreeditSelStart + e->selectionLength(),
                              true );

  // insert new preedit
  m_doc->insertText( m_imPreeditStartLine, m_imPreeditStart, e->text() );


  // update cursor
  m_cursor.setPosition( m_imPreeditStartLine, m_imPreeditSelStart );
  updateCursor( cursor, true );

  updateView( true );
}

void KateViewInternal::imEndEvent( QIMEvent *e )
{
  if ( m_doc->m_bReadOnly ) {
    e->ignore();
    return;
  }

  if ( m_imPreeditLength > 0 ) {
    m_cursor.setPosition( m_imPreeditStartLine, m_imPreeditStart );
    m_doc->removeText( m_imPreeditStartLine, m_imPreeditStart,
                       m_imPreeditStartLine, m_imPreeditStart + m_imPreeditLength );
  }

  m_view->setIMSelectionValue( m_imPreeditStartLine, m_imPreeditStart, 0, 0, 0, false );

  if ( e->text().length() > 0 ) {
    m_doc->insertText( m_cursor.line(), m_cursor.column(), e->text() );

    if ( !m_cursorTimer.isActive() && KApplication::cursorFlashTime() > 0 )
      m_cursorTimer.start ( KApplication::cursorFlashTime() / 2 );

    updateView( true );
    updateCursor( cursor, true );
  }

  m_imPreeditStart = 0;
  m_imPreeditLength = 0;
  m_imPreeditSelStart = 0;
}
//END IM INPUT STUFF
#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
