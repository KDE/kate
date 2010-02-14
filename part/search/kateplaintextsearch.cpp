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


//BEGIN d'tor, c'tor
//
// KateSearch Constructor
//
KatePlainTextSearch::KatePlainTextSearch ( KateDocument *document, Qt::CaseSensitivity caseSensitivity, bool wholeWords )
: QObject (document)
, m_document (document)
, m_caseSensitivity (caseSensitivity)
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

KTextEditor::Range KatePlainTextSearch::search (const QString & text, const KTextEditor::Range & inputRange, bool backwards)
{
  // abuse regex for whole word plaintext search
  if (m_wholeWords)
  {
    // escape dot and friends
    const QString workPattern = "\\b" + QRegExp::escape(text) + "\\b";

    return KateRegExpSearch(m_document, m_caseSensitivity).search(workPattern, inputRange, backwards)[0];
  }

  if (text.isEmpty() || !inputRange.isValid() || (inputRange.start() == inputRange.end()))
  {
    return KTextEditor::Range::invalid();
  }

  // split multi-line needle into single lines
  const QStringList needleLines = text.split("\n");

  if (needleLines.count() > 1)
  {
    // multi-line plaintext search (both forwards or backwards)
    const int lastLine = inputRange.end().line();

    const int forMin   = inputRange.start().line(); // first line in range
    const int forMax   = lastLine + 1 - needleLines.count(); // last line in range
    const int forInit  = backwards ? forMax : forMin;
    const int forInc   = backwards ? -1 : +1;

    for (int j = forInit; (forMin <= j) && (j <= forMax); j += forInc)
    {
      // try to match all lines
      uint startCol = 0; // init value not important
      for (int k = 0; k < needleLines.count(); k++)
      {
        // which lines to compare
        const QString & needleLine = needleLines[k];
        KateTextLine::Ptr hayLine = m_document->plainKateTextLine(j + k);

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
            const uint colOffset = (j > forMin) ? 0 : inputRange.start().column();
            const bool matches = hayLine->searchText(colOffset, hayLine->length(),needleLine, &startCol,
              &myMatchLen, m_caseSensitivity, true);
            if (!matches || (startCol + myMatchLen != static_cast<uint>(hayLine->length()))) {
              break;
            }
          }
        } else if (k == needleLines.count() - 1) {
          // last line
          const uint maxRight = inputRange.end().column();

          uint foundAt, myMatchLen;
          const bool matches = hayLine->searchText(0,hayLine->length(), needleLine, &foundAt, &myMatchLen, m_caseSensitivity, false);
          if (matches && (foundAt == 0) && !((k == lastLine) && (foundAt + myMatchLen > maxRight)))
          {
            return KTextEditor::Range(j, startCol, j + k, needleLine.length());
          }
        } else {
          // mid lines
          uint foundAt, myMatchLen;
          const bool matches = hayLine->searchText(0, hayLine->length(),needleLine, &foundAt, &myMatchLen, m_caseSensitivity, false);
          if (!matches || (foundAt != 0) || (myMatchLen != static_cast<uint>(needleLine.length()))) {
            break;
          }
        }
      }
    }

    // not found
    return KTextEditor::Range::invalid();
  }
  else
  {
    // single-line plaintext search (both forward of backward mode)
    const int startCol  = inputRange.start().column();
    const int endCol    = inputRange.end().column(); // first not included
    const int startLine = inputRange.start().line();
    const int endLine   = inputRange.end().line();
    const int forInc    = backwards ? -1 : +1;

    for (int line = backwards ? endLine : startLine; (startLine <= line) && (line <= endLine); line += forInc)
    {
      KateTextLine::Ptr textLine = m_document->plainKateTextLine(line);
      if (!textLine)
      {
        kWarning() << "line " << line << " is not within interval [0.." << m_document->lines() << ") ... returning invalid range";
        return KTextEditor::Range::invalid();
      }

      const int offset   = (line == startLine) ? startCol : 0;
      const int line_end = (line ==   endLine) ?   endCol : textLine->length();
      uint foundAt, myMatchLen;
      const bool found = textLine->searchText (offset,line_end, text, &foundAt, &myMatchLen, m_caseSensitivity, backwards);

      if (found && (foundAt + myMatchLen <= static_cast<uint>(line_end)))
        return KTextEditor::Range(line, foundAt, line, foundAt + myMatchLen);
    }
  }
  return KTextEditor::Range::invalid();
}

// kate: space-indent on; indent-width 2; replace-tabs on;
