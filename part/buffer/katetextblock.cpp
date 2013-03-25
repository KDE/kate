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

#include "katetextblock.h"
#include "katetextbuffer.h"

namespace Kate {

TextBlock::TextBlock (TextBuffer *buffer, int startLine)
  : m_buffer (buffer)
  , m_startLine (startLine)
{
}

TextBlock::~TextBlock ()
{
  // blocks should be empty before they are deleted!
  Q_ASSERT (m_lines.empty());
  Q_ASSERT (m_cursors.empty());

  // it only is a hint for ranges for this block, not the storage of them
}

void TextBlock::setStartLine (int startLine)
{
  // allow only valid lines
  Q_ASSERT (startLine >= 0);
  Q_ASSERT (startLine < m_buffer->lines ());

  m_startLine = startLine;
}

TextLine TextBlock::line (int line) const
{
  // right input
  Q_ASSERT (line >= startLine ());

  // calc internal line
  line = line - startLine ();

  // in range
  Q_ASSERT (line < m_lines.size ());

  // get text line
  return m_lines.at(line);
}

void TextBlock::text (QString &text) const
{
  // combine all lines
  for (int i = 0; i < m_lines.size(); ++i) {
      // not first line, insert \n
      if (i > 0 || startLine() > 0)
        text.append ('\n');

      text.append (m_lines.at(i)->text ());
  }
}

void TextBlock::wrapLine (const KTextEditor::Cursor &position, int fixStartLinesStartIndex)
{
  // calc internal line
  int line = position.line () - startLine ();

  // get text
  QString &text = m_lines.at(line)->textReadWrite ();

  // check if valid column
  Q_ASSERT (position.column() >= 0);
  Q_ASSERT (position.column() <= text.size());

  // create new line and insert it
  m_lines.insert (m_lines.begin() + line + 1, TextLine (new TextLineData()));

  // cases for modification:
  // 1. line is wrapped in the middle
  // 2. if empty line is wrapped, mark new line as modified
  // 3. line-to-be-wrapped is already modified
  if (position.column() > 0 || text.size() == 0 || m_lines.at(line)->markedAsModified()) {
    m_lines.at(line + 1)->markAsModified(true);
  } else if (m_lines.at(line)->markedAsSavedOnDisk()) {
    m_lines.at(line + 1)->markAsSavedOnDisk(true);
  }

  // perhaps remove some text from previous line and append it
  if (position.column() < text.size ()) {
    // text from old line moved first to new one
    m_lines.at(line+1)->textReadWrite() = text.right (text.size() - position.column());

    // now remove wrapped text from old line
    text.chop (text.size() - position.column());

    // mark line as modified
    m_lines.at(line)->markAsModified(true);
  }

  /**
   * fix all start lines
   * we need to do this NOW, else the range update will FAIL!
   * bug 313759
   */
  m_buffer->fixStartLines (fixStartLinesStartIndex);

  /**
   * notify the text history
   */
  m_buffer->history().wrapLine (position);

  /**
   * cursor and range handling below
   */

  // no cursors will leave or join this block

  // no cursors in this block, no work to do..
  if (m_cursors.empty())
    return;

  // move all cursors on the line which has the text inserted
  // remember all ranges modified
  QSet<TextRange *> changedRanges;
  foreach (TextCursor *cursor, m_cursors) {
      // skip cursors on lines in front of the wrapped one!
      if (cursor->lineInBlock() < line)
        continue;

      // either this is simple, line behind the wrapped one
      if (cursor->lineInBlock() > line) {
        // patch line of cursor
        cursor->m_line++;
      }

      // this is the wrapped line
      else {
        // skip cursors with too small column
        if (cursor->column() <= position.column()) {
          if (cursor->column() < position.column() || !cursor->m_moveOnInsert)
            continue;
        }

        // move cursor

        // patch line of cursor
        cursor->m_line++;

        // patch column
        cursor->m_column -= position.column();
      }

      // remember range, if any
      if (cursor->kateRange())
        changedRanges.insert (cursor->kateRange());
  }

  // check validity of all ranges, might invalidate them...
  foreach (TextRange *range, changedRanges)
    range->checkValidity ();
}

void TextBlock::unwrapLine (int line, TextBlock *previousBlock, int fixStartLinesStartIndex)
{
  // calc internal line
  line = line - startLine ();

  // two possiblities: either first line of this block or later line
  if (line == 0) {
    // we need previous block with at least one line
    Q_ASSERT (previousBlock);
    Q_ASSERT (previousBlock->lines () > 0);

    // move last line of previous block to this one, might result in empty block
    TextLine oldFirst = m_lines.at(0);
    int lastLineOfPreviousBlock = previousBlock->lines ()-1;
    TextLine newFirst = previousBlock->m_lines.last();
    m_lines[0] = newFirst;
    previousBlock->m_lines.erase (previousBlock->m_lines.begin() + (previousBlock->lines () - 1));

    // append text
    const int oldSizeOfPreviousLine = newFirst->text().size();
    if (oldFirst->length() > 0) {
      newFirst->textReadWrite().append (oldFirst->text());
      QVector<int> tmp=newFirst->foldingListArray();
      tmp+=oldFirst->foldingListArray();
      newFirst->setFoldingList(tmp); //JOWENN MERGE CORRECTLY

      // mark line as modified, since text was appended
      newFirst->markAsModified(true);
    }

    // patch startLine of this block
    --m_startLine;

    /**
     * fix all start lines
     * we need to do this NOW, else the range update will FAIL!
     * bug 313759
     */
    m_buffer->fixStartLines (fixStartLinesStartIndex);

    /**
     * notify the text history in advance
     */
    m_buffer->history().unwrapLine (startLine () + line, oldSizeOfPreviousLine);

    /**
     * cursor and range handling below
     */

    // no cursors in this block and the previous one, no work to do..
    if (m_cursors.empty() && previousBlock->m_cursors.empty())
      return;

    // move all cursors because of the unwrapped line
    // remember all ranges modified
    QSet<TextRange *> changedRanges;
    foreach (TextCursor *cursor, m_cursors) {
        // this is the unwrapped line
        if (cursor->lineInBlock() == 0) {
          // patch column
          cursor->m_column += oldSizeOfPreviousLine;

          // remember range, if any
          if (cursor->kateRange())
            changedRanges.insert (cursor->kateRange());
        }
    }

    // move cursors of the moved line from previous block to this block now
    QSet<TextCursor *> newPreviousCursors;
    foreach (TextCursor *cursor, previousBlock->m_cursors) {
      if (cursor->lineInBlock() == lastLineOfPreviousBlock) {
        cursor->m_line = 0;
        cursor->m_block = this;
        m_cursors.insert (cursor);

        // remember range, if any
        if (cursor->kateRange())
          changedRanges.insert (cursor->kateRange());
      }
      else
        newPreviousCursors.insert (cursor);
    }
    previousBlock->m_cursors = newPreviousCursors;

    // fixup the ranges that might be effected, because they moved from last line to this block
    foreach (TextRange *range, changedRanges) {
        // update both blocks
        updateRange (range);
        previousBlock->updateRange (range);
    }

    // check validity of all ranges, might invalidate them...
    foreach (TextRange *range, changedRanges)
      range->checkValidity ();

    // be done
    return;
  }

  // easy: just move text to previous line and remove current one
  const int oldSizeOfPreviousLine = m_lines.at(line-1)->length();
  const int sizeOfCurrentLine = m_lines.at(line)->length();
  if (sizeOfCurrentLine > 0) {
    m_lines.at(line-1)->textReadWrite().append (m_lines.at(line)->text());
    QVector<int> tmp=m_lines.at(line-1)->foldingListArray();
    tmp+=m_lines.at(line)->foldingListArray();
    m_lines.at(line-1)->setFoldingList(tmp); //JOWENN MERGE CORRECTLY

  }

  const bool lineChanged = (oldSizeOfPreviousLine > 0 && m_lines.at(line - 1)->markedAsModified())
    || (sizeOfCurrentLine > 0 && (oldSizeOfPreviousLine > 0 || m_lines.at(line)->markedAsModified()));
  m_lines.at(line-1)->markAsModified(lineChanged);
  if (oldSizeOfPreviousLine == 0 && m_lines.at(line)->markedAsSavedOnDisk())
    m_lines.at(line-1)->markAsSavedOnDisk(true);

  m_lines.erase (m_lines.begin () + line);

  /**
   * fix all start lines
   * we need to do this NOW, else the range update will FAIL!
   * bug 313759
   */
  m_buffer->fixStartLines (fixStartLinesStartIndex);

  /**
   * notify the text history in advance
   */
  m_buffer->history().unwrapLine (startLine () + line, oldSizeOfPreviousLine);

  /**
   * cursor and range handling below
   */

  // no cursors in this block, no work to do..
  if (m_cursors.empty())
    return;

  // move all cursors because of the unwrapped line
  // remember all ranges modified
  QSet<TextRange *> changedRanges;
  foreach (TextCursor *cursor, m_cursors) {
      // skip cursors in lines in front of removed one
      if (cursor->lineInBlock() < line)
        continue;

      // this is the unwrapped line
      if (cursor->lineInBlock() == line) {
        // patch column
        cursor->m_column += oldSizeOfPreviousLine;
      }

      // patch line of cursor
      cursor->m_line--;

      // remember range, if any
      if (cursor->kateRange())
        changedRanges.insert (cursor->kateRange());
  }

  // check validity of all ranges, might invalidate them...
  foreach (TextRange *range, changedRanges)
    range->checkValidity ();
}

void TextBlock::insertText (const KTextEditor::Cursor &position, const QString &text)
{
  // calc internal line
  int line = position.line () - startLine ();

  // get text
  QString &textOfLine = m_lines.at(line)->textReadWrite ();
  int oldLength = textOfLine.size ();
  m_lines.at(line)->markAsModified(true);

  // check if valid column
  Q_ASSERT (position.column() >= 0);
  Q_ASSERT (position.column() <= textOfLine.size());

  // insert text
  textOfLine.insert (position.column(), text);

  /**
   * notify the text history
   */
  m_buffer->history().insertText (position, text.size(), oldLength);

  /**
   * cursor and range handling below
   */

  // no cursors in this block, no work to do..
  if (m_cursors.empty())
    return;

  // move all cursors on the line which has the text inserted
  // remember all ranges modified
  QSet<TextRange *> changedRanges;
  foreach (TextCursor *cursor, m_cursors) {
      // skip cursors not on this line!
      if (cursor->lineInBlock() != line)
        continue;

      // skip cursors with too small column
      if (cursor->column() <= position.column()) {
        if (cursor->column() < position.column() || !cursor->m_moveOnInsert)
          continue;
      }

      // patch column of cursor
      if (cursor->m_column <= oldLength)
        cursor->m_column += text.size ();

      // special handling if cursor behind the real line, e.g. non-wrapping cursor in block selection mode
      else if (cursor->m_column < textOfLine.size())
        cursor->m_column = textOfLine.size();

      // remember range, if any
      if (cursor->kateRange())
        changedRanges.insert (cursor->kateRange());
  }

  // check validity of all ranges, might invalidate them...
  foreach (TextRange *range, changedRanges)
    range->checkValidity ();
}

void TextBlock::removeText (const KTextEditor::Range &range, QString &removedText)
{
  // calc internal line
  int line = range.start().line () - startLine ();

  // get text
  QString &textOfLine = m_lines.at(line)->textReadWrite ();
  int oldLength = textOfLine.size ();

  // check if valid column
  Q_ASSERT (range.start().column() >= 0);
  Q_ASSERT (range.start().column() <= textOfLine.size());
  Q_ASSERT (range.end().column() >= 0);
  Q_ASSERT (range.end().column() <= textOfLine.size());

  // get text which will be removed
  removedText = textOfLine.mid (range.start().column(), range.end().column() - range.start().column());

  // remove text
  textOfLine.remove (range.start().column(), range.end().column() - range.start().column());
  m_lines.at(line)->markAsModified(true);

  /**
   * notify the text history
   */
  m_buffer->history().removeText (range, oldLength);

  /**
   * cursor and range handling below
   */

  // no cursors in this block, no work to do..
  if (m_cursors.empty())
    return;

  // move all cursors on the line which has the text removed
  // remember all ranges modified
  QSet<TextRange *> changedRanges;
  foreach (TextCursor *cursor, m_cursors) {
      // skip cursors not on this line!
      if (cursor->lineInBlock() != line)
        continue;

      // skip cursors with too small column
      if (cursor->column() <= range.start().column())
          continue;

      // patch column of cursor
      if (cursor->column() <= range.end().column())
        cursor->m_column = range.start().column ();
      else
        cursor->m_column -= (range.end().column() - range.start().column());

      // remember range, if any
      if (cursor->kateRange())
        changedRanges.insert (cursor->kateRange());
  }

  // check validity of all ranges, might invalidate them...
  foreach (TextRange *range, changedRanges)
    range->checkValidity ();
}

void TextBlock::debugPrint (int blockIndex) const
{
  // print all blocks
  for (int i = 0; i < m_lines.size(); ++i)
    printf ("%4d - %4d : %4d : '%s'\n", blockIndex, startLine() + i
      , m_lines.at(i)->text().size(), qPrintable (m_lines.at(i)->text()));
}

TextBlock *TextBlock::splitBlock (int fromLine)
{
  // half the block
  int linesOfNewBlock = lines () - fromLine;

  // create and insert new block
  TextBlock *newBlock = new TextBlock (m_buffer, startLine() + fromLine);

  // move lines
  newBlock->m_lines.reserve (linesOfNewBlock);
  for (int i = fromLine; i < m_lines.size(); ++i)
    newBlock->m_lines.append (m_lines.at(i));
  m_lines.resize (fromLine);

  // move cursors
  QSet<TextCursor*> oldBlockSet;
  foreach (TextCursor *cursor, m_cursors) {
      if (cursor->lineInBlock() >= fromLine) {
        cursor->m_line = cursor->lineInBlock() - fromLine;
        cursor->m_block = newBlock;
        newBlock->m_cursors.insert (cursor);
      }
      else
        oldBlockSet.insert (cursor);
  }
  m_cursors = oldBlockSet;

  // fix ALL ranges!
  QList<TextRange*> allRanges = m_uncachedRanges.toList() + m_cachedLineForRanges.keys();
  foreach (TextRange *range, allRanges) {
      // update both blocks
      updateRange (range);
      newBlock->updateRange (range);
  }

  // return the new generated block
  return newBlock;
}

void TextBlock::mergeBlock (TextBlock *targetBlock)
{
  // move cursors, do this first, now still lines() count is correct for target
  foreach (TextCursor *cursor, m_cursors) {
    cursor->m_line = cursor->lineInBlock() + targetBlock->lines ();
    cursor->m_block = targetBlock;
    targetBlock->m_cursors.insert (cursor);
  }
  m_cursors.clear ();

  // move lines
  targetBlock->m_lines.reserve (targetBlock->lines() + lines ());
  for (int i = 0; i < m_lines.size(); ++i)
    targetBlock->m_lines.append (m_lines.at(i));
  m_lines.clear ();

  // fix ALL ranges!
  QList<TextRange*> allRanges = m_uncachedRanges.toList() + m_cachedLineForRanges.keys();
  foreach(TextRange* range, allRanges) {
    // update both blocks
    updateRange (range);
    targetBlock->updateRange (range);
  }
}

void TextBlock::deleteBlockContent ()
{
  // kill cursors, if not belonging to a range
  QSet<TextCursor *> copy = m_cursors;
  foreach (TextCursor *cursor, copy)
    if (!cursor->kateRange())
      delete cursor;

  // kill lines
  m_lines.clear ();
}

void TextBlock::clearBlockContent (TextBlock *targetBlock)
{
  // move cursors, if not belonging to a range
  QSet<TextCursor *> copy = m_cursors;
  foreach (TextCursor *cursor, copy) {
    if (!cursor->kateRange()) {
      cursor->m_column = 0;
      cursor->m_line = 0;
      cursor->m_block = targetBlock;
      targetBlock->m_cursors.insert (cursor);
      m_cursors.remove (cursor);
    }
  }

  // kill lines
  m_lines.clear ();
}

void TextBlock::markModifiedLinesAsSaved ()
{
  // mark all modified lines as saved
  for (int i = 0; i < m_lines.size(); ++i) {
    TextLine textLine = m_lines[i];
    if (textLine->markedAsModified())
      textLine->markAsSavedOnDisk(true);
  }
}

void TextBlock::updateRange (TextRange* range)
{
  /**
   * get some simple facts about our nice range
   */
  const int startLine = range->startInternal().lineInternal();
  const int endLine = range->endInternal().lineInternal();
  const bool isSingleLine = startLine == endLine;

  /**
   * perhaps remove range and be done
   */
  if ((endLine < m_startLine) || (startLine >= (m_startLine + lines()))) {
    removeRange (range);
    return;
  }

  /**
   * The range is still a single-line range, and is still cached to the correct line.
   */
  if(isSingleLine && m_cachedLineForRanges.contains (range) && (m_cachedLineForRanges.value(range) == startLine - m_startLine))
    return;

  /**
   * The range is still a multi-line range, and is already in the correct set.
   */
  if(!isSingleLine && m_uncachedRanges.contains (range))
    return;

  /**
   * remove, if already there!
   */
  removeRange(range);

  /**
   * simple case: multi-line range
   */
  if (!isSingleLine) {
    /**
     * The range cannot be cached per line, as it spans multiple lines
     */
    m_uncachedRanges.insert(range);
    return;
  }

  /**
   * The range is contained by a single line, put it into the line-cache
   */
  const int lineOffset = startLine - m_startLine;

  /**
   * enlarge cache if needed
   */
  if (m_cachedRangesForLine.size() <= lineOffset)
    m_cachedRangesForLine.resize(lineOffset+1);

  /**
   * insert into mapping
   */
  m_cachedRangesForLine[lineOffset].insert(range);
  m_cachedLineForRanges[range] = lineOffset;
}

void TextBlock::removeRange (TextRange* range)
{
  /**
   * uncached range? remove it and be done
   */
  if(m_uncachedRanges.remove (range)) {
    /**
     * must be only uncached!
     */
    Q_ASSERT (!m_cachedLineForRanges.contains(range));
    return;
  }

  /**
   * cached range?
   */
  QHash<TextRange*, int>::iterator it = m_cachedLineForRanges.find(range);
  if (it != m_cachedLineForRanges.end()) {
    /**
     * must be only cached!
     */
    Q_ASSERT (!m_uncachedRanges.contains(range));

    /**
     * query the range from cache, must be there
     */
    Q_ASSERT (m_cachedRangesForLine.at(*it).contains(range));

    /**
     * remove it and be done
     */
    m_cachedRangesForLine[*it].remove(range);
    m_cachedLineForRanges.erase(it);
    return;
  }

  /**
   * else: range was not for this block, just do nothing, removeRange should be "safe" to use
   */
}

}
