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

#include "katecursor.h"

#include "katedocument.h"
#include "katetextline.h"
#include "katesuperrange.h"

#include <ktexteditor/attribute.h>

//
// KateDocCursor implementation
//

KateDocCursor::KateDocCursor(KateDocument *doc) : KTextEditor::Cursor(), m_doc(doc)
{
}

KateDocCursor::KateDocCursor(const KTextEditor::Cursor &position, KateDocument *doc)
  : KTextEditor::Cursor(position), m_doc(doc)
{
}

KateDocCursor::KateDocCursor(int line, int col, KateDocument *doc)
  : KTextEditor::Cursor(line, col), m_doc(doc)
{
}

bool KateDocCursor::validPosition(int line, int col)
{
  return line >= 0 && col >= 0 && line < m_doc->lines() && col <= m_doc->lineLength(line);
}

bool KateDocCursor::validPosition()
{
  return validPosition(line(), column());
}

bool KateDocCursor::gotoNextLine()
{
  bool ok = (line() + 1 < m_doc->lines());

  if (ok) {
    m_line++;
    m_column = 0;
  }

  return ok;
}

bool KateDocCursor::gotoPreviousLine()
{
  bool ok = (line() > 0);

  if (ok) {
    m_line--;
    m_column = 0;
  }

  return ok;
}

bool KateDocCursor::gotoEndOfNextLine()
{
  bool ok = gotoNextLine();
  if(ok)
    m_column = m_doc->lineLength(line());

  return ok;
}

bool KateDocCursor::gotoEndOfPreviousLine()
{
  bool ok = gotoPreviousLine();
  if(ok)
    m_column = m_doc->lineLength(line());

  return ok;
}

int KateDocCursor::nbCharsOnLineAfter()
{
  return ((int)m_doc->lineLength(line()) - column());
}

bool KateDocCursor::moveForward(uint nbChar)
{
  int nbCharLeft = nbChar - nbCharsOnLineAfter();

  if(nbCharLeft > 0) {
    return gotoNextLine() && moveForward((uint)nbCharLeft);
  } else {
    m_column += nbChar;
    return true;
  }
}

bool KateDocCursor::moveBackward(uint nbChar)
{
  int nbCharLeft = nbChar - m_column;
  if(nbCharLeft > 0) {
    return gotoEndOfPreviousLine() && moveBackward((uint)nbCharLeft);
  } else {
    m_column -= nbChar;
    return true;
  }
}

bool KateDocCursor::insertText(const QString& s)
{
  return m_doc->insertText(*this, s);
}

bool KateDocCursor::removeText(uint nbChar)
{
  // Get a cursor at the end of the removed area
  KateDocCursor endCursor = *this;
  endCursor.moveForward(nbChar);

  // Remove the text
  return m_doc->removeText(KTextEditor::Range(*this, endCursor));
}

QChar KateDocCursor::currentChar() const
{
  return m_doc->plainKateTextLine(line())->getChar(column());
}

uchar KateDocCursor::currentAttrib() const
{
  return m_doc->plainKateTextLine(line())->attribute(column());
}

bool KateDocCursor::nextNonSpaceChar()
{
  for(; m_line < m_doc->lines(); ++m_line) {
    m_column = m_doc->plainKateTextLine(line())->nextNonSpaceChar(column());
    if(m_column != -1)
      return true; // Next non-space char found
    m_column = 0;
  }
  // No non-space char found
  setPosition(-1, -1);
  return false;
}

bool KateDocCursor::previousNonSpaceChar()
{
  while (true) {
    m_column = m_doc->plainKateTextLine(line())->previousNonSpaceChar(column());
    if(m_column != -1) return true; // Previous non-space char found
    if(m_line == 0) return false;
    --m_line;
    m_column = m_doc->plainKateTextLine(m_line)->length();
  }
  // No non-space char found
  setPosition(-1, -1);
  return false;
}

// kate: space-indent on; indent-width 2; replace-tabs on;
