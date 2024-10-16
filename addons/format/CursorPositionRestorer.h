/*
    SPDX-FileCopyrightText: 2023 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include <KTextEditor/Document>
#include <KTextEditor/View>
#include <QPointer>

class CursorPositionRestorer
{
public:
    CursorPositionRestorer(KTextEditor::Document *doc)
        : m_doc(doc)
    {
        if (doc) {
            const auto views = m_doc->views();
            m_viewToPosition.reserve(views.size());
            for (auto v : views) {
                const auto off = cursorToSpaceIgnoredOffset(doc, v->cursorPosition());
                m_viewToPosition.push_back({v, Position{v->cursorPosition(), off}});
            }
        }
    }

    void restore()
    {
        if (!m_doc) {
            return;
        }
        for (auto [view, position] : m_viewToPosition) {
            if (view) {
                // Convert the offset to cursor and restore it
                const auto pos = spaceIgnoredOffsetToCursor(m_doc, position.spaceIgnoredOffset);
                if (pos.isValid()) {
                    view->setCursorPosition(pos);
                } else if (position.cursor.isValid()) {
                    view->setCursorPosition(position.cursor);
                }
            }
        }
    }

private:
    static int countNonSpace(const QString &l)
    {
        int count = 0;
        for (auto c : l) {
            count += !c.isSpace();
        }
        return count;
    }

    // Get the space ignored offset for @p c
    static int cursorToSpaceIgnoredOffset(const KTextEditor::Document *const doc, KTextEditor::Cursor c)
    {
        if (!c.isValid()) {
            return -1;
        }

        int o = 0;
        for (int i = 0; i < c.line(); ++i) {
            o += countNonSpace(doc->line(i));
        }
        const QString ln = doc->line(c.line());
        int count = 0;
        for (int i = 0; i < c.column() && i < ln.size(); ++i) {
            count += !ln[i].isSpace();
        }
        o += count;
        return o;
    }

    // Convert the offset into a valid cursor position
    static KTextEditor::Cursor spaceIgnoredOffsetToCursor(const KTextEditor::Document *const doc, int off)
    {
        if (off == -1) {
            return KTextEditor::Cursor::invalid();
        }

        const int lines = doc->lines();
        int line = -1;
        int newOff = 0;
        for (int i = 0; i < lines; ++i) {
            const QString ln = doc->line(i);
            const int len = countNonSpace(ln);
            if (newOff + len >= off) {
                line = i;
                break;
            }
            newOff += len;
        }
        if (line >= 0) {
            int nsCount = 0;
            int col = 0;
            const QString ln = doc->line(line);
            for (auto ch : ln) {
                if (nsCount + newOff == off) {
                    break;
                }
                col++;
                nsCount += !ch.isSpace();
            }
            return KTextEditor::Cursor(line, col);
        }
        return KTextEditor::Cursor::invalid();
    }

    QPointer<KTextEditor::Document> m_doc;
    struct Position {
        KTextEditor::Cursor cursor;
        int spaceIgnoredOffset = 0;
    };
    std::vector<std::pair<KTextEditor::View *, Position>> m_viewToPosition;
};
