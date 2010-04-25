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

qint64 TextHistory::revision () const
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
  m_lastSavedRevision = revision ();
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

void TextHistory::unwrapLine (int line, int oldLineLength)
{
  // create and add new entry
  Entry entry;
  entry.type = Entry::UnwrapLine;
  entry.line = line;
  entry.column = 0;
  entry.oldLineLength = oldLineLength;
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
    m_firstHistoryEntryRevision = revision () + 1;

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

void TextHistory::lockRevision (qint64 revision)
{
  /**
   * some invariants must hold
   */
  Q_ASSERT (!m_historyEntries.empty ());
  Q_ASSERT (revision >= m_firstHistoryEntryRevision);
  Q_ASSERT (revision < (m_firstHistoryEntryRevision + m_historyEntries.size()));

  /**
   * increment revision reference counter
   */
  Entry &entry = m_historyEntries[revision - m_firstHistoryEntryRevision];
  ++entry.referenceCounter;
}

void TextHistory::unlockRevision (qint64 revision)
{
  /**
   * some invariants must hold
   */
  Q_ASSERT (!m_historyEntries.empty ());
  Q_ASSERT (revision >= m_firstHistoryEntryRevision);
  Q_ASSERT (revision < (m_firstHistoryEntryRevision + m_historyEntries.size()));

  /**
   * decrement revision reference counter
   */
  Entry &entry = m_historyEntries[revision - m_firstHistoryEntryRevision];
  Q_ASSERT (entry.referenceCounter);
  --entry.referenceCounter;

  /**
   * clean up no longer used revisions...
   */
  if (!entry.referenceCounter) {
    /**
     * search for now unused stuff
     */
    int unreferencedEdits = 0;
    for (int i = 0; i < m_historyEntries.size(); ++i) {
      if (m_historyEntries[i].referenceCounter)
        break;

      // remember deleted count
      ++unreferencedEdits;
    }

    /**
     * remove unrefed from the list now
     */
    if (unreferencedEdits > 0) {
      // remove stuff from history
      m_historyEntries.erase (m_historyEntries.begin(), m_historyEntries.begin() + unreferencedEdits);

      // patch first entry revision
      m_firstHistoryEntryRevision += unreferencedEdits;
    }
  }
}

void TextHistory::Entry::transformCursor (KTextEditor::Cursor &cursor, bool moveOnInsert) const
{
  /**
   * simple stuff, sort out generic things
   */

  /**
   * no change, if this change is in line behind cursor
   */
  if (line > cursor.line ())
    return;

  /**
   * handle all history types
   */
  switch (type) {
    /**
     * Wrap a line
     */
    case WrapLine:
      /**
       * we wrap this line
       */
      if (cursor.line() == line) {
        /**
         * skip cursors with too small column
         */
        if (cursor.column() <= column) {
          if (cursor.column() < column || !moveOnInsert)
            return;
        }

        /**
         * adjust column
         */
        cursor.setColumn (cursor.column() - column);
      }

      /**
       * always increment cursor line
       */
      cursor.setLine (cursor.line() + 1);
      return;

    /**
     * Unwrap a line
     */
    case UnwrapLine:
      /**
       * we unwrap this line, adjust column
       */
      if (cursor.line() == line)
        cursor.setColumn (cursor.column() + oldLineLength);

      /**
       * decrease cursor line
       */
      cursor.setLine (cursor.line() - 1);
      return;

    /**
     * Insert text
     */
    case InsertText:
      /**
       * only interesting, if same line
       */
      if (cursor.line() != line)
        return;

      // skip cursors with too small column
      if (cursor.column() <= column)
        if (cursor.column() < column || !moveOnInsert)
          return;

      // patch column of cursor
      if (cursor.column() <= oldLineLength)
        cursor.setColumn (cursor.column () + length);

      // special handling if cursor behind the real line, e.g. non-wrapping cursor in block selection mode
      else if (cursor.column() < (oldLineLength + length))
        cursor.setColumn (oldLineLength + length);

      return;

    /**
     * Remove text
     */
    case RemoveText:
      /**
       * only interesting, if same line
       */
      if (cursor.line() != line)
        return;

      // skip cursors with too small column
      if (cursor.column() <= column)
          return;

      // patch column of cursor
      if (cursor.column() <= (column + length))
        cursor.setColumn (column);
      else
        cursor.setColumn (cursor.column () - length);

      return;

    /**
     * nothing
     */
    default:
      return;
  }
}

void TextHistory::transformCursor (KTextEditor::Cursor &cursor, KTextEditor::MovingCursor::InsertBehavior insertBehavior, qint64 fromRevision, qint64 toRevision)
{
  /**
   * -1 special meaning for toRevision
   */
  if (toRevision == -1)
    toRevision = revision ();

  /**
   * shortcut, same revision
   */
  if (fromRevision == toRevision)
    return;

  /**
   * some invariants must hold
   */
  Q_ASSERT (!m_historyEntries.empty ());
  Q_ASSERT (fromRevision < toRevision);
  Q_ASSERT (fromRevision >= m_firstHistoryEntryRevision);
  Q_ASSERT (fromRevision < (m_firstHistoryEntryRevision + m_historyEntries.size()));
  Q_ASSERT (toRevision >= m_firstHistoryEntryRevision);
  Q_ASSERT (toRevision < (m_firstHistoryEntryRevision + m_historyEntries.size()));

  /**
   * transform cursor
   */
  bool moveOnInsert = insertBehavior == KTextEditor::MovingCursor::MoveOnInsert;
  for (int rev = fromRevision - m_firstHistoryEntryRevision + 1; rev <= (toRevision - m_firstHistoryEntryRevision); ++rev) {
    const Entry &entry = m_historyEntries[rev];
    entry.transformCursor (cursor, moveOnInsert);
  }
}

void TextHistory::transformRange (KTextEditor::Range &range, KTextEditor::MovingRange::InsertBehaviors insertBehaviors, qint64 fromRevision, qint64 toRevision)
{
  /**
   * -1 special meaning for toRevision
   */
  if (toRevision == -1)
    toRevision = revision ();

  /**
   * shortcut, same revision
   */
  if (fromRevision == toRevision)
    return;

  /**
   * some invariants must hold
   */
  Q_ASSERT (!m_historyEntries.empty ());
  Q_ASSERT (fromRevision < toRevision);
  Q_ASSERT (fromRevision >= m_firstHistoryEntryRevision);
  Q_ASSERT (fromRevision < (m_firstHistoryEntryRevision + m_historyEntries.size()));
  Q_ASSERT (toRevision >= m_firstHistoryEntryRevision);
  Q_ASSERT (toRevision < (m_firstHistoryEntryRevision + m_historyEntries.size()));

  /**
   * transform cursors
   */

  // first: copy cursors, without range association
  KTextEditor::Cursor start = range.start ();
  KTextEditor::Cursor end = range.end ();

  bool moveOnInsertStart = !(insertBehaviors & KTextEditor::MovingRange::ExpandLeft);
  bool moveOnInsertEnd = (insertBehaviors & KTextEditor::MovingRange::ExpandRight);
  for (int rev = fromRevision - m_firstHistoryEntryRevision + 1; rev <= (toRevision - m_firstHistoryEntryRevision); ++rev) {
    const Entry &entry = m_historyEntries[rev];
    entry.transformCursor (start, moveOnInsertStart);
    entry.transformCursor (end, moveOnInsertEnd);

    // normalize them
    if (end < start)
      end = start;
  }

  // now, copy cursors back
  range.setRange (start, end);
}

}
