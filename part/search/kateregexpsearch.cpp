/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2010 Bernhard Beschow <bbeschow@cs.tu-berlin.de>
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
#include "kateregexpsearch.h"
#include "kateregexp.h"

#include <ktexteditor/document.h>
//END  includes



// Turn debug messages on/off here
// #define FAST_DEBUG_ENABLE

#ifdef FAST_DEBUG_ENABLE
# define FAST_DEBUG(x) (kDebug( 13020 ) << x)
#else
# define FAST_DEBUG(x)
#endif


//BEGIN d'tor, c'tor
//
// KateSearch Constructor
//
KateRegExpSearch::KateRegExpSearch ( KTextEditor::Document *document, Qt::CaseSensitivity caseSensitivity )
: QObject (document)
, m_document (document)
, m_caseSensitivity (caseSensitivity)
{
}

//
// KateSearch Destructor
//
KateRegExpSearch::~KateRegExpSearch()
{
}


// helper structs for captures re-construction
struct TwoViewCursor {
  int index;
  int openLine;
  int openCol;
  int closeLine;
  int closeCol;
  // note: open/close distinction does not seem needed
  // anymore. i keep it to make a potential way back
  // easier. overhead is minimal.
};

struct IndexPair {
  int openIndex;
  int closeIndex;
};



QVector<KTextEditor::Range> KateRegExpSearch::search(
    const QString &pattern,
    const KTextEditor::Range & inputRange,
    bool backwards)
{
  // regex search
  KateRegExp regexp(pattern, m_caseSensitivity);

  FAST_DEBUG("KateRegExpSearch::searchRegex( " << inputRange.start().line() << ", "
    << inputRange.start().column() << ", " << regexp.pattern() << ", " << backwards << " )");
  if (regexp.isEmpty() || !regexp.isValid() || !inputRange.isValid() || (inputRange.start() == inputRange.end()))
  {
    QVector<KTextEditor::Range> result;
    result.append(KTextEditor::Range::invalid());
    return result;
  }


  // detect pattern type (single- or mutli-line)
  bool isMultiLine;

  // detect '.' and '\s' and fix them
  const bool dotMatchesNewline = false; // TODO
  const int replacements = regexp.repairPattern(isMultiLine);
  if (dotMatchesNewline && (replacements > 0))
  {
    isMultiLine = true;
  }

  const int firstLineIndex = inputRange.start().line();
  const int minColStart = inputRange.start().column();
//  const int maxColEnd = inputRange.end().column();
  if (isMultiLine)
  {
    // multi-line regex search (both forward and backward mode)
    QString wholeDocument;
    const int inputLineCount = inputRange.end().line() - inputRange.start().line() + 1;
    FAST_DEBUG("multi line search (lines " << firstLineIndex << ".." << firstLineIndex + inputLineCount - 1 << ")");

    // nothing to do...
    if (firstLineIndex >= m_document->lines())
    {
      QVector<KTextEditor::Range> result;
      result.append(KTextEditor::Range::invalid());
      return result;
    }

    QVector<int> lineLens (inputLineCount);

    // first line
    if (firstLineIndex < 0 || m_document->lines() >= firstLineIndex)
    {
      QVector<KTextEditor::Range> result;
      result.append(KTextEditor::Range::invalid());
      return result;
    }

    const QString firstLine = m_document->line(firstLineIndex);

    const int firstLineLen = firstLine.length() - minColStart;
    wholeDocument.append(firstLine.right(firstLineLen));
    lineLens[0] = firstLineLen;
    FAST_DEBUG("  line" << 0 << "has length" << lineLens[0]);

    // second line and after
    const QString sep("\n");
    for (int i = 1; i < inputLineCount; i++)
    {
      const int lineNum = firstLineIndex + i;
      if (lineNum < 0 || m_document->lines() >= lineNum)
      {
        QVector<KTextEditor::Range> result;
        result.append(KTextEditor::Range::invalid());
        return result;
      }
      const QString text = m_document->line(lineNum);

      lineLens[i] = text.length();
      wholeDocument.append(sep);
      wholeDocument.append(text);
      FAST_DEBUG("  line" << i << "has length" << lineLens[i]);
    }

    const int pos = backwards
        ? regexp.lastIndexIn(wholeDocument, -1, QRegExp::CaretAtZero)
        : regexp.indexIn(wholeDocument, 0, QRegExp::CaretAtZero);
    if (pos == -1)
    {
      // no match
      FAST_DEBUG("not found");
      {
        QVector<KTextEditor::Range> result;
        result.append(KTextEditor::Range::invalid());
        return result;
      }
    }

#ifdef FAST_DEBUG_ENABLE
    const int matchLen = regexp.matchedLength();
    FAST_DEBUG("found at relative pos " << pos << ", length " << matchLen);
#endif

    // save opening and closing indices and build a map.
    // the correct values will be written into it later.
    QMap<int, TwoViewCursor *> indicesToCursors;
    const int numCaptures = regexp.numCaptures();
    QVector<IndexPair> indexPairs(1 + numCaptures);
    for (int z = 0; z <= numCaptures; z++)
    {
      const int openIndex = regexp.pos(z);
      IndexPair & pair = indexPairs[z];
      if (openIndex == -1)
      {
        // empty capture gives invalid
        pair.openIndex = -1;
        pair.closeIndex = -1;
        FAST_DEBUG("capture []");
      }
      else
      {
        const int closeIndex = openIndex + regexp.cap(z).length();
        pair.openIndex = openIndex;
        pair.closeIndex = closeIndex;
        FAST_DEBUG("capture [" << pair.openIndex << ".." << pair.closeIndex << "]");

        // each key no more than once
        if (!indicesToCursors.contains(openIndex))
        {
          TwoViewCursor * twoViewCursor = new TwoViewCursor;
          twoViewCursor->index = openIndex;
          indicesToCursors.insert(openIndex, twoViewCursor);
          FAST_DEBUG("  border index added: " << openIndex);
        }
        if (!indicesToCursors.contains(closeIndex))
        {
          TwoViewCursor * twoViewCursor = new TwoViewCursor;
          twoViewCursor->index = closeIndex;
          indicesToCursors.insert(closeIndex, twoViewCursor);
          FAST_DEBUG("  border index added: " << closeIndex);
        }
      }
    }

    // find out where they belong
    int curRelLine = 0;
    int curRelCol = 0;
    int curRelIndex = 0;
    QMap<int, TwoViewCursor *>::const_iterator iter = indicesToCursors.constBegin();
    while (iter != indicesToCursors.constEnd())
    {
      // forward to index, save line/col
      const int index = (*iter)->index;
      FAST_DEBUG("resolving position" << index);
      TwoViewCursor & twoViewCursor = *(*iter);
      while (curRelIndex <= index)
      {
        FAST_DEBUG("walk pos (" << curRelLine << "," << curRelCol << ") = "
            << curRelIndex << "relative, steps more to go" << index - curRelIndex);
        const int curRelLineLen = lineLens[curRelLine];
        const int curLineRemainder = curRelLineLen - curRelCol;
        const int lineFeedIndex = curRelIndex + curLineRemainder;
        if (index <= lineFeedIndex) {
            if (index == lineFeedIndex) {
                // on this line _on_ line feed
                FAST_DEBUG("  on line feed");
                const int absLine = curRelLine + firstLineIndex;
                twoViewCursor.openLine
                    = twoViewCursor.closeLine
                    = absLine;
                twoViewCursor.openCol
                    = twoViewCursor.closeCol
                    = ((curRelLine == 0) ? minColStart : 0) + curRelLineLen;

                // advance to next line
                const int advance = (index - curRelIndex) + 1;
                curRelLine++;
                curRelCol = 0;
                curRelIndex += advance;
            } else { // index < lineFeedIndex
                // on this line _before_ line feed
                FAST_DEBUG("  before line feed");
                const int diff = (index - curRelIndex);
                const int absLine = curRelLine + firstLineIndex;
                const int absCol = ((curRelLine == 0) ? minColStart : 0) + curRelCol + diff;
                twoViewCursor.openLine
                    = twoViewCursor.closeLine
                    = absLine;
                twoViewCursor.openCol
                    = twoViewCursor.closeCol
                    = absCol;

                // advance on same line
                const int advance = diff + 1;
                curRelCol += advance;
                curRelIndex += advance;
            }
            FAST_DEBUG("open(" << twoViewCursor.openLine << "," << twoViewCursor.openCol
                << ")  close(" << twoViewCursor.closeLine << "," << twoViewCursor.closeCol << ")");
        }
        else // if (index > lineFeedIndex)
        {
          // not on this line
          // advance to next line
          FAST_DEBUG("  not on this line");
          const int advance = curLineRemainder + 1;
          curRelLine++;
          curRelCol = 0;
          curRelIndex += advance;
        }
      }

      ++iter;
    }

    // build result array
    QVector<KTextEditor::Range> result(1 + numCaptures);
    for (int y = 0; y <= numCaptures; y++)
    {
      IndexPair & pair = indexPairs[y];
      if ((pair.openIndex == -1) || (pair.closeIndex == -1))
      {
        result[y] = KTextEditor::Range::invalid();
      }
      else
      {
        const TwoViewCursor * const openCursors = indicesToCursors[pair.openIndex];
        const TwoViewCursor * const closeCursors = indicesToCursors[pair.closeIndex];
        const int startLine = openCursors->openLine;
        const int startCol = openCursors->openCol;
        const int endLine = closeCursors->closeLine;
        const int endCol = closeCursors->closeCol;
        FAST_DEBUG("range " << y << ": (" << startLine << ", " << startCol << ")..(" << endLine << ", " << endCol << ")");
        result[y] = KTextEditor::Range(startLine, startCol, endLine, endCol);
      }
    }

    // free structs allocated for indicesToCursors
    iter = indicesToCursors.constBegin();
    while (iter != indicesToCursors.constEnd())
    {
      TwoViewCursor * const twoViewCursor = *iter;
      delete twoViewCursor;
      ++iter;
    }
    return result;
  }
  else
  {
    // single-line regex search (both forward of backward mode)
    const int minLeft  = inputRange.start().column();
    const uint maxRight = inputRange.end().column(); // first not included
    const int forMin   = inputRange.start().line();
    const int forMax   = inputRange.end().line();
    const int forInit  = backwards ? forMax : forMin;
    const int forInc   = backwards ? -1 : +1;
    FAST_DEBUG("single line " << (backwards ? forMax : forMin) << ".."
      << (backwards ? forMin : forMax));
    for (int j = forInit; (forMin <= j) && (j <= forMax); j += forInc)
    {
      if (j < 0 || m_document->lines() <= j)
      {
        FAST_DEBUG("searchText | line " << j << ": no");
        QVector<KTextEditor::Range> result;
        result.append(KTextEditor::Range::invalid());
        return result;
      }
      const QString textLine = m_document->line(j);

        // Find (and don't match ^ in between...)
        const int first = (j == forMin) ? minLeft : 0;
        const int afterLast = (j == forMax) ? maxRight : textLine.length();
        const QString hay = textLine.left(afterLast);
        bool found = true;
        int foundAt;
        uint myMatchLen;
        if (backwards) {
            const int lineLen = textLine.length();
            const int offset = afterLast - lineLen - 1;
            FAST_DEBUG("lastIndexIn(" << hay << "," << offset << ")");
            foundAt = regexp.lastIndexIn(hay, offset);
            found = (foundAt != -1) && (foundAt >= first);
        } else {
            FAST_DEBUG("indexIn(" << hay << "," << first << ")");
            foundAt = regexp.indexIn(hay, first);
            found = (foundAt != -1);
        }
        myMatchLen = found ? regexp.matchedLength() : 0;

      /*
      TODO do we still need this?

          // A special case which can only occur when searching with a regular expression consisting
          // only of a lookahead (e.g. ^(?=\{) for a function beginning without selecting '{').
          if (myMatchLen == 0 && line == startPosition.line() && foundAt == (uint) col)
          {
            if (col < lineLength(line))
              col++;
            else {
              line++;
              col = 0;
            }
            continue;
          }
      */

      if (found && !((j == forMax) && (static_cast<uint>(foundAt + myMatchLen) > maxRight)))
      {
        FAST_DEBUG("line " << j << ": yes");

        // build result array
        const int numCaptures = regexp.numCaptures();
        QVector<KTextEditor::Range> result(1 + numCaptures);
        result[0] = KTextEditor::Range(j, foundAt, j, foundAt + myMatchLen);
        FAST_DEBUG("result range " << 0 << ": (" << j << ", " << foundAt << ")..(" << j << ", " << foundAt + myMatchLen << ")");
        for (int y = 1; y <= numCaptures; y++)
        {
          const int openIndex = regexp.pos(y);
          if (openIndex == -1)
          {
            result[y] = KTextEditor::Range::invalid();
            FAST_DEBUG("capture []");
          }
          else
          {
            const int closeIndex = openIndex + regexp.cap(y).length();
            FAST_DEBUG("result range " << y << ": (" << j << ", " << openIndex << ")..(" << j << ", " << closeIndex << ")");
            result[y] = KTextEditor::Range(j, openIndex, j, closeIndex);
          }
        }
        return result;
      }
      else
      {
        FAST_DEBUG("searchText | line " << j << ": no");
      }
    }
  }

  QVector<KTextEditor::Range> result;
  result.append(KTextEditor::Range::invalid());
  return result;
}


/*static*/ QString KateRegExpSearch::escapePlaintext(const QString & text, QList<ReplacementPart> * parts,
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
      case L'l': // FALLTHROUGH
      case L'U': // FALLTHROUGH
      case L'u':
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

          case L'l':
            curPart.type = ReplacementPart::LowerCaseFirst;
            break;

          case L'U':
            curPart.type = ReplacementPart::UpperCase;
            break;

          case L'u':
            curPart.type = ReplacementPart::UpperCaseFirst;
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


// Kill our helpers again
#ifdef FAST_DEBUG_ENABLE
# undef FAST_DEBUG_ENABLE
#endif
#undef FAST_DEBUG

// kate: space-indent on; indent-width 2; replace-tabs on;
