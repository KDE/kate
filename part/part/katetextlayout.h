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

#ifndef _KATE_TEXTLAYOUT_H_
#define _KATE_TEXTLAYOUT_H_

#include <QTextLine>

#include <ksharedptr.h>

#include "katecursor.h"

class QTextLayout;
class KateDocument;

class KateLineLayout;
typedef KSharedPtr<KateLineLayout> KateLineLayoutPtr;

class KateTextLayout
{
  friend class KateLineLayout;
  friend class KateLayoutCache;
  template <class T> friend class QVector;

  public:
    bool isValid() const;
    static KateTextLayout invalid();

    int line() const;
    int virtualLine() const;
    int viewLine() const;

    const QTextLine& lineLayout() const;
    KateLineLayoutPtr kateLineLayout() const;

    int startCol() const;
    KTextEditor::Cursor start() const;

    int endCol() const;
    KTextEditor::Cursor end() const;

    int length() const;
    bool isEmpty() const;

    bool wrap() const;

    bool isDirty() const;
    bool setDirty(bool dirty = true);

    int startX() const;
    int endX() const;
    int width() const;

    int xOffset() const;

    bool includesCursor(const KTextEditor::Cursor& realCursor) const;

    friend bool operator> (const KateLineLayout& r, const KTextEditor::Cursor& c);
    friend bool operator>= (const KateLineLayout& r, const KTextEditor::Cursor& c);
    friend bool operator< (const KateLineLayout& r, const KTextEditor::Cursor& c);
    friend bool operator<= (const KateLineLayout& r, const KTextEditor::Cursor& c);

    void debugOutput() const;

  private:
    explicit KateTextLayout(KateLineLayoutPtr line = KateLineLayoutPtr(), int viewLine = 0);

    KateLineLayoutPtr m_lineLayout;
    QTextLine m_textLayout;

    int m_viewLine;
    mutable int m_startX;
    bool m_invalidDirty;
};

#endif
