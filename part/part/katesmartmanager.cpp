/* This file is part of the KDE libraries
   Copyright (C) 2005 Hamish Rodda <rodda@kde.org>

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

#include "katesmartmanager.h"

#include "katedocument.h"
#include "katesupercursor.h"
#include "katesuperrange.h"

#include <kdebug.h>

static const int s_defaultGroupSize = 4;
static const int s_minimumGroupSize = 2;
static const int s_maximumGroupSize = 6;

KateSmartManager::KateSmartManager(KateDocument* parent)
  : QObject(parent)
  , m_firstGroup(new KateSmartGroup(0, 0, 0L, 0L))
  , m_invalidGroup(new KateSmartGroup(-1, -1, 0L, 0L))
{
  connect(doc()->history(), SIGNAL(editDone(KateEditInfo*)), SLOT(slotTextChanged(KateEditInfo*)));
}

KateSmartManager::~KateSmartManager()
{
}

KateDocument * KateSmartManager::doc( ) const
{
  return static_cast<KateDocument*>(parent());
}

KTextEditor::SmartCursor * KateSmartManager::newSmartCursor( const KTextEditor::Cursor & position, bool moveOnInsert )
{
  return new KateSmartCursor(position, doc(), moveOnInsert);
}

KTextEditor::SmartRange * KateSmartManager::newSmartRange( const KTextEditor::Range & range, KTextEditor::SmartRange * parent, int insertBehaviour )
{
  return new KateSmartRange(range, doc(), parent, insertBehaviour);
}

void KateSmartManager::requestFeedback( KateSmartRange * range, int previousLevelOfFeedback)
{
  if (range->feedbackLevel() == previousLevelOfFeedback)
    return;

  switch (previousLevelOfFeedback) {
    case KateSmartRange::MostSpecificContentChanged:
      removeRangeWantingMostSpecificContentFeedback(range);
      break;

    case KateSmartRange::PositionChanged:
      removePositionRange(range);
      break;
  }

  switch (range->feedbackLevel()) {
    case KateSmartRange::MostSpecificContentChanged:
      addRangeWantingMostSpecificContentFeedback(range);
      break;

    case KateSmartRange::PositionChanged:
      addPositionRange(range);
      break;
  }
}

void KateSmartManager::addPositionRange( KateSmartRange * range )
{
  const KateSmartCursor* start = static_cast<const KateSmartCursor*>(&range->start());
  start->smartGroup()->addStartingPositionRange(range);
}

void KateSmartManager::removePositionRange( KateSmartRange * range )
{
  const KateSmartCursor* start = static_cast<const KateSmartCursor*>(&range->start());
  start->smartGroup()->removeStartingPositionRange(range);
}

void KateSmartManager::addRangeWantingMostSpecificContentFeedback( KateSmartRange * range )
{
  m_specificRanges.insert(range);
}

void KateSmartManager::removeRangeWantingMostSpecificContentFeedback( KateSmartRange * range )
{
  m_specificRanges.remove(range);
}

void KateSmartGroup::addCursor( KateSmartCursor * cursor)
{
  if (cursor->feedbackEnabled())
    m_feedbackCursors.insert(cursor);
  else
    m_normalCursors.insert(cursor);
}

void KateSmartGroup::changeCursorFeedback( KateSmartCursor * cursor )
{
  if (cursor->feedbackEnabled()) {
    m_normalCursors.remove(cursor);
    m_feedbackCursors.insert(cursor);
  } else {
    m_feedbackCursors.remove(cursor);
    m_normalCursors.insert(cursor);
  }
}

void KateSmartGroup::removeCursor( KateSmartCursor * cursor)
{
  if (cursor->feedbackEnabled())
    m_feedbackCursors.remove(cursor);
  else
    m_normalCursors.remove(cursor);
}

void KateSmartGroup::addTraversingRange( KateSmartRange * range )
{
  m_rangesTraversing.insert(range);
}

void KateSmartGroup::removeTraversingRange( KateSmartRange * range )
{
  m_rangesTraversing.remove(range);
}

void KateSmartGroup::addStartingPositionRange( KateSmartRange * range )
{
  m_rangesStartingPosition.insert(range);
}

void KateSmartGroup::removeStartingPositionRange( KateSmartRange * range )
{
  m_rangesStartingPosition.remove(range);
}

void KateSmartGroup::joined( KateSmartCursor * cursor )
{
  addCursor(cursor);

  if (KateSmartRange* range = static_cast<KateSmartRange*>(cursor->belongsToRange()))
    range->kateDocument()->smartManager()->requestFeedback(range, KateSmartRange::NoFeedback);
}

void KateSmartGroup::leaving( KateSmartCursor * cursor )
{
  removeCursor(cursor);

  if (KateSmartRange* range = static_cast<KateSmartRange*>(cursor->belongsToRange())) {
    int currentFeedback = range->feedbackLevel();
    range->setFeedbackLevel(KateSmartRange::NoFeedback, false);
    range->kateDocument()->smartManager()->requestFeedback(range, currentFeedback);
    range->setFeedbackLevel(currentFeedback, false);
  }
}

KateSmartGroup * KateSmartManager::groupForLine( int line ) const
{
  // Special case
  if (line == -1)
    return m_invalidGroup;

  // FIXME maybe this should perform a bit better
  KateSmartGroup* smartGroup = m_firstGroup;
  while (smartGroup && !smartGroup->containsLine(line))
    smartGroup = smartGroup->next();

  Q_ASSERT(smartGroup);
  return smartGroup;
}

void KateSmartManager::slotTextChanged(KateEditInfo* edit)
{
  KateSmartGroup* firstSmartGroup = groupForLine(edit->oldRange().start().line());
  KateSmartGroup* currentGroup = firstSmartGroup;

  // Check to see if we need to split or consolidate
  int splitEndLine = edit->translate().line() + firstSmartGroup->endLine();
  if (edit->translate().line() >= 0) {
    //kdDebug() << k_funcinfo << "Need to translate smartGroups by " << edit->translate().line() << " line(s); startLine " << firstSmartGroup->startLine() << " endLine " << firstSmartGroup->endLine() << " splitEndLine " << splitEndLine << "." << endl;
    KateSmartGroup* endGroup = currentGroup->next();
    int currentCanExpand = endGroup ? s_maximumGroupSize - currentGroup->length() : s_defaultGroupSize - currentGroup->length();
    //int nextCanExpand = currentGroup->next() ? (endGroup ? s_maximumGroupSize - currentGroup->next()->length() : s_defaultGroupSize - currentGroup->next()->length()) : 0;

    if (currentCanExpand >= edit->translate().line()) {
      // Current group can expand to accommodate the extra lines
      currentGroup->setEndLine(currentGroup->endLine() + edit->translate().line());

    /*} else if (nextCanExpand >= edit->translate().line()) {
      // Next group can expand to accommodate the extra lines
      currentGroup->next()->setStartLine(currentGroup->next()->startLine() - 1);*/

    } else {
      // Need at least one new group
      int newStartLine, newEndLine;

      // Trying to get the debugger to stop before the loop
      newStartLine = 0;

      do {
        newStartLine = currentGroup->endLine() + 1;
        newEndLine = QMIN(newStartLine + s_defaultGroupSize - 1, splitEndLine);
        //kdDebug() << k_funcinfo << "NewStartLine " << newStartLine << " NewEndLine " << newEndLine << endl;
        currentGroup = new KateSmartGroup(newStartLine, newEndLine, currentGroup, endGroup);
      } while (newEndLine < splitEndLine);
    }


  } else {
    // might need to consolitate
    while (currentGroup->next() && currentGroup->length() - edit->translate().line() < s_minimumGroupSize)
      currentGroup->merge();
  }

  QLinkedList<KateSmartRange*> changedRanges;

  // Translate affected groups
  bool changed = true;
  currentGroup->translateChanged(*edit, changedRanges, true);

  for (KateSmartGroup* smartGroup = currentGroup->next(); smartGroup; smartGroup = smartGroup->next()) {
    if (changed)
      changed = smartGroup->endLine() <= edit->oldRange().end().line(); // + edit->translate().line()

    if (changed)
      smartGroup->translateChanged(*edit, changedRanges, false);
    else
      smartGroup->translateShifted(*edit);
  }

  foreach (KateSmartRange* range, changedRanges)
    range->translated(*edit);

  for (KateSmartGroup* smartGroup = firstSmartGroup->next(); smartGroup; smartGroup = smartGroup->next())
    smartGroup->translated(*edit);

  //debugOutput();
}

void KateSmartGroup::translateChanged( const KateEditInfo& edit, QLinkedList< KateSmartRange * > & m_ranges, bool first )
{
  //kdDebug() << k_funcinfo << edit.oldRange().start().line() << "," << edit.oldRange().start().column() << " was to " << edit.oldRange().end().line() << "," << edit.oldRange().end().column() << " now to " << edit.newRange().end().line() << "," << edit.newRange().end().column() << " numcursors feedback " << m_feedbackCursors.count() << " normal " << m_normalCursors.count() << endl;

  if (!first)
    translateShifted(edit);

  foreach (KateSmartCursor* cursor, m_feedbackCursors)
    if (cursor->translate(edit))
      if (cursor->belongsToRange())
        m_ranges.append(static_cast<KateSmartRange*>(cursor->belongsToRange()));

  foreach (KateSmartCursor* cursor, m_normalCursors)
    if (cursor->translate(edit))
      if (cursor->belongsToRange())
        m_ranges.append(static_cast<KateSmartRange*>(cursor->belongsToRange()));
}

void KateSmartGroup::translateShifted(const KateEditInfo& edit)
{
  m_newStartLine = QMAX(m_startLine, m_startLine + edit.translate().line());
}

void KateSmartGroup::translated(const KateEditInfo& edit)
{
  if (m_startLine != m_newStartLine) {
    m_startLine = m_newStartLine;
    m_endLine += edit.translate().line();
  }

  // Todo: don't need to provide positionChanged to all feedback cursors?
  foreach (KateSmartCursor* cursor, m_feedbackCursors)
    cursor->translated();

  foreach (KateSmartRange* range, m_rangesStartingPosition)
    range->translated(edit);
}

KateSmartGroup::KateSmartGroup( int startLine, int endLine, KateSmartGroup * previous, KateSmartGroup * next )
  : m_startLine(startLine)
  , m_newStartLine(startLine)
  , m_endLine(endLine)
  , m_next(next)
  , m_previous(previous)
{
  if (m_previous)
    m_previous->setNext(this);

  if (m_next)
    m_next->setPrevious(this);
}

void KateSmartGroup::merge( )
{
  Q_ASSERT(m_next);

  foreach (KateSmartCursor* cursor, next()->feedbackCursors())
    cursor->migrate(this);
  m_feedbackCursors += next()->feedbackCursors();

  foreach (KateSmartCursor* cursor, next()->normalCursors())
    cursor->migrate(this);
  m_normalCursors += next()->normalCursors();

  m_rangesTraversing += next()->rangesTraversing();
  m_rangesStartingPosition += next()->rangesStaringPosition();

  m_endLine = next()->endLine();
  KateSmartGroup* newNext = next()->next();
  delete m_next;
  m_next = newNext;
}

const QSet< KateSmartCursor * > & KateSmartGroup::feedbackCursors( ) const
{
  return m_feedbackCursors;
}

const QSet< KateSmartCursor * > & KateSmartGroup::normalCursors( ) const
{
  return m_normalCursors;
}

const QSet< KateSmartRange * > & KateSmartGroup::rangesTraversing( ) const
{
  return m_rangesTraversing;
}

const QSet< KateSmartRange * > & KateSmartGroup::rangesStaringPosition( ) const
{
  return m_rangesTraversing;
}

void KateSmartManager::debugOutput( ) const
{
  int groupCount = 1;
  KateSmartGroup* currentGroup = m_firstGroup;
  while (currentGroup->next()) {
    ++groupCount;
    currentGroup = currentGroup->next();
  }

  //kdDebug() << "KateSmartManager: SmartGroups " << groupCount << " from " << m_firstGroup->startLine() << " to " << currentGroup->endLine() << "; Specific Ranges " << m_specificRanges.count() << endl;

  currentGroup = m_firstGroup;
  while (currentGroup) {
    currentGroup->debugOutput();
    currentGroup = currentGroup->next();
  }
}

void KateSmartGroup::debugOutput( ) const
{
  kdDebug() << " -> KateSmartGroup: from " << startLine() << " to " << endLine() << "; Cursors " << m_normalCursors.count() + m_feedbackCursors.count() << " (" << m_feedbackCursors.count() << " feedback), Ranges Traversing " << m_rangesTraversing.count() << ", Starting+Feedback " << m_rangesStartingPosition.count() << endl;
}

#include "katesmartmanager.moc"
