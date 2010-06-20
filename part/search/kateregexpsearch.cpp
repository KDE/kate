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


class KateRegExpSearch::ReplacementStream
{
public:
  struct counter {
    counter(int value, int minWidth)
      : value(value)
      , minWidth(minWidth)
    {}

    const int value;
    const int minWidth;
  };

  struct cap {
    cap(int n)
      : n(n)
    {}

    const int n;
  };

  enum CaseConversion {
    upperCase,      ///< \U ... uppercase from now on
    upperCaseFirst, ///< \u ... uppercase the first letter
    lowerCase,      ///< \L ... lowercase from now on
    lowerCaseFirst, ///< \l ... lowercase the first letter
    keepCase        ///< \E ... back to original case
  };

public:
  ReplacementStream(const QStringList &capturedTexts);

  QString str() const { return m_str; }

  ReplacementStream &operator<<(const QString &);
  ReplacementStream &operator<<(const counter &);
  ReplacementStream &operator<<(const cap &);
  ReplacementStream &operator<<(CaseConversion);

private:
  const QStringList m_capturedTexts;
  CaseConversion m_caseConversion;
  QString m_str;
};


KateRegExpSearch::ReplacementStream::ReplacementStream(const QStringList &capturedTexts)
  : m_capturedTexts(capturedTexts)
  , m_caseConversion(keepCase)
{
}

KateRegExpSearch::ReplacementStream &KateRegExpSearch::ReplacementStream::operator<<(const QString &str)
{
    switch (m_caseConversion) {
    case upperCase:
        // Copy as uppercase
        m_str.append(str.toUpper());
        break;

    case upperCaseFirst:
        if (str.length() > 0) {
            m_str.append(str.at(0).toUpper());
            m_str.append(str.mid(1));
            m_caseConversion = keepCase;
        }
        break;

    case lowerCase:
        // Copy as lowercase
        m_str.append(str.toLower());
        break;

    case lowerCaseFirst:
        if (str.length() > 0) {
            m_str.append(str.at(0).toLower());
            m_str.append(str.mid(1));
            m_caseConversion = keepCase;
        }
        break;

    case keepCase: // FALLTHROUGH
    default:
        // Copy unmodified
        m_str.append(str);
        break;

    }

    return *this;
}


KateRegExpSearch::ReplacementStream &KateRegExpSearch::ReplacementStream::operator<<(const counter &c)
{
  // Zero padded counter value
  m_str.append(QString("%1").arg(c.value, c.minWidth, 10, QLatin1Char('0')));

  return *this;
}


KateRegExpSearch::ReplacementStream &KateRegExpSearch::ReplacementStream::operator<<(const cap &cap)
{
  if (0 <= cap.n && cap.n < m_capturedTexts.size()) {
      (*this) << m_capturedTexts[cap.n];
  } else {
      // Insert just the number to be consistent with QRegExp ("\c" becomes "c")
      m_str.append(QString::number(cap.n));
  }

  return *this;
}


KateRegExpSearch::ReplacementStream &KateRegExpSearch::ReplacementStream::operator<<(CaseConversion caseConversion)
{
  m_caseConversion = caseConversion;

  return *this;
}


//BEGIN d'tor, c'tor
//
// KateSearch Constructor
//
KateRegExpSearch::KateRegExpSearch ( KTextEditor::Document *document, Qt::CaseSensitivity caseSensitivity )
: m_document (document)
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
        ? regexp.lastIndexIn(wholeDocument, 0, wholeDocument.length())
        : regexp.indexIn(wholeDocument, 0, wholeDocument.length());
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
        const int last = (j == forMax) ? maxRight : textLine.length();
        const int foundAt = (backwards ? regexp.lastIndexIn(textLine, first, last)
                                       : regexp.indexIn(textLine, first, last));
        const bool found = (foundAt != -1);

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

      if (found)
      {
        FAST_DEBUG("line " << j << ": yes");

        // build result array
        const int numCaptures = regexp.numCaptures();
        QVector<KTextEditor::Range> result(1 + numCaptures);
        result[0] = KTextEditor::Range(j, foundAt, j, foundAt + regexp.matchedLength());
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


/*static*/ QString KateRegExpSearch::escapePlaintext(const QString & text)
{
  return buildReplacement(text, QStringList(), 0, false);
}


/*static*/ QString KateRegExpSearch::buildReplacement(const QString & text, const QStringList &capturedTexts, int replacementCounter)
{
  return buildReplacement(text, capturedTexts, replacementCounter, true);
}


/*static*/ QString KateRegExpSearch::buildReplacement(const QString & text, const QStringList &capturedTexts, int replacementCounter, bool replacementGoodies) {
  // get input
  const int inputLen = text.length();
  int input = 0; // walker index

  // prepare output
  ReplacementStream out(capturedTexts);

  while (input < inputLen)
  {
    switch (text[input].unicode())
    {
    case L'\n':
      out << text[input];
      input++;
      break;

    case L'\\':
      if (input + 1 >= inputLen)
      {
        // copy backslash
        out << text[input];
        input++;
        break;
      }

      switch (text[input + 1].unicode())
      {
      case L'0': // "\0000".."\0377"
        if (input + 4 >= inputLen)
        {
          out << ReplacementStream::cap(0);
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
                out << QChar(ch);
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
            out << ReplacementStream::cap(0);
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
        out << ReplacementStream::cap(9 - (L'9' - text[input + 1].unicode()));
        input += 2;
        break;

      case L'E': // FALLTHROUGH
      case L'L': // FALLTHROUGH
      case L'l': // FALLTHROUGH
      case L'U': // FALLTHROUGH
      case L'u':
        if (!replacementGoodies) {
          // strip backslash ("\?" -> "?")
          out << text[input + 1];
        } else {
          // handle case switcher
          switch (text[input + 1].unicode()) {
          case L'L':
            out << ReplacementStream::lowerCase;
            break;

          case L'l':
            out << ReplacementStream::lowerCaseFirst;
            break;

          case L'U':
            out << ReplacementStream::upperCase;
            break;

          case L'u':
            out << ReplacementStream::upperCaseFirst;
            break;

          case L'E': // FALLTHROUGH
          default:
            out << ReplacementStream::keepCase;

          }
        }
        input += 2;
        break;

      case L'#':
        if (!replacementGoodies) {
          // strip backslash ("\?" -> "?")
          out << text[input + 1];
          input += 2;
        } else {
          // handle replacement counter
          // eat and count all following hash marks
          // each hash stands for a leading zero: \### will produces 001, 002, ...
          int minWidth = 1;
          while ((input + minWidth + 1 < inputLen) && (text[input + minWidth + 1].unicode() == L'#')) {
            minWidth++;
          }
          out << ReplacementStream::counter(replacementCounter, minWidth);
          input += 1 + minWidth;
        }
        break;

      case L'a':
        out << QChar(0x07);
        input += 2;
        break;

      case L'f':
        out << QChar(0x0c);
        input += 2;
        break;

      case L'n':
        out << QChar(0x0a);
        input += 2;
        break;

      case L'r':
        out << QChar(0x0d);
        input += 2;
        break;

      case L't':
        out << QChar(0x09);
        input += 2;
        break;

      case L'v':
        out << QChar(0x0b);
        input += 2;
        break;

      case L'x': // "\x0000".."\xffff"
        if (input + 5 >= inputLen)
        {
          // strip backslash ("\x" -> "x")
          out << text[input + 1];
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
                  out << QChar(ch);
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
            out << text[input + 1];
            input += 2;
          }
        }
        break;

      default:
        // strip backslash ("\?" -> "?")
        out << text[input + 1];
        input += 2;

      }
      break;

    default:
      out << text[input];
      input++;

    }
  }

  return out.str();
}


// Kill our helpers again
#ifdef FAST_DEBUG_ENABLE
# undef FAST_DEBUG_ENABLE
#endif
#undef FAST_DEBUG

// kate: space-indent on; indent-width 2; replace-tabs on;
