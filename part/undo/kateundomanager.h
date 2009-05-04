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

/**
 * Central class to handle undo stuff.
 *
 * KateUndoManager supports grouping of undo items.
 */
class KateUndoManager : public QObject
{
  Q_OBJECT

  public:
    /**
     * Creates a clean undo history.
     *
     * @param doc the document the KateUndoManager will belong to
     */
    KateUndoManager (KateDocument *doc);

    ~KateUndoManager();

  public:
    /**
     * @short Marks the start of a new undo group. New undo items may be added using addUndo().
     */
    void undoStart();

    /**
     * @short Marks the end of the undo group started by undoStart(). No more undo items may be added until
     * the next call to undoStart().
     */
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

    /**
     * @short Add an undo item to the current undo group. The undo item must be non-null.
     *
     * Call this method only when the following conditions are satisfied:
     * @li an edit is in progress, i.e. KateDocument::isEditRunning()
     * @li the document has undo enabled, i.e. KateDocument::isWithUndo()
     *
     * @param undo undo item to be added, must be non-null
     */
    void addUndo (KateUndo *undo);

  private:
    KateDocument *m_document;
    bool m_undoComplexMerge;
    KateUndoGroup* m_editCurrentUndo;

  public Q_SLOTS:
    void undo ();
    void redo ();
    void clearUndo ();
    void clearRedo ();

  public:
    uint undoCount () const;
    uint redoCount () const;

  private Q_SLOTS:
    void undoCancel();
    void viewCreated (KTextEditor::Document *, KTextEditor::View *newView);

  private:
    QList<KateUndoGroup*> undoItems;
    QList<KateUndoGroup*> redoItems;
    bool m_undoDontMerge;
    // these two variables are for resetting the document to
    // non-modified if all changes have been undone...
    KateUndoGroup* lastUndoGroupWhenSaved;
    KateUndoGroup* lastRedoGroupWhenSaved;
    bool docWasSavedWhenUndoWasEmpty;
    bool docWasSavedWhenRedoWasEmpty;

    void updateModified();

  Q_SIGNALS:
    void undoChanged ();

  public:
    void setModified( bool m );
    void updateConfig ();
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
