/* This file is part of the KDE libraries
   Copyright (C) 2003 Hamish Rodda <rodda@kde.org>

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

#include "katesupercursor.h"

#include <qobjectlist.h>

#include <kdebug.h>

#include "katebuffer.h"
#include "katedocument.h"

#ifdef DEBUGTESTING
static KateSuperCursor* debugCursor = 0L;
static KateSuperRange* debugRange = 0L;
#endif

KateSuperCursor::KateSuperCursor(KateDocument* doc, const KateTextCursor& cursor, QObject* parent, const char* name)
  : QObject(parent, name)
  , KateDocCursor(cursor.line(), cursor.col(), doc)
  , m_linePtr(doc->kateTextLine(cursor.line()))
{
  Q_ASSERT(m_linePtr);

  connectSS();
}

KateSuperCursor::KateSuperCursor(KateDocument* doc, int lineNum, int col, QObject* parent, const char* name)
  : QObject(parent, name)
  , KateDocCursor(lineNum, col, doc)
  , m_linePtr(doc->kateTextLine(lineNum))
{
  Q_ASSERT(m_linePtr);

  connectSS();
}

#ifdef DEBUGTESTING
KateSuperCursor::~KateSuperCursor()
{
  if (debugCursor == this) kdDebug() << k_funcinfo << endl;
}
#endif

bool KateSuperCursor::atStartOfLine() const
{
  return col() == 0;
}

bool KateSuperCursor::atEndOfLine() const
{
  return col() >= (int)m_linePtr->length();
}

bool KateSuperCursor::moveOnInsert() const
{
  return m_moveOnInsert;
}

void KateSuperCursor::setMoveOnInsert(bool moveOnInsert)
{
  m_moveOnInsert = moveOnInsert;
}

void KateSuperCursor::setLine(int lineNum)
{
  int tempLine = line(), tempcol = col();
  KateDocCursor::setLine(lineNum);
  if (tempLine != line() || tempcol != col())
    emit positionDirectlyChanged();
}

void KateSuperCursor::setCol(int colNum)
{
  KateDocCursor::setCol(colNum);
}

void KateSuperCursor::setPos(const KateTextCursor& pos)
{
  KateDocCursor::setPos(pos);
}

void KateSuperCursor::setPos(int lineNum, int colNum)
{
  KateDocCursor::setPos(lineNum, colNum);
}

void KateSuperCursor::connectSS()
{
  m_moveOnInsert = false;
  m_lineRemoved = false;

#ifdef DEBUGTESTING
  if (!debugCursor) {
    debugCursor = this;
    connect(this, SIGNAL(positionDirectlyChanged()), SLOT(slotPositionDirectlyChanged()));
    connect(this, SIGNAL(positionChanged()), SLOT(slotPositionChanged()));
    connect(this, SIGNAL(positionUnChanged()), SLOT(slotPositionUnChanged()));
    connect(this, SIGNAL(positionDeleted()), SLOT(slotPositionDeleted()));
    connect(this, SIGNAL(charInsertedAt()), SLOT(slotCharInsertedAt()));
    connect(this, SIGNAL(charDeletedBefore()), SLOT(slotCharDeletedBefore()));
    connect(this, SIGNAL(charDeletedAfter()), SLOT(slotCharDeletedAfter()));
  }
#endif

  connect(m_linePtr->buffer(), SIGNAL(textInserted(TextLine::Ptr, uint, uint)), SLOT(slotTextInserted(TextLine::Ptr, uint, uint)));
  connect(m_linePtr->buffer(), SIGNAL(lineInsertedBefore(TextLine::Ptr, uint)), SLOT(slotLineInsertedBefore(TextLine::Ptr, uint)));
  connect(m_linePtr->buffer(), SIGNAL(textRemoved(TextLine::Ptr, uint, uint)), SLOT(slotTextRemoved(TextLine::Ptr, uint, uint)));
  connect(m_linePtr->buffer(), SIGNAL(lineRemoved(uint)), SLOT(slotLineRemoved(uint)));
  connect(m_linePtr->buffer(), SIGNAL(textWrapped(TextLine::Ptr, TextLine::Ptr, uint)), SLOT(slotTextWrapped(TextLine::Ptr, TextLine::Ptr, uint)));
  connect(m_linePtr->buffer(), SIGNAL(textUnWrapped(TextLine::Ptr, TextLine::Ptr, uint, uint)), SLOT(slotTextUnWrapped(TextLine::Ptr, TextLine::Ptr, uint, uint)));
}

void KateSuperCursor::slotTextInserted(TextLine::Ptr linePtr, uint pos, uint len)
{
  if (m_linePtr == linePtr) {
    bool insertedAt = col() == int(pos);

    if (m_moveOnInsert ? (col() > int(pos)) : (col() >= int(pos))) {
      m_col += len;

      if (insertedAt)
        emit charInsertedAt();

      emit positionChanged();
      return;
    }
  }

  emit positionUnChanged();
}

void KateSuperCursor::slotLineInsertedBefore(TextLine::Ptr linePtr, uint lineNum)
{
  // NOTE not >= because the = case is taken care of slotTextWrapped.
  // Comparing to linePtr is because we can get a textWrapped signal then a lineInsertedBefore signal.
  if (m_linePtr != linePtr && line() > int(lineNum)) {
    m_line++;
    emit positionChanged();
    return;

  } else if (m_linePtr != linePtr) {
    emit positionUnChanged();
  }
}

void KateSuperCursor::slotTextRemoved(TextLine::Ptr linePtr, uint pos, uint len)
{
  if (m_linePtr == linePtr) {
    if (col() > int(pos)) {
      if (col() > int(pos + len)) {
        m_col -= len;

      } else {
        bool prevCharDeleted = col() == int(pos + len);

        m_col = pos;

        if (prevCharDeleted)
          emit charDeletedBefore();
        else
          emit positionDeleted();
      }

      emit positionChanged();
      return;

    } else if (col() == int(pos)) {
      emit charDeletedAfter();
    }
  }

  emit positionUnChanged();
}

// TODO does this work with multiple line deletions?
// TODO does this need the same protection with the actual TextLine::Ptr as lineInsertedBefore?
void KateSuperCursor::slotLineRemoved(uint lineNum)
{
  if (line() == int(lineNum)) {
    // They took my line! :(
    bool atStart = col() == 0;

    if (m_doc->numLines() <= lineNum) {
      m_line = m_doc->numLines() - 1;
      m_linePtr = m_doc->kateTextLine(line());
      m_col = m_linePtr->length();

    } else {
      m_linePtr = m_doc->kateTextLine(line());
      m_col = 0;
    }

    if (atStart)
      emit charDeletedBefore();
    else
      emit positionDeleted();

    emit positionChanged();
    return;

  } else if (line() > int(lineNum)) {
    m_line--;
    emit positionChanged();
    return;
  }

  if (m_lineRemoved) {
    m_lineRemoved = false;
    return;
  }

  emit positionUnChanged();
}

void KateSuperCursor::slotTextWrapped(TextLine::Ptr linePtr, TextLine::Ptr nextLine, uint pos)
{
  if (m_linePtr == linePtr) {
    if (col() >= int(pos)) {
      bool atStart = col() == 0;
      m_col -= pos;
      m_line++;
      m_linePtr = nextLine;

      if (atStart)
        emit charDeletedBefore();

      emit positionChanged();
      return;
    }
  }

  // This is the job of the lineRemoved / lineInsertedBefore methods.
  //emit positionUnChanged();
}

void KateSuperCursor::slotTextUnWrapped(TextLine::Ptr linePtr, TextLine::Ptr nextLine, uint pos, uint len)
{
  if (m_linePtr == nextLine) {
    if (col() < int(len)) {
      m_col += pos;
      m_line--;
      m_linePtr = linePtr;
      emit positionChanged();
      m_lineRemoved = true;
      return;
    }
  }

  // This is the job of the lineRemoved / lineInsertedBefore methods.
  //emit positionUnChanged();
}

#ifdef DEBUGTESTING
void KateSuperCursor::slotPositionDirectlyChanged()
{
  if (debugCursor == this) kdDebug() << k_funcinfo << endl/* << kdBacktrace(7)*/;
}

void KateSuperCursor::slotPositionChanged()
{
  if (debugCursor == this) kdDebug() << k_funcinfo << endl/* << kdBacktrace(7)*/;
}

void KateSuperCursor::slotPositionUnChanged()
{
  if (debugCursor == this) kdDebug() << k_funcinfo << endl/* << kdBacktrace(7)*/;
}

void KateSuperCursor::slotPositionDeleted()
{
  if (debugCursor == this) kdDebug() << k_funcinfo << endl;
}

void KateSuperCursor::slotCharInsertedAt()
{
  if (debugCursor == this) kdDebug() << k_funcinfo << endl;
}

void KateSuperCursor::slotCharDeletedBefore()
{
  if (debugCursor == this) kdDebug() << k_funcinfo << endl;
}

void KateSuperCursor::slotCharDeletedAfter()
{
  if (debugCursor == this) kdDebug() << k_funcinfo << endl;
}
#endif

KateSuperCursor::operator QString()
{
  return QString("[%1,%1]").arg(line()).arg(col());
}


KateSuperRange::KateSuperRange(KateSuperCursor* start, KateSuperCursor* end, QObject* parent, const char* name)
  : QObject(parent, name)
  , m_start(start)
  , m_end(end)
  , m_evaluate(false)
  , m_startChanged(false)
  , m_endChanged(false)
{
  init();
}

KateSuperRange::KateSuperRange(KateDocument* doc, const KateRange& range, QObject* parent, const char* name)
  : QObject(parent, name)
  , m_start(new KateSuperCursor(doc, range.start()))
  , m_end(new KateSuperCursor(doc, range.end()))
  , m_evaluate(false)
  , m_startChanged(false)
  , m_endChanged(false)
{
  init();
}

KateSuperRange::KateSuperRange(KateDocument* doc, const KateTextCursor& start, const KateTextCursor& end, QObject* parent, const char* name)
  : QObject(parent, name)
  , m_start(new KateSuperCursor(doc, start))
  , m_end(new KateSuperCursor(doc, end))
  , m_evaluate(false)
  , m_startChanged(false)
  , m_endChanged(false)
{
  init();
}

void KateSuperRange::init()
{
  Q_ASSERT(isValid());
  if (!isValid())
    kdDebug() << superStart() << " " << superEnd() << endl;

  insertChild(m_start);
  insertChild(m_end);

  setBehaviour(DoNotExpand);

#ifdef DEBUGTESTING
  if (!debugRange) {
    debugRange = this;
    connect(this, SIGNAL(positionChanged()), SLOT(slotPositionChanged()));
    connect(this, SIGNAL(positionUnChanged()), SLOT(slotPositionUnChanged()));
    connect(this, SIGNAL(contentsChanged()), SLOT(slotContentsChanged()));
    connect(this, SIGNAL(boundaryDeleted()), SLOT(slotBoundaryDeleted()));
    connect(this, SIGNAL(eliminated()), SLOT(slotEliminated()));
  }
#endif

  // Not necessarily the best implementation
  connect(m_start, SIGNAL(positionDirectlyChanged()),  SIGNAL(contentsChanged()));
  connect(m_end, SIGNAL(positionDirectlyChanged()),  SIGNAL(contentsChanged()));

  connect(m_start, SIGNAL(positionChanged()),  SLOT(slotEvaluateChanged()));
  connect(m_end, SIGNAL(positionChanged()),  SLOT(slotEvaluateChanged()));
  connect(m_start, SIGNAL(positionUnChanged()), SLOT(slotEvaluateUnChanged()));
  connect(m_end, SIGNAL(positionUnChanged()), SLOT(slotEvaluateUnChanged()));
  connect(m_start, SIGNAL(positionDeleted()), SIGNAL(boundaryDeleted()));
  connect(m_end, SIGNAL(positionDeleted()), SIGNAL(boundaryDeleted()));
}

#ifdef DEBUGTESTING
KateSuperRange::~KateSuperRange()
{
  if (debugRange == this) kdDebug() << k_funcinfo << endl;
}
#endif

KateTextCursor& KateSuperRange::start()
{
  return *m_start;
}

const KateTextCursor& KateSuperRange::start() const
{
  return *m_start;
}

KateTextCursor& KateSuperRange::end()
{
  return *m_end;
}

const KateTextCursor& KateSuperRange::end() const
{
  return *m_end;
}

KateSuperCursor& KateSuperRange::superStart()
{
  return *m_start;
}

const KateSuperCursor& KateSuperRange::superStart() const
{
  return *m_start;
}

KateSuperCursor& KateSuperRange::superEnd()
{
  return *m_end;
}

const KateSuperCursor& KateSuperRange::superEnd() const
{
  return *m_end;
}

int KateSuperRange::behaviour() const
{
  return (m_start->moveOnInsert() ? DoNotExpand : ExpandLeft) | (m_end->moveOnInsert() ? ExpandRight : DoNotExpand);
}

void KateSuperRange::setBehaviour(int behaviour)
{
  m_start->setMoveOnInsert(behaviour & ExpandLeft);
  m_end->setMoveOnInsert(!(behaviour & ExpandRight));
}

bool KateSuperRange::isValid() const
{
  return superStart() <= superEnd();
}

bool KateSuperRange::owns(const KateTextCursor& cursor) const
{
  if (!includes(cursor)) return false;

  if (children())
    for (QObjectListIt it(*children()); *it; ++it)
      if ((*it)->inherits("KateSuperRange"))
        if (static_cast<KateSuperRange*>(*it)->owns(cursor))
          return false;

  return true;
}

bool KateSuperRange::includes(const KateTextCursor& cursor) const
{
  return isValid() && cursor >= superStart() && cursor < superEnd();
}

bool KateSuperRange::includes(uint lineNum) const
{
  return isValid() && (int)lineNum >= superStart().line() && (int)lineNum <= superEnd().line();
}

bool KateSuperRange::includesWholeLine(uint lineNum) const
{
  return isValid() && ((int)lineNum > superStart().line() || ((int)lineNum == superStart().line() && superStart().atStartOfLine())) && ((int)lineNum < superEnd().line() || ((int)lineNum == superEnd().line() && superEnd().atEndOfLine()));
}

bool KateSuperRange::boundaryAt(const KateTextCursor& cursor) const
{
  return isValid() && (cursor == superStart() || cursor == superEnd());
}

bool KateSuperRange::boundaryOn(uint lineNum) const
{
  return isValid() && (superStart().line() == (int)lineNum || superEnd().line() == (int)lineNum);
}

void KateSuperRange::slotEvaluateChanged()
{
  if (sender() == static_cast<QObject*>(m_start)) {
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
  if (sender() == static_cast<QObject*>(m_start)) {
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
  emit tagRange(this);
}

void KateSuperRange::evaluateEliminated()
{
  if (superStart() == superEnd())
    emit eliminated();
  else
    emit contentsChanged();
}

void KateSuperRange::evaluatePositionChanged()
{
  if (superStart() == superEnd())
    emit eliminated();
  else
    emit positionChanged();
}

#ifdef DEBUGTESTING
void KateSuperRange::slotPositionChanged()
{
  if (debugRange == this) kdDebug() << k_funcinfo << endl;
}

void KateSuperRange::slotPositionUnChanged()
{
  if (debugRange == this) kdDebug() << k_funcinfo << endl;
}

void KateSuperRange::slotContentsChanged()
{
  if (debugRange == this) kdDebug() << k_funcinfo << endl;
}

void KateSuperRange::slotBoundaryDeleted()
{
  if (debugRange == this) kdDebug() << k_funcinfo << endl;
}

void KateSuperRange::slotEliminated()
{
  if (debugRange == this) kdDebug() << k_funcinfo << endl;
}
#endif

int KateSuperCursorList::compareItems(QPtrCollection::Item item1, QPtrCollection::Item item2)
{
  if (*(static_cast<KateSuperCursor*>(item1)) == *(static_cast<KateSuperCursor*>(item2)))
    return 0;

  return *(static_cast<KateSuperCursor*>(item1)) < *(static_cast<KateSuperCursor*>(item2)) ? -1 : 1;
}

KateSuperRangeList::KateSuperRangeList(bool autoManage, QObject* parent, const char* name)
  : QObject(parent, name)
  , m_autoManage(autoManage)
  , m_connect(true)
  , m_trackingBoundaries(false)
{
  setAutoManage(autoManage);
}

KateSuperRangeList::KateSuperRangeList(const QPtrList<KateSuperRange>& rangeList, QObject* parent, const char* name)
  : QObject(parent, name)
  , m_autoManage(false)
  , m_connect(false)
  , m_trackingBoundaries(false)
{
  appendList(rangeList);
}

void KateSuperRangeList::appendList(const QPtrList<KateSuperRange>& rangeList)
{
  for (QPtrListIterator<KateSuperRange> it = rangeList; *it; ++it)
    append(*it);
}

void KateSuperRangeList::clear()
{
  for (KateSuperRange* range = first(); range; range = next())
    emit rangeEliminated(range);

  QPtrList<KateSuperRange>::clear();
}

void KateSuperRangeList::connectAll()
{
  if (!m_connect) {
    m_connect = true;
    for (KateSuperRange* range = first(); range; range = next()) {
      connect(range, SIGNAL(destroyed(QObject*)), SLOT(slotDeleted(QObject*)));
      connect(range, SIGNAL(eliminated()), SLOT(slotEliminated()));
    }
  }
}

bool KateSuperRangeList::autoManage() const
{
  return m_autoManage;
}

void KateSuperRangeList::setAutoManage(bool autoManage)
{
  m_autoManage = autoManage;
  setAutoDelete(m_autoManage);
}

QPtrList<KateSuperRange> KateSuperRangeList::rangesIncluding(const KateTextCursor& cursor)
{
  sort();

  QPtrList<KateSuperRange> ret;

  for (KateSuperRange* r = first(); r; r = next())
    if (r->includes(cursor))
      ret.append(r);

  return ret;
}

QPtrList<KateSuperRange> KateSuperRangeList::rangesIncluding(uint line)
{
  sort();

  QPtrList<KateSuperRange> ret;

  for (KateSuperRange* r = first(); r; r = next())
    if (r->includes(line))
      ret.append(r);

  return ret;
}

bool KateSuperRangeList::rangesInclude(const KateTextCursor& cursor)
{
  for (KateSuperRange* r = first(); r; r = next())
    if (r->includes(cursor))
      return true;

  return false;
}

void KateSuperRangeList::slotEliminated()
{
  if (sender()) {
    KateSuperRange* range = static_cast<KateSuperRange*>(const_cast<QObject*>(sender()));
    emit rangeEliminated(range);

    if (m_trackingBoundaries) {
      m_columnBoundaries.removeRef(&(range->superStart()));
      m_columnBoundaries.removeRef(&(range->superEnd()));
    }

    if (m_autoManage)
      removeRef(range);

    if (!count())
      emit listEmpty();
  }
}

void KateSuperRangeList::slotDeleted(QObject* range)
{
  KateSuperRange* r = static_cast<KateSuperRange*>(range);
  int index = findRef(r);
  if (index != -1)
    take(index);

  emit rangeDeleted(r);

  if (!count())
      emit listEmpty();
}

KateSuperCursor* KateSuperRangeList::firstBoundary(const KateTextCursor* start)
{
  if (!m_trackingBoundaries) {
    m_trackingBoundaries = true;

    for (KateSuperRange* r = first(); r; r = next()) {
      m_columnBoundaries.append(&(r->superStart()));
      m_columnBoundaries.append(&(r->superEnd()));
    }
  }

  m_columnBoundaries.sort();

  if (start)
    // OPTIMISE: QMap with QPtrList for each line? (==> sorting issues :( )
    for (KateSuperCursor* c = m_columnBoundaries.first(); c; c = m_columnBoundaries.next())
      if (*start <= *c)
        break;

  return m_columnBoundaries.current();
}

KateSuperCursor* KateSuperRangeList::nextBoundary()
{
  KateSuperCursor* current = m_columnBoundaries.current();

  // make sure the new cursor is after the current cursor; multiple cursors with the same position can be in the list.
  if (current)
    while (m_columnBoundaries.next())
      if (*(m_columnBoundaries.current()) != *current)
        break;

  return m_columnBoundaries.current();
}

KateSuperCursor* KateSuperRangeList::currentBoundary()
{
  return m_columnBoundaries.current();
}

int KateSuperRangeList::compareItems(QPtrCollection::Item item1, QPtrCollection::Item item2)
{
  if (static_cast<KateSuperRange*>(item1)->superStart() == static_cast<KateSuperRange*>(item2)->superStart()) {
    if (static_cast<KateSuperRange*>(item1)->superEnd() == static_cast<KateSuperRange*>(item2)->superEnd()) {
      return 0;
    } else {
      return static_cast<KateSuperRange*>(item1)->superEnd() < static_cast<KateSuperRange*>(item2)->superEnd() ? -1 : 1;
    }
  }

  return static_cast<KateSuperRange*>(item1)->superStart() < static_cast<KateSuperRange*>(item2)->superStart() ? -1 : 1;
}

QPtrCollection::Item KateSuperRangeList::newItem(QPtrCollection::Item d)
{
  if (m_connect) {
    connect(static_cast<KateSuperRange*>(d), SIGNAL(destroyed(QObject*)), SLOT(slotDeleted(QObject*)));
    connect(static_cast<KateSuperRange*>(d), SIGNAL(eliminated()), SLOT(slotEliminated()));
    connect(static_cast<KateSuperRange*>(d), SIGNAL(tagRange(KateSuperRange*)), SIGNAL(tagRange(KateSuperRange*)));

    // HACK HACK
    static_cast<KateSuperRange*>(d)->slotTagRange();
  }

  if (m_trackingBoundaries) {
    m_columnBoundaries.append(&(static_cast<KateSuperRange*>(d)->superStart()));
    m_columnBoundaries.append(&(static_cast<KateSuperRange*>(d)->superEnd()));
  }

  return QPtrList<KateSuperRange>::newItem(d);
}

#include "katesupercursor.moc"
