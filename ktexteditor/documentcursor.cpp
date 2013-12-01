/* This file is part of the KDE project
 * 
 *  Copyright (C) 2010 Christoph Cullmann <cullmann@kde.org>
 *  Copyright (C) 2012 Dominik Haumann <dhaumann@kde.org>
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

#include "documentcursor.h"

namespace KTextEditor {
  
DocumentCursor::DocumentCursor(KTextEditor::Document* document)
  : m_document(document)
  , m_cursor(KTextEditor::Cursor::invalid())
{
  // we require a valid document
  Q_ASSERT(m_document);
}

DocumentCursor::DocumentCursor(KTextEditor::Document* document, const KTextEditor::Cursor& position)
  : m_document(document)
  , m_cursor(position)
{
  // we require a valid document
  Q_ASSERT(m_document);
}

DocumentCursor::DocumentCursor(KTextEditor::Document* document, int line, int column)
  : m_document(document)
  , m_cursor(line, column)
{
  // we require a valid document
  Q_ASSERT(m_document);
}

DocumentCursor::DocumentCursor (const DocumentCursor &other)
: m_document(other.m_document)
, m_cursor(other.m_cursor)
{
}

DocumentCursor& DocumentCursor::operator= (const DocumentCursor &other)
{
  m_document = other.m_document;
  m_cursor = other.m_cursor;
  return *this;
}

KTextEditor::Document *DocumentCursor::document () const
{
  return m_document;
}

void DocumentCursor::setPosition (const KTextEditor::Cursor& position)
{
  if (position.isValid()) {
    m_cursor = position;
  } else {
    m_cursor = KTextEditor::Cursor::invalid();
  }
}

int DocumentCursor::line() const
{
  return m_cursor.line();
}

int DocumentCursor::column() const
{
  return m_cursor.column();
}

DocumentCursor::~DocumentCursor ()
{
}

void DocumentCursor::makeValid()
{
  const int line = m_cursor.line();
  const int col = m_cursor.line();

  if (line < 0) {
    m_cursor.setPosition(0, 0);
  } else if (line >= m_document->lines()) {
    m_cursor = m_document->documentEnd();
  } else if (col > m_document->lineLength(line)) {
    m_cursor.setColumn(m_document->lineLength(line));
  }
// TODO KDE5 if QChar::isLowSurrogate() -> move one to the left.
//   } else if (m_document->character(m_cursor).isLowSurrogate()) {
//     Q_ASSERT(col > 0);
//     m_cursor.setColumn(col - 1);
//   }

  Q_ASSERT(isValidTextPosition());
}

void DocumentCursor::setPosition (int line, int column)
{
  m_cursor.setPosition(line, column);
}

void DocumentCursor::setLine(int line)
{
  setPosition(line, column());
}

void DocumentCursor::setColumn(int column)
{
  setPosition(line(), column);
}

bool DocumentCursor::atStartOfLine() const
{
  return isValidTextPosition() && column() == 0;
}

bool DocumentCursor::atEndOfLine() const
{
  return isValidTextPosition() && column() == document()->lineLength(line());
}

bool DocumentCursor::atStartOfDocument() const
{
  return line() == 0 && column() == 0;
}

bool DocumentCursor::atEndOfDocument() const
{
  return m_cursor == document()->documentEnd();
}

bool DocumentCursor::gotoNextLine()
{
  // only allow valid cursors
  const bool ok = isValid() && (line() + 1 < document()->lines());
  
  if (ok) {
    setPosition(Cursor(line() + 1, 0));
  }

  return ok;
}

bool DocumentCursor::gotoPreviousLine()
{
  // only allow valid cursors
  bool ok = (line() > 0) && (column() >= 0);
  
  if (ok) {
    setPosition(Cursor(line() - 1, 0));
  }

  return ok;
}

bool DocumentCursor::move(int chars, WrapBehavior wrapBehavior)
{
  if (!isValid()) {
    return false;
  }

  Cursor c(m_cursor);

  // cache lineLength to minimize calls of KateDocument::lineLength(), as
  // results in locating the correct block in the text buffer every time,
  // which is relatively slow
  int lineLength = document()->lineLength(c.line());

  // special case: cursor position is not in valid text, then the algo does
  // not work for Wrap mode. Hence, catch this special case by setting
  // c.column() to the lineLength()
  if (chars > 0 && wrapBehavior == Wrap && c.column() > lineLength) {
    c.setColumn(lineLength);
  }

  while (chars != 0) {
    if (chars > 0) {
      if (wrapBehavior == Wrap) {
        int advance = qMin(lineLength - c.column(), chars);
        
        if (chars > advance) {
          if (c.line() + 1 >= document()->lines()) {
            return false;
          }
          
          c.setPosition(c.line() + 1, 0);
          chars -= advance + 1; // +1 because of end-of-line wrap

          // advanced one line, so cache correct line length again
          lineLength = document()->lineLength(c.line());
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

        // advanced one line, so cache correct line length again
        lineLength = document()->lineLength(c.line());
      } else {
        c.setColumn(c.column() + chars);
        chars = 0;
      }
    }
  }

  if (c != m_cursor) {
    setPosition(c);
  }
  return true;
}

const Cursor& DocumentCursor::toCursor () const
{
  return m_cursor;
}

DocumentCursor::operator const Cursor& () const
{
  return m_cursor;
}

}

// kate: space-indent on; indent-width 2; replace-tabs on;
