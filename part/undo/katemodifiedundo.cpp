/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2011 Dominik Haumann <dhaumann@kde.org>
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

#include "katemodifiedundo.h"

#include "kateundomanager.h"
#include "katedocument.h"

#include <ktexteditor/cursor.h>
#include <ktexteditor/view.h>

KateModifiedInsertText::KateModifiedInsertText (KateDocument *document, int line, int col, const QString &text)
  : KateEditInsertTextUndo (document, line, col, text)
{
  setFlag(RedoLine1Modified);
  Kate::TextLine tl = document->plainKateTextLine(line);
  Q_ASSERT(tl);
  if (tl->markedAsModified()) {
    setFlag(UndoLine1Modified);
  } else {
    setFlag(UndoLine1Saved);
  }
}

KateModifiedRemoveText::KateModifiedRemoveText (KateDocument *document, int line, int col, const QString &text)
  : KateEditRemoveTextUndo (document, line, col, text)
{
  setFlag(RedoLine1Modified);
  Kate::TextLine tl = document->plainKateTextLine(line);
  Q_ASSERT(tl);
  if (tl->markedAsModified()) {
    setFlag(UndoLine1Modified);
  } else {
    setFlag(UndoLine1Saved);
  }
}

KateModifiedWrapLine::KateModifiedWrapLine (KateDocument *document, int line, int col, int len, bool newLine)
  : KateEditWrapLineUndo (document, line, col, len, newLine)
{
  Kate::TextLine tl = document->plainKateTextLine(line);
  Q_ASSERT(tl);
  if (len > 0 || tl->markedAsModified()) {
    setFlag(RedoLine1Modified);
  } else if (tl->markedAsSavedOnDisk()) {
    setFlag(RedoLine1Saved);
  }

  if (col > 0 || len == 0 || tl->markedAsModified()) {
    setFlag(RedoLine2Modified);
  } else if (tl->markedAsSavedOnDisk()) {
    setFlag(RedoLine2Saved);
  }

  if (tl->markedAsModified()) {
    setFlag(UndoLine1Modified);
  } else if ((len > 0  && col > 0) || tl->markedAsSavedOnDisk()) {
    setFlag(UndoLine1Saved);
  }
}

KateModifiedUnWrapLine::KateModifiedUnWrapLine (KateDocument *document, int line, int col, int len, bool removeLine)
  : KateEditUnWrapLineUndo (document, line, col, len, removeLine)
{
  Kate::TextLine tl = document->plainKateTextLine(line);
  Kate::TextLine nextLine = document->plainKateTextLine(line + 1);
  Q_ASSERT(tl);
  Q_ASSERT(nextLine);

  const int len1 = tl->length();
  const int len2 = nextLine->length();
  
  if (len1 > 0 && len2 > 0) {
    setFlag(RedoLine1Modified);

    if (tl->markedAsModified()) {
      setFlag(UndoLine1Modified);
    } else {
      setFlag(UndoLine1Saved);
    }

    if (nextLine->markedAsModified()) {
      setFlag(UndoLine2Modified);
    } else {
      setFlag(UndoLine2Saved);
    }
  } else if (len1 == 0) {
    if (nextLine->markedAsModified()) {
      setFlag(RedoLine1Modified);
    } else if (nextLine->markedAsSavedOnDisk()) {
      setFlag(RedoLine1Saved);
    }

    if (tl->markedAsModified()) {
      setFlag(UndoLine1Modified);
    } else {
      setFlag(UndoLine1Saved);
    }

    if (nextLine->markedAsModified()) {
      setFlag(UndoLine2Modified);
    } else if (nextLine->markedAsSavedOnDisk()) {
      setFlag(UndoLine2Saved);
    }
  } else { // len2 == 0
    if (nextLine->markedAsModified()) {
      setFlag(RedoLine1Modified);
    } else if (nextLine->markedAsSavedOnDisk()) {
      setFlag(RedoLine1Saved);
    }

    if (tl->markedAsModified()) {
      setFlag(UndoLine1Modified);
    } else if (tl->markedAsSavedOnDisk()) {
      setFlag(UndoLine1Saved);
    }

    if (nextLine->markedAsModified()) {
      setFlag(UndoLine2Modified);
    } else {
      setFlag(UndoLine2Saved);
    }
  }
}

KateModifiedInsertLine::KateModifiedInsertLine (KateDocument *document, int line, const QString &text)
  : KateEditInsertLineUndo (document, line, text)
{
  setFlag(RedoLine1Modified);
}

KateModifiedRemoveLine::KateModifiedRemoveLine (KateDocument *document, int line, const QString &text)
  : KateEditRemoveLineUndo (document, line, text)
{
  Kate::TextLine tl = document->plainKateTextLine(line);
  Q_ASSERT(tl);
  if (tl->markedAsModified()) {
    setFlag(UndoLine1Modified);
  } else {
    setFlag(UndoLine1Saved);
  }
}

void KateModifiedInsertText::undo ()
{
  KateEditInsertTextUndo::undo();

  KateDocument *doc = document();
  Kate::TextLine tl = doc->plainKateTextLine(line());
  Q_ASSERT(tl);

  tl->markAsModified(isFlagSet(UndoLine1Modified));
  tl->markAsSavedOnDisk(isFlagSet(UndoLine1Saved));
}

void KateModifiedRemoveText::undo ()
{
  KateEditRemoveTextUndo::undo();

  KateDocument *doc = document();
  Kate::TextLine tl = doc->plainKateTextLine(line());
  Q_ASSERT(tl);

  tl->markAsModified(isFlagSet(UndoLine1Modified));
  tl->markAsSavedOnDisk(isFlagSet(UndoLine1Saved));
}

void KateModifiedWrapLine::undo ()
{
  KateEditWrapLineUndo::undo();

  KateDocument *doc = document();
  Kate::TextLine tl = doc->plainKateTextLine(line());
  Q_ASSERT(tl);

  tl->markAsModified(isFlagSet(UndoLine1Modified));
  tl->markAsSavedOnDisk(isFlagSet(UndoLine1Saved));
}

void KateModifiedUnWrapLine::undo ()
{
  KateEditUnWrapLineUndo::undo();

  KateDocument *doc = document();
  Kate::TextLine tl = doc->plainKateTextLine(line());
  Q_ASSERT(tl);

  tl->markAsModified(isFlagSet(UndoLine1Modified));
  tl->markAsSavedOnDisk(isFlagSet(UndoLine1Saved));

  Kate::TextLine nextLine = doc->plainKateTextLine(line() + 1);
  Q_ASSERT(nextLine);
  nextLine->markAsModified(isFlagSet(UndoLine2Modified));
  nextLine->markAsSavedOnDisk(isFlagSet(UndoLine2Saved));
}

void KateModifiedInsertLine::undo ()
{
  KateEditInsertLineUndo::undo();

  // no line modification needed, since the line is removed
}

void KateModifiedRemoveLine::undo ()
{
  KateEditRemoveLineUndo::undo();

  KateDocument *doc = document();
  Kate::TextLine tl = doc->plainKateTextLine(line());
  Q_ASSERT(tl);

  tl->markAsModified(isFlagSet(UndoLine1Modified));
  tl->markAsSavedOnDisk(isFlagSet(UndoLine1Saved));
}


void KateModifiedRemoveText::redo ()
{
  KateEditRemoveTextUndo::redo();

  KateDocument *doc = document();
  Kate::TextLine tl = doc->plainKateTextLine(line());
  Q_ASSERT(tl);

  tl->markAsModified(isFlagSet(RedoLine1Modified));
  tl->markAsSavedOnDisk(isFlagSet(RedoLine1Saved));
}

void KateModifiedInsertText::redo ()
{
  KateEditInsertTextUndo::redo();

  KateDocument *doc = document();
  Kate::TextLine tl = doc->plainKateTextLine(line());
  Q_ASSERT(tl);

  tl->markAsModified(isFlagSet(RedoLine1Modified));
  tl->markAsSavedOnDisk(isFlagSet(RedoLine1Saved));
}

void KateModifiedUnWrapLine::redo ()
{
  KateEditUnWrapLineUndo::redo();

  KateDocument *doc = document();
  Kate::TextLine tl = doc->plainKateTextLine(line());
  Q_ASSERT(tl);

  tl->markAsModified(isFlagSet(RedoLine1Modified));
  tl->markAsSavedOnDisk(isFlagSet(RedoLine1Saved));
}

void KateModifiedWrapLine::redo ()
{
  KateEditWrapLineUndo::redo ();

  KateDocument *doc = document();
  Kate::TextLine tl = doc->plainKateTextLine(line());
  Q_ASSERT(tl);

  tl->markAsModified(isFlagSet(RedoLine1Modified));
  tl->markAsSavedOnDisk(isFlagSet(RedoLine1Saved));

  Kate::TextLine nextLine = doc->plainKateTextLine(line() + 1);
  Q_ASSERT(nextLine);

  nextLine->markAsModified(isFlagSet(RedoLine2Modified));
  nextLine->markAsSavedOnDisk(isFlagSet(RedoLine2Saved));
}

void KateModifiedRemoveLine::redo ()
{
  KateEditRemoveLineUndo::redo();

  // no line modification needed, since the line is removed
}

void KateModifiedInsertLine::redo ()
{
  KateEditInsertLineUndo::redo();

  KateDocument *doc = document();
  Kate::TextLine tl = doc->plainKateTextLine(line());
  Q_ASSERT(tl);

  tl->markAsModified(isFlagSet(RedoLine1Modified));
  tl->markAsSavedOnDisk(isFlagSet(RedoLine1Saved));
}

void KateModifiedInsertText::updateRedoSavedOnDiskFlag(QBitArray & lines)
{
  if (line() >= lines.size()) {
    lines.resize(line() + 1);
  }

  if (!lines.testBit(line())) {
    lines.setBit(line());

    unsetFlag(RedoLine1Modified);
    setFlag(RedoLine1Saved);
  }
}

void KateModifiedInsertText::updateUndoSavedOnDiskFlag(QBitArray & lines)
{
  if (line() >= lines.size()) {
    lines.resize(line() + 1);
  }

  if (!lines.testBit(line())) {
    lines.setBit(line());

    unsetFlag(UndoLine1Modified);
    setFlag(UndoLine1Saved);
  }
}

void KateModifiedRemoveText::updateRedoSavedOnDiskFlag(QBitArray & lines)
{
  if (line() >= lines.size()) {
    lines.resize(line() + 1);
  }

  if (!lines.testBit(line())) {
    lines.setBit(line());

    unsetFlag(RedoLine1Modified);
    setFlag(RedoLine1Saved);
  }
}

void KateModifiedRemoveText::updateUndoSavedOnDiskFlag(QBitArray & lines)
{
  if (line() >= lines.size()) {
    lines.resize(line() + 1);
  }

  if (!lines.testBit(line())) {
    lines.setBit(line());

    unsetFlag(UndoLine1Modified);
    setFlag(UndoLine1Saved);
  }
}

void KateModifiedWrapLine::updateRedoSavedOnDiskFlag(QBitArray & lines)
{
  if (line() + 1 >= lines.size()) {
    lines.resize(line() + 2);
  }

  if (isFlagSet(RedoLine1Modified) && !lines.testBit(line())) {
    lines.setBit(line());

    unsetFlag(RedoLine1Modified);
    setFlag(RedoLine1Saved);
  }

  if (isFlagSet(RedoLine2Modified) && !lines.testBit(line() + 1)) {
    lines.setBit(line() + 1);

    unsetFlag(RedoLine2Modified);
    setFlag(RedoLine2Saved);
  }
}

void KateModifiedWrapLine::updateUndoSavedOnDiskFlag(QBitArray & lines)
{
  if (line() >= lines.size()) {
    lines.resize(line() + 1);
  }

  if (isFlagSet(UndoLine1Modified) && !lines.testBit(line())) {
    lines.setBit(line());

    unsetFlag(UndoLine1Modified);
    setFlag(UndoLine1Saved);
  }
}

void KateModifiedUnWrapLine::updateRedoSavedOnDiskFlag(QBitArray & lines)
{
  if (line() >= lines.size()) {
    lines.resize(line() + 1);
  }

  if (isFlagSet(RedoLine1Modified) && !lines.testBit(line())) {
    lines.setBit(line());

    unsetFlag(RedoLine1Modified);
    setFlag(RedoLine1Saved);
  }
}

void KateModifiedUnWrapLine::updateUndoSavedOnDiskFlag(QBitArray & lines)
{
  if (line() + 1 >= lines.size()) {
    lines.resize(line() + 2);
  }

  if (isFlagSet(UndoLine1Modified) && !lines.testBit(line())) {
    lines.setBit(line());

    unsetFlag(UndoLine1Modified);
    setFlag(UndoLine1Saved);
  }

  if (isFlagSet(UndoLine2Modified) && !lines.testBit(line() + 1)) {
    lines.setBit(line() + 1);

    unsetFlag(UndoLine2Modified);
    setFlag(UndoLine2Saved);
  }
}

void KateModifiedInsertLine::updateRedoSavedOnDiskFlag(QBitArray & lines)
{
  if (line() >= lines.size()) {
    lines.resize(line() + 1);
  }

  if (!lines.testBit(line())) {
    lines.setBit(line());

    unsetFlag(RedoLine1Modified);
    setFlag(RedoLine1Saved);
  }
}

void KateModifiedRemoveLine::updateUndoSavedOnDiskFlag(QBitArray & lines)
{
  if (line() >= lines.size()) {
    lines.resize(line() + 1);
  }

  if (!lines.testBit(line())) {
    lines.setBit(line());

    unsetFlag(UndoLine1Modified);
    setFlag(UndoLine1Saved);
  }
}

// kate: space-indent on; indent-width 2; replace-tabs on;
