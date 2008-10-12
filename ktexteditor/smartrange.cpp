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

//Uncomment this to enable debugging of the child-order. If it is enabled, an assertion will
//be triggered when the order is violated.
//This is slow.
//   #define SHOULD_DEBUG_CHILD_ORDER

//Uncomment this to debug the m_overlapCount values. When it is enabled,
//extensive tests will be done to verify that the values are true,
//and an assertion is triggered when not.
//This is very slow, especially with many child-ranges.
//   #define SHOULD_DEBUG_OVERLAP

#ifdef SHOULD_DEBUG_CHILD_ORDER
#define DEBUG_CHILD_ORDER \
  {\
  KTextEditor::Cursor lastEnd = KTextEditor::Cursor(-1,-1);\
  for(int a = 0; a < m_childRanges.size(); ++a) {\
    Q_ASSERT(m_childRanges[a]->end() >= lastEnd);\
    lastEnd = m_childRanges[a]->end();\
  }\
  }\
  
#else
#define DEBUG_CHILD_ORDER
#endif

#ifdef SHOULD_DEBUG_OVERLAP
#define DEBUG_CHILD_OVERLAP  \
{\
  QVector<int> counter(m_childRanges.size(), 0);\
\
  for(int a = 0; a < m_childRanges.size(); ++a) {\
    const SmartRange& overlapper(*m_childRanges[a]);\
    for(int b = 0; b < a; ++b) {\
      const SmartRange& overlapped(*m_childRanges[b]);\
      Q_ASSERT(overlapped.end() <= overlapper.end());\
      if(overlapped.end() > overlapper.start()) {\
        ++counter[b];\
      }\
    }\
  }\
  for(int a = 0; a < m_childRanges.size(); ++a) {\
    Q_ASSERT(m_childRanges[a]->m_overlapCount == counter[a]);\
  }\
}\

#define DEBUG_PARENT_OVERLAP  \
if(m_parentRange) {\
  QVector<int> counter(m_parentRange->m_childRanges.size(), 0);\
\
  for(int a = 0; a < m_parentRange->m_childRanges.size(); ++a) {\
    const SmartRange& overlapper(*m_parentRange->m_childRanges[a]);\
    for(int b = 0; b < a; ++b) {\
      const SmartRange& overlapped(*m_parentRange->m_childRanges[b]);\
      Q_ASSERT(overlapped.end() <= overlapper.end());\
      if(overlapped.end() > overlapper.start()) {\
        ++counter[b];\
      }\
    }\
  }\
  for(int a = 0; a < m_parentRange->m_childRanges.size(); ++a) {\
    Q_ASSERT(m_parentRange->m_childRanges[a]->m_overlapCount == counter[a]);\
  }\
}\

#else
#define DEBUG_CHILD_OVERLAP
#define DEBUG_PARENT_OVERLAP
#endif

///Returns the index of the first range that ends behind @param pos
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

///Searches for the given range, or a lower bound for the given position. Does not work correctly in case of overlaps,
///but for that case we have a fallback. Only for use in findIndex.
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
  , m_overlapCount(0)
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
  
  DEBUG_CHILD_OVERLAP
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
  DEBUG_CHILD_OVERLAP

  Q_ASSERT(newChild->parentRange() == this);

  // A new child has been added, so expand this range if required.
  expandToRange(*newChild);
  
  DEBUG_CHILD_ORDER

  int insertAt = lowerBound(m_childRanges, newChild->end());
  m_childRanges.insert(insertAt, newChild);
  
  //Increase the overlap of previous ranges
  for(int current = insertAt-1; current >= 0; --current) {
    SmartRange& range(*m_childRanges[current]);
    Q_ASSERT(range.end() <= newChild->end());
    
    if(range.end() > newChild->start()) {
      ++range.m_overlapCount;
    }else{
      //range.end() <= start(), The range does not overlap, and the same applies for all earlier ranges
      break;
    }
  }

  //Increase this ranges overlap from already existing ranges
  for(int current = insertAt+1; current < m_childRanges.size(); ++current) {
    SmartRange& range(*m_childRanges[current]);
    Q_ASSERT(range.end() >= newChild->end());
    
    if(range.start() < newChild->end())
      ++newChild->m_overlapCount; //The range overlaps newChild
    
    if(!range.m_overlapCount)
      break; //If this follower-range isn't overlapped by any other ranges, we can break here.
  }

  DEBUG_CHILD_OVERLAP

  DEBUG_CHILD_ORDER
  
  QMutableListIterator<SmartRange*> it = m_childRanges;
  it.toBack();

  foreach (SmartRangeNotifier* n, m_notifiers)
    emit n->childRangeInserted(this, newChild);

  foreach (SmartRangeWatcher* w, m_watchers)
    w->childRangeInserted(this, newChild);
  
  DEBUG_CHILD_OVERLAP
  DEBUG_CHILD_ORDER
}

void SmartRange::removeChildRange(SmartRange* child)
{
  DEBUG_CHILD_OVERLAP
  
  int index = findIndex(m_childRanges, child, child);
  if (index != -1) {
    m_childRanges.removeAt(index);

    //Reduce the overlap with all previously overlapping ranges(parentChildren is still sorted by the old end-position)
    for(int current = index-1; current >= 0; --current) {
      SmartRange& range(*m_childRanges[current]);
      Q_ASSERT(range.end() <= child->end());
      if(range.end() <= child->start()) {
        break; //This range did not overlap before, the same applies for all earlier ranges because of the order
      }else{
        if(range.m_overlapCount) {
          --range.m_overlapCount;
        } else {
          //May happen with more than 64 overlaps
#ifdef SHOULD_DEBUG_OVERLAP
          Q_ASSERT(0);
#endif
        }
      }
    }
    
    DEBUG_CHILD_OVERLAP
    
    child->m_overlapCount = 0; //It has no neighbors any more, so no overlap
    
    foreach (SmartRangeNotifier* n, m_notifiers)
      emit n->childRangeInserted(this, child);

    foreach (SmartRangeWatcher* w, m_watchers)
      w->childRangeInserted(this, child);
  }

  DEBUG_CHILD_OVERLAP
}

SmartRange * SmartRange::mostSpecificRange( const Range & input ) const
{
  if (!input.isValid())
    return 0L;

  if (contains(input)) {
    int child = lowerBound(m_childRanges, input.start());

    SmartRange* mostSpecific = 0;
    
    while(child != m_childRanges.size()) {
      SmartRange* r = m_childRanges[child];
      if(r->contains(input)) {
        SmartRange* candidate = r->mostSpecificRange(input);
        if(mostSpecific == 0 ||
          ((candidate->end() - candidate->start()) < (mostSpecific->end() - mostSpecific->start())) ||
          (candidate->end() < mostSpecific->end()))
          mostSpecific = candidate;
      }
      
      if(r->m_overlapCount == 0)
        break;
      
      ++child; //We have to iterate as long as there is overlapping ranges
    }

    if(mostSpecific)
      return mostSpecific;
    else
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

int SmartRange::overlapCount() const
{
  return m_overlapCount;
}

QList<SmartRange*> SmartRange::deepestRangesContaining(const Cursor& pos) const
{
  QList<SmartRange*> ret;
  
  if(!contains(pos))
    return ret;
  
  int child = lowerBound(m_childRanges, pos);
  
  while(child != m_childRanges.size()) {
    SmartRange* r = m_childRanges[child];
    //The list will be unchanged if the range doesn't contain the position
    ret += r->deepestRangesContaining(pos);
    
    if(r->m_overlapCount == 0)
      break;
    
    ++child; //We have to iterate as long as there is overlapping ranges
  }
  
  if(!ret.isEmpty())
    return ret;
  else
    return ret << const_cast<SmartRange*>(this);
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

    QStack<SmartRange*> mostSpecificStack;
    SmartRange* mostSpecific = 0;
    
    while(child != m_childRanges.size()) {
      SmartRange* r = m_childRanges[child];
      if(r->contains(pos)) {
        QStack<SmartRange*> candidateStack;
        SmartRange* candidateRange = r->deepestRangeContainingInternal(pos, rangesEntered ? &candidateStack : 0, 0);
        
        Q_ASSERT(!rangesEntered || !candidateStack.isEmpty());
        Q_ASSERT(candidateRange);
        
        if(!mostSpecific ||
          ((candidateRange->end() - candidateRange->start()) < (mostSpecific->end() - mostSpecific->start())) ||
          (candidateRange->end() < mostSpecific->end())) {
          mostSpecific = candidateRange;
          mostSpecificStack = candidateStack;
        }
      }
      
      if(r->m_overlapCount == 0)
        break;
      
      ++child; //We have to iterate as long as there is overlapping ranges
    }
    
    if(mostSpecific) {
      if(rangesEntered)
        *rangesEntered += mostSpecificStack;
      return mostSpecific;
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
  
  DEBUG_PARENT_OVERLAP

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
  
  DEBUG_PARENT_OVERLAP
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

static bool rangeEndLessThan(const SmartRange* s1, const SmartRange* s2) {
  return s1->end() < s2->end();
}

void SmartRange::rebuildChildStructure() {
  
  ///Re-order
  qStableSort(m_childRanges.begin(), m_childRanges.end(), rangeEndLessThan);
  DEBUG_CHILD_ORDER
  ///Update overlap
  for(int a = 0; a < m_childRanges.size(); ++a) {
    SmartRange& overlapper(*m_childRanges[a]);
    overlapper.m_overlapCount = 0;
    
    //Increase the overlap of overlapped ranegs
    for(int current = a-1; current >= 0; --current) {
      SmartRange& range(*m_childRanges[current]);
      Q_ASSERT(range.end() <= overlapper.end());
      
      if(range.end() > overlapper.start()) {
        ++range.m_overlapCount;
      }else{
        //range.end() <= start(), The range does not overlap, and the same applies for all earlier ranges
        break;
      }
    }
  }
  
  DEBUG_CHILD_OVERLAP
}

void SmartRange::rangeChanged( Cursor* c, const Range& from )
{
#ifdef SHOULD_DEBUG_CHILD_ORDER
  if (parentRange() ) {
    //Make sure the child-order is correct, in respect to "from"
    QList<SmartRange*>& parentChildren(parentRange()->m_childRanges);
    
    int index = findIndex(parentChildren, this, &from);
    Q_ASSERT(index != -1);
    Q_ASSERT(parentChildren[index] == this);
    const Range* lastRange = 0;
    for(int a = 0; a < index; ++a) {
      if(lastRange) {
        Q_ASSERT(lastRange->end() <= parentChildren[a]->end());
      }
      lastRange = parentChildren[a];
    }
    
    if(lastRange) {
      Q_ASSERT(lastRange->end() <= from.end());
    }
    
    if(index+1 < parentChildren.size()) {
      Q_ASSERT(from.end() <= parentChildren[index+1]->end());
    }
    lastRange = &from;
    
    for(int a = index+1; a < parentChildren.size(); ++a) {
      if(lastRange) {
        Q_ASSERT(lastRange->end() <= parentChildren[a]->end());
      }
      lastRange = parentChildren[a];
    }
  }
#endif
  Range::rangeChanged(c, from);

  // Decide whether the parent range has expanded or contracted, if there is one
  if (parentRange() ) {
    QList<SmartRange*>& parentChildren(parentRange()->m_childRanges);
    
    int index = findIndex(parentChildren, this, &from);
    Q_ASSERT(index != -1);
    Q_ASSERT(parentChildren[index] == this);
    
    //Reduce the overlap with all previously overlapping ranges(parentChildren is still sorted by the old end-position)
    for(int current = index-1; current >= 0; --current) {
      SmartRange& range(*parentChildren[current]);
      Q_ASSERT(range.end() <= from.end());
      if(range.end() <= from.start()) {
//         break; //This range did not overlap before, the same applies for all earlier ranges because of the order
      }else{
        if(range.m_overlapCount) {
          --range.m_overlapCount;
        }else{
#ifdef SHOULD_DEBUG_OVERLAP
          Q_ASSERT(0);
#endif
        }
      }
    }
    
  //Decrease this ranges overlap from existing ranges behind, since it may be moved so it isn't overlapped any more
  for(int current = index+1; current < parentChildren.size(); ++current) {
    SmartRange& range(*parentChildren[current]);
    Q_ASSERT(range.end() >= from.end());
    
    if(range.start() < from.end())
      --m_overlapCount; //The range overlaps newChild
    
    if(!range.m_overlapCount)
      break; //If this follower-range isn't overlapped by any other ranges, we can break here.
  }
    
    if(from.end() != end()) {
      //Update the order in the parent, the ranges must be strictly sorted
      if(from.end() > end()) {
        //Bubble backwards, the position has been reduced
        while(index > 0 && parentChildren[index-1]->end() > end()) {
          parentChildren[index] = parentChildren[index-1];
          parentChildren[index-1] = this;
          --index;
        }
      }else{
        //Bubble forwards, the position has moved forwards
        while( index+1 < parentChildren.size() && (parentChildren[index+1]->end() < end()) ) {
          parentChildren[index] = parentChildren[index+1];
          parentChildren[index+1] = this;
          ++index;
        }
      }
    }
    Q_ASSERT(parentChildren[index] == this);
    
    //Increase the overlap
    for(int current = index-1; current >= 0; --current) {
      SmartRange& range(*parentChildren[current]);
      Q_ASSERT(range.end() <= end());
      
      if(range.end() > start()) {
        ++range.m_overlapCount;
      }else{
        //range.end() <= start(), The range does not overlap, and the same applies for all earlier ranges
        break;
      }
    }
    
  //Increase this ranges overlap from existing ranges behind, since it may have been moved
  for(int current = index+1; current < parentChildren.size(); ++current) {
    SmartRange& range(*parentChildren[current]);
    Q_ASSERT(range.end() >= end());
    
    if(range.start() < end())
      ++m_overlapCount; //The range overlaps newChild
    
    if(!range.m_overlapCount)
      break; //If this follower-range isn't overlapped by any other ranges, we can break here.
  }
    
  DEBUG_CHILD_ORDER
  DEBUG_PARENT_OVERLAP
  
  //Expand the parent in the end, so the overlap is consistent when the parent gets control
  if ((start() < from.start() || end() > from.end()))
    parentRange()->expandToRange(*this);
  }
  
  DEBUG_CHILD_OVERLAP

  // Contract child ranges if required
  if(!m_childRanges.isEmpty()) {
    if (start() > from.start()) {
      foreach(SmartRange* child, m_childRanges) {
        if(child->start() < start())
          child->start() = start();
        else if(!child->m_overlapCount)
          break; //We can safely break here, because the child is not overlapped
      }
    }

    if (end() < from.end()) {
      for(int a = m_childRanges.size()-1; a >= 0; --a) {
        if(m_childRanges[a]->end() <= end())
          break; //Child-ranges are sorted by the end-cursor, so we can just break here
        m_childRanges[a]->end() = end();
      }
    }
  }

  DEBUG_CHILD_OVERLAP
  
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
