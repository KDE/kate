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

#ifndef KATEUNDOMANAGER_H
#define KATEUNDOMANAGER_H

#include <QtCore/QObject>
#include <QtCore/QList>

class KateDocument;
class KateUndo;
class KateUndoGroup;

namespace KTextEditor {
  class Document;
  class View;
}

class QTimer;

/**
 * Central class to handle undo stuff.
 */
class KateUndoManager : public QObject
{
  Q_OBJECT

  public:
    /**
     * Constructor.
     * @param doc the document the KateUndoManager will belong to
     */
    KateUndoManager (KateDocument *doc);

    ~KateUndoManager();

  public:
    void undoStart();
    void undoEnd();

    /**
     * Marks the current KateUndoGroup as not mergable with following
     * undo groups.
     */
    void undoSafePoint();

    bool undoDontMerge() const;
    void setUndoDontMerge(bool dontMerge);

    bool undoDontMergeComplex() const;
    void setUndoDontMergeComplex(bool dontMerge);

    void setMergeAllEdits(bool merge) { m_mergeAllEdits = merge; m_firstMergeGroupSkipped = false; }
  public Q_SLOTS:  // FIXME make this slot private again?
    void undoCancel();

  public:
    void addUndo (KateUndo *undo);

  private:
    KateDocument *m_document;
    bool m_undoComplexMerge;
    KateUndoGroup* m_editCurrentUndo;

  //
  // KTextEditor::UndoInterface stuff
  //
  public Q_SLOTS:
    void undo ();
    void redo ();
    void clearUndo ();
    void clearRedo ();

  public:
    uint undoCount () const;
    uint redoCount () const;

  private Q_SLOTS:
    void viewCreated (KTextEditor::Document *, KTextEditor::View *newView);

  private:
    //
    // some internals for undo/redo
    //
    QList<KateUndoGroup*> undoItems;
    QList<KateUndoGroup*> redoItems;
    bool m_undoDontMerge; //create a setter later on and remove the friend declaration
    bool m_undoIgnoreCancel;
    bool m_mergeAllEdits; // if true, all undo groups are merged continually
    bool m_firstMergeGroupSkipped;  // used to make sure the first undo group isn't merged after
                                    // setting m_mergeAllEdits
    QTimer* m_undoMergeTimer;
    // these two variables are for resetting the document to
    // non-modified if all changes have been undone...
    KateUndoGroup* lastUndoGroupWhenSaved;
    KateUndoGroup* lastRedoGroupWhenSaved;
    bool docWasSavedWhenUndoWasEmpty;
    bool docWasSavedWhenRedoWasEmpty;

    // this sets
    void updateModified();

  Q_SIGNALS:
    void undoChanged ();

  public:
    void setModified( bool m );
    void updateConfig ();
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
