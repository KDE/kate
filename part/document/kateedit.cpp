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

KateEditInfo::KateEditInfo(KateDocument* doc, Kate::EditSource source, const KTextEditor::Range& oldRange, const QStringList& oldText, const KTextEditor::Range& newRange, const QStringList& newText)
  : m_doc(doc)
  , m_editSource(source)
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

QString KateEditInfo::oldTextString( const KTextEditor::Range & range ) const
{
  Q_ASSERT(m_oldRange.contains(range) && range.start().line() == range.end().line());
  QString original = m_oldText[range.start().line() - m_oldRange.start().line()];
  int startCol = (range.start().line() == m_oldRange.start().line()) ? m_oldRange.start().column() : 0;
  return original.mid(range.start().column() - startCol, range.end().column());
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

QString KateEditInfo::newTextString( const KTextEditor::Range & range ) const
{
  Q_ASSERT(m_newRange.contains(range) && range.start().line() == range.end().line());
  QString original = m_newText[range.start().line() - m_newRange.start().line()];
  int startCol = (range.start().line() == m_newRange.start().line()) ? m_newRange.start().column() : 0;
  return original.mid(range.start().column() - startCol, range.end().column());
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

KateEditHistory::KateEditHistory( KateDocument * doc )
  : m_doc(doc)
  , m_buffer(new KateEditInfoGroup())
//  , m_redo(new KateEditInfoGroup())
  , m_revision(0)
{
}

void KateEditInfoGroup::addEdit( KateEditInfo * edit )
{
  m_edits.append(edit);
}

KateEditInfoGroup::KateEditInfoGroup( )
{
}

int KateEditHistory::revision()
{
  if (!m_buffer->edits().isEmpty()) {
    KateEditInfo* edit = buffer()->edits().last();
    if (!edit->isReferenced())
      m_revisions.insert(++m_revision, edit);

    edit->referenceRevision();
    return m_revision;
  }

  return 0;
}

void KateEditHistory::releaseRevision(int revision)
{
  if (m_revisions.contains(revision)) {
    KateEditInfo* edit = m_revisions[revision];
    edit->dereferenceRevision();
    if (!edit->isReferenced())
      m_revisions.remove(revision);
    return;
  }

  kWarning() << "Unknown revision token " << revision;
}

QList<KateEditInfo*> KateEditHistory::editsBetweenRevisions(int from, int to) const
{
  QList<KateEditInfo*> ret;

  if (from == -1)
    return ret;

  if (buffer()->edits().isEmpty())
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
    fromIndex = buffer()->edits().indexOf(fromEdit);
  }

  KateEditInfo* toEdit = to == -1 ? buffer()->edits().last() : m_revisions[to];
  Q_ASSERT(toEdit);

  int toIndex = buffer()->edits().indexOf(toEdit);
  Q_ASSERT(fromIndex != -1);
  Q_ASSERT(toIndex != -1);
  Q_ASSERT(fromIndex <= toIndex);

  for (int i = fromIndex; i < toIndex; ++i)
    ret.append(buffer()->edits().at(i));

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

#include "kateedit.moc"
