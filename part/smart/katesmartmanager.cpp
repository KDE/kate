/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2005 Hamish Rodda <rodda@kde.org>
 *  Copyright (C) 2010 Christoph Cullmann <cullmann@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "katesmartmanager.h"

#include "katedocument.h"
#include "kateedit.h"
#include "katesmartcursor.h"
#include "katesmartrange.h"
#include "katesmartgroup.h"

#include <QThreadStorage>
#include <QMutexLocker>
#include <QHash>

#include <kdebug.h>

//#define DEBUG_TRANSLATION

/**
 * thread local storage for the revisions corresponding to some smart manager
 * we need to store them per thread, to allow for example the kdevelop
 * background parser to use its own revision and still not interfer with the
 * kate parts own ranges. It is no member of the smartmanager, as this leads
 * to problems with dynamic creation of the thread storage and its deletion.
 * Tune by using a hash instead of map.
 */
static QThreadStorage<QHash<const KateSmartManager *, int> *> threadLocalRevision;

/**
 * special hash table to identify the main thread...
 */
static QHash<const KateSmartManager *, int> *mainThread = 0;

//Uncomment this to debug the translation of ranges. If that is enabled,
//all ranges are first translated completely separately out of the translation process,
//and at the end the result is compared. If the result mismatches, an assertion is triggered.
// #define DEBUG_TRANSLATION

static const int s_defaultGroupSize = 40;
static const int s_minimumGroupSize = 20;
static const int s_maximumGroupSize = 60;

using namespace KTextEditor;

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

#ifdef DEBUG_TRANSLATION
struct KateSmartManager::KateTranslationDebugger {
  KateTranslationDebugger(KateSmartManager* manager, KateEditInfo* edit) : m_manager(manager), m_edit(edit) {
    manager->m_currentKateTranslationDebugger = this;
    foreach(KateSmartRange* range, manager->m_topRanges)
      addRange(range);
  }

  ~KateTranslationDebugger() {
    m_manager->m_currentKateTranslationDebugger = 0;
  }

  void addRange(SmartRange* _range) {

    KateSmartRange* range = dynamic_cast<KateSmartRange*>(_range);
    Q_ASSERT(range);

    RangeTranslation translation;
    translation.from = *range;
    KTextEditor::Cursor toStart = range->start();
    KTextEditor::Cursor toEnd = range->end();

    translate(m_edit, toStart, (range->insertBehavior() & SmartRange::ExpandLeft) ? SmartCursor::StayOnInsert : SmartCursor::MoveOnInsert);
    translate(m_edit, toEnd, (range->insertBehavior() & SmartRange::ExpandRight) ? SmartCursor::MoveOnInsert : SmartCursor::StayOnInsert);
    translation.to = KTextEditor::Range(toStart, toEnd);

    m_rangeTranslations[range] = translation;
    m_cursorTranslations[&range->smartStart()] = CursorTranslation(range->start(), toStart);
    m_cursorTranslations[&range->smartEnd()] = CursorTranslation(range->end(), toEnd);

    foreach(SmartRange* child, range->childRanges())
      addRange(child);
  }

  void verifyAll() {
    for(QMap<const SmartRange*, RangeTranslation>::iterator it = m_rangeTranslations.begin(); it != m_rangeTranslations.end(); ++it) {
      if(*it.key() != it.value().to) {
        kWarning() << "mismatch. Translation should be:" << it.value().to << "translation is:" << *it.key() << "from:" << it.value().from;
        kWarning() << "edit:" << m_edit->oldRange() << m_edit->newRange();
        Q_ASSERT(0);
      }
    }
  }

  void verifyChange(SmartCursor* _cursor) {
    if(!m_cursorTranslations.contains(_cursor))
      return;
    return;
    KateSmartCursor* cursor = dynamic_cast<KateSmartCursor*>(_cursor);
    Q_ASSERT(cursor);
    KTextEditor::Cursor realPos( cursor->line() + cursor->smartGroup()->newStartLine(), cursor->column());
    kDebug() << "changing cursor from" << m_cursorTranslations[cursor].from << "to" << realPos;

    if(m_cursorTranslations[cursor].to != realPos) {
      kWarning() << "mismatch. Translation of cursor should be:" << m_cursorTranslations[cursor].to << "is:" << realPos << "from:" << m_cursorTranslations[cursor].from;
      kWarning() << "edit:" << m_edit->oldRange() << m_edit->newRange();
      Q_ASSERT(0);
    }
  }

  struct RangeTranslation {
    KTextEditor::Range from, to;
  };

  struct CursorTranslation {
    CursorTranslation() {
    }
    CursorTranslation(const KTextEditor::Cursor& _from, const KTextEditor::Cursor& _to) : from(_from), to(_to) {
    }
    KTextEditor::Cursor from, to;
  };

  QMap<const SmartRange*, RangeTranslation> m_rangeTranslations;
  QMap<const SmartCursor*, CursorTranslation> m_cursorTranslations;
  KateSmartManager* m_manager;
  KateEditInfo* m_edit;
};
#endif

KateSmartManager::KateSmartManager(KateDocument* parent)
  : QObject(parent)
  , m_firstGroup(new KateSmartGroup(0, 0, 0L, 0L))
  , m_invalidGroup(new KateSmartGroup(-1, -1, 0L, 0L))
  , m_clearing(false)
  , m_currentKateTranslationDebugger(0)
{
  // create the global hash to know if we are in gui thread!
  if (!mainThread)
    mainThread = new QHash<const KateSmartManager *, int> ();
  
  // remember our static hash for the mainthread
  if (!threadLocalRevision.hasLocalData())
    threadLocalRevision.setLocalData (mainThread);
  
  // connect to editDone, in this signal, the edit history lock is already released
  connect(doc()->history(), SIGNAL(editDone(KateEditInfo*)), SLOT(slotTextChanged(KateEditInfo*)));
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
  QMutexLocker l(doc()->smartMutex());

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
  QMutexLocker l(doc()->smartMutex());

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
  QMutexLocker l (doc()->smartMutex());

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

#ifdef DEBUG_TRANSLATION
  KateTranslationDebugger KateTranslationDebugger(this, edit);
#endif

  KateSmartGroup* firstSmartGroup = groupForLine(edit->oldRange().start().line());
  KateSmartGroup* currentGroup = firstSmartGroup;

#ifdef DEBUG_TRANSLATION
  // lala
  kDebug() << "line diff for edit" << edit->translate().line ();

  kDebug() << "before edit tranlate";
  for (KateSmartGroup* smartGroup = m_firstGroup; smartGroup; smartGroup = smartGroup->next())
  {  
    kDebug () << "group from" << smartGroup->startLine() << "to" << smartGroup->endLine();
    foreach (KateSmartCursor *c, smartGroup->normalCursors ())
      kDebug() << "cursor" << c << "line" << c->line ();
  }
#endif

  // Check to see if we need to split or consolidate
  int splitEndLine = edit->translate().line() + firstSmartGroup->endLine();

  if (edit->translate().line() > 0) {
    // May need to expand smart groups
    KateSmartGroup* endGroup = currentGroup->next();

    int currentCanExpand = endGroup ? s_maximumGroupSize - currentGroup->length() : s_defaultGroupSize - currentGroup->length();
    int expanded = 0;

    if (currentCanExpand > 0) {
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
    // this leads to broken groups where end line is smaller than the line of
    // some contained cursors, bug 226409
    //currentGroup->setNewEndLine(currentGroup->endLine() + edit->translate().line());
  }
//   
#ifdef DEBUG_TRANSLATION  
  kDebug() << "before edit 2 tranlate";
  for (KateSmartGroup* smartGroup = m_firstGroup; smartGroup; smartGroup = smartGroup->next())
  {  
    kDebug () << "group from" << smartGroup->startLine() << "to" << smartGroup->newEndLine();
    foreach (KateSmartCursor *c, smartGroup->normalCursors ())
      kDebug() << "cursor" << c << "line" << c->line ();
  }
#endif

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

  // Allow rebuilding the child-structure of affected ranges
  for (KateSmartGroup* smartGroup = firstSmartGroup; smartGroup; smartGroup = smartGroup->next()) {
    if (smartGroup->startLine() > edit->oldRange().end().line())
      break;

    smartGroup->translatedChanged2(*edit);
  }

#ifdef DEBUG_TRANSLATION
  KateTranslationDebugger.verifyAll();
#endif

  // Range feedback
  foreach (KateSmartRange* range, m_topRanges) {
    KateSmartRange* mostSpecific = feedbackRange(*edit, range);

    if (!mostSpecific)
      mostSpecific = range;
    range->feedbackRangeContentsChanged(mostSpecific);
  }

#ifdef DEBUG_TRANSLATION
  KateTranslationDebugger.verifyAll();
  //debugOutput();
  verifyCorrect();

  kDebug() << "after edit tranlate";
  for (KateSmartGroup* smartGroup = m_firstGroup; smartGroup; smartGroup = smartGroup->next())
  {  
    kDebug () << "group from" << smartGroup->startLine() << "to" << smartGroup->endLine();
    foreach (KateSmartCursor *c, smartGroup->normalCursors ())
      kDebug() << "cursor" << c << "line" << c->line ();
  }
#endif
}

KateSmartRange* KateSmartManager::feedbackRange( const KateEditInfo& edit, KateSmartRange * range )
{
  KateSmartRange* mostSpecific = 0L;

  // This range precedes the edit... no more to do
  if ((range->end() < edit.start()  && range->kEnd().lastPosition() < edit.start()) ||
    (range->end() == edit.start() && range->kEnd().lastPosition() == edit.start() && !range->isEmpty())
  ) {
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
  // create thread local data if not already there
  if (!threadLocalRevision.hasLocalData())
    threadLocalRevision.setLocalData (new QHash<const KateSmartManager*, int> ());

  if (threadLocalRevision.localData() == mainThread) {
    // clear revision is kind of ok in main thread, at least it will create no errors
    // but it should indicate a problem in your code
    if (revision == -1)
      kWarning() << "tried to do useRevision(-1) == clearRevision() for main thread, that is not supported, this will break the kate part";
    else
      kFatal() << "tried to do useRevision for main thread, that is not supported, this will break the kate part";
    
    return;
  }
  
  // insert revision
  threadLocalRevision.localData()->insert (this, revision);
}

int KateSmartManager::usingRevision()
{
  // query thread local revision info, if any
  if (threadLocalRevision.hasLocalData()) {
    // no using revision for main thread!
    QHash<const KateSmartManager*, int> *hash = threadLocalRevision.localData();
    if (hash == mainThread)
      return -1;
    
    QHash<const KateSmartManager*, int>::const_iterator it = hash->constFind (this);
    if (it != threadLocalRevision.localData()->constEnd ())
      return it.value ();
  }
  
  // no thread local data or nothing found, no revision set
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

Cursor KateSmartManager::translateFromRevision(const Cursor& cursor, SmartCursor::InsertBehavior insertBehavior)
{
  // guard the generated edit history list
  QMutexLocker historyLocker (doc()->history()->mutex());
  QMutexLocker locker (doc()->smartMutex());
  
  Cursor ret = cursor;

  foreach (KateEditInfo* edit, doc()->history()->editsBetweenRevisions(usingRevision()))
    translate(edit, ret, insertBehavior);

  return ret;
}

Range KateSmartManager::translateFromRevision(const Range& range, KTextEditor::SmartRange::InsertBehaviors insertBehavior)
{
  // guard the generated edit history list
  QMutexLocker historyLocker (doc()->history()->mutex());
  QMutexLocker locker (doc()->smartMutex());
  
  Cursor start = range.start(), end = range.end();

  foreach (KateEditInfo* edit, doc()->history()->editsBetweenRevisions(usingRevision())) {
    translate(edit, start, insertBehavior & KTextEditor::SmartRange::ExpandLeft ? SmartCursor::StayOnInsert : SmartCursor::MoveOnInsert);
    translate(edit, end, insertBehavior & KTextEditor::SmartRange::ExpandRight ? SmartCursor::MoveOnInsert : SmartCursor::StayOnInsert);
  }

  return Range(start, end);
}

#include "katesmartmanager.moc"
