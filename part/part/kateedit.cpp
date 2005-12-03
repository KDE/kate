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

KateEditInfo::KateEditInfo(KateDocument* doc, int source, const KTextEditor::Range& oldRange, const QStringList& oldText, const KTextEditor::Range& newRange, const QStringList& newText)
  : m_doc(doc)
  , m_editSource(source)
  , m_oldRange(oldRange)
  , m_oldText(oldText)
  , m_newRange(newRange)
  , m_newText(newText)
{
  m_translate = (m_newRange.end() - m_newRange.start()) - (m_oldRange.end() - m_oldRange.start());
}

KateEditInfo::~KateEditInfo()
{
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
  , m_undo(new KateEditInfoGroup())
  , m_redo(new KateEditInfoGroup())
{
}

void KateEditInfoGroup::addEdit( KateEditInfo * edit )
{
  m_edits.append(edit);
}

KateEditInfoGroup::KateEditInfoGroup( )
{
}

#include "kateedit.moc"
