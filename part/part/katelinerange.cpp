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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "katelinerange.h"

KateLineRange::KateLineRange()
  : line(-1)
  , virtualLine(-1)
  , startCol(-1)
  , endCol(-1)
  , startX(-1)
  , endX(-1)
  , dirty(false)
  , viewLine(-1)
  , wrap(false)
  , startsInvisibleBlock(false)
  , shiftX(0)
{
}

KateLineRange::~KateLineRange ()
{
}

void KateLineRange::clear()
{
  line = -1;
  virtualLine = -1;
  startCol = -1;
  endCol = -1;
  startX = -1;
  shiftX = 0;
  endX = -1;
  viewLine = -1;
  wrap = false;
  startsInvisibleBlock = false;
}

bool operator> (const KateLineRange& r, const KateTextCursor& c)
{
  return r.line > c.line() || r.endCol > c.col();
}

bool operator>= (const KateLineRange& r, const KateTextCursor& c)
{
  return r.line > c.line() || r.endCol >= c.col();
}

bool operator< (const KateLineRange& r, const KateTextCursor& c)
{
  return r.line < c.line() || r.startCol < c.col();
}

bool operator<= (const KateLineRange& r, const KateTextCursor& c)
{
  return r.line < c.line() || r.startCol <= c.col();
}

// kate: space-indent on; indent-width 2; replace-tabs on;
