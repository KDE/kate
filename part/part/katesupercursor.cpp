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
#include "katesupercursor.moc"

#include "katedocument.h"

#include <kdebug.h>

#include <qobjectlist.h>

KateSuperCursor::KateSuperCursor(KateDocument* doc, bool privateC, const KateTextCursor& cursor, QObject* parent, const char* name)
  : QObject(parent, name)
  , KateDocCursor(cursor.line(), cursor.col(), doc)
  , Kate::Cursor ()
  , m_doc (doc)
{
  m_moveOnInsert = false;
  m_lineRemoved = false;
  m_privateCursor = privateC;

  m_doc->addSuperCursor (this, privateC);
}

KateSuperCursor::KateSuperCursor(KateDocument* doc, bool privateC, int lineNum, int col, QObject* parent, const char* name)
  : QObject(parent, name)
  , KateDocCursor(lineNum, col, doc)
  , Kate::Cursor ()
  , m_doc (doc)
{
  m_moveOnInsert = false;
  m_lineRemoved = false;
  m_privateCursor = privateC;

  m_doc->addSuperCursor (this, privateC);
}

KateSuperCursor::~KateSuperCursor ()
{
  m_doc->removeSuperCursor (this, m_privateCursor);
}

void KateSuperCursor::position(uint *pline, uint *pcol) const
{
  KateDocCursor::position(pline, pcol);
}

bool KateSuperCursor::setPosition(uint line, uint col)
{
  if (line == uint(-2) && col == uint(-2)) { delete this; return true; }
  return KateDocCursor::setPosition(line, col);
}

bool KateSuperCursor::insertText(const QString& s)
{
  return KateDocCursor::insertText(s);
}

bool KateSuperCursor::removeText(uint nbChar)
{
  return KateDocCursor::removeText(nbChar);
}

QChar KateSuperCursor::currentChar() const
{
  return KateDocCursor::currentChar();
}

bool KateSuperCursor::atStartOfLine() const
{
  return col() == 0;
}

bool KateSuperCursor::atEndOfLine() const
{
  return col() >= (int)m_doc->kateTextLine(line())->length();
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

void KateSuperCursor::editTextInserted(uint line, uint col, uint len)
{
  if (m_line == int(line))
  {
    if ((m_col > int(col)) || (m_moveOnInsert && (m_col == int(col))))
    {
      bool insertedAt = m_col == int(col);

      m_col += len;

      if (insertedAt)
        emit charInsertedAt();

      emit positionChanged();
      return;
    }
  }

  emit positionUnChanged();
}

void KateSuperCursor::editTextRemoved(uint line, uint col, uint len)
{
  if (m_line == int(line))
  {
    if (m_col > int(col))
    {
      if (m_col > int(col + len))
      {
        m_col -= len;
      }
      else
      {
        bool prevCharDeleted = m_col == int(col + len);

        m_col = col;

        if (prevCharDeleted)
          emit charDeletedBefore();
        else
          emit positionDeleted();
      }

      emit positionChanged();
      return;

    }
    else if (m_col == int(col))
    {
      emit charDeletedAfter();
    }
  }

  emit positionUnChanged();
}

void KateSuperCursor::editLineWrapped(uint line, uint col, bool newLine)
{
  if (newLine && (m_line > int(line)))
  {
    m_line++;

    emit positionChanged();
    return;
  }
  else if ( (m_line == int(line)) && (m_col >= int(col)) )
  {
    m_line++;
    m_col -= col;

    emit positionChanged();
    return;
  }

  emit positionUnChanged();
}

void KateSuperCursor::editLineUnWrapped(uint line, uint col, bool removeLine, uint length)
{
  if (removeLine && (m_line > int(line+1)))
  {
    m_line--;

    emit positionChanged();
    return;
  }
  else if ( (m_line == int(line+1)) && (removeLine || (m_col < int(length))) )
  {
    m_line = line;
    m_col += col;

    emit positionChanged();
    return;
  }
  else if ( (m_line == int(line+1)) && (m_col >= int(length)) )
  {
    m_col -= length;

    emit positionChanged();
    return;
  }

  emit positionUnChanged();
}

void KateSuperCursor::editLineInserted (uint line)
{
  if (m_line >= int(line))
  {
    m_line++;

    emit positionChanged();
    return;
  }

  emit positionUnChanged();
}

void KateSuperCursor::editLineRemoved(uint line)
{
  if (m_line > int(line))
  {
    m_line--;

    emit positionChanged();
    return;
  }
  else if (m_line == int(line))
  {
    m_line = (line <= m_doc->lastLine()) ? line : (line - 1);
    m_col = 0;

    emit positionDeleted();

    emit positionChanged();
    return;
  }

  emit positionUnChanged();
}

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
  , m_deleteCursors(false)
  , m_allowZeroLength(false)
{
  init();
}

KateSuperRange::KateSuperRange(KateDocument* doc, const KateRange& range, QObject* parent, const char* name)
  : QObject(parent, name)
  , m_start(new KateSuperCursor(doc, true, range.start()))
  , m_end(new KateSuperCursor(doc, true, range.end()))
  , m_evaluate(false)
  , m_startChanged(false)
  , m_endChanged(false)
  , m_deleteCursors(true)
  , m_allowZeroLength(false)
{
  init();
}

KateSuperRange::KateSuperRange(KateDocument* doc, const KateTextCursor& start, const KateTextCursor& end, QObject* parent, const char* name)
  : QObject(parent, name)
  , m_start(new KateSuperCursor(doc, true, start))
  , m_end(new KateSuperCursor(doc, true, end))
  , m_evaluate(false)
  , m_startChanged(false)
  , m_endChanged(false)
  , m_deleteCursors(true)
  , m_allowZeroLength(false)
{
  init();
}

void KateSuperRange::init()
{
  Q_ASSERT(isValid());
  if (!isValid())
    kdDebug(13020) << superStart() << " " << superEnd() << endl;

  insertChild(m_start);
  insertChild(m_end);

  setBehaviour(DoNotExpand);

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

KateSuperRange::~KateSuperRange()
{
  if (m_deleteCursors)
  {
    //insertChild(m_start);
    //insertChild(m_end);
    delete m_start;
    delete m_end;
  }
}

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
  if (superStart() == superEnd()) {
      if (!m_allowZeroLength) emit eliminated();
    }
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
      m_columnBoundaries.removeRef(range->m_start);
      m_columnBoundaries.removeRef(range->m_end);
    }

    if (m_autoManage)
      removeRef(range);

    if (!count())
      emit listEmpty();
  }
}

void KateSuperRangeList::slotDeleted(QObject* range)
{
  //kdDebug(13020)<<"KateSuperRangeList::slotDeleted"<<endl;
  KateSuperRange* r = static_cast<KateSuperRange*>(range);

  if (m_trackingBoundaries) {
      m_columnBoundaries.removeRef(r->m_start);
      m_columnBoundaries.removeRef(r->m_end);
  }

  int index = findRef(r);
  if (index != -1)
    take(index);
  //else kdDebug(13020)<<"Range not found in list"<<endl;

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

// kate: space-indent on; indent-width 2; replace-tabs on;
