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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef KATESMARTMANAGER_H
#define KATESMARTMANAGER_H

#include <QObject>
#include <QSet>
#include <QLinkedList>

#include <ktexteditor/smartrange.h>

#include "kateedit.h"

#ifndef NDEBUG
#define KATE_TESTONLY_EXPORT KDE_EXPORT
#elif
#define KATE_TESTONLY_EXPORT
#endif

class KateDocument;
class KateSmartCursor;
class KateSmartRange;
class KateSmartGroup;

/**
 * Manages SmartCursors and SmartRanges.
 */
class KATE_TESTONLY_EXPORT KateSmartManager : public QObject
{
  Q_OBJECT

  public:
    KateSmartManager(KateDocument* parent);
    virtual ~KateSmartManager();

    KateDocument* doc() const;

    KTextEditor::SmartCursor* newSmartCursor(const KTextEditor::Cursor& position, bool moveOnInsert = true);
    KTextEditor::SmartRange* newSmartRange(const KTextEditor::Range& range, KTextEditor::SmartRange* parent = 0L, KTextEditor::SmartRange::InsertBehaviours insertBehaviour = KTextEditor::SmartRange::DoNotExpand);
    KTextEditor::SmartRange* newSmartRange(KateSmartCursor* start, KateSmartCursor* end, KTextEditor::SmartRange* parent = 0L, KTextEditor::SmartRange::InsertBehaviours insertBehaviour = KTextEditor::SmartRange::DoNotExpand);

    void rangeGotParent(KateSmartRange* range);
    void rangeLostParent(KateSmartRange* range);

    KateSmartGroup* groupForLine(int line) const;

  private slots:
    void slotTextChanged(KateEditInfo* edit);
    void verifyCorrect() const;

  private:
    KateSmartRange* feedbackRange(const KateEditInfo& edit, KateSmartRange* range);

    const QSet<KateSmartRange*>& rangesWantingMostSpecificContentFeedback() const;

    void debugOutput() const;

    KateSmartGroup* m_firstGroup;
    QSet<KateSmartRange*> m_topRanges;
    KateSmartGroup* m_invalidGroup;
};

/**
 * This class holds a ground of cursors and ranges which involve a certain
 * number of lines in a document.  It allows us to optimise away having to
 * iterate though every single cursor and range when anything changes in the
 * document.
 *
 * Needs a comprehensive regression and performance test suite...
 */
class KATE_TESTONLY_EXPORT KateSmartGroup
{
  public:
    KateSmartGroup(int startLine, int endLine, KateSmartGroup* previous, KateSmartGroup* next);

    inline int startLine() const { return m_startLine; }
    inline int newStartLine() const { return m_newStartLine; }
    inline int endLine() const { return m_endLine; }
    inline int newEndLine() const { return m_newEndLine; }
    inline void setEndLine(int endLine) { m_newEndLine = m_endLine = endLine; }
    inline void setNewEndLine(int endLine) { m_newEndLine = endLine; }
    inline int length() const { return m_endLine - m_startLine + 1; }
    inline bool containsLine(int line) const { return line >= m_newStartLine && line <= m_newEndLine; }

    inline KateSmartGroup* previous() const { return m_previous; }
    inline void setPrevious(KateSmartGroup* previous) { m_previous = previous; }

    inline KateSmartGroup* next() const { return m_next; }
    inline void setNext(KateSmartGroup* next) { m_next = next; }

    void addCursor(KateSmartCursor* cursor);
    void changeCursorFeedback(KateSmartCursor* cursor);
    void removeCursor(KateSmartCursor* cursor);
    // Cursors requiring position change feedback
    const QSet<KateSmartCursor*>& feedbackCursors() const;
    // Cursors not requiring feedback
    const QSet<KateSmartCursor*>& normalCursors() const;

    // Cursor movement!
    /**
     * A cursor has joined this group.
     *
     * The cursor already has its new position.
     */
    void joined(KateSmartCursor* cursor);

    /**
     * A cursor is leaving this group.
     *
     * The cursor still has its old position.
     */
    void leaving(KateSmartCursor* cursor);

    // Merge with the next Smart Group, because this group became too small.
    void merge();

    // First pass for translation
    void translateChanged(const KateEditInfo& edit);
    void translateShifted(const KateEditInfo& edit);

    // Second pass for feedback
    void translatedChanged(const KateEditInfo& edit);
    void translatedShifted(const KateEditInfo& edit);

    void debugOutput() const;

  private:
    int m_startLine, m_newStartLine, m_endLine, m_newEndLine;
    KateSmartGroup* m_next;
    KateSmartGroup* m_previous;

    QSet<KateSmartCursor*> m_feedbackCursors;
    QSet<KateSmartCursor*> m_normalCursors;
};

#endif
