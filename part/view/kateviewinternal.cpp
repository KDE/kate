/* This file is part of the KDE libraries
   Copyright (C) 2002 John Firebaugh <jfirebaugh@kde.org>
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2002,2003 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002-2007 Hamish Rodda <rodda@kde.org>
   Copyright (C) 2003 Anakim Border <aborder@sources.sourceforge.net>
   Copyright (C) 2007 Mirko Stocker <me@misto.ch>
   Copyright (C) 2007 Matthew Woehlke <mw_triad@users.sourceforge.net>
   Copyright (C) 2008 Erlend Hamberg <ehamberg@gmail.com>

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
#include "kateviewhelpers.h"
#include "katehighlight.h"
#include "katebuffer.h"
#include "katerenderer.h"
#include "kateconfig.h"
#include "katelayoutcache.h"
#include "katecompletionwidget.h"
#include "kateviinputmodemanager.h"
#include "katesearchbar.h"
#include "spellcheck/spellingmenu.h"
#include "kateviewaccessible.h"
#include "katetextanimation.h"
#include "katemessagewidget.h"

#include <ktexteditor/movingrange.h>
#include <kcursor.h>
#include <kdebug.h>
#include <kapplication.h>
#include <kglobalsettings.h>

#include <QtCore/QMimeData>
#include <QtGui/QPainter>
#include <QtGui/QClipboard>
#include <QtGui/QPixmap>
#include <QtGui/QKeyEvent>
#include <QtGui/QLayout>
#include <QToolTip>

static const bool debugPainting = false;

KateViewInternal::KateViewInternal(KateView *view)
  : QWidget (view)
  , editSessionNumber (0)
  , editIsRunning (false)
  , m_view (view)
  , m_cursor(doc()->buffer(), KTextEditor::Cursor(0, 0), Kate::TextCursor::MoveOnInsert)
  , m_mouse()
  , m_possibleTripleClick (false)
  , m_completionItemExpanded (false)
  , m_bm(doc()->newMovingRange(KTextEditor::Range::invalid(), KTextEditor::MovingRange::DoNotExpand))
  , m_bmStart(doc()->newMovingRange(KTextEditor::Range::invalid(), KTextEditor::MovingRange::DoNotExpand))
  , m_bmEnd(doc()->newMovingRange(KTextEditor::Range::invalid(), KTextEditor::MovingRange::DoNotExpand))
  , m_bmLastFlashPos(doc()->newMovingCursor(KTextEditor::Cursor::invalid()))
  , m_dummy (0)

  // stay on cursor will avoid that the view scroll around on press return at beginning
  , m_startPos (doc()->buffer(), KTextEditor::Cursor (0, 0), Kate::TextCursor::StayOnInsert)

  , m_visibleLineCount(0)
  , m_madeVisible(false)
  , m_shiftKeyPressed (false)
  , m_autoCenterLines(0)
  , m_minLinesVisible(0)
  , m_selChangedByUser (false)
  , m_selectAnchor (-1, -1)
  , m_selectionMode( Default )
  , m_layoutCache(new KateLayoutCache(renderer(), this))
  , m_preserveX(false)
  , m_preservedX(0)
  , m_cachedMaxStartPos(-1, -1)
  , m_dragScrollTimer(this)
  , m_scrollTimer (this)
  , m_cursorTimer (this)
  , m_textHintTimer (this)
  , m_textHintEnabled(false)
  , m_textHintPos(-1, -1)
  , m_imPreeditRange(0)
  , m_viInputMode(false)
  , m_viInputModeManager (0)
{
  setMinimumSize (0,0);
  setAttribute(Qt::WA_OpaquePaintEvent);
  setAttribute(Qt::WA_InputMethodEnabled);

  // invalidate m_selectionCached.start(), or keyb selection is screwed initially
  m_selectionCached = KTextEditor::Range::invalid();

  // bracket markers are only for this view and should not be printed
  m_bm->setView (m_view);
  m_bmStart->setView (m_view);
  m_bmEnd->setView (m_view);
  m_bm->setAttributeOnlyForViews (true);
  m_bmStart->setAttributeOnlyForViews (true);
  m_bmEnd->setAttributeOnlyForViews (true);

  // use z depth defined in moving ranges interface
  m_bm->setZDepth (-1000.0);
  m_bmStart->setZDepth (-1000.0);
  m_bmEnd->setZDepth (-1000.0);

  // update mark attributes
  updateBracketMarkAttributes();

  //
  // scrollbar for lines
  //
  m_lineScroll = new KateScrollBar(Qt::Vertical, this);
  m_lineScroll->show();
  m_lineScroll->setTracking (true);
  m_lineScroll->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Expanding );

  // Hijack the line scroller's controls, so we can scroll nicely for word-wrap
  connect(m_lineScroll, SIGNAL(actionTriggered(int)), SLOT(scrollAction(int)));
  connect(m_lineScroll, SIGNAL(sliderMoved(int)), SLOT(scrollLines(int)));
  connect(m_lineScroll, SIGNAL(sliderMMBMoved(int)), SLOT(scrollLines(int)));
  connect(m_lineScroll, SIGNAL(valueChanged(int)), SLOT(scrollLines(int)));

  // catch wheel events, completing the hijack
  m_lineScroll->installEventFilter(this);

  //
  // scrollbar for columns
  //
  m_columnScroll = new QScrollBar(Qt::Horizontal,m_view);

  if (m_view->dynWordWrap())
    m_columnScroll->hide();
  else
    m_columnScroll->show();

  m_columnScroll->setTracking(true);
  m_startX = 0;

  connect(m_columnScroll, SIGNAL(valueChanged(int)), SLOT(scrollColumns(int)));

  // bottom corner box
  m_dummy = new QWidget(m_view);
  m_dummy->setFixedSize(m_lineScroll->width(), m_columnScroll->sizeHint().height());
  m_dummy->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );

  if (m_view->dynWordWrap())
    m_dummy->hide();
  else
    m_dummy->show();

  cache()->setWrap(m_view->dynWordWrap());

  //
  // iconborder ;)
  //
  m_leftBorder = new KateIconBorder( this, m_view );
  m_leftBorder->show ();

  // update view if folding ranges change
  connect( &m_view->textFolding(), SIGNAL(foldingRangesChanged()), SLOT(slotRegionVisibilityChanged()));

  m_displayCursor.setPosition(0, 0);

  setAcceptDrops( true );

  // event filter
  installEventFilter(this);

  // set initial cursor
  m_mouseCursor = Qt::IBeamCursor;
  setCursor(m_mouseCursor);

  // call mouseMoveEvent also if no mouse button is pressed
  setMouseTracking(true);

  m_dragInfo.state = diNone;

  // timers
  connect( &m_dragScrollTimer, SIGNAL(timeout()),
             this, SLOT(doDragScroll()) );

  connect( &m_scrollTimer, SIGNAL(timeout()),
             this, SLOT(scrollTimeout()) );

  connect( &m_cursorTimer, SIGNAL(timeout()),
             this, SLOT(cursorTimeout()) );

  connect( &m_textHintTimer, SIGNAL(timeout()),
             this, SLOT(textHintTimeout()) );

  // selection changed to set anchor
  connect( m_view, SIGNAL(selectionChanged(KTextEditor::View*)),
             this, SLOT(viewSelectionChanged()) );

#ifndef QT_NO_ACCESSIBILITY
#if QT_VERSION >= 0x040800
  QAccessible::installFactory(accessibleInterfaceFactory);
#endif
#endif

  // update is called in KateView, after construction and layout is over
  // but before any other kateviewinternal call
}

KateViewInternal::~KateViewInternal ()
{
  // delete text animation object here, otherwise it updates the view in its destructor
  if (m_textAnimation) {
    delete m_textAnimation;
  }

#ifndef QT_NO_ACCESSIBILITY
#if QT_VERSION >= 0x040800
  QAccessible::removeFactory(accessibleInterfaceFactory);
#endif
#endif

  // kill preedit ranges
  delete m_imPreeditRange;
  qDeleteAll (m_imPreeditRangeChildren);

  delete m_viInputModeManager;

  // delete bracket markers
  delete m_bm;
  delete m_bmStart;
  delete m_bmEnd;
}

void KateViewInternal::prepareForDynWrapChange()
{
  // Which is the current view line?
  m_wrapChangeViewLine = cache()->displayViewLine(m_displayCursor, true);
}

void KateViewInternal::dynWrapChanged()
{
  m_dummy->setFixedSize(m_lineScroll->width(), m_columnScroll->sizeHint().height());
  if (m_view->dynWordWrap())
  {
    m_columnScroll->hide();
    m_dummy->hide();

  }
  else
  {
    // column scrollbar + bottom corner box
    m_columnScroll->show();
    m_dummy->show();
  }

  cache()->setWrap(m_view->dynWordWrap());
  updateView();

  if (m_view->dynWordWrap())
    scrollColumns(0);

  // Determine where the cursor should be to get the cursor on the same view line
  if (m_wrapChangeViewLine != -1) {
    KTextEditor::Cursor newStart = viewLineOffset(m_displayCursor, -m_wrapChangeViewLine);
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

  for (int i = qMin(linesDisplayed() - 1, cache()->viewCacheLineCount() - 1); i >= 0; i--) {
    const KateTextLayout& thisLine = cache()->viewLine(i);

    if (thisLine.line() == -1) continue;

    if (thisLine.virtualLine() >= m_view->textFolding().visibleLines()) {
      // Cache is too out of date
      return KTextEditor::Cursor(m_view->textFolding().visibleLines() - 1, doc()->lineLength(m_view->textFolding().visibleLineToLine(m_view->textFolding().visibleLines() - 1)));
    }

    return KTextEditor::Cursor(thisLine.virtualLine(), thisLine.wrap() ? thisLine.endCol() - 1 : thisLine.endCol());
  }

  return KTextEditor::Cursor();
}

int KateViewInternal::endLine() const
{
  return endPos().line();
}

KateTextLayout KateViewInternal::yToKateTextLayout(int y) const
{
  if (y < 0 || y > size().height())
    return KateTextLayout::invalid();

  int range = y / renderer()->lineHeight();

  // lineRanges is always bigger than 0, after the initial updateView call
  if (range >= 0 && range < cache()->viewCacheLineCount())
    return cache()->viewLine(range);

  return KateTextLayout::invalid();
}

int KateViewInternal::lineToY(int viewLine) const
{
  return (viewLine-startLine()) * renderer()->lineHeight();
}

void KateViewInternal::slotIncFontSizes()
{
  renderer()->increaseFontSizes();
  update();
}

void KateViewInternal::slotDecFontSizes()
{
  renderer()->decreaseFontSizes();
  update();
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

  bool blocked = m_lineScroll->blockSignals(true);
  m_lineScroll->setValue(startLine());
  m_lineScroll->blockSignals(blocked);
}

void KateViewInternal::scrollAction( int action )
{
  switch  (action) {
    case QAbstractSlider::SliderSingleStepAdd:
      scrollNextLine();
      break;

    case QAbstractSlider::SliderSingleStepSub:
      scrollPrevLine();
      break;

    case QAbstractSlider::SliderPageStepAdd:
      scrollNextPage();
      break;

    case QAbstractSlider::SliderPageStepSub:
      scrollPrevPage();
      break;

    case QAbstractSlider::SliderToMinimum:
      top_home();
      break;

    case QAbstractSlider::SliderToMaximum:
      bottom_end();
      break;
  }
}

void KateViewInternal::scrollNextPage()
{
  scrollViewLines(qMax( linesDisplayed() - 1, 0 ));
}

void KateViewInternal::scrollPrevPage()
{
  scrollViewLines(-qMax( linesDisplayed() - 1, 0 ));
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
  cache()->setAcceptDirtyLayouts(true);

  if (m_cachedMaxStartPos.line() == -1 || changed)
  {
    KTextEditor::Cursor end(m_view->textFolding().visibleLines() - 1, doc()->lineLength(m_view->textFolding().visibleLineToLine(m_view->textFolding().visibleLines() - 1)));

    if (m_view->config()->scrollPastEnd())
      m_cachedMaxStartPos = viewLineOffset(end, -m_minLinesVisible);
    else
      m_cachedMaxStartPos = viewLineOffset(end, -(linesDisplayed() - 1));
  }

  cache()->setAcceptDirtyLayouts(false);

  return m_cachedMaxStartPos;
}

// c is a virtual cursor
void KateViewInternal::scrollPos(KTextEditor::Cursor& c, bool force, bool calledExternally)
{
  if (!force && ((!m_view->dynWordWrap() && c.line() == startLine()) || c == startPos()))
    return;

  if (c.line() < 0)
    c.setLine(0);

  KTextEditor::Cursor limit = maxStartPos();
  if (c > limit) {
    c = limit;

    // Re-check we're not just scrolling to the same place
    if (!force && ((!m_view->dynWordWrap() && c.line() == startLine()) || c == startPos()))
      return;
  }

  int viewLinesScrolled = 0;

  // only calculate if this is really used and useful, could be wrong here, please recheck
  // for larger scrolls this makes 2-4 seconds difference on my xeon with dyn. word wrap on
  // try to get it really working ;)
  bool viewLinesScrolledUsable = !force
                                 && (c.line() >= startLine() - linesDisplayed() - 1)
                                 && (c.line() <= endLine() + linesDisplayed() + 1);

  if (viewLinesScrolledUsable) {
    viewLinesScrolled = cache()->displayViewLine(c);
  }

  m_startPos.setPosition(c);

  // set false here but reversed if we return to makeVisible
  m_madeVisible = false;

  if (viewLinesScrolledUsable)
  {
    int lines = linesDisplayed();
    if (m_view->textFolding().visibleLines() < lines) {
      KTextEditor::Cursor end(m_view->textFolding().visibleLines() - 1, doc()->lineLength(m_view->textFolding().visibleLineToLine(m_view->textFolding().visibleLines() - 1)));
      lines = qMin(linesDisplayed(), cache()->displayViewLine(end) + 1);
    }

    Q_ASSERT(lines >= 0);

    if (!calledExternally && qAbs(viewLinesScrolled) < lines)
    {
      updateView(false, viewLinesScrolled);

      int scrollHeight = -(viewLinesScrolled * (int)renderer()->lineHeight());

      // scroll excluding child widgets (floating notifications)
      scroll(0, scrollHeight, rect());
      m_leftBorder->scroll(0, scrollHeight);

      if ((m_view->m_floatTopMessageWidget && m_view->m_floatTopMessageWidget->isVisible())
        || (m_view->m_bottomMessageWidget && m_view->m_bottomMessageWidget->isVisible()))
      {
        // NOTE: on some machines we must update if the floating widget is visible
        //       otherwise strange painting bugs may occur during scrolling...
        update();
      }

      emit m_view->verticalScrollPositionChanged( m_view, c );
      emit m_view->displayRangeChanged(m_view);
      return;
    }
  }

  updateView();
  update();
  m_leftBorder->update();
  emit m_view->verticalScrollPositionChanged( m_view, c );
  emit m_view->displayRangeChanged(m_view);
}

void KateViewInternal::scrollColumns ( int x )
{
  if (x < 0)
    x = 0;

  if (x > m_columnScroll->maximum())
    x = m_columnScroll->maximum();

  if (x == m_startX)
    return;

  int dx = m_startX - x;
  m_startX = x;

  if (qAbs(dx) < width()) {
    // scroll excluding child widgets (floating notifications)
    scroll(dx, 0, rect());
  } else
    update();

  emit m_view->horizontalScrollPositionChanged( m_view );
  emit m_view->displayRangeChanged(m_view);

  bool blocked = m_columnScroll->blockSignals(true);
  m_columnScroll->setValue(m_startX);
  m_columnScroll->blockSignals(blocked);
}

// If changed is true, the lines that have been set dirty have been updated.
void KateViewInternal::updateView(bool changed, int viewLinesScrolled)
{
  doUpdateView(changed, viewLinesScrolled);

  if (changed)
    updateDirty();
}

void KateViewInternal::doUpdateView(bool changed, int viewLinesScrolled)
{
  if(!isVisible() && !viewLinesScrolled && !changed )
    return; //When this view is not visible, don't do anything

  bool blocked = m_lineScroll->blockSignals(true);

  if (width() != cache()->viewWidth()) {
    cache()->setViewWidth(width());
    changed = true;
  }

  /* It was observed that height() could be negative here --
     when the main Kate view has 0 as size (during creation),
     and there frame arount KateViewInternal.  In which
     case we'd set the view cache to 0 (or less!) lines, and
     start allocating huge chunks of data, later. */
  int newSize = (qMax (0, height()) / renderer()->lineHeight()) + 1;
  cache()->updateViewCache(startPos(), newSize, viewLinesScrolled);
  m_visibleLineCount = newSize;

  KTextEditor::Cursor maxStart = maxStartPos(changed);
  int maxLineScrollRange = maxStart.line();
  if (m_view->dynWordWrap() && maxStart.column() != 0)
    maxLineScrollRange++;
  m_lineScroll->setRange(0, maxLineScrollRange);

  m_lineScroll->setValue(startPos().line());
  m_lineScroll->setSingleStep(1);
  m_lineScroll->setPageStep(qMax (0, height()) / renderer()->lineHeight());
  m_lineScroll->blockSignals(blocked);

  KateViewConfig::ScrollbarMode show_scrollbars = static_cast<KateViewConfig::ScrollbarMode>(view()->config()->showScrollbars());

  bool visible = ( (show_scrollbars == KateViewConfig::AlwaysOn) ||
                   ((show_scrollbars == KateViewConfig::ShowWhenNeeded) && (maxLineScrollRange != 0)) );
  bool visible_dummy = visible;

  m_lineScroll->setVisible( visible );

  if (!m_view->dynWordWrap())
  {
    int max = maxLen(startLine()) - width();
    if (max < 0)
      max = 0;

    // if we lose the ability to scroll horizontally, move view to the far-left
    if (max == 0)
    {
      scrollColumns(0);
    }

    blocked = m_columnScroll->blockSignals(true);

    // disable scrollbar
    m_columnScroll->setDisabled (max == 0);

    visible = ( (show_scrollbars == KateViewConfig::AlwaysOn) ||
                ((show_scrollbars == KateViewConfig::ShowWhenNeeded) && (max != 0)) );
    visible_dummy &= visible;
    m_columnScroll->setVisible( visible );

    m_columnScroll->setRange(0, max);

    m_columnScroll->setValue(m_startX);

    // Approximate linescroll
    m_columnScroll->setSingleStep(renderer()->config()->fontMetrics().width('a'));
    m_columnScroll->setPageStep(width());

    m_columnScroll->blockSignals(blocked);
  } else {
    visible_dummy = false;
  }

  m_dummy->setVisible( visible_dummy );
}

/**
 * this function ensures a certain location is visible on the screen.
 * if endCol is -1, ignore making the columns visible.
 */
void KateViewInternal::makeVisible (const KTextEditor::Cursor& c, int endCol, bool force, bool center, bool calledExternally)
{
  //kDebug(13030) << "MakeVisible start " << startPos() << " end " << endPos() << " -> request: " << c;// , new start [" << scroll.line << "," << scroll.col << "] lines " << (linesDisplayed() - 1) << " height " << height();
    // if the line is in a folded region, unfold all the way up
    //if ( doc()->foldingTree()->findNodeForLine( c.line )->visible )
    //  kDebug(13030)<<"line ("<<c.line<<") should be visible";

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
  else if ( c > viewLineOffset(startPos(), linesDisplayed() - m_minLinesVisible - 1) )
  {
    KTextEditor::Cursor scroll = viewLineOffset(c, -(linesDisplayed() - m_minLinesVisible - 1));
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

  if (!m_view->dynWordWrap() && (endCol != -1 || m_view->wrapCursor()))
  {
    KTextEditor::Cursor rc = toRealCursor(c);
    int sX = renderer()->cursorToX(cache()->textLayout(rc), rc, !m_view->wrapCursor());

    int sXborder = sX-8;
    if (sXborder < 0)
      sXborder = 0;

    if (sX < m_startX)
      scrollColumns (sXborder);
    else if  (sX > m_startX + width())
      scrollColumns (sX - width() + 8);
  }

  m_madeVisible = !force;

#ifndef QT_NO_ACCESSIBILITY
#if QT_VERSION >= 0x040800
  QAccessible::updateAccessibility( this, KateCursorAccessible::ChildId, QAccessible::Focus );
#endif
#endif
}

void KateViewInternal::slotRegionVisibilityChanged()
{
  kDebug(13030);
  cache()->clear();

  m_cachedMaxStartPos.setLine(-1);
  KTextEditor::Cursor max = maxStartPos();
  if (startPos() > max)
    scrollPos(max);

  // if text was folded: make sure the cursor is on a visible line
  qint64 foldedRangeId = -1;
  if (!m_view->textFolding().isLineVisible (m_cursor.line(), &foldedRangeId)) {
    KTextEditor::Range foldingRange = m_view->textFolding().foldingRange(foldedRangeId);
    Q_ASSERT(foldingRange.start().isValid());

    // set cursor to start of folding region
    updateCursor(foldingRange.start(), true);
  }

  updateView();
  update();
  m_leftBorder->update();
  emit m_view->displayRangeChanged(m_view);
}

void KateViewInternal::slotRegionBeginEndAddedRemoved(unsigned int)
{
  kDebug(13030);
  // FIXME: performance problem
  m_leftBorder->update();
}

void KateViewInternal::showEvent ( QShowEvent *e )
{
  updateView ();

  QWidget::showEvent (e);
}

int KateViewInternal::linesDisplayed() const
{
  int h = height();

  // catch zero heights, even if should not happen
  int fh = qMax (1, renderer()->lineHeight());

  // default to 1, there is always one line around....
  // too many places calc with linesDisplayed() - 1
  return qMax (1, (h - (h % fh)) / fh);
}

QPoint KateViewInternal::cursorToCoordinate( const KTextEditor::Cursor & cursor, bool realCursor, bool includeBorder ) const
{
  int viewLine = cache()->displayViewLine(realCursor ? toVirtualCursor(cursor) : cursor, true);

  if (viewLine < 0 || viewLine >= cache()->viewCacheLineCount())
    return QPoint(-1, -1);

  int y = (int)viewLine * renderer()->lineHeight();

  KateTextLayout layout = cache()->viewLine(viewLine);
  int x = 0;

  // only set x value if we have a valid layout (bug #171027)
  if (layout.isValid())
    x = (int)layout.lineLayout().cursorToX(cursor.column());
//  else
//    kDebug() << "Invalid Layout";

  if (includeBorder) x += m_leftBorder->width();

  x -= startX();

  return QPoint(x, y);
}

QPoint KateViewInternal::cursorCoordinates(bool includeBorder) const
{
  return cursorToCoordinate(m_displayCursor, false, includeBorder);
}

KTextEditor::Cursor KateViewInternal::findMatchingBracket()
{
  KTextEditor::Cursor c;

  if (!m_bm->toRange().isValid())
    return KTextEditor::Cursor::invalid();

  Q_ASSERT(m_bmEnd->toRange().isValid());
  Q_ASSERT(m_bmStart->toRange().isValid());

  if (m_bmStart->toRange().contains(m_cursor) || m_bmStart->end() == m_cursor.toCursor()) {
    c = m_bmEnd->end();
  } else if (m_bmEnd->toRange().contains(m_cursor) || m_bmEnd->end() == m_cursor.toCursor()) {
    c = m_bmStart->start();
  } else {
    // should never happen: a range exists, but the cursor position is
    // neither at the start nor at the end...
    return KTextEditor::Cursor::invalid();
  }

  return c;
}

void KateViewInternal::doReturn()
{
  doc()->newLine( m_view );
  updateView();
}

void KateViewInternal::doSmartNewline()
{
  int ln = m_cursor.line();
  Kate::TextLine line = doc()->kateTextLine(ln);
  int col = qMin(m_cursor.column(), line->firstChar());
  if (col != -1) {
    while (line->length() > col &&
            !(line->at(col).isLetterOrNumber() || line->at(col) == QLatin1Char('_')) &&
            col < m_cursor.column()) ++col;
  } else {
    col = line->length(); // stay indented
  }
  doc()->editStart();
  doc()->editWrapLine(ln, m_cursor.column());
  doc()->insertText(KTextEditor::Cursor(ln + 1, 0), line->string(0, col));
  doc()->editEnd();

  updateView();
}

void KateViewInternal::doDelete()
{
  doc()->del( m_view, m_cursor );
}

void KateViewInternal::doBackspace()
{
  doc()->backspace( m_view, m_cursor );
}

void KateViewInternal::doTabulator()
{
  doc()->insertTab( m_view, m_cursor );
}

void KateViewInternal::doTranspose()
{
  doc()->transpose( m_cursor );
}

void KateViewInternal::doDeletePrevWord()
{
  doc()->editStart();
  wordPrev( true );
  KTextEditor::Range selection = m_view->selectionRange();
  m_view->removeSelectedText();
  doc()->editEnd();
  tagRange(selection, true);
  updateDirty();
}

void KateViewInternal::doDeleteNextWord()
{
  doc()->editStart();
  wordNext( true );
  KTextEditor::Range selection = m_view->selectionRange();
  m_view->removeSelectedText();
  doc()->editEnd();
  tagRange(selection, true);
  updateDirty();
}

class CalculatingCursor : public KTextEditor::Cursor {
public:
  // These constructors constrain their arguments to valid positions
  // before only the third one did, but that leads to crashs
  // see bug 227449
  CalculatingCursor(KateViewInternal* vi)
    : KTextEditor::Cursor()
    , m_vi(vi)
  {
    makeValid();
  }

  CalculatingCursor(KateViewInternal* vi, const KTextEditor::Cursor& c)
    : KTextEditor::Cursor(c)
    , m_vi(vi)
  {
    makeValid();
  }

  CalculatingCursor(KateViewInternal* vi, int line, int col)
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
    setLine(qBound( 0, line(), int( doc()->lines() - 1 ) ) );
    if (view()->wrapCursor())
      m_column = qBound( 0, column(), doc()->lineLength( line() ) );
    else
      m_column = qMax( 0, column() );
    Q_ASSERT( valid() );
  }

  void toEdge( KateViewInternal::Bias bias ) {
    if( bias == KateViewInternal::left ) m_column = 0;
    else if( bias == KateViewInternal::right ) m_column = doc()->lineLength( line() );
  }

  bool atEdge() const { return atEdge( KateViewInternal::left ) || atEdge( KateViewInternal::right ); }

  bool atEdge( KateViewInternal::Bias bias ) const {
    switch( bias ) {
    case KateViewInternal::left:  return column() == 0;
    case KateViewInternal::none:  return atEdge();
    case KateViewInternal::right: return column() >= doc()->lineLength( line() );
    default: Q_ASSERT(false); return false;
    }
  }

protected:
  bool valid() const {
    return line() >= 0 &&
            line() < doc()->lines() &&
            column() >= 0 &&
            (!view()->wrapCursor() || column() <= doc()->lineLength( line() ));
  }
  KateView* view() { return m_vi->m_view; }
  const KateView* view() const { return m_vi->m_view; }
  KateDocument* doc() { return view()->doc(); }
  const KateDocument* doc() const { return view()->doc(); }
  KateViewInternal* m_vi;
};

class BoundedCursor : public CalculatingCursor {
public:
  BoundedCursor(KateViewInternal* vi)
    : CalculatingCursor( vi ) {}
  BoundedCursor(KateViewInternal* vi, const KTextEditor::Cursor& c )
    : CalculatingCursor( vi, c ) {}
  BoundedCursor(KateViewInternal* vi, int line, int col )
    : CalculatingCursor( vi, line, col ) {}
  virtual CalculatingCursor& operator+=( int n ) {
    KateLineLayoutPtr thisLine = m_vi->cache()->line(line());
    if (!thisLine->isValid()) {
      kWarning() << "Did not retrieve valid layout for line " << line();
      return *this;
    }

    const bool wrapCursor = view()->wrapCursor();
    int maxColumn = -1;
    if (n >= 0) {
      for (int i = 0; i < n; i++) {
        if (m_column >= thisLine->length()) {
          if (wrapCursor) {
            break;

          } else if (view()->dynWordWrap()) {
            // Don't go past the edge of the screen in dynamic wrapping mode
            if (maxColumn == -1)
              maxColumn = thisLine->length() + ((m_vi->width() - thisLine->widthOfLastLine()) / m_vi->renderer()->spaceWidth()) - 1;

            if (m_column >= maxColumn) {
              m_column = maxColumn;
              break;
            }

            ++m_column;

          } else {
            ++m_column;
          }

        } else {
          m_column = thisLine->layout()->nextCursorPosition(m_column);
        }
      }
    } else {
      for (int i = 0; i > n; i--) {
        if (m_column >= thisLine->length())
          --m_column;
        else if (m_column == 0)
          break;
        else
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
    : CalculatingCursor( vi) {}
  WrappingCursor(KateViewInternal* vi, const KTextEditor::Cursor& c )
    : CalculatingCursor( vi, c ) {}
  WrappingCursor(KateViewInternal* vi, int line, int col )
    : CalculatingCursor( vi, line, col ) {}

  virtual CalculatingCursor& operator+=( int n ) {
    KateLineLayoutPtr thisLine = m_vi->cache()->line(line());
    if (!thisLine->isValid()) {
      kWarning() << "Did not retrieve a valid layout for line " << line();
      return *this;
    }

    if (n >= 0) {
      for (int i = 0; i < n; i++) {
        if (m_column >= thisLine->length()) {
          // Have come to the end of a line
          if (line() >= doc()->lines() - 1)
            // Have come to the end of the document
            break;

          // Advance to the beginning of the next line
          m_column = 0;
          setLine(line() + 1);

          // Retrieve the next text range
          thisLine = m_vi->cache()->line(line());
          if (!thisLine->isValid()) {
            kWarning() << "Did not retrieve a valid layout for line " << line();
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
            kWarning() << "Did not retrieve a valid layout for line " << line();
            return *this;
          }

          // Finish going back to the end of the last line
          m_column = thisLine->length();

          continue;
        }

        if (m_column > thisLine->length())
          --m_column;
        else
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

void KateViewInternal::cursorPrevChar( bool sel )
{
  if ( ! m_view->wrapCursor() && m_cursor.column() == 0 )
    return;

  moveChar( KateViewInternal::left, sel );
}

void KateViewInternal::cursorNextChar( bool sel )
{
  moveChar( KateViewInternal::right, sel );
}

void KateViewInternal::wordPrev( bool sel )
{
  WrappingCursor c( this, m_cursor );

  // First we skip backwards all space.
  // Then we look up into which category the current position falls:
  // 1. a "word" character
  // 2. a "non-word" character (except space)
  // 3. the beginning of the line
  // and skip all preceding characters that fall into this class.
  // The code assumes that space is never part of the word character class.

  KateHighlighting* h = doc()->highlight();
  if( !c.atEdge( left ) ) {

    while( !c.atEdge( left ) && doc()->line( c.line() )[ c.column() - 1 ].isSpace() )
      --c;
  }
  if( c.atEdge( left ) )
  {
    --c;
  }
  else if( h->isInWord( doc()->line( c.line() )[ c.column() - 1 ] ) )
  {
    while( !c.atEdge( left ) && h->isInWord( doc()->line( c.line() )[ c.column() - 1 ] ) )
      --c;
  }
  else
  {
    while( !c.atEdge( left )
           && !h->isInWord( doc()->line( c.line() )[ c.column() - 1 ] )
           // in order to stay symmetric to wordLeft()
           // we must not skip space preceding a non-word sequence
           && !doc()->line( c.line() )[ c.column() - 1 ].isSpace() )
    {
      --c;
    }
  }

  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::wordNext( bool sel )
{
  WrappingCursor c( this, m_cursor );

  // We look up into which category the current position falls:
  // 1. a "word" character
  // 2. a "non-word" character (except space)
  // 3. the end of the line
  // and skip all following characters that fall into this class.
  // If the skipped characters are followed by space, we skip that too.
  // The code assumes that space is never part of the word character class.

  KateHighlighting* h = doc()->highlight();
  if( c.atEdge( right ) )
  {
    ++c;
  }
  else if( h->isInWord( doc()->line( c.line() )[ c.column() ] ) )
  {
    while( !c.atEdge( right ) && h->isInWord( doc()->line( c.line() )[ c.column() ] ) )
      ++c;
  }
  else
  {
    while( !c.atEdge( right )
           && !h->isInWord( doc()->line( c.line() )[ c.column() ] )
           // we must not skip space, because if that space is followed
           // by more non-word characters, we would skip them, too
           && !doc()->line( c.line() )[ c.column() ].isSpace() )
    {
      ++c;
    }
  }

  while( !c.atEdge( right ) && doc()->line( c.line() )[ c.column() ].isSpace() )
    ++c;

  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::moveEdge( KateViewInternal::Bias bias, bool sel )
{
  BoundedCursor c( this, m_cursor );
  c.toEdge( bias );
  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::home( bool sel )
{
  if (m_view->dynWordWrap() && currentLayout().startCol()) {
    // Allow us to go to the real start if we're already at the start of the view line
    if (m_cursor.column() != currentLayout().startCol()) {
      KTextEditor::Cursor c = currentLayout().start();
      updateSelection( c, sel );
      updateCursor( c );
      return;
    }
  }

  if( !doc()->config()->smartHome() ) {
    moveEdge( left, sel );
    return;
  }

  Kate::TextLine l = doc()->kateTextLine( m_cursor.line() );

  if (!l)
    return;

  KTextEditor::Cursor c = m_cursor;
  int lc = l->firstChar();

  if( lc < 0 || c.column() == lc ) {
    c.setColumn(0);
  } else {
    c.setColumn(lc);
  }

  updateSelection( c, sel );
  updateCursor( c, true );
}

void KateViewInternal::end( bool sel )
{
  KateTextLayout layout = currentLayout();

  if (m_view->dynWordWrap() && layout.wrap()) {
    // Allow us to go to the real end if we're already at the end of the view line
    if (m_cursor.column() < layout.endCol() - 1) {
      KTextEditor::Cursor c(m_cursor.line(), layout.endCol() - 1);
      updateSelection( c, sel );
      updateCursor( c );
      return;
    }
  }

  if( !doc()->config()->smartHome() ) {
    moveEdge( right, sel );
    return;
  }

  Kate::TextLine l = doc()->kateTextLine( m_cursor.line() );

  if (!l)
    return;

  // "Smart End", as requested in bugs #78258 and #106970
  if (m_cursor.column() == doc()->lineLength(m_cursor.line())) {
    KTextEditor::Cursor c = m_cursor;
    c.setColumn(l->lastChar() + 1);
    updateSelection(c, sel);
    updateCursor(c, true);
  } else {
    moveEdge(right, sel);
  }
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
    return cache()->textLayout(m_view->textFolding().visibleLineToLine(m_displayCursor.line() - 1), -1);
}

KateTextLayout KateViewInternal::nextLayout() const
{
  int currentViewLine = cache()->viewLine(m_cursor) + 1;

  if (currentViewLine >= cache()->line(m_cursor.line())->viewLineCount()) {
    currentViewLine = 0;
    return cache()->textLayout(m_view->textFolding().visibleLineToLine(m_displayCursor.line() + 1), currentViewLine);
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
    KTextEditor::Cursor ret(qMin((int)m_view->textFolding().visibleLines() - 1, virtualCursor.line() + offset), 0);

    if (ret.line() < 0)
      ret.setLine(0);

    if (keepX) {
      int realLine = m_view->textFolding().visibleLineToLine(ret.line());
      KateTextLayout t = cache()->textLayout(realLine, 0);
      Q_ASSERT(t.isValid());

      ret.setColumn(renderer()->xToCursor(t, m_preservedX, !m_view->wrapCursor()).column());
    }

    return ret;
  }

  KTextEditor::Cursor realCursor = virtualCursor;
  realCursor.setLine(m_view->textFolding().visibleLineToLine(m_view->textFolding().lineToVisibleLine(virtualCursor.line())));

  int cursorViewLine = cache()->viewLine(realCursor);

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
      Q_ASSERT(thisLine.virtualLine() == (int) m_view->textFolding().lineToVisibleLine(virtualCursor.line()));
      return KTextEditor::Cursor(virtualCursor.line(), thisLine.startCol());
    }

    virtualLine = virtualCursor.line() - 1;
  }

  currentOffset++;

  while (virtualLine >= 0 && virtualLine < (int)m_view->textFolding().visibleLines())
  {
    int realLine = m_view->textFolding().visibleLineToLine(virtualLine);
    KateLineLayoutPtr thisLine = cache()->line(realLine, virtualLine);
    if (!thisLine)
      break;

    for (int i = 0; i < thisLine->viewLineCount(); ++i) {
      if (offset == currentOffset) {
        KateTextLayout thisViewLine = thisLine->viewLine(i);

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
          KTextEditor::Cursor realCursor = toRealCursor(virtualCursor);
          KateTextLayout t = cache()->textLayout(realCursor);
          // renderer()->cursorToX(t, realCursor, !m_view->wrapCursor());

          realCursor = renderer()->xToCursor(thisViewLine, m_preservedX, !m_view->wrapCursor());
          ret.setColumn(realCursor.column());
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
    return KTextEditor::Cursor(m_view->textFolding().visibleLines() - 1, doc()->lineLength(m_view->textFolding().visibleLineToLine (m_view->textFolding().visibleLines() - 1)));
  else
    return KTextEditor::Cursor(0, 0);
}

int KateViewInternal::lineMaxCursorX(const KateTextLayout& range)
{
  if (!m_view->wrapCursor() && !range.wrap())
    return INT_MAX;

  int maxX = range.endX();

  if (maxX && range.wrap()) {
    QChar lastCharInLine = doc()->kateTextLine(range.line())->at(range.endCol() - 1);
    maxX -= renderer()->config()->fontMetrics().width(lastCharInLine);
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
  if(!sel && m_view->completionWidget()->isCompletionActive()) {
    m_view->completionWidget()->cursorUp();
    return;
  }

  if (m_displayCursor.line() == 0 && (!m_view->dynWordWrap() || cache()->viewLine(m_cursor) == 0))
    return;

  m_preserveX = true;

  KateTextLayout thisLine = currentLayout();
  // This is not the first line because that is already simplified out above
  KateTextLayout pRange = previousLayout();

  // Ensure we're in the right spot
  Q_ASSERT(m_cursor.line() == thisLine.line());
  Q_ASSERT(m_cursor.column() >= thisLine.startCol());
  Q_ASSERT(!thisLine.wrap() || m_cursor.column() < thisLine.endCol());

  KTextEditor::Cursor c = renderer()->xToCursor(pRange, m_preservedX, !m_view->wrapCursor());

  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::cursorDown(bool sel)
{
  if(!sel && m_view->completionWidget()->isCompletionActive()) {
    m_view->completionWidget()->cursorDown();
    return;
  }

  if ((m_displayCursor.line() >= m_view->textFolding().visibleLines() - 1) && (!m_view->dynWordWrap() || cache()->viewLine(m_cursor) == cache()->lastViewLine(m_cursor.line())))
    return;

  m_preserveX = true;

  KateTextLayout thisLine = currentLayout();
  // This is not the last line because that is already simplified out above
  KateTextLayout nRange = nextLayout();

  // Ensure we're in the right spot
  Q_ASSERT((m_cursor.line() == thisLine.line()) &&
      (m_cursor.column() >= thisLine.startCol()) &&
      (!thisLine.wrap() || m_cursor.column() < thisLine.endCol()));

  KTextEditor::Cursor c = renderer()->xToCursor(nRange, m_preservedX, !m_view->wrapCursor());

  updateSelection(c, sel);
  updateCursor(c);
}

void KateViewInternal::cursorToMatchingBracket( bool sel )
{
  KTextEditor::Cursor c = findMatchingBracket();

  if (c.isValid()) {
    updateSelection( c, sel );
    updateCursor( c );
  }
}

void KateViewInternal::topOfView( bool sel )
{
  KTextEditor::Cursor c = viewLineOffset(startPos(), m_minLinesVisible);
  updateSelection( toRealCursor(c), sel );
  updateCursor( toRealCursor(c) );
}

void KateViewInternal::bottomOfView( bool sel )
{
  KTextEditor::Cursor c = viewLineOffset(endPos(), -m_minLinesVisible);
  updateSelection( toRealCursor(c), sel );
  updateCursor( toRealCursor(c) );
}

// lines is the offset to scroll by
void KateViewInternal::scrollLines( int lines, bool sel )
{
  KTextEditor::Cursor c = viewLineOffset(m_displayCursor, lines, true);

  // Fix the virtual cursor -> real cursor
  c.setLine(m_view->textFolding().visibleLineToLine(c.line()));

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
  m_minLinesVisible = qMin(int((linesDisplayed() - 1)/2), m_autoCenterLines);
  if (updateView)
    KateViewInternal::updateView();
}

void KateViewInternal::pageUp( bool sel, bool half )
{
  if (m_view->isCompletionActive()) {
    m_view->completionWidget()->pageUp();
    return;
  }

  // remember the view line and x pos
  int viewLine = cache()->displayViewLine(m_displayCursor);
  bool atTop = startPos().atStartOfDocument();

  // Adjust for an auto-centering cursor
  int lineadj = m_minLinesVisible;

  int linesToScroll;
  if ( ! half )
    linesToScroll = -qMax( (linesDisplayed() - 1) - lineadj, 0 );
  else
    linesToScroll = -qMax( (linesDisplayed()/2 - 1) - lineadj, 0 );

  m_preserveX = true;

  if (!doc()->pageUpDownMovesCursor () && !atTop) {
    KTextEditor::Cursor newStartPos = viewLineOffset(startPos(), linesToScroll - 1);
    scrollPos(newStartPos);

    // put the cursor back approximately where it was
    KTextEditor::Cursor newPos = toRealCursor(viewLineOffset(newStartPos, viewLine, true));

    KateTextLayout newLine = cache()->textLayout(newPos);

    newPos = renderer()->xToCursor(newLine, m_preservedX, !m_view->wrapCursor());

    m_preserveX = true;
    updateSelection( newPos, sel );
    updateCursor(newPos);

  } else {
    scrollLines( linesToScroll, sel );
  }
}

void KateViewInternal::pageDown( bool sel ,bool half)
{
  if (m_view->isCompletionActive()) {
    m_view->completionWidget()->pageDown();
    return;
  }

  // remember the view line
  int viewLine = cache()->displayViewLine(m_displayCursor);
  bool atEnd = startPos() >= m_cachedMaxStartPos;

  // Adjust for an auto-centering cursor
  int lineadj = m_minLinesVisible;

  int linesToScroll;
  if ( ! half )
    linesToScroll = qMax( (linesDisplayed() - 1) - lineadj, 0 );
  else
    linesToScroll = qMax( (linesDisplayed()/2 - 1) - lineadj, 0 );

  m_preserveX = true;

  if (!doc()->pageUpDownMovesCursor () && !atEnd) {
    KTextEditor::Cursor newStartPos = viewLineOffset(startPos(), linesToScroll + 1);
    scrollPos(newStartPos);

    // put the cursor back approximately where it was
    KTextEditor::Cursor newPos = toRealCursor(viewLineOffset(newStartPos, viewLine, true));

    KateTextLayout newLine = cache()->textLayout(newPos);

    newPos = renderer()->xToCursor(newLine, m_preservedX, !m_view->wrapCursor());

    m_preserveX = true;
    updateSelection( newPos, sel );
    updateCursor(newPos);

  } else {
    scrollLines( linesToScroll, sel );
  }
}

int KateViewInternal::maxLen(int startLine)
{
  Q_ASSERT(!m_view->dynWordWrap());

  int displayLines = (m_view->height() / renderer()->lineHeight()) + 1;

  int maxLen = 0;

  for (int z = 0; z < displayLines; z++) {
    int virtualLine = startLine + z;

    if (virtualLine < 0 || virtualLine >= (int)m_view->textFolding().visibleLines())
      break;

    maxLen = qMax(maxLen, cache()->line(m_view->textFolding().visibleLineToLine(virtualLine))->width());
  }

  return maxLen;
}

bool KateViewInternal::columnScrollingPossible ()
{
  return !m_view->dynWordWrap() && m_columnScroll->isEnabled() && (m_columnScroll->maximum() > 0);
}

void KateViewInternal::top( bool sel )
{
  KTextEditor::Cursor newCursor(0, 0);

  newCursor = renderer()->xToCursor(cache()->textLayout(newCursor), m_preservedX, !m_view->wrapCursor());

  updateSelection( newCursor, sel );
  updateCursor( newCursor );
}

void KateViewInternal::bottom( bool sel )
{
  KTextEditor::Cursor newCursor(doc()->lastLine(), 0);

  newCursor = renderer()->xToCursor(cache()->textLayout(newCursor), m_preservedX, !m_view->wrapCursor());

  updateSelection( newCursor, sel );
  updateCursor( newCursor );
}

void KateViewInternal::top_home( bool sel )
{
  if (m_view->isCompletionActive()) {
    m_view->completionWidget()->top();
    return;
  }

  KTextEditor::Cursor c( 0, 0 );
  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::bottom_end( bool sel )
{
  if (m_view->isCompletionActive()) {
    m_view->completionWidget()->bottom();
    return;
  }

  KTextEditor::Cursor c( doc()->lastLine(), doc()->lineLength( doc()->lastLine() ) );
  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::updateSelection( const KTextEditor::Cursor& _newCursor, bool keepSel )
{
  KTextEditor::Cursor newCursor = _newCursor;
  if( keepSel )
  {
    if ( !m_view->selection() || (m_selectAnchor.line() == -1)
        //don't kill the selection if we have a persistent selection and
        //the cursor is inside or at the boundaries of the selected area
         || (m_view->config()->persistentSelection()
             && !(m_view->selectionRange().contains(m_cursor)
                   || m_view->selectionRange().boundaryAtCursor(m_cursor))) )
    {
      m_selectAnchor = m_cursor;
      setSelection( KTextEditor::Range(m_cursor, newCursor) );
    }
    else
    {
      bool doSelect = true;
      switch (m_selectionMode)
      {
        case Word:
        {
          // Restore selStartCached if needed. It gets nuked by
          // viewSelectionChanged if we drag the selection into non-existence,
          // which can legitimately happen if a shift+DC selection is unable to
          // set a "proper" (i.e. non-empty) cached selection, e.g. because the
          // start was on something that isn't a word. Word select mode relies
          // on the cached selection being set properly, even if it is empty
          // (i.e. selStartCached == selEndCached).
          if ( !m_selectionCached.isValid() )
            m_selectionCached.start() = m_selectionCached.end();

          int c;
          if ( newCursor > m_selectionCached.start() )
          {
            m_selectAnchor = m_selectionCached.start();

            Kate::TextLine l = doc()->kateTextLine( newCursor.line() );

            c = newCursor.column();
            if ( c > 0 && doc()->highlight()->isInWord( l->at( c-1 ) ) ) {
              for ( ; c < l->length(); c++ )
                if ( !doc()->highlight()->isInWord( l->at( c ) ) )
                  break;
            }

            newCursor.setColumn( c );
          }
          else if ( newCursor < m_selectionCached.start() )
          {
            m_selectAnchor = m_selectionCached.end();

            Kate::TextLine l = doc()->kateTextLine( newCursor.line() );

            c = newCursor.column();
            if ( c > 0 && c < doc()->lineLength( newCursor.line() )
                 && doc()->highlight()->isInWord( l->at( c ) )
                 && doc()->highlight()->isInWord( l->at( c-1 ) ) ) {
              for ( c -= 2; c >= 0; c-- )
                if ( !doc()->highlight()->isInWord( l->at( c ) ) )
                  break;
              newCursor.setColumn( c+1 );
            }
          }
          else
            doSelect = false;

        }
        break;
        case Line:
          if ( !m_selectionCached.isValid() ) {
            m_selectionCached = KTextEditor::Range(endLine(), 0, endLine(), 0);
          }
          if ( newCursor.line() > m_selectionCached.start().line() )
          {
            if (newCursor.line() + 1 >= doc()->lines() )
              newCursor.setColumn( doc()->line( newCursor.line() ).length() );
            else
              newCursor.setPosition( newCursor.line() + 1, 0 );
            // Grow to include the entire line
            m_selectAnchor = m_selectionCached.start();
            m_selectAnchor.setColumn( 0 );
          }
          else if ( newCursor.line() < m_selectionCached.start().line() )
          {
            newCursor.setColumn( 0 );
            // Grow to include entire line
            m_selectAnchor = m_selectionCached.end();
            if ( m_selectAnchor.column() > 0 )
            {
              if ( m_selectAnchor.line()+1 >= doc()->lines() )
                m_selectAnchor.setColumn( doc()->line( newCursor.line() ).length() );
              else
                m_selectAnchor.setPosition( m_selectAnchor.line() + 1, 0 );
            }
          }
          else // same line, ignore
            doSelect = false;
        break;
        case Mouse:
        {
          if ( !m_selectionCached.isValid() )
            break;

          if ( newCursor > m_selectionCached.end() )
            m_selectAnchor = m_selectionCached.start();
          else if ( newCursor < m_selectionCached.start() )
            m_selectAnchor = m_selectionCached.end();
          else
            doSelect = false;
        }
        break;
        default: /* nothing special to do */;
      }

      if ( doSelect )
        setSelection( KTextEditor::Range(m_selectAnchor, newCursor) );
      else if ( m_selectionCached.isValid() ) // we have a cached selection, so we restore that
        setSelection( m_selectionCached );
    }

    m_selChangedByUser = true;
  }
  else if ( !m_view->config()->persistentSelection() )
  {
    m_view->clearSelection();

    m_selectionCached = KTextEditor::Range::invalid();
    m_selectAnchor = KTextEditor::Cursor::invalid();
  }

#ifndef QT_NO_ACCESSIBILITY
#if QT_VERSION >= 0x040800
  QAccessible::updateAccessibility(this, 0, QAccessible::TextSelectionChanged);
#endif
#endif
}

void KateViewInternal::setCaretStyle( KateRenderer::caretStyles style, bool repaint )
{
  renderer()->setCaretStyle( style );

  if ( repaint ) {
    if ( m_cursorTimer.isActive() &&
         KApplication::cursorFlashTime() > 0 ) {
      m_cursorTimer.start( KApplication::cursorFlashTime() / 2 );
    }
    renderer()->setDrawCaret(true);
    paintCursor();
  }
}

void KateViewInternal::setSelection( const KTextEditor::Range &range )
{
  disconnect(m_view, SIGNAL(selectionChanged(KTextEditor::View*)), this, SLOT(viewSelectionChanged()));
  m_view->setSelection(range);
  connect(m_view, SIGNAL(selectionChanged(KTextEditor::View*)), this, SLOT(viewSelectionChanged()));
}

void KateViewInternal::moveCursorToSelectionEdge()
{
  if (!m_view->selection())
    return;

  int tmp = m_minLinesVisible;
  m_minLinesVisible = 0;

  if ( m_view->selectionRange().start() < m_selectAnchor )
    updateCursor( m_view->selectionRange().start() );
  else
    updateCursor( m_view->selectionRange().end() );

  m_minLinesVisible = tmp;
}

void KateViewInternal::updateCursor( const KTextEditor::Cursor& newCursor, bool force, bool center, bool calledExternally )
{
  if ( !force && (m_cursor.toCursor() == newCursor) )
  {
    m_displayCursor = toVirtualCursor(newCursor);
    if ( !m_madeVisible && m_view == doc()->activeView() )
    {
      // unfold if required
      m_view->textFolding().ensureLineIsVisible ( newCursor.line() );

      makeVisible ( m_displayCursor, m_displayCursor.column(), false, center, calledExternally );
    }

    return;
  }

  if (m_cursor.line() != newCursor.line()) {
    m_leftBorder->updateViRelLineNumbers();
  }

  // unfold if required
  m_view->textFolding().ensureLineIsVisible ( newCursor.line() );

  KTextEditor::Cursor oldDisplayCursor = m_displayCursor;

  m_displayCursor = toVirtualCursor(newCursor);
  m_cursor.setPosition( newCursor );

  if ( m_view == doc()->activeView() )
    makeVisible ( m_displayCursor, m_displayCursor.column(), false, center, calledExternally );

  updateBracketMarks();

  // It's efficient enough to just tag them both without checking to see if they're on the same view line
/*  kdDebug()<<"oldDisplayCursor:"<<oldDisplayCursor<<endl;
  kdDebug()<<"m_displayCursor:"<<m_displayCursor<<endl;*/
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
  if (m_preserveX)
    m_preserveX = false;
  else
    m_preservedX = renderer()->cursorToX(cache()->textLayout(m_cursor), m_cursor, !m_view->wrapCursor());

  //kDebug(13030) << "m_preservedX: " << m_preservedX << " (was "<< oldmaxx << "), m_cursorX: " << m_cursorX;
  //kDebug(13030) << "Cursor now located at real " << cursor.line << "," << cursor.col << ", virtual " << m_displayCursor.line << ", " << m_displayCursor.col << "; Top is " << startLine() << ", " << startPos().col;

  cursorMoved();

  updateDirty(); //paintText(0, 0, width(), height(), true);

  emit m_view->cursorPositionChanged(m_view, m_cursor);
}

void KateViewInternal::updateBracketMarkAttributes()
{
  KTextEditor::Attribute::Ptr bracketFill = KTextEditor::Attribute::Ptr(new KTextEditor::Attribute());
  bracketFill->setBackground(m_view->m_renderer->config()->highlightedBracketColor());
  bracketFill->setBackgroundFillWhitespace(false);
  if (QFontInfo(renderer()->currentFont()).fixedPitch()) {
    // make font bold only for fixed fonts, otherwise text jumps around
    bracketFill->setFontBold();
  }

  m_bmStart->setAttribute(bracketFill);
  m_bmEnd->setAttribute(bracketFill);

  if (m_view->m_renderer->config()->showWholeBracketExpression()) {
    KTextEditor::Attribute::Ptr expressionFill = KTextEditor::Attribute::Ptr(new KTextEditor::Attribute());
    expressionFill->setBackground(m_view->m_renderer->config()->highlightedBracketColor());
    expressionFill->setBackgroundFillWhitespace(false);

    m_bm->setAttribute(expressionFill);
  } else {
    m_bm->setAttribute(KTextEditor::Attribute::Ptr(new KTextEditor::Attribute()));
  }
}

void KateViewInternal::updateBracketMarks()
{
  // add some limit to this, this is really endless on big files without limit
  int maxLines = 5000;
  KTextEditor::Range newRange;
  doc()->newBracketMark( m_cursor, newRange, maxLines );

  // new range valid, then set ranges to it
  if (newRange.isValid ()) {
    if (m_bm->toRange() == newRange) {
      return;
    }

    // modify full range
    m_bm->setRange (newRange);

    // modify start and end ranges
    m_bmStart->setRange (KTextEditor::Range (m_bm->start(), KTextEditor::Cursor (m_bm->start().line(), m_bm->start().column() + 1)));
    m_bmEnd->setRange (KTextEditor::Range (m_bm->end(), KTextEditor::Cursor (m_bm->end().line(), m_bm->end().column() + 1)));

    // flash matching bracket
    if (!renderer()->config()->animateBracketMatching()) {
      return;
    }
    const KTextEditor::Cursor flashPos = (m_cursor == m_bmStart->start() || m_cursor == m_bmStart->end()) ? m_bmEnd->start() : m_bm->start();
    if (flashPos != m_bmLastFlashPos->toCursor()) {
      m_bmLastFlashPos->setPosition(flashPos);

      KTextEditor::Attribute::Ptr attribute = doc()->attributeAt(flashPos);
      attribute->setBackground(m_view->m_renderer->config()->highlightedBracketColor());
      attribute->setFontBold(m_bmStart->attribute()->fontBold());

      flashChar(flashPos, attribute);
    }
    return;
  }

  // new range was invalid
  m_bm->setRange (KTextEditor::Range::invalid());
  m_bmStart->setRange (KTextEditor::Range::invalid());
  m_bmEnd->setRange (KTextEditor::Range::invalid());
  m_bmLastFlashPos->setPosition (KTextEditor::Cursor::invalid());
}

bool KateViewInternal::tagLine(const KTextEditor::Cursor& virtualCursor)
{
  // FIXME may be a more efficient way for this
  if ((int)m_view->textFolding().visibleLineToLine(virtualCursor.line()) > doc()->lastLine())
    return false;
  // End FIXME

  int viewLine = cache()->displayViewLine(virtualCursor, true);
  if (viewLine >= 0 && viewLine < cache()->viewCacheLineCount()) {
    cache()->viewLine(viewLine).setDirty();
    m_leftBorder->update (0, lineToY(viewLine), m_leftBorder->width(), renderer()->lineHeight());
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
    cache()->relayoutLines(start.line(), end.line());

    //kDebug(13030)<<"realLines is true";
    start = toVirtualCursor(start);
    end = toVirtualCursor(end);

  } else {
    cache()->relayoutLines(toRealCursor(start).line(), toRealCursor(end).line());
  }

  if (end.line() < startLine())
  {
    //kDebug(13030)<<"end<startLine";
    return false;
  }
  // Used to be > endLine(), but cache may not be valid when checking, so use a
  // less optimal but still adequate approximation (potential overestimation but minimal performance difference)
  if (start.line() > startLine() + cache()->viewCacheLineCount())
  {
    //kDebug(13030)<<"start> endLine"<<start<<" "<<(endLine());
    return false;
  }

  cache()->updateViewCache(startPos());

  //kDebug(13030) << "tagLines( [" << start << "], [" << end << "] )";

  bool ret = false;

  for (int z = 0; z < cache()->viewCacheLineCount(); z++)
  {
    KateTextLayout& line = cache()->viewLine(z);
    if ((line.virtualLine() > start.line() || (line.virtualLine() == start.line() && line.endCol() >= start.column() && start.column() != -1)) &&
        (line.virtualLine() < end.line() || (line.virtualLine() == end.line() && (line.startCol() <= end.column() || end.column() == -1)))) {
      ret = true;
      break;
      //kDebug(13030) << "Tagged line " << line.line();
    }
  }

  if (!m_view->dynWordWrap())
  {
    int y = lineToY( start.line() );
    // FIXME is this enough for when multiple lines are deleted
    int h = (end.line() - start.line() + 2) * renderer()->lineHeight();
    if (end.line() >= m_view->textFolding().visibleLines() - 1)
      h = height();

    m_leftBorder->update (0, y, m_leftBorder->width(), h);
  }
  else
  {
    // FIXME Do we get enough good info in editRemoveText to optimize this more?
    //bool justTagged = false;
    for (int z = 0; z < cache()->viewCacheLineCount(); z++)
    {
      KateTextLayout& line = cache()->viewLine(z);
      if (!line.isValid() ||
          ((line.virtualLine() > start.line() || (line.virtualLine() == start.line() && line.endCol() >= start.column() && start.column() != -1)) &&
           (line.virtualLine() < end.line() || (line.virtualLine() == end.line() && (line.startCol() <= end.column() || end.column() == -1)))))
      {
        //justTagged = true;
        m_leftBorder->update (0, z * renderer()->lineHeight(), m_leftBorder->width(), m_leftBorder->height());
        break;
      }
      /*else if (justTagged)
      {
        justTagged = false;
        leftBorder->update (0, z * doc()->viewFont.fontHeight, leftBorder->width(), doc()->viewFont.fontHeight);
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
  // clear the cache...
  cache()->clear ();

  m_leftBorder->updateFont();
  m_leftBorder->update();
}

void KateViewInternal::paintCursor()
{
  if (tagLine(m_displayCursor))
    updateDirty(); //paintText (0,0,width(), height(), true);
}

// Point in content coordinates
void KateViewInternal::placeCursor( const QPoint& p, bool keepSelection, bool updateSelection )
{
  KateTextLayout thisLine = yToKateTextLayout(p.y());
  KTextEditor::Cursor c;

  if (!thisLine.isValid()) // probably user clicked below the last line -> use the last line
    thisLine = cache()->textLayout(doc()->lines() - 1, -1);

  c = renderer()->xToCursor(thisLine, startX() + p.x(), !m_view->wrapCursor());

  if (c.line () < 0 || c.line() >= doc()->lines()) {
    return;
  }

  if (updateSelection)
    KateViewInternal::updateSelection( c, keepSelection );

  int tmp = m_minLinesVisible;
  m_minLinesVisible = 0;
  updateCursor( c );
  m_minLinesVisible = tmp;

  if (updateSelection && keepSelection)
    moveCursorToSelectionEdge();
}

// Point in content coordinates
bool KateViewInternal::isTargetSelected( const QPoint& p )
{
  const KateTextLayout& thisLine = yToKateTextLayout(p.y());
  if (!thisLine.isValid())
    return false;

  return m_view->cursorSelected(renderer()->xToCursor(thisLine, startX() + p.x(), !m_view->wrapCursor()));
}

//BEGIN EVENT HANDLING STUFF

bool KateViewInternal::eventFilter( QObject *obj, QEvent *e )
{
  if (obj == m_lineScroll)
  {
    // the second condition is to make sure a scroll on the vertical bar doesn't cause a horizontal scroll ;)
    if (e->type() == QEvent::Wheel && m_lineScroll->minimum() != m_lineScroll->maximum())
    {
      wheelEvent((QWheelEvent*)e);
      return true;
    }

    // continue processing
    return QWidget::eventFilter( obj, e );
  }

  switch( e->type() )
  {
    case QEvent::ChildAdded:
    case QEvent::ChildRemoved: {
      QChildEvent* c = static_cast<QChildEvent*>(e);
      if (c->added()) {
        c->child()->installEventFilter(this);
        /*foreach (QWidget* child, c->child()->findChildren<QWidget*>())
          child->installEventFilter(this);*/

      } else if (c->removed()) {
        c->child()->removeEventFilter(this);

        /*foreach (QWidget* child, c->child()->findChildren<QWidget*>())
          child->removeEventFilter(this);*/
      }
    } break;

    case QEvent::ShortcutOverride:
    {
      QKeyEvent *k = static_cast<QKeyEvent *>(e);

      if (k->key() == Qt::Key_Escape && k->modifiers() == Qt::NoModifier) {
        if (m_view->isCompletionActive()) {
          m_view->abortCompletion();
          k->accept();
          //kDebug() << obj << "shortcut override" << k->key() << "aborting completion";
          return true;
        } else if (m_view->bottomViewBar()->isVisible()) {
          m_view->bottomViewBar()->hideCurrentBarWidget();
          k->accept();
          //kDebug() << obj << "shortcut override" << k->key() << "closing view bar";
          return true;
        } else if (!m_view->config()->persistentSelection() && m_view->selection()) {
          if (!m_view->viInputMode()) // Vim mode handles clearing selections itself.
          {
            m_view->clearSelection();
          }
          k->accept();
          //kDebug() << obj << "shortcut override" << k->key() << "clearing selection";
          return true;
        }
      }

      // if vi input mode key stealing is on, override kate shortcuts
      if (m_view->viInputMode() && m_view->viInputModeStealKeys() &&  ( m_view->getCurrentViMode() != InsertMode ||
          k->modifiers() == Qt::ControlModifier || k->key() == Qt::Key_Insert ) ) {
        k->accept();
        return true;
      }

    } break;

    case QEvent::KeyPress:
    {
      QKeyEvent *k = static_cast<QKeyEvent *>(e);

      // Override all other single key shortcuts which do not use a modifier other than Shift
      if (obj == this && (!k->modifiers() || k->modifiers() == Qt::ShiftModifier)) {
        keyPressEvent( k );
        if (k->isAccepted()) {
          //kDebug() << obj << "shortcut override" << k->key() << "using keystroke";
          return true;
        }
      }

      //kDebug() << obj << "shortcut override" << k->key() << "ignoring";
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
      if (isVisible()) {
        doc()->ignoreModifiedOnDiskOnce();
      }
      break;

    default:
      break;
  }

  return QWidget::eventFilter( obj, e );
}

void KateViewInternal::keyPressEvent( QKeyEvent* e )
{
  if( e->key() == Qt::Key_Left && e->modifiers() == Qt::AltModifier ) {
    m_view->emitNavigateLeft();
    e->setAccepted(true);
    return;
  }
  if( e->key() == Qt::Key_Right && e->modifiers() == Qt::AltModifier ) {
    m_view->emitNavigateRight();
    e->setAccepted(true);
    return;
  }
  if( e->key() == Qt::Key_Up && e->modifiers() == Qt::AltModifier ) {
    m_view->emitNavigateUp();
    e->setAccepted(true);
    return;
  }
  if( e->key() == Qt::Key_Down && e->modifiers() == Qt::AltModifier ) {
    m_view->emitNavigateDown();
    e->setAccepted(true);
    return;
  }
  if( e->key() == Qt::Key_Return && e->modifiers() == Qt::AltModifier ) {
    m_view->emitNavigateAccept();
    e->setAccepted(true);
    return;
  }
  if( e->key() == Qt::Key_Backspace && e->modifiers() == Qt::AltModifier ) {
    m_view->emitNavigateBack();
    e->setAccepted(true);
    return;
  }

  if( e->key() == Qt::Key_Alt && m_view->completionWidget()->isCompletionActive() ) {
    m_completionItemExpanded = m_view->completionWidget()->toggleExpanded(true);
    m_view->completionWidget()->resetHadNavigation();
    m_altDownTime = QTime::currentTime();
  }

  // Note: AND'ing with <Shift> is a quick hack to fix Key_Enter
  const int key = e->key() | (e->modifiers() & Qt::ShiftModifier);

  if (m_view->isCompletionActive()
     && !m_view->viInputMode() /* Vi input mode needs to handle completion keypresses itself for e.g. mappings */)
  {
    if( key == Qt::Key_Enter || key == Qt::Key_Return ) {
      m_view->completionWidget()->execute();
      e->accept();
      return;
    }
  }

  if ( m_view->viInputMode() ) {
    if ( getViInputModeManager()->getCurrentViMode() == InsertMode
        || getViInputModeManager()->getCurrentViMode() == ReplaceMode ) {
      if ( getViInputModeManager()->handleKeypress( e ) ) {
        return;
      } else if ( e->modifiers() != Qt::NoModifier && e->modifiers() != Qt::ShiftModifier ) {
        // re-post key events not handled if they have a modifier other than shift
        QEvent *copy = new QKeyEvent ( e->type(), e->key(), e->modifiers(), e->text(),
            e->isAutoRepeat(), e->count() );
        QCoreApplication::postEvent( parent(), copy );
      }
    } else { // !InsertMode
      if ( !getViInputModeManager()->handleKeypress( e ) ) {
        // we didn't need that keypress, un-steal it :-)
        QEvent *copy = new QKeyEvent ( e->type(), e->key(), e->modifiers(), e->text(),
            e->isAutoRepeat(), e->count() );
        QCoreApplication::postEvent( parent(), copy );
      }
      m_view->updateViModeBarCmd();
      return;
    }
  }

  if( !doc()->isReadWrite() )
  {
    e->ignore();
    return;
  }

  if ((key == Qt::Key_Return) || (key == Qt::Key_Enter) || (key == Qt::SHIFT + Qt::Key_Return) || (key == Qt::SHIFT + Qt::Key_Enter))
  {
    doReturn();
    e->accept();
    return;
  }

  if (key == Qt::Key_Backspace || key == Qt::SHIFT + Qt::Key_Backspace)
  {
    //m_view->backspace();
    e->accept();

    return;
  }

  if  (key == Qt::Key_Tab || key == Qt::SHIFT+Qt::Key_Backtab || key == Qt::Key_Backtab)
  {
    if(m_view->completionWidget()->isCompletionActive())
    {
      e->accept();
      m_view->completionWidget()->tab(key != Qt::Key_Tab);
      return;
    }

    if( key == Qt::Key_Tab )
    {
      uint tabHandling = doc()->config()->tabHandling();
      // convert tabSmart into tabInsertsTab or tabIndents:
      if (tabHandling == KateDocumentConfig::tabSmart)
      {
        // multiple lines selected
        if (m_view->selection() && !m_view->selectionRange().onSingleLine())
        {
          tabHandling = KateDocumentConfig::tabIndents;
        }

        // otherwise: take look at cursor position
        else
        {
          // if the cursor is at or before the first non-space character
          // or on an empty line,
          // Tab indents, otherwise it inserts a tab character.
          Kate::TextLine line = doc()->kateTextLine( m_cursor.line() );
          int first = line->firstChar();
          if (first < 0 || m_cursor.column() <= first)
            tabHandling = KateDocumentConfig::tabIndents;
          else
            tabHandling = KateDocumentConfig::tabInsertsTab;
        }
      }

      if (tabHandling == KateDocumentConfig::tabInsertsTab)
        doc()->typeChars( m_view, QString("\t") );
      else
        doc()->indent( m_view->selection() ? m_view->selectionRange() : KTextEditor::Range(m_cursor.line(), 0, m_cursor.line(), 0), 1 );

      e->accept();

      return;
    }
    else if (doc()->config()->tabHandling() != KateDocumentConfig::tabInsertsTab)
    {
      // key == Qt::SHIFT+Qt::Key_Backtab || key == Qt::Key_Backtab
      doc()->indent( m_view->selection() ? m_view->selectionRange() : KTextEditor::Range(m_cursor.line(), 0, m_cursor.line(), 0), -1 );
      e->accept();

      return;
    }
  }

  if ( !(e->modifiers() & Qt::ControlModifier) && !e->text().isEmpty() && doc()->typeChars ( m_view, e->text() ) )
  {
    e->accept();

    return;
  }

  // allow composition of AltGr + (q|2|3) on windows
  static const int altGR = Qt::ControlModifier | Qt::AltModifier;
  if( (e->modifiers() & altGR) == altGR && !e->text().isEmpty() && doc()->typeChars ( m_view, e->text() ) )
  {
    e->accept();

    return;
  }

  e->ignore();
}

void KateViewInternal::keyReleaseEvent( QKeyEvent* e )
{
  if( e->key() == Qt::Key_Alt && m_view->completionWidget()->isCompletionActive() && ((m_completionItemExpanded && (m_view->completionWidget()->hadNavigation() || m_altDownTime.msecsTo(QTime::currentTime()) > 300)) || (!m_completionItemExpanded && !m_view->completionWidget()->hadNavigation())) ) {

    m_view->completionWidget()->toggleExpanded(false, true);
  }

  if ((e->modifiers() & Qt::SHIFT) == Qt::SHIFT)
  {
    m_shiftKeyPressed = true;
  }
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

  if ( doc()->browserView() )
  {
    m_view->contextMenuEvent( e );
    return;
  }

  if ( e->reason() == QContextMenuEvent::Keyboard )
  {
    makeVisible( m_displayCursor, 0 );
    p = cursorCoordinates(false);
    p.rx() -= startX();
  }
  else if ( ! m_view->selection() || m_view->config()->persistentSelection() )
    placeCursor( e->pos() );

  // popup is a qguardedptr now
  if (m_view->contextMenu()) {
    m_view->spellingMenu()->setUseMouseForMisspelledRange((e->reason() == QContextMenuEvent::Mouse));
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

          if ( e->modifiers() & Qt::ShiftModifier )
          {
            updateSelection( m_cursor, true );
          }
          else
          {
            m_view->selectLine( m_cursor );
            if (m_view->selection())
              m_selectAnchor = m_view->selectionRange().start();
          }

          if (m_view->selection())
            QApplication::clipboard()->setText(m_view->selectionText (), QClipboard::Selection);

          // Keep the line at the select anchor selected during further
          // mouse selection
          if ( m_selectAnchor.line() > m_view->selectionRange().start().line() )
          {
            // Preserve the last selected line
            if ( m_selectAnchor == m_view->selectionRange().end() && m_selectAnchor.column() == 0 )
              m_selectionCached.start().setPosition( m_selectAnchor.line()-1, 0 );
            else
              m_selectionCached.start().setPosition( m_selectAnchor.line(), 0 );
            m_selectionCached.end() = m_view->selectionRange().end();
          }
          else
          {
            // Preserve the first selected line
            m_selectionCached.start() = m_view->selectionRange().start();
            if ( m_view->selectionRange().end().line() > m_view->selectionRange().start().line() )
              m_selectionCached.end().setPosition( m_view->selectionRange().start().line()+1, 0 );
            else
              m_selectionCached.end() = m_view->selectionRange().end();
          }

          moveCursorToSelectionEdge();

          m_scrollX = 0;
          m_scrollY = 0;
          m_scrollTimer.start (50);

          e->accept();
          return;
        }
        else if ( m_selectionMode == Default )
        {
          m_selectionMode = Mouse;
        }

        // request the software keyboard, if any
        if ( e->button() == Qt::LeftButton && qApp->autoSipEnabled() )
        {
          QStyle::RequestSoftwareInputPanel behavior = QStyle::RequestSoftwareInputPanel( style()->styleHint( QStyle::SH_RequestSoftwareInputPanel ) );
          if ( hasFocus() || behavior == QStyle::RSIP_OnMouseClick )
          {
            QEvent event( QEvent::RequestSoftwareInputPanel );
            QApplication::sendEvent( this, &event );
          }
        }

        if ( e->modifiers() & Qt::ShiftModifier )
        {
          if ( !m_selectAnchor.isValid() )
            m_selectAnchor = m_cursor;
        }
        else
        {
          m_selectionCached = KTextEditor::Range::invalid();
        }

        if( !(e->modifiers() & Qt::ShiftModifier) && isTargetSelected( e->pos() ) )
        {
          m_dragInfo.state = diPending;
          m_dragInfo.start = e->pos();
        }
        else
        {
          m_dragInfo.state = diNone;

          if ( e->modifiers() & Qt::ShiftModifier )
          {
            placeCursor( e->pos(), true, false );
            if ( m_selectionCached.start().isValid() )
            {
              if ( m_cursor.toCursor() < m_selectionCached.start() )
                m_selectAnchor = m_selectionCached.end();
              else
                m_selectAnchor = m_selectionCached.start();
            }
            setSelection( KTextEditor::Range( m_selectAnchor, m_cursor ) );
          }
          else
          {
            placeCursor( e->pos() );
          }

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

      if ( e->modifiers() & Qt::ShiftModifier )
      {
        KTextEditor::Range oldSelection = m_view->selectionRange();

        // Now select the word under the select anchor
        int cs, ce;
        Kate::TextLine l = doc()->kateTextLine( m_selectAnchor.line() );

        ce = m_selectAnchor.column();
        if ( ce > 0 && doc()->highlight()->isInWord( l->at(ce) ) ) {
          for (; ce < l->length(); ce++ )
            if ( !doc()->highlight()->isInWord( l->at(ce) ) )
              break;
        }

        cs = m_selectAnchor.column() - 1;
        if ( cs < doc()->lineLength( m_selectAnchor.line() )
              && doc()->highlight()->isInWord( l->at(cs) ) ) {
          for ( cs--; cs >= 0; cs-- )
            if ( !doc()->highlight()->isInWord( l->at(cs) ) )
              break;
        }

        // ...and keep it selected
        if (cs+1 < ce)
        {
          m_selectionCached.start().setPosition( m_selectAnchor.line(), cs+1 );
          m_selectionCached.end().setPosition( m_selectAnchor.line(), ce );
        }
        else
        {
          m_selectionCached.start() = m_selectAnchor;
          m_selectionCached.end() = m_selectAnchor;
        }
        // Now word select to the mouse cursor
        placeCursor( e->pos(), true );
      }
      else
      {
        // first clear the selection, otherwise we run into bug #106402
        // ...and set the cursor position, for the same reason (otherwise there
        // are *other* idiosyncrasies we can't fix without reintroducing said
        // bug)
        // Parameters: don't redraw, and don't emit selectionChanged signal yet
        m_view->clearSelection( false, false );
        placeCursor( e->pos() );
        m_view->selectWord( m_cursor );
        cursorToMatchingBracket(true);

        if (m_view->selection())
        {
          m_selectAnchor = m_view->selectionRange().start();
          m_selectionCached = m_view->selectionRange();
        }
        else
        {
          m_selectAnchor = m_cursor;
          m_selectionCached = KTextEditor::Range(m_cursor, m_cursor);
        }
      }

      // Move cursor to end (or beginning) of selected word
      if (m_view->selection())
        QApplication::clipboard()->setText( m_view->selectionText(), QClipboard::Selection );

      moveCursorToSelectionEdge();
      m_possibleTripleClick = true;
      QTimer::singleShot ( QApplication::doubleClickInterval(), this, SLOT(tripleClickTimeout()) );

      m_scrollX = 0;
      m_scrollY = 0;

      m_scrollTimer.start (50);

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
        if (m_view->selection()) {
          QApplication::clipboard()->setText(m_view->selectionText (), QClipboard::Selection);
        }
        moveCursorToSelectionEdge();

        m_selChangedByUser = false;
      }

      if (m_dragInfo.state == diPending)
        placeCursor( e->pos(), e->modifiers() & Qt::ShiftModifier );
      else if (m_dragInfo.state == diNone)
        m_scrollTimer.stop ();

      m_dragInfo.state = diNone;

      e->accept ();
      break;

    case Qt::MidButton:
      placeCursor( e->pos() );

      if( doc()->isReadWrite() ) {
        QString clipboard = QApplication::clipboard()->text(QClipboard::Selection);
        m_view->paste( &clipboard );
      }

      e->accept ();
      break;

    default:
      e->ignore ();
      break;
  }
}

void KateViewInternal::leaveEvent( QEvent* )
{
  m_textHintTimer.stop();

  // fix bug 194452, scrolling keeps going if you scroll via mouse drag and press and other mouse
  // button outside the view area
  if (m_dragInfo.state == diNone)
    m_scrollTimer.stop ();
}

KTextEditor::Cursor KateViewInternal::coordinatesToCursor(const QPoint& _coord, bool includeBorder) const
{
  QPoint coord(_coord);

  KTextEditor::Cursor ret = KTextEditor::Cursor::invalid();

  if (includeBorder) coord.rx() -= m_leftBorder->width();
  coord.rx() += startX();

  const KateTextLayout& thisLine = yToKateTextLayout(coord.y());
  if (thisLine.isValid())
    ret = renderer()->xToCursor(thisLine, coord.x(), !m_view->wrapCursor());

  if (ret.column() == view()->document()->lineLength(ret.line())) {
    // The cursor is beyond the end of the line; in that case the renderer
    // gives the index of the character behind the last one.
    return KTextEditor::Cursor::invalid();
  }

  return ret;
}

void KateViewInternal::mouseMoveEvent( QMouseEvent* e )
{
  KTextEditor::Cursor newPosition = coordinatesToCursor (e->pos(), false);
  if (newPosition != m_mouse) {
    m_mouse = newPosition;
    mouseMoved();
  }

  if( e->buttons() & Qt::LeftButton )
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
    else if (m_dragInfo.state == diDragging)
    {
      // Don't do anything after a canceled drag until the user lets go of
      // the mouse button!
      return;
    }

    m_mouseX = e->x();
    m_mouseY = e->y();

    m_scrollX = 0;
    m_scrollY = 0;
    int d = renderer()->lineHeight();

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
        m_mouseCursor = Qt::ArrowCursor;
        setCursor(m_mouseCursor);
      }
    } else {
      // normal text cursor
      if (m_mouseCursor != Qt::IBeamCursor) {
        m_mouseCursor = Qt::IBeamCursor;
        setCursor(m_mouseCursor);
      }
    }
    //We need to check whether the mouse position is actually within the widget,
    //because other widgets like the icon border forward their events to this,
    //and we will create invalid text hint requests if we don't check
    if (m_textHintEnabled && geometry().contains(parentWidget()->mapFromGlobal(e->globalPos())))
    {
      if ( QToolTip::isVisible() ) {
        QToolTip::hideText();
      }
      m_textHintTimer.start(m_textHintTimeout);
      m_textHintPos = e->pos();
    }
  }
}

void KateViewInternal::updateDirty( )
{
  uint h = renderer()->lineHeight();

  int currentRectStart = -1;
  int currentRectEnd = -1;

  QRegion updateRegion;

  {
    for (int i = 0; i < cache()->viewCacheLineCount(); ++i) {
      if (cache()->viewLine(i).isDirty()) {
        if (currentRectStart == -1) {
          currentRectStart = h * i;
          currentRectEnd = h;
        } else {
          currentRectEnd += h;
        }

      } else if (currentRectStart != -1) {
        updateRegion += QRect(0, currentRectStart, width(), currentRectEnd);
        currentRectStart = -1;
        currentRectEnd = -1;
      }
    }
  }


  if (currentRectStart != -1)
    updateRegion += QRect(0, currentRectStart, width(), currentRectEnd);

  if (!updateRegion.isEmpty()) {
    if (debugPainting) kDebug( 13030 ) << k_funcinfo << "Update dirty region " << updateRegion;
    update(updateRegion);
  }
}

void KateViewInternal::hideEvent(QHideEvent* e)
{
  Q_UNUSED(e);
  if(m_view->isCompletionActive())
    m_view->completionWidget()->abortCompletion();
}

void KateViewInternal::paintEvent(QPaintEvent *e)
{
  if (debugPainting) kDebug (13030) << "GOT PAINT EVENT: Region" << e->region();

  const QRect& unionRect = e->rect();

  int xStart = startX() + unionRect.x();
  int xEnd = xStart + unionRect.width();
  uint h = renderer()->lineHeight();
  uint startz = (unionRect.y() / h);
  uint endz = startz + 1 + (unionRect.height() / h);
  uint lineRangesSize = cache()->viewCacheLineCount();

  QPainter paint(this);
  paint.setRenderHints (QPainter::Antialiasing);
  
  paint.save();

  // TODO put in the proper places
  if ( !m_view->viInputMode() ) {
    setCaretStyle( m_view->isOverwriteMode() ? KateRenderer::Block : KateRenderer::Line );
  }
  renderer()->setShowTabs(doc()->config()->showTabs());
  renderer()->setShowTrailingSpaces(doc()->config()->showSpaces());

  int sy = startz * h;
  paint.translate(unionRect.x(), startz * h);

  for (uint z=startz; z <= endz; z++)
  {
    if ( (z >= lineRangesSize) || (cache()->viewLine(z).line() == -1) )
    {
      if (!(z >= lineRangesSize))
        cache()->viewLine(z).setDirty(false);

      paint.fillRect( 0, 0, unionRect.width(), h, renderer()->config()->backgroundColor() );
    }
    else
    {
      //kDebug( 13030 )<<"KateViewInternal::paintEvent(QPaintEvent *e):cache()->viewLine"<<z;
      KateTextLayout& thisLine = cache()->viewLine(z);

      /* If viewLine() returns non-zero, then a document line was split
         in several visual lines, and we're trying to paint visual line
         that is not the first.  In that case, this line was already
         painted previously, since KateRenderer::paintTextLine paints
         all visual lines.
         Except if we're at the start of the region that needs to
         be painted -- when no previous calls to paintTextLine were made.
      */
      if (!thisLine.viewLine() || z == startz) {
        // Don't bother if we're not in the requested update region
        if (!e->region().contains(QRect(unionRect.x(), startz * h, unionRect.width(), h)))
          continue;

        //kDebug (13030) << "paint text: line: " << thisLine.line() << " viewLine " << thisLine.viewLine() << " x: " << unionRect.x() << " y: " << sy
        //  << " width: " << xEnd-xStart << " height: " << h << endl;

        if (thisLine.viewLine())
          paint.translate(QPoint(0, h * - thisLine.viewLine()));

        // The paintTextLine function should be well behaved, but if not, this clipping may be needed
        //paint.setClipRect(QRect(xStart, 0, xEnd - xStart, h * (thisLine.kateLineLayout()->viewLineCount())));

        KTextEditor::Cursor pos = m_cursor;
        renderer()->paintTextLine(paint, thisLine.kateLineLayout(), xStart, xEnd, &pos);

        //paint.setClipping(false);

        if (thisLine.viewLine())
          paint.translate(0, h * thisLine.viewLine());

        thisLine.setDirty(false);
      }
    }

    paint.translate(0, h);
    sy += h;
  }
  
  paint.restore();
  if (m_textAnimation)
    m_textAnimation->draw(paint);
}

void KateViewInternal::resizeEvent(QResizeEvent* e)
{
  bool expandedHorizontally = width() > e->oldSize().width();
  bool expandedVertically = height() > e->oldSize().height();
  bool heightChanged = height() != e->oldSize().height();

  m_dummy->setFixedSize(m_lineScroll->width(), m_columnScroll->sizeHint().height());
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
      KateTextLayout viewLine = cache()->viewLine(i);

      if (viewLine.wrap() || viewLine.isRightToLeft() || viewLine.width() > width()) {
        dirtied = true;
        viewLine.setDirty();
        break;
      }
    }

    if (dirtied || heightChanged) {
      updateView(true);
      m_leftBorder->update();
    }
  } else {
    updateView();

    if (expandedHorizontally && startX() > 0)
      scrollColumns(startX() - (width() - e->oldSize().width()));
  }

  if (width() < e->oldSize().width() && !m_view->wrapCursor()) {
    // May have to restrain cursor to new smaller width...
    if (m_cursor.column() > doc()->lineLength(m_cursor.line())) {
      KateTextLayout thisLine = currentLayout();

      KTextEditor::Cursor newCursor(m_cursor.line(), thisLine.endCol() + ((width() - thisLine.xOffset() - (thisLine.width()- m_startX)) / renderer()->spaceWidth()) - 1);
      if (newCursor.column() < m_cursor.column())
        updateCursor(newCursor);
    }
  }


  if (expandedVertically) {
    KTextEditor::Cursor max = maxStartPos();
    if (startPos() > max) {
      scrollPos(max);
      return; // already fired displayRangeChanged
    }
  }
  emit m_view->displayRangeChanged(m_view);
}

void KateViewInternal::scrollTimeout ()
{
  if (m_scrollX || m_scrollY)
  {
    scrollLines (startPos().line() + (m_scrollY / (int) renderer()->lineHeight()));
    placeCursor( QPoint( m_mouseX, m_mouseY ), true );
  }
}

void KateViewInternal::cursorTimeout ()
{
  if (!debugPainting && !m_view->viInputMode()) {
    renderer()->setDrawCaret(!renderer()->drawCaret());
    paintCursor();
  }
}

void KateViewInternal::textHintTimeout ()
{
  m_textHintTimer.stop ();

  KTextEditor::Cursor c = coordinatesToCursor(m_textHintPos, false);
  if (!c.isValid()) return;

  QString tmp;

  emit m_view->needTextHint(c, tmp);

  if (!tmp.isEmpty()) {
    kDebug(13030) << "Hint text: " << tmp;
    QPoint pos(startX() + m_textHintPos.x(), m_textHintPos.y());
    QToolTip::showText(mapToGlobal(pos), tmp);
  }
}

void KateViewInternal::focusInEvent (QFocusEvent *)
{
  if (KApplication::cursorFlashTime() > 0)
    m_cursorTimer.start ( KApplication::cursorFlashTime() / 2 );

  paintCursor();

  doc()->setActiveView( m_view );

  // this will handle focus stuff in kateview
  m_view->slotGotFocus ();
}

void KateViewInternal::focusOutEvent (QFocusEvent *)
{
  //if (m_view->isCompletionActive())
    //m_view->abortCompletion();

  m_cursorTimer.stop();
  m_view->renderer()->setDrawCaret(true);
  paintCursor();

  m_textHintTimer.stop();

  m_view->slotLostFocus ();
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
  event->setAccepted( (event->mimeData()->hasText() && doc()->isReadWrite()) ||
                  KUrl::List::canDecode(event->mimeData()) );
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
  if ( KUrl::List::canDecode(event->mimeData()) ) {

      emit dropEventPass(event);

  } else if ( event->mimeData()->hasText() && doc()->isReadWrite() ) {

    QString text=event->mimeData()->text();

    // is the source our own document?
    bool priv = false;
    if (KateViewInternal* vi = qobject_cast<KateViewInternal*>(event->source()))
      priv = doc()->ownedView( vi->m_view );

    // dropped on a text selection area?
    bool selected = m_view->cursorSelected(m_cursor);

    fixDropEvent(event);

    if( priv && selected && event->dropAction() != Qt::CopyAction ) {
      // this is a drag that we started and dropped on our selection
      // ignore this case
      return;
    }

    // fix the cursor position before editStart(), so that it is correctly
    // stored for the undo action
    KTextEditor::Cursor targetCursor(m_cursor); // backup current cursor
    int selectionWidth = m_view->selectionRange().columnWidth(); // for block selection
    int selectionHeight = m_view->selectionRange().numberOfLines(); // for block selection

    if ( event->dropAction() != Qt::CopyAction ) {
      editSetCursor(m_view->selectionRange().end());
    } else {
      m_view->clearSelection();
    }

    // use one transaction
    doc()->editStart ();

    // on move: remove selected text; on copy: duplicate text
    doc()->insertText(targetCursor, text, m_view->blockSelection());

    Kate::TextCursor startCursor(doc()->buffer(), targetCursor, KTextEditor::MovingCursor::MoveOnInsert);

    if ( event->dropAction() != Qt::CopyAction )
      m_view->removeSelectedText();

    Kate::TextCursor endCursor1(doc()->buffer(), startCursor, KTextEditor::MovingCursor::MoveOnInsert);

    if ( !m_view->blockSelection() ) {
      endCursor1.move(text.length());
    } else {
      endCursor1.setColumn(startCursor.column()+selectionWidth);
      endCursor1.setLine(startCursor.line()+selectionHeight);
    }

    KTextEditor::Cursor endCursor(endCursor1);
    kDebug( 13030 )<<startCursor<<"---("<<text.length()<<")---"<<endCursor;
    setSelection(KTextEditor::Range(startCursor,endCursor));
    editSetCursor(endCursor);

    doc()->editEnd ();

    event->acceptProposedAction();
    updateView();
  }

  // finally finish drag and drop mode
  m_dragInfo.state = diNone;
  // important, because the eventFilter`s DragLeave does not occur
  stopDragScroll();
}
//END EVENT HANDLING STUFF

void KateViewInternal::clear()
{
  m_startPos.setPosition (0, 0);
  m_displayCursor = KTextEditor::Cursor(0, 0);
  m_cursor.setPosition(0, 0);
  cache()->clear();
  updateView(true);
}

void KateViewInternal::wheelEvent(QWheelEvent* e)
{
  // zoom text, if ctrl is pressed
  if (e->modifiers() == Qt::ControlModifier) {
    if (e->delta() > 0) {
      slotIncFontSizes();
    } else {
      slotDecFontSizes();
    }
    e->accept();
    return;
  }

  // scroll up/down or left/right, if possible
  if (m_lineScroll->minimum() != m_lineScroll->maximum() && e->orientation() != Qt::Horizontal) {
    // React to this as a vertical event
    if ( e->modifiers() & Qt::ShiftModifier ) {
      if (e->delta() > 0)
        scrollPrevPage();
      else
        scrollNextPage();
    } else {
      const int scrollLines = QApplication::wheelScrollLines();
      scrollViewLines(e->delta() > 0 ? -scrollLines : scrollLines);
      e->accept();
      return;
    }

  } else if (columnScrollingPossible()) {
    QWheelEvent copy = *e;
    QApplication::sendEvent(m_columnScroll, &copy);

  } else {
    e->ignore();
  }
}

void KateViewInternal::startDragScroll()
{
  if ( !m_dragScrollTimer.isActive() ) {
    m_dragScrollTimer.start( s_scrollTime );
  }
}

void KateViewInternal::stopDragScroll()
{
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

  if (columnScrollingPossible () && dx)
    scrollColumns(qMin (m_startX + dx, m_columnScroll->maximum()));

  if (!dy && !dx)
    stopDragScroll();
}

void KateViewInternal::enableTextHints(int timeout)
{
  if (timeout >= 0) {
    m_textHintTimeout = timeout;
    m_textHintEnabled = true;
    m_textHintTimer.start(timeout);
  } else {
    kWarning() << "Attempt to enable text hints with negative timeout:" << timeout;
  }
}

void KateViewInternal::disableTextHints()
{
  if (m_textHintEnabled) {
    m_textHintEnabled = false;
    m_textHintTimer.stop ();
  }
}

//BEGIN EDIT STUFF
void KateViewInternal::editStart()
{
  editSessionNumber++;

  if (editSessionNumber > 1)
    return;

  editIsRunning = true;
  editOldCursor = m_cursor;
  editOldSelection = m_view->selectionRange();
}

void KateViewInternal::editEnd(int editTagLineStart, int editTagLineEnd, bool tagFrom)
{
   if (editSessionNumber == 0)
    return;

  editSessionNumber--;

  if (editSessionNumber > 0)
    return;

  // fix start position, might have moved from column 0
  // try to clever calculate the right start column for the tricky dyn word wrap case
  int col = 0;
  if (m_view->dynWordWrap()) {
    if (KateLineLayoutPtr layout = cache()->line(m_startPos.line())) {
      int index = layout->viewLineForColumn (m_startPos.column());
      if (index >= 0 && index < layout->viewLineCount())
        col = layout->viewLine (index).startCol();
    }
  }
  m_startPos.setPosition (m_startPos.line(), col);

  if (tagFrom && (editTagLineStart <= int(m_view->textFolding().visibleLineToLine(startLine()))))
    tagAll();
  else
    tagLines (editTagLineStart, tagFrom ? qMax(doc()->lastLine() + 1, editTagLineEnd) : editTagLineEnd, true);

  if (editOldCursor == m_cursor.toCursor())
    updateBracketMarks();

  updateView(true);

  if (editOldCursor != m_cursor.toCursor() || m_view == doc()->activeView())
  {
    // Only scroll the view to the cursor if the insertion happens at the cursor.
    // This might not be the case for e.g. collaborative editing, when a remote user
    // inserts text at a position not at the caret.
    if ( m_cursor.line() >= editTagLineStart && m_cursor.line() <= editTagLineEnd ) {
      m_madeVisible = false;
      updateCursor ( m_cursor, true );
    }
  }

  /**
   * selection changed?
   * fixes bug 316226
   */
  if (editOldSelection != m_view->selectionRange()
      || (editOldSelection.isValid() && !editOldSelection.isEmpty() && !(editTagLineStart > editOldSelection.end().line() && editTagLineEnd < editOldSelection.start().line())))
    emit m_view->selectionChanged (m_view);

  editIsRunning = false;
}

void KateViewInternal::editSetCursor (const KTextEditor::Cursor &_cursor)
{
  if (m_cursor.toCursor() != _cursor)
  {
    m_cursor.setPosition(_cursor);
  }
}
//END

void KateViewInternal::viewSelectionChanged ()
{
  if (!m_view->selection()) {
    m_selectAnchor = KTextEditor::Cursor::invalid();
  } else {
    m_selectAnchor = m_view->selectionRange().start();
  }
  // Do NOT nuke the entire range! The reason is that a shift+DC selection
  // might (correctly) set the range to be empty (i.e. start() == end()), and
  // subsequent dragging might shrink the selection into non-existence. When
  // this happens, we use the cached end to restore the cached start so that
  // updateSelection is not confused. See also comments in updateSelection.
  m_selectionCached.start() = KTextEditor::Cursor::invalid();
}

KateLayoutCache* KateViewInternal::cache( ) const
{
  return m_layoutCache;
}

KTextEditor::Cursor KateViewInternal::toRealCursor( const KTextEditor::Cursor & virtualCursor ) const
{
  return KTextEditor::Cursor(m_view->textFolding().visibleLineToLine(virtualCursor.line()), virtualCursor.column());
}

KTextEditor::Cursor KateViewInternal::toVirtualCursor( const KTextEditor::Cursor & realCursor ) const
{
  /**
   * only convert valid lines, folding doesn't like invalid input!
   * don't validate whole cursor, column might be -1
   */
  if (realCursor.line() < 0)
    return KTextEditor::Cursor::invalid ();

  return KTextEditor::Cursor(m_view->textFolding().lineToVisibleLine(realCursor.line()), realCursor.column());
}

KateRenderer * KateViewInternal::renderer( ) const
{
  return m_view->renderer();
}

void KateViewInternal::mouseMoved( )
{
  m_view->notifyMousePositionChanged(m_mouse);
  m_view->updateRangesIn (KTextEditor::Attribute::ActivateMouseIn);
}

void KateViewInternal::cursorMoved( )
{
  m_view->updateRangesIn (KTextEditor::Attribute::ActivateCaretIn);
#ifndef QT_NO_ACCESSIBILITY
#if QT_VERSION >= 0x040800
  QAccessible::updateAccessibility(this, 0, QAccessible::TextCaretMoved);
#endif
#endif
}

bool KateViewInternal::rangeAffectsView(const KTextEditor::Range& range, bool realCursors) const
{
  int startLine = m_startPos.line();
  int endLine = startLine + (int)m_visibleLineCount;

  if ( realCursors ) {
    startLine = (int)m_view->textFolding().visibleLineToLine(startLine);
    endLine = (int)m_view->textFolding().visibleLineToLine(endLine);
  }

  return (range.end().line() >= startLine) || (range.start().line() <= endLine);
}

//BEGIN IM INPUT STUFF
QVariant KateViewInternal::inputMethodQuery ( Qt::InputMethodQuery query ) const
{
  switch (query) {
    case Qt::ImMicroFocus: {
      // Cursor placement code is changed for Asian input method that
      // shows candidate window. This behavior is same as Qt/E 2.3.7
      // which supports Asian input methods. Asian input methods need
      // start point of IM selection text to place candidate window as
      // adjacent to the selection text.
      return QRect (cursorToCoordinate(m_cursor, true, false), QSize(0, renderer()->lineHeight()));
    }

    case Qt::ImFont:
      return renderer()->currentFont();

    case Qt::ImCursorPosition:
      return m_cursor.column();

    case Qt::ImAnchorPosition:
      // If selectAnchor is at the same line, return the real anchor position
      // Otherwise return the same position of cursor
      if (m_view->selection() && m_selectAnchor.line() == m_cursor.line())
        return m_selectAnchor.column();
      else
        return m_cursor.column();

    case Qt::ImSurroundingText:
      if (Kate::TextLine l = doc()->kateTextLine(m_cursor.line()))
        return l->string();
      else
        return QString();

    case Qt::ImCurrentSelection:
      if (m_view->selection())
        return m_view->selectionText();
      else
        return QString();
    default:
      /* values: ImMaximumTextLength */
      break;
  }

  return QWidget::inputMethodQuery(query);
}

void KateViewInternal::inputMethodEvent(QInputMethodEvent* e)
{
  if ( doc()->readOnly() ) {
    e->ignore();
    return;
  }

  //kDebug( 13030 ) << "Event: cursor" << m_cursor << "commit" << e->commitString() << "preedit" << e->preeditString() << "replacement start" << e->replacementStart() << "length" << e->replacementLength();

  if (!m_imPreeditRange) {
    m_imPreeditRange = doc()->newMovingRange (KTextEditor::Range(m_cursor, m_cursor), KTextEditor::MovingRange::ExpandLeft | KTextEditor::MovingRange::ExpandRight);
  }

  if (!m_imPreeditRange->toRange().isEmpty()) {
    doc()->inputMethodStart();
    doc()->removeText(*m_imPreeditRange);
    doc()->inputMethodEnd();
  }

  if (!e->commitString().isEmpty() || e->replacementLength()) {
    m_view->removeSelectedText();

    KTextEditor::Range preeditRange = *m_imPreeditRange;

    KTextEditor::Cursor start(m_imPreeditRange->start().line(), m_imPreeditRange->start().column() + e->replacementStart());
    KTextEditor::Cursor removeEnd = start + KTextEditor::Cursor(0, e->replacementLength());

    doc()->editStart();
    if (start != removeEnd)
      doc()->removeText(KTextEditor::Range(start, removeEnd));
    if (!e->commitString().isEmpty()) {
      // if the input method event is text that should be inserted, call KateDocument::typeChars()
      // with the text. that method will handle the input and take care of overwrite mode, etc.
      doc()->typeChars(m_view, e->commitString());
    }
    doc()->editEnd();

    // Revert to the same range as above
    m_imPreeditRange->setRange(preeditRange);
  }

  if (!e->preeditString().isEmpty()) {
    doc()->inputMethodStart();
    doc()->insertText(m_imPreeditRange->start(), e->preeditString());
    doc()->inputMethodEnd();
    // The preedit range gets automatically repositioned
  }

  // Finished this input method context?
  if (m_imPreeditRange && e->preeditString().isEmpty()) {
    // delete the range and reset the pointer
    delete m_imPreeditRange;
    m_imPreeditRange = 0L;
    qDeleteAll (m_imPreeditRangeChildren);
    m_imPreeditRangeChildren.clear ();

    if ( KApplication::cursorFlashTime() > 0 )
      renderer()->setDrawCaret(false);
    renderer()->setCaretOverrideColor(QColor());

    e->accept();
    return;
  }

  KTextEditor::Cursor newCursor = m_cursor;
  bool hideCursor = false;
  QColor caretColor;

  if (m_imPreeditRange) {
    qDeleteAll (m_imPreeditRangeChildren);
    m_imPreeditRangeChildren.clear ();

    int decorationColumn = 0;
    foreach (const QInputMethodEvent::Attribute &a, e->attributes()) {
      if (a.type == QInputMethodEvent::Cursor) {
        newCursor = m_imPreeditRange->start() + KTextEditor::Cursor(0, a.start);
        hideCursor = !a.length;
        QColor c = qvariant_cast<QColor>(a.value);
        if (c.isValid())
          caretColor = c;

      } else if (a.type == QInputMethodEvent::TextFormat) {
        QTextCharFormat f = qvariant_cast<QTextFormat>(a.value).toCharFormat();
        if (f.isValid() && decorationColumn <= a.start) {
          KTextEditor::Range fr(m_imPreeditRange->start().line(),  m_imPreeditRange->start().column() + a.start, m_imPreeditRange->start().line(), m_imPreeditRange->start().column() + a.start + a.length);
          KTextEditor::MovingRange* formatRange = doc()->newMovingRange (fr);
          KTextEditor::Attribute::Ptr attribute(new KTextEditor::Attribute());
          attribute->merge(f);
          formatRange->setAttribute(attribute);
          decorationColumn = a.start + a.length;
          m_imPreeditRangeChildren.push_back (formatRange);
        }
      }
    }
  }

  renderer()->setDrawCaret(hideCursor);
  renderer()->setCaretOverrideColor(caretColor);

  if (newCursor != m_cursor.toCursor())
    updateCursor(newCursor);

  e->accept();
}

//END IM INPUT STUFF

ViMode KateViewInternal::getCurrentViMode()
{
  return getViInputModeManager()->getCurrentViMode();
}

KateViInputModeManager* KateViewInternal::getViInputModeManager()
{
  if (!m_viInputModeManager) {
    m_viInputModeManager = new KateViInputModeManager(m_view, this);
  }

  return m_viInputModeManager;
}

KateViInputModeManager* KateViewInternal::resetViInputModeManager()
{
    if (m_viInputModeManager) {
       delete m_viInputModeManager;
    }
    m_viInputModeManager = new KateViInputModeManager(m_view, this);

  return m_viInputModeManager;
}

void KateViewInternal::flashChar(const KTextEditor::Cursor & pos, KTextEditor::Attribute::Ptr attribute)
{
  Q_ASSERT(pos.isValid());
  Q_ASSERT(!attribute.isNull());

  // if line is folded away, do nothing
  if (!m_view->textFolding().isLineVisible (pos.line())) {
    return;
  }

  KTextEditor::Range range(pos, KTextEditor::Cursor(pos.line(), pos.column() + 1));
  if (m_textAnimation) m_textAnimation->deleteLater();
  m_textAnimation = new KateTextAnimation(range, attribute, this);
}

// kate: space-indent on; indent-width 2; replace-tabs on;
