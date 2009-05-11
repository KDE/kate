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
 * KateUndoManager implements a document's history. It is in either of the two states:
 * @li the default state, which allows rolling back and forth the history of a document, and
 * @li a state in which a new element is being added to the history.
 *
 * The state of the KateUndomanager can be switched using undoStart() and undoEnd().
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
     * @short Starts a new undo group.
     */
    void undoStart();

    /**
     * @short Adds the undo group started by undoStart() to the history.
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
     * Prevent the current KateUndoGroup from being merged with the next one.
     */
    void undoSafePoint();

    bool undoDontMerge() const;

    /**
     * Allows or disallows merging with previous undo group.
     *
     * @param dontMerge whether merging is not allowed
     */
    void setUndoDontMerge(bool dontMerge);

    bool undoDontMergeComplex() const;

    /**
     * Allows or disallows merging of "complex" undo groups.
     *
     * When an undo group contains different types of undo items, it is considered
     * a "complex" group.
     *
     * @param dontMerge whether complex merging is allowed
     */
    void setUndoDontMergeComplex(bool dontMerge);

    void setModified( bool m );
    void updateConfig ();

  public Q_SLOTS:
    /**
     * Undo the latest undo group.
     *
     * Make sure isDefaultState() is true when calling this method.
     */
    void undo ();

    /**
     * Redo the latest undo group.
     *
     * Make sure isDefaultState() is true when calling this method.
     */
    void redo ();

    void clearUndo ();
    void clearRedo ();

    /**
     * Notify KateUndoManager that text was inserted.
     */
    void slotTextInserted(int line, int col, const QString &s);

    /**
     * Notify KateUndoManager that text was removed.
     */
    void slotTextRemoved(int line, int col, const QString &s);

    /**
     * Notify KateUndoManager that a line was marked as autowrapped.
     */
    void slotMarkLineAutoWrapped(int line, bool autowrapped);

    /**
     * Notify KateUndoManager that a line was wrapped.
     */
    void slotLineWrapped(int line, int col, int pos, bool newLine);

    /**
     * Notify KateUndoManager that a line was un-wrapped.
     */
    void slotLineUnWrapped(int line, int col, int length, bool lineRemoved);

    /**
     * Notify KateUndoManager that a line was inserted.
     */
    void slotLineInserted(int line, const QString &s);

    /**
     * Notify KateUndoManager that a line was removed.
     */
    void slotLineRemoved(int line, const QString &s);

  Q_SIGNALS:
    void undoChanged ();

  private:
    /**
     * @short Add an undo item to the current undo group. The undo item must be non-null.
     *
     * @param undo undo item to be added, must be non-null
     */
    void addUndoItem(KateUndo *undo);

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
