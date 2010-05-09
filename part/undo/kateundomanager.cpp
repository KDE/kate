/* This file is part of the KDE libraries
   Copyright (C) 2009-2010 Bernhard Beschow <bbeschow@cs.tu-berlin.de>

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
#include "kateundomanager.h"

#include <ktexteditor/view.h>

#include "katedocument.h"
#include "kateundo.h"

KateUndoManager::KateUndoManager (KateDocument *doc)
  : QObject (doc)
  , m_document (doc)
  , m_undoComplexMerge (false)
  , m_isActive (true)
  , m_editCurrentUndo (0)
  , lastUndoGroupWhenSaved(0)
  , lastRedoGroupWhenSaved(0)
  , docWasSavedWhenUndoWasEmpty(true)
  , docWasSavedWhenRedoWasEmpty(true)
{
  connect(this, SIGNAL(undoEnd(KTextEditor::Document*)), this, SIGNAL(undoChanged()));
  connect(this, SIGNAL(redoEnd(KTextEditor::Document*)), this, SIGNAL(undoChanged()));

  connect(doc, SIGNAL(viewCreated(KTextEditor::Document*, KTextEditor::View*)), SLOT(viewCreated(KTextEditor::Document*, KTextEditor::View*)));
}

KateUndoManager::~KateUndoManager()
{
  delete m_editCurrentUndo;

  // cleanup the undo/redo items, very important, truee :/
  qDeleteAll(undoItems);
  undoItems.clear();
  qDeleteAll(redoItems);
  redoItems.clear();
}

KTextEditor::Document *KateUndoManager::document()
{
  return m_document;
}

void KateUndoManager::viewCreated (KTextEditor::Document *, KTextEditor::View *newView)
{
  connect(newView, SIGNAL(cursorPositionChanged(KTextEditor::View*, const KTextEditor::Cursor&)), SLOT(undoCancel()));
}

void KateUndoManager::editStart()
{
  if (!m_isActive)
    return;

  // editStart() and editEnd() must be called in alternating fashion
  Q_ASSERT(m_editCurrentUndo == 0); // make sure to enter a clean state

  const KTextEditor::Cursor cursorPosition = activeView() ? activeView()->cursorPosition() : KTextEditor::Cursor::invalid();
  const KTextEditor::Range selectionRange = activeView() ? activeView()->selectionRange() : KTextEditor::Range::invalid();

  // new current undo item
  m_editCurrentUndo = new KateUndoGroup(this, cursorPosition, selectionRange);

  Q_ASSERT(m_editCurrentUndo != 0); // a new undo group must be created by this method
}

void KateUndoManager::editEnd()
{
  if (!m_isActive)
    return;

  // editStart() and editEnd() must be called in alternating fashion
  Q_ASSERT(m_editCurrentUndo != 0); // an undo group must have been created by editStart()

  const KTextEditor::Cursor cursorPosition = activeView() ? activeView()->cursorPosition() : KTextEditor::Cursor::invalid();
  const KTextEditor::Range selectionRange = activeView() ? activeView()->selectionRange() : KTextEditor::Range::invalid();

  m_editCurrentUndo->editEnd(cursorPosition, selectionRange);

    bool changedUndo = false;

    if (m_editCurrentUndo->isEmpty()) {
      delete m_editCurrentUndo;
    } else if (!undoItems.isEmpty()
        && undoItems.last()->merge(m_editCurrentUndo, m_undoComplexMerge)) {
      delete m_editCurrentUndo;
    } else {
      undoItems.append(m_editCurrentUndo);
      changedUndo = true;
    }

    m_editCurrentUndo = 0L;

    if (changedUndo)
      emit undoChanged();

  Q_ASSERT(m_editCurrentUndo == 0); // must be 0 after calling this method
}

void KateUndoManager::inputMethodStart()
{
  setActive(false);
  m_document->editStart();
}

void KateUndoManager::inputMethodEnd()
{
  m_document->editEnd();
  setActive(true);
}

void KateUndoManager::startUndo()
{
  setActive(false);
  m_document->editStart();
}

void KateUndoManager::endUndo()
{
  m_document->editEnd();
  setActive(true);
}

void KateUndoManager::slotTextInserted(int line, int col, const QString &s)
{
  if (m_editCurrentUndo != 0) // do we care about notifications?
    addUndoItem(new KateEditInsertTextUndo(m_document, line, col, s));
}

void KateUndoManager::slotTextRemoved(int line, int col, const QString &s)
{
  if (m_editCurrentUndo != 0) // do we care about notifications?
    addUndoItem(new KateEditRemoveTextUndo(m_document, line, col, s));
}

void KateUndoManager::slotMarkLineAutoWrapped(int line, bool autowrapped)
{
  if (m_editCurrentUndo != 0) // do we care about notifications?
    addUndoItem(new KateEditMarkLineAutoWrappedUndo(m_document, line, autowrapped));
}

void KateUndoManager::slotLineWrapped(int line, int col, int pos, bool newLine)
{
  if (m_editCurrentUndo != 0) // do we care about notifications?
    addUndoItem(new KateEditWrapLineUndo(m_document, line, col, pos, newLine));
}

void KateUndoManager::slotLineUnWrapped(int line, int col, int length, bool lineRemoved)
{
  if (m_editCurrentUndo != 0) // do we care about notifications?
    addUndoItem(new KateEditUnWrapLineUndo(m_document, line, col, length, lineRemoved));
}

void KateUndoManager::slotLineInserted(int line, const QString &s)
{
  if (m_editCurrentUndo != 0) // do we care about notifications?
    addUndoItem(new KateEditInsertLineUndo(m_document, line, s));
}

void KateUndoManager::slotLineRemoved(int line, const QString &s)
{
  if (m_editCurrentUndo != 0) // do we care about notifications?
    addUndoItem(new KateEditRemoveLineUndo(m_document, line, s));
}

void KateUndoManager::undoCancel()
{
  // Don't worry about this when an edit is in progress
  if (m_document->isEditRunning())
    return;

  undoSafePoint();
}

void KateUndoManager::undoSafePoint() {
  KateUndoGroup *undoGroup = m_editCurrentUndo;

  if (undoGroup == 0 && !undoItems.isEmpty())
    undoGroup = undoItems.last();

  if (undoGroup == 0)
    return;

  undoGroup->safePoint();
}

void KateUndoManager::addUndoItem(KateUndo *undo)
{
  Q_ASSERT(undo != 0); // don't add null pointers to our history
  Q_ASSERT(m_editCurrentUndo != 0); // make sure there is an undo group for our item

  m_editCurrentUndo->addItem(undo);

  // Clear redo buffer
  qDeleteAll(redoItems);
  redoItems.clear();
}

void KateUndoManager::setActive(bool enabled)
{
  Q_ASSERT(m_editCurrentUndo == 0); // must not already be in edit mode
  Q_ASSERT(m_isActive != enabled);

  m_isActive = enabled;

  emit isActiveChanged(enabled);
}

uint KateUndoManager::undoCount () const
{
  return undoItems.count ();
}

uint KateUndoManager::redoCount () const
{
  return redoItems.count ();
}

void KateUndoManager::undo()
{
  Q_ASSERT(m_editCurrentUndo == 0); // undo is not supported while we care about notifications (call editEnd() first)

  if (undoItems.count() > 0)
  {
    emit undoStart(document());

    undoItems.last()->undo(activeView());
    redoItems.append (undoItems.last());
    undoItems.removeLast ();
    updateModified();

    emit undoEnd(document());
  }
}

void KateUndoManager::redo()
{
  Q_ASSERT(m_editCurrentUndo == 0); // redo is not supported while we care about notifications (call editEnd() first)

  if (redoItems.count() > 0)
  {
    emit redoStart(document());

    redoItems.last()->redo(activeView());
    undoItems.append (redoItems.last());
    redoItems.removeLast ();
    updateModified();

    emit redoEnd(document());
  }
}

void KateUndoManager::updateModified()
{
  /*
  How this works:

    After noticing that there where to many scenarios to take into
    consideration when using 'if's to toggle the "Modified" flag
    I came up with this baby, flexible and repetitive calls are
    minimal.

    A numeric unique pattern is generated by toggling a set of bits,
    each bit symbolizes a different state in the Undo Redo structure.

      undoItems.isEmpty() != null          BIT 1
      redoItems.isEmpty() != null          BIT 2
      docWasSavedWhenUndoWasEmpty == true  BIT 3
      docWasSavedWhenRedoWasEmpty == true  BIT 4
      lastUndoGroupWhenSavedIsLastUndo     BIT 5
      lastUndoGroupWhenSavedIsLastRedo     BIT 6
      lastRedoGroupWhenSavedIsLastUndo     BIT 7
      lastRedoGroupWhenSavedIsLastRedo     BIT 8

    If you find a new pattern, please add it to the patterns array
  */

  unsigned char currentPattern = 0;
  const unsigned char patterns[] = {5,16,21,24,26,88,90,93,133,144,149,154,165};
  const unsigned char patternCount = sizeof(patterns);
  KateUndoGroup* undoLast = 0;
  KateUndoGroup* redoLast = 0;

  if (undoItems.isEmpty())
  {
    currentPattern |= 1;
  }
  else
  {
    undoLast = undoItems.last();
  }

  if (redoItems.isEmpty())
  {
    currentPattern |= 2;
  }
  else
  {
    redoLast = redoItems.last();
  }

  if (docWasSavedWhenUndoWasEmpty) currentPattern |= 4;
  if (docWasSavedWhenRedoWasEmpty) currentPattern |= 8;
  if (lastUndoGroupWhenSaved == undoLast) currentPattern |= 16;
  if (lastUndoGroupWhenSaved == redoLast) currentPattern |= 32;
  if (lastRedoGroupWhenSaved == undoLast) currentPattern |= 64;
  if (lastRedoGroupWhenSaved == redoLast) currentPattern |= 128;

  // This will print out the pattern information

  kDebug() << "Pattern:" << static_cast<unsigned int>(currentPattern);

  for (uint patternIndex = 0; patternIndex < patternCount; ++patternIndex)
  {
    if ( currentPattern == patterns[patternIndex] )
    {
      m_document->setModified( false );
      // (dominik) whenever the doc is not modified, succeeding edits
      // should not be merged
      undoSafePoint();
      kDebug() << "setting modified to false!";
      break;
    }
  }
}

void KateUndoManager::clearUndo()
{
  qDeleteAll(undoItems);
  undoItems.clear ();

  lastUndoGroupWhenSaved = 0;
  docWasSavedWhenUndoWasEmpty = false;

  emit undoChanged ();
}

void KateUndoManager::clearRedo()
{
  qDeleteAll(redoItems);
  redoItems.clear ();

  lastRedoGroupWhenSaved = 0;
  docWasSavedWhenRedoWasEmpty = false;

  emit undoChanged ();
}

void KateUndoManager::setModified(bool m) {
  if ( m == false )
  {
    if ( ! undoItems.isEmpty() )
    {
      lastUndoGroupWhenSaved = undoItems.last();
    }

    if ( ! redoItems.isEmpty() )
    {
      lastRedoGroupWhenSaved = redoItems.last();
    }

    docWasSavedWhenUndoWasEmpty = undoItems.isEmpty();
    docWasSavedWhenRedoWasEmpty = redoItems.isEmpty();
  }
}

void KateUndoManager::updateConfig ()
{
  emit undoChanged ();
}

void KateUndoManager::setAllowComplexMerge(bool allow)
{
  m_undoComplexMerge = allow;
}

KTextEditor::View* KateUndoManager::activeView()
{
  return m_document->activeView();
}

// kate: space-indent on; indent-width 2; replace-tabs on;
