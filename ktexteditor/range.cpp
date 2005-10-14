/* This file is part of the KDE libraries
   Copyright (C) 2003-2005 Hamish Rodda <rodda@kde.org>
   Copyright (C) 2002 Christian Couder <christian@kdevelop.org>
   Copyright (C) 2001, 2003 Christoph Cullmann <cullmann@kde.org>
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

#include "range.h"

#include "document.h"
#include "view.h"
#include "attribute.h"

#include <kaction.h>
#include <kdebug.h>

using namespace KTextEditor;

Range::Range()
  : m_start(new Cursor())
  , m_end(new Cursor())
{
}

Range::Range(const Cursor& start, const Cursor& end)
{
  if (start <= end) {
    m_start = new Cursor(start);
    m_end = new Cursor(end);

  } else {
    m_start = new Cursor(end);
    m_end = new Cursor(start);
  }
}

Range::Range(const Cursor& start, int width)
  : m_start(new Cursor(start))
  , m_end(new Cursor(start.line(), start.column() + width))
{
}

Range::Range(const Cursor& start, int endLine, int endCol)
  : m_start(new Cursor(start))
  , m_end(new Cursor(endLine, endCol))
{
  if (*m_end < *m_start) {
    Cursor* temp = m_end;
    m_end = m_start;
    m_start = temp;
  }
}

Range::Range(int startLine, int startCol, int endLine, int endCol)
  : m_start(new Cursor(startLine, startCol))
  , m_end(new Cursor(endLine, endCol))
{
  if (*m_end < *m_start) {
    Cursor* temp = m_end;
    m_end = m_start;
    m_start = temp;
  }
}

Range::Range(Cursor* start, Cursor* end)
  : m_start(start)
  , m_end(end)
{
  if (*m_end < *m_start) {
    Cursor temp = *m_end;
    *m_end = *m_start;
    *m_start = temp;
  }
}

Range::Range(const Range& copy)
  : m_start(new Cursor(copy.start()))
  , m_end(new Cursor(copy.end()))
{
}

Range::~Range()
{
  delete m_start;
  delete m_end;
}

bool KTextEditor::Range::isValid( ) const
{
  return start().line() >= 0 && start().column() >= 0;
}

const Range& Range::invalid()
{
  static Range r;
  if (r.start().line() != -1)
    r.setRange(Cursor(-1,-1),Cursor(-1,-1));
  return r;
}

Range& Range::operator= (const Range& rhs)
{
  if (this == &rhs)
    return *this;
  
  setRange(rhs);
  
  return *this;
}

void Range::setStart(const Cursor& newStart)
{
  *m_start = newStart;
  if (start() > end())
    *m_end = newStart;
}

void Range::setEnd(const Cursor& newEnd)
{
  *m_end = newEnd;
  if (start() > end())
    *m_start = newEnd;
}

void Range::setRange(const Range& range)
{
  *m_start = range.start();
  *m_end = range.end();
}

void Range::setRange( const Cursor & start, const Cursor & end )
{
  if (start > end) {
    *m_start = end;
    *m_end = start;
  } else {
    *m_start = start;
    *m_end = end;
  }
}

bool Range::containsLine(int line) const
{
  return (line > start().line() || line == start().line() && !start().column()) && line < end().line();
}

bool Range::includesLine(int line) const
{
  return line >= start().line() && line <= end().line();
}

bool Range::spansColumn( int col ) const
{
  return start().column() <= col && end().column() > col;
}

bool Range::contains( const Cursor& cursor ) const
{
  return cursor >= start() && cursor < end();
}

bool Range::contains( const Range& range ) const
{
  return range.start() >= start() && range.end() <= end();
}

bool Range::overlaps( const Range& range ) const
{
  if (range.start() <= start())
    return range.end() > start();

  else if (range.end() >= end())
    return range.start() < end();

  else
    return contains(range);
}

bool Range::boundaryAt(const KTextEditor::Cursor& cursor) const
{
  return cursor == start() || cursor == end();
}

bool Range::boundaryOnLine(int line) const
{
  return start().line() == line || end().line() == line;
}

void Range::confineToRange(const Range& range)
{
  if (start() < range.start())
    if (end() < range.end())
      setRange(range);
    else
      setStart(range.start());
  else if (end() < range.end())
    setEnd(range.end());
}

void SmartRange::confineToRange(const Range& range)
{
  if (start() < range.start())
    if (end() < range.end())
      setRange(range);
    else
      setStart(range.start());
  else if (end() < range.end())
    setEnd(range.end());
  else
    // Don't need to check if children should be confined, they already are
    return;

  foreach (SmartRange* child, m_childRanges)
    child->confineToRange(*this);
}

SmartRange::SmartRange(SmartCursor* start, SmartCursor* end, SmartRange * parent, InsertBehaviours insertBehaviour )
  : Range(start, end)
  , m_attribute(0L)
  , m_parentRange(parent)
  , m_ownsAttribute(false)
{
  start->setBelongsToRange(this);
  end->setBelongsToRange(this);
  setInsertBehaviour(insertBehaviour);
}

SmartRange * SmartRange::childBefore( const SmartRange * range ) const
{
  int index = m_childRanges.indexOf(const_cast<SmartRange*>(range));
  if (index != -1 && --index > 0)
    return m_childRanges[index];
  return 0L;
}

SmartRange * SmartRange::childAfter( const SmartRange * range ) const
{
  int index = m_childRanges.indexOf(const_cast<SmartRange*>(range));
  if (index != -1 && ++index < m_childRanges.count())
    return m_childRanges[index];
  return 0L;
}

SmartRange::~SmartRange( )
{
  setAttribute(0L);

  /*if (!m_deleteCursors)
  {
    // Save from deletion in the parent
    m_start = 0L;
    m_end = 0L;
  }*/
}

SmartRange * KTextEditor::SmartRange::findMostSpecificRange( const Range & input ) const
{
  if (contains(input)) {
    for (QList<SmartRange*>::ConstIterator it = m_childRanges.constBegin(); it != m_childRanges.constEnd(); ++it)
      if ((*it)->contains(input))
        return (*it)->findMostSpecificRange(input);

    return const_cast<SmartRange*>(this);

  } else if (parentRange()) {
    return parentRange()->findMostSpecificRange(input);

  } else {
    return 0L;
  }
}

SmartRange * KTextEditor::SmartRange::firstRangeIncluding( const Cursor & pos ) const
{
  switch (relativePosition(pos)) {
    case 0:
      if (parentRange() && parentRange()->contains(pos))
        return parentRange()->firstRangeIncluding(pos);

      return const_cast<SmartRange*>(this);

    case -1:
      if (!parentRange())
        return 0L;

      if (!parentRange()->contains(pos))
        return parentRange()->firstRangeIncluding(pos);

      if (SmartRange* r = parentRange()->childAfter(this))
        return r->firstRangeIncluding(pos);

      return 0L;

    case 1:
      if (!parentRange())
        return 0L;

      if (!parentRange()->contains(pos))
        return parentRange()->firstRangeIncluding(pos);

      if (const SmartRange* r = parentRange()->childBefore(this))
        return r->firstRangeIncluding(pos);

      return 0L;

    default:
      Q_ASSERT(false);
      return 0L;
  }
}

SmartRange * KTextEditor::SmartRange::deepestRangeContaining( const Cursor & pos ) const
{
  switch (relativePosition(pos)) {
    case 0:
      for (QList<SmartRange*>::ConstIterator it = m_childRanges.constBegin(); it != m_childRanges.constEnd(); ++it)
        if ((*it)->contains(pos) == 0)
          return ((*it)->deepestRangeContaining(pos));

      return const_cast<SmartRange*>(this);

    case -1:
      if (!parentRange())
        return 0L;

      if (!parentRange()->contains(pos))
        return parentRange()->deepestRangeContaining(pos);

      if (const SmartRange* r = parentRange()->childAfter(this))
        return r->deepestRangeContaining(pos);

      return 0L;

    case 1:
      if (!parentRange())
        return 0L;

      if (!parentRange()->contains(pos))
        return parentRange()->deepestRangeContaining(pos);

      if (const SmartRange* r = parentRange()->childBefore(this))
        return r->firstRangeIncluding(pos);

      return 0L;

    default:
      Q_ASSERT(false);
      return 0L;
  }
}

Document* SmartRange::document( ) const
{
  return smartStart().document();
}

void SmartRange::attachAction( KAction * action )
{
  m_associatedActions.append(action);

  bool enable = false;
  if (View* v = document()->activeView())
    if (contains(v->cursorPosition()))
      enable = true;

  action->setEnabled(enable);

  if (m_associatedActions.count() == 1)
    checkFeedback();
}

void SmartRange::detachAction( KAction * action )
{
  m_associatedActions.removeAt(m_associatedActions.indexOf(action));
  if (!m_associatedActions.count())
    checkFeedback();
}

SmartRange::InsertBehaviours SmartRange::insertBehaviour( ) const
{
  return (smartStart().moveOnInsert() ? DoNotExpand : ExpandLeft) | (smartEnd().moveOnInsert() ? ExpandRight : DoNotExpand);
}

void SmartRange::setInsertBehaviour(SmartRange::InsertBehaviours behaviour)
{
  static_cast<SmartCursor*>(m_start)->setMoveOnInsert(behaviour & ExpandLeft);
  static_cast<SmartCursor*>(m_end)->setMoveOnInsert(behaviour & ExpandRight);
}

void SmartRange::clearAllChildRanges()
{
  // FIXME: Probably more efficient to prevent them from unlinking themselves?
  qDeleteAll(m_childRanges);

  // i.e. this is probably already clear
  m_childRanges.clear();
}

void SmartRange::deleteAllChildRanges()
{
  foreach (KTextEditor::Range* r, m_childRanges);
    // FIXME editDeleteText here? or a call to delete the text on the range object

  qDeleteAll(m_childRanges);
}

void KTextEditor::SmartRange::setParentRange( SmartRange * r )
{
  m_parentRange = r;
}

KTextEditor::SmartRangeWatcher::~ SmartRangeWatcher( )
{
}

void KTextEditor::SmartRangeWatcher::positionChanged( SmartRange * )
{
}

void KTextEditor::SmartRangeWatcher::contentsChanged( SmartRange * )
{
}

void KTextEditor::SmartRangeWatcher::eliminated( SmartRange * )
{
}

void SmartRange::setAttribute( Attribute * attribute, bool ownsAttribute )
{
  if (m_attribute)
    m_attribute->removeRange(this);

  if (m_ownsAttribute) delete m_attribute;

  m_attribute = attribute;
  m_ownsAttribute = ownsAttribute;
  if (m_attribute)
    m_attribute->addRange(this);
}

Attribute * KTextEditor::SmartRange::attribute( ) const
{
  return m_attribute;
}

/*void KTextEditor::SmartRangeWatcher::boundaryDeleted( SmartRange * , bool )
{
}

void KTextEditor::SmartRangeWatcher::firstCharacterDeleted( SmartRange * )
{
}

void KTextEditor::SmartRangeWatcher::lastCharacterDeleted( SmartRange * )
{
}*/

#include "range.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
