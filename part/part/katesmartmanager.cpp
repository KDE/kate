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
#include "katesmartcursor.h"
#include "katesmartrange.h"

#include <kdebug.h>

static const int s_defaultGroupSize = 40;
static const int s_minimumGroupSize = 20;
static const int s_maximumGroupSize = 60;

KateSmartManager::KateSmartManager(KateDocument* parent)
  : QObject(parent)
  , m_firstGroup(new KateSmartGroup(0, 0, 0L, 0L))
  , m_invalidGroup(new KateSmartGroup(-1, -1, 0L, 0L))
{
  connect(doc()->history(), SIGNAL(editDone(KateEditInfo*)), SLOT(slotTextChanged(KateEditInfo*)));
  //connect(doc(), SIGNAL(textChanged(KTextEditor::Document*)), SLOT(verifyCorrect()));
}

KateSmartManager::~KateSmartManager()
{
}

KateDocument * KateSmartManager::doc( ) const
{
  return static_cast<KateDocument*>(parent());
}

KTextEditor::SmartCursor * KateSmartManager::newSmartCursor( const KTextEditor::Cursor & position, bool moveOnInsert, bool internal )
{
  KateSmartCursor* c = new KateSmartCursor(position, doc(), moveOnInsert);
  if (internal)
    c->setInternal();
  return c;
}

KTextEditor::SmartRange * KateSmartManager::newSmartRange( const KTextEditor::Range & range, KTextEditor::SmartRange * parent, KTextEditor::SmartRange::InsertBehaviours insertBehaviour, bool internal )
{
  KateSmartRange* newRange = new KateSmartRange(range, doc(), parent, insertBehaviour);
  if (internal)
    newRange->setInternal();
  if (!parent)
    rangeLostParent(newRange);
  return newRange;
}

KTextEditor::SmartRange * KateSmartManager::newSmartRange( KateSmartCursor * start, KateSmartCursor * end, KTextEditor::SmartRange * parent, KTextEditor::SmartRange::InsertBehaviours insertBehaviour, bool internal )
{
  KateSmartRange* newRange = new KateSmartRange(start, end, parent, insertBehaviour);
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
        newEndLine = QMIN(newStartLine + s_defaultGroupSize - 1, splitEndLine);
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
  for (KateSmartGroup* smartGroup = currentGroup; smartGroup; smartGroup = smartGroup->next()) {
    if (smartGroup->startLine() > edit->oldRange().end().line())
      break;

    smartGroup->translateChanged(*edit);
  }

  // Cursor feedback
  bool groupChanged = true;
  for (KateSmartGroup* smartGroup = firstSmartGroup; smartGroup; smartGroup = smartGroup->next()) {
    if (groupChanged) {
      groupChanged = smartGroup->startLine() <= edit->oldRange().end().line();
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
  if (range->end() < edit.start())
    return mostSpecific;

  foreach (KTextEditor::SmartRange* child, range->childRanges())
    if (!mostSpecific)
      mostSpecific = feedbackRange(edit, static_cast<KateSmartRange*>(child));
    else
      feedbackRange(edit, static_cast<KateSmartRange*>(child));

  if (range->start() > edit.oldRange().end()) {
    // This range is after the edit... has only been shifted
    range->shifted();

  } else {
    // This range is within the edit.
    if (!mostSpecific)
      if (range->start() < edit.oldRange().start() && range->end() > edit.oldRange().end())
        mostSpecific = range;

    range->translated(edit);
  }

  return mostSpecific;
}


void KateSmartGroup::translateChanged( const KateEditInfo& edit)
{
  //kdDebug() << k_funcinfo << "Was " << edit.oldRange() << " now " << edit.newRange() << " numcursors feedback " << m_feedbackCursors.count() << " normal " << m_normalCursors.count() << endl;

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

  kdDebug() << "KateSmartManager: SmartGroups " << groupCount << " from " << m_firstGroup->startLine() << " to " << currentGroup->endLine() << endl;

  currentGroup = m_firstGroup;
  while (currentGroup) {
    currentGroup->debugOutput();
    currentGroup = currentGroup->next();
  }
}

void KateSmartGroup::debugOutput( ) const
{
  kdDebug() << " -> KateSmartGroup: from " << startLine() << " to " << endLine() << "; Cursors " << m_normalCursors.count() + m_feedbackCursors.count() << " (" << m_feedbackCursors.count() << " feedback)" << endl;
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

  kdDebug() << k_funcinfo << "Verified correct." << endl;
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

void KateSmartManager::rangeDeleted( KateSmartRange * range )
{
  if (!range->parentRange())
    m_topRanges.remove(range);
}

void KateSmartManager::unbindSmartRange( KTextEditor::SmartRange * range )
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
    qDeleteAll(m_normalCursors);
    Q_ASSERT(!m_feedbackCursors.count());
    Q_ASSERT(!m_normalCursors.count());

  } else {
    deleteCursorsInternal(m_feedbackCursors);
    deleteCursorsInternal(m_normalCursors);
  }
}

void KateSmartGroup::deleteCursorsInternal( QSet< KateSmartCursor * > & set )
{
  QSet<KateSmartCursor*>::ConstIterator it = set.constBegin();
  while (it != set.constEnd()) {
    KateSmartCursor* c = *it;
    ++it;
    if (!c->range() && !c->isInternal()) {
      m_feedbackCursors.remove(c);
      delete c;
      // Removing from set is already taken care of
    }
  }
}

void KateSmartManager::deleteRanges( bool includingInternal )
{
  QSet<KateSmartRange*>::ConstIterator it = m_topRanges.constBegin();
  while (it != m_topRanges.constEnd()) {
    KateSmartRange* range = *it;
    ++it;
    if (includingInternal || !range->isInternal()) {
      range->deleteChildRanges();
      delete range;
      // Removing from m_topRanges is already taken care of
    }
  }
}

void KateSmartManager::clear( bool includingInternal )
{
  deleteCursors(includingInternal);
  deleteRanges(includingInternal);
}

#include "katesmartmanager.moc"
