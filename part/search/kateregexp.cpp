/* This file is part of the KDE libraries
   Copyright (C) 2009 Bernhard Beschow <bbeschow@cs.tu-berlin.de>
   Copyright (C) 2007 Sebastian Pipping <webmaster@hartwork.org>

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
   Boston, MA 02111-13020, USA.
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



int KateRegExp::lastIndexIn(const QString & str,
        int offset, QRegExp::CaretMode caretMode) {
    int prevPos = -1;
    int prevLen = 1;
    const int strLen = str.length();
    for (;;) {
        const int pos = m_regExp.indexIn(str, prevPos + prevLen, caretMode);
        if (pos == -1) {
            // No more matches
            break;
        } else {
            const int len = m_regExp.matchedLength();
            if (pos > strLen + offset + 1) {
                // Gone too far, match in no way of use
                break;
            }

            if (pos + len > strLen + offset + 1) {
                // Gone too far, check if usable
                if (offset == -1) {
                    // No shrinking possible
                    break;
                }

                const QString str2 = str.mid(0, strLen + offset + 1);
                const int pos2 = m_regExp.indexIn(str2, pos, caretMode);
                if (pos2 != -1) {
                    // Match usable
                    return pos2;
                } else {
                    // Match NOT usable
                    break;
                }
            }

            // Valid match, but maybe not the last one
            prevPos = pos;
            prevLen = (len == 0) ? 1 : len;
        }
    }

    // Previous match is what we want
    if (prevPos != -1) {
        // Do that very search again
        m_regExp.indexIn(str, prevPos, caretMode);
        return prevPos;
    } else {
        return -1;
    }
}
