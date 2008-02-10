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

#include <QtCore/QStringList>
#include <QtCore/QObject>
#include <QtCore/QMap>

#include <ktexteditor/range.h>

#include "katenamespace.h"

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
    KateEditInfo(KateDocument* doc, Kate::EditSource source, const KTextEditor::Range& oldRange, const QStringList& oldText, const KTextEditor::Range& newRange, const QStringList& newText);
    virtual ~KateEditInfo();

    KateDocument* document() const;

    /// Returns true if this edit is a pure insertion of text
    bool isInsert() const;
    /// Returns true if this edit is a pure removal of text
    bool isRemoval() const;
    /// Returns true if text is being inserted and removed
    bool isModification() const;

    /**
     * Indicate that this edit is completed and should not be merged with
     * another edit should the opportunity arise.
     *
     * \return true if this edit is completed
     */
    bool isEditBoundary() const;

    /**
     * Set the status of this edit.
     * \param boundary If the edit is complete, the value should be true,
     * otherwise it should be false
     */
    void setEditBoundary(bool boundary);

    /**
     * Returns how this edit was generated.
     * \sa Kate::EditSource
     */
    Kate::EditSource editSource() const;

    /// \internal
    /// No real implementation
    bool merge(KateEditInfo* edit) { return false; }

    /**
     * Returns the starting location of the text occupied by the edit region
     * before the edit took place.
     * \return a KTextEditor::Cursor indicating the start location of the edit
     */
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
     * \sa oldText()
     */
    virtual QStringList oldText(const KTextEditor::Range& range) const;

    /**
     * Returns all of the text that was in place before the edit occurred.
     * \sa oldText(const KTextEditor::Range&) const
     * \sa oldTextString(const KTextEditor::Range&) const
     */ 
    const QStringList& oldText() const;

    /**
     * Returns the range of text occupied by the edit region after the edit took place.
     */
    const KTextEditor::Range& newRange() const;

    /**
     * Returns the text which occupies \p range after this edit took place.
     *
     * \p range must start and end on the same line for all relevant text to be returned.
     * \sa newText()
     * \sa newText(const KTextEditor::Range&) const
     */
    virtual QString newTextString(const KTextEditor::Range& range) const;

    /**
     * Returns the text which occupies \p range after this edit took place.
     * \sa newText()
     * \sa newTextString(const KTextEditor::Range&) const
     */
    virtual QStringList newText(const KTextEditor::Range& range) const;

    /**
     * Returns the text which occupies the edit region now that the edit
     * has taken place.
     * \sa newText(const KTextEditor::Range&) const
     * \sa newTextString(const KTextEditor::Range&) const
     */
    const QStringList& newText() const;

    inline const KTextEditor::Cursor& translate() const { return m_translate; }

    void referenceRevision();
    void dereferenceRevision();
    bool isReferenced() const;

  private:
    void undo();
    void redo();

    KateDocument* m_doc;
    Kate::EditSource m_editSource;
    bool m_editBoundary;
    KTextEditor::Range m_oldRange;
    QStringList m_oldText;
    KTextEditor::Range m_newRange;
    QStringList m_newText;
    KTextEditor::Cursor m_translate;
    int m_revisionTokenCounter;
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

    inline const QList<KateEditInfo*>& edits() const { return m_edits; }

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
    explicit KateEditHistory(KateDocument* doc);
    virtual ~KateEditHistory();

    inline KateEditInfoGroup* buffer() const { return m_buffer; }
    //KateEditInfoGroup* redoBuffer() const { return m_redo; }

    int revision();
    void releaseRevision(int revision);

    QList<KateEditInfo*> editsBetweenRevisions(int from, int to = -1) const;

    void doEdit(KateEditInfo* edit) { buffer()->addEdit(edit); emit editDone(edit); }
    //void undo();
    //void redo();

  Q_SIGNALS:
    void editDone(KateEditInfo* edit);
    //void editUndone(KateEditInfo* edit);

  private:
    KateDocument* m_doc;
    KateEditInfoGroup* m_buffer;
    //KateEditInfoGroup* m_redo;

    QMap<int, KateEditInfo*> m_revisions;
    int m_revision;
};

#endif
