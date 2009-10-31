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

//BEGIN includes
#include "kateplaintextsearch.h"

#include "kateregexpsearch.h"
#include "katedocument.h"
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
KatePlainTextSearch::KatePlainTextSearch ( KateDocument *document, bool casesensitive, bool wholeWords )
: QObject (document)
, m_document (document)
, m_casesensitive (casesensitive)
, m_wholeWords (wholeWords)
{
}

//
// KateSearch Destructor
//
KatePlainTextSearch::~KatePlainTextSearch()
{
}
//END

QVector<KTextEditor::Range> KatePlainTextSearch::search (const KTextEditor::Range & inputRange, const QString & text, bool backwards)
{
  // abuse regex for whole word plaintext search
  if (m_wholeWords)
  {
    // escape dot and friends
    const QString workPattern = "\\b" + QRegExp::escape(text) + "\\b";

    return KateRegExpSearch(m_document, m_casesensitive).search(inputRange, workPattern, backwards);
  }

  QVector<KTextEditor::Range> result;
  KTextEditor::Range resultRange = searchText(inputRange, text, backwards);
  result.append(resultRange);

  return result;
}

//BEGIN KTextEditor::SearchInterface stuff

KTextEditor::Range KatePlainTextSearch::searchText (const KTextEditor::Range & inputRange, const QString & text, bool backwards)
{
  FAST_DEBUG("KatePlainTextSearch::searchText( " << inputRange.start().line() << ", "
    << inputRange.start().column() << ", " << text << ", " << backwards << " )");
  if (text.isEmpty() || !inputRange.isValid() || (inputRange.start() == inputRange.end()))
  {
    return KTextEditor::Range::invalid();
  }

  // split multi-line needle into single lines
  const QString sep("\n");
  const QStringList needleLines = text.split(sep);
  const int numNeedleLines = needleLines.count();
  FAST_DEBUG("searchText | needle has " << numNeedleLines << " lines");

  if (numNeedleLines > 1)
  {
    // multi-line plaintext search (both forwards or backwards)
    const int lastLine = inputRange.end().line();

    const int forMin   = inputRange.start().line(); // first line in range
    const int forMax   = lastLine + 1 - numNeedleLines; // last line in range
    const int forInit  = backwards ? forMax : forMin;
    const int forInc   = backwards ? -1 : +1;

    const int minLeft  = inputRange.start().column();
    const uint maxRight = inputRange.end().column(); // first not included

    // init hay line ring buffer
    int hayLinesZeroIndex = 0;
    QVector<KateTextLine::Ptr> hayLinesWindow;
    for (int i = 0; i < numNeedleLines; i++) {
      KateTextLine::Ptr textLine = m_document->plainKateTextLine((backwards ? forMax : forMin) + i);

      if (!textLine)
        return KTextEditor::Range::invalid();

      hayLinesWindow.append (textLine);
      FAST_DEBUG("searchText | hayLinesWindow[" << i << "] = \"" << hayLinesWindow[i]->string() << "\"");
    }

    for (int j = forInit; (forMin <= j) && (j <= forMax); j += forInc)
    {
      // try to match all lines
      uint startCol = 0; // init value not important
      for (int k = 0; k < numNeedleLines; k++)
      {
        // which lines to compare
        const QString & needleLine = needleLines[k];
        KateTextLine::Ptr & hayLine = hayLinesWindow[(k + hayLinesZeroIndex) % numNeedleLines];
        FAST_DEBUG("searchText | hayLine = \"" << hayLine->string() << "\"");

        // position specific comparison (first, middle, last)
        if (k == 0) {
          // first line
          if (needleLine.length() == 0) // if needle starts with a newline
          {
            startCol = hayLine->length();
          }
          else
          {
            uint myMatchLen;
            const uint colOffset = (j > forMin) ? 0 : minLeft;
            const bool matches = hayLine->searchText(colOffset, hayLine->length(),needleLine, &startCol,
              &myMatchLen, m_casesensitive, false);
            if (!matches || (startCol + myMatchLen != static_cast<uint>(hayLine->length()))) {
              FAST_DEBUG("searchText | [" << j << " + " << k << "] line " << j + k << ": no");
              break;
            }
            FAST_DEBUG("searchText | [" << j << " + " << k << "] line " << j + k << ": yes");
          }
        } else if (k == numNeedleLines - 1) {
          // last line
          uint foundAt, myMatchLen;
          const bool matches = hayLine->searchText(0,hayLine->length(), needleLine, &foundAt, &myMatchLen, m_casesensitive, false);
          if (matches && (foundAt == 0) && !((k == lastLine)
              && (static_cast<uint>(foundAt + myMatchLen) > maxRight))) // full match!
          {
            FAST_DEBUG("searchText | [" << j << " + " << k << "] line " << j + k << ": yes");
            return KTextEditor::Range(j, startCol, j + k, needleLine.length());
          }
          FAST_DEBUG("searchText | [" << j << " + " << k << "] line " << j + k << ": no");
        } else {
          // mid lines
          uint foundAt, myMatchLen;
          const bool matches = hayLine->searchText(0, hayLine->length(),needleLine, &foundAt, &myMatchLen, m_casesensitive, false);
          if (!matches || (foundAt != 0) || (myMatchLen != static_cast<uint>(needleLine.length()))) {
            FAST_DEBUG("searchText | [" << j << " + " << k << "] line " << j + k << ": no");
            break;
          }
          FAST_DEBUG("searchText | [" << j << " + " << k << "] line " << j + k << ": yes");
        }
      }

      // get a fresh line into the ring buffer
      if ((backwards && (j > forMin)) || (!backwards && (j < forMax)))
      {
        if (backwards)
        {
          hayLinesZeroIndex = (hayLinesZeroIndex + numNeedleLines - 1) % numNeedleLines;

          KateTextLine::Ptr textLine = m_document->plainKateTextLine(j - 1);

          if (!textLine)
            return KTextEditor::Range::invalid();

          hayLinesWindow[hayLinesZeroIndex] = textLine;

          FAST_DEBUG("searchText | filling slot " << hayLinesZeroIndex << " with line "
            << j - 1 << ": " << hayLinesWindow[hayLinesZeroIndex]->string());
        }
        else
        {
          hayLinesWindow[hayLinesZeroIndex] = m_document->plainKateTextLine(j + numNeedleLines);
          FAST_DEBUG("searchText | filling slot " << hayLinesZeroIndex << " with line "
            << j + numNeedleLines << ": " << hayLinesWindow[hayLinesZeroIndex]->string());
          hayLinesZeroIndex = (hayLinesZeroIndex + 1) % numNeedleLines;
        }
      }
    }

    // not found
    return KTextEditor::Range::invalid();
  }
  else
  {
    // single-line plaintext search (both forward of backward mode)
    const int minLeft  = inputRange.start().column();
    const uint maxRight = inputRange.end().column(); // first not included
    const int forMin   = inputRange.start().line();
    const int forMax   = inputRange.end().line();
    const int forInit  = backwards ? forMax : forMin;
    const int forInc   = backwards ? -1 : +1;
    FAST_DEBUG("searchText | single line " << (backwards ? forMax : forMin) << ".."
      << (backwards ? forMin : forMax));
    for (int j = forInit; (forMin <= j) && (j <= forMax); j += forInc)
    {
      KateTextLine::Ptr textLine = m_document->plainKateTextLine(j);
      if (!textLine)
      {
        FAST_DEBUG("searchText | line " << j << ": no");
        return KTextEditor::Range::invalid();
      }

      const int offset = (j == forMin) ? minLeft : 0;
      const int line_end= (j==forMax) ? maxRight : textLine->length();
      uint foundAt, myMatchLen;
      FAST_DEBUG("searchText | searching in line line: " << j);
      const bool found = textLine->searchText (offset,line_end, text, &foundAt, &myMatchLen, m_casesensitive, backwards);
      if (found && !((j == forMax) && (static_cast<uint>(foundAt + myMatchLen) > maxRight)))
      {
        FAST_DEBUG("searchText | line " << j << ": yes");
        return KTextEditor::Range(j, foundAt, j, foundAt + myMatchLen);
      }
      else
      {
        FAST_DEBUG("searchText | line " << j << ": no");
      }
    }
  }
  return KTextEditor::Range::invalid();
}
//END
// Kill our helpers again
#ifdef FAST_DEBUG_ENABLE
# undef FAST_DEBUG_ENABLE
#endif
#undef FAST_DEBUG

// kate: space-indent on; indent-width 2; replace-tabs on;
