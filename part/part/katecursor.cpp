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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "katecursor.h"

#include "katedocument.h"
#include "katetextline.h"

//
// KateDocCursor implementation
//

KateDocCursor::KateDocCursor(KateDocument *doc) : KateTextCursor(), m_doc(doc)
{
}

KateDocCursor::KateDocCursor(int line, int col, KateDocument *doc)
  : KateTextCursor(line, col), m_doc(doc)
{
}

bool KateDocCursor::validPosition(uint line, uint col)
{
  return line < m_doc->numLines() && (int)col <= m_doc->lineLength(line);
}

bool KateDocCursor::validPosition()
{
  return validPosition(line(), col());
}

void KateDocCursor::position(uint *pline, uint *pcol) const
{
  if (pline)
    *pline = (uint)line();

  if (pcol)
    *pcol = (uint)col();
}

bool KateDocCursor::setPosition(uint line, uint col)
{
  bool ok = validPosition(line, col);

  if(ok)
    setPos(line, col);

  return ok;
}

bool KateDocCursor::gotoNextLine()
{
  bool ok = (line() + 1 < (int)m_doc->numLines());

  if (ok) {
    m_line++;
    m_col = 0;
  }

  return ok;
}

bool KateDocCursor::gotoPreviousLine()
{
  bool ok = (line() > 0);

  if (ok) {
    m_line--;
    m_col = 0;
  }

  return ok;
}

bool KateDocCursor::gotoEndOfNextLine()
{
  bool ok = gotoNextLine();
  if(ok)
    m_col = m_doc->lineLength(line());

  return ok;
}

bool KateDocCursor::gotoEndOfPreviousLine()
{
  bool ok = gotoPreviousLine();
  if(ok)
    m_col = m_doc->lineLength(line());

  return ok;
}

int KateDocCursor::nbCharsOnLineAfter()
{
  return ((int)m_doc->lineLength(line()) - col());
}

bool KateDocCursor::moveForward(uint nbChar)
{
  int nbCharLeft = nbChar - nbCharsOnLineAfter();

  if(nbCharLeft > 0) {
    return gotoNextLine() && moveForward((uint)nbCharLeft);
  } else {
    m_col += nbChar;
    return true;
  }
}

bool KateDocCursor::moveBackward(uint nbChar)
{
  int nbCharLeft = nbChar - m_col;
  if(nbCharLeft > 0) {
    return gotoEndOfPreviousLine() && moveBackward((uint)nbCharLeft);
  } else {
    m_col -= nbChar;
    return true;
  }
}

bool KateDocCursor::insertText(const QString& s)
{
  return m_doc->insertText(line(), col(), s);
}

bool KateDocCursor::removeText(uint nbChar)
{
  // Get a cursor at the end of the removed area
  KateDocCursor endCursor = *this;
  endCursor.moveForward(nbChar);

  // Remove the text
  return m_doc->removeText((uint)line(), (uint)col(),
         (uint)endCursor.line(), (uint)endCursor.col());
}

QChar KateDocCursor::currentChar() const
{
  return m_doc->plainKateTextLine(line())->getChar(col());
}

uchar KateDocCursor::currentAttrib() const
{
  return m_doc->plainKateTextLine(line())->attribute(col());
}

bool KateDocCursor::nextNonSpaceChar()
{
  for(; m_line < (int)m_doc->numLines(); m_line++) {
    m_col = m_doc->plainKateTextLine(line())->nextNonSpaceChar(col());
    if(m_col != -1)
      return true; // Next non-space char found
    m_col = 0;
  }
  // No non-space char found
  setPos(-1, -1);
  return false;
}

bool KateDocCursor::previousNonSpaceChar()
{
  while (true) {
    m_col = m_doc->plainKateTextLine(line())->previousNonSpaceChar(col());
    if(m_col != -1) return true; // Previous non-space char found
    if(m_line == 0) return false;
    --m_line;
    m_col = m_doc->plainKateTextLine(m_line)->length();
  }
  // No non-space char found
  setPos(-1, -1);
  return false;
}

// kate: space-indent on; indent-width 2; replace-tabs on;
