/* This file is part of the KDE libraries
   Copyright (C) 2002 John Firebaugh <jfirebaugh@kde.org>
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>

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

#include "kateundo.h"

#include "katedocument.h"
#include "kateview.h"

KateUndo::KateUndo (KateDocument *doc, uint type, uint line, uint col, uint len, QString text)
{
  this->myDoc = doc;
  this->type = type;
  this->line = line;
  this->col = col;
  this->len = len;
  this->text = text;
}

KateUndo::~KateUndo ()
{
}

void KateUndo::undo ()
{
  if (type == KateUndo::editInsertText)
  {
    myDoc->editRemoveText (line, col, len);
  }
  else if (type == KateUndo::editRemoveText)
  {
    myDoc->editInsertText (line, col, text);
  }
  else if (type == KateUndo::editWrapLine)
  {
    myDoc->editUnWrapLine (line, col);
  }
  else if (type == KateUndo::editUnWrapLine)
  {
    myDoc->editWrapLine (line, col);
  }
  else if (type == KateUndo::editInsertLine)
  {
    myDoc->editRemoveLine (line);
  }
  else if (type == KateUndo::editRemoveLine)
  {
    myDoc->editInsertLine (line, text);
  }
}

void KateUndo::redo ()
{
  if (type == KateUndo::editRemoveText)
  {
    myDoc->editRemoveText (line, col, len);
  }
  else if (type == KateUndo::editInsertText)
  {
    myDoc->editInsertText (line, col, text);
  }
  else if (type == KateUndo::editUnWrapLine)
  {
    myDoc->editUnWrapLine (line, col);
  }
  else if (type == KateUndo::editWrapLine)
  {
    myDoc->editWrapLine (line, col);
  }
  else if (type == KateUndo::editRemoveLine)
  {
    myDoc->editRemoveLine (line);
  }
  else if (type == KateUndo::editInsertLine)
  {
    myDoc->editInsertLine (line, text);
  }
}

KateUndoGroup::KateUndoGroup (KateDocument *doc)
{
  myDoc = doc;
}

KateUndoGroup::~KateUndoGroup ()
{
}

void KateUndoGroup::undo ()
{
  if (items.count() == 0)
    return;

  myDoc->editStart (false);

  for (int pos=(int)items.count()-1; pos >= 0; pos--)
  {
    items.at(pos)->undo();

    if (myDoc->myActiveView != 0L)
    {
      myDoc->myActiveView->myViewInternal->cursorCache.line = items.at(pos)->line;
      myDoc->myActiveView->myViewInternal->cursorCache.col = items.at(pos)->col;
      myDoc->myActiveView->myViewInternal->cursorCacheChanged = true;
    }
  }

  myDoc->editEnd ();
}

void KateUndoGroup::redo ()
{
  if (items.count() == 0)
    return;

  myDoc->editStart (false);

  for (uint pos=0; pos < items.count(); pos++)
  {
    items.at(pos)->redo();

    if (myDoc->myActiveView != 0L)
    {
      myDoc->myActiveView->myViewInternal->cursorCache.line = items.at(pos)->line;
      myDoc->myActiveView->myViewInternal->cursorCache.col = items.at(pos)->col;
      myDoc->myActiveView->myViewInternal->cursorCacheChanged = true;
    }
  }

  myDoc->editEnd ();
}

void KateUndoGroup::addItem (KateUndo *undo)
{
  items.append (undo);
}
