/* This file is part of the KDE libraries
   Copyright (C) 2009-2010 Bernhard Beschow <bbeschow@cs.tu-berlin.de>
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

#include "kateundo.h"

#include "kateundomanager.h"
#include "katedocument.h"

#include <ktexteditor/cursor.h>
#include <ktexteditor/view.h>

KateUndo::KateUndo (KateDocument *document)
: m_document (document)
{
}

KateUndo::~KateUndo ()
{
}

bool KateUndo::isEmpty() const
{
  return false;
}

bool KateEditInsertTextUndo::isEmpty() const
{
  return len() == 0;
}

bool KateEditRemoveTextUndo::isEmpty() const
{
  return len() == 0;
}

bool KateUndo::mergeWith (const KateUndo* /*undo*/)
{
  return false;
}

bool KateEditInsertTextUndo::mergeWith (const KateUndo* undo)
{
  const KateEditInsertTextUndo *u = dynamic_cast<const KateEditInsertTextUndo *> (undo);
  if (u != 0
      && m_line == u->m_line
      && (m_col + len()) == u->m_col)
  {
    m_text += u->m_text;
    return true;
  }

  return false;
}

bool KateEditRemoveTextUndo::mergeWith (const KateUndo* undo)
{
  const KateEditRemoveTextUndo *u = dynamic_cast<const KateEditRemoveTextUndo *> (undo);

  if (u != 0
      && m_line == u->m_line
      && m_col == (u->m_col + u->len()))
  {
    m_text.prepend(u->m_text);
    m_col = u->m_col;
    return true;
  }

  return false;
}

void KateEditInsertTextUndo::undo ()
{
  KateDocument *doc = document();

      doc->editRemoveText (m_line, m_col, len());
}

void KateEditRemoveTextUndo::undo ()
{
  KateDocument *doc = document();

      doc->editInsertText (m_line, m_col, m_text);
}

void KateEditWrapLineUndo::undo ()
{
  KateDocument *doc = document();

      doc->editUnWrapLine (m_line, m_newLine, m_len);
}

void KateEditUnWrapLineUndo::undo ()
{
  KateDocument *doc = document();

      doc->editWrapLine (m_line, m_col, m_removeLine);
}

void KateEditInsertLineUndo::undo ()
{
  KateDocument *doc = document();

      doc->editRemoveLine (m_line);
}

void KateEditRemoveLineUndo::undo ()
{
  KateDocument *doc = document();

      doc->editInsertLine (m_line, m_text);
}

void KateEditMarkLineAutoWrappedUndo::undo ()
{
  KateDocument *doc = document();

      doc->editMarkLineAutoWrapped (m_line, m_autowrapped);
}

void KateEditRemoveTextUndo::redo ()
{
  KateDocument *doc = document();

      doc->editRemoveText (m_line, m_col, len());
}

void KateEditInsertTextUndo::redo ()
{
  KateDocument *doc = document();

      doc->editInsertText (m_line, m_col, m_text);
}

void KateEditUnWrapLineUndo::redo ()
{
  KateDocument *doc = document();

      doc->editUnWrapLine (m_line, m_removeLine, m_len);
}

void KateEditWrapLineUndo::redo ()
{
  KateDocument *doc = document();

      doc->editWrapLine (m_line, m_col, m_newLine);
}

void KateEditRemoveLineUndo::redo ()
{
  KateDocument *doc = document();

      doc->editRemoveLine (m_line);
}

void KateEditInsertLineUndo::redo ()
{
  KateDocument *doc = document();

      doc->editInsertLine (m_line, m_text);
}

void KateEditMarkLineAutoWrappedUndo::redo ()
{
  KateDocument *doc = document();

      doc->editMarkLineAutoWrapped (m_line, m_autowrapped);
}

KateUndoGroup::KateUndoGroup (KateUndoManager *manager, const KTextEditor::Cursor &cursorPosition, const KTextEditor::Range &selectionRange)
  : m_manager (manager)
  , m_safePoint(false)
  , m_undoSelection(selectionRange)
  , m_redoSelection(-1, -1, -1, -1)
  , m_undoCursor(cursorPosition)
  , m_redoCursor(-1, -1)
{
}

KateUndoGroup::~KateUndoGroup ()
{
  qDeleteAll (m_items);
}

void KateUndoGroup::undo (KTextEditor::View *view)
{
  if (m_items.isEmpty())
    return;

  m_manager->startUndo ();

  for (int i=m_items.size()-1; i >= 0; --i)
    m_items[i]->undo();

  if (view != 0) {
    if (m_undoSelection.isValid())
      view->setSelection(m_undoSelection);
    else
      view->removeSelection();

    if (m_undoCursor.isValid())
      view->setCursorPosition(m_undoCursor);
  }

  m_manager->endUndo ();
}

void KateUndoGroup::redo (KTextEditor::View *view)
{
  if (m_items.isEmpty())
    return;

  m_manager->startUndo ();

  for (int i=0; i < m_items.size(); ++i)
    m_items[i]->redo();

  if (view != 0) {
    if (m_redoSelection.isValid())
      view->setSelection(m_redoSelection);
    else
      view->removeSelection();

    if (m_redoCursor.isValid())
      view->setCursorPosition(m_redoCursor);
  }

  m_manager->endUndo ();
}

void KateUndoGroup::editEnd(const KTextEditor::Cursor &cursorPosition, const KTextEditor::Range selectionRange)
{
  m_redoCursor = cursorPosition;
  m_redoSelection = selectionRange;
}

void KateUndoGroup::addItem(KateUndo* u)
{
  if (u->isEmpty())
    delete u;
  else if (!m_items.isEmpty() && m_items.last()->mergeWith(u))
    delete u;
  else
    m_items.append(u);
}

bool KateUndoGroup::merge (KateUndoGroup* newGroup,bool complex)
{
  if (m_safePoint)
    return false;

  if (newGroup->isOnlyType(singleType()) || complex) {
    // Take all of its items first -> last
    KateUndo* u = newGroup->m_items.isEmpty() ? 0 : newGroup->m_items.takeFirst ();
    while (u) {
      addItem(u);
      u = newGroup->m_items.isEmpty() ? 0 : newGroup->m_items.takeFirst ();
    }

    if (newGroup->m_safePoint)
      safePoint();

    m_redoCursor = newGroup->m_redoCursor;
    m_redoSelection = newGroup->m_redoSelection;

    return true;
  }

  return false;
}

void KateUndoGroup::safePoint (bool safePoint)
{
  m_safePoint=safePoint;
}

KTextEditor::Document *KateUndoGroup::document()
{
  return m_manager->document();
}

KateUndo::UndoType KateUndoGroup::singleType() const
{
  KateUndo::UndoType ret = KateUndo::editInvalid;

  Q_FOREACH(const KateUndo *item, m_items) {
    if (ret == KateUndo::editInvalid)
      ret = item->type();
    else if (ret != item->type())
      return KateUndo::editInvalid;
  }

  return ret;
}

bool KateUndoGroup::isOnlyType(KateUndo::UndoType type) const
{
  if (type == KateUndo::editInvalid) return false;

  Q_FOREACH(const KateUndo *item, m_items)
    if (item->type() != type)
      return false;

  return true;
}

// kate: space-indent on; indent-width 2; replace-tabs on;
