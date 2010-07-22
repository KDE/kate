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

#include "config.h"

#include "katetextbuffer.h"
#include "katetextloader.h"

// this is unfortunate, but needed for performance
#include "katedocument.h"
#include "kateview.h"

#include <kde_file.h>

namespace Kate {

TextBuffer::TextBuffer (KTextEditor::Document *parent, int blockSize)
  : QObject (parent)
  , m_document (parent)
  , m_history (*this)
  , m_blockSize (blockSize)
  , m_lines (0)
  , m_lastUsedBlock (0)
  , m_revision (0)
  , m_editingTransactions (0)
  , m_editingLastRevision (0)
  , m_editingLastLines (0)
  , m_editingMinimalLineChanged (-1)
  , m_editingMaximalLineChanged (-1)
  , m_encodingProberType (KEncodingProber::Universal)
  , m_fallbackTextCodec (0)
  , m_textCodec (0)
  , m_generateByteOrderMark (false)
  , m_endOfLineMode (eolUnix)
  , m_removeTrailingSpaces (false)
{
  // minimal block size must be > 0
  Q_ASSERT (m_blockSize > 0);

  // create initial state
  clear ();
}

TextBuffer::~TextBuffer ()
{
  // remove document pointer, this will avoid any notifyAboutRangeChange to have a effect
  m_document = 0;

  // not allowed during editing
  Q_ASSERT (m_editingTransactions == 0);

  // kill all ranges, work on copy, they will remove themself from the hash
  QSet<TextRange *> copyRanges = m_ranges;
  qDeleteAll (copyRanges);
  Q_ASSERT (m_ranges.empty());

  // clean out all cursors and lines, only cursors belonging to range will survive
  for (int i = 0; i < m_blocks.size(); ++i)
    m_blocks[i]->deleteBlockContent ();

  // delete all blocks, now that all cursors are really deleted
  // else asserts in destructor of blocks will fail!
  qDeleteAll (m_blocks);
  m_blocks.clear ();

  // kill all invalid cursors, do this after block deletion, to uncover if they might be still linked in blocks
  QSet<TextCursor *> copyCursors = m_invalidCursors;
  qDeleteAll (copyCursors);
  Q_ASSERT (m_invalidCursors.empty());
}

void TextBuffer::clear ()
{
  // not allowed during editing
  Q_ASSERT (m_editingTransactions == 0);

  // invalidate all ranges, work on copy, they might delete themself...
  QSet<TextRange *> copyRanges = m_ranges;
  foreach (TextRange *range, copyRanges)
    range->setRange (KTextEditor::Cursor::invalid(), KTextEditor::Cursor::invalid());

  // new block for empty buffer
  TextBlock *newBlock = new TextBlock (this, 0);
  newBlock->appendLine (TextLine (new TextLineData()));

  // clean out all cursors and lines, either move them to newBlock or invalidate them, if belonging to a range
  for (int i = 0; i < m_blocks.size(); ++i)
    m_blocks[i]->clearBlockContent (newBlock);

  // kill all buffer blocks
  qDeleteAll (m_blocks);
  m_blocks.clear ();

  // insert one block with one empty line
  m_blocks.append (newBlock);

  // reset lines and last used block
  m_lines = 1;
  m_lastUsedBlock = 0;

  // reset revision
  m_revision = 0;

  // reset bom detection
  m_generateByteOrderMark = false;

  // reset the filter device
  m_mimeTypeForFilterDev = "text/plain";

  // clear edit history
  m_history.clear ();

  // we got cleared
  emit cleared ();
}

TextLine TextBuffer::line (int line) const
{
  // get block, this will assert on invalid line
  int blockIndex = blockForLine (line);

  // get line
  return m_blocks[blockIndex]->line (line);
}

QString TextBuffer::text () const
{
  QString text;

  // combine all blocks
  for (int i = 0; i < m_blocks.size(); ++i)
    m_blocks[i]->text (text);

  // return generated string
  return text;
}

bool TextBuffer::startEditing ()
{
  // increment transaction counter
  ++m_editingTransactions;

  // if not first running transaction, do nothing
  if (m_editingTransactions > 1)
    return false;

  // reset informations about edit...
  m_editingLastRevision = m_revision;
  m_editingLastLines = m_lines;
  m_editingMinimalLineChanged = -1;
  m_editingMaximalLineChanged = -1;

  // transaction has started
  emit editingStarted ();

  // first transaction started
  return true;
}

bool TextBuffer::finishEditing ()
{
  // only allowed if still transactions running
  Q_ASSERT (m_editingTransactions > 0);

  // decrement counter
  --m_editingTransactions;

  // if not last running transaction, do nothing
  if (m_editingTransactions > 0)
    return false;

  // assert that if buffer changed, the line ranges are set and valid!
  Q_ASSERT (!editingChangedBuffer() || (m_editingMinimalLineChanged != -1 && m_editingMaximalLineChanged != -1));
  Q_ASSERT (!editingChangedBuffer() || (m_editingMinimalLineChanged <= m_editingMaximalLineChanged));
  Q_ASSERT (!editingChangedBuffer() || (m_editingMinimalLineChanged >= 0 && m_editingMinimalLineChanged < m_lines));
  Q_ASSERT (!editingChangedBuffer() || (m_editingMaximalLineChanged >= 0 && m_editingMaximalLineChanged < m_lines));

  // transaction has finished
  emit editingFinished ();

  // last transaction finished
  return true;
}

void TextBuffer::wrapLine (const KTextEditor::Cursor &position)
{
  // only allowed if editing transaction running
  Q_ASSERT (m_editingTransactions > 0);

  // get block, this will assert on invalid line
  int blockIndex = blockForLine (position.line());

  // let the block handle the wrapLine
  // this can only lead to one more line in this block
  // no other blocks will change
  ++m_lines; // first alter the line counter, as functions called will need the valid one
  m_blocks[blockIndex]->wrapLine (position);

  // remember changes
  ++m_revision;

  // update changed line interval
  if (position.line() < m_editingMinimalLineChanged || m_editingMinimalLineChanged == -1)
    m_editingMinimalLineChanged = position.line();

  if (position.line() <= m_editingMaximalLineChanged)
    ++m_editingMaximalLineChanged;
  else
    m_editingMaximalLineChanged = position.line() + 1;

  // fixup all following blocks
  fixStartLines (blockIndex);

  // balance the changed block if needed
  balanceBlock (blockIndex);

  // emit signal about done change
  emit lineWrapped (position);
}

void TextBuffer::unwrapLine (int line)
{
  // only allowed if editing transaction running
  Q_ASSERT (m_editingTransactions > 0);

  // line 0 can't be unwrapped
  Q_ASSERT (line > 0);

  // get block, this will assert on invalid line
  int blockIndex = blockForLine (line);

  // is this the first line in the block?
  bool firstLineInBlock = (line == m_blocks[blockIndex]->startLine());

  // let the block handle the unwrapLine
  // this can either lead to one line less in this block or the previous one
  // the previous one could even end up with zero lines
  m_blocks[blockIndex]->unwrapLine (line, (blockIndex > 0) ? m_blocks[blockIndex-1] : 0);
  --m_lines;

  // decrement index for later fixup, if we modified the block in front of the found one
  if (firstLineInBlock)
    --blockIndex;

  // remember changes
  ++m_revision;

  // update changed line interval
   if ((line - 1) < m_editingMinimalLineChanged || m_editingMinimalLineChanged == -1)
    m_editingMinimalLineChanged = line - 1;

  if (line <= m_editingMaximalLineChanged)
    --m_editingMaximalLineChanged;
  else
    m_editingMaximalLineChanged = line -1;

  // fixup all following blocks
  fixStartLines (blockIndex);

  // balance the changed block if needed
  balanceBlock (blockIndex);

  // emit signal about done change
  emit lineUnwrapped (line);
}

void TextBuffer::insertText (const KTextEditor::Cursor &position, const QString &text)
{
  // only allowed if editing transaction running
  Q_ASSERT (m_editingTransactions > 0);

  // skip work, if no text to insert
  if (text.isEmpty())
    return;

  // get block, this will assert on invalid line
  int blockIndex = blockForLine (position.line());

  // let the block handle the insertText
  m_blocks[blockIndex]->insertText (position, text);

  // remember changes
  ++m_revision;

  // update changed line interval
  if (position.line () < m_editingMinimalLineChanged || m_editingMinimalLineChanged == -1)
    m_editingMinimalLineChanged = position.line ();

  if (position.line () > m_editingMaximalLineChanged)
    m_editingMaximalLineChanged = position.line ();

  // emit signal about done change
  emit textInserted (position, text);
}

void TextBuffer::removeText (const KTextEditor::Range &range)
{
  // only allowed if editing transaction running
  Q_ASSERT (m_editingTransactions > 0);

  // only ranges on one line are supported
  Q_ASSERT (range.start().line() == range.end().line());

  // start colum <= end column and >= 0
  Q_ASSERT (range.start().column() <= range.end().column());
  Q_ASSERT (range.start().column() >= 0);

  // skip work, if no text to remove
  if (range.isEmpty())
    return;

  // get block, this will assert on invalid line
  int blockIndex = blockForLine (range.start().line());

  // let the block handle the removeText, retrieve removed text
  QString text;
  m_blocks[blockIndex]->removeText (range, text);

  // remember changes
  ++m_revision;

  // update changed line interval
  if (range.start().line() < m_editingMinimalLineChanged || m_editingMinimalLineChanged == -1)
    m_editingMinimalLineChanged = range.start().line();

  if (range.start().line() > m_editingMaximalLineChanged)
    m_editingMaximalLineChanged = range.start().line();

  // emit signal about done change
  emit textRemoved (range, text);
}

int TextBuffer::blockForLine (int line) const
{
  // only allow valid lines
  Q_ASSERT (line >= 0);
  Q_ASSERT (line < lines());

  // block to start search with
  int index = m_lastUsedBlock;
  int blockStart = 0;
  int blockEnd = m_blocks.size();

  // check if start is ok
  if (index < 0 || index >= m_blocks.size())
    index = 0;

  // search for right block
  forever {
    // facts bout this block
    const int start = m_blocks[index]->startLine();
    const int lines = m_blocks[index]->lines ();

    // right block found, remember it and return it
    if (start <= line && line < (start + lines)) {
      m_lastUsedBlock = index;
      return index;
    }

    if (line < start)
	{
	  // Search left of index
	  blockEnd = index;
	}else{
	  // Search right of index
	  blockStart = index+1;
	}
	index = (blockStart + (blockEnd-1)) / 2;
  }

  // we should always find a block
  Q_ASSERT (false);
  return -1;
}

void TextBuffer::fixStartLines (int startBlock)
{
  // only allow valid start block
  Q_ASSERT (startBlock >= 0);
  Q_ASSERT (startBlock < m_blocks.size());

  // new start line for next block
  int newStartLine = m_blocks[startBlock]->startLine () + m_blocks[startBlock]->lines ();

  // fixup block
  for (int index = startBlock + 1; index < m_blocks.size(); ++index) {
    // set new start line
    m_blocks[index]->setStartLine (newStartLine);

    // calculate next start line
    newStartLine += m_blocks[index]->lines ();
  }
}

void TextBuffer::balanceBlock (int index)
{
  /**
   * two cases, too big or too small block
   */
  TextBlock *blockToBalance = m_blocks[index];

  // first case, too big one, split it
  if (blockToBalance->lines () >= 2 * m_blockSize) {
    // half the block
    int halfSize = blockToBalance->lines () / 2;

    // create and insert new block behind current one, already set right start line
    TextBlock *newBlock = blockToBalance->splitBlock (halfSize);
    Q_ASSERT (newBlock);
    m_blocks.insert (m_blocks.begin() + index + 1, newBlock);

    // split is done
    return;
  }

  // second case: possibly too small block

  // if only one block, no chance to unite
  // same if this is first block, we always append to previous one
  if (index == 0)
    return;

  // block still large enough, do nothing
  if (2 * blockToBalance->lines () > m_blockSize)
    return;

  // unite small block with predecessor
  TextBlock *targetBlock = m_blocks[index-1];

  // merge block
  blockToBalance->mergeBlock (targetBlock);

  // delete old block
  delete blockToBalance;
  m_blocks.erase (m_blocks.begin() + index);
}

void TextBuffer::debugPrint (const QString &title) const
{
  // print header with title
  printf ("%s (lines: %d bs: %d)\n", qPrintable (title), m_lines, m_blockSize);

  // print all blocks
  for (int i = 0; i < m_blocks.size(); ++i)
    m_blocks[i]->debugPrint (i);
}

bool TextBuffer::load (const QString &filename, bool &encodingErrors)
{
  // fallback codec must exist
  Q_ASSERT (m_fallbackTextCodec);

  // codec must be set!
  Q_ASSERT (m_textCodec);

  /**
   * first: clear buffer in any case!
   */
  clear ();

  /**
   * check if this is a normal file or not, else exit
   */
  KDE_struct_stat sbuf;
  if (KDE::stat(filename, &sbuf) != 0 || !S_ISREG(sbuf.st_mode))
    return false;

  /**
   * construct the file loader for the given file, with correct prober type
   */
  Kate::TextLoader file (filename, m_encodingProberType);

  /**
   * triple play, maximal three loading rounds
   * 0) use the given encoding, be done, if no encoding errors happen
   * 1) use BOM to decided if unicode or if that fails, use encoding prober, if no encoding errors happen, be done
   * 2) use fallback encoding, be done, if no encoding errors happen
   * 3) use again given encoding, be done in any case
   */
  for (int i = 0; i < 4;  ++i) {
    /**
     * kill all blocks beside first one
     */
    for (int b = 1; b < m_blocks.size(); ++b) {
      m_blocks[b]->m_lines.clear ();
      delete m_blocks[b];
    }
    m_blocks.resize (1);

    /**
     * remove lines in first block
     */
    m_blocks.last()->m_lines.clear ();
    m_lines = 0;

    /**
     * try to open file, with given encoding
     * in round 0 + 3 use the given encoding from user
     * in round 1 use 0, to trigger detection
     * in round 2 use fallback
     */
    QTextCodec *codec = m_textCodec;
    if (i == 1)
      codec = 0;
    else if (i == 2)
      codec = m_fallbackTextCodec;

    if (!file.open (codec)) {
      // create one dummy textline, in any case
      m_blocks.last()->appendLine (TextLine (new TextLineData()));
      m_lines++;
      return false;
    }

    // read in all lines...
    encodingErrors = false;
    while ( !file.eof() )
    {
      // read line
      int offset = 0, length = 0;
      bool currentError = !file.readLine (offset, length);
      encodingErrors = encodingErrors || currentError;

      // bail out on encoding error, if not last round!
      if (encodingErrors && i < 3)
        break;

      // get unicode data for this line
      const QChar *unicodeData = file.unicode () + offset;

      // strip trailing spaces
      if (m_removeTrailingSpaces) {
        while (length > 0) {
          if (unicodeData[length-1].isSpace())
            --length;
          else
            break;
        }
      }

      // construct new text line with content from file
      TextLine textLine = TextLine (new TextLineData(QString (unicodeData, length)));

      // ensure blocks aren't too large
      if (m_blocks.last()->lines() >= m_blockSize)
        m_blocks.append (new TextBlock (this, m_blocks.last()->startLine() + m_blocks.last()->lines()));

      m_blocks.last()->appendLine (textLine);
      m_lines++;
    }

    // if no encoding error, break out of reading loop
    if (!encodingErrors) {
      // remember used codec
      m_textCodec = file.textCodec ();
      break;
    }
  }

  // remember if BOM was found
  if (file.byteOrderMarkFound ())
    setGenerateByteOrderMark (true);

  // remember eol mode, if any found in file
  if (file.eol() != eolUnknown)
    setEndOfLineMode (file.eol());

  // remember mime type for filter device
  m_mimeTypeForFilterDev = file.mimeTypeForFilterDev ();

  // assert that one line is there!
  Q_ASSERT (m_lines > 0);

  // report CODEC + ERRORS
  kDebug (13020) << "Loaded file " << filename << "with codec" << m_textCodec->name()
    << (encodingErrors ? "with" : "without") << "encoding errors";

  // report BOM
  kDebug (13020) << (file.byteOrderMarkFound () ? "Found" : "Didn't find") << "byte order mark";

  // report filter device mime-type
  kDebug (13020) << "used filter device for mime-type" << m_mimeTypeForFilterDev;

  // emit success
  emit loaded (filename, encodingErrors);

  // file loading worked, modulo encoding problems
  return true;
}

bool TextBuffer::save (const QString &filename)
{
  // codec must be set!
  Q_ASSERT (m_textCodec);

  /**
   * construct correct filter device and try to open
   */
  QIODevice *file = KFilterDev::deviceForFile (filename, m_mimeTypeForFilterDev, false);
  if (!file->open (QIODevice::WriteOnly)) {
    delete file;
    return false;
  }

  /**
   * construct stream + disable Unicode headers
   */
  QTextStream stream (file);
  stream.setCodec (QTextCodec::codecForName("UTF-16"));

  // set the correct codec
  stream.setCodec (m_textCodec);

  // generate byte order mark?
  stream.setGenerateByteOrderMark (generateByteOrderMark());

  // our loved eol string ;)
  QString eol = "\n"; //m_doc->config()->eolString ();
  if (endOfLineMode() == eolDos)
    eol = QString ("\r\n");
  else if (endOfLineMode() == eolMac)
    eol = QString ("\r");

  // just dump the lines out ;)
  for (int i = 0; i < m_lines; ++i)
  {
    // get line to save
    Kate::TextLine textline = line (i);

    // strip trailing spaces
    if (m_removeTrailingSpaces)
    {
      int lastChar = textline->lastChar();
      if (lastChar > -1)
      {
        stream << textline->text().left (lastChar+1);
      }
    }
    else // simple, dump the line
      stream << textline->text();

    // append correct end of line string
    if ((i+1) < m_lines)
      stream << eol;
  }

  // flush stream
  stream.flush ();

  // close and delete file
  file->close ();
  delete file;

#ifndef Q_OS_WIN
  // ensure that the file is written to disk
  // we crete new qfile, as the above might be wrapper around compression
  QFile syncFile (filename);
  syncFile.open (QIODevice::ReadOnly);

#ifdef HAVE_FDATASYNC
  fdatasync (syncFile.handle());
#else
  fsync (syncFile.handle());
#endif
#endif

  // did save work?
  bool ok = stream.status() == QTextStream::Ok;

  // remember this revision as last saved if we had success!
  if (ok)
    m_history.setLastSavedRevision ();

  // report CODEC + ERRORS
  kDebug (13020) << "Saved file " << filename << "with codec" << m_textCodec->name()
    << (ok ? "without" : "with") << "errors";

  // emit signal on success
  if (ok)
    emit saved (filename);

  // return success or not
  return ok;
}

void TextBuffer::notifyAboutRangeChange (KTextEditor::View *view, int startLine, int endLine, bool rangeWithAttribute)
{
  /**
   * ignore calls if no document is around
   */
  if (!m_document)
    return;

  /**
   * update all views, this IS ugly and could be a signal, but I profiled and a signal is TOO slow, really
   * just create 20k ranges in a go and you wait seconds on a decent machine
   */
  const QList<KTextEditor::View *> &views = m_document->views ();
  for (int i = 0; i < views.size(); ++i) {
    // get view
    KTextEditor::View *curView = views[i];

    // filter wrong views
    if (view && view != curView)
     continue;

    // notify view, it is really a kate view
    static_cast<KateView *> (curView)->notifyAboutRangeChange (startLine, endLine, rangeWithAttribute);
  }
}

QList<TextRange *> TextBuffer::rangesForLine (int line, KTextEditor::View *view, bool rangesWithAttributeOnly) const
{
  // get block, this will assert on invalid line
  const int blockIndex = blockForLine (line);

  // get the ranges of the right block

  QList<TextRange *> rightRanges;

  foreach(const QSet<TextRange *> &ranges, m_blocks[blockIndex]->allRangesIntersectingLine(line))
  {
    foreach (TextRange * const range, ranges) {
        /**
        * we want only ranges with attributes, but this one has none
        */
        if (rangesWithAttributeOnly && !range->hasAttribute())
            continue;

        /**
        * we want ranges for no view, but this one's attribute is only valid for views
        */
        if (!view && range->attributeOnlyForViews())
            continue;

        /**
        * the range's attribute is not valid for this view
        */
        if (range->view() && range->view() != view)
            continue;

        /**
        * if line is in the range, ok
        */
        if (range->startInternal().lineInternal() <= line && line <= range->endInternal().lineInternal())
          rightRanges.append (range);
    }
  }

  // return right ranges
  return rightRanges;
}

}
