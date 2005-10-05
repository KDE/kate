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

#include "kateundo.h"

#include "katedocument.h"
#include "kateview.h"
#include "katecursor.h"

/**
 * Private class, only for KateUndoGroup, no need to use it elsewhere
 */
 class KateUndo
{
  public:
    /**
     * Constructor
     * @param type undo item type
     * @param line line affected
     * @param col start column
     * @param len lenght of change
     * @param text text removed/inserted
     */
    KateUndo (KateUndoGroup::UndoType type, uint line, uint col, uint len, const QString &text);

    /**
     * Destructor
     */
    ~KateUndo ();

  public:
    /**
     * Invalid examples: insert / remove 0 length text
     * I could probably fix this in KateDocument, but it's more work there
     * (and probably better here too)
     * @return validity
     */
    bool isValid();

    /**
     * merge an undo item
     * Saves a bit of memory and potentially many calls when undo/redoing.
     * @param u undo item to merge
     * @return success
     */
    bool merge(KateUndo* u);

    /**
     * undo this item at given doc
     * @param doc document
     */
    void undo (KateDocument *doc);

    /**
     * redo this item at given doc
     * @param doc document
     */
    void redo (KateDocument *doc);

    /**
     * The cursor before the action took place
     */
    KTextEditor::Cursor cursorBefore() const;

    /**
     * The cursor after the action took place
     */
    KTextEditor::Cursor cursorAfter() const;

    /**
     * type of item
     * @return type
     */
    inline KateUndoGroup::UndoType type() const { return m_type; }

    /**
     * line of changes
     * @return line
     */
    inline uint line () const { return m_line; }

    /**
     * startcol of changes
     * @return column
     */
    inline uint col () const { return m_col; }

    /**
     * length of changes
     * @return length
     */
    inline uint len() const { return m_len; }

    /**
     * text inserted/removed
     * @return text
     */
    inline const QString& text() const { return m_text; };

  private:
    /**
     * type
     */
    KateUndoGroup::UndoType m_type;

    /**
     * line
     */
    uint m_line;

    /**
     * column
     */
    uint m_col;

    /**
     * length
     */
    uint m_len;

    /**
     * text
     */
    QString m_text;
};

KateUndo::KateUndo (KateUndoGroup::UndoType type, uint line, uint col, uint len, const QString &text)
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

bool KateUndo::isValid()
{
  if (m_type == KateUndoGroup::editInsertText || m_type == KateUndoGroup::editRemoveText)
    if (len() == 0)
      return false;

  return true;
}

bool KateUndo::merge(KateUndo* u)
{
  if (m_type != u->type())
    return false;

  if (m_type == KateUndoGroup::editInsertText
      && m_line == u->line()
      && (m_col + m_len) == u->col())
  {
    m_text += u->text();
    m_len += u->len();
    return true;
  }
  else if (m_type == KateUndoGroup::editRemoveText
      && m_line == u->line()
      && m_col == (u->col() + u->len()))
  {
    m_text.prepend(u->text());
    m_col = u->col();
    m_len += u->len();
    return true;
  }

  return false;
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
    doc->editUnWrapLine (m_line, (m_text == "1"), m_len);
  }
  else if (m_type == KateUndoGroup::editUnWrapLine)
  {
    doc->editWrapLine (m_line, m_col, (m_text == "1"));
  }
  else if (m_type == KateUndoGroup::editInsertLine)
  {
    doc->editRemoveLine (m_line);
  }
  else if (m_type == KateUndoGroup::editRemoveLine)
  {
    doc->editInsertLine (m_line, m_text);
  }
  else if (m_type == KateUndoGroup::editMarkLineAutoWrapped)
  {
    doc->editMarkLineAutoWrapped (m_line, m_col == 0);
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
    doc->editUnWrapLine (m_line, (m_text == "1"), m_len);
  }
  else if (m_type == KateUndoGroup::editWrapLine)
  {
    doc->editWrapLine (m_line, m_col, (m_text == "1"));
  }
  else if (m_type == KateUndoGroup::editRemoveLine)
  {
    doc->editRemoveLine (m_line);
  }
  else if (m_type == KateUndoGroup::editInsertLine)
  {
    doc->editInsertLine (m_line, m_text);
  }
  else if (m_type == KateUndoGroup::editMarkLineAutoWrapped)
  {
    doc->editMarkLineAutoWrapped (m_line, m_col == 1);
  }
}

KTextEditor::Cursor KateUndo::cursorBefore() const
{
  if (m_type == KateUndoGroup::editInsertLine || m_type == KateUndoGroup::editUnWrapLine)
    return KTextEditor::Cursor(m_line+1, m_col);
  else if (m_type == KateUndoGroup::editRemoveText)
    return KTextEditor::Cursor(m_line, m_col+m_len);

  return KTextEditor::Cursor(m_line, m_col);
}

KTextEditor::Cursor KateUndo::cursorAfter() const
{
  if (m_type == KateUndoGroup::editRemoveLine || m_type == KateUndoGroup::editWrapLine)
    return KTextEditor::Cursor(m_line+1, m_col);
  else if (m_type == KateUndoGroup::editInsertText)
    return KTextEditor::Cursor(m_line, m_col+m_len);

  return KTextEditor::Cursor(m_line, m_col);
}

KateUndoGroup::KateUndoGroup (KateDocument *doc)
: m_doc (doc),m_safePoint(false)
{
}

KateUndoGroup::~KateUndoGroup ()
{
  qDeleteAll (m_items);
}

void KateUndoGroup::undo ()
{
  if (m_items.isEmpty())
    return;

  m_doc->editStart (false);

  for (int i=m_items.size()-1; i >= 0; --i)
    m_items[i]->undo(m_doc);

  if (m_doc->activeView())
  {
    for (int z=0; z < m_items.size(); ++z)
      if (m_items[z]->type() != KateUndoGroup::editMarkLineAutoWrapped)
      {
        m_doc->activeKateView()->editSetCursor (m_items[z]->cursorBefore());
        break;
      }
  }

  m_doc->editEnd ();
}

void KateUndoGroup::redo ()
{
  if (m_items.isEmpty())
    return;

  m_doc->editStart (false);

  for (int i=0; i < m_items.size(); ++i)
    m_items[i]->redo(m_doc);

  if (m_doc->activeView())
  {
    for (int z=0; z < m_items.size(); ++z)
      if (m_items[z]->type() != KateUndoGroup::editMarkLineAutoWrapped)
      {
        m_doc->activeKateView()->editSetCursor (m_items[z]->cursorAfter());
        break;
      }
  }

  m_doc->editEnd ();
}

void KateUndoGroup::addItem (KateUndoGroup::UndoType type, uint line, uint col, uint len, const QString &text)
{
  addItem(new KateUndo(type, line, col, len, text));
}

void KateUndoGroup::addItem(KateUndo* u)
{
  if (!u->isValid())
    delete u;
  else if (!m_items.isEmpty() && m_items.last()->merge(u))
    delete u;
  else
    m_items.append(u);
}

bool KateUndoGroup::merge(KateUndoGroup* newGroup,bool complex)
{
  if (m_safePoint) return false;
  if (newGroup->isOnlyType(singleType()) || complex) {
    // Take all of its items first -> last
    KateUndo* u = newGroup->m_items.isEmpty() ? 0 : newGroup->m_items.takeFirst ();
    while (u) {
      addItem(u);
      u = newGroup->m_items.isEmpty() ? 0 : newGroup->m_items.takeFirst ();
    }
    if (newGroup->m_safePoint) safePoint();
    return true;
  }
  return false;
}

void KateUndoGroup::safePoint (bool safePoint) {
  m_safePoint=safePoint;
}

KateUndoGroup::UndoType KateUndoGroup::singleType()
{
  KateUndoGroup::UndoType ret = editInvalid;

  for (int i=0; i < m_items.size(); ++i) {
    if (ret == editInvalid)
      ret = m_items[i]->type();
    else if (ret != m_items[i]->type())
      return editInvalid;
  }

  return ret;
}

bool KateUndoGroup::isOnlyType(KateUndoGroup::UndoType type)
{
  if (type == editInvalid) return false;

  for (int i=0; i < m_items.size(); ++i)
    if (m_items[i]->type() != type)
      return false;

  return true;
}

// kate: space-indent on; indent-width 2; replace-tabs on;
