/*  This file is part of the Kate project.
 *
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

#include "katetexthistory.h"
#include "katetextbuffer.h"

namespace Kate {

TextHistory::TextHistory (TextBuffer &buffer)
  : m_buffer (buffer)
  , m_lastSavedRevision (-1)
  , m_firstHistoryEntryRevision (0)
{
  // just call clear to init
  clear ();
}

TextHistory::~TextHistory ()
{
}

qint64 TextHistory::currentRevision () const
{
  // just output last revisions of buffer
  return m_buffer.revision ();
}

void TextHistory::clear ()
{
  // reset last saved revision
  m_lastSavedRevision = -1;

  // remove all history entries and add no-change dummy for first revision
  m_historyEntries.clear ();
  m_historyEntries.push_back (Entry ());

  // first entry will again belong to first revision
  m_firstHistoryEntryRevision = 0;
}

void TextHistory::setLastSavedRevision ()
{
  // current revision was succesful saved
  m_lastSavedRevision = currentRevision ();
}

void TextHistory::wrapLine (const KTextEditor::Cursor &position)
{
}

void TextHistory::unwrapLine (int line)
{
}

void TextHistory::insertText (const KTextEditor::Cursor &position, int length, int oldLineLength)
{
}

void TextHistory::removeText (const KTextEditor::Range &range)
{
}

}
