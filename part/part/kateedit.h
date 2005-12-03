/* This file is part of the KDE libraries
   Copyright (C) 2005 Hamish Rodda <rodda@kde.org>

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

#ifndef KATEEDIT_H
#define KATEEDIT_H

#include <ktexteditor/range.h>
#include <QStringList>
#include <QObject>

class KateDocument;

/**
 * Represents a single edit to a KateDocument.
 *
 * @author Hamish Rodda
 */
class KateEditInfo
{
  friend class KateEditHistory;

  public:
    enum EditSources {
      OpenFile,
      CloseFile,
      UserInput,
      CutCopyPaste,
      SearchReplace,
      AutomaticIndentation,
      CodeCompletion,
      ScriptAction,
      CommandLine,
      Plugin,
      ThirdParty
    };

    KateEditInfo(KateDocument* doc, int source, const KTextEditor::Range& oldRange, const QStringList& oldText, const KTextEditor::Range& newRange, const QStringList& newText);
    virtual ~KateEditInfo();

    KateDocument* document() const;

    /// Returns true if this edit is a pure insertion of text
    bool isInsert() const;
    /// Returns true if this edit is a pure removal of text
    bool isRemoval() const;
    /// Returns true if text is being inserted and removed
    bool isModification() const;

    /// Returns true if this edit is completed and should not be merged with another edit, should the opportunity arise.
    bool isEditBoundary() const;
    void setEditBoundary(bool boundary);

    /**
     * Returns how this edit was generated.
     * \sa EditSources
     */
    int editSource() const;

    bool merge(KateEditInfo* edit);

    const KTextEditor::Cursor& start() const { return m_oldRange.start(); }

    /**
     * Returns the range of text occupied by the edit region before the edit took place.
     */
    const KTextEditor::Range& oldRange() const;

    /**
     * Returns the text which occupied \p range before this edit took place.
     * \note \p range must start and end on the same line for all relevant text to be returned.
     * \sa oldText()
     */
    virtual QString oldTextString(const KTextEditor::Range& range) const;
    /**
     * Returns the text which occupied \p range before this edit took place.
     * \note \p range must start and end on the same line for all relevant text to be returned.
     * \sa oldText()
     */
    virtual QStringList oldText(const KTextEditor::Range& range) const;

    const QStringList& oldText() const;

    /**
     * Returns the range of text occupied by the edit region after the edit took place.
     */
    const KTextEditor::Range& newRange() const;

    /**
     * Returns the text which occupied \p range before this edit took place.
     * \note \p range must start and end on the same line for all relevant text to be returned.
     * \sa oldText()
     */
    virtual QString newTextString(const KTextEditor::Range& range) const;

    /**
     * Returns the text which occupied \p range before this edit took place.
     * \note \p range must start and end on the same line for all relevant text to be returned.
     * \sa oldText()
     */
    virtual QStringList newText(const KTextEditor::Range& range) const;

    const QStringList& newText() const;

    inline const KTextEditor::Cursor& translate() const { return m_translate; }

  private:
    void undo();
    void redo();

    KateDocument* m_doc;
    int m_editSource;
    bool m_editBoundary;
    KTextEditor::Range m_oldRange;
    QStringList m_oldText;
    KTextEditor::Range m_newRange;
    QStringList m_newText;
    KTextEditor::Cursor m_translate;
};

/**
 * Represents multiple edits to a KateDocument
 */
class KateEditInfoGroup
{
  public:
    KateEditInfoGroup();

    KateDocument* doc() const;

    void addEdit(KateEditInfo* edit);
    void removeEdit(int indexTo = -1);
    KateEditInfo* takeEdit(int indexTo = -1);

    const QList<KateEditInfo*>& edits() const;

    QList<KateEditInfo> summariseEdits(int from = -1, int to = -1, bool destructiveCollapse = false) const;

  private:
    QList<KateEditInfo*> m_edits;
};

/**
 * Manages edit history in a document.
 */
class KateEditHistory : public QObject
{
  Q_OBJECT

  public:
    KateEditHistory(KateDocument* doc);

    KateEditInfoGroup* undoBuffer() const { return m_undo; }
    KateEditInfoGroup* redoBuffer() const { return m_redo; }

    void doEdit(KateEditInfo* edit) { undoBuffer()->addEdit(edit); emit editDone(edit); }
    void undo();
    void redo();

  signals:
    void editDone(KateEditInfo* edit);
    void editUndone(KateEditInfo* edit);

  private:
    KateDocument* m_doc;
    KateEditInfoGroup* m_undo;
    KateEditInfoGroup* m_redo;
};

#endif
