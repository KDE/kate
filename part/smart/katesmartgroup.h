/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2005 Hamish Rodda <rodda@kde.org>
 *  Copyright (C) 2010 Christoph Cullmann <cullmann@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#ifndef KATESMARTGROUP_H
#define KATESMARTGROUP_H

#include <QtCore/QSet>

#include <ktexteditor/smartrange.h>

class KateEditInfo;
class KateDocument;
class KateSmartCursor;
class KateSmartRange;
class KateSmartGroup;

/**
 * This class holds a ground of cursors and ranges which involve a certain
 * number of lines in a document.  It allows us to optimize away having to
 * iterate though every single cursor and range when anything changes in the
 * document.
 *
 * Needs a comprehensive regression and performance test suite...
 */
class KateSmartGroup
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

    // Second pass for finalizing the translation
    void translatedChanged(const KateEditInfo& edit);
    void translatedShifted(const KateEditInfo& edit);

    // Third pass, when all translations are complete
    void translatedChanged2(const KateEditInfo& edit);
    
    void deleteCursors(bool includingInternal);
    void deleteCursorsInternal(QSet<KateSmartCursor*>& set);

    void debugOutput() const;

  private:
    int m_startLine, m_newStartLine, m_endLine, m_newEndLine;
    KateSmartGroup* m_next;
    KateSmartGroup* m_previous;

    QSet<KateSmartCursor*> m_feedbackCursors;
    QSet<KateSmartCursor*> m_normalCursors;
};

#endif
