/* This file is part of the KDE libraries
   Copyright (C) 2002 Christian Couder <christian@kdevelop.org>
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/
      
// $Id$

#include "katecursor.h"
#include "katedocument.h"
#include "katetextline.h"

//
// KateDocCursor implementation
//

KateDocCursor::KateDocCursor(KateDocument *doc) : KateTextCursor(), m_doc(doc)
{
}

KateDocCursor::KateDocCursor(int _line, int _col, KateDocument *doc)
  : KateTextCursor(_line, _col), m_doc(doc)
{
}

KateDocCursor::~KateDocCursor()
{
}

bool KateDocCursor::validPosition(uint _line, uint _col)
{
  return _line < m_doc->numLines() && (int)_col <= m_doc->textLength(_line);
}

bool KateDocCursor::validPosition()
{
  return validPosition(line, col);
}

int KateDocCursor::nbCharsOnLineAfter()
{
  return ((int)m_doc->textLength(line) - (int)col);
}

void KateDocCursor::position(uint *pline, uint *pcol) const
{
  *pline = (uint) line;
  *pcol = (uint) col;
}

bool KateDocCursor::setPosition(uint _line, uint _col)
{
  bool ok = validPosition(_line, _col);

  if(ok)
    setPos(_line, _col);

  return ok;
}

bool KateDocCursor::gotoNextLine()
{
  bool ok = (line + 1 < (int)m_doc->numLines());
 
  if (ok) {
    line++;
    col = 0;
  }

  return ok;
}

bool KateDocCursor::gotoPreviousLine()
{
  bool ok = (line > 0);
 
  if (ok) {
    line--;
    col = 0;
  }

  return ok;
}

bool KateDocCursor::gotoEndOfNextLine()
{
  bool ok = gotoNextLine();
  if(ok)
    col = m_doc->textLength(line);

  return ok;
}

bool KateDocCursor::gotoEndOfPreviousLine()
{
  bool ok = gotoPreviousLine();
  if(ok)
    col = m_doc->textLength(line);

  return ok;
}

bool KateDocCursor::moveForward(uint nbChar)
{
  int nbCharLeft = nbChar - nbCharsOnLineAfter();

  if(nbCharLeft > 0) {
    return gotoNextLine() && moveForward((uint)nbCharLeft);
  } else {
    col += nbChar;
    return true;
  }
}

bool KateDocCursor::moveBackward(uint nbChar)
{
  int nbCharLeft = nbChar - col;
  if(nbCharLeft > 0) {
    return gotoEndOfPreviousLine() && moveBackward((uint)nbCharLeft);
  } else {
    col -= nbChar;
    return true;
  }
}

bool KateDocCursor::insertText(const QString& s)
{
  return m_doc->insertText(line, col, s);
}

bool KateDocCursor::removeText(uint nbChar)
{
  // Get a cursor at the end of the removed area
  KateDocCursor endCursor = *this;
  endCursor.moveForward(nbChar);

  // Remove the text
  return m_doc->removeText((uint)line, (uint)col, 
			   (uint)endCursor.line, (uint)endCursor.col);
}

QChar KateDocCursor::currentChar() const
{
  return m_doc->kateTextLine(line)->getChar(col);
}


/*
  Find the position (line and col) of the next char
  that is not a space. Return true if found.
*/
bool KateDocCursor::nextNonSpaceChar()
{
  for(; line < (int)m_doc->numLines(); line++) {
    col = m_doc->kateTextLine(line)->nextNonSpaceChar(col);
    if(col != -1)
      return true; // Next non-space char found 
    col = 0;
  }
  // No non-space char found
  setPos(-1, -1);
  return false;
}

/*
  Find the position (line and col) of the previous char
  that is not a space. Return true if found.
*/
bool KateDocCursor::previousNonSpaceChar()
{
  for(; line >= 0; line--) {
    col = m_doc->kateTextLine(line)->previousNonSpaceChar(col);
    if(col != -1)
      return true; // Previous non-space char found 
    col = 0;
  }
  // No non-space char found
  setPos(-1, -1);
  return false;
}


//
// KateCursor implementation
//

KateCursor::KateCursor(KateDocument *doc) : KateDocCursor(doc)
{
  m_doc->addCursor(this);
}

KateCursor::~KateCursor()
{
  m_doc->removeCursor (this);
}

void KateCursor::position(uint *pline, uint *pcol) const
{
  KateDocCursor::position(pline, pcol);
}

bool KateCursor::setPosition(uint _line, uint _col)
{
  return KateDocCursor::setPosition(_line, _col);
}

bool KateCursor::insertText(const QString& s)
{
  return KateDocCursor::insertText(s);
}

bool KateCursor::removeText(uint nbChar)
{
  return KateDocCursor::removeText(nbChar);
}

QChar KateCursor::currentChar() const
{
  return KateDocCursor::currentChar();
}


