/* This file is part of the KDE libraries
 * Copyright (C) 2008 Erlend Hamberg <ehamberg@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) version 3.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "katevimodebase.h"
#include "katevirange.h"
#include "kateglobal.h"
#include "kateviglobal.h"
#include "katevivisualmode.h"
#include "katevinormalmode.h"

#include <QString>
#include <QRegExp>
#include "kateview.h"
#include "kateviewinternal.h"
#include "katedocument.h"

////////////////////////////////////////////////////////////////////////////////
// HELPER METHODS
////////////////////////////////////////////////////////////////////////////////

bool KateViModeBase::deleteRange( KateViRange &r, bool linewise, bool addToRegister)
{
  r.normalize();
  bool res = false;
  QString removedText = getRange( r, linewise );

  if ( linewise ) {
    for ( int i = 0; i < m_commandRange.endLine-m_commandRange.startLine+1; i++ ) {
      res = m_view->doc()->removeLine( r.startLine );
    }
  } else {
      res = m_view->doc()->removeText( KTextEditor::Range( r.startLine, r.startColumn, r.endLine, r.endColumn) );
  }

  if ( addToRegister ) {
    if ( r.startLine == r.endLine ) {
      fillRegister( getChosenRegister( '-' ), removedText );
    } else {
      fillRegister( getChosenRegister( '0' ), removedText );
    }
  }

  return res;
}

const QString KateViModeBase::getRange( KateViRange &r, bool linewise) const
{
  r.normalize();
  QString s;

  if ( linewise ) {
    r.startColumn = 0;
    r.endColumn = getLine( r.endLine ).length();
  }

  if ( r.motionType == ViMotion::InclusiveMotion ) {
    r.endColumn++;
  }

  KTextEditor::Range range( r.startLine, r.startColumn, r.endLine, r.endColumn);

  if ( linewise ) {
    s = m_view->doc()->textLines( range ).join( QChar( '\n' ) );
    s.append( QChar( '\n' ) );
  } else {
      s = m_view->doc()->text( range );
  }

  return s;
}

QString KateViModeBase::getLine( int lineNumber ) const
{
  QString line;

  if ( lineNumber == -1 ) {
    KTextEditor::Cursor cursor ( m_view->cursorPosition() );
    line = m_view->currentTextLine();
  } else {
    line = m_view->doc()->line( lineNumber );
  }

  return line;
}

KTextEditor::Cursor KateViModeBase::findNextWordStart( int fromLine, int fromColumn, bool onlyCurrentLine ) const
{
  QString line = getLine( fromLine );

  // the start of word pattern need to take m_extraWordCharacters into account if defined
  QString startOfWordPattern("\\b(\\w");
  if ( m_extraWordCharacters.length() > 0 ) {
    startOfWordPattern.append( "|["+m_extraWordCharacters+']' );
  }
  startOfWordPattern.append( ")" );

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
            return KTextEditor::Cursor( l, c );
        } else if ( l >= m_view->doc()->lines()-1 ) {
            c = line.length()-1;
            return KTextEditor::Cursor( l, c );
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

  return KTextEditor::Cursor( l, c );
}

KTextEditor::Cursor KateViModeBase::findNextWORDStart( int fromLine, int fromColumn, bool onlyCurrentLine ) const
{
  KTextEditor::Cursor cursor ( m_view->cursorPosition() );
  QString line = getLine();
  KateViRange r( cursor.line(), cursor.column(), ViMotion::ExclusiveMotion );

  int l = fromLine;
  int c = fromColumn;

  bool found = false;
  QRegExp startOfWORD("\\s\\S");

  while ( !found ) {
    c = startOfWORD.indexIn( line, c+1 );

    if ( c == -1 ) {
      if ( onlyCurrentLine ) {
          return KTextEditor::Cursor( l, c );
      } else if ( l >= m_view->doc()->lines()-1 ) {
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

  return KTextEditor::Cursor( l, c );
}

KTextEditor::Cursor KateViModeBase::findPrevWordStart( int fromLine, int fromColumn, bool onlyCurrentLine ) const
{
  QString line = getLine( fromLine );

  // the start of word pattern need to take m_extraWordCharacters into account if defined
  QString startOfWordPattern("\\b(\\w");
  if ( m_extraWordCharacters.length() > 0 ) {
    startOfWordPattern.append( "|["+m_extraWordCharacters+']' );
  }
  startOfWordPattern.append( ")" );

  QRegExp startOfWord( startOfWordPattern );    // start of a word
  QRegExp nonSpaceAfterSpace( "\\s\\S" );       // non-space right after space
  QRegExp nonWordAfterWord( "\\b(?!\\s)\\W" );  // word-boundary followed by a non-word which is not a space
  QRegExp startOfLine( "^\\S" );                  // non-space at start of line

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
          return KTextEditor::Cursor( l, c );
      } else if ( l <= 0 ) {
        return KTextEditor::Cursor( 0, 0 );
      } else {
        line = getLine( --l );
        c = line.length()-1;

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

  return KTextEditor::Cursor( l, c );
}

KTextEditor::Cursor KateViModeBase::findPrevWORDStart( int fromLine, int fromColumn, bool onlyCurrentLine ) const
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
          return KTextEditor::Cursor( l, c );
      } else if ( l <= 0 ) {
        return KTextEditor::Cursor( 0, 0 );
      } else {
        line = getLine( --l );
        c = line.length()-1;

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

  return KTextEditor::Cursor( l, c );
}

KTextEditor::Cursor KateViModeBase::findWordEnd( int fromLine, int fromColumn, bool onlyCurrentLine ) const
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
              return KTextEditor::Cursor( l, c );
          } else if ( l >= m_view->doc()->lines()-1 ) {
              c = line.length()-1;
              return KTextEditor::Cursor( l, c );
          } else {
              c = -1;
              line = getLine( ++l );

              continue;
          }
      }
  }

  return KTextEditor::Cursor( l, c );
}

KTextEditor::Cursor KateViModeBase::findWORDEnd( int fromLine, int fromColumn, bool onlyCurrentLine ) const
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
              return KTextEditor::Cursor( l, c );
          } else if ( l >= m_view->doc()->lines()-1 ) {
              c = line.length()-1;
              return KTextEditor::Cursor( l, c );
          } else {
              c = -1;
              line = getLine( ++l );

              continue;
          }
      }
  }

  return KTextEditor::Cursor( l, c );
}

// FIXME: i" won't work if the cursor is on one of the chars
KateViRange KateViModeBase::findSurrounding( const QChar &c1, const QChar &c2, bool inner )
{
  KTextEditor::Cursor cursor( m_view->cursorPosition() );
  QString line = getLine();

  int col1 = line.lastIndexOf( c1, cursor.column() );
  int col2 = line.indexOf( c2, cursor.column() );

  KateViRange r( cursor.line(), col1, cursor.line(), col2, ViMotion::InclusiveMotion );

  if ( col1 == -1 || col2 == -1 || col1 > col2 ) {
      r.valid = false;
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
  int lines = m_view->doc()->lines();
  unsigned int hits = 0;

  if ( forward ) {
    line++;
  } else {
    line--;
  }

  while ( line < lines && line > 0 && hits < count ) {
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

void KateViModeBase::updateCursor( const KTextEditor::Cursor &c ) const
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

QString KateViModeBase::getRegisterContent( const QChar &reg ) const
{
  return KateGlobal::self()->viInputModeGlobal()->getRegisterContent( reg );
}

void KateViModeBase::fillRegister( const QChar &reg, const QString &text )
{
    KateGlobal::self()->viInputModeGlobal()->fillRegister( reg, text );
}

bool KateViModeBase::startInsertMode()
{
  //m_view->changeViMode( InsertMode );
  m_viewInternal->repaint ();

  emit m_view->viewModeChanged( m_view );

  return true;
}

bool KateViModeBase::startVisualMode()
{
  //F//if ( m_view->getCurrentViMode() == VisualLineMode ) {
  //F//  m_viewInternal->getViVisualMode()->setVisualLine( false );
  //F//  m_view->changeViMode(VisualMode);
  //F//} else {
    m_view->viEnterVisualMode();
  //F//}

  emit m_view->viewModeChanged( m_view );

  return true;
}

bool KateViModeBase::startVisualLineMode()
{
  //F//if ( m_view->getCurrentViMode() == VisualMode ) {
  //F//  m_viewInternal->getViVisualMode()->setVisualLine( true );
  //F//  m_view->changeViMode(VisualLineMode);
  //F//} else {
    m_view->viEnterVisualMode( true );
  //F//}

  emit m_view->viewModeChanged( m_view );

  return true;
}

KateViVisualMode* KateViModeBase::getViVisualMode()
{
  return NULL;//m_viewInternal->getViVisualMode();
}

KateViNormalMode* KateViModeBase::getViNormalMode()
{
  return NULL;//m_viewInternal->getViNormalMode();
}
