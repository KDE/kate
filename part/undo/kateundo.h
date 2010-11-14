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

#ifndef kate_undo_h
#define kate_undo_h

#include <QtCore/QList>

#include <ktexteditor/range.h>

class KateUndoManager;
class KateDocument;

namespace KTextEditor {
  class View;
}

/**
 * Base class for Kate undo commands.
 */
class KateUndo
{
  public:
    /**
     * Constructor
     * @param document the document the undo item belongs to
     */
    KateUndo (KateDocument *document);

    /**
     * Destructor
     */
    virtual ~KateUndo();

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

  public:
    /**
     * Check whether the item is empty.
     *
     * @return whether the item is empty
     */
    virtual bool isEmpty() const;

    /**
     * merge an undo item
     * Saves a bit of memory and potentially many calls when undo/redoing.
     * @param undo undo item to merge
     * @return success
     */
    virtual bool mergeWith(const KateUndo* undo);

    /**
     * undo this item
     */
    virtual void undo() = 0;

    /**
     * redo this item
     */
    virtual void redo() = 0;

    /**
     * type of item
     * @return type
     */
    virtual KateUndo::UndoType type() const = 0;

  protected:
    /**
     * Return the document the undo item belongs to.
     * @return the document the undo item belongs to
     */
    inline KateDocument *document() { return m_document; }

  private:
    /**
     * the document the undo item belongs to
     */
    KateDocument *m_document;
};

class KateEditInsertTextUndo : public KateUndo
{
  public:
    KateEditInsertTextUndo (KateDocument *document, int line, int col, const QString &text)
      : KateUndo (document)
      , m_line (line)
      , m_col (col)
      , m_text (text)
    {}

    /**
     * @copydoc KateUndo::isEmpty()
     */
    bool isEmpty() const;

    /**
     * @copydoc KateUndo::undo()
     */
    void undo();

    /**
     * @copydoc KateUndo::redo()
     */
    void redo();

    /**
     * @copydoc KateUndo::mergeWith(const KateUndo)
     */
    bool mergeWith (const KateUndo *undo);

    /**
     * @copydoc KateUndo::type()
     */
    KateUndo::UndoType type() const { return KateUndo::editInsertText; }

  private:
    int len() const { return m_text.length(); }

  private:
    const int m_line;
    const int m_col;
    QString m_text;
};

class KateEditRemoveTextUndo : public KateUndo
{
  public:
    KateEditRemoveTextUndo (KateDocument *document, int line, int col, const QString &text)
      : KateUndo (document)
      , m_line (line)
      , m_col (col)
      , m_text (text)
    {}

    /**
     * @copydoc KateUndo::isEmpty()
     */
    bool isEmpty() const;

    /**
     * @copydoc KateUndo::undo()
     */
    void undo();

    /**
     * @copydoc KateUndo::redo()
     */
    void redo();

    /**
     * @copydoc KateUndo::mergeWith(const KateUndo)
     */
    bool mergeWith (const KateUndo *undo);

    /**
     * @copydoc KateUndo::type()
     */
    KateUndo::UndoType type() const { return KateUndo::editRemoveText; }

  private:
    int len() const { return m_text.length(); }

  private:
    const int m_line;
    int m_col;
    QString m_text;
};

class KateEditMarkLineAutoWrappedUndo : public KateUndo
{
  public:
    KateEditMarkLineAutoWrappedUndo (KateDocument *document, int line, bool autowrapped)
      : KateUndo (document)
      , m_line (line)
      , m_autowrapped (autowrapped)
    {}

    /**
     * @copydoc KateUndo::undo()
     */
    void undo();

    /**
     * @copydoc KateUndo::redo()
     */
    void redo();

    /**
     * @copydoc KateUndo::type()
     */
    KateUndo::UndoType type() const { return KateUndo::editMarkLineAutoWrapped; }

  private:
    const int m_line;
    const bool m_autowrapped;
};

class KateEditWrapLineUndo : public KateUndo
{
  public:
    KateEditWrapLineUndo (KateDocument *document, int line, int col, int len, bool newLine)
      : KateUndo (document)
      , m_line (line)
      , m_col (col)
      , m_len (len)
      , m_newLine (newLine)
    {}

    /**
     * @copydoc KateUndo::undo()
     */
    void undo();

    /**
     * @copydoc KateUndo::redo()
     */
    void redo();

    /**
     * @copydoc KateUndo::type()
     */
    KateUndo::UndoType type() const { return KateUndo::editWrapLine; }

  private:
    const int m_line;
    const int m_col;
    const int m_len;
    const bool m_newLine;
};

class KateEditUnWrapLineUndo : public KateUndo
{
  public:
    KateEditUnWrapLineUndo (KateDocument *document, int line, int col, int len, bool removeLine)
      : KateUndo (document)
      , m_line (line)
      , m_col (col)
      , m_len (len)
      , m_removeLine (removeLine)
    {}

    /**
     * @copydoc KateUndo::undo()
     */
    void undo();

    /**
     * @copydoc KateUndo::redo()
     */
    void redo();

    /**
     * @copydoc KateUndo::type()
     */
    KateUndo::UndoType type() const { return KateUndo::editUnWrapLine; }

  private:
    const int m_line;
    const int m_col;
    const int m_len;
    const bool m_removeLine;
};

class KateEditInsertLineUndo : public KateUndo
{
  public:
    KateEditInsertLineUndo (KateDocument *document, int line, const QString &text)
      : KateUndo (document)
      , m_line (line)
      , m_text (text)
    {}

    /**
     * @copydoc KateUndo::undo()
     */
    void undo();

    /**
     * @copydoc KateUndo::redo()
     */
    void redo();

    /**
     * @copydoc KateUndo::type()
     */
    KateUndo::UndoType type() const { return KateUndo::editInsertLine; }

  private:
    const int m_line;
    const QString m_text;
};

class KateEditRemoveLineUndo : public KateUndo
{
  public:
    KateEditRemoveLineUndo (KateDocument *document, int line, const QString &text)
      : KateUndo (document)
      , m_line (line)
      , m_text (text)
    {}

    /**
     * @copydoc KateUndo::undo()
     */
    void undo();

    /**
     * @copydoc KateUndo::redo()
     */
    void redo();

    /**
     * @copydoc KateUndo::type()
     */
    KateUndo::UndoType type() const { return KateUndo::editRemoveLine; }

  private:
    const int m_line;
    const QString m_text;
};

/**
 * Class to manage a group of undo items
 */
class KateUndoGroup
{
  public:
    /**
     * Constructor
     * @param manager KateUndoManager this undo group will belong to
     */
    explicit KateUndoGroup (KateUndoManager *manager, const KTextEditor::Cursor &cursorPosition, const KTextEditor::Range &selectionRange);

    /**
     * Destructor
     */
    ~KateUndoGroup();

  public:
    /**
     * Undo the contained undo items
     */
    void undo(KTextEditor::View *view);

    /**
     * Redo the contained undo items
     */
    void redo(KTextEditor::View *view);

    void editEnd(const KTextEditor::Cursor &cursorPosition, const KTextEditor::Range selectionRange);

    /**
     * merge this group with an other
     * @param newGroup group to merge into this one
     * @param complex set if a complex undo
     * @return success
     */
    bool merge (KateUndoGroup* newGroup,bool complex);

    /**
     * set group as as savepoint. the next group will not merge with this one
     */
    void safePoint (bool safePoint=true);

    /**
     * is this undogroup empty?
     */
    bool isEmpty() const { return m_items.isEmpty(); }

  private:
    KTextEditor::Document *document();

    /**
     * singleType
     * @return the type if it's only one type, or editInvalid if it contains multiple types.
     */
    KateUndo::UndoType singleType() const;

    /**
     * are we only of this type ?
     * @param type type to query
     * @return we contain only the given type
     */
    bool isOnlyType(KateUndo::UndoType type) const;

  public:
    /**
     * add an undo item
     * @param u item to add
     */
    void addItem (KateUndo *u);

  private:
    KateUndoManager *const m_manager;

    /**
     * list of items contained
     */
    QList<KateUndo*> m_items;

    /**
     * prohibit merging with the next group
     */
    bool m_safePoint;

    /**
     * the text selection of the active view before the edit step
     */
    const KTextEditor::Range m_undoSelection;

    /**
     * the text selection of the active view after the edit step
     */
    KTextEditor::Range m_redoSelection;

    /**
     * the cursor position of the active view before the edit step
     */
    const KTextEditor::Cursor m_undoCursor;

    /**
     * the cursor position of the active view after the edit step
     */
    KTextEditor::Cursor m_redoCursor;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
