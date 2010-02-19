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

//BEGIN includes
#include "kateescapedtextsearch.h"

#include "kateplaintextsearch.h"

#include <ktexteditor/document.h>
//END  includes


//BEGIN d'tor, c'tor
//
// KateEscapedTextSearch Constructor
//
KateEscapedTextSearch::KateEscapedTextSearch ( KTextEditor::Document *document, Qt::CaseSensitivity caseSensitivity, bool wholeWords )
: QObject (document)
, m_document (document)
, m_caseSensitivity (caseSensitivity)
, m_wholeWords (wholeWords)
{
}

//
// KateEscapedTextSearch Destructor
//
KateEscapedTextSearch::~KateEscapedTextSearch()
{
}
//END

KTextEditor::Range KateEscapedTextSearch::search (const QString & text,
        const KTextEditor::Range & inputRange, bool backwards)
{
  KatePlainTextSearch searcher(m_document, m_caseSensitivity, m_wholeWords);
  return searcher.search(escapePlaintext(text), inputRange, backwards);
}

//BEGIN KTextEditor::SearchInterface stuff
/*static*/ QString KateEscapedTextSearch::escapePlaintext(const QString & text, QList<ReplacementPart> * parts,
        bool replacementGoodies) {
  // get input
  const int inputLen = text.length();
  int input = 0; // walker index

  // prepare output
  QString output;
  output.reserve(inputLen + 1);

  while (input < inputLen)
  {
    switch (text[input].unicode())
    {
    case L'\n':
      output.append(text[input]);
      input++;
      break;

    case L'\\':
      if (input + 1 >= inputLen)
      {
        // copy backslash
        output.append(text[input]);
        input++;
        break;
      }

      switch (text[input + 1].unicode())
      {
      case L'0': // "\0000".."\0377"
        if (input + 4 >= inputLen)
        {
          if (parts == NULL)
          {
            // strip backslash ("\0" -> "0")
            output.append(text[input + 1]);
          }
          else
          {
            // handle reference
            ReplacementPart curPart;

            // append text before the reference
            if (!output.isEmpty())
            {
              curPart.type = ReplacementPart::Text;
              curPart.text = output;
              output.clear();
              parts->append(curPart);
              curPart.text.clear();
            }

            // append reference
            curPart.type = ReplacementPart::Reference;
            curPart.index = 0;
            parts->append(curPart);
          }
          input += 2;
        }
        else
        {
          bool stripAndSkip = false;
          const ushort text_2 = text[input + 2].unicode();
          if ((text_2 >= L'0') && (text_2 <= L'3'))
          {
            const ushort text_3 = text[input + 3].unicode();
            if ((text_3 >= L'0') && (text_3 <= L'7'))
            {
              const ushort text_4 = text[input + 4].unicode();
              if ((text_4 >= L'0') && (text_4 <= L'7'))
              {
                int digits[3];
                for (int i = 0; i < 3; i++)
                {
                  digits[i] = 7 - (L'7' - text[input + 2 + i].unicode());
                }
                const int ch = 64 * digits[0] + 8 * digits[1] + digits[2];
                output.append(QChar(ch));
                input += 5;
              }
              else
              {
                stripAndSkip = true;
              }
            }
            else
            {
              stripAndSkip = true;
            }
          }
          else
          {
            stripAndSkip = true;
          }

          if (stripAndSkip)
          {
            if (parts == NULL)
            {
              // strip backslash ("\0" -> "0")
              output.append(text[input + 1]);
            }
            else
            {
              // handle reference
              ReplacementPart curPart;

              // append text before the reference
              if (!output.isEmpty())
              {
                curPart.type = ReplacementPart::Text;
                curPart.text = output;
                output.clear();
                parts->append(curPart);
                curPart.text.clear();
              }

              // append reference
              curPart.type = ReplacementPart::Reference;
              curPart.index = 0;
              parts->append(curPart);
            }
            input += 2;
          }
        }
        break;

      case L'1':
      case L'2':
      case L'3':
      case L'4':
      case L'5':
      case L'6':
      case L'7':
      case L'8':
      case L'9':
        if (parts == NULL)
        {
          // strip backslash ("\?" -> "?")
          output.append(text[input + 1]);
        }
        else
        {
          // handle reference
          ReplacementPart curPart;

          // append text before the reference
          if (!output.isEmpty())
          {
            curPart.type = ReplacementPart::Text;
            curPart.text = output;
            output.clear();
            parts->append(curPart);
            curPart.text.clear();
          }

          // append reference
          curPart.type = ReplacementPart::Reference;
          curPart.index = 9 - (L'9' - text[input + 1].unicode());
          parts->append(curPart);
        }
        input += 2;
        break;

      case L'E': // FALLTHROUGH
      case L'L': // FALLTHROUGH
      case L'U':
        if ((parts == NULL) || !replacementGoodies) {
          // strip backslash ("\?" -> "?")
          output.append(text[input + 1]);
        } else {
          // handle case switcher
          ReplacementPart curPart;

          // append text before case switcher
          if (!output.isEmpty())
          {
            curPart.type = ReplacementPart::Text;
            curPart.text = output;
            output.clear();
            parts->append(curPart);
            curPart.text.clear();
          }

          // append case switcher
          switch (text[input + 1].unicode()) {
          case L'L':
            curPart.type = ReplacementPart::LowerCase;
            break;

          case L'U':
            curPart.type = ReplacementPart::UpperCase;
            break;

          case L'E': // FALLTHROUGH
          default:
            curPart.type = ReplacementPart::KeepCase;

          }
          parts->append(curPart);
        }
        input += 2;
        break;

      case L'#':
        if ((parts == NULL) || !replacementGoodies) {
          // strip backslash ("\?" -> "?")
          output.append(text[input + 1]);
          input += 2;
        } else {
          // handle replacement counter
          ReplacementPart curPart;

          // append text before replacement counter
          if (!output.isEmpty())
          {
            curPart.type = ReplacementPart::Text;
            curPart.text = output;
            output.clear();
            parts->append(curPart);
            curPart.text.clear();
          }

          // eat and count all following hash marks
          // each hash stands for a leading zero: \### will produces 001, 002, ...
          int count = 1;
          while ((input + count + 1 < inputLen) && (text[input + count + 1].unicode() == L'#')) {
            count++;
          }
          curPart.type = ReplacementPart::Counter;
          curPart.index = count; // Each hash stands
          parts->append(curPart);
          input += 1 + count;
        }
        break;

      case L'a':
        output.append(QChar(0x07));
        input += 2;
        break;

      case L'f':
        output.append(QChar(0x0c));
        input += 2;
        break;

      case L'n':
        output.append(QChar(0x0a));
        input += 2;
        break;

      case L'r':
        output.append(QChar(0x0d));
        input += 2;
        break;

      case L't':
        output.append(QChar(0x09));
        input += 2;
        break;

      case L'v':
        output.append(QChar(0x0b));
        input += 2;
        break;

      case L'x': // "\x0000".."\xffff"
        if (input + 5 >= inputLen)
        {
          // strip backslash ("\x" -> "x")
          output.append(text[input + 1]);
          input += 2;
        }
        else
        {
          bool stripAndSkip = false;
          const ushort text_2 = text[input + 2].unicode();
          if (((text_2 >= L'0') && (text_2 <= L'9'))
              || ((text_2 >= L'a') && (text_2 <= L'f'))
              || ((text_2 >= L'A') && (text_2 <= L'F')))
          {
            const ushort text_3 = text[input + 3].unicode();
            if (((text_3 >= L'0') && (text_3 <= L'9'))
                || ((text_3 >= L'a') && (text_3 <= L'f'))
                || ((text_3 >= L'A') && (text_3 <= L'F')))
            {
              const ushort text_4 = text[input + 4].unicode();
              if (((text_4 >= L'0') && (text_4 <= L'9'))
                  || ((text_4 >= L'a') && (text_4 <= L'f'))
                  || ((text_4 >= L'A') && (text_4 <= L'F')))
              {
                const ushort text_5 = text[input + 5].unicode();
                if (((text_5 >= L'0') && (text_5 <= L'9'))
                    || ((text_5 >= L'a') && (text_5 <= L'f'))
                    || ((text_5 >= L'A') && (text_5 <= L'F')))
                {
                  int digits[4];
                  for (int i = 0; i < 4; i++)
                  {
                    const ushort cur = text[input + 2 + i].unicode();
                    if ((cur >= L'0') && (cur <= L'9'))
                    {
                      digits[i] = 9 - (L'9' - cur);
                    }
                    else if ((cur >= L'a') && (cur <= L'f'))
                    {
                      digits[i] = 15 - (L'f' - cur);
                    }
                    else // if ((cur >= L'A') && (cur <= L'F')))
                    {
                      digits[i] = 15 - (L'F' - cur);
                    }
                  }

                  const int ch = 4096 * digits[0] + 256 * digits[1] + 16 * digits[2] + digits[3];
                  output.append(QChar(ch));
                  input += 6;
                }
                else
                {
                  stripAndSkip = true;
                }
              }
              else
              {
                stripAndSkip = true;
              }
            }
            else
            {
              stripAndSkip = true;
            }
          }

          if (stripAndSkip)
          {
            // strip backslash ("\x" -> "x")
            output.append(text[input + 1]);
            input += 2;
          }
        }
        break;

      default:
        // strip backslash ("\?" -> "?")
        output.append(text[input + 1]);
        input += 2;

      }
      break;

    default:
      output.append(text[input]);
      input++;

    }
  }

  if (parts != NULL)
  {
    // append text after the last reference if any
    if (!output.isEmpty())
    {
      ReplacementPart curPart;
      curPart.type = ReplacementPart::Text;
      curPart.text = output;
      parts->append(curPart);
    }
  }

  return output;
}
//END

// kate: space-indent on; indent-width 2; replace-tabs on;
