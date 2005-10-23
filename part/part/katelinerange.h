/* This file is part of the KDE libraries
   Copyright (C) 2002-2005 Hamish Rodda <rodda@kde.org>
   Copyright (C) 2003      Anakim Border <aborder@sources.sourceforge.net>

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

#ifndef _KATE_LINERANGE_H_
#define _KATE_LINERANGE_H_

#include "katecursor.h"
#include "katetextline.h"
#include "katetextlayout.h"

class QTextLayout;
class KateDocument;

class KateLineLayout : public KShared
{
  public:
    KateLineLayout(KateDocument* doc = 0L);
    ~KateLineLayout();

    KateDocument* doc() const;
    void debugOutput() const;

    void clear();
    bool isValid() const;
    bool isOutsideDocument() const;

    bool includesCursor(const KTextEditor::Cursor& realCursor) const;

    friend bool operator> (const KateLineLayout& r, const KTextEditor::Cursor& c);
    friend bool operator>= (const KateLineLayout& r, const KTextEditor::Cursor& c);
    friend bool operator< (const KateLineLayout& r, const KTextEditor::Cursor& c);
    friend bool operator<= (const KateLineLayout& r, const KTextEditor::Cursor& c);

    const KateTextLine::Ptr& textLine() const;
    int length() const;

    int line() const;
    /**
     * Only pass virtualLine if you know it (and thus we shouldn't try to look it up)
     */
    void setLine(int line, int virtualLine = -1);
    KTextEditor::Cursor start() const;

    int virtualLine() const;
    void setVirtualLine(int virtualLine);

    bool isDirty(int viewLine) const;
    bool setDirty(int viewLine, bool dirty = true);

    int width() const;

    int viewLineCount() const;
    KateTextLayout viewLine(int viewLine) const;
    int viewLineForColumn(int column) const;

    bool startsInvisibleBlock() const;

    // This variable is used as follows:
    // non-dynamic-wrapping mode: unused
    // dynamic wrapping mode:
    //   first viewLine of a line: the X position of the first non-whitespace char
    //   subsequent viewLines: the X offset from the left of the display.
    //
    // this is used to provide a dynamic-wrapping-retains-indent feature.
    int shiftX() const;
    void setShiftX(int shiftX);

    QTextLayout* layout() const;
    void setLayout(QTextLayout* layout);
    void invalidateLayout();

private:
    // Disable copy
    KateLineLayout(const KateLineLayout& copy);

    QTextLayout* takeLayout() const;

    KateDocument* m_doc;
    mutable KateTextLine::Ptr m_textLine;
    int m_line;
    int m_virtualLine;
    int m_shiftX;

    QTextLayout* m_layout;
    QList<bool> m_dirtyList;
};

typedef KSharedPtr<KateLineLayout> KateLineLayoutPtr;

#endif
