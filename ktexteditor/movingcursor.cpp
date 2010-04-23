/* This file is part of the KDE project
 *
 *  Copyright (C) 2010 Christoph Cullmann <cullmann@kde.org>
 *  Copyright (C) 2010 Dominik Haumann <dhaumann kde org>
 *
 *  Based on code of the SmartCursor/Range by:
 *  Copyright (C) 2003-2005 Hamish Rodda <rodda@kde.org>
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

#include "movingcursor.h"
#include "document.h"

using namespace KTextEditor;

MovingCursor::MovingCursor ()
{
}

MovingCursor::~MovingCursor ()
{
}

void MovingCursor::setPosition(int line, int column)
{
  // just use setPosition
  setPosition(Cursor(line, column));
}

void MovingCursor::setLine (int line)
{
  // just use setPosition
  setPosition (line, column());
}

void MovingCursor::setColumn (int column)
{
  // just use setPosition
  setPosition (line(), column);
}

bool MovingCursor::atStartOfLine() const {
  return isValidTextPosition() && column() == 0;
}

bool MovingCursor::atEndOfLine() const {
  return isValidTextPosition() && column() == document()->lineLength(line());
}

bool MovingCursor::atEndOfDocument() const {
  return *this == document()->documentEnd();
}

bool MovingCursor::atStartOfDocument() const {
  return line() == 0 && column() == 0;
}

bool MovingCursor::gotoNextLine()
{
  // only touch valid cursors
  const bool ok = isValid() && (line() + 1 < document()->lines());

  if (ok) {
    setPosition(Cursor(line() + 1, 0));
  }

  return ok;
}

bool MovingCursor::gotoPreviousLine()
{
  // only touch valid cursors
  bool ok = (line() > 0) && (column() >= 0);

  if (ok) {
    setPosition(Cursor(line() - 1, 0));
  }

  return ok;
}

bool MovingCursor::move(int chars, WrapBehavior wrapBehavior)
{
  if (!isValid()) {
    return false;
  }

  Cursor c(toCursor());

  // special case: cursor position is not in valid text, then the algo does
  // not work for Wrap mode. Hence, catch this special case by setting
  // c.column() to the lineLength()
  if (chars > 0 && wrapBehavior == Wrap && c.column() > document()->lineLength(c.line())) {
    c.setColumn(document()->lineLength(c.line()));
  }

  while (chars != 0) {
    if (chars > 0) {
      if (wrapBehavior == Wrap) {
        int advance = qMin(document()->lineLength(c.line()) - c.column(), chars);

        if (chars > advance) {
          if (c.line() + 1 >= document()->lines()) {
            return false;
          }

          c.setPosition(c.line() + 1, 0);
          chars -= advance + 1; // +1 because of end-of-line wrap
        } else {
          c.setColumn(c.column() + chars);
          chars = 0;
        }
      } else { // NoWrap
        c.setColumn(c.column() + chars);
        chars = 0;
      }
    } else {
      int back = qMin(c.column(), -chars);
      if (-chars > back) {
        if (c.line() == 0)
          return false;

        c.setPosition(c.line() - 1, document()->lineLength(c.line() - 1));
        chars += back + 1; // +1 because of wrap-around at start-of-line
      } else {
        c.setColumn(c.column() + chars);
        chars = 0;
      }
    }
  }

  if (c != *this) {
    setPosition(c);
  }
  return true;
}

// kate: space-indent on; indent-width 2; replace-tabs on;
