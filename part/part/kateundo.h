/* This file is part of the KDE libraries
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

#ifndef kate_undo_h
#define kate_undo_h

#include <QList>
#include <QString>

class KateDocument;
class KateUndo;

/**
 * Class to manage a group of undo items
 */
class KateUndoGroup
{
  public:
    /**
     * Constructor
     * @param doc document to belong to
     */
    KateUndoGroup (KateDocument *doc);

    /**
     * Destructor
     */
    ~KateUndoGroup ();

  public:
    /**
     * Undo the contained undo items
     */
    void undo ();

    /**
     * Redo the contained undo items
     */
    void redo ();

  public:
    /**
     * Types for undo items
     */
    enum UndoType
    {
      editInsertText,
      editRemoveText,
      editWrapLine,
      editUnWrapLine,
      editInsertLine,
      editRemoveLine,
      editMarkLineAutoWrapped,
      editInvalid
    };

    /**
     * add an item to the group
     * @param type undo item type
     * @param line line affected
     * @param col start column
     * @param len lenght of change
     * @param text text removed/inserted
     */
    void addItem (KateUndoGroup::UndoType type, uint line, uint col, uint len, const QString &text);

    /**
     * merge this group with an other
     * @param newGroup group to merge into this one
     * @param complex set if a complex undo
     * @return success
     */
    bool merge(KateUndoGroup* newGroup,bool complex);

    /**
    * set group as as savepoint. the next group will not merge with this one
    */
    void safePoint (bool safePoint=true);

    /**
     * is this undogroup empty?
     */
    bool isEmpty () const { return m_items.isEmpty(); }

  private:
    /**
     * singleType
     * @return the type if it's only one type, or editInvalid if it contains multiple types.
     */
    KateUndoGroup::UndoType singleType();

    /**
     * are we only of this type ?
     * @param type type to query
     * @return we contain only the given type
     */
    bool isOnlyType(KateUndoGroup::UndoType type);

    /**
     * add an undo item
     * @param u item to add
     */
    void addItem (KateUndo *u);

  private:
    /**
     * Document we belong to
     */
    KateDocument *m_doc;

    /**
     * list of items contained
     */
    QList<KateUndo*> m_items;

    /**
     * prohibit merging with the next group
     */
    bool m_safePoint;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
