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

#include "katevinormalmode.h"
#include "katevivisualmode.h"
#include "katesmartmanager.h"
#include "kateglobal.h"
#include "kateviglobal.h"
#include "katesmartrange.h"
#include <QApplication>
#include <QList>

KateViNormalMode::KateViNormalMode( KateView * view, KateViewInternal * viewInternal )
{
  m_view = view;
  m_viewInternal = viewInternal;
  m_stickyColumn = -1;

  // FIXME: make configurable:
  m_extraWordCharacters = "";
  m_matchingItems["/*"] = "*/";
  m_matchingItems["*/"] = "-/*";

  m_matchItemRegex = generateMatchingItemRegex();

  m_defaultRegister = '"';
  m_marks = new QMap<QChar, KTextEditor::SmartCursor*>;
  m_keyParser = new KateViKeySequenceParser();

  initializeCommands();
  resetParser(); // initialise with start configuration
}

KateViNormalMode::~KateViNormalMode()
{
  delete m_marks;
  delete m_keyParser;
}

/**
 * parses a key stroke to check if it's a valid (part of) a command
 * @return true if a command was completed and executed, false otherwise
 */
bool KateViNormalMode::handleKeypress( QKeyEvent *e )
{
  int keyCode = e->key();
  QString text = e->text();

  // ignore modifier keys alone
  if ( keyCode == Qt::Key_Shift || keyCode == Qt::Key_Control
      || keyCode == Qt::Key_Alt || keyCode == Qt::Key_Meta ) {
    return false;
  }

  if ( keyCode == Qt::Key_Escape ) {
    reset();
    return true;
  }

  int mods = e->modifiers();
  QChar key;

  // special keys
  if ( text.isEmpty() || ( text.length() ==1 && text.at(0) < 0x20 )
      || ( mods != Qt::NoModifier && mods != Qt::ShiftModifier ) ) {
    QString keyPress;

    keyPress.append( '<' );
    keyPress.append( ( mods & Qt::ShiftModifier ) ? "s-" : "" );
    keyPress.append( ( mods & Qt::ControlModifier ) ? "c-" : "" );
    keyPress.append( ( mods & Qt::AltModifier ) ? "a-" : "" );
    keyPress.append( ( mods & Qt::MetaModifier ) ? "m-" : "" );
    keyPress.append( keyCode <= 0xFF ? QChar( keyCode ) : m_keyParser->qt2vi( keyCode ) );
    //keyPress.append( m_keyParser->qt2vi( keyCode ) );
    keyPress.append( '>' );

    key = m_keyParser->encodeKeySequence( keyPress ).at( 0 );
  }
  else {
    key = text.at( 0 );
    kDebug( 13070 ) << key << "(" << keyCode << ")";
  }

  m_keysVerbatim.append( m_keyParser->decodeKeySequence( key ) );
  kDebug( 13070 ) << " *** Command so far: " << m_keysVerbatim << " ***";

  QChar c = QChar::Null;
  if ( m_keys.size() > 0 ) {
    c = m_keys.at( m_keys.size()-1 ); // last char
  }

  if ( ( keyCode >= Qt::Key_0 && keyCode <= Qt::Key_9 && c != '"' )     // key 0-9
      && ( m_countTemp != 0 || keyCode != Qt::Key_0 )                   // the first digit can't be 0
      && ( c != 'f' && c != 't' && c != 'F' && c != 'T' && c != 'r' ) ){// "find character" motions

    m_countTemp *= 10;
    m_countTemp += keyCode-Qt::Key_0;

    return true;
  } else if ( m_countTemp != 0 ) {
    m_count = getCount() * m_countTemp;
    m_countTemp = 0;

    kDebug( 13070 ) << "count = " << getCount();
  }

  m_keys.append( key );

  if ( m_keys[ 0 ] == Qt::Key_QuoteDbl ) {
    if ( m_keys.size() < 2 ) {
      return true; // waiting for a register
    }
    else {
      QChar r = m_keys[ 1 ].toLower();

      if ( ( r >= '0' && r <= '9' ) || ( r >= 'a' && r <= 'z' ) || r == '_' || r == '+' || r == '*' ) {
        m_register = r;
        kDebug( 13070 ) << "Register set to " << r;
        m_keys.clear();
        return true;
      }
      else {
        resetParser();
        return true;
      }
    }
  }


  // if we have any matching commands so far, check which ones still match
  if ( m_matchingCommands.size() > 0 ) {
    int n = m_matchingCommands.size()-1;

    // remove commands not matching anymore
    for ( int i = n; i >= 0; i-- ) {
      if ( !m_commands.at( m_matchingCommands.at( i ) )->matches( m_keys ) ) {
        kDebug( 13070 ) << "removing " << m_commands.at( m_matchingCommands.at( i ) )->pattern() << ", size before remove is " << m_matchingCommands.size();
        if ( m_commands.at( m_matchingCommands.at( i ) )->needsMotionOrTextObject() ) {
          // "cache" command needing a motion for later
          kDebug( 13070 ) << "m_motionOperatorIndex set to " << m_motionOperatorIndex;
          m_motionOperatorIndex = m_matchingCommands.at( i );
        }
        m_matchingCommands.remove( i );
      }
    }

    // check if any of the matching commands need a motion/text object, if so
    // push the current command length to m_awaitingMotionOrTextObject so one
    // knows where to split the command between the operator and the motion
    for ( int i = 0; i < m_matchingCommands.size(); i++ ) {
      if ( m_commands.at( m_matchingCommands.at( i ) )->needsMotionOrTextObject() ) {
        m_awaitingMotionOrTextObject.push( m_keys.size() );
        break;
      }
    }
  } else {
    // go through all registered commands and put possible matches in m_matchingCommands
    for ( int i = 0; i < m_commands.size(); i++ ) {
      if ( m_commands.at( i )->matches( m_keys ) ) {
        m_matchingCommands.push_back( i );
        if ( m_commands.at( i )->needsMotionOrTextObject() && m_commands.at( i )->pattern().length() == m_keys.size() ) {
          m_awaitingMotionOrTextObject.push( m_keys.size() );
        }
      }
    }
  }

  // this indicates where in the command string one should start looking for a motion command
  int checkFrom = ( m_awaitingMotionOrTextObject.isEmpty() ? 0 : m_awaitingMotionOrTextObject.top() );

  kDebug( 13070 ) << "checkFrom: " << checkFrom;

  // look for matching motion commands from position 'checkFrom'
  // FIXME: if checkFrom hasn't changed, only motions whose index is in
  // m_matchingMotions shold be checked
  if ( checkFrom < m_keys.size() ) {
    for ( int i = 0; i < m_motions.size(); i++ ) {
      //kDebug( 13070 )  << "\tchecking " << m_keys.mid( checkFrom )  << " against " << m_motions.at( i )->pattern();
      if ( m_motions.at( i )->matches( m_keys.mid( checkFrom ) ) ) {
        kDebug( 13070 )  << m_keys.mid( checkFrom ) << " matches!";
        m_matchingMotions.push_back( i );

        // if it matches exact, we have found the motion command to execute
        if ( m_motions.at( i )->matchesExact( m_keys.mid( checkFrom ) ) ) {
          if ( checkFrom == 0 ) {
            // no command given before motion, just move the cursor to wherever
            // the motion says it should go to
            KateViRange r = m_motions.at( i )->execute();

            if ( r.valid && r.endLine >= 0 && r.endColumn >= 0 ) {
              kDebug( 13070 ) << "no command given, going to position (" << r.endLine << "," << r.endColumn << ")";
              goToPos( r );
            } else {
              kDebug( 13070 ) << "invalid position: (" << r.endLine << "," << r.endColumn << ")";
            }

            resetParser();
            return true;
          } else {
            // execute the specified command and supply the position returned from
            // the motion
            m_commandRange = m_motions.at( i )->execute();

            if ( m_commandRange.valid ) {
              kDebug( 13070 ) << "Run command" << m_commands.at( m_motionOperatorIndex )->pattern() << "to position (" << m_commandRange.endLine << "," << m_commandRange.endColumn << ")";
              m_commands.at( m_motionOperatorIndex )->execute();
            } else {
              kDebug( 13070 ) << "invalid position";
            }

            reset();
            return true;
          }
        }
      }
    }
  }

  kDebug( 13070 ) << "'" << m_keys << "' MATCHING COMMANDS: " << m_matchingCommands.size();
  kDebug( 13070 ) << "'" << m_keys << "' MATCHING MOTIONS: " << m_matchingMotions.size();
  kDebug( 13070 ) << "'" << m_keys << "' AWAITING MOTION OR TO (INDEX): " << ( m_awaitingMotionOrTextObject.isEmpty() ? 0 : m_awaitingMotionOrTextObject.top() );

  // if we have only one match, check if it is a perfect match and if so, execute it
  // if it's not waiting for a motion or a text object
  if ( m_matchingCommands.size() == 1 ) {
    if ( m_commands.at( m_matchingCommands.at( 0 ) )->matchesExact( m_keys )
        && !m_commands.at( m_matchingCommands.at( 0 ) )->needsMotionOrTextObject() ) {
      kDebug( 13070 ) << "Running command at index " << m_matchingCommands.at( 0 );
      m_commands.at( m_matchingCommands.at( 0 ) )->execute();

      // check if reset() should be called. some commands in visual mode should not end visual mode
      if ( m_commands.at( m_matchingCommands.at( 0 ) )->shouldReset() ) {
        reset();
      }
      resetParser();

      return true;
    }
  } else if ( m_matchingCommands.size() == 0 && m_matchingMotions.size() == 0 ) {
    //if ( m_awaitingMotionOrTextObject.size() == 0 ) {
      resetParser();
      return false;
    //} else {

    //}
  }

  m_matchingMotions.clear();
  return false;
}

/**
 * @return the register given for the command. If no register was given, defaultReg is returned.
 */
QChar KateViNormalMode::getChosenRegister( const QChar &defaultReg ) const
{
  QChar reg = ( m_register != QChar::Null ) ? m_register : defaultReg;

  return reg;
}

QString KateViNormalMode::getRegisterContent( const QChar &reg ) const
{
    return KateGlobal::self()->viInputModeGlobal()->getRegisterContent( reg );
}

void KateViNormalMode::error( const QString &errorMsg ) const
{
  kError( 13070 ) << "\033[31m" << errorMsg << "\033[0m\n";
}

void KateViNormalMode::message( const QString &msg ) const
{
  kError( 13070 ) << "\033[34m" << msg << "\033[0m\n";
}

/**
 * (re)set to start configuration. This is done when a command is completed
 * executed or when a command is aborted
 */
void KateViNormalMode::resetParser()
{
  kDebug( 13070 ) << "***RESET***";
  m_keys.clear();
  m_keysVerbatim.clear();
  m_count = 0;
  m_countTemp = 0;
  m_register = QChar::Null;
  m_findWaitingForChar = false;
  m_waitingForMotionOrTextObject = -1;
  m_matchingCommands.clear();
  m_matchingMotions.clear();
  m_awaitingMotionOrTextObject.clear();
  m_motionOperatorIndex = 0;
}

// reset the command parser
void KateViNormalMode::reset()
{
    resetParser();
    m_commandRange.startLine = -1;
    m_commandRange.startColumn = -1;
}

void KateViNormalMode::goToPos( KateViRange r )
{
  KTextEditor::Cursor cursor;
  cursor.setLine( r.endLine );
  cursor.setColumn( r.endColumn );

  if ( r.jump ) {
    addCurrentPositionToJumpList();
  }

  m_viewInternal->updateCursor( cursor );
}


void KateViNormalMode::fillRegister( const QChar &reg, const QString &text )
{
    KateGlobal::self()->viInputModeGlobal()->fillRegister( reg, text );
}

void KateViNormalMode::addCurrentPositionToJumpList()
{
    KTextEditor::Cursor c( m_view->cursorPosition() );

    KateSmartCursor *cursor = m_view->doc()->smartManager()->newSmartCursor( c );

    m_marks->insert( '\'', cursor );
}

////////////////////////////////////////////////////////////////////////////////
// HELPER METHODS
////////////////////////////////////////////////////////////////////////////////

bool KateViNormalMode::deleteRange( KateViRange &r, bool linewise, bool addToRegister)
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

const QString KateViNormalMode::getRange( KateViRange &r, bool linewise) const
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

QString KateViNormalMode::getLine( int lineNumber ) const
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

KTextEditor::Cursor KateViNormalMode::findNextWordStart( int fromLine, int fromColumn, bool onlyCurrentLine ) const
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

KTextEditor::Cursor KateViNormalMode::findNextWORDStart( int fromLine, int fromColumn, bool onlyCurrentLine ) const
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

KTextEditor::Cursor KateViNormalMode::findPrevWordStart( int fromLine, int fromColumn, bool onlyCurrentLine ) const
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

KTextEditor::Cursor KateViNormalMode::findPrevWORDStart( int fromLine, int fromColumn, bool onlyCurrentLine ) const
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

KTextEditor::Cursor KateViNormalMode::findWordEnd( int fromLine, int fromColumn, bool onlyCurrentLine ) const
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

KTextEditor::Cursor KateViNormalMode::findWORDEnd( int fromLine, int fromColumn, bool onlyCurrentLine ) const
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
KateViRange KateViNormalMode::findSurrounding( const QChar &c1, const QChar &c2, bool inner )
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

int KateViNormalMode::findLineStartingWitchChar( const QChar &c, unsigned int count, bool forward ) const
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

////////////////////////////////////////////////////////////////////////////////
// COMMANDS AND OPERATORS
////////////////////////////////////////////////////////////////////////////////

/**
 * enter insert mode at the cursor position
 */

bool KateViNormalMode::commandEnterInsertMode()
{
  m_view->changeViMode( InsertMode );
  m_viewInternal->repaint ();

  emit m_view->viewModeChanged( m_view );

  return true;
}

/**
 * enter insert mode after the current character
 */

bool KateViNormalMode::commandEnterInsertModeAppend()
{
  KTextEditor::Cursor c( m_view->cursorPosition() );
  c.setColumn( c.column()+1 );

  if ( getLine( c.line() ).length() == 0 ) {
    c.setColumn( 0 );
  }

  m_viewInternal->updateCursor( c );

  m_view->changeViMode( InsertMode );
  m_viewInternal->repaint ();

  emit m_view->viewModeChanged( m_view );
  //emit viewEditModeChanged( this,viewEditMode() );

  return false;
}

/**
 * start insert mode after the last character of the line
 */

bool KateViNormalMode::commandEnterInsertModeAppendEOL()
{
  KTextEditor::Cursor c( m_view->cursorPosition() );
  c.setColumn( m_view->doc()->lineLength( c.line() ) );
  m_viewInternal->updateCursor( c );

  m_view->changeViMode( InsertMode );
  m_viewInternal->repaint ();

  emit m_view->viewModeChanged( m_view );

  return true;
}

bool KateViNormalMode::commandEnterVisualLineMode()
{
  if ( m_view->getCurrentViMode() == VisualLineMode ) {
    reset();
  } else if ( m_view->getCurrentViMode() == VisualMode ) {
    m_viewInternal->getViVisualMode()->setVisualLine( true );
    m_view->changeViMode(VisualLineMode);
    emit m_view->viewModeChanged( m_view );
  } else {
    m_view->viEnterVisualMode( true );
  }

  return true;
}

bool KateViNormalMode::commandEnterVisualMode()
{
  if ( m_view->getCurrentViMode() == VisualMode ) {
    reset();
  } else if ( m_view->getCurrentViMode() == VisualLineMode ) {
    m_viewInternal->getViVisualMode()->setVisualLine( false );
    m_view->changeViMode(VisualMode);
    emit m_view->viewModeChanged( m_view );
  } else {
    m_view->viEnterVisualMode();
  }

  return true;
}

bool KateViNormalMode::commandToOtherEnd()
{
  if ( m_view->getCurrentViMode() == VisualLineMode || m_view->getCurrentViMode() == VisualMode ) {
    m_viewInternal->getViVisualMode()->switchStartEnd();
    return true;
  }

  return false;
}

KateViRange KateViNormalMode::motionWordForward()
{
  KTextEditor::Cursor c( m_view->cursorPosition() );
  KateViRange r( c.line(), c.column(), ViMotion::ExclusiveMotion );

  for ( unsigned int i = 0; i < getCount(); i++ ) {
    c = findNextWordStart( c.line(), c.column() );

    // stop when at the last char in the document
    if ( c.line() == m_view->doc()->lines()-1 && c.column() == m_view->doc()->lineLength( c.line() )-1 ) {
      break;
    }
  }

  r.endColumn = c.column();
  r.endLine = c.line();

  return r;
}

KateViRange KateViNormalMode::motionWordBackward()
{
  KTextEditor::Cursor c( m_view->cursorPosition() );
  KateViRange r( c.line(), c.column(), ViMotion::ExclusiveMotion );

  for ( unsigned int i = 0; i < getCount(); i++ ) {
    c = findPrevWordStart( c.line(), c.column() );

    // stop when at the first char in the document
    if ( c.line() == 0 && c.column() == 0 ) {
      break;
    }
  }

  r.endColumn = c.column();
  r.endLine = c.line();

  return r;
}

KateViRange KateViNormalMode::motionWORDForward()
{
  KTextEditor::Cursor c( m_view->cursorPosition() );
  KateViRange r( c.line(), c.column(), ViMotion::ExclusiveMotion );

  for ( unsigned int i = 0; i < getCount(); i++ ) {
    c = findNextWORDStart( c.line(), c.column() );

    // stop when at the last char in the document
    if ( c.line() == m_view->doc()->lines()-1 && c.column() == m_view->doc()->lineLength( c.line() )-1 ) {
      break;
    }
  }

  r.endColumn = c.column();
  r.endLine = c.line();

  return r;
}

KateViRange KateViNormalMode::motionWORDBackward()
{
  KTextEditor::Cursor c( m_view->cursorPosition() );
  KateViRange r( c.line(), c.column(), ViMotion::ExclusiveMotion );

  for ( unsigned int i = 0; i < getCount(); i++ ) {
    c = findPrevWORDStart( c.line(), c.column() );

    // stop when at the first char in the document
    if ( c.line() == 0 && c.column() == 0 ) {
      break;
    }
  }

  r.endColumn = c.column();
  r.endLine = c.line();

  return r;
}

KateViRange KateViNormalMode::motionToEndOfWord()
{
    KTextEditor::Cursor c( m_view->cursorPosition() );
    KateViRange r( c.line(), c.column(), ViMotion::InclusiveMotion );

    for ( unsigned int i = 0; i < getCount(); i++ ) {
        c = findWordEnd( c.line(), c.column() );
    }

    r.endColumn = c.column();
    r.endLine = c.line();

    return r;
}

KateViRange KateViNormalMode::motionToEndOfWORD()
{
    KTextEditor::Cursor c( m_view->cursorPosition() );
    KateViRange r( c.line(), c.column(), ViMotion::InclusiveMotion );

    for ( unsigned int i = 0; i < getCount(); i++ ) {
        c = findWORDEnd( c.line(), c.column() );
    }

    r.endColumn = c.column();
    r.endLine = c.line();

    return r;
}

bool KateViNormalMode::commandDeleteLine()
{
  KTextEditor::Cursor c( m_view->cursorPosition() );

  QString removedLines;

  KateViRange r;

  r.startLine = c.line();
  r.endLine = c.line()+getCount()-1;

  int column = c.column();

  bool ret = deleteRange( r, true );

  c = m_view->cursorPosition();
  if ( column > getLine().length()-1 ) {
    column = getLine().length()-1;
  }
  if ( column < 0 ) {
    column = 0;
  }

  if ( c.line() > m_view->doc()->lines()-1 ) {
    c.setLine( m_view->doc()->lines()-1 );
  }

  c.setColumn( column );
  m_viewInternal->updateCursor( c );

  return ret;
}

bool KateViNormalMode::commandDelete()
{
  KTextEditor::Cursor c( m_view->cursorPosition() );

  if ( m_commandRange.startLine == -1 ) {
    m_commandRange.startLine = c.line();
    m_commandRange.startColumn = c.column();
  }

  bool linewise = ( m_commandRange.startLine != m_commandRange.endLine
      && m_view->getCurrentViMode() != VisualMode );

  return deleteRange( m_commandRange, linewise );
}

bool KateViNormalMode::commandDeleteToEOL()
{
  KTextEditor::Cursor c( m_view->cursorPosition() );

  m_commandRange.endLine = c.line();
  m_commandRange.endColumn = m_view->doc()->lineLength( c.line() );

  if ( m_view->getCurrentViMode() == NormalMode ) {
    m_commandRange.startLine = c.line();
    m_commandRange.startColumn = c.column();
  }

  // can only be true for visual mode
  bool linewise = ( m_commandRange.startLine != m_commandRange.endLine );

  bool r = deleteRange( m_commandRange, linewise );

  if ( !linewise ) {
    c.setColumn( m_view->doc()->lineLength( c.line() )-1 );
  } else {
    c.setLine( m_commandRange.startLine-1 );
    c.setColumn( m_commandRange.startColumn );
  }

  // make sure cursor position is valid after deletion
  if ( c.line() < 0 ) {
    c.setLine( 0 );
  }
  if ( c.column() > m_view->doc()->lineLength( c.line() )-1 ) {
    c.setColumn( m_view->doc()->lineLength( c.line() )-1 );
  }
  if ( c.column() < 0 ) {
    c.setColumn( 0 );
  }

  m_viewInternal->updateCursor( c );

  return r;
}

bool KateViNormalMode::commandMakeLowercase()
{
  KTextEditor::Cursor c( m_view->cursorPosition() );

  int line1 = ( m_commandRange.startLine != -1 ? m_commandRange.startLine : c.line() );
  int line2 = m_commandRange.endLine;

  if ( line1 == line2 ) { // characterwise
    int endColumn = ( m_commandRange.endColumn > c.column() ) ? m_commandRange.endColumn : c.column();
    int startColumn = ( m_commandRange.endColumn > c.column() ) ? c.column() : m_commandRange.endColumn;
    if ( m_commandRange.isInclusive() )
      endColumn++;

    KTextEditor::Range range( KTextEditor::Cursor( c.line(), startColumn ), KTextEditor::Cursor( c.line(), endColumn ) );
    m_view->doc()->replaceText( range, getLine().mid( startColumn, endColumn-startColumn ).toLower() );
  }
  else { // linewise
    QString s;
    for ( int i = ( line1 < line2 ? line1 : line2 ); i <= ( line1 < line2 ? line2 : line1 ); i++ ) {
       s.append( m_view->doc()->line( i ).toLower() + '\n' );
    }

    // remove the laste \n
    s.chop( 1 );

    KTextEditor::Range range( KTextEditor::Cursor( c.line(), 0 ),
        KTextEditor::Cursor( line2, m_view->doc()->lineLength( line2 ) ) );

    m_view->doc()->replaceText( range, s );
  }

  m_viewInternal->updateCursor( c );

  return true;
}

bool KateViNormalMode::commandMakeLowercaseLine()
{
  KTextEditor::Cursor c( m_view->cursorPosition() );

  m_commandRange.startLine = c.line();
  m_commandRange.endLine = c.line();
  m_commandRange.startColumn = 0;
  m_commandRange.endColumn = m_view->doc()->lineLength( c.line() );

  return commandMakeLowercase();
}

bool KateViNormalMode::commandMakeUppercase()
{
  KTextEditor::Cursor c( m_view->cursorPosition() );

  int line1 = ( m_commandRange.startLine != -1 ? m_commandRange.startLine : c.line() );
  int line2 = m_commandRange.endLine;

  if ( line1 == line2 ) { // characterwise
    int endColumn = ( m_commandRange.endColumn > c.column() ) ? m_commandRange.endColumn : c.column();
    int startColumn = ( m_commandRange.endColumn > c.column() ) ? c.column() : m_commandRange.endColumn;
    if ( m_commandRange.isInclusive() )
      endColumn++;

    KTextEditor::Range range( KTextEditor::Cursor( c.line(), startColumn ), KTextEditor::Cursor( c.line(), endColumn ) );
    m_view->doc()->replaceText( range, getLine().mid( startColumn, endColumn-startColumn ).toUpper() );
  }
  else { // linewise
    QString s;
    for ( int i = ( line1 < line2 ? line1 : line2 ); i <= ( line1 < line2 ? line2 : line1 ); i++ ) {
       s.append( m_view->doc()->line( i ).toUpper() + '\n' );
    }

    // remove the laste \n
    s.chop( 1 );

    KTextEditor::Range range( KTextEditor::Cursor( c.line(), 0 ),
        KTextEditor::Cursor( line2, m_view->doc()->lineLength( line2 ) ) );

    m_view->doc()->replaceText( range, s );
  }

  m_viewInternal->updateCursor( c );

  return true;
}

bool KateViNormalMode::commandMakeUppercaseLine()
{
  KTextEditor::Cursor c( m_view->cursorPosition() );

  m_commandRange.startLine = c.line();
  m_commandRange.endLine = c.line();
  m_commandRange.startColumn = 0;
  m_commandRange.endColumn = m_view->doc()->lineLength( c.line() );

  return commandMakeUppercase();
}

bool KateViNormalMode::commandOpenNewLineUnder()
{
  KTextEditor::Cursor c( m_view->cursorPosition() );

  c.setColumn( getLine().length() );
  m_viewInternal->updateCursor( c );

  for ( unsigned int i = 0; i < getCount(); i++ ) {
    m_view->doc()->newLine( m_view );
  }

  m_view->changeViMode( InsertMode );
  m_viewInternal->repaint ();

  return true;
}

bool KateViNormalMode::commandOpenNewLineOver()
{
  KTextEditor::Cursor c( m_view->cursorPosition() );

  if ( c.line() == 0 ) {
    for (unsigned int i = 0; i < getCount(); i++ ) {
      m_view->doc()->insertLine( 0, QString() );
    }
    c.setColumn( 0 );
    c.setLine( 0 );
    m_viewInternal->updateCursor( c );
  } else {
    c.setLine( c.line()-1 );
    c.setColumn( getLine( c.line() ).length() );
    m_viewInternal->updateCursor( c );
    for ( unsigned int i = 0; i < getCount(); i++ ) {
        m_view->doc()->newLine( m_view );
    }

    if ( getCount() > 1 ) {
      c = m_view->cursorPosition();
      c.setLine( c.line()-(getCount()-1 ) );
      m_viewInternal->updateCursor( c );
    }
    //c.setLine( c.line()-getCount() );
  }

  m_view->changeViMode( InsertMode );
  m_viewInternal->repaint ();

  return true;
}

bool KateViNormalMode::commandJoinLines()
{
  KTextEditor::Cursor c( m_view->cursorPosition() );

  int n = getCount();

  // if we were given a range of lines, this information overrides the previous
  if ( m_commandRange.startLine != -1 && m_commandRange.endLine != -1 ) {
    c.setLine ( m_commandRange.startLine );
    n = m_commandRange.endLine-m_commandRange.startLine;
  }

  // make sure we don't try to join lines past the document end
  if ( n > m_view->doc()->lines()-1-c.line() ) {
      n = m_view->doc()->lines()-1-c.line();
  }

  m_view->doc()->joinLines( c.line(), c.line()+n );

  return true;
}

bool KateViNormalMode::commandChange()
{
  KTextEditor::Cursor c( m_view->cursorPosition() );

  if ( m_commandRange.startLine == -1 ) {
    m_commandRange.startLine = c.line();
    m_commandRange.startColumn = c.column();
  }

  bool linewise = ( m_commandRange.startLine != m_commandRange.endLine
      && m_view->getCurrentViMode() != VisualMode );

  commandDelete();

  // if we deleted several lines, insert an empty line and put the cursor there
  if ( linewise ) {
    m_view->doc()->insertLine( m_commandRange.startLine, QString() );
    c.setLine( m_commandRange.startLine );
    m_viewInternal->updateCursor( c );
  }

  commandEnterInsertMode();

  return true;
}

bool KateViNormalMode::commandChangeToEOL()
{
  commandDeleteToEOL();
  commandEnterInsertModeAppend();

  return true;
}

bool KateViNormalMode::commandChangeLine()
{
  // FIXME: take count and range into account
  KTextEditor::Cursor c( m_view->cursorPosition() );
  c.setColumn( 0 );
  m_viewInternal->updateCursor( c );

  commandDeleteToEOL();
  commandEnterInsertModeAppend();

  return true;
}

bool KateViNormalMode::commandSubstituteChar()
{
  if ( commandDeleteChar() ) {
    return commandEnterInsertMode();
  }

  return false;
}

bool KateViNormalMode::commandSubstituteLine()
{
  return commandChangeLine();
}

bool KateViNormalMode::commandYank()
{
  KTextEditor::Cursor c( m_view->cursorPosition() );

  bool r = false;
  QString yankedText;

  if ( m_commandRange.startLine == -1 ) {
    m_commandRange.startLine = c.line();
    m_commandRange.startColumn = c.column();
  }

  bool linewise = ( m_commandRange.startLine != m_commandRange.endLine
      && m_view->getCurrentViMode() != VisualMode );

  yankedText = getRange( m_commandRange, linewise );

  fillRegister( getChosenRegister( '0' ), yankedText );

  return r;
}

bool KateViNormalMode::commandYankLine()
{
  KTextEditor::Cursor c( m_view->cursorPosition() );
  QString lines;
  int linenum = c.line();

  for ( unsigned int i = 0; i < getCount(); i++ ) {
      lines.append( getLine( linenum + i ) + '\n' );
  }
  fillRegister( getChosenRegister( '0' ), lines );

  return true;
}

bool KateViNormalMode::commandYankToEOL()
{
  return false;
}

bool KateViNormalMode::commandFormatLines()
{
  return false;
}

// insert the text in the given register at the cursor position
// the cursor should end up at the end of what was pasted
bool KateViNormalMode::commandPaste()
{
  KTextEditor::Cursor c( m_view->cursorPosition() );
  KTextEditor::Cursor cAfter = c;
  QChar reg = getChosenRegister( m_defaultRegister );

  QString textToInsert = getRegisterContent( reg );

  if ( getCount() > 1 ) {
    QString temp = textToInsert;
    for ( unsigned int i = 1; i < getCount(); i++ ) {
      textToInsert.append( temp );
    }
  }

  if ( textToInsert.indexOf('\n') != -1 ) { // lines
    textToInsert.chop( 1 ); // remove the last \n
    c.setColumn( getLine().length() ); // paste after the current line and ...
    textToInsert.prepend( QChar( '\n' ) ); // ... prepend a \n, so the text starts on a new line

    cAfter.setLine( cAfter.line()+1 );
  } else { // one line
    if ( getLine( c.line() ).length() > 0 ) {
      c.setColumn( c.column()+1 );
    }

    cAfter.setColumn( cAfter.column() + textToInsert.length() );
  }

  m_view->doc()->insertText( c, textToInsert );

  m_viewInternal->updateCursor( cAfter );

  return true;
}

// insert the text in the given register before the cursor position
// the cursor should end up at the end of what was pasted
bool KateViNormalMode::commandPasteBefore()
{
  KTextEditor::Cursor c( m_view->cursorPosition() );
  KTextEditor::Cursor cAfter = c;
  QChar reg = getChosenRegister( m_defaultRegister );

  QString textToInsert = getRegisterContent( reg );

  if ( getCount() > 1 ) {
    QString temp = textToInsert;
    for ( unsigned int i = 1; i < getCount(); i++ ) {
      textToInsert.append( temp );
    }
  }

  if ( textToInsert.indexOf('\n') != -1 ) { // lines
    c.setColumn( 0 );
  } else {
    cAfter.setColumn( cAfter.column()+textToInsert.length()-1 );
  }

  m_view->doc()->insertText( c, textToInsert );

  m_viewInternal->updateCursor( cAfter );

  return true;
}

bool KateViNormalMode::commandDeleteChar()
{
    KTextEditor::Cursor c( m_view->cursorPosition() );
    KateViRange r( c.line(), c.column(), c.line(), c.column()+getCount(), ViMotion::ExclusiveMotion );

    if ( m_commandRange.startLine != -1 && m_commandRange.startColumn != -1 ) {
      r = m_commandRange;
    } else {
      if ( r.endColumn > m_view->doc()->lineLength( r.startLine ) ) {
        r.endColumn = m_view->doc()->lineLength( r.startLine );
      }
    }

    return deleteRange( r, false );
}

bool KateViNormalMode::commandDeleteCharBackward()
{
    KTextEditor::Cursor c( m_view->cursorPosition() );

    KateViRange r( c.line(), c.column()-getCount(), c.line(), c.column(), ViMotion::ExclusiveMotion );

    if ( m_commandRange.startLine != -1 && m_commandRange.startColumn != -1 ) {
      r = m_commandRange;
    } else {
      if ( r.startColumn < 0 ) {
        r.startColumn = 0;
      }
    }

    return deleteRange( r, false );
}

bool KateViNormalMode::commandReplaceCharacter()
{
  KTextEditor::Cursor c1( m_view->cursorPosition() );
  KTextEditor::Cursor c2( m_view->cursorPosition() );

  c2.setColumn( c2.column()+1 );

  bool r = m_view->doc()->replaceText( KTextEditor::Range( c1, c2 ), m_keys.right( 1 ) );

  m_viewInternal->updateCursor( c1 );

  return r;
}

bool KateViNormalMode::commandSwitchToCmdLine()
{
    m_view->switchToCmdLine();
    return true;
}

bool KateViNormalMode::commandSearch()
{
    m_view->find();
    return true;
}

bool KateViNormalMode::commandUndo()
{
    m_view->doc()->undo();
    return true;
}

bool KateViNormalMode::commandRedo()
{
    m_view->doc()->redo();
    return true;
}

bool KateViNormalMode::commandSetMark()
{
  KTextEditor::Cursor c( m_view->cursorPosition() );

  KateSmartCursor *cursor = m_view->doc()->smartManager()->newSmartCursor( c );

  m_marks->insert( m_keys.at( m_keys.size()-1 ), cursor );
  kDebug( 13070 ) << "set mark at (" << c.line() << "," << c.column() << ")";

  return true;
}

bool KateViNormalMode::commandFindPrev()
{
    m_view->findPrevious();

    return true;
}

bool KateViNormalMode::commandFindNext()
{
    m_view->findNext();

    return true;
}

bool KateViNormalMode::commandIndentLine()
{
    KTextEditor::Cursor c( m_view->cursorPosition() );

    for ( unsigned int i = 0; i < getCount(); i++ ) {
        m_view->doc()->indent( m_view, c.line()+i, 1 );
    }

    return true;
}

bool KateViNormalMode::commandUnindentLine()
{
    KTextEditor::Cursor c( m_view->cursorPosition() );

    for ( unsigned int i = 0; i < getCount(); i++ ) {
        m_view->doc()->indent( m_view, c.line()+i, -1 );
    }

    return true;
}

bool KateViNormalMode::commandIndentLines()
{
  KTextEditor::Cursor c( m_view->cursorPosition() );

  m_commandRange.normalize();

  int line1 = ( m_commandRange.startLine != -1 ? m_commandRange.startLine : c.line() );
  int line2 = m_commandRange.endLine;

  for ( int i = line1; i <= line2; i++ ) {
      m_view->doc()->indent( m_view, i, 1 );
  }

  return true;
}

bool KateViNormalMode::commandUnindentLines()
{
  KTextEditor::Cursor c( m_view->cursorPosition() );

  m_commandRange.normalize();

  int line1 = ( m_commandRange.startLine != -1 ? m_commandRange.startLine : c.line() );
  int line2 = m_commandRange.endLine;

  for ( int i = line1; i <= line2; i++ ) {
      m_view->doc()->indent( m_view, i, -1 );
  }

  return true;
}

bool KateViNormalMode::commandScrollPageDown()
{
  m_view->pageDown();

  return true;
}

bool KateViNormalMode::commandScrollPageUp()
{
  m_view->pageUp();

  return true;
}

bool KateViNormalMode::commandAbort()
{
  reset();
  return true;
}

bool KateViNormalMode::commandPrintCharacterCode()
{
  KTextEditor::Cursor c( m_view->cursorPosition() );

  QString line = getLine( c.line() );

  if ( c.column() >= line.length() ) {
    return false;
  }

  QChar ch = line.at( c.column() );
  int code = ch.unicode();

  message( QString("<")+ch+">"+"  "+QString::number( code )+",  Hex "+QString::number( code, 16  )
      +",  Octal "+QString::number( code, 8 ) );

  return true;
}

////////////////////////////////////////////////////////////////////////////////
// MOTIONS
////////////////////////////////////////////////////////////////////////////////

KateViRange KateViNormalMode::motionDown()
{
  KTextEditor::Cursor c( m_view->cursorPosition() );

  KateViRange r( c.line(), c.column(), ViMotion::InclusiveMotion );
  r.endLine += getCount();

  // don't go below the last line
  if ( m_view->doc()->lines()-1 < r.endLine ) {
    r.endLine = m_view->doc()->lines()-1;
  }

  // if sticky is set, check if we can co back to the sticky column in the new
  // line. if line is longer than previous line, but shorter than sticky, go
  // to the last column
  if ( m_stickyColumn != -1 ) {
    if ( getLine( r.endLine ).length()-1 > m_stickyColumn )
      r.endColumn = m_stickyColumn;
    else if ( getLine( r.endLine ).length()-1 > r.endLine )
      r.endColumn = getLine( r.endLine ).length()-1;
  }

  // if the line below is shorter than the current cursor column, set sticky
  // to the old value and go to the last character of the previous line
  if ( getLine( r.endLine ).length()-1 < r.endColumn ) {
    if ( m_stickyColumn == -1 )
      m_stickyColumn = r.endColumn;
    r.endColumn = getLine( r.endLine ).length()-1;
    r.endColumn = ( r.endColumn < 0 ) ? 0 : r.endColumn;
  }

  return r;
}


KateViRange KateViNormalMode::motionUp()
{
  KTextEditor::Cursor cursor ( m_view->cursorPosition() );

  KateViRange r( cursor.line(), cursor.column(), ViMotion::InclusiveMotion );
  r.endLine -= getCount();

  // don't go above the first line
  if ( r.endLine < 0 ) {
    r.endLine = 0;
  }

  // if sticky is set, check if we can co back to the sticky column in the new
  // line. if line is longer than previous line, but shorter than sticky, go
  // to the last column
  if ( m_stickyColumn != -1 ) {
    if ( getLine( r.endLine ).length()-1 > m_stickyColumn )
      r.endColumn = m_stickyColumn;
    else if ( getLine( r.endLine ).length()-1 > r.endLine )
      r.endColumn = getLine( r.endLine ).length()-1;
  }

  // if the line above is shorter than the current cursor column, set sticky
  // to the old value and go to the last character of the previous line
  if ( getLine( r.endLine ).length()-1 < r.endColumn ) {
    if ( m_stickyColumn == -1 )
      m_stickyColumn = r.endColumn;
    r.endColumn = getLine( r.endLine ).length()-1;
    r.endColumn = ( r.endColumn < 0 ) ? 0 : r.endColumn;
  }

  return r;
}

KateViRange KateViNormalMode::motionLeft()
{
  KTextEditor::Cursor cursor ( m_view->cursorPosition() );

  if ( m_stickyColumn != -1 )
    m_stickyColumn = -1;

  KateViRange r( cursor.line(), cursor.column(), ViMotion::ExclusiveMotion );

  r.endColumn -= getCount();

  if ( r.endColumn < 0 ) {
    r.endColumn = 0;
  }

  return r;
}

KateViRange KateViNormalMode::motionRight()
{
  KTextEditor::Cursor cursor ( m_view->cursorPosition() );

  if ( m_stickyColumn != -1 )
    m_stickyColumn = -1;

  KateViRange r( cursor.line(), cursor.column(), ViMotion::ExclusiveMotion );

  r.endColumn += getCount();

  if ( r.endColumn > getLine().length()-1 ) {
    r.endColumn = getLine().length()-1;
    r.endColumn = ( r.endColumn < 0 ) ? 0 : r.endColumn;
  }

  return r;
}

KateViRange KateViNormalMode::motionToEOL()
{
  if ( m_stickyColumn != -1 )
    m_stickyColumn = -1;

  KTextEditor::Cursor cursor ( m_view->cursorPosition() );

  KateViRange r( cursor.line(), getLine().length()-1, ViMotion::InclusiveMotion );

  return r;
}

KateViRange KateViNormalMode::motionToColumn0()
{
  if ( m_stickyColumn != -1 )
    m_stickyColumn = -1;

  KTextEditor::Cursor cursor ( m_view->cursorPosition() );

  KateViRange r( cursor.line(), 0, ViMotion::ExclusiveMotion );

  return r;
}

KateViRange KateViNormalMode::motionToFirstCharacterOfLine()
{
  if ( m_stickyColumn != -1 )
    m_stickyColumn = -1;

    KTextEditor::Cursor cursor ( m_view->cursorPosition() );
    QRegExp nonSpace( "\\S" );
    int c = getLine().indexOf( nonSpace );

    KateViRange r( cursor.line(), c, ViMotion::ExclusiveMotion );

    if ( c == -1 ) {
        r.valid = false;
    }

    return r;
}

KateViRange KateViNormalMode::motionFindChar()
{
  KTextEditor::Cursor cursor ( m_view->cursorPosition() );
  QString line = getLine();

  int matchColumn = cursor.column();

  for ( unsigned int i = 0; i < getCount(); i++ ) {
    matchColumn = line.indexOf( m_keys.right( 1 ), matchColumn+1 );
    if ( matchColumn == -1 )
      break;
  }

  KateViRange r;

  if ( matchColumn >= 0 ) {
    r.startColumn = cursor.column();
    r.startLine = cursor.line();
    r.endColumn = matchColumn;
    r.endLine = cursor.line();
  } else {
    r.valid = false;
  }

  return r;
}

KateViRange KateViNormalMode::motionFindCharBackward()
{
  KTextEditor::Cursor cursor ( m_view->cursorPosition() );
  QString line = getLine();

  int matchColumn = -1;

  unsigned int hits = 0;
  int i = cursor.column()-1;

  while ( hits != getCount() && i >= 0 ) {
    if ( line.at( i ) == m_keys.at( m_keys.size()-1 ) )
      hits++;

    if ( hits == getCount() )
      matchColumn = i;

    i--;
  }

  KateViRange r;

  if ( matchColumn >= 0 ) {
    r.endColumn = matchColumn;
    r.endLine = cursor.line();
  } else {
    r.valid = false;
  }

  return r;
}

KateViRange KateViNormalMode::motionToChar()
{
  KTextEditor::Cursor cursor ( m_view->cursorPosition() );
  QString line = getLine();

  int matchColumn = cursor.column()+1;

  for ( unsigned int i = 0; i < getCount(); i++ ) {
    matchColumn = line.indexOf( m_keys.right( 1 ), matchColumn+1 );
    if ( matchColumn == -1 )
      break;
  }

  KateViRange r;

  if ( matchColumn >= 0 ) {
    r.endColumn = matchColumn-1;
    r.endLine = cursor.line();
  } else {
    r.valid = false;
  }

  return r;
}

KateViRange KateViNormalMode::motionToCharBackward()
{
  KTextEditor::Cursor cursor ( m_view->cursorPosition() );
  QString line = getLine();

  int matchColumn = -1;

  unsigned int hits = 0;
  int i = cursor.column()-2;

  while ( hits != getCount() && i >= 0 ) {
    if ( line.at( i ) == m_keys.at( m_keys.size()-1 ) )
      hits++;

    if ( hits == getCount() )
      matchColumn = i;

    i--;
  }

  KateViRange r;

  if ( matchColumn >= 0 ) {
    r.endColumn = matchColumn+1;
    r.endLine = cursor.line();
  } else {
    r.valid = false;
  }

  return r;
}

KateViRange KateViNormalMode::motionToLineFirst()
{
    KateViRange r( getCount()-1, 0, ViMotion::InclusiveMotion );
    r.jump = true;

    return r;
}

KateViRange KateViNormalMode::motionToLineLast()
{
  KateViRange r( m_view->doc()->lines()-1, 0, ViMotion::InclusiveMotion );

  if ( getCount()-1 != 0 ) {
    r.endLine = getCount()-1;
  }

  r.jump = true;

  return r;
}

KateViRange KateViNormalMode::motionToScreenColumn()
{
  if ( m_stickyColumn != -1 )
    m_stickyColumn = -1;

  KTextEditor::Cursor c( m_view->cursorPosition() );

  int column = getCount()-1;

  if ( m_view->doc()->lineLength( c.line() )-1 < (int)getCount()-1 ) {
      column = m_view->doc()->lineLength( c.line() )-1;
  }

  return KateViRange( c.line(), column, ViMotion::ExclusiveMotion );
}

KateViRange KateViNormalMode::motionToMark()
{
  KateViRange r;

  QChar reg = m_keys.at( m_keys.size()-1 );

  // ` and ' is the same register (position before jump)
  if ( reg == '`' ) {
      reg = '\'';
  }

  KTextEditor::SmartCursor* cursor;
  if ( m_marks->contains( reg ) ) {
    cursor = m_marks->value( reg );
    r.endLine = cursor->line();
    r.endColumn = cursor->column();
  } else {
    error(QString("Mark not set: ") + m_keys.right( 1 ) );
    r.valid = false;
  }

  r.jump = true;

  return r;
}

KateViRange KateViNormalMode::motionToMarkLine()
{
  KateViRange r = motionToMark();
  r.endColumn = 0; // FIXME: should be first non-blank on line

  r.jump = true;

  return r;
}

KateViRange KateViNormalMode::motionToMatchingItem()
{
  KateViRange r;
  KTextEditor::Cursor c( m_view->cursorPosition() );
  int lines = m_view->doc()->lines();
  QString l = getLine();
  int n1 = l.indexOf( m_matchItemRegex, c.column() );
  int n2;

  if ( n1 == -1 ) {
    r.valid = false;
    return r;
  } else {
    n2 = l.indexOf( QRegExp( "\\s|$" ), n1 );
  }

  // text item we want to find a matching item for
  QString item = l.mid( n1, n2-n1 );

  // use kate's built-in matching bracket finder for brackets
  if ( QString("{}()[]").indexOf( item ) != -1 ) {
    c.setColumn( n1+1 );
    m_viewInternal->updateCursor( c );
    KateSmartRange *m_bm, *m_bmStart, *m_bmEnd;

    m_bm = m_viewInternal->m_bm;
    m_bmStart = m_viewInternal->m_bmStart;
    m_bmEnd = m_viewInternal->m_bmEnd;

    if (!m_bm->isValid()) {
      r.valid = false;
      return r;
    }

    Q_ASSERT(m_bmEnd->isValid());
    Q_ASSERT(m_bmStart->isValid());

    if (m_bmStart->contains(m_viewInternal->m_cursor) || m_bmStart->end() == m_viewInternal->m_cursor) {
      c = m_bmEnd->end();
    } else if (m_bmEnd->contains(m_viewInternal->m_cursor) || m_bmEnd->end() == m_viewInternal->m_cursor) {
      c = m_bmStart->end();
    } else {
      // should never happen: a range exists, but the cursor position is
      // neither at the start nor at the end...
      r.valid = false;
      return r;
    }

    c.setColumn( c.column()-1 );
  } else {
    int toFind = 1;
    //int n2 = l.indexOf( QRegExp( "\\s|$" ), n1 );

    //QString item = l.mid( n1, n2-n1 );
    QString matchingItem = m_matchingItems[ item ];

    int line = c.line();
    int column = n2-item.length();
    bool reverse = false;

    if ( matchingItem.left( 1 ) == "-" ) {
      matchingItem.remove( 0, 1 ); // remove the '-'
      reverse = true;
    }

    // make sure we don't hit the text item we start the search from
    if ( column == 0 && reverse ) {
      column -= item.length();
    }

    int itemIdx;
    int matchItemIdx;

    while ( toFind > 0 ) {
      if ( !reverse ) {
        itemIdx = l.indexOf( item, column );
        matchItemIdx = l.indexOf( matchingItem, column );

        if ( itemIdx != -1 && ( matchItemIdx == -1 || itemIdx < matchItemIdx ) ) {
            toFind++;
        }
      } else {
        itemIdx = l.lastIndexOf( item, column-1 );
        matchItemIdx = l.lastIndexOf( matchingItem, column-1 );

        if ( itemIdx != -1 && ( matchItemIdx == -1 || itemIdx > matchItemIdx ) ) {
            toFind++;
        }
      }

      if ( matchItemIdx != -1 || itemIdx != -1 ) {
          if ( !reverse ) {
            column = qMin( (unsigned int)itemIdx, (unsigned int)matchItemIdx );
          } else {
            column = qMax( itemIdx, matchItemIdx );
          }
      }

      if ( matchItemIdx != -1 ) { // match on current line
          if ( matchItemIdx == column ) {
              toFind--;
              c.setLine( line );
              c.setColumn( column );
          }
      } else { // no match, advance one line if possible
        ( reverse) ? line-- : line++;
        column = 0;

        if ( ( !reverse && line >= lines ) || ( reverse && line < 0 ) ) {
          r.valid = false;
          break;
        } else {
          l = getLine( line );
        }
      }
    }
  }

  r.endLine = c.line();
  r.endColumn = c.column();
  r.jump = true;

  return r;
}

KateViRange KateViNormalMode::motionToNextBraceBlockStart()
{
  KateViRange r;

  int line = findLineStartingWitchChar( '{', getCount() );

  if ( line == -1 ) {
    r.valid = false;
    return r;
  }

  r.endLine = line;
  r.endColumn = 0;
  r.jump = true;

  return r;
}

KateViRange KateViNormalMode::motionToPreviousBraceBlockStart()
{
  KateViRange r;

  int line = findLineStartingWitchChar( '{', getCount(), false );

  if ( line == -1 ) {
    r.valid = false;
    return r;
  }

  r.endLine = line;
  r.endColumn = 0;
  r.jump = true;

  return r;
}

KateViRange KateViNormalMode::motionToNextBraceBlockEnd()
{
  KateViRange r;

  int line = findLineStartingWitchChar( '}', getCount() );

  if ( line == -1 ) {
    r.valid = false;
    return r;
  }

  r.endLine = line;
  r.endColumn = 0;
  r.jump = true;

  return r;
}

KateViRange KateViNormalMode::motionToPreviousBraceBlockEnd()
{
  KateViRange r;

  int line = findLineStartingWitchChar( '{', getCount(), false );

  if ( line == -1 ) {
    r.valid = false;
    return r;
  }

  r.endLine = line;
  r.endColumn = 0;
  r.jump = true;

  return r;
}

////////////////////////////////////////////////////////////////////////////////
// TEXT OBJECTS
////////////////////////////////////////////////////////////////////////////////

KateViRange KateViNormalMode::textObjectAWord()
{
    KTextEditor::Cursor c( m_view->cursorPosition() );

    KTextEditor::Cursor c1 = findPrevWordStart( c.line(), c.column()+1, true );
    KTextEditor::Cursor c2( c );

    for ( unsigned int i = 0; i < getCount(); i++ ) {
        c2 = findNextWordStart( c2.line(), c2.column(), true );
    }

    KateViRange r( c.line(), c.column(), ViMotion::ExclusiveMotion );

    // sanity check
    if ( c1.line() != c2.line() || c1.column() > c2.column() ) {
        r.valid = false;
    } else {
        r.startLine = c1.line();
        r.endLine = c2.line();
        r.startColumn = c1.column();
        r.endColumn = c2.column();
    }

    return r;
}

KateViRange KateViNormalMode::textObjectInnerWord()
{
    KTextEditor::Cursor c( m_view->cursorPosition() );

    KTextEditor::Cursor c1 = findPrevWordStart( c.line(), c.column()+1, true );
    // need to start search in column-1 because it might be a one-character word
    KTextEditor::Cursor c2( c.line(), c.column()-1 );

    for ( unsigned int i = 0; i < getCount(); i++ ) {
        c2 = findWordEnd( c2.line(), c2.column(), true );
    }

    KateViRange r;

    // sanity check
    if ( c1.line() != c2.line() || c1.column() > c2.column() ) {
        r.valid = false;
    } else {
        r.startLine = c1.line();
        r.endLine = c2.line();
        r.startColumn = c1.column();
        r.endColumn = c2.column();
    }

    return r;
}

KateViRange KateViNormalMode::textObjectAWORD()
{
    KTextEditor::Cursor c( m_view->cursorPosition() );

    KTextEditor::Cursor c1 = findPrevWORDStart( c.line(), c.column()+1, true );
    KTextEditor::Cursor c2( c );

    for ( unsigned int i = 0; i < getCount(); i++ ) {
        c2 = findNextWORDStart( c2.line(), c2.column(), true );
    }

    KateViRange r( c.line(), c.column(), ViMotion::ExclusiveMotion );

    // sanity check
    if ( c1.line() != c2.line() || c1.column() > c2.column() ) {
        r.valid = false;
    } else {
        r.startLine = c1.line();
        r.endLine = c2.line();
        r.startColumn = c1.column();
        r.endColumn = c2.column();
    }

    return r;
}

KateViRange KateViNormalMode::textObjectInnerWORD()
{
    KTextEditor::Cursor c( m_view->cursorPosition() );

    KTextEditor::Cursor c1 = findPrevWORDStart( c.line(), c.column()+1, true );
    KTextEditor::Cursor c2( c );

    for ( unsigned int i = 0; i < getCount(); i++ ) {
        c2 = findWORDEnd( c2.line(), c2.column(), true );
    }

    KateViRange r;

    // sanity check
    if ( c1.line() != c2.line() || c1.column() > c2.column() ) {
        r.valid = false;
    } else {
        r.startLine = c1.line();
        r.endLine = c2.line();
        r.startColumn = c1.column();
        r.endColumn = c2.column();
    }

    return r;
}

KateViRange KateViNormalMode::textObjectAQuoteDouble()
{
    return findSurrounding( '"', '"', false );
}

KateViRange KateViNormalMode::textObjectInnerQuoteDouble()
{
    return findSurrounding( '"', '"', true );
}

KateViRange KateViNormalMode::textObjectAQuoteSingle()
{
    return findSurrounding( '\'', '\'', false );
}

KateViRange KateViNormalMode::textObjectInnerQuoteSingle()
{
    return findSurrounding( '\'', '\'', true );
}

KateViRange KateViNormalMode::textObjectAParen()
{
    return findSurrounding( '(', ')', false );
}

KateViRange KateViNormalMode::textObjectInnerParen()
{
    return findSurrounding( '(', ')', true );
}

KateViRange KateViNormalMode::textObjectABracket()
{
    return findSurrounding( '[', ']', false );
}

KateViRange KateViNormalMode::textObjectInnerBracket()
{
    return findSurrounding( '[', ']', true );
}

// add commands
// when adding commands here, remember to add them to visual mode too (if applicable)
void KateViNormalMode::initializeCommands()
{
  m_commands.push_back( new KateViCommand( this, "a", &KateViNormalMode::commandEnterInsertModeAppend, false ) );
  m_commands.push_back( new KateViCommand( this, "A", &KateViNormalMode::commandEnterInsertModeAppendEOL, false ) );
  m_commands.push_back( new KateViCommand( this, "i", &KateViNormalMode::commandEnterInsertMode, false ) );
  m_commands.push_back( new KateViCommand( this, "v", &KateViNormalMode::commandEnterVisualMode, false ) );
  m_commands.push_back( new KateViCommand( this, "V", &KateViNormalMode::commandEnterVisualLineMode, false ) );
  m_commands.push_back( new KateViCommand( this, "o", &KateViNormalMode::commandOpenNewLineUnder, false ) );
  m_commands.push_back( new KateViCommand( this, "O", &KateViNormalMode::commandOpenNewLineOver, false ) );
  m_commands.push_back( new KateViCommand( this, "J", &KateViNormalMode::commandJoinLines, false ) );
  m_commands.push_back( new KateViCommand( this, "c", &KateViNormalMode::commandChange, false, true ) );
  m_commands.push_back( new KateViCommand( this, "C", &KateViNormalMode::commandChangeToEOL, false ) );
  m_commands.push_back( new KateViCommand( this, "cc", &KateViNormalMode::commandChangeLine, false ) );
  m_commands.push_back( new KateViCommand( this, "s", &KateViNormalMode::commandSubstituteChar, false ) );
  m_commands.push_back( new KateViCommand( this, "S", &KateViNormalMode::commandSubstituteLine, false ) );
  m_commands.push_back( new KateViCommand( this, "dd", &KateViNormalMode::commandDeleteLine, false ) );
  m_commands.push_back( new KateViCommand( this, "d", &KateViNormalMode::commandDelete, false, true ) );
  m_commands.push_back( new KateViCommand( this, "D", &KateViNormalMode::commandDeleteToEOL, false ) );
  m_commands.push_back( new KateViCommand( this, "x", &KateViNormalMode::commandDeleteChar, false ) );
  m_commands.push_back( new KateViCommand( this, "X", &KateViNormalMode::commandDeleteCharBackward, false ) );
  m_commands.push_back( new KateViCommand( this, "gq", &KateViNormalMode::commandFormatLines, false, true ) );
//m_commands.push_back( new KateViCommand( this, "gqgq", &KateViNormalMode::commandFormatLines, false, true ) );
//m_commands.push_back( new KateViCommand( this, "gqq", &KateViNormalMode::commandFormatLines, false, true ) );
  m_commands.push_back( new KateViCommand( this, "gu", &KateViNormalMode::commandMakeLowercase, false, true ) );
  m_commands.push_back( new KateViCommand( this, "guu", &KateViNormalMode::commandMakeLowercaseLine, false ) );
  m_commands.push_back( new KateViCommand( this, "gU", &KateViNormalMode::commandMakeUppercase, false, true ) );
  m_commands.push_back( new KateViCommand( this, "gUU", &KateViNormalMode::commandMakeUppercaseLine, false ) );
  m_commands.push_back( new KateViCommand( this, "y", &KateViNormalMode::commandYank, false, true ) );
  m_commands.push_back( new KateViCommand( this, "yy", &KateViNormalMode::commandYankLine, false ) );
  m_commands.push_back( new KateViCommand( this, "Y", &KateViNormalMode::commandYankToEOL, false, true ) );
  m_commands.push_back( new KateViCommand( this, "p", &KateViNormalMode::commandPaste, false, false ) );
  m_commands.push_back( new KateViCommand( this, "P", &KateViNormalMode::commandPasteBefore, false, false ) );
  m_commands.push_back( new KateViCommand( this, "r.", &KateViNormalMode::commandReplaceCharacter, true ) );
  m_commands.push_back( new KateViCommand( this, ":", &KateViNormalMode::commandSwitchToCmdLine, false ) );
  m_commands.push_back( new KateViCommand( this, "/", &KateViNormalMode::commandSearch, false ) );
  m_commands.push_back( new KateViCommand( this, "u", &KateViNormalMode::commandUndo, false ) );
  m_commands.push_back( new KateViCommand( this, "<c-r>", &KateViNormalMode::commandRedo, false ) );
  m_commands.push_back( new KateViCommand( this, "U", &KateViNormalMode::commandRedo, false ) );
  m_commands.push_back( new KateViCommand( this, "m.", &KateViNormalMode::commandSetMark, true, false ) );
  m_commands.push_back( new KateViCommand( this, "n", &KateViNormalMode::commandFindNext, false ) );
  m_commands.push_back( new KateViCommand( this, "N", &KateViNormalMode::commandFindPrev, false ) );
  m_commands.push_back( new KateViCommand( this, ">>", &KateViNormalMode::commandIndentLine, false ) );
  m_commands.push_back( new KateViCommand( this, "<<", &KateViNormalMode::commandUnindentLine, false ) );
  m_commands.push_back( new KateViCommand( this, ">", &KateViNormalMode::commandIndentLines, false, true ) );
  m_commands.push_back( new KateViCommand( this, "<", &KateViNormalMode::commandUnindentLines, false, true ) );
  m_commands.push_back( new KateViCommand( this, "<c-f>", &KateViNormalMode::commandScrollPageDown, false ) );
  m_commands.push_back( new KateViCommand( this, "<c-b>", &KateViNormalMode::commandScrollPageUp, false ) );
  m_commands.push_back( new KateViCommand( this, "ga", &KateViNormalMode::commandPrintCharacterCode, false, false, false ) );

  // regular motions
  m_motions.push_back( new KateViMotion( this, "h", &KateViNormalMode::motionLeft ) );
  m_motions.push_back( new KateViMotion( this, "<left>", &KateViNormalMode::motionLeft ) );
  m_motions.push_back( new KateViMotion( this, "<backspace>", &KateViNormalMode::motionLeft ) );
  m_motions.push_back( new KateViMotion( this, "j", &KateViNormalMode::motionDown ) );
  m_motions.push_back( new KateViMotion( this, "<down>", &KateViNormalMode::motionDown ) );
  m_motions.push_back( new KateViMotion( this, "k", &KateViNormalMode::motionUp ) );
  m_motions.push_back( new KateViMotion( this, "<up>", &KateViNormalMode::motionUp ) );
  m_motions.push_back( new KateViMotion( this, "l", &KateViNormalMode::motionRight ) );
  m_motions.push_back( new KateViMotion( this, "<right>", &KateViNormalMode::motionRight ) );
  m_motions.push_back( new KateViMotion( this, " ", &KateViNormalMode::motionRight ) );
  m_motions.push_back( new KateViMotion( this, "$", &KateViNormalMode::motionToEOL ) );
  m_motions.push_back( new KateViMotion( this, "<end>", &KateViNormalMode::motionToEOL ) );
  m_motions.push_back( new KateViMotion( this, "0", &KateViNormalMode::motionToColumn0 ) );
  m_motions.push_back( new KateViMotion( this, "<home>", &KateViNormalMode::motionToColumn0 ) );
  m_motions.push_back( new KateViMotion( this, "^", &KateViNormalMode::motionToFirstCharacterOfLine ) );
  m_motions.push_back( new KateViMotion( this, "f.", &KateViNormalMode::motionFindChar, true ) );
  m_motions.push_back( new KateViMotion( this, "F.", &KateViNormalMode::motionFindCharBackward, true ) );
  m_motions.push_back( new KateViMotion( this, "t.", &KateViNormalMode::motionToChar, true ) );
  m_motions.push_back( new KateViMotion( this, "T.", &KateViNormalMode::motionToCharBackward, true ) );
  m_motions.push_back( new KateViMotion( this, "gg", &KateViNormalMode::motionToLineFirst ) );
  m_motions.push_back( new KateViMotion( this, "G", &KateViNormalMode::motionToLineLast ) );
  m_motions.push_back( new KateViMotion( this, "w", &KateViNormalMode::motionWordForward ) );
  m_motions.push_back( new KateViMotion( this, "W", &KateViNormalMode::motionWORDForward ) );
  m_motions.push_back( new KateViMotion( this, "b", &KateViNormalMode::motionWordBackward ) );
  m_motions.push_back( new KateViMotion( this, "B", &KateViNormalMode::motionWORDBackward ) );
  m_motions.push_back( new KateViMotion( this, "e", &KateViNormalMode::motionToEndOfWord ) );
  m_motions.push_back( new KateViMotion( this, "E", &KateViNormalMode::motionToEndOfWORD ) );
  m_motions.push_back( new KateViMotion( this, "|", &KateViNormalMode::motionToScreenColumn ) );
  m_motions.push_back( new KateViMotion( this, "%", &KateViNormalMode::motionToMatchingItem ) );
  m_motions.push_back( new KateViMotion( this, "`.", &KateViNormalMode::motionToMark, true ) );
  m_motions.push_back( new KateViMotion( this, "'.", &KateViNormalMode::motionToMarkLine, true ) );
  m_motions.push_back( new KateViMotion( this, "[[", &KateViNormalMode::motionToPreviousBraceBlockStart ) );
  m_motions.push_back( new KateViMotion( this, "]]", &KateViNormalMode::motionToNextBraceBlockStart ) );
  m_motions.push_back( new KateViMotion( this, "[]", &KateViNormalMode::motionToPreviousBraceBlockEnd ) );
  m_motions.push_back( new KateViMotion( this, "][", &KateViNormalMode::motionToNextBraceBlockEnd ) );

  // text objects
  m_motions.push_back( new KateViMotion( this, "iw", &KateViNormalMode::textObjectInnerWord ) );
  m_motions.push_back( new KateViMotion( this, "aw", &KateViNormalMode::textObjectAWord ) );
  m_motions.push_back( new KateViMotion( this, "i\"", &KateViNormalMode::textObjectInnerQuoteDouble ) );
  m_motions.push_back( new KateViMotion( this, "a\"", &KateViNormalMode::textObjectAQuoteDouble ) );
  m_motions.push_back( new KateViMotion( this, "i'", &KateViNormalMode::textObjectInnerQuoteSingle ) );
  m_motions.push_back( new KateViMotion( this, "a'", &KateViNormalMode::textObjectAQuoteSingle ) );
  m_motions.push_back( new KateViMotion( this, "i[()]", &KateViNormalMode::textObjectInnerParen, true ) );
  m_motions.push_back( new KateViMotion( this, "a[()]", &KateViNormalMode::textObjectAParen, true ) );
  m_motions.push_back( new KateViMotion( this, "i[\\[\\]]", &KateViNormalMode::textObjectInnerBracket, true ) );
  m_motions.push_back( new KateViMotion( this, "a[\\[\\]]", &KateViNormalMode::textObjectABracket, true ) );
}

QRegExp KateViNormalMode::generateMatchingItemRegex()
{
  QString pattern("\\[|\\]|\\{|\\}|\\(|\\)|");
  QList<QString> keys = m_matchingItems.keys();

  for ( int i = 0; i < keys.size(); i++ ) {
    QString s = m_matchingItems[ keys[ i ] ];
    s = s.replace( QRegExp( "^-" ), "" );
    s = s.replace( QRegExp( "\\*" ), "\\*" );
    s = s.replace( QRegExp( "\\+" ), "\\+" );
    s = s.replace( QRegExp( "\\[" ), "\\[" );
    s = s.replace( QRegExp( "\\]" ), "\\]" );
    s = s.replace( QRegExp( "\\(" ), "\\(" );
    s = s.replace( QRegExp( "\\)" ), "\\)" );
    s = s.replace( QRegExp( "\\{" ), "\\{" );
    s = s.replace( QRegExp( "\\}" ), "\\}" );

    pattern.append( s );

    if ( i != keys.size()-1 ) {
      pattern.append( "|" );
    }
  }

  return QRegExp( pattern );
}
