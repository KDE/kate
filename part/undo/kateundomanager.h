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
 * KateUndoManager is a state machine which either accepts new undo items to be added or undo- and redo operations to be performed.
 * The state where new undo items are accepted is entered by calling undoStart(). Calling undoEnd() enters the state where undo-
 * and redo operations are accepted, which is also the state of a newly created KateUndoManager.
 *
 * KateUndoManager organizes single undo items into undo groups which are embraced by undoStart() and undoEnd(). As long as the
 * undo group is not finished by calling undoEnd(), any number of undo items may be added using addUndo(). Only in this state,
 * undoSafePoint() may be called.
 *
 * KateUndoManager supports merging of the last two undo groups. The merging happens when undoEnd() is called unless merging is
 * suppressed. Merging the current undo group with succeeding edit groups may be suppressed by calling undoSafePoint(), whereas
 * merging with previous undo groups may be suppressed using setUndoDontMerge(false). By default, undo groups only get merged
 * if all items are of the same type. Whether undo groups consisting of different types of undo items may be merged can be
 * controlled using setUndoDontMergeComplex().
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
     * Returns how many undo() actions can be performed.
     *
     * @return the number of undo groups which can be undone
     */
    uint undoCount () const;

    /**
     * Returns how many redo() actions can be performed.
     *
     * @return the number of undo groups which can be redone
     */
    uint redoCount () const;

    /**
     * Marks the current KateUndoGroup as not mergable with following
     * undo groups.
     */
    void undoSafePoint();

    bool undoDontMerge() const;
    void setUndoDontMerge(bool dontMerge);

    bool undoDontMergeComplex() const;
    void setUndoDontMergeComplex(bool dontMerge);

    void setModified( bool m );
    void updateConfig ();

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

  public Q_SLOTS:
    /**
     * Performs an undo action.
     *
     * Undo the last undo group.
     */
    void undo ();

    /**
     * Performs a redo action.
     *
     * Redo the earliest undo group.
     */
    void redo ();

    void clearUndo ();
    void clearRedo ();

  Q_SIGNALS:
    void undoChanged ();

  private:
    void updateModified();

  private Q_SLOTS:
    void undoCancel();
    void viewCreated (KTextEditor::Document *, KTextEditor::View *newView);

  private:
    KateDocument *m_document;
    bool m_undoComplexMerge;
    KateUndoGroup* m_editCurrentUndo;
    QList<KateUndoGroup*> undoItems;
    QList<KateUndoGroup*> redoItems;
    bool m_undoDontMerge;
    // these two variables are for resetting the document to
    // non-modified if all changes have been undone...
    KateUndoGroup* lastUndoGroupWhenSaved;
    KateUndoGroup* lastRedoGroupWhenSaved;
    bool docWasSavedWhenUndoWasEmpty;
    bool docWasSavedWhenRedoWasEmpty;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
