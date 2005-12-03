/* This file is part of the KDE libraries
   Copyright (C) 2002 Christian Couder <christian@kdevelop.org>
   Copyright (C) 2001, 2003 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>

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

#ifndef kate_range_h
#define kate_range_h

#include <ktexteditor/range.h>

class KateBracketRange : public KTextEditor::Range
{
  public:
    KateBracketRange()
      : KTextEditor::Range()
      , m_minIndent(0)
    {
    };

    KateBracketRange(int startline, int startcol, int endline, int endcol, int minIndent)
      : KTextEditor::Range(startline, startcol, endline, endcol)
      , m_minIndent(minIndent)
    {
    };

    KateBracketRange(const KTextEditor::Cursor& start, const KTextEditor::Cursor& end, int minIndent)
      : KTextEditor::Range(start, end)
      , m_minIndent(minIndent)
    {
    };

    int getMinIndent() const
    {
      return m_minIndent;
    }

    void setIndentMin(int m)
    {
      m_minIndent = m;
    }

  protected:
    int m_minIndent;
};


#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
