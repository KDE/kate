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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "katesmartmanager.h"

#include "katedocument.h"
#include "katesmartcursor.h"
#include "katesmartrange.h"

#include <QThread>
#include <QMutexLocker>

#include <kdebug.h>

static const int s_defaultGroupSize = 40;
static const int s_minimumGroupSize = 20;
static const int s_maximumGroupSize = 60;

using namespace KTextEditor;

KateSmartManager::KateSmartManager(KateDocument* parent)
  : QObject(parent)
  , m_firstGroup(new KateSmartGroup(0, 0, 0L, 0L))
  , m_invalidGroup(new KateSmartGroup(-1, -1, 0L, 0L))
  , m_clearing(false)
{
  connect(doc()->history(), SIGNAL(editDone(KateEditInfo*)), SLOT(slotTextChanged(KateEditInfo*)));
  //connect(doc(), SIGNAL(textChanged(Document*)), SLOT(verifyCorrect()));
}

KateSmartManager::~KateSmartManager()
{
  clear(true);

  KateSmartGroup* smartGroup = m_firstGroup;
  while (smartGroup) {
    KateSmartGroup* toDelete = smartGroup;
    smartGroup = smartGroup->next();
    delete toDelete;
  }

  delete m_invalidGroup;
}

KateDocument * KateSmartManager::doc( ) const
{
  return static_cast<KateDocument*>(parent());
}

KateSmartCursor * KateSmartManager::newSmartCursor( const Cursor & position, SmartCursor::InsertBehavior insertBehavior, bool internal )
{
  QMutexLocker l(internal ? doc()->smartMutex() : 0);

  KateSmartCursor* c;
  if (usingRevision() != -1 && !internal)
    c = new KateSmartCursor(translateFromRevision(position), doc(), insertBehavior);
  else
    c = new KateSmartCursor(position, doc(), insertBehavior);

  if (internal)
    c->setInternal();
  return c;
}

KateSmartRange * KateSmartManager::newSmartRange( const Range & range, SmartRange * parent, SmartRange::InsertBehaviors insertBehavior, bool internal )
{
  QMutexLocker l(internal ? doc()->smartMutex() : 0);

  KateSmartRange* newRange;

  if (usingRevision() != -1 && !internal)
    newRange = new KateSmartRange(translateFromRevision(range), doc(), parent, insertBehavior);
  else
    newRange = new KateSmartRange(range, doc(), parent, insertBehavior);
  
  if (internal)
    newRange->setInternal();
  if (!parent)
    rangeLostParent(newRange);
  return newRange;
}

KateSmartRange * KateSmartManager::newSmartRange( KateSmartCursor * start, KateSmartCursor * end, SmartRange * parent, SmartRange::InsertBehaviors insertBehavior, bool internal )
{
  QMutexLocker l(internal ? doc()->smartMutex() : 0);

//Why translate "smart" cursors? They should translate automatically!
//   if (usingRevision() != -1 && !internal) {
//     KTextEditor::Cursor tempStart = translateFromRevision(*start, (insertBehavior & SmartRange::ExpandLeft) ? SmartCursor::StayOnInsert : SmartCursor::MoveOnInsert);
//     KTextEditor::Cursor tempEnd = translateFromRevision(*end, (insertBehavior & SmartRange::ExpandRight) ? SmartCursor::MoveOnInsert : SmartCursor::StayOnInsert);
//     *start = tempStart;
//     *end = tempEnd;
//   }

  KateSmartRange* newRange = new KateSmartRange(start, end, parent, insertBehavior);
  if (internal)
    newRange->setInternal();
  if (!parent)
    rangeLostParent(newRange);
  return newRange;
}

void KateSmartGroup::addCursor( KateSmartCursor * cursor)
{
  Q_ASSERT(!m_feedbackCursors.contains(cursor));
  Q_ASSERT(!m_normalCursors.contains(cursor));

  if (cursor->feedbackEnabled())
    m_feedbackCursors.insert(cursor);
  else
    m_normalCursors.insert(cursor);
}

void KateSmartGroup::changeCursorFeedback( KateSmartCursor * cursor )
{
  if (!cursor->feedbackEnabled()) {
    Q_ASSERT(!m_feedbackCursors.contains(cursor));
    Q_ASSERT(m_normalCursors.contains(cursor));
    m_normalCursors.remove(cursor);
    m_feedbackCursors.insert(cursor);

  } else {
    Q_ASSERT(m_feedbackCursors.contains(cursor));
    Q_ASSERT(!m_normalCursors.contains(cursor));
    m_feedbackCursors.remove(cursor);
    m_normalCursors.insert(cursor);
  }
}

void KateSmartGroup::removeCursor( KateSmartCursor * cursor)
{
  if (cursor->feedbackEnabled()) {
    Q_ASSERT(m_feedbackCursors.contains(cursor));
    Q_ASSERT(!m_normalCursors.contains(cursor));
    m_feedbackCursors.remove(cursor);

  } else {
    Q_ASSERT(!m_feedbackCursors.contains(cursor));
    Q_ASSERT(m_normalCursors.contains(cursor));
    m_normalCursors.remove(cursor);
  }
}

void KateSmartGroup::joined( KateSmartCursor * cursor )
{
  addCursor(cursor);
}

void KateSmartGroup::leaving( KateSmartCursor * cursor )
{
  removeCursor(cursor);
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

  // If you hit this assert, it is a fundamental bug in katepart.  A cursor's
  // position is being set beyond the end of the document, or (perhaps less
  // likely), in this class itself.
  //
  // Please figure out how to reproduce, and report to rodda@kde.org.
  Q_ASSERT(smartGroup);
  return smartGroup;
}

void KateSmartManager::slotTextChanged(KateEditInfo* edit)
{
  QMutexLocker lock(doc()->smartMutex());

  KateSmartGroup* firstSmartGroup = groupForLine(edit->oldRange().start().line());
  KateSmartGroup* currentGroup = firstSmartGroup;

  // Check to see if we need to split or consolidate
  int splitEndLine = edit->translate().line() + firstSmartGroup->endLine();

  if (edit->translate().line() > 0) {
    // May need to expand smart groups
    KateSmartGroup* endGroup = currentGroup->next();

    int currentCanExpand = endGroup ? s_maximumGroupSize - currentGroup->length() : s_defaultGroupSize - currentGroup->length();
    int expanded = 0;

    if (currentCanExpand) {
       int expandBy = qMin(edit->translate().line(), currentCanExpand);
      // Current group can expand to accommodate the extra lines
      currentGroup->setNewEndLine(currentGroup->endLine() + expandBy);

      expanded = expandBy;
    }

    if (expanded < edit->translate().line()) {
      // Need at least one new group
      int newStartLine, newEndLine;

      do {
        newStartLine = currentGroup->newEndLine() + 1;
        newEndLine = qMin(newStartLine + s_defaultGroupSize - 1, splitEndLine);
        currentGroup = new KateSmartGroup(newStartLine, newEndLine, currentGroup, endGroup);

      } while (newEndLine < splitEndLine);
    }


  } else if (edit->translate().line() < 0) {
    // Might need to consolitate
    // Consolidate groups together while keeping the end result the same
    while (currentGroup->next() && currentGroup->length() + edit->translate().line() < s_minimumGroupSize)
      currentGroup->merge();

    // Reduce the size of the current group
    currentGroup->setNewEndLine(currentGroup->endLine() + edit->translate().line());
  }

  // Shift the groups so they have their new start and end lines
  if (edit->translate().line())
    for (KateSmartGroup* smartGroup = currentGroup->next(); smartGroup; smartGroup = smartGroup->next())
      smartGroup->translateShifted(*edit);

  // Translate affected groups
  for (KateSmartGroup* smartGroup = firstSmartGroup; smartGroup; smartGroup = smartGroup->next()) {
    if (smartGroup->startLine() > edit->oldRange().end().line())
      break;

    smartGroup->translateChanged(*edit);
  }

  // Cursor feedback
  bool groupChanged = true;
  for (KateSmartGroup* smartGroup = firstSmartGroup; smartGroup; smartGroup = smartGroup->next()) {
    if (groupChanged) {
      groupChanged = smartGroup->startLine() <= edit->oldRange().end().line();
      // Don't continue iterating if no line translation occurred.
      if (!groupChanged && !edit->translate().line())
        break;
    }

    if (groupChanged)
      smartGroup->translatedChanged(*edit);
    else
      smartGroup->translatedShifted(*edit);
  }

  // Range feedback
  foreach (KateSmartRange* range, m_topRanges) {
    KateSmartRange* mostSpecific = feedbackRange(*edit, range);

    if (!mostSpecific)
      mostSpecific = range;
    range->feedbackMostSpecific(mostSpecific);
  }

  //debugOutput();
  //verifyCorrect();
}

KateSmartRange* KateSmartManager::feedbackRange( const KateEditInfo& edit, KateSmartRange * range )
{
  KateSmartRange* mostSpecific = 0L;

  // This range preceeds the edit... no more to do
  if (range->end() < edit.start() || range->end() == edit.start() && !range->isEmpty()) {
    //kDebug() << "Not feeding back to " << *range << "as edit start" << edit.start();
    return mostSpecific;
  }

  foreach (SmartRange* child, range->childRanges())
    if (!mostSpecific)
      mostSpecific = feedbackRange(edit, static_cast<KateSmartRange*>(child));
    else
      feedbackRange(edit, static_cast<KateSmartRange*>(child));

  //kDebug() << "edit" << edit.oldRange() << edit.newRange() << "last at" << range->kStart().lastPosition() << range->kEnd().lastPosition() << "now" << *range;

  if (range->start() > edit.newRange().end() ||
      (range->start() == edit.newRange().end() && range->kStart().lastPosition() == edit.oldRange().end()))
  {
    // This range is after the edit... has only been shifted
    //kDebug() << "Feeding back shifted to " << *range;
    range->shifted();

  } else {
    // This range is within the edit.
    //kDebug() << "Feeding back translated to " << *range;
    if (!mostSpecific)
      if (range->start() < edit.oldRange().start() && range->end() > edit.oldRange().end())
        mostSpecific = range;

    range->translated(edit);
  }

  return mostSpecific;
}


void KateSmartGroup::translateChanged( const KateEditInfo& edit)
{
  //kDebug() << "Was " << edit.oldRange() << " now " << edit.newRange() << " numcursors feedback " << m_feedbackCursors.count() << " normal " << m_normalCursors.count();

  foreach (KateSmartCursor* cursor, m_feedbackCursors)
    cursor->translate(edit);
    
  foreach (KateSmartCursor* cursor, m_normalCursors)
    cursor->translate(edit);
}

void KateSmartGroup::translateShifted(const KateEditInfo& edit)
{
  m_newStartLine = m_startLine + edit.translate().line();
  m_newEndLine = m_endLine + edit.translate().line();
}

void KateSmartGroup::translatedChanged(const KateEditInfo& edit)
{
  m_startLine = m_newStartLine;
  m_endLine = m_newEndLine;

  foreach (KateSmartCursor* cursor, m_feedbackCursors)
    cursor->translated(edit);
}

void KateSmartGroup::translatedShifted(const KateEditInfo& edit)
{
  if (m_startLine != m_newStartLine) {
    m_startLine = m_newStartLine;
    m_endLine = m_newEndLine;
  }

  if (edit.translate().line() == 0)
    return;

  // Todo: don't need to provide positionChanged to all feedback cursors?
  foreach (KateSmartCursor* cursor, m_feedbackCursors)
    cursor->shifted();
}

KateSmartGroup::KateSmartGroup( int startLine, int endLine, KateSmartGroup * previous, KateSmartGroup * next )
  : m_startLine(startLine)
  , m_newStartLine(startLine)
  , m_endLine(endLine)
  , m_newEndLine(endLine)
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

  m_newEndLine = m_endLine = next()->endLine();
  KateSmartGroup* newNext = next()->next();
  delete m_next;
  m_next = newNext;
  if (m_next)
    m_next->setPrevious(this);
}

const QSet< KateSmartCursor * > & KateSmartGroup::feedbackCursors( ) const
{
  return m_feedbackCursors;
}

const QSet< KateSmartCursor * > & KateSmartGroup::normalCursors( ) const
{
  return m_normalCursors;
}

void KateSmartManager::debugOutput( ) const
{
  int groupCount = 1;
  KateSmartGroup* currentGroup = m_firstGroup;
  while (currentGroup->next()) {
    ++groupCount;
    currentGroup = currentGroup->next();
  }

  kDebug() << "KateSmartManager: SmartGroups " << groupCount << " from " << m_firstGroup->startLine() << " to " << currentGroup->endLine();

  currentGroup = m_firstGroup;
  while (currentGroup) {
    currentGroup->debugOutput();
    currentGroup = currentGroup->next();
  }
}

void KateSmartGroup::debugOutput( ) const
{
  kDebug() << " -> KateSmartGroup: from " << startLine() << " to " << endLine() << "; Cursors " << m_normalCursors.count() + m_feedbackCursors.count() << " (" << m_feedbackCursors.count() << " feedback)";
}

void KateSmartManager::verifyCorrect() const
{
  KateSmartGroup* currentGroup = groupForLine(0);
  Q_ASSERT(currentGroup);
  Q_ASSERT(currentGroup == m_firstGroup);

  forever {
    if (!currentGroup->previous())
      Q_ASSERT(currentGroup->startLine() == 0);

    foreach (KateSmartCursor* cursor, currentGroup->feedbackCursors()) {
      Q_ASSERT(currentGroup->containsLine(cursor->line()));
      Q_ASSERT(cursor->smartGroup() == currentGroup);
    }

    if (!currentGroup->next())
      break;

    Q_ASSERT(currentGroup->endLine() == currentGroup->next()->startLine() - 1);
    Q_ASSERT(currentGroup->next()->previous() == currentGroup);

    currentGroup = currentGroup->next();
  }

  Q_ASSERT(currentGroup->endLine() == doc()->lines() - 1);

  kDebug() << "Verified correct." << currentGroup->endLine() << doc()->lines() - 1;
}

void KateSmartManager::rangeGotParent( KateSmartRange * range )
{
  Q_ASSERT(m_topRanges.contains(range));
  m_topRanges.remove(range);
}

void KateSmartManager::rangeLostParent( KateSmartRange * range )
{
  Q_ASSERT(!m_topRanges.contains(range));
  m_topRanges.insert(range);
}

void KateSmartManager::rangeDeleted( KateSmartRange* range )
{
  emit signalRangeDeleted(range);

  if (!range->parentRange())
    m_topRanges.remove(range);
}

void KateSmartManager::unbindSmartRange( SmartRange * range )
{
  static_cast<KateSmartRange*>(range)->unbindAndDelete();
}

void KateSmartManager::deleteCursors(bool includingInternal)
{
  m_invalidGroup->deleteCursors(includingInternal);
  for (KateSmartGroup* g = m_firstGroup; g; g = g->next())
    g->deleteCursors(includingInternal);
}

void KateSmartGroup::deleteCursors( bool includingInternal )
{
  if (includingInternal) {
    qDeleteAll(m_feedbackCursors);
    m_feedbackCursors.clear();

    qDeleteAll(m_normalCursors);
    m_normalCursors.clear();

  } else {
    deleteCursorsInternal(m_feedbackCursors);
    deleteCursorsInternal(m_normalCursors);
  }
}

void KateSmartGroup::deleteCursorsInternal( QSet< KateSmartCursor * > & set )
{
  foreach (KateSmartCursor* c, set.toList()) {
    if (!c->range() && !c->isInternal()) {
      set.remove(c);
      delete c;
    }
  }
}

void KateSmartManager::deleteRanges( bool includingInternal )
{
  foreach (KateSmartRange* range, m_topRanges.toList()) {
    if (includingInternal || !range->isInternal()) {
      range->deleteChildRanges();
      delete range;

      if (!includingInternal)
        m_topRanges.remove(range);
    }
  }

  if (includingInternal)
    m_topRanges.clear();
}

void KateSmartManager::clear( bool includingInternal )
{
  deleteRanges(includingInternal);

  m_clearing = true;
  deleteCursors(includingInternal);
  m_clearing = false;
}

void KateSmartManager::useRevision(int revision)
{
  if (!m_usingRevision.hasLocalData())
    m_usingRevision.setLocalData(new int);

  *m_usingRevision.localData() = revision;
}

int KateSmartManager::usingRevision() const
{
  if (m_usingRevision.hasLocalData())
    return *m_usingRevision.localData();

  return -1;
}

void KateSmartManager::releaseRevision(int revision) const
{
  doc()->history()->releaseRevision(revision);
}

int KateSmartManager::currentRevision() const
{
  return doc()->history()->revision();
}

static void translate(KateEditInfo* edit, Cursor& ret, SmartCursor::InsertBehavior insertBehavior)
{
  // NOTE: copied from KateSmartCursor::translate()
  // If this cursor is before the edit, no action is required
  if (ret < edit->start())
    return;

  // If this cursor is on a line affected by the edit
  if (edit->oldRange().overlapsLine(ret.line())) {
    // If this cursor is at the start of the edit
    if (ret == edit->start()) {
      // And it doesn't need to move, no action is required
      if (insertBehavior == SmartCursor::StayOnInsert)
        return;
    }

    // Calculate the new position
    Cursor newPos;
    if (edit->oldRange().contains(ret)) {
      if (insertBehavior == SmartCursor::MoveOnInsert)
        ret = edit->newRange().end();
      else
        ret = edit->start();

    } else {
      ret += edit->translate();
    }

    return;
  }

  // just need to adjust line number
  ret.setLine(ret.line() + edit->translate().line());
}

Cursor KateSmartManager::translateFromRevision(const Cursor& cursor, SmartCursor::InsertBehavior insertBehavior) const
{
  Cursor ret = cursor;

  foreach (KateEditInfo* edit, doc()->history()->editsBetweenRevisions(usingRevision()))
    translate(edit, ret, insertBehavior);

  return ret;
}

Range KateSmartManager::translateFromRevision(const Range& range, KTextEditor::SmartRange::InsertBehaviors insertBehavior) const
{
  Cursor start = range.start(), end = range.end();

  foreach (KateEditInfo* edit, doc()->history()->editsBetweenRevisions(usingRevision())) {
    translate(edit, start, insertBehavior & KTextEditor::SmartRange::ExpandLeft ? SmartCursor::StayOnInsert : SmartCursor::MoveOnInsert);
    translate(edit, end, insertBehavior & KTextEditor::SmartRange::ExpandRight ? SmartCursor::MoveOnInsert : SmartCursor::StayOnInsert);
  }

  return Range(start, end);
}

#include "katesmartmanager.moc"
