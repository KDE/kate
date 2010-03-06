/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2010 Christoph Cullmann <cullmann@kde.org>
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

#include "katetextline.h"

namespace Kate {

TextLineData::TextLineData ()
  : m_flags (0)
{
}

TextLineData::TextLineData (const QString &text)
  : m_text (text)
  , m_flags (0)
{
}

TextLineData::~TextLineData ()
{
}

int TextLineData::firstChar() const
{
  return nextNonSpaceChar(0);
}

int TextLineData::lastChar() const
{
  return previousNonSpaceChar(m_text.length() - 1);
}

int TextLineData::nextNonSpaceChar (int pos) const
{
  Q_ASSERT (pos >= 0);

  for(int i = pos; i < m_text.length(); i++)
    if (!m_text[i].isSpace())
      return i;

  return -1;
}

int TextLineData::previousNonSpaceChar (int pos) const
{
  if (pos >= m_text.length())
    pos = m_text.length() - 1;

  for(int i = pos; i >= 0; i--)
    if (!m_text[i].isSpace())
      return i;

  return -1;
}

QString TextLineData::leadingWhitespace() const
{
  if (firstChar() < 0)
    return string(0, length());

  return string(0, firstChar());
}

int TextLineData::indentDepth (int tabWidth) const
{
  int d = 0;
  const int len = m_text.length();
  const QChar *unicode = m_text.unicode();

  for(int i = 0; i < len; ++i)
  {
    if(unicode[i].isSpace())
    {
      if (unicode[i] == QLatin1Char('\t'))
        d += tabWidth - (d % tabWidth);
      else
        d++;
    }
    else
      return d;
  }

  return d;
}

bool TextLineData::matchesAt(int column, const QString& match) const
{
  if (column < 0)
    return false;

  const int len = m_text.length();
  const int matchlen = match.length();

  if ((column + matchlen) > len)
    return false;

  const QChar *unicode = m_text.unicode();
  const QChar *matchUnicode = match.unicode();

  for (int i=0; i < matchlen; ++i)
    if (unicode[i+column] != matchUnicode[i])
      return false;

  return true;
}

int TextLineData::toVirtualColumn (int column, int tabWidth) const
{
  if (column < 0)
    return 0;

  int x = 0;
  const int zmax = qMin(column, m_text.length());
  const QChar *unicode = m_text.unicode();

  for ( int z = 0; z < zmax; ++z)
  {
    if (unicode[z] == QLatin1Char('\t'))
      x += tabWidth - (x % tabWidth);
    else
      x++;
  }

  return x + column - zmax;
}

int TextLineData::fromVirtualColumn (int column, int tabWidth) const
{
  if (column < 0)
    return 0;

  const int zmax = qMin(m_text.length(), column);
  const QChar *unicode = m_text.unicode();

  int x = 0;
  int z = 0;
  for (; z < zmax; ++z)
  {
    int diff = 1;
    if (unicode[z] == QLatin1Char('\t'))
      diff = tabWidth - (x % tabWidth);

    if (x + diff > column)
      break;
    x += diff;
  }

  return z;
}

int TextLineData::virtualLength (int tabWidth) const
{
  int x = 0;
  const int len = m_text.length();
  const QChar *unicode = m_text.unicode();

  for ( int z = 0; z < len; ++z)
  {
    if (unicode[z] == QLatin1Char('\t'))
      x += tabWidth - (x % tabWidth);
    else
      x++;
  }

  return x;
}

void TextLineData::addAttribute (int start, int length, int attribute)
{
  // try to append to previous range
  if ((m_attributesList.size() > 2) && (m_attributesList[m_attributesList.size()-1] == attribute)
      && (m_attributesList[m_attributesList.size()-3]+m_attributesList[m_attributesList.size()-2]
         == start))
  {
    m_attributesList[m_attributesList.size()-2] += length;
    return;
  }

  m_attributesList.resize (m_attributesList.size()+3);
  m_attributesList[m_attributesList.size()-3] = start;
  m_attributesList[m_attributesList.size()-2] = length;
  m_attributesList[m_attributesList.size()-1] = attribute;
}

}
