/* This file is part of the KDE libraries
   Copyright (C) 2003,2004,2005 Hamish Rodda <rodda@kde.org>

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

#include "katesuperrange.h"

#include "katedocument.h"
#include "kateview.h"
#include "kateattribute.h"
#include "katerangelist.h"
#include "katerangetype.h"

#include <kdebug.h>

KateSuperRange::KateSuperRange(KateSuperCursor* start, KateSuperCursor* end, KateSuperRange* parent)
  : QObject(parent)
  , KTextEditor::Range(start, end)
  , m_attribute(0L)
  , m_parentRange(0L)
  , m_attachedView(0L)
  , m_attachActions(TagLines)
  , m_valid(true)
  , m_ownsAttribute(true)
  , m_evaluate(false)
  , m_startChanged(false)
  , m_endChanged(false)
  , m_deleteCursors(false)
  , m_allowZeroLength(false)
  , m_mouseOver(false)
  , m_caretOver(false)
{
  init();
}

KateSuperRange::KateSuperRange( KateSuperCursor * start, KateSuperCursor * end, QObject * parent )
  : QObject(parent)
  , KTextEditor::Range(start, end)
  , m_attribute(0L)
  , m_parentRange(0L)
  , m_attachedView(0L)
  , m_attachActions(TagLines)
  , m_valid(true)
  , m_ownsAttribute(true)
  , m_evaluate(false)
  , m_startChanged(false)
  , m_endChanged(false)
  , m_deleteCursors(true)
  , m_allowZeroLength(false)
  , m_mouseOver(false)
  , m_caretOver(false)
{
  init();
}

KateSuperRange::KateSuperRange(KateDocument* doc, const KateSuperRange& range, KateSuperRange* parent)
  : QObject(parent)
  , KTextEditor::Range(new KateSuperCursor(doc, true, range.start()), new KateSuperCursor(doc, true, range.end()))
  , m_attribute(0L)
  , m_parentRange(0L)
  , m_attachedView(0L)
  , m_attachActions(TagLines)
  , m_valid(true)
  , m_ownsAttribute(true)
  , m_evaluate(false)
  , m_startChanged(false)
  , m_endChanged(false)
  , m_deleteCursors(true)
  , m_allowZeroLength(false)
  , m_mouseOver(false)
  , m_caretOver(false)
{
  init();
}

KateSuperRange::KateSuperRange(KateDocument* doc, const KTextEditor::Cursor& start, const KTextEditor::Cursor& end, KateSuperRange* parent)
  : QObject(parent)
  , KTextEditor::Range(new KateSuperCursor(doc, true, start), new KateSuperCursor(doc, true, end))
  , m_attribute(0L)
  , m_parentRange(0L)
  , m_attachedView(0L)
  , m_attachActions(TagLines)
  , m_valid(true)
  , m_ownsAttribute(true)
  , m_evaluate(false)
  , m_startChanged(false)
  , m_endChanged(false)
  , m_deleteCursors(true)
  , m_allowZeroLength(false)
  , m_mouseOver(false)
  , m_caretOver(false)
{
  init();
}

KateSuperRange::KateSuperRange( KateDocument * doc, uint startLine, uint startCol, uint endLine, uint endCol, KateSuperRange * parent )
  : QObject(parent)
  , KTextEditor::Range(new KateSuperCursor(doc, true, startLine, startCol), new KateSuperCursor(doc, true, endLine, endCol))
  , m_attribute(0L)
  , m_parentRange(0L)
  , m_attachedView(0L)
  , m_attachActions(TagLines)
  , m_valid(true)
  , m_ownsAttribute(true)
  , m_evaluate(false)
  , m_startChanged(false)
  , m_endChanged(false)
  , m_deleteCursors(true)
  , m_allowZeroLength(false)
  , m_mouseOver(false)
  , m_caretOver(false)
{
}

void KateSuperRange::init()
{
  Q_ASSERT(isValid());
  Q_ASSERT(superStart().doc() == superStart().doc());

  insertChild(&superStart());
  insertChild(&superEnd());

  setBehaviour(DoNotExpand);

  // Not necessarily the best implementation
  connect(&superStart(), SIGNAL(positionDirectlyChanged()),  SIGNAL(contentsChanged()));
  connect(&superEnd(), SIGNAL(positionDirectlyChanged()),  SIGNAL(contentsChanged()));

  connect(&superStart(), SIGNAL(positionChanged()),  SLOT(slotEvaluateChanged()));
  connect(&superEnd(), SIGNAL(positionChanged()),  SLOT(slotEvaluateChanged()));
  connect(&superStart(), SIGNAL(positionUnChanged()), SLOT(slotEvaluateUnChanged()));
  connect(&superEnd(), SIGNAL(positionUnChanged()), SLOT(slotEvaluateUnChanged()));
  connect(&superStart(), SIGNAL(positionDeleted()), SIGNAL(boundaryDeleted()));
  connect(&superEnd(), SIGNAL(positionDeleted()), SIGNAL(boundaryDeleted()));
}

KateSuperRange::~KateSuperRange()
{
  if (m_ownsAttribute) delete m_attribute;

  if (!m_deleteCursors)
  {
    // Save from deletion in the parent
    m_start = 0L;
    m_end = 0L;
  }
}

KateSuperCursor& KateSuperRange::superStart()
{
  return *dynamic_cast<KateSuperCursor*>(m_start);
}

const KateSuperCursor& KateSuperRange::superStart() const
{
  return *dynamic_cast<KateSuperCursor*>(m_start);
}

KateSuperCursor& KateSuperRange::superEnd()
{
  return *dynamic_cast<KateSuperCursor*>(m_end);
}

const KateSuperCursor& KateSuperRange::superEnd() const
{
  return *dynamic_cast<KateSuperCursor*>(m_end);
}

void KateSuperRange::attachToView(KateView* view, int actions)
{
  m_attachedView = view;
  m_attachActions = actions;
}

int KateSuperRange::behaviour() const
{
  return (superStart().moveOnInsert() ? DoNotExpand : ExpandLeft) | (superEnd().moveOnInsert() ? ExpandRight : DoNotExpand);
}

void KateSuperRange::setBehaviour(int behaviour)
{
  superStart().setMoveOnInsert(behaviour & ExpandLeft);
  superEnd().setMoveOnInsert(!(behaviour & ExpandRight));
}

bool KateSuperRange::isValid() const
{
  return start() <= end();
}

void KateSuperRange::slotEvaluateChanged()
{
  if (sender() == dynamic_cast<QObject*>(m_start)) {
    if (m_evaluate) {
      if (!m_endChanged) {
        // Only one was changed
        evaluateEliminated();

      } else {
        // Both were changed
        evaluatePositionChanged();
        m_endChanged = false;
      }

    } else {
      m_startChanged = true;
    }

  } else {
    if (m_evaluate) {
      if (!m_startChanged) {
        // Only one was changed
        evaluateEliminated();

      } else {
        // Both were changed
        evaluatePositionChanged();
        m_startChanged = false;
      }

    } else {
      m_endChanged = true;
    }
  }

  m_evaluate = !m_evaluate;
}

void KateSuperRange::slotEvaluateUnChanged()
{
  if (sender() == dynamic_cast<QObject*>(m_start)) {
    if (m_evaluate) {
      if (m_endChanged) {
        // Only one changed
        evaluateEliminated();
        m_endChanged = false;

      } else {
        // Neither changed
        emit positionUnChanged();
      }
    }

  } else {
    if (m_evaluate) {
      if (m_startChanged) {
        // Only one changed
        evaluateEliminated();
        m_startChanged = false;

      } else {
        // Neither changed
        emit positionUnChanged();
      }
    }
  }

  m_evaluate = !m_evaluate;
}

void KateSuperRange::slotTagRange()
{
  if (m_attachActions & TagLines)
    if (m_attachedView)
      m_attachedView->tagRange(*this);
    else
      doc()->tagLines(start(), end());

  if (m_attachActions & Redraw)
    if (m_attachedView)
      m_attachedView->repaintText(true);
    else
      doc()->repaintViews(true);
  //else
    //FIXME this method doesn't exist??
    //doc()->updateViews();

  emit tagRange(this);
}

void KateSuperRange::evaluateEliminated()
{
  if (start() == end()) {
    if (!m_allowZeroLength) emit eliminated();
  }
  else
    emit contentsChanged();
}

void KateSuperRange::evaluatePositionChanged()
{
  if (start() == end())
    emit eliminated();
  else
    emit positionChanged();
}

void KateSuperRange::slotMousePositionChanged(const KTextEditor::Cursor& newPosition)
{
  bool includesMouse = includes(newPosition);
  if (includesMouse != m_mouseOver) {
    m_mouseOver = includesMouse;

    slotTagRange();
  }
}

void KateSuperRange::slotCaretPositionChanged(const KTextEditor::Cursor& newPosition)
{
  bool includesCaret = includes(newPosition);
  if (includesCaret != m_caretOver) {
    m_caretOver = includesCaret;

    slotTagRange();
  }
}

KateDocument * KateSuperRange::doc( ) const
{
  return superStart().doc();
}

KateRangeType * KateSuperRange::rangeType( ) const
{
  return owningList() ? owningList()->rangeType() : 0L;
}

KateAttribute * KateSuperRange::attribute( ) const
{
  if (owningList() && owningList()->rangeType())
    if (KateAttribute* a = owningList()->rangeType()->activatedAttribute(m_mouseOver, m_caretOver))
      return a;

  return 0L;
}

KateRangeList * KateSuperRange::owningList( ) const
{
  return static_cast<KateRangeList*>(parent());
}

void KateSuperRange::allowZeroLength( bool allow )
{
  m_allowZeroLength = allow;
}

int KateSuperRange::depth( ) const
{
  return parentRange() ? parentRange()->depth() + 1 : 0;
}

KateSuperRange* KateSuperRange::findMostSpecificRange( const KTextEditor::Range & input )
{
  if (contains(input)) {
    for (QList<KateSuperRange*>::ConstIterator it = m_childRanges.constBegin(); it != m_childRanges.constEnd(); ++it)
      if ((*it)->contains(input))
        return (*it)->findMostSpecificRange(input);

    return const_cast<KateSuperRange*>(this);

  } else if (parentRange()) {
    return parentRange()->findMostSpecificRange(input);

  } else {
    return 0L;
  }
}

KateSuperRange* KateSuperRange::firstRangeIncluding( const KTextEditor::Cursor & pos )
{
  switch (includes(pos)) {
    case 0:
      if (parentRange() && parentRange()->includes(pos))
        return parentRange()->firstRangeIncluding(pos);

      return this;

    case -1:
      if (!parentRange())
        return 0L;

      if (!parentRange()->includes(pos))
        return parentRange()->firstRangeIncluding(pos);

      if (KateSuperRange* r = parentRange()->rangeAfter(this))
        return r->firstRangeIncluding(pos);

      return 0L;

    case 1:
      if (!parentRange())
        return 0L;

      if (!parentRange()->includes(pos))
        return parentRange()->firstRangeIncluding(pos);

      if (KateSuperRange* r = parentRange()->rangeBefore(this))
        return r->firstRangeIncluding(pos);

      return 0L;

    default:
      Q_ASSERT(false);
      return 0L;
  }
}

KateSuperRange* KateSuperRange::deepestRangeIncluding( const KTextEditor::Cursor & pos )
{
  switch (includes(pos)) {
    case 0:
      for (QList<KateSuperRange*>::ConstIterator it = m_childRanges.constBegin(); it != m_childRanges.constEnd(); ++it)
        if ((*it)->includes(pos) == 0)
          return ((*it)->deepestRangeIncluding(pos));

      return this;

    case -1:
      if (!parentRange())
        return 0L;

      if (!parentRange()->includes(pos))
        return parentRange()->deepestRangeIncluding(pos);

      if (KateSuperRange* r = parentRange()->rangeAfter(this))
        return r->deepestRangeIncluding(pos);

      return 0L;

    case 1:
      if (!parentRange())
        return 0L;

      if (!parentRange()->includes(pos))
        return parentRange()->deepestRangeIncluding(pos);

      if (KateSuperRange* r = parentRange()->rangeBefore(this))
        return r->firstRangeIncluding(pos);

      return 0L;

    default:
      Q_ASSERT(false);
      return 0L;
  }
}

KateSuperRange* KateSuperRange::parentRange( ) const
{
  return m_parentRange;
}

void KateSuperRange::setParentRange( KateSuperRange * r )
{
  m_parentRange = r;
}

KateSuperRange* KateSuperRange::firstChildRange( ) const
{
  if (m_childRanges.count())
    return m_childRanges.first();

  return 0L;
}

const QList< KateSuperRange * > & KateSuperRange::childRanges( ) const
{
  return m_childRanges;
}

void KateSuperRange::clearAllChildRanges()
{
  while (m_childRanges.count()) {
    KTextEditor::Range* r = m_childRanges.first();
    m_childRanges.remove(m_childRanges.begin());

    delete r;
  }
}

void KateSuperRange::deleteAllChildRanges()
{
  while (m_childRanges.count()) {
    KTextEditor::Range* r = m_childRanges.first();
    m_childRanges.remove(m_childRanges.begin());

    // FIXME editDeleteText here? or a call to delete the text on the range object
    delete r;
  }
}

bool KateSuperRange::includesWholeLine(uint lineNum) const
{
  return isValid() && ((int)lineNum > start().line() || ((int)lineNum == start().line() && superStart().atStartOfLine())) && ((int)lineNum < end().line() || ((int)lineNum == end().line() && superEnd().atEndOfLine()));
}

void KateSuperRange::setValid( bool valid )
{
  m_valid = valid;
}

void KateSuperRange::setAttribute( KateAttribute * attribute, bool ownsAttribute )
{
  if (m_ownsAttribute)
    delete m_attribute;

  m_attribute = attribute;
  m_ownsAttribute = ownsAttribute;
}

KateSuperRange * KateSuperRange::rangeAfter( KateSuperRange * range ) const
{
  QList<KateSuperRange*>::ConstIterator it = m_childRanges.find(range);
  if (++it != m_childRanges.end())
    return *it;
  return 0L;
}

KateSuperRange* KateSuperRange::rangeBefore( KateSuperRange* range ) const
{
  QList<KateSuperRange*>::ConstIterator it = m_childRanges.find(range);
  if (--it != m_childRanges.end())
    return *it;
  return 0L;
}

KateTopRange::KateTopRange( KateDocument * doc, KateRangeList * ownerList )
  : KateSuperRange(new KateSuperCursor(doc, 0, 0), new KateSuperCursor(doc, doc->lines() - 1, doc->kateTextLine(doc->lines() - 1)->length()), ownerList)
{
  setBehaviour(KateSuperRange::ExpandRight);
}

KateRangeList * KateTopRange::owningList( ) const
{
  return static_cast<KateRangeList*>(const_cast<QObject*>(parent()));
}

// kate: space-indent on; indent-width 2; replace-tabs on;

#include "katesuperrange.moc"
