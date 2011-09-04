/* This file is part of the KDE libraries
   Copyright (C) 2009-2010 Bernhard Beschow <bbeschow@cs.tu-berlin.de>
   Copyright (C) 2002 John Firebaugh <jfirebaugh@kde.org>
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>

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

#include "kateundo.h"

#include "kateundomanager.h"
#include "katedocument.h"

#include <ktexteditor/cursor.h>
#include <ktexteditor/view.h>

KateUndo::KateUndo (KateDocument *document)
: m_document (document)
, m_lineModFlags(0x00)
{
}

KateUndo::~KateUndo ()
{
}

void KateUndo::setFlag(ModificationFlag flag)
{
  m_lineModFlags |= flag;
}

void KateUndo::unsetFlag(ModificationFlag flag)
{
  m_lineModFlags &= (~flag);
}

bool KateUndo::isFlagSet(ModificationFlag flag) const
{
  return m_lineModFlags & flag;
}

KateEditInsertTextUndo::KateEditInsertTextUndo (KateDocument *document, int line, int col, const QString &text)
  : KateUndo (document)
  , m_line (line)
  , m_col (col)
  , m_text (text)
{
  setFlag(RedoLine1Modified);
  Kate::TextLine tl = document->plainKateTextLine(line);
  Q_ASSERT(tl);
  if (tl->markedAsModified()) {
    setFlag(UndoLine1Modified);
  }
  if (tl->markedAsSavedOnDisk()) {
    setFlag(UndoLine1Saved);
  }
}

KateEditRemoveTextUndo::KateEditRemoveTextUndo (KateDocument *document, int line, int col, const QString &text)
  : KateUndo (document)
  , m_line (line)
  , m_col (col)
  , m_text (text)
{
  setFlag(RedoLine1Modified);
  Kate::TextLine tl = document->plainKateTextLine(line);
  Q_ASSERT(tl);
  if (tl->markedAsModified()) {
    setFlag(UndoLine1Modified);
  }
  if (tl->markedAsSavedOnDisk()) {
    setFlag(UndoLine1Saved);
  }
}

KateEditWrapLineUndo::KateEditWrapLineUndo (KateDocument *document, int line, int col, int len, bool newLine)
  : KateUndo (document)
  , m_line (line)
  , m_col (col)
  , m_len (len)
  , m_newLine (newLine)
{
  Kate::TextLine tl = document->plainKateTextLine(line);
  Q_ASSERT(tl);
  if (col < len || tl->markedAsModified()) {
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
  }
  if (tl->markedAsSavedOnDisk()) {
    setFlag(UndoLine1Saved);
  }
}

KateEditUnWrapLineUndo::KateEditUnWrapLineUndo (KateDocument *document, int line, int col, int len, bool removeLine)
  : KateUndo (document)
  , m_line (line)
  , m_col (col)
  , m_len (len)
  , m_removeLine (removeLine)
{
  setFlag(RedoLine1Modified);

  Kate::TextLine tl = document->plainKateTextLine(line);
  Q_ASSERT(tl);
  if (tl->markedAsModified()) {
    setFlag(UndoLine1Modified);
  }
  if (tl->markedAsSavedOnDisk()) {
    setFlag(UndoLine1Saved);
  }

  Kate::TextLine nextLine = document->plainKateTextLine(line + 1);
  Q_ASSERT(nextLine);
  if (nextLine->markedAsModified()) {
    setFlag(UndoLine2Modified);
  }
  if (nextLine->markedAsSavedOnDisk()) {
    setFlag(UndoLine2Saved);
  }
}

KateEditInsertLineUndo::KateEditInsertLineUndo (KateDocument *document, int line, const QString &text)
  : KateUndo (document)
  , m_line (line)
  , m_text (text)
{
  setFlag(RedoLine1Modified);
}

KateEditRemoveLineUndo::KateEditRemoveLineUndo (KateDocument *document, int line, const QString &text)
  : KateUndo (document)
  , m_line (line)
  , m_text (text)
{
  Kate::TextLine tl = document->plainKateTextLine(line);
  Q_ASSERT(tl);
  if (tl->markedAsModified()) {
    setFlag(UndoLine1Modified);
  }
  if (tl->markedAsSavedOnDisk()) {
    setFlag(UndoLine1Saved);
  }
}


bool KateUndo::isEmpty() const
{
  return false;
}

bool KateEditInsertTextUndo::isEmpty() const
{
  return len() == 0;
}

bool KateEditRemoveTextUndo::isEmpty() const
{
  return len() == 0;
}

bool KateUndo::mergeWith (const KateUndo* /*undo*/)
{
  return false;
}

bool KateEditInsertTextUndo::mergeWith (const KateUndo* undo)
{
  const KateEditInsertTextUndo *u = dynamic_cast<const KateEditInsertTextUndo *> (undo);
  if (u != 0
      && m_line == u->m_line
      && (m_col + len()) == u->m_col)
  {
    m_text += u->m_text;
    return true;
  }

  return false;
}

bool KateEditRemoveTextUndo::mergeWith (const KateUndo* undo)
{
  const KateEditRemoveTextUndo *u = dynamic_cast<const KateEditRemoveTextUndo *> (undo);

  if (u != 0
      && m_line == u->m_line
      && m_col == (u->m_col + u->len()))
  {
    m_text.prepend(u->m_text);
    m_col = u->m_col;
    return true;
  }

  return false;
}

void KateEditInsertTextUndo::undo ()
{
  KateDocument *doc = document();

  doc->editRemoveText (m_line, m_col, len());

  Kate::TextLine tl = doc->plainKateTextLine(m_line);
  Q_ASSERT(tl);
  tl->markAsModified(isFlagSet(UndoLine1Modified));
  tl->markAsSavedOnDisk(isFlagSet(UndoLine1Saved));
}

void KateEditRemoveTextUndo::undo ()
{
  KateDocument *doc = document();

  doc->editInsertText (m_line, m_col, m_text);

  Kate::TextLine tl = doc->plainKateTextLine(m_line);
  Q_ASSERT(tl);
  tl->markAsModified(isFlagSet(UndoLine1Modified));
  tl->markAsSavedOnDisk(isFlagSet(UndoLine1Saved));
}

void KateEditWrapLineUndo::undo ()
{
  KateDocument *doc = document();

  doc->editUnWrapLine (m_line, m_newLine, m_len);

  Kate::TextLine tl = doc->plainKateTextLine(m_line);
  Q_ASSERT(tl);
  tl->markAsModified(isFlagSet(UndoLine1Modified));
  tl->markAsSavedOnDisk(isFlagSet(UndoLine1Saved));
}

void KateEditUnWrapLineUndo::undo ()
{
  KateDocument *doc = document();

  doc->editWrapLine (m_line, m_col, m_removeLine);

  Kate::TextLine tl = doc->plainKateTextLine(m_line);
  Q_ASSERT(tl);
  tl->markAsModified(isFlagSet(UndoLine1Modified));
  tl->markAsSavedOnDisk(isFlagSet(UndoLine1Saved));

  Kate::TextLine nextLine = doc->plainKateTextLine(m_line + 1);
  Q_ASSERT(nextLine);
  nextLine->markAsModified(isFlagSet(UndoLine2Modified));
  nextLine->markAsSavedOnDisk(isFlagSet(UndoLine2Saved));
}

void KateEditInsertLineUndo::undo ()
{
  KateDocument *doc = document();

  doc->editRemoveLine (m_line);

  // no line modification needed, since the line is removed
}

void KateEditRemoveLineUndo::undo ()
{
  KateDocument *doc = document();

  doc->editInsertLine (m_line, m_text);

  Kate::TextLine tl = doc->plainKateTextLine(m_line);
  Q_ASSERT(tl);
  tl->markAsModified(isFlagSet(UndoLine1Modified));
  tl->markAsSavedOnDisk(isFlagSet(UndoLine1Saved));
}

void KateEditMarkLineAutoWrappedUndo::undo ()
{
  KateDocument *doc = document();

  doc->editMarkLineAutoWrapped (m_line, m_autowrapped);
}

void KateEditRemoveTextUndo::redo ()
{
  KateDocument *doc = document();

  doc->editRemoveText (m_line, m_col, len());

  Kate::TextLine tl = doc->plainKateTextLine(m_line);
  Q_ASSERT(tl);
  tl->markAsModified(isFlagSet(RedoLine1Modified));
  tl->markAsSavedOnDisk(isFlagSet(RedoLine1Saved));
}

void KateEditInsertTextUndo::redo ()
{
  KateDocument *doc = document();

  doc->editInsertText (m_line, m_col, m_text);

  Kate::TextLine tl = doc->plainKateTextLine(m_line);
  Q_ASSERT(tl);
  tl->markAsModified(isFlagSet(RedoLine1Modified));
  tl->markAsSavedOnDisk(isFlagSet(RedoLine1Saved));
}

void KateEditUnWrapLineUndo::redo ()
{
  KateDocument *doc = document();

  doc->editUnWrapLine (m_line, m_removeLine, m_len);

  Kate::TextLine tl = doc->plainKateTextLine(m_line);
  Q_ASSERT(tl);
  tl->markAsModified(isFlagSet(RedoLine1Modified));
  tl->markAsSavedOnDisk(isFlagSet(RedoLine1Saved));
}

void KateEditWrapLineUndo::redo ()
{
  KateDocument *doc = document();

  doc->editWrapLine (m_line, m_col, m_newLine);

  Kate::TextLine tl = doc->plainKateTextLine(m_line);
  Q_ASSERT(tl);
  tl->markAsModified(isFlagSet(RedoLine1Modified));
  tl->markAsSavedOnDisk(isFlagSet(RedoLine1Saved));

  Kate::TextLine nextLine = doc->plainKateTextLine(m_line + 1);
  Q_ASSERT(nextLine);
  nextLine->markAsModified(isFlagSet(RedoLine2Modified));
  nextLine->markAsSavedOnDisk(isFlagSet(RedoLine2Saved));
}

void KateEditRemoveLineUndo::redo ()
{
  KateDocument *doc = document();

  doc->editRemoveLine (m_line);

  // no line modification needed, since the line is removed
}

void KateEditInsertLineUndo::redo ()
{
  KateDocument *doc = document();

  doc->editInsertLine (m_line, m_text);

  Kate::TextLine tl = doc->plainKateTextLine(m_line);
  Q_ASSERT(tl);
  tl->markAsModified(isFlagSet(RedoLine1Modified));
  tl->markAsSavedOnDisk(isFlagSet(RedoLine1Saved));
}

void KateEditMarkLineAutoWrappedUndo::redo ()
{
  KateDocument *doc = document();

  doc->editMarkLineAutoWrapped (m_line, m_autowrapped);
}

void KateEditInsertTextUndo::updateRedoSavedOnDiskFlag(QBitArray & lines)
{
  if (m_line >= lines.size()) {
    lines.resize(m_line + 1);
  }

  if (!lines.testBit(m_line)) {
    lines.setBit(m_line);

    unsetFlag(RedoLine1Modified);
    setFlag(RedoLine1Saved);
  }
}

void KateEditInsertTextUndo::updateUndoSavedOnDiskFlag(QBitArray & lines)
{
  if (m_line >= lines.size()) {
    lines.resize(m_line + 1);
  }

  if (!lines.testBit(m_line)) {
    lines.setBit(m_line);

    unsetFlag(UndoLine1Modified);
    setFlag(UndoLine1Saved);
  }
}

void KateEditRemoveTextUndo::updateRedoSavedOnDiskFlag(QBitArray & lines)
{
  if (m_line >= lines.size()) {
    lines.resize(m_line + 1);
  }

  if (!lines.testBit(m_line)) {
    lines.setBit(m_line);

    unsetFlag(RedoLine1Modified);
    setFlag(RedoLine1Saved);
  }
}

void KateEditRemoveTextUndo::updateUndoSavedOnDiskFlag(QBitArray & lines)
{
  if (m_line >= lines.size()) {
    lines.resize(m_line + 1);
  }

  if (!lines.testBit(m_line)) {
    lines.setBit(m_line);

    unsetFlag(UndoLine1Modified);
    setFlag(UndoLine1Saved);
  }
}

void KateEditWrapLineUndo::updateRedoSavedOnDiskFlag(QBitArray & lines)
{
  if (m_line + 1 >= lines.size()) {
    lines.resize(m_line + 2);
  }

  if (isFlagSet(RedoLine1Modified) && !lines.testBit(m_line)) {
    lines.setBit(m_line);

    unsetFlag(RedoLine1Modified);
    setFlag(RedoLine1Saved);
  }

  if (isFlagSet(RedoLine2Modified) && !lines.testBit(m_line + 1)) {
    lines.setBit(m_line + 1);

    unsetFlag(RedoLine2Modified);
    setFlag(RedoLine2Saved);
  }
}

void KateEditWrapLineUndo::updateUndoSavedOnDiskFlag(QBitArray & lines)
{
  if (m_line >= lines.size()) {
    lines.resize(m_line + 1);
  }

  if (isFlagSet(UndoLine1Modified) && !lines.testBit(m_line)) {
    lines.setBit(m_line);

    unsetFlag(UndoLine1Modified);
    setFlag(UndoLine1Saved);
  }
}

void KateEditUnWrapLineUndo::updateRedoSavedOnDiskFlag(QBitArray & lines)
{
  if (m_line >= lines.size()) {
    lines.resize(m_line + 1);
  }

  if (isFlagSet(RedoLine1Modified) && !lines.testBit(m_line)) {
    lines.setBit(m_line);

    unsetFlag(RedoLine1Modified);
    setFlag(RedoLine1Saved);
  }
}

void KateEditUnWrapLineUndo::updateUndoSavedOnDiskFlag(QBitArray & lines)
{
  if (m_line + 1 >= lines.size()) {
    lines.resize(m_line + 2);
  }

  if (isFlagSet(UndoLine1Modified) && !lines.testBit(m_line)) {
    lines.setBit(m_line);

    unsetFlag(UndoLine1Modified);
    setFlag(UndoLine1Saved);
  }

  if (isFlagSet(UndoLine2Modified) && !lines.testBit(m_line + 1)) {
    lines.setBit(m_line + 1);

    unsetFlag(UndoLine2Modified);
    setFlag(UndoLine2Saved);
  }
}

KateUndoGroup::KateUndoGroup (KateUndoManager *manager, const KTextEditor::Cursor &cursorPosition, const KTextEditor::Range &selectionRange)
  : m_manager (manager)
  , m_safePoint(false)
  , m_undoSelection(selectionRange)
  , m_redoSelection(-1, -1, -1, -1)
  , m_undoCursor(cursorPosition)
  , m_redoCursor(-1, -1)
{
}

KateUndoGroup::~KateUndoGroup ()
{
  qDeleteAll (m_items);
}

void KateUndoGroup::undo (KTextEditor::View *view)
{
  if (m_items.isEmpty())
    return;

  m_manager->startUndo ();

  for (int i=m_items.size()-1; i >= 0; --i)
    m_items[i]->undo();

  if (view != 0) {
    if (m_undoSelection.isValid())
      view->setSelection(m_undoSelection);
    else
      view->removeSelection();

    if (m_undoCursor.isValid())
      view->setCursorPosition(m_undoCursor);
  }

  m_manager->endUndo ();
}

void KateUndoGroup::redo (KTextEditor::View *view)
{
  if (m_items.isEmpty())
    return;

  m_manager->startUndo ();

  for (int i=0; i < m_items.size(); ++i)
    m_items[i]->redo();

  if (view != 0) {
    if (m_redoSelection.isValid())
      view->setSelection(m_redoSelection);
    else
      view->removeSelection();

    if (m_redoCursor.isValid())
      view->setCursorPosition(m_redoCursor);
  }

  m_manager->endUndo ();
}

void KateUndoGroup::editEnd(const KTextEditor::Cursor &cursorPosition, const KTextEditor::Range selectionRange)
{
  m_redoCursor = cursorPosition;
  m_redoSelection = selectionRange;
}

void KateUndoGroup::addItem(KateUndo* u)
{
  if (u->isEmpty())
    delete u;
  else if (!m_items.isEmpty() && m_items.last()->mergeWith(u))
    delete u;
  else
    m_items.append(u);
}

bool KateUndoGroup::merge (KateUndoGroup* newGroup,bool complex)
{
  if (m_safePoint)
    return false;

  if (newGroup->isOnlyType(singleType()) || complex) {
    // Take all of its items first -> last
    KateUndo* u = newGroup->m_items.isEmpty() ? 0 : newGroup->m_items.takeFirst ();
    while (u) {
      addItem(u);
      u = newGroup->m_items.isEmpty() ? 0 : newGroup->m_items.takeFirst ();
    }

    if (newGroup->m_safePoint)
      safePoint();

    m_redoCursor = newGroup->m_redoCursor;
    m_redoSelection = newGroup->m_redoSelection;

    return true;
  }

  return false;
}

void KateUndoGroup::safePoint (bool safePoint)
{
  m_safePoint=safePoint;
}

void KateUndoGroup::flagSavedAsModified()
{
  foreach (KateUndo *item, m_items) {
    if (item->isFlagSet(KateUndo::UndoLine1Saved)) {
      item->unsetFlag(KateUndo::UndoLine1Saved);
      item->setFlag(KateUndo::UndoLine1Modified);
    }

    if (item->isFlagSet(KateUndo::UndoLine2Saved)) {
      item->unsetFlag(KateUndo::UndoLine2Saved);
      item->setFlag(KateUndo::UndoLine2Modified);
    }

    if (item->isFlagSet(KateUndo::RedoLine1Saved)) {
      item->unsetFlag(KateUndo::RedoLine1Saved);
      item->setFlag(KateUndo::RedoLine1Modified);
    }

    if (item->isFlagSet(KateUndo::RedoLine2Saved)) {
      item->unsetFlag(KateUndo::RedoLine2Saved);
      item->setFlag(KateUndo::RedoLine2Modified);
    }
  }
}

void KateUndoGroup::markUndoAsSaved(QBitArray & lines)
{
  for (int i = m_items.size() - 1; i >= 0; --i) {
    KateUndo* item = m_items[i];
    item->updateUndoSavedOnDiskFlag(lines);
  }
}

void KateUndoGroup::markRedoAsSaved(QBitArray & lines)
{
  for (int i = m_items.size() - 1; i >= 0; --i) {
    KateUndo* item = m_items[i];
    item->updateRedoSavedOnDiskFlag(lines);
  }
}

KTextEditor::Document *KateUndoGroup::document()
{
  return m_manager->document();
}

KateUndo::UndoType KateUndoGroup::singleType() const
{
  KateUndo::UndoType ret = KateUndo::editInvalid;

  Q_FOREACH(const KateUndo *item, m_items) {
    if (ret == KateUndo::editInvalid)
      ret = item->type();
    else if (ret != item->type())
      return KateUndo::editInvalid;
  }

  return ret;
}

bool KateUndoGroup::isOnlyType(KateUndo::UndoType type) const
{
  if (type == KateUndo::editInvalid) return false;

  Q_FOREACH(const KateUndo *item, m_items)
    if (item->type() != type)
      return false;

  return true;
}

// kate: space-indent on; indent-width 2; replace-tabs on;
