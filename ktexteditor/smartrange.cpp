/* This file is part of the KDE libraries
   Copyright (C) 2003-2005 Hamish Rodda <rodda@kde.org>
   Copyright (C) 2008 David Nolden <david.nolden.kdevelop@art-master.de>

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

#include "smartrange.h"

#include <QtCore/QStack>

#include "document.h"
#include "view.h"
#include "attribute.h"
#include "rangefeedback.h"

#include <kaction.h>
#include <kdebug.h>

using namespace KTextEditor;

// #define DEBUG_BINARY_SEARCH

///Returns the first index of a range that contains @param pos, or the index of the first range that is behind pos(or ranges.count() if pos is behind all ranges)
///The list must be sorted by the ranges end-positions.
static int lowerBound(const QList<SmartRange*>& ranges, const Cursor& pos)
{
    int begin = 0;
    int n = ranges.count();

    int half;
    int middle;

    while (n > 0) {
        half = n >> 1;
        middle = begin + half;
        if(ranges[middle]->end() > pos) {
          n = half;
        }else{
          begin = middle + 1;
          n -= half + 1;
        }
    }
    return begin;
}

///Searches for the given range, or a lower bound for the given position.
static int lowerBoundRange(const QList<SmartRange*>& ranges, const Cursor& pos, const SmartRange* range)
{
    int begin = 0;
    int n = ranges.count();

    int half;
    int middle;

    while (n > 0) {
        half = n >> 1;
        middle = begin + half;
        if(ranges[begin] == range)
          return begin;
        if(ranges[middle] == range)
          return middle;
        
        if(ranges[middle]->end() > pos) {
          n = half;
        }else{
          begin = middle + 1;
          n -= half + 1;
        }
    }
    return begin;
}

///Finds the index of the given SmartRange in the sorted list using binary search. Uses @param range For searching, and @param smartRange for equality comparison.
static int findIndex(const QList<SmartRange*>& ranges, const SmartRange* smartRange, const Range* range) {
  int index = lowerBoundRange(ranges, range->start(), smartRange);
  int childCount = ranges.count();
  
  //In case of degenerated ranges, we may have found the wrong index.
  while(index < childCount && ranges[index] != smartRange)
    ++index;

  if(index == childCount) {
    //During rangeChanged the order may temporarily be inconsistent, so we use indexOf as fallback
    return ranges.indexOf(const_cast<SmartRange*>(smartRange));
/*    if(smartRange != range) //Try again finding the range, using the smart-range only
      return findIndex(ranges, smartRange, smartRange);*/
    
//     Q_ASSERT(ranges.indexOf(const_cast<SmartRange*>(smartRange)) == -1);
    return -1;
  }
  
  return index;
}

SmartRange::SmartRange(SmartCursor* start, SmartCursor* end, SmartRange * parent, InsertBehaviors insertBehavior )
  : Range(start, end)
  , m_attribute(0L)
  , m_parentRange(parent)
  , m_ownsAttribute(false)
{
  setInsertBehavior(insertBehavior);

  // Not calling setParentRange here...:
  // 1) subclasses are not yet constructed
  // 2) it would otherwise give the wrong impression
  if (m_parentRange)
    m_parentRange->insertChildRange(this);
}

SmartRange::~SmartRange( )
{
  deleteChildRanges();

  setParentRange(0L);

  /*if (!m_deleteCursors)
  {
    // Save from deletion in the parent
    m_start = 0L;
    m_end = 0L;
  }*/
}

bool SmartRange::confineToRange(const Range& range)
{
  if (!Range::confineToRange(range))
    // Don't need to check if children should be confined, they already are
    return false;

  foreach (SmartRange* child, m_childRanges)
    child->confineToRange(*this);

  return true;
}

bool SmartRange::expandToRange(const Range& range)
{
  if (!Range::expandToRange(range))
    // Don't need to check if parents should be expanded, they already are
    return false;

  if (parentRange())
    parentRange()->expandToRange(*this);

  return true;
}

void SmartRange::setRange(const Range& range)
{
  if (range == *this)
    return;

  Range old = *this;

  Range::setRange(range);

  rangeChanged(0L, old);
}

const QList<SmartRange*>& SmartRange::childRanges() const
{
  return m_childRanges;
}

SmartRange * SmartRange::childBefore( const SmartRange * range ) const
{
  int index = findIndex(m_childRanges, range, range);
  
  if (--index >= 0)
    return m_childRanges[index];
  
  return 0L;
}

SmartRange * SmartRange::childAfter( const SmartRange * range ) const
{
  int index = findIndex(m_childRanges, range, range);
  if (index != -1 && ++index < m_childRanges.count())
    return m_childRanges[index];
  return 0L;
}

void SmartRange::insertChildRange( SmartRange * newChild )
{
  // This function is backwards because it's most likely the new child will go onto the end
  // of the child list
  Q_ASSERT(newChild->parentRange() == this);

  // A new child has been added, so expand this range if required.
  expandToRange(*newChild);
  #ifdef DEBUG_BINARY_SEARCH
  {
  KTextEditor::Cursor lastEnd = KTextEditor::Cursor(-1,-1);
  for(int a = 0; a < m_childRanges.size(); ++a) {
    Q_ASSERT(m_childRanges[a]->end() >= lastEnd);
    lastEnd = m_childRanges[a]->end();
  }
  }
  #endif

  int insertAt = lowerBound(m_childRanges, newChild->start());
  if(insertAt != m_childRanges.size()) {
    if(m_childRanges[insertAt]->start() < newChild->end()) {
      //Give a warning here, because this most probably results in unwanted behavior, and is extremely hard to debug. This at least gives a clue what'is going wrong. The alternative would be an assertion.
      kDebug() << "SmartRange warning: " << this << ": Added child-range " << newChild << "(" << *newChild << ") intersects child-range " << m_childRanges[insertAt] << "(" << *m_childRanges[insertAt] << "), the old one is trimmed." << endl;
      m_childRanges[insertAt]->start() = newChild->end(); //Smartrange logic will trim all the other ranges out of the way
      if(m_childRanges[insertAt]->start() != newChild->end()) {
        m_childRanges[insertAt]->start() = newChild->end();
        Q_ASSERT(m_childRanges[insertAt]->start() == newChild->end());
      }
      for(int a = insertAt; a < m_childRanges.size(); ++a) {
        Q_ASSERT(m_childRanges[a]->start() >= newChild->end());
      }
    }
  }
  m_childRanges.insert(insertAt, newChild);
  
  #ifdef DEBUG_BINARY_SEARCH
  {
  KTextEditor::Cursor lastEnd = KTextEditor::Cursor(-1,-1);
  for(int a = 0; a < m_childRanges.size(); ++a) {
    Q_ASSERT(m_childRanges[a]->end() >= lastEnd);
    lastEnd = m_childRanges[a]->end();
  }
  }
  #endif
  
  QMutableListIterator<SmartRange*> it = m_childRanges;
  it.toBack();

  foreach (SmartRangeNotifier* n, m_notifiers)
    emit n->childRangeInserted(this, newChild);

  foreach (SmartRangeWatcher* w, m_watchers)
    w->childRangeInserted(this, newChild);
}

void SmartRange::removeChildRange(SmartRange* child)
{
  int index = findIndex(m_childRanges, child, child);
  if (index != -1) {
    m_childRanges.removeAt(index);

    foreach (SmartRangeNotifier* n, m_notifiers)
      emit n->childRangeInserted(this, child);

    foreach (SmartRangeWatcher* w, m_watchers)
      w->childRangeInserted(this, child);
  }
}

SmartRange * SmartRange::mostSpecificRange( const Range & input ) const
{
  if (!input.isValid())
    return 0L;

  if (contains(input)) {
    int child = lowerBound(m_childRanges, input.start());

    if(child != m_childRanges.size()) {
      SmartRange* r = m_childRanges[child];
      if(r->contains(input))
        return r->mostSpecificRange(input);
    }

    return const_cast<SmartRange*>(this);

  } else if (parentRange()) {
    return parentRange()->mostSpecificRange(input);

  } else {
    return 0L;
  }
}

SmartRange * SmartRange::firstRangeContaining( const Cursor & pos ) const
{
  if (!pos.isValid())
    return 0L;

  if (contains(pos)) {
    if (parentRange() && parentRange()->contains(pos))
      return parentRange()->firstRangeContaining(pos);

    return const_cast<SmartRange*>(this);

  } else {
    if (!parentRange())
      return 0L;

    return parentRange()->firstRangeContaining(pos);
  }
}

SmartRange * SmartRange::deepestRangeContaining( const Cursor & pos, QStack<SmartRange*>* rangesEntered, QStack<SmartRange*>* rangesExited ) const
{
  if (!pos.isValid()) {
    // Just leave all ranges
    if (rangesExited) {
      SmartRange* range = const_cast<SmartRange*>(this);
      while (range) {
        rangesExited->append(range);
        range = range->parentRange();
      }
    }
    return 0L;
  }

  return deepestRangeContainingInternal(pos, rangesEntered, rangesExited, true);
}

SmartRange * SmartRange::deepestRangeContainingInternal( const Cursor & pos, QStack<SmartRange*>* rangesEntered, QStack<SmartRange*>* rangesExited, bool first ) const
{
  if (contains(pos)) {
    if (!first && rangesEntered)
      rangesEntered->push(const_cast<SmartRange*>(this));

    int child = lowerBound(m_childRanges, pos);
    if(child != m_childRanges.size()) {
      SmartRange* r = m_childRanges[child];
      if(r->contains(pos))
        return r->deepestRangeContainingInternal(pos, rangesEntered, rangesExited);
    }
      
    return const_cast<SmartRange*>(this);

  } else {
    if (rangesExited)
      rangesExited->push(const_cast<SmartRange*>(this));

    if (!parentRange())
      return 0L;

    // first is true, because the parentRange won't be "entered" on first descent
    return parentRange()->deepestRangeContainingInternal(pos, rangesEntered, rangesExited, true);
  }
}

Document* SmartRange::document( ) const
{
  return smartStart().document();
}

void SmartRange::associateAction( KAction * action )
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

void SmartRange::dissociateAction( KAction * action )
{
  m_associatedActions.removeAll(action);
  if (!m_associatedActions.count())
    checkFeedback();
}

void SmartRange::clearAssociatedActions( )
{
  m_associatedActions.clear();
  checkFeedback();
}

SmartRange::InsertBehaviors SmartRange::insertBehavior( ) const
{
  return ((smartStart().insertBehavior() == SmartCursor::MoveOnInsert) ? DoNotExpand : ExpandLeft) | ((smartEnd().insertBehavior() == SmartCursor::MoveOnInsert) ? ExpandRight : DoNotExpand);
}

void SmartRange::setInsertBehavior(SmartRange::InsertBehaviors behavior)
{
  static_cast<SmartCursor*>(m_start)->setInsertBehavior((behavior & ExpandLeft) ? SmartCursor::StayOnInsert : SmartCursor::MoveOnInsert);
  static_cast<SmartCursor*>(m_end)->setInsertBehavior((behavior & ExpandRight) ? SmartCursor::MoveOnInsert : SmartCursor::StayOnInsert);
}

void SmartRange::clearChildRanges()
{
  foreach (SmartRange* r, m_childRanges)
    r->removeText();
}

void SmartRange::deleteChildRanges()
{
  // FIXME: Probably more efficient to prevent them from unlinking themselves?
  qDeleteAll(m_childRanges);

  // i.e. this is probably already clear
  m_childRanges.clear();
}

void SmartRange::clearAndDeleteChildRanges( )
{
  // FIXME: Probably more efficient to prevent them from unlinking themselves?
  foreach (SmartRange* r, m_childRanges)
    r->removeText();

  qDeleteAll(m_childRanges);

  // i.e. this is probably already clear
  m_childRanges.clear();
}

void SmartRange::setParentRange( SmartRange * r )
{
  if (m_parentRange == r)
    return;

  if (m_parentRange)
    m_parentRange->removeChildRange(this);

  SmartRange* oldParent = m_parentRange;

  m_parentRange = r;

  if (m_parentRange)
    m_parentRange->insertChildRange(this);

  foreach (SmartRangeNotifier* n, m_notifiers)
    emit n->parentRangeChanged(this, m_parentRange, oldParent);

  foreach (SmartRangeWatcher* w, m_watchers)
    w->parentRangeChanged(this, m_parentRange, oldParent);
}

void SmartRange::setAttribute( Attribute::Ptr attribute )
{
  if (attribute == m_attribute)
    return;

  Attribute::Ptr prev = m_attribute;

  m_attribute = attribute;

  foreach (SmartRangeNotifier* n, m_notifiers)
    emit n->rangeAttributeChanged(this, attribute, prev);

  foreach (SmartRangeWatcher* w, m_watchers)
    w->rangeAttributeChanged(this, attribute, prev);
}

Attribute::Ptr SmartRange::attribute( ) const
{
  return m_attribute;
}

QStringList SmartRange::text( bool block ) const
{
  return document()->textLines(*this, block);
}

bool SmartRange::replaceText( const QStringList & text, bool block )
{
  return document()->replaceText(*this, text, block);
}

bool SmartRange::removeText( bool block )
{
  return document()->removeText(*this, block);
}

void SmartRange::rangeChanged( Cursor* c, const Range& from )
{
  Range::rangeChanged(c, from);

  // Decide whether the parent range has expanded or contracted, if there is one
  if (parentRange() && (start() < from.start() || end() > from.end()))
    parentRange()->expandToRange(*this);

  // Adjust sibling ranges if required
  if (parentRange()) {
    if (SmartRange* beforeRange = parentRange()->childBefore(this)) {
      if (beforeRange->end() > start())
        beforeRange->end() = start();
    }

    if (SmartRange* afterRange = parentRange()->childAfter(this)) {
      if (afterRange->start() < end())
        afterRange->start() = end();
    }
  }

  // Contract child ranges if required
  if(!m_childRanges.isEmpty()) {
    if (start() > from.start())
      if(m_childRanges.front()->start() < start())
        m_childRanges.front()->start() = start(); //Adjust the first child-range, all the other ones will be changed automatically using this mechanism

    if (end() < from.end())
      if(m_childRanges.back()->end() > end())
        m_childRanges.back()->end() = end(); //Adjust the last child-range only, all the other ones will be changed automatically using this mechanism
  }

  // SmartCursor and its subclasses take care of adjusting ranges if the tree
  // structure is being used.
  foreach (SmartRangeNotifier* n, m_notifiers)
    if (n->wantsDirectChanges()) {
      emit n->rangePositionChanged(this);
      emit n->rangeContentsChanged(this);

      if (start() == end())
        emit n->rangeEliminated(this);
    }

  foreach (SmartRangeWatcher* w, m_watchers)
    if (w->wantsDirectChanges()) {
      w->rangePositionChanged(this);
      w->rangeContentsChanged(this);

      if (start() == end())
        w->rangeEliminated(this);
    }
}

bool SmartRange::isSmartRange( ) const
{
  return true;
}

SmartRange* SmartRange::toSmartRange( ) const
{
  return const_cast<SmartRange*>(this);
}

bool SmartRange::hasParent( SmartRange * parent ) const
{
  if (parentRange() == parent)
    return true;

  if (parentRange())
    return parentRange()->hasParent(parent);

  return false;
}

const QList< SmartRangeWatcher * > & SmartRange::watchers( ) const
{
  return m_watchers;
}

void SmartRange::addWatcher( SmartRangeWatcher * watcher )
{
  if (!m_watchers.contains(watcher))
    m_watchers.append(watcher);

  checkFeedback();
}

void SmartRange::removeWatcher( SmartRangeWatcher * watcher )
{
  m_watchers.removeAll(watcher);
  checkFeedback();
}

SmartRangeNotifier * SmartRange::primaryNotifier( )
{
  if (m_notifiers.isEmpty())
    m_notifiers.append(createNotifier());

  return m_notifiers.first();
}

const QList< SmartRangeNotifier * > SmartRange::notifiers( ) const
{
  return m_notifiers;
}

void SmartRange::addNotifier( SmartRangeNotifier * notifier )
{
  if (!m_notifiers.contains(notifier))
    m_notifiers.append(notifier);

  checkFeedback();
}

void SmartRange::removeNotifier( SmartRangeNotifier * notifier )
{
  m_notifiers.removeAll(notifier);
  checkFeedback();
}

void SmartRange::deletePrimaryNotifier( )
{
  if (m_notifiers.isEmpty())
    return;

  SmartRangeNotifier* n = m_notifiers.first();
  removeNotifier(n);
  delete n;
}

void SmartRange::checkFeedback( )
{
}

// kate: space-indent on; indent-width 2; replace-tabs on;
