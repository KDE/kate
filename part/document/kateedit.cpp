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

#include "kateedit.h"
#include "katedocument.h"

KateEditInfo::KateEditInfo(Kate::EditSource source, const KTextEditor::Range& oldRange, const QStringList& oldText, const KTextEditor::Range& newRange, const QStringList& newText)
  : m_editSource(source)
  , m_oldRange(oldRange)
  , m_oldText(oldText)
  , m_newRange(newRange)
  , m_newText(newText)
  , m_revisionTokenCounter(0)
{
  m_translate = (m_newRange.end() - m_newRange.start()) - (m_oldRange.end() - m_oldRange.start());
}

KateEditInfo::~KateEditInfo()
{
}

Kate::EditSource KateEditInfo::editSource() const
{
  return m_editSource;
}

const KTextEditor::Range & KateEditInfo::oldRange( ) const
{
  return m_oldRange;
}

QStringList KateEditInfo::oldText( const KTextEditor::Range & range ) const
{
  QStringList ret;
  for (int i = range.start().line(); i <= range.end().line(); ++i) {
    QString original = m_oldText[range.start().line() - m_oldRange.start().line()];

    int startCol = 0, length = -1;
    if (range.start().line() == m_oldRange.start().line())
      startCol = range.start().column() - m_oldRange.start().column();
    if (range.end().line() == m_oldRange.end().line())
      length = range.end().column() - startCol;

    ret << original.mid(startCol, length);
  }
  return ret;
}

const QStringList & KateEditInfo::oldText( ) const
{
  return m_oldText;
}

const KTextEditor::Range & KateEditInfo::newRange( ) const
{
  return m_newRange;
}

QStringList KateEditInfo::newText( const KTextEditor::Range & range ) const
{
  QStringList ret;
  for (int i = range.start().line(); i <= range.end().line(); ++i) {
    QString original = m_newText[range.start().line() - m_newRange.start().line()];

    int startCol = 0, length = -1;
    if (range.start().line() == m_newRange.start().line())
      startCol = range.start().column() - m_oldRange.start().column();
    if (range.end().line() == m_newRange.end().line())
      length = range.end().column() - startCol;

    ret << original.mid(startCol, length);
  }
  return ret;
}

bool KateEditInfo::isReferenced() const
{
  return m_revisionTokenCounter;
}

void KateEditInfo::dereferenceRevision()
{
  --m_revisionTokenCounter;
}

void KateEditInfo::referenceRevision()
{
  ++m_revisionTokenCounter;
}

const QStringList & KateEditInfo::newText() const
{
  return m_newText;
}

bool KateEditInfo::isRemoval() const
{
  return !m_oldRange.isEmpty() && m_newRange.isEmpty();
}

KateEditHistory::KateEditHistory( KateDocument * doc )
  : QObject(doc)
  , m_mutex (QMutex::Recursive)
  , m_revision(0)
{
}

KateEditHistory::~KateEditHistory()
{
  // just delete all edits, referenced or not
  qDeleteAll (m_edits);
}

int KateEditHistory::revision()
{
  // protect the edit history
  QMutexLocker locker (&m_mutex);

  // just take the last edit
  if (!m_edits.isEmpty()) {
    KateEditInfo* edit = m_edits.last();
    if (!edit->isReferenced())
      m_revisions.insert(++m_revision, edit);

    edit->referenceRevision();
    return m_revision;
  }

  // no edits, no revision
  return 0;
}

void KateEditHistory::releaseRevision(int revision)
{
  // protect the edit history
  QMutexLocker locker (&m_mutex);

  // search for revision, if not found, back out
  QMap<int, KateEditInfo*>::iterator it = m_revisions.find (revision);
  if (it == m_revisions.end())
    return;

  // dereference the revision
  KateEditInfo *edit = *it;
  edit->dereferenceRevision();
  if (!edit->isReferenced()) {
    // no longer refed
    m_revisions.remove(revision);

    // clean up memory, try to delete any unrefed revisions

    // delete unrefed
    int deletedEdits = 0;
    for (int i = 0; i < m_edits.size(); ++i) {
      if (m_edits[i]->isReferenced())
	break;

      // already delete edit
      delete m_edits[i];

      // remember deleted count
      ++deletedEdits;
    }

    // remove unrefed from the list now, already deleted
    if (deletedEdits > 0)
      m_edits.erase (m_edits.begin(), m_edits.begin() + deletedEdits);
  }
}

void KateEditHistory::doEdit (KateEditInfo* edit) {
  {
    QMutexLocker locker (&m_mutex);

    // here we go, don't be a memory pirate
    // if we atm only have one edit and it is not referenced, just delete it
    // in other cases, the releaseRevision will clean up
    if (m_edits.size() == 1 && !m_edits.last()->isReferenced()) {
	delete m_edits.takeLast ();
    }

    // append the new edit in any case
    m_edits.append(edit);
  }

  emit editDone(edit);
}

QList<KateEditInfo*> KateEditHistory::editsBetweenRevisions(int from, int to) const
{
  QList<KateEditInfo*> ret;

  if (from == -1)
    return ret;

  QMutexLocker locker (&m_mutex);

  if (m_edits.isEmpty())
    return ret;

  if (to != -1) {
    Q_ASSERT(from <= to);
    Q_ASSERT(m_revisions.contains(to));
  }

  int fromIndex = 0;
  if (from != 0) {
    Q_ASSERT(m_revisions.contains(from));
    KateEditInfo* fromEdit = m_revisions[from];
    Q_ASSERT(fromEdit);

    fromIndex = m_edits.indexOf(fromEdit);
    if(fromIndex != -1) {
        //Since the "from" edit already known, we need to start one behind it
        ++fromIndex;
    }
  }

  KateEditInfo* toEdit = to == -1 ? m_edits.last() : m_revisions[to];
  Q_ASSERT(toEdit);

  int toIndex = m_edits.indexOf(toEdit);
  Q_ASSERT(fromIndex != -1);
  Q_ASSERT(toIndex != -1);

  for (int i = fromIndex; i <= toIndex; ++i)
    ret.append(m_edits.at(i));

  return ret;
}

#include "kateedit.moc"
