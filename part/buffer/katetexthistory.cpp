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

void TextHistory::removeText (const KTextEditor::Range &range, int oldLineLength)
{
  // create and add new entry
  Entry entry;
  entry.type = Entry::RemoveText;
  entry.line = range.start().line ();
  entry.column = range.start().column ();
  entry.length = range.end().column() - range.start().column();
  entry.oldLineLength = oldLineLength;
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
    for (int i = 0; i + 1 < m_historyEntries.size(); ++i) {
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

void TextHistory::Entry::transformCursor (int &cursorLine, int &cursorColumn, bool moveOnInsert) const
{
  /**
   * simple stuff, sort out generic things
   */

  /**
   * no change, if this change is in line behind cursor
   */
  if (line > cursorLine)
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
      if (cursorLine == line) {
        /**
         * skip cursors with too small column
         */
        if (cursorColumn <= column) {
          if (cursorColumn < column || !moveOnInsert)
            return;
        }

        /**
         * adjust column
         */
        cursorColumn =  cursorColumn - column;
      }

      /**
       * always increment cursor line
       */
      cursorLine +=  1;
      return;

    /**
     * Unwrap a line
     */
    case UnwrapLine:
      /**
       * we unwrap this line, adjust column
       */
      if (cursorLine == line)
        cursorColumn +=  oldLineLength;

      /**
       * decrease cursor line
       */
      cursorLine -= 1;
      return;

    /**
     * Insert text
     */
    case InsertText:
      /**
       * only interesting, if same line
       */
      if (cursorLine != line)
        return;

      // skip cursors with too small column
      if (cursorColumn <= column)
        if (cursorColumn < column || !moveOnInsert)
          return;

      // patch column of cursor
      if (cursorColumn <= oldLineLength)
        cursorColumn += length;

      // special handling if cursor behind the real line, e.g. non-wrapping cursor in block selection mode
      else if (cursorColumn < oldLineLength + length)
        cursorColumn =  oldLineLength + length;

      return;

    /**
     * Remove text
     */
    case RemoveText:
      /**
       * only interesting, if same line
       */
      if (cursorLine != line)
        return;

      // skip cursors with too small column
      if (cursorColumn <= column)
          return;

      // patch column of cursor
      if (cursorColumn <= column + length)
        cursorColumn =  column;
      else
        cursorColumn -=  length;

      return;

    /**
     * nothing
     */
    default:
      return;
  }
}

void TextHistory::Entry::reverseTransformCursor (int &cursorLine, int &cursorColumn, bool moveOnInsert) const
{   
  /**
   * handle all history types
   */
  switch (type) {
    /**
     * Wrap a line
     */
    case WrapLine:
      /**
       * ignore this line
       */
      if (cursorLine <= line)
          return;
        
      /**
       * next line is unwrapped
       */
      if (cursorLine == line + 1) {
        /**
         * adjust column
         */
        cursorColumn = cursorColumn + column;
      }

      /**
       * always decrement cursor line
       */
      cursorLine -=  1;
      return;

    /**
     * Unwrap a line
     */
    case UnwrapLine:
      /**
       * ignore lines before unwrapped one
       */
      if (cursorLine < line - 1)
          return;
        
      /**
       * we unwrap this line, try to adjust cursor column if needed
       */
      if (cursorLine == line - 1) {
        /**
         * skip cursors with to small columns
         */
        if (cursorColumn <= oldLineLength) {
            if (cursorColumn < oldLineLength || !moveOnInsert)
                return;
        }
          
        cursorColumn -= oldLineLength;
      }
      
      /**
       * increase cursor line
       */
      cursorLine += 1;
      return;

    /**
     * Insert text
     */
    case InsertText:
      /**
       * only interesting, if same line
       */
      if (cursorLine != line)
        return;

      // skip cursors with too small column
      if (cursorColumn <= column)
        return;

      // patch column of cursor
      if (cursorColumn - length < column)
        cursorColumn = column;
      else
        cursorColumn -= length;

      return;

    /**
     * Remove text
     */
    case RemoveText:
      /**
       * only interesting, if same line
       */
      if (cursorLine != line)
        return;

      // skip cursors with too small column
      if (cursorColumn <= column)
        if (cursorColumn < column || !moveOnInsert)
          return;

      // patch column of cursor
      if (cursorColumn <= oldLineLength)
        cursorColumn += length;

      // special handling if cursor behind the real line, e.g. non-wrapping cursor in block selection mode
      else if (cursorColumn < oldLineLength + length)
        cursorColumn =  oldLineLength + length;
      return;

    /**
     * nothing
     */
    default:
      return;
  }
}

void TextHistory::transformCursor (int& line, int& column, KTextEditor::MovingCursor::InsertBehavior insertBehavior, qint64 fromRevision, qint64 toRevision)
{
  /**
   * -1 special meaning for from/toRevision
   */
  if (fromRevision == -1)
    fromRevision = revision ();
  
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
  Q_ASSERT (fromRevision != toRevision);
  Q_ASSERT (fromRevision >= m_firstHistoryEntryRevision);
  Q_ASSERT (fromRevision < (m_firstHistoryEntryRevision + m_historyEntries.size()));
  Q_ASSERT (toRevision >= m_firstHistoryEntryRevision);
  Q_ASSERT (toRevision < (m_firstHistoryEntryRevision + m_historyEntries.size()));

  /**
   * transform cursor
   */
  bool moveOnInsert = insertBehavior == KTextEditor::MovingCursor::MoveOnInsert;
  
  /**
   * forward or reverse transform?
   */
  if (toRevision > fromRevision) {
    for (int rev = fromRevision - m_firstHistoryEntryRevision + 1; rev <= (toRevision - m_firstHistoryEntryRevision); ++rev) {
        const Entry &entry = m_historyEntries[rev];
        entry.transformCursor (line, column, moveOnInsert);
    }
  } else {
    for (int rev = fromRevision - m_firstHistoryEntryRevision; rev >= (toRevision - m_firstHistoryEntryRevision + 1); --rev) {
        const Entry &entry = m_historyEntries[rev];
        entry.reverseTransformCursor (line, column, moveOnInsert);
    }
  }
}

void TextHistory::transformRange (KTextEditor::Range &range, KTextEditor::MovingRange::InsertBehaviors insertBehaviors, KTextEditor::MovingRange::EmptyBehavior emptyBehavior, qint64 fromRevision, qint64 toRevision)
{
  /**
   * invalidate on empty?
   */
  bool invalidateIfEmpty = emptyBehavior == KTextEditor::MovingRange::InvalidateIfEmpty;
  if (invalidateIfEmpty && range.end() <= range.start()) {
    range = KTextEditor::Range::invalid();
    return;
  }

  /**
   * -1 special meaning for from/toRevision
   */
  if (fromRevision == -1)
    fromRevision = revision ();

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
  Q_ASSERT (fromRevision != toRevision);
  Q_ASSERT (fromRevision >= m_firstHistoryEntryRevision);
  Q_ASSERT (fromRevision < (m_firstHistoryEntryRevision + m_historyEntries.size()));
  Q_ASSERT (toRevision >= m_firstHistoryEntryRevision);
  Q_ASSERT (toRevision < (m_firstHistoryEntryRevision + m_historyEntries.size()));
  
  /**
   * transform cursors
   */
      
  // first: copy cursors, without range association
  int startLine = range.start().line(), startColumn = range.start().column(), endLine = range.end().line(), endColumn = range.end().column();
  
  bool moveOnInsertStart = !(insertBehaviors & KTextEditor::MovingRange::ExpandLeft);
  bool moveOnInsertEnd = (insertBehaviors & KTextEditor::MovingRange::ExpandRight);
  
  /**
   * forward or reverse transform?
   */
  if (toRevision > fromRevision) {
    for (int rev = fromRevision - m_firstHistoryEntryRevision + 1; rev <= (toRevision - m_firstHistoryEntryRevision); ++rev) {
        const Entry &entry = m_historyEntries[rev];
        
        entry.transformCursor (startLine, startColumn, moveOnInsertStart);
        
        entry.transformCursor (endLine, endColumn, moveOnInsertEnd);

        // got empty?
        if(endLine < startLine || (endLine == startLine && endColumn <= startColumn))
        {
            if (invalidateIfEmpty) {
                range = KTextEditor::Range::invalid();
                return;
            }
            else{
                // else normalize them
                endLine = startLine;
                endColumn = startColumn;
            }
        }
    }
  } else {
    for (int rev = fromRevision - m_firstHistoryEntryRevision ; rev >= (toRevision - m_firstHistoryEntryRevision + 1); --rev) {
        const Entry &entry = m_historyEntries[rev];
        
        entry.reverseTransformCursor (startLine, startColumn, moveOnInsertStart);
        
        entry.reverseTransformCursor (endLine, endColumn, moveOnInsertEnd);

        // got empty?
        if(endLine < startLine || (endLine == startLine && endColumn <= startColumn))
        {
            if (invalidateIfEmpty) {
                range = KTextEditor::Range::invalid();
                return;
            }
            else{
                // else normalize them
                endLine = startLine;
                endColumn = startColumn;
            }
        }
    }
  }

  // now, copy cursors back
  range.start().setLine(startLine);
  range.start().setColumn(startColumn);
  range.end().setLine(endLine);
  range.end().setColumn(endColumn);
  
}

}
