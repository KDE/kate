/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2008 - 2012 Erlend Hamberg <ehamberg@gmail.com>
 *  Copyright (C) 2009 Paul Gideon Dann <pdgiddie@gmail.com>
 *  Copyright (C) 2011 Svyatoslav Kuzmich <svatoslav1@gmail.com>
 *  Copyright (C) 2012 - 2013 Simon St James <kdedevel@etotheipiplusone.com>
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

#include "katevimodebase.h"
#include "katevirange.h"
#include "kateglobal.h"
#include "kateviglobal.h"
#include "katevivisualmode.h"
#include "katevinormalmode.h"
#include "katevireplacemode.h"
#include "kateviinputmodemanager.h"

#include <QString>
#include <QRegExp>
#include "kateconfig.h"
#include "katedocument.h"
#include "kateviewinternal.h"
#include <ktexteditor/view.h>
#include "katerenderer.h"

using KTextEditor::Cursor;
using KTextEditor::Range;

// TODO: the "previous word/WORD [end]" methods should be optimized. now they're being called in a
// loop and all calculations done up to finding a match are trown away when called with a count > 1
// because they will simply be called again from the last found position.
// They should take the count as a parameter and collect the positions in a QList, then return
// element (count - 1)

////////////////////////////////////////////////////////////////////////////////
// HELPER METHODS
////////////////////////////////////////////////////////////////////////////////

void KateViModeBase::yankToClipBoard(QChar chosen_register, QString text)
{
  //only yank to the clipboard if no register was specified,
  // textlength > 1 and there is something else then whitespace
  if ((chosen_register == QLatin1Char('0') || chosen_register == QLatin1Char('-'))
    && text.length() > 1 && !text.trimmed().isEmpty()
  ) {
    KateGlobal::self()->copyToClipboard(text);
  }
}

bool KateViModeBase::deleteRange( KateViRange &r, OperationMode mode, bool addToRegister)
{
  r.normalize();
  bool res = false;
  QString removedText = getRange( r, mode );

  if ( mode == LineWise ) {
    doc()->editStart();
    for ( int i = 0; i < r.endLine-r.startLine+1; i++ ) {
      res = doc()->removeLine( r.startLine );
    }
    doc()->editEnd();
  } else {
      res = doc()->removeText( Range( r.startLine, r.startColumn, r.endLine, r.endColumn), mode == Block );
  }

  QChar chosenRegister = getChosenRegister( '0' );
  if ( addToRegister ) {
    if ( r.startLine == r.endLine ) {
      chosenRegister = getChosenRegister( '-' );
      fillRegister( chosenRegister    , removedText, mode );
    } else {
      fillRegister(chosenRegister,  removedText, mode );
    }
  }
  yankToClipBoard(chosenRegister, removedText);

  return res;
}

const QString KateViModeBase::getRange( KateViRange &r, OperationMode mode) const
{
  r.normalize();
  QString s;

  if ( mode == LineWise ) {
    r.startColumn = 0;
    r.endColumn = getLine( r.endLine ).length();
  }

  if ( r.motionType == ViMotion::InclusiveMotion ) {
    r.endColumn++;
  }

  Range range( r.startLine, r.startColumn, r.endLine, r.endColumn);

  if ( mode == LineWise ) {
    s = doc()->textLines( range ).join( QChar( '\n' ) );
    s.append( QChar( '\n' ) );
  } else if ( mode == Block ) {
      s = doc()->text( range, true );
  } else {
      s = doc()->text( range );
  }

  return s;
}

const QString KateViModeBase::getLine(int line) const
{
    return (line < 0) ? m_view->currentTextLine() : doc()->line(line);
}

const QChar KateViModeBase::getCharUnderCursor() const
{
  Cursor c( m_view->cursorPosition() );

  QString line = getLine( c.line() );

  if ( line.length() == 0 && c.column() >= line.length() ) {
    return QChar::Null;
  }

  return line.at( c.column() );
}

const QString KateViModeBase::getWordUnderCursor() const
{

  return doc()->text( getWordRangeUnderCursor() );
}

const Range KateViModeBase::getWordRangeUnderCursor() const
{
  Cursor c( m_view->cursorPosition() );

  // find first character that is a “word letter” and start the search there
  QChar ch = doc()->character( c );
  int i = 0;
  while ( !ch.isLetterOrNumber() && ! ch.isMark() && ch != '_'
      && m_extraWordCharacters.indexOf( ch) == -1 ) {

    // advance cursor one position
    c.setColumn( c.column()+1 );
    if ( c.column() > doc()->lineLength( c.line() ) ) {
      c.setColumn(0);
      c.setLine( c.line()+1 );
      if (c.line() == doc()->lines())
      {
        return Range::invalid();
      }
    }

    ch = doc()->character( c );
    i++; // count characters that were advanced so we know where to start the search
  }

  // move cursor the word (if cursor was placed on e.g. a paren, this will move
  // it to the right
  updateCursor( c );

  Cursor c1 = findPrevWordStart( c.line(), c.column()+1+i, true );
  Cursor c2 = findWordEnd( c1.line(), c1.column()+i-1, true );
  c2.setColumn( c2.column()+1 );

  return Range(c1, c2);
}

Range KateViModeBase::findPattern(const QString& pattern, bool backwards, bool caseSensitive, const Cursor& startFrom, int count) const
{
  Cursor searchBegin = startFrom;
  KTextEditor::Search::SearchOptions flags = KTextEditor::Search::Regex;

  if (count == -1)
  {
    count = getCount();
  }

  if ( backwards ) {
    flags |= KTextEditor::Search::Backwards;
  }
  if (!caseSensitive)
  {
    flags |= KTextEditor::Search::CaseInsensitive;
  }
  Range finalMatch;
  for (int i = 0; i < count; i++)
  {
    if (!backwards)
    {
      const KTextEditor::Range matchRange = m_view->doc()->searchText(KTextEditor::Range(Cursor(searchBegin.line(), searchBegin.column() + 1), m_view->doc()->documentEnd()), pattern, flags).first();

      if (matchRange.isValid())
      {
        finalMatch = matchRange;
      }
      else
      {
        // Wrap around.
        const KTextEditor::Range wrappedMatchRange = m_view->doc()->searchText(KTextEditor::Range(m_view->doc()->documentRange().start(), m_view->doc()->documentEnd()), pattern, flags).first();
        if (wrappedMatchRange.isValid())
        {
          finalMatch = wrappedMatchRange;
        }
        else
        {
          return Range::invalid();
        }
      }
    }
    else
    {
      // Ok - this is trickier: we can't search in the range from doc start to searchBegin, because
      // the match might extend *beyond* searchBegin.
      // We could search through the entire document and then filter out only those matches that are
      // after searchBegin, but it's more efficient to instead search from the start of the
      // document until the beginning of the line after searchBegin, and then filter.
      // Unfortunately, searchText doesn't necessarily turn up all matches (just the first one, sometimes)
      // so we must repeatedly search in such a way that the previous match isn't found, until we either
      // find no matches at all, or the first match that is before searchBegin.
      Cursor newSearchBegin = Cursor(searchBegin.line(), m_view->doc()->lineLength(searchBegin.line()));
      Range bestMatch = Range::invalid();
      while (true)
      {
        QVector<Range> matchesUnfiltered = m_view->doc()->searchText(Range(newSearchBegin, m_view->doc()->documentRange().start()), pattern, flags);
        kDebug(13070) << "matchesUnfiltered: " << matchesUnfiltered << " searchBegin: " << newSearchBegin;

        if (matchesUnfiltered.size() == 1 && !matchesUnfiltered.first().isValid())
        {
          break;
        }

        // After sorting, the last element in matchesUnfiltered is the last match position.
        qSort(matchesUnfiltered);

        QVector<Range> filteredMatches;
        foreach(Range unfilteredMatch, matchesUnfiltered)
        {
          if (unfilteredMatch.start() < searchBegin)
          {
            filteredMatches.append(unfilteredMatch);
          }
        }
        if (!filteredMatches.isEmpty())
        {
          // Want the latest matching range that is before searchBegin.
          bestMatch = filteredMatches.last();
          break;
        }

        // We found some unfiltered matches, but none were suitable. In case matchesUnfiltered wasn't
        // all matching elements, search again, starting from before the earliest matching range.
        if (filteredMatches.isEmpty())
        {
          newSearchBegin = matchesUnfiltered.first().start();
        }
      }

      Range matchRange = bestMatch;

      if (matchRange.isValid())
      {
        finalMatch = matchRange;
      }
      else
      {
        const KTextEditor::Range wrappedMatchRange = m_view->doc()->searchText(KTextEditor::Range(m_view->doc()->documentEnd(), m_view->doc()->documentRange().start()), pattern, flags).first();


        if (wrappedMatchRange.isValid())
        {
          finalMatch = wrappedMatchRange;
        }
        else
        {
          return Range::invalid();
        }
      }
    }
    searchBegin = finalMatch.start();
  }
  return finalMatch;
}

KateViRange KateViModeBase::findPatternForMotion( const QString& pattern, bool backwards, bool caseSensitive, const Cursor& startFrom, int count ) const
{
  kDebug( 13070 ) << "searching for pattern \"" << pattern << "\", backwards = " << backwards
    << ", caseSensitive = " << caseSensitive << ", count = " << count;
  if ( pattern.isEmpty() ) {
    return KateViRange();
  }

  Range match = findPattern(pattern, backwards, caseSensitive, startFrom, count);

  return KateViRange( match.start().line(), match.start().column(), match.end().line(), match.end().column(), ViMotion::ExclusiveMotion );
}

Cursor KateViModeBase::findNextWordStart( int fromLine, int fromColumn, bool onlyCurrentLine ) const
{
  QString line = getLine( fromLine );

  // the start of word pattern need to take m_extraWordCharacters into account if defined
  QString startOfWordPattern("\\b(\\w");
  if ( m_extraWordCharacters.length() > 0 ) {
    startOfWordPattern.append( QLatin1String( "|[" )+m_extraWordCharacters+']' );
  }
  startOfWordPattern.append( ')' );

  QRegExp startOfWord( startOfWordPattern );    // start of a word
  QRegExp nonSpaceAfterSpace( "\\s\\S" );       // non-space right after space
  QRegExp nonWordAfterWord( "\\b(?!\\s)\\W" );  // word-boundary followed by a non-word which is not a space

  int l = fromLine;
  int c = fromColumn;

  bool found = false;

  while ( !found ) {
    int c1 = startOfWord.indexIn( line, c + 1 );
    int c2 = nonSpaceAfterSpace.indexIn( line, c );
    int c3 = nonWordAfterWord.indexIn( line, c + 1 );

    if ( c1 == -1 && c2 == -1 && c3 == -1 ) {
        if ( onlyCurrentLine ) {
            return Cursor::invalid();
        } else if ( l >= doc()->lines()-1 ) {
            c = qMax(line.length()-1, 0);
            return Cursor::invalid();
        } else {
            c = 0;
            l++;

            line = getLine( l );

        if ( line.length() == 0 || !line.at( c ).isSpace() ) {
          found = true;
        }

        continue;
      }
    }

    c2++; // the second regexp will match one character *before* the character we want to go to

    if ( c1 <= 0 )
      c1 = line.length()-1;
    if ( c2 <= 0 )
      c2 = line.length()-1;
    if ( c3 <= 0 )
      c3 = line.length()-1;

    c = qMin( c1, qMin( c2, c3 ) );

    found = true;
  }

  return Cursor( l, c );
}

Cursor KateViModeBase::findNextWORDStart( int fromLine, int fromColumn, bool onlyCurrentLine ) const
{
  Cursor cursor ( m_view->cursorPosition() );
  QString line = getLine();
  KateViRange r( cursor.line(), cursor.column(), ViMotion::ExclusiveMotion );

  int l = fromLine;
  int c = fromColumn;

  bool found = false;
  QRegExp startOfWORD("\\s\\S");

  while ( !found ) {
    c = startOfWORD.indexIn( line, c );

    if ( c == -1 ) {
      if ( onlyCurrentLine ) {
          return Cursor( l, c );
      } else if ( l >= doc()->lines()-1 ) {
        c = line.length()-1;
        break;
      } else {
        c = 0;
        l++;

        line = getLine( l );

        if ( line.length() == 0 || !line.at( c ).isSpace() ) {
          found = true;
        }

        continue;
      }
    } else {
      c++;
      found = true;
    }
  }

  return Cursor( l, c );
}

Cursor KateViModeBase::findPrevWordEnd( int fromLine, int fromColumn, bool onlyCurrentLine ) const
{
  QString line = getLine( fromLine );

  QString endOfWordPattern = "\\S\\s|\\S$|\\w\\W|\\S\\b|^$";

  if ( m_extraWordCharacters.length() > 0 ) {
   endOfWordPattern.append( "|["+m_extraWordCharacters+"][^" +m_extraWordCharacters+']' );
  }

  QRegExp endOfWord( endOfWordPattern );

  int l = fromLine;
  int c = fromColumn;

  bool found = false;

  while ( !found ) {
      int c1 = endOfWord.lastIndexIn( line, c-1 );

      if ( c1 != -1 && c-1 != -1 ) {
          found = true;
          c = c1;
      } else {
          if ( onlyCurrentLine ) {
              return Cursor::invalid();
          } else if ( l > 0 ) {
              line = getLine( --l );
              c = line.length();

              continue;
          } else {
              return Cursor::invalid();
          }
      }
  }

  return Cursor( l, c );
}

Cursor KateViModeBase::findPrevWORDEnd( int fromLine, int fromColumn, bool onlyCurrentLine ) const
{
  QString line = getLine( fromLine );

  QRegExp endOfWORDPattern( "\\S\\s|\\S$|^$" );

  QRegExp endOfWORD( endOfWORDPattern );

  int l = fromLine;
  int c = fromColumn;

  bool found = false;

  while ( !found ) {
      int c1 = endOfWORD.lastIndexIn( line, c-1 );

      if ( c1 != -1 && c-1 != -1 ) {
          found = true;
          c = c1;
      } else {
          if ( onlyCurrentLine ) {
              return Cursor::invalid();
          } else if ( l > 0 ) {
              line = getLine( --l );
              c = line.length();

              continue;
          } else {
              c = 0;
              return Cursor::invalid();
          }
      }
  }

  return Cursor( l, c );
}

Cursor KateViModeBase::findPrevWordStart( int fromLine, int fromColumn, bool onlyCurrentLine ) const
{
  QString line = getLine( fromLine );

  // the start of word pattern need to take m_extraWordCharacters into account if defined
  QString startOfWordPattern("\\b(\\w");
  if ( m_extraWordCharacters.length() > 0 ) {
    startOfWordPattern.append( QLatin1String( "|[" )+m_extraWordCharacters+']' );
  }
  startOfWordPattern.append( ')' );

  QRegExp startOfWord( startOfWordPattern );    // start of a word
  QRegExp nonSpaceAfterSpace( "\\s\\S" );       // non-space right after space
  QRegExp nonWordAfterWord( "\\b(?!\\s)\\W" );  // word-boundary followed by a non-word which is not a space
  QRegExp startOfLine( "^\\S" );                // non-space at start of line

  int l = fromLine;
  int c = fromColumn;

  bool found = false;

  while ( !found ) {
    int c1 = startOfWord.lastIndexIn( line, -line.length()+c-1 );
    int c2 = nonSpaceAfterSpace.lastIndexIn( line, -line.length()+c-2 );
    int c3 = nonWordAfterWord.lastIndexIn( line, -line.length()+c-1 );
    int c4 = startOfLine.lastIndexIn( line, -line.length()+c-1 );

    if ( c1 == -1 && c2 == -1 && c3 == -1 && c4 == -1 ) {
      if ( onlyCurrentLine ) {
          return Cursor::invalid();
      } else if ( l <= 0 ) {
        return Cursor::invalid();
      } else {
        line = getLine( --l );
        c = line.length();

        if ( line.length() == 0 ) {
          c = 0;
          found = true;
        }

        continue;
      }
    }

    c2++; // the second regexp will match one character *before* the character we want to go to

    if ( c1 <= 0 )
      c1 = 0;
    if ( c2 <= 0 )
      c2 = 0;
    if ( c3 <= 0 )
      c3 = 0;
    if ( c4 <= 0 )
      c4 = 0;

    c = qMax( c1, qMax( c2, qMax( c3, c4 ) ) );

    found = true;
  }

  return Cursor( l, c );
}

Cursor KateViModeBase::findPrevWORDStart( int fromLine, int fromColumn, bool onlyCurrentLine ) const
{
  QString line = getLine( fromLine );

  QRegExp startOfWORD("\\s\\S");
  QRegExp startOfLineWORD("^\\S");

  int l = fromLine;
  int c = fromColumn;

  bool found = false;

  while ( !found ) {
    int c1 = startOfWORD.lastIndexIn( line, -line.length()+c-2 );
    int c2 = startOfLineWORD.lastIndexIn( line, -line.length()+c-1 );

    if ( c1 == -1 && c2 == -1 ) {
      if ( onlyCurrentLine ) {
          return Cursor::invalid();
      } else if ( l <= 0 ) {
        return Cursor::invalid();
      } else {
        line = getLine( --l );
        c = line.length();

        if ( line.length() == 0 ) {
          c = 0;
          found = true;
        }

        continue;
      }
    }

    c1++; // the startOfWORD pattern matches one character before the word

    c = qMax( c1, c2 );

    if ( c <= 0 )
      c = 0;

    found = true;
  }

  return Cursor( l, c );
}

Cursor KateViModeBase::findWordEnd( int fromLine, int fromColumn, bool onlyCurrentLine ) const
{
  QString line = getLine( fromLine );

  QString endOfWordPattern = "\\S\\s|\\S$|\\w\\W|\\S\\b";

  if ( m_extraWordCharacters.length() > 0 ) {
   endOfWordPattern.append( "|["+m_extraWordCharacters+"][^" +m_extraWordCharacters+']' );
  }

  QRegExp endOfWORD( endOfWordPattern );

  int l = fromLine;
  int c = fromColumn;

  bool found = false;

  while ( !found ) {
      int c1 = endOfWORD.indexIn( line, c+1 );

      if ( c1 != -1 ) {
          found = true;
          c = c1;
      } else {
          if ( onlyCurrentLine ) {
              return Cursor::invalid();
          } else if ( l >= doc()->lines()-1 ) {
              c = line.length()-1;
              return Cursor::invalid();
          } else {
              c = -1;
              line = getLine( ++l );

              continue;
          }
      }
  }

  return Cursor( l, c );
}

Cursor KateViModeBase::findWORDEnd( int fromLine, int fromColumn, bool onlyCurrentLine ) const
{
  QString line = getLine( fromLine );

  QRegExp endOfWORD( "\\S\\s|\\S$" );

  int l = fromLine;
  int c = fromColumn;

  bool found = false;

  while ( !found ) {
      int c1 = endOfWORD.indexIn( line, c+1 );

      if ( c1 != -1 ) {
          found = true;
          c = c1;
      } else {
          if ( onlyCurrentLine ) {
              return Cursor::invalid();
          } else if ( l >= doc()->lines()-1 ) {
              c = line.length()-1;
              return Cursor::invalid();
          } else {
              c = -1;
              line = getLine( ++l );

              continue;
          }
      }
  }

  return Cursor( l, c );
}

KateViRange innerRange(KateViRange range, bool inner) {
  KateViRange r = range;

  if (inner) {
    const int columnDistance = qAbs(r.startColumn - r.endColumn);
    if ((r.startLine == r.endLine) && columnDistance == 1 )
    {
      // Start and end are right next to each other; there is nothing inside them.
      return KateViRange::invalid();
    }
    r.startColumn++;
    r.endColumn--;
  }

  return r;
}

KateViRange KateViModeBase::findSurroundingQuotes( const QChar &c, bool inner ) const {
  Cursor cursor(m_view->cursorPosition());
  KateViRange r;
  r.startLine = cursor.line();
  r.endLine = cursor.line();

  QString line = doc()->line(cursor.line());


  // If cursor on the quote we should shoose the best direction.
  if (line.at(cursor.column()) == c) {

    int attribute = m_view->doc()->kateTextLine(cursor.line())->attribute(cursor.column());

    //  If at the beginning of the line - then we might search the end.
    if ( doc()->kateTextLine(cursor.line())->attribute(cursor.column() + 1) == attribute &&
         doc()->kateTextLine(cursor.line())->attribute(cursor.column() - 1) != attribute ) {
      r.startColumn = cursor.column();
      r.endColumn = line.indexOf( c, cursor.column() + 1 );

      return innerRange(r, inner);
    }

    //  If at the end of the line - then we might search the beginning.
    if ( doc()->kateTextLine(cursor.line())->attribute(cursor.column() + 1) != attribute &&
        doc()->kateTextLine(cursor.line())->attribute(cursor.column() - 1) == attribute ) {

      r.startColumn =line.lastIndexOf( c, cursor.column() - 1 ) ;
      r.endColumn = cursor.column();

      return innerRange(r, inner);

    }
    // Try to search the quote to right
    int c1 = line.indexOf(c, cursor.column() + 1);
    if ( c1 != -1 ) {
      r.startColumn = cursor.column();
      r.endColumn = c1;

      return innerRange(r, inner);
    }

    // Try to search the quote to left
    int c2 = line.lastIndexOf(c, cursor.column() - 1);
    if ( c2 != -1 ) {
      r.startColumn = c2;
      r.endColumn =  cursor.column();

      return innerRange(r, inner);
    }

    // Nothing found - give up :)
    return KateViRange::invalid();
  }


  r.startColumn = line.lastIndexOf( c, cursor.column() );
  r.endColumn = line.indexOf( c, cursor.column() );

  if ( r.startColumn == -1 || r.endColumn == -1 || r.startColumn > r.endColumn ) {
    return KateViRange::invalid();
  }

  return innerRange(r, inner);
}


KateViRange KateViModeBase::findSurroundingBrackets( const QChar &c1,
                                                     const QChar &c2,
                                                     bool inner,
                                                     const QChar &nested1,
                                                     const QChar &nested2) const
{

  Cursor cursor( m_view->cursorPosition() );

  KateViRange r( cursor.line(), cursor.column(), ViMotion::InclusiveMotion );

  // Chars should not differs. For equal chars use findSurroundingQuotes.
  Q_ASSERT( c1 != c2 );

  QStack<QChar> stack;
  int column = cursor.column();
  int line = cursor.line();
  bool should_break = false;

  // Going through the text and pushing respectively brackets to the stack.
  // Then pop it out if the stack head is the bracket under cursor.

  if (column < m_view->doc()->line(line).size() && m_view->doc()->line(line).at(column) == c2) {
    r.endLine = line;
    r.endColumn = column;
  } else {

    if ( column < m_view->doc()->line(line).size() && m_view->doc()->line(line).at(column) == c1 )
      column++;

    stack.push(c2);
    for (; line < m_view->doc()->lines() && !should_break; line++ ) {
      for (;column < m_view->doc()->line( line ).size(); column++ ) {
        QChar next_char = stack.pop();

        if (next_char != m_view->doc()->line(line).at(column))
          stack.push(next_char);

        if ( stack.isEmpty() ) {
          should_break = true;
          break;
        }

        if ( m_view->doc()->line(line).at(column) == nested1 )
          stack.push(nested2);
      }
      if (should_break)
        break;

      column = 0;
    }

    if (!should_break) {
      return KateViRange::invalid();
    }

    r.endColumn = column;
    r.endLine = line;

  }

  // The same algorythm but going from the left to right.

  line = cursor.line();
  column = cursor.column();

  if (column < m_view->doc()->line(line).size() && m_view->doc()->line(line).at(column) == c1) {
    r.startLine = line;
    r.startColumn = column;
  } else {
    if (column < m_view->doc()->line(line).size() && m_view->doc()->line(line).at(column) == c2) {
      column--;
    }

    stack.clear();
    stack.push(c1);

    should_break = false;
    for (; line >= 0 && !should_break; line-- ) {
      for (;column >= 0 &&  column < m_view->doc()->line(line).size(); column-- ) {
        QChar next_char = stack.pop();

        if (next_char != m_view->doc()->line(line).at(column)) {
          stack.push(next_char);
        }

        if ( stack.isEmpty() ) {
          should_break = true;
          break;
        }

        if ( m_view->doc()->line(line).at(column) == nested2 )
          stack.push(nested1);
      }

      if (should_break)
        break;

      column = m_view->doc()->line(line - 1).size() - 1;
    }

    if (!should_break) {
      return KateViRange::invalid();
    }

    r.startColumn = column;
    r.startLine = line;

  }

  return innerRange(r, inner);
}

KateViRange KateViModeBase::findSurrounding( const QRegExp &c1, const QRegExp &c2, bool inner ) const
{
  Cursor cursor( m_view->cursorPosition() );
  QString line = getLine();

  int col1 = line.lastIndexOf( c1, cursor.column() );
  int col2 = line.indexOf( c2, cursor.column() );

  KateViRange r( cursor.line(), col1, cursor.line(), col2, ViMotion::InclusiveMotion );

  if ( col1 == -1 || col2 == -1 || col1 > col2 ) {
    return KateViRange::invalid();
  }

  if ( inner ) {
      r.startColumn++;
      r.endColumn--;
  }

  return r;
}

int KateViModeBase::findLineStartingWitchChar( const QChar &c, unsigned int count, bool forward ) const
{
  int line = m_view->cursorPosition().line();
  int lines = doc()->lines();
  unsigned int hits = 0;

  if ( forward ) {
    line++;
  } else {
    line--;
  }

  while ( line < lines && line >= 0 && hits < count ) {
    QString l = getLine( line );
    if ( l.length() > 0 && l.at( 0 ) == c ) {
      hits++;
    }
    if ( hits != count ) {
      if ( forward ) {
        line++;
      } else {
        line--;
      }
    }
  }

  if ( hits == getCount() ) {
    return line;
  }

  return -1;
}

void KateViModeBase::updateCursor( const Cursor &c ) const
{
  m_viewInternal->updateCursor( c );
}

/**
 * @return the register given for the command. If no register was given, defaultReg is returned.
 */
QChar KateViModeBase::getChosenRegister( const QChar &defaultReg ) const
{
  QChar reg = ( m_register != QChar::Null ) ? m_register : defaultReg;

  return reg;
}

QString KateViModeBase::getRegisterContent( const QChar &reg )
{
  QString r = KateGlobal::self()->viInputModeGlobal()->getRegisterContent( reg );

  if ( r.isNull() ) {
    error( i18n( "Nothing in register %1", reg ));
  }

  return r;
}

OperationMode KateViModeBase::getRegisterFlag( const QChar &reg ) const
{
  return KateGlobal::self()->viInputModeGlobal()->getRegisterFlag( reg );
}

void KateViModeBase::fillRegister( const QChar &reg, const QString &text, OperationMode flag )
{
  KateGlobal::self()->viInputModeGlobal()->fillRegister( reg, text, flag );
}


void KateViModeBase::addJump(KTextEditor::Cursor cursor)
{
  m_viInputModeManager->addJump(cursor);
}

KTextEditor::Cursor KateViModeBase::getNextJump(KTextEditor::Cursor cursor)
{
  return m_viInputModeManager->getNextJump(cursor);
}

KTextEditor::Cursor KateViModeBase::getPrevJump(KTextEditor::Cursor cursor)
{
  return m_viInputModeManager->getPrevJump(cursor);
}

KateViRange KateViModeBase::goLineDown()
{
  return goLineUpDown( getCount() );
}

KateViRange KateViModeBase::goLineUp()
{
  return goLineUpDown( -getCount() );
}

/**
 * method for moving up or down one or more lines
 * note: the sticky column is always a virtual column
 */
KateViRange KateViModeBase::goLineUpDown( int lines )
{
  Cursor c( m_view->cursorPosition() );
  KateViRange r( c.line(), c.column(), ViMotion::InclusiveMotion );
  int tabstop = doc()->config()->tabWidth();

  // if in an empty document, just return
  if ( lines == 0 ) {
    return r;
  }

  r.endLine += lines;

  // limit end line to be from line 0 through the last line
  if ( r.endLine < 0 ) {
    r.endLine = 0;
  } else if ( r.endLine > doc()->lines()-1 ) {
    r.endLine = doc()->lines()-1;
  }

  Kate::TextLine startLine = doc()->plainKateTextLine( c.line() );
  Kate::TextLine endLine = doc()->plainKateTextLine( r.endLine );

  int endLineLen = doc()->lineLength( r.endLine )-1;

  if ( endLineLen < 0 ) {
    endLineLen = 0;
  }

  int endLineLenVirt = endLine->toVirtualColumn(endLineLen, tabstop);
  int virtColumnStart = startLine->toVirtualColumn(c.column(), tabstop);

  // if sticky column isn't set, set end column and set sticky column to its virtual column
  if ( m_stickyColumn == -1 ) {
    r.endColumn = endLine->fromVirtualColumn( virtColumnStart, tabstop );
    m_stickyColumn = virtColumnStart;
  } else {
    // sticky is set - set end column to its value
    r.endColumn = endLine->fromVirtualColumn( m_stickyColumn, tabstop );
  }

  // make sure end column won't be after the last column of a line
  if ( r.endColumn > endLineLen ) {
    r.endColumn = endLineLen;
  }

  // if we move to a line shorter than the current column, go to its end
  if ( virtColumnStart > endLineLenVirt ) {
    r.endColumn = endLineLen;
  }

  return r;
}

KateViRange KateViModeBase::goVisualLineUpDown(int lines) {

  Cursor c( m_view->cursorPosition() );
  KateViRange r( c.line(), c.column(), ViMotion::InclusiveMotion );
  int tabstop = doc()->config()->tabWidth();

  if ( lines == 0 ) {
    // We're not moving anywhere.
    return r;
  }

  // Work out the real and visual line pair of the beginning of the visual line we'd end up
  // on by moving lines visual lines.  We ignore the column, for now.
  int finishVisualLine = m_viewInternal->cache()->viewLine(m_view->cursorPosition());
  int finishRealLine = m_view->cursorPosition().line();
  int count = qAbs(lines);
  bool invalidPos = false;
  if (lines > 0)
  {
    // Find the beginning of the visual line "lines" visual lines down.
    while (count > 0)
    {
      finishVisualLine++;
      if (finishVisualLine >= m_viewInternal->cache()->line(finishRealLine)->viewLineCount())
      {
        finishRealLine++;
        finishVisualLine = 0;
      }
      if (finishRealLine >= doc()->lines())
      {
        invalidPos = true;
        break;
      }
      count--;
    }
  }
  else
  {
    // Find the beginning of the visual line "lines" visual lines up.
    while (count > 0)
    {
      finishVisualLine--;
      if (finishVisualLine < 0)
      {
        finishRealLine--;
        if (finishRealLine < 0)
        {
          invalidPos = true;
          break;
        }
        finishVisualLine = m_viewInternal->cache()->line(finishRealLine)->viewLineCount() - 1;
      }
      count--;
    }
  }
  if (invalidPos)
  {
    r.endLine = -1;
    r.endColumn = -1;
    return r;
  }

  // We know the final (real) line ...
  r.endLine = finishRealLine;
  // ... now work out the final (real) column.

  if ( m_stickyColumn == -1 || !m_lastMotionWasVisualLineUpOrDown) {
    // Compute new sticky column. It is a *visual* sticky column.
    int startVisualLine = m_viewInternal->cache()->viewLine(m_view->cursorPosition());
    int startRealLine = m_view->cursorPosition().line();
    const Kate::TextLine startLine = doc()->plainKateTextLine( c.line() );
    // Adjust for the fact that if the portion of the line before wrapping is indented,
    // the continuations are also "invisibly" (i.e. without any spaces in the text itself) indented.
    const bool isWrappedContinuation = (m_viewInternal->cache()->textLayout(startRealLine, startVisualLine).lineLayout().lineNumber() != 0);
    const int numInvisibleIndentChars = isWrappedContinuation ? startLine->toVirtualColumn(m_viewInternal->cache()->line(startRealLine)->textLine()->nextNonSpaceChar(0), tabstop) : 0;

    const int realLineStartColumn = m_viewInternal->cache()->textLayout(startRealLine, startVisualLine).startCol();
    const int lineStartVirtualColumn = startLine->toVirtualColumn( realLineStartColumn, tabstop );
    const int visualColumnNoInvisibleIndent  = startLine->toVirtualColumn(c.column(), tabstop) - lineStartVirtualColumn;
    m_stickyColumn = visualColumnNoInvisibleIndent + numInvisibleIndentChars;
    Q_ASSERT(m_stickyColumn >= 0);
  }

  // The "real" (non-virtual) beginning of the current "line", which might be a wrapped continuation of a
  // "real" line.
  const int realLineStartColumn = m_viewInternal->cache()->textLayout(finishRealLine, finishVisualLine).startCol();
  const Kate::TextLine endLine = doc()->plainKateTextLine( r.endLine );
  // Adjust for the fact that if the portion of the line before wrapping is indented,
  // the continuations are also "invisibly" (i.e. without any spaces in the text itself) indented.
  const bool isWrappedContinuation = (m_viewInternal->cache()->textLayout(finishRealLine, finishVisualLine).lineLayout().lineNumber() != 0);
  const int numInvisibleIndentChars = isWrappedContinuation ? endLine->toVirtualColumn(m_viewInternal->cache()->line(finishRealLine)->textLine()->nextNonSpaceChar(0), tabstop) : 0;
  if (m_stickyColumn == (unsigned int)KateVi::EOL)
  {
    const int visualEndColumn = m_viewInternal->cache()->textLayout(finishRealLine, finishVisualLine).lineLayout().textLength() - 1;
    r.endColumn = endLine->fromVirtualColumn( visualEndColumn + realLineStartColumn - numInvisibleIndentChars, tabstop );
  }
  else
  {
    // Algorithm: find the "real" column corresponding to the start of the line.  Offset from that
    // until the "visual" column is equal to the "visual" sticky column.
    int realOffsetToVisualStickyColumn = 0;
    const int lineStartVirtualColumn = endLine->toVirtualColumn( realLineStartColumn, tabstop );
    while (true)
    {
      const int visualColumn = endLine->toVirtualColumn( realLineStartColumn + realOffsetToVisualStickyColumn, tabstop ) - lineStartVirtualColumn + numInvisibleIndentChars;
      if (visualColumn >= m_stickyColumn)
      {
        break;
      }
      realOffsetToVisualStickyColumn++;
    }
    r.endColumn = realLineStartColumn + realOffsetToVisualStickyColumn;
  }
  m_currentMotionWasVisualLineUpOrDown = true;

  return r;
}


bool KateViModeBase::startNormalMode()
{
  /* store the key presses for this "insert mode session" so that it can be repeated with the
   * '.' command
   * - ignore transition from Visual Modes
   */
  if (!( m_viInputModeManager->isAnyVisualMode() || m_viInputModeManager->isReplayingLastChange() ))
  {
    m_viInputModeManager->storeLastChangeCommand();
    m_viInputModeManager->clearCurrentChangeLog();
  }

  m_viInputModeManager->viEnterNormalMode();
  m_view->doc()->setUndoMergeAllEdits(false);
  m_view->updateViModeBarMode();

  return true;
}

bool KateViModeBase::startInsertMode()
{
  m_viInputModeManager->viEnterInsertMode();
  m_view->doc()->setUndoMergeAllEdits(true);
  m_view->updateViModeBarMode();

  return true;
}

bool KateViModeBase::startReplaceMode()
{
  m_view->doc()->setUndoMergeAllEdits(true);
  m_viInputModeManager->viEnterReplaceMode();
  m_view->updateViModeBarMode();

  return true;
}

bool KateViModeBase::startVisualMode()
{
  if ( m_view->getCurrentViMode() == VisualLineMode ) {
    m_viInputModeManager->getViVisualMode()->setVisualModeType( VisualMode );
    m_viInputModeManager->changeViMode(VisualMode);
  } else if (m_view->getCurrentViMode() == VisualBlockMode ) {
    m_viInputModeManager->getViVisualMode()->setVisualModeType( VisualMode );
    m_viInputModeManager->changeViMode(VisualMode);
  } else {
    m_viInputModeManager->viEnterVisualMode();
  }

  m_view->updateViModeBarMode();

  return true;
}

bool KateViModeBase::startVisualBlockMode()
{
  if ( m_view->getCurrentViMode() == VisualMode ) {
    m_viInputModeManager->getViVisualMode()->setVisualModeType( VisualBlockMode );
    m_viInputModeManager->changeViMode(VisualBlockMode);
  } else {
    m_viInputModeManager->viEnterVisualMode( VisualBlockMode );
  }

  m_view->updateViModeBarMode();

  return true;
}

bool KateViModeBase::startVisualLineMode()
{
  if ( m_view->getCurrentViMode() == VisualMode ) {
    m_viInputModeManager->getViVisualMode()->setVisualModeType( VisualLineMode );
    m_viInputModeManager->changeViMode(VisualLineMode);
  } else {
    m_viInputModeManager->viEnterVisualMode( VisualLineMode );
  }

  m_view->updateViModeBarMode();

  return true;
}

void KateViModeBase::error( const QString &errorMsg )
{
  delete m_infoMessage;

  // nop if no vi mode around
  if (!m_view->viInputMode())
    return;

  m_infoMessage = new KTextEditor::Message(errorMsg, KTextEditor::Message::Error);
  m_infoMessage->setPosition(KTextEditor::Message::BottomInView);
  m_infoMessage->setAutoHide(2000); // 2 seconds
  m_infoMessage->setView(m_view);

  m_view->doc()->postMessage(m_infoMessage);
}

void KateViModeBase::message( const QString &msg )
{
  delete m_infoMessage;

  // nop if no vi mode around
  if (!m_view->viInputMode())
    return;

  m_infoMessage = new KTextEditor::Message(msg, KTextEditor::Message::Positive);
  m_infoMessage->setPosition(KTextEditor::Message::BottomInView);
  m_infoMessage->setAutoHide(2000); // 2 seconds
  m_infoMessage->setView(m_view);

  m_view->doc()->postMessage(m_infoMessage);
}

QString KateViModeBase::getVerbatimKeys() const
{
  return m_keysVerbatim;
}

const QChar KateViModeBase::getCharAtVirtualColumn( QString &line, int virtualColumn,
    int tabWidth ) const
{
  int column = 0;
  int tempCol = 0;

  // sanity check: if the line is empty, there are no chars
  if ( line.length() == 0 ) {
    return QChar::Null;
  }

  while ( tempCol < virtualColumn ) {
    if ( line.at( column ) == '\t' ) {
      tempCol += tabWidth - ( tempCol % tabWidth );
    } else {
      tempCol++;
    }

    if ( tempCol <= virtualColumn ) {
      column++;

      if ( column >= line.length() ) {
        return QChar::Null;
      }
    }
  }

  if ( line.length() > column )
    return line.at( column );

  return QChar::Null;
}

void KateViModeBase::addToNumberUnderCursor( int count )
{
    Cursor c( m_view->cursorPosition() );
    QString line = getLine();

    if (line.isEmpty())
    {
      return;
    }

    int numberStartPos = -1;
    QString numberAsString;
    QRegExp numberRegex( "(0x)([0-9a-fA-F]+)|\\-?\\d+" );
    const int cursorColumn = c.column();
    const int currentLineLength = doc()->lineLength(c.line());
    const Cursor prevWordStart = findPrevWordStart(c.line(), cursorColumn);
    int wordStartPos = prevWordStart.column();
    if (prevWordStart.line() < c.line())
    {
      // The previous word starts on the previous line: ignore.
      wordStartPos = 0;
    }
    if (wordStartPos > 0 && line.at(wordStartPos - 1) == '-') wordStartPos--;
    for (int searchFromColumn = wordStartPos; searchFromColumn < currentLineLength; searchFromColumn++)
    {
      numberStartPos = numberRegex.indexIn( line, searchFromColumn );

      numberAsString = numberRegex.cap();

      const bool numberEndedBeforeCursor = (numberStartPos + numberAsString.length() <= c.column());
      if (!numberEndedBeforeCursor)
      {
        // This is the first number-like string under or after the cursor - this'll do!
        break;
      }
    }

    if (numberStartPos == -1)
    {
      // None found.
      return;
    }

    bool parsedNumberSuccessfully = false;
    int base = numberRegex.cap( 1 ).isEmpty() ? 10 : 16;
    if (base != 16 && numberAsString.startsWith("0") && numberAsString.length() > 1)
    {
      // If a non-hex number with a leading 0 can be parsed as octal, then assume
      // it is octal.
      numberAsString.toInt( &parsedNumberSuccessfully, 8 );
      if (parsedNumberSuccessfully)
      {
        base = 8;
      }
    }
    const int originalNumber = numberAsString.toInt( &parsedNumberSuccessfully, base );

    kDebug( 13070 ) << "base: " << base;
    kDebug( 13070 ) << "n: " << originalNumber;

    if ( !parsedNumberSuccessfully ) {
        // conversion to int failed. give up.
        return;
    }

    QString basePrefix;
    if (base == 16)
    {
      basePrefix = "0x";
    }
    else if (base == 8)
    {
      basePrefix = "0";
    }
    const QString withoutBase = numberAsString.mid(basePrefix.length());

    const int newNumber = originalNumber + count;

    // Create the new text string to be inserted. Prepend with “0x” if in base 16, and "0" if base 8.
    // For non-decimal numbers, try to keep the length of the number the same (including leading 0's).
    const QString newNumberPadded = (base == 10) ?
        QString("%1").arg(newNumber, 0, base) :
        QString("%1").arg(newNumber, withoutBase.length(), base, QChar('0'));
    const QString newNumberText = basePrefix + newNumberPadded;

    // Replace the old number string with the new.
    doc()->editStart();
    doc()->removeText( KTextEditor::Range( c.line(), numberStartPos , c.line(), numberStartPos+numberAsString.length() ) );
    doc()->insertText( KTextEditor::Cursor( c.line(), numberStartPos ), newNumberText );
    doc()->editEnd();
    updateCursor(Cursor(m_view->cursorPosition().line(), numberStartPos + newNumberText.length() - 1));
}

void KateViModeBase::switchView(Direction direction) {

  QList<KateView*> visible_views;
  foreach (KateView* view,  KateGlobal::self()->views() ) {
      if (view->isVisible())
        visible_views.push_back(view);
  }

  QPoint current_point = m_view->mapToGlobal(m_view->pos());
  int curr_x1 = current_point.x();
  int curr_x2 = current_point.x() + m_view->width();
  int curr_y1 = current_point.y();
  int curr_y2 = current_point.y() + m_view->height();
  int curr_cursor_y = m_view->mapToGlobal(m_view->cursorToCoordinate(m_view->cursorPosition())).y();
  int curr_cursor_x = m_view->mapToGlobal(m_view->cursorToCoordinate(m_view->cursorPosition())).x();

  KateView *bestview = NULL;
  int  best_x1 = -1, best_x2 = -1, best_y1 = -1, best_y2 = -1, best_center_y = -1, best_center_x = -1;

  if (direction == Next && visible_views.count() != 1) {
    for (int i=0; i< visible_views.count(); i++) {
        if (visible_views.at(i) == m_view) {
          if (i != visible_views.count() - 1 )
            bestview = visible_views.at(i + 1);
          else
            bestview = visible_views.at(0);
        }
      }
  } else {
    foreach (KateView* view, visible_views ) {
      QPoint point = view->mapToGlobal(view->pos());
      int x1 = point.x();
      int x2 = point.x() + view->width();
      int y1 = point.y();
      int y2 = point.y() + m_view->height();
      int  center_y = (y1 + y2) / 2;
      int  center_x = (x1 + x2) / 2;

      switch (direction) {
        case Left:
          if (view != m_view && x2 <= curr_x1 &&
              ( x2 > best_x2 ||
                (x2 == best_x2 && qAbs(curr_cursor_y - center_y) < qAbs(curr_cursor_y - best_center_y)) ||
                bestview == NULL)){
            bestview = view;
            best_x2 = x2;
            best_center_y = center_y;
          }
          break;
        case Right:
          if (view != m_view && x1 >= curr_x2 &&
              ( x1 < best_x1 ||
                (x1 == best_x1 && qAbs(curr_cursor_y - center_y) < qAbs(curr_cursor_y - best_center_y)) ||
                bestview == NULL)){
            bestview = view;
            best_x1 = x1;
            best_center_y = center_y;
          }
          break;
        case Down:
          if (view != m_view && y1 >= curr_y2 &&
              ( y1 < best_y1 ||
                (y1 == best_y1 && qAbs(curr_cursor_x - center_x) < qAbs(curr_cursor_x - best_center_x)) ||
                bestview == NULL)){
            bestview = view;
            best_y1 = y1;
            best_center_x = center_x;
          }
        case Up:
          if (view != m_view && y2 <= curr_y1 &&
              ( y2 > best_y2 ||
                (y2 == best_y2 && qAbs(curr_cursor_x - center_x) < qAbs(curr_cursor_x - best_center_x)) ||
                bestview == NULL)){
            bestview = view;
            best_y2 = y2;
            best_center_x = center_x;
          }
          break;
        default:
          return;
      }

    }
  }
  if (bestview != NULL ) {
    bestview->setFocus();
    KateViewConfig::global()->setViInputMode(true);
  }
}
