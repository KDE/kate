/* This file is part of the KDE libraries
   Copyright (C) 2009 Bernhard Beschow <bbeschow@cs.tu-berlin.de>

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

#include "katedocument.h"
#include "kateview.h"
#include "katesearchbar.h"
#include "kateundo.h"

KateUndoManager::KateUndoManager (KateDocument *doc)
  : QObject (doc)
  , m_document (doc)
  , m_undoComplexMerge (false)
  , m_editCurrentUndo (0)
  , m_undoDontMerge (false)
  , m_mergeAllEdits(false)
  , m_firstMergeGroupSkipped(false)
  , lastUndoGroupWhenSaved(0)
  , lastRedoGroupWhenSaved(0)
  , docWasSavedWhenUndoWasEmpty(true)
  , docWasSavedWhenRedoWasEmpty(true)
{
  connect(this, SIGNAL(undoChanged()), m_document, SIGNAL(undoChanged()));
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

void KateUndoManager::viewCreated (KTextEditor::Document *, KTextEditor::View *newView)
{
  connect(newView, SIGNAL(cursorPositionChanged(KTextEditor::View*, const KTextEditor::Cursor&)), SLOT(undoCancel()));
}

void KateUndoManager::undoStart()
{
  if (m_editCurrentUndo || (m_document->activeKateView() && m_document->activeKateView()->imComposeEvent()))
    return;

  // new current undo item
  m_editCurrentUndo = new KateUndoGroup(m_document);
  if (m_document->activeKateView()) {
    m_editCurrentUndo->setUndoCursor(m_document->activeKateView()->cursorPosition());
    m_editCurrentUndo->setUndoSelection(m_document->activeKateView()->selectionRange());
  }
}

void KateUndoManager::undoEnd()
{
  if (m_document->activeKateView() && m_document->activeKateView()->imComposeEvent())
    return;

  if (m_editCurrentUndo)
  {
    bool changedUndo = false;

    if (m_document->activeKateView()) {
        m_editCurrentUndo->setRedoCursor(m_document->activeKateView()->cursorPosition());
        m_editCurrentUndo->setRedoSelection(m_document->activeKateView()->selectionRange());
    }

    if (m_editCurrentUndo->isEmpty()) {
      delete m_editCurrentUndo;
    } else if (((m_mergeAllEdits && !m_firstMergeGroupSkipped) || !m_undoDontMerge)
        && !undoItems.isEmpty()
        && undoItems.last()->merge(m_editCurrentUndo, m_undoComplexMerge || m_mergeAllEdits)) {
      delete m_editCurrentUndo;
      m_firstMergeGroupSkipped = true;
    } else {
      undoItems.append(m_editCurrentUndo);
      changedUndo = true;
    }

    m_undoDontMerge = false;

    m_editCurrentUndo = 0L;

    if (changedUndo)
      emit undoChanged();
  }
}

void KateUndoManager::undoCancel()
{
  // Don't worry about this when an edit is in progress
  if (m_document->isEditRunning())
    return;

  m_undoDontMerge = true;

  Q_ASSERT(!m_editCurrentUndo);
}

void KateUndoManager::undoSafePoint() {
//  Q_ASSERT(m_editCurrentUndo);
  if (!m_editCurrentUndo)
    return;

  m_editCurrentUndo->safePoint();
}

void KateUndoManager::addUndo (KateUndo *undo)
{
  if (m_document->isEditRunning() && m_document->isWithUndo() && m_editCurrentUndo) {
    m_editCurrentUndo->addItem(undo);

    // Clear redo buffer
    if (redoItems.count()) {
      qDeleteAll(redoItems);
      redoItems.clear();
    }
  }
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
  if (undoItems.count() > 0)
  {
    //clearSelection ();
    /*Disable searchbar highlights due to performance issue
     * if undoGroup contains n items, and there're m search highlight regions,
     * the total cost is n*m*log(m),
     * to undo a simple Replace operation, n=2*m 
     * (replace contains both delete and insert undoItem, assume the replaced regions are highlighted),
     * cost = 2*m^2*log(m), too high
     * since there's a qStableSort inside KTextEditor::SmartRegion::rebuildChildStruct()
     */
    foreach (KTextEditor::View *v, m_document->views()) {
      KateView *view = qobject_cast<KateView*>(v);

      if (view->searchBar(false))
        view->searchBar(false)->enableHighlights(false);
    }

    undoItems.last()->undo();
    redoItems.append (undoItems.last());
    undoItems.removeLast ();
    updateModified();
    emit undoChanged ();
  }
}

void KateUndoManager::redo()
{
  if (redoItems.count() > 0)
  {
    //clearSelection ();
    //Disable searchbar highlights due to performance issue, see ::undo()'s comment
    foreach (KTextEditor::View *v, m_document->views()) {
      KateView *view = qobject_cast<KateView*>(v);

      if (view->searchBar(false))
        view->searchBar(false)->enableHighlights(false);
    }

    redoItems.last()->redo();
    undoItems.append (redoItems.last());
    redoItems.removeLast ();
    updateModified();

    emit undoChanged ();
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
  const unsigned char patterns[] = {5,16,21,24,26,88,90,93,133,144,149,165};
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
      setUndoDontMerge(true);
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

bool KateUndoManager::undoDontMerge( ) const
{
  return m_undoDontMerge;
}

void KateUndoManager::setUndoDontMergeComplex(bool dontMerge)
{
  m_undoComplexMerge = dontMerge;
}

bool KateUndoManager::undoDontMergeComplex() const
{
  return m_undoComplexMerge;
}

void KateUndoManager::setUndoDontMerge(bool dontMerge)
{
  m_undoDontMerge = dontMerge;
}

// kate: space-indent on; indent-width 2; replace-tabs on;
