/* This file is part of the KDE libraries
   Copyright (C) 2002,2003 Hamish Rodda <rodda@kde.org>
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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef KATELINERANGE_H
#define KATELINERANGE_H

#include "katecursor.h"

class LineRange
{
  public:
    LineRange();

    void clear();
    void debugOutput() const;
    bool includesCursor(const KateTextCursor& realCursor) const;

    friend bool operator> (const LineRange& r, const KateTextCursor& c);
    friend bool operator>= (const LineRange& r, const KateTextCursor& c);
    friend bool operator< (const LineRange& r, const KateTextCursor& c);
    friend bool operator<= (const LineRange& r, const KateTextCursor& c);

    int line;
    int virtualLine;
    int startCol;
    int endCol;
    int startX;
    int endX;

    // This variable is used as follows:
    // non-dynamic-wrapping mode: unused
    // dynamic wrapping mode:
    //   first viewLine of a line: the X position of the first non-whitespace char
    //   subsequent viewLines: the X offset from the left of the display.
    //
    // this is used to provide a dynamic-wrapping-retains-indent feature.
    int shiftX;
    int xOffset() const;

    bool dirty;
    int viewLine;
    bool wrap;
    bool startsInvisibleBlock;
};

#endif
