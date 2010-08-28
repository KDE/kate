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
{
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

const KTextEditor::Range & KateEditInfo::newRange( ) const
{
  return m_newRange;
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
{
}

KateEditHistory::~KateEditHistory()
{
}

void KateEditHistory::doEdit (KateEditInfo* edit) {
  // emit it
  emit editDone(edit);
  
  // kill it
  delete edit;
}

#include "kateedit.moc"
