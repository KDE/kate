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
                          
// $Id$

#include "kateundo.h"

#include "katedocument.h"
#include "kateview.h"
     
/**
 Private class, only for KateUndoGroup, no need to use it elsewhere
 */                         
 class KateUndo
{
  public:
    KateUndo (uint type, uint line, uint col, uint len, const QString &text);
    ~KateUndo ();

  public:
    void undo (KateDocument *doc);
    void redo (KateDocument *doc);
    
    inline uint line () const { return m_line; }
    inline uint col () const { return m_col; }
    
  private:
    uint m_type;
    uint m_line;
    uint m_col;
    uint m_len;
    QString m_text;
};

KateUndo::KateUndo (uint type, uint line, uint col, uint len, const QString &text)
: m_type (type),
  m_line (line),
  m_col (col),
  m_len (len),
  m_text (text)  
{
}

KateUndo::~KateUndo ()
{
}

void KateUndo::undo (KateDocument *doc)
{
  if (m_type == KateUndoGroup::editInsertText)
  {
    doc->editRemoveText (m_line, m_col, m_len);
  }
  else if (m_type == KateUndoGroup::editRemoveText)
  {
    doc->editInsertText (m_line, m_col, m_text);
  }
  else if (m_type == KateUndoGroup::editWrapLine)
  {
    doc->editUnWrapLine (m_line, m_col);
  }
  else if (m_type == KateUndoGroup::editUnWrapLine)
  {
    doc->editWrapLine (m_line, m_col);
  }
  else if (m_type == KateUndoGroup::editInsertLine)
  {
    doc->editRemoveLine (m_line);
  }
  else if (m_type == KateUndoGroup::editRemoveLine)
  {
    doc->editInsertLine (m_line, m_text);
  }
}

void KateUndo::redo (KateDocument *doc)
{
  if (m_type == KateUndoGroup::editRemoveText)
  {
    doc->editRemoveText (m_line, m_col, m_len);
  }
  else if (m_type == KateUndoGroup::editInsertText)
  {
    doc->editInsertText (m_line, m_col, m_text);
  }
  else if (m_type == KateUndoGroup::editUnWrapLine)
  {
    doc->editUnWrapLine (m_line, m_col);
  }
  else if (m_type == KateUndoGroup::editWrapLine)
  {
    doc->editWrapLine (m_line, m_col);
  }
  else if (m_type == KateUndoGroup::editRemoveLine)
  {
    doc->editRemoveLine (m_line);
  }
  else if (m_type == KateUndoGroup::editInsertLine)
  {
    doc->editInsertLine (m_line, m_text);
  }
}

KateUndoGroup::KateUndoGroup (KateDocument *doc)
: m_doc (doc)  
{  
  m_items.setAutoDelete (true);
}

KateUndoGroup::~KateUndoGroup ()
{
}

void KateUndoGroup::undo ()
{
  if (m_items.count() == 0)
    return;

  m_doc->editStart (false);

  for (int pos=(int)m_items.count()-1; pos >= 0; pos--)
  {
    m_items.at(pos)->undo(m_doc);

    if (m_doc->myActiveView != 0L)
    {
      m_doc->myActiveView->m_viewInternal->cursorCache.line = m_items.at(pos)->line();
      m_doc->myActiveView->m_viewInternal->cursorCache.col = m_items.at(pos)->col();
      m_doc->myActiveView->m_viewInternal->cursorCacheChanged = true;
    }
  }

  m_doc->editEnd ();
}

void KateUndoGroup::redo ()
{
  if (m_items.count() == 0)
    return;

  m_doc->editStart (false);

  for (uint pos=0; pos < m_items.count(); pos++)
  {
    m_items.at(pos)->redo(m_doc);

    if (m_doc->myActiveView != 0L)
    {
      m_doc->myActiveView->m_viewInternal->cursorCache.line = m_items.at(pos)->line();
      m_doc->myActiveView->m_viewInternal->cursorCache.col = m_items.at(pos)->col();
      m_doc->myActiveView->m_viewInternal->cursorCacheChanged = true;
    }
  }

  m_doc->editEnd ();
}

void KateUndoGroup::addItem (uint type, uint line, uint col, uint len, const QString &text)
{
  m_items.append (new KateUndo (type, line, col, len, text));
}
