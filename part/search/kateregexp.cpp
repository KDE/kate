/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2009 Bernhard Beschow <bbeschow@cs.tu-berlin.de>
 *  Copyright (C) 2007 Sebastian Pipping <webmaster@hartwork.org>
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

#include "kateregexp.h"

KateRegExp::KateRegExp(const QString &pattern, Qt::CaseSensitivity cs,
    QRegExp::PatternSyntax syntax)
  : m_regExp(pattern, cs, syntax)
{
}

// these things can besides '.' and '\s' make apptern multi-line:
// \n, \x000A, \x????-\x????, \0012, \0???-\0???
// a multi-line pattern must not pass as single-line, the other
// way around will just result in slower searches and is therefore
// not as critical
int KateRegExp::repairPattern(bool & stillMultiLine)
{
  const QString & text = pattern(); // read-only input for parsing

  // get input
  const int inputLen = text.length();
  int input = 0; // walker index

  // prepare output
  QString output;
  output.reserve(2 * inputLen + 1); // twice should be enough for the average case

  // parser state
  stillMultiLine = false;
  int replaceCount = 0;
  bool insideClass = false;

  while (input < inputLen)
  {
    if (insideClass)
    {
      // wait for closing, unescaped ']'
      switch (text[input].unicode())
      {
      case L'\\':
        switch (text[input + 1].unicode())
        {
        case L'x':
          if (input + 5 < inputLen)
          {
            // copy "\x????" unmodified
            output.append(text.mid(input, 6));
            input += 6;
          } else {
            // copy "\x" unmodified
            output.append(text.mid(input, 2));
            input += 2;
          }
          stillMultiLine = true;
          break;

        case L'0':
          if (input + 4 < inputLen)
          {
            // copy "\0???" unmodified
            output.append(text.mid(input, 5));
            input += 5;
          } else {
            // copy "\0" unmodified
            output.append(text.mid(input, 2));
            input += 2;
          }
          stillMultiLine = true;
          break;

        case L's':
          // replace "\s" with "[ \t]"
          output.append("[ \\t]");
          input += 2;
          replaceCount++;
          break;

        case L'n':
          stillMultiLine = true;
          // FALLTROUGH

        default:
          // copy "\?" unmodified
          output.append(text.mid(input, 2));
          input += 2;
        }
        break;

      case L']':
        // copy "]" unmodified
        insideClass = false;
        output.append(text[input]);
        input++;
        break;

      default:
        // copy "?" unmodified
        output.append(text[input]);
        input++;

      }
    }
    else
    {
      // search for real dots and \S
      switch (text[input].unicode())
      {
      case L'\\':
        switch (text[input + 1].unicode())
        {
        case L'x':
          if (input + 5 < inputLen)
          {
            // copy "\x????" unmodified
            output.append(text.mid(input, 6));
            input += 6;
          } else {
            // copy "\x" unmodified
            output.append(text.mid(input, 2));
            input += 2;
          }
          stillMultiLine = true;
          break;

        case L'0':
          if (input + 4 < inputLen)
          {
            // copy "\0???" unmodified
            output.append(text.mid(input, 5));
            input += 5;
          } else {
            // copy "\0" unmodified
            output.append(text.mid(input, 2));
            input += 2;
          }
          stillMultiLine = true;
          break;

        case L's':
          // replace "\s" with "[ \t]"
          output.append("[ \\t]");
          input += 2;
          replaceCount++;
          break;

        case L'n':
          stillMultiLine = true;
          // FALLTROUGH

        default:
          // copy "\?" unmodified
          output.append(text.mid(input, 2));
          input += 2;
        }
        break;

      case L'.':
        // replace " with "[^\n]"
        output.append("[^\\n]");
        input++;
        replaceCount++;
        break;

      case L'[':
        // copy "]" unmodified
        insideClass = true;
        output.append(text[input]);
        input++;
        break;

      default:
        // copy "?" unmodified
        output.append(text[input]);
        input++;

      }
    }
  }

  // Overwrite with repaired pattern
  m_regExp.setPattern(output);
  return replaceCount;
}



bool KateRegExp::isMultiLine() const
{
  const QString &text = pattern();

  // parser state
  bool insideClass = false;

  for (int input = 0; input < text.length(); /*empty*/ )
  {
    if (insideClass)
    {
      // wait for closing, unescaped ']'
      switch (text[input].unicode())
      {
      case L'\\':
        switch (text[input + 1].unicode())
        {
        case L'x':
          return true;

        case L'0':
          return true;

        case L's':
          // replace "\s" with "[ \t]"
          input += 2;
          break;

        case L'n':
          return true;
          // FALLTROUGH

        default:
          // copy "\?" unmodified
          input += 2;
        }
        break;

      case L']':
        // copy "]" unmodified
        insideClass = false;
        input++;
        break;

      default:
        // copy "?" unmodified
        input++;

      }
    }
    else
    {
      // search for real dots and \S
      switch (text[input].unicode())
      {
      case L'\\':
        switch (text[input + 1].unicode())
        {
        case L'x':
          return true;

        case L'0':
          return true;

        case L's':
          // replace "\s" with "[ \t]"
          input += 2;
          break;

        case L'n':
          return true;

        default:
          // copy "\?" unmodified
          input += 2;
        }
        break;

      case L'.':
        // replace " with "[^\n]"
        input++;
        break;

      case L'[':
        // copy "]" unmodified
        insideClass = true;
        input++;
        break;

      default:
        // copy "?" unmodified
        input++;

      }
    }
  }

  return false;
}



int KateRegExp::indexIn(const QString &str, int start, int end) const
{
  return m_regExp.indexIn(str.left(end), start, QRegExp::CaretAtZero);
}



int KateRegExp::lastIndexIn(const QString &str, int start, int end) const
{
  const int index = m_regExp.lastIndexIn(str.mid(start, end-start), -1, QRegExp::CaretAtZero);

  if (index == -1)
    return -1;

  const int index2 = m_regExp.indexIn(str.left(end), start+index, QRegExp::CaretAtZero);

  return index2;
}

// kate: space-indent on; indent-width 2; replace-tabs on;
