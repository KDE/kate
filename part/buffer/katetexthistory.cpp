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
  // create and add new entry
  Entry entry;
  entry.type = Entry::WrapLine;
  entry.line = position.line ();
  entry.column = position.column ();
  addEntry (entry);
}

void TextHistory::unwrapLine (int line)
{
  // create and add new entry
  Entry entry;
  entry.type = Entry::UnwrapLine;
  entry.line = line;
  addEntry (entry);
}

void TextHistory::insertText (const KTextEditor::Cursor &position, int length, int oldLineLength)
{
  // create and add new entry
  Entry entry;
  entry.type = Entry::InsertText;
  entry.line = position.line ();
  entry.column = position.column ();
  entry.length = length;
  entry.oldLineLength = oldLineLength;
  addEntry (entry);
}

void TextHistory::removeText (const KTextEditor::Range &range)
{
  // create and add new entry
  Entry entry;
  entry.type = Entry::RemoveText;
  entry.line = range.start().line ();
  entry.column = range.start().column ();
  entry.length = range.end().column() - range.start().column();
  addEntry (entry);
}

void TextHistory::addEntry (const Entry &entry)
{
  /**
   * history should never be empty
   */
  Q_ASSERT (!m_historyEntries.empty ());

  /**
   * simple efficient check: if we only have one entry, and the entry is not referenced
   * just replace it with the new one and adjust the revision
   */
  if ((m_historyEntries.size () == 1) && !m_historyEntries.first().referenceCounter) {
    /**
     * remember new revision for first element, it is the revision we get after this change
     */
    m_firstHistoryEntryRevision = currentRevision () + 1;

    /**
     * remember edit
     */
    m_historyEntries.first() = entry;

    /**
     * be done...
     */
    return;
  }

  /**
   * ok, we have more than one entry or the entry is referenced, just add up new entries
   */
  m_historyEntries.push_back (entry);
}

}
