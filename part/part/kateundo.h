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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef kate_undo_h
#define kate_undo_h

#include <qptrlist.h>
#include <qstring.h>

class KateDocument;

class KateUndoGroup
{
  public:
    KateUndoGroup (KateDocument *doc);
    ~KateUndoGroup ();

    void undo ();
    void redo ();

    void addItem (uint type, uint line, uint col, uint len, const QString &text);

    bool merge(KateUndoGroup* newGroup);

    enum types
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

  private:
    // returns the type if it's only one type, or editInvalid if it contains multiple types.
    uint singleType();
    bool isOnlyType(uint type);

    void addItem(class KateUndo* u);

    KateDocument *m_doc;
    QPtrList<KateUndo> m_items;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
