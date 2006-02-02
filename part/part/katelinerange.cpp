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

#include "katelinerange.h"

#include <QTextLayout>

#include <kdebug.h>

#include "katedocument.h"

KateLineLayout::KateLineLayout(KateDocument* doc)
  : m_doc(doc)
  , m_textLine(0L)
  , m_line(-1)
  , m_virtualLine(-1)
  , m_shiftX(0)
  , m_layout(0L)
{
  Q_ASSERT(doc);
}

KateLineLayout::~KateLineLayout()
{
  delete m_layout;
}

void KateLineLayout::clear()
{
  m_textLine = 0L;
  m_line = -1;
  m_virtualLine = -1;
  m_shiftX = 0;
  // not touching dirty
  delete m_layout;
  m_layout = 0L;
}

bool KateLineLayout::includesCursor(const KTextEditor::Cursor& realCursor) const
{
  return realCursor.line() == line();
}

const KateTextLine::Ptr& KateLineLayout::textLine() const
{
  if (!m_textLine)
    m_textLine = m_doc->kateTextLine(line());

  Q_ASSERT(m_textLine);

  return m_textLine;
}

int KateLineLayout::line( ) const
{
  return m_line;
}

void KateLineLayout::setLine( int line, int virtualLine )
{
  m_line = line;
  m_virtualLine = (virtualLine == -1) ? m_doc->getVirtualLine(line) : virtualLine;
  m_textLine = 0L;
}

int KateLineLayout::virtualLine( ) const
{
  return m_virtualLine;
}

void KateLineLayout::setVirtualLine( int virtualLine )
{
  m_virtualLine = virtualLine;
}

bool KateLineLayout::startsInvisibleBlock() const
{
  if (!isValid() || virtualLine() == 0)
    return false;

  return (virtualLine() - 1) != (int)m_doc->getVirtualLine(line() - 1);
}

int KateLineLayout::shiftX() const
{
  return m_shiftX;
}

void KateLineLayout::setShiftX(int shiftX)
{
  m_shiftX = shiftX;
}

KateDocument* KateLineLayout::doc() const
{
  return m_doc;
}

bool KateLineLayout::isValid( ) const
{
  return line() != -1 && textLine() && layout();
}

QTextLayout* KateLineLayout::layout() const
{
  return m_layout;
}

void KateLineLayout::setLayout(QTextLayout* layout)
{
  delete m_layout;
  m_layout = layout;
  m_dirtyList.clear();
  if (m_layout)
    for (int i = 0; i < qMax(1, m_layout->lineCount()); ++i)
      m_dirtyList.append(true);
}

void KateLineLayout::invalidateLayout( )
{
  setLayout(0L);
}

bool KateLineLayout::isDirty( int viewLine ) const
{
  Q_ASSERT(isValid() && viewLine >= 0 && viewLine < viewLineCount());
  return m_dirtyList[viewLine];
}

bool KateLineLayout::setDirty( int viewLine, bool dirty )
{
  Q_ASSERT(isValid() && viewLine >= 0 && viewLine < viewLineCount());
  m_dirtyList[viewLine] = dirty;
  return dirty;
}

KTextEditor::Cursor KateLineLayout::start( ) const
{
  return KTextEditor::Cursor(line(), 0);
}

int KateLineLayout::length( ) const
{
  return textLine()->length();
}

int KateLineLayout::viewLineCount( ) const
{
  return m_layout->lineCount();
}

KateTextLayout KateLineLayout::viewLine( int viewLine ) const
{
  if (viewLine < 0)
    viewLine += viewLineCount();
  Q_ASSERT(isValid());
  Q_ASSERT(viewLine >= 0 && viewLine < viewLineCount());
  return KateTextLayout(KateLineLayoutPtr(const_cast<KateLineLayout*>(this)), viewLine);
}

int KateLineLayout::width( ) const
{
  int width = 0;

  for (int i = 0; i < m_layout->lineCount(); ++i)
    width = qMax((int)m_layout->lineAt(i).naturalTextWidth(), width);

  return width;
}

bool KateLineLayout::isOutsideDocument( ) const
{
  return line() < 0 || line() >= m_doc->lines();
}

void KateLineLayout::debugOutput() const
{
  kDebug() << "KateLineLayout: " << this << " valid " << isValid() << " line " << line() << " length " << length() << " width " << width() << " viewLineCount " << viewLineCount() << endl;
}

int KateLineLayout::viewLineForColumn( int column ) const
{
  int len = 0;
  int i = 0;
  for (; i < m_layout->lineCount() - 1; ++i) {
    len += m_layout->lineAt(i).textLength();
    if (column < len)
      return i;
  }
  return i;
}

// kate: space-indent on; indent-width 2; replace-tabs on;
