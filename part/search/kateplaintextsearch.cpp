/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2009-2010 Bernhard Beschow <bbeschow@cs.tu-berlin.de>
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
#include "kateplaintextsearch.h"

#include "kateregexpsearch.h"

#include <ktexteditor/document.h>

#include <kdebug.h>
//END  includes


//BEGIN d'tor, c'tor
//
// KateSearch Constructor
//
KatePlainTextSearch::KatePlainTextSearch ( KTextEditor::Document *document, Qt::CaseSensitivity caseSensitivity, bool wholeWords )
: m_document (document)
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
    const int forMin  = inputRange.start().line(); // first line in range
    const int forMax  = inputRange.end().line() + 1 - needleLines.count(); // last line in range
    const int forInit = backwards ? forMax : forMin;
    const int forInc  = backwards ? -1 : +1;

    for (int j = forInit; (forMin <= j) && (j <= forMax); j += forInc)
    {
      // try to match all lines
      const int startCol = m_document->lineLength(j) - needleLines[0].length();
      for (int k = 0; k < needleLines.count(); k++)
      {
        // which lines to compare
        const QString & needleLine = needleLines[k];
        const QString & hayLine = m_document->line(j + k);

        // position specific comparison (first, middle, last)
        if (k == 0) {
          // first line
          if (forMin == j && startCol < inputRange.start().column())
            break;
          if (!hayLine.endsWith(needleLine, m_caseSensitivity))
            break;
        } else if (k == needleLines.count() - 1) {
          // last line
          const int maxRight = (j + k == inputRange.end().line()) ? inputRange.end().column() : hayLine.length();

          if (hayLine.startsWith(needleLine, m_caseSensitivity) && needleLine.length() <= maxRight)
            return KTextEditor::Range(j, startCol, j + k, needleLine.length());
        } else {
          // mid lines
          if (hayLine.compare(needleLine, m_caseSensitivity) != 0) {
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
      if ((line < 0) || (m_document->lines() <= line))
      {
        kWarning() << "line " << line << " is not within interval [0.." << m_document->lines() << ") ... returning invalid range";
        return KTextEditor::Range::invalid();
      }

      const QString textLine = m_document->line(line);

      const int offset   = (line == startLine) ? startCol : 0;
      const int line_end = (line ==   endLine) ?   endCol : textLine.length();
      const int foundAt = backwards ? textLine.lastIndexOf(text, line_end-text.length(), m_caseSensitivity) :
                                      textLine.indexOf(text, offset, m_caseSensitivity);

      if ((offset <= foundAt) && (foundAt + text.length() <= line_end))
        return KTextEditor::Range(line, foundAt, line, foundAt + text.length());
    }
  }
  return KTextEditor::Range::invalid();
}

// kate: space-indent on; indent-width 2; replace-tabs on;
