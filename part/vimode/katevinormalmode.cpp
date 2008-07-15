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
#include "katesmartmanager.h"
#include <QApplication>
#include <QList>

KateViNormalMode::KateViNormalMode( KateView * view, KateViewInternal * viewInternal )
{
  m_view = view;
  m_viewInternal = viewInternal;
  m_stickyColumn = -1;
  m_extraWordCharacters = ""; // FIXME: make configurable
  m_numberedRegisters = new QList<QString>;
  m_registers = new QMap<QChar, QString>;
  m_marks = new QMap<QChar, KTextEditor::SmartCursor*>;
  m_keyParser = new KateViKeySequenceParser();

  connect ( m_view->doc(), SIGNAL( textRemoved( const QString& ) ), this, SLOT ( textRemoved( const QString& ) ) );

  initializeCommands();
  reset(); // initialise with start configuration
}

KateViNormalMode::~KateViNormalMode()
{
  disconnect( m_view->doc(), SIGNAL( textRemoved( const QString& ) ), this, SLOT( textRemoved( const QString& ) ) );
  delete m_numberedRegisters;
  delete m_registers;
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

  // escape key aborts current command
  if ( keyCode == Qt::Key_Escape ) {
    reset();
    return true;
  }

  int mods = e->modifiers();
  QChar key;

  if ( text.isEmpty() || ( mods != Qt::NoModifier && mods != Qt::ShiftModifier ) ) {
    QString keyPress;

    keyPress.append( '<' );
    keyPress.append( ( mods & Qt::ShiftModifier ) ? "s-" : "" );
    keyPress.append( ( mods & Qt::ControlModifier ) ? "c-" : "" );
    keyPress.append( ( mods & Qt::AltModifier ) ? "a-" : "" );
    keyPress.append( ( mods & Qt::MetaModifier ) ? "m-" : "" );
    keyPress.append( keyCode <= 0xFF ? QChar( keyCode ) : m_keyParser->qt2vi( keyCode ) );
    //keyPress.append( m_keyParser->qt2vi( keyCode ) );
    keyPress.append( '>' );

    QString fafa = m_keyParser->encodeKeySequence( keyPress );

    //key = QChar( viKeyCode );
    //kDebug( 13070 ) << "Super key code: " << viKeyCode << " modifiers: " << modifiers << " " << key.unicode();
    kDebug( 13070 ) << fafa << "   " << (int)(e->key()) << fafa.length();
    kDebug( 13070 ) << "^--- fafa " << keyCode;

    key = fafa.at( 0 );
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
    kDebug( 13070 ) << "Double quote!";
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
        reset();
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
        kDebug( 13070 )  << m_keys.mid( checkFrom ) << " matcher!";
        m_matchingMotions.push_back( i );

        // if it matches exact, we have found the motion command to execute
        if ( m_motions.at( i )->matchesExact( m_keys.mid( checkFrom ) ) ) {
          kDebug( 13070 )  << "Bingo!"; // <--------- !

          if ( checkFrom == 0 ) {
            // no command given before motion, just move the cursor to wherever
            // the motion says it should go to
            KateViRange r = m_motions.at( i )->execute();

            if ( r.valid ) {
              kDebug( 13070 ) << "no command given, going to position (" << r.endLine << "," << r.endColumn << ")";
              KTextEditor::Cursor c;
              c.setLine( r.endLine );
              c.setColumn( r.endColumn );
              m_viewInternal->updateCursor( c );
            } else {
              kDebug( 13070 ) << "invalid position";
            }

            reset();
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
      reset();
      return true;
    }
  } else if ( m_matchingCommands.size() == 0 && m_matchingMotions.size() == 0 ) {
    //if ( m_awaitingMotionOrTextObject.size() == 0 ) {
      reset();
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
QChar KateViNormalMode::getRegister( const QChar &defaultReg ) const
{
  QChar reg = ( m_register != QChar::Null ) ? m_register : defaultReg;

  return reg;
}

QString KateViNormalMode::getRegisterContent( const QChar &reg ) const
{
  QString regContent;

  if ( reg >= '1' && reg <= '9' ) { // numbered register
    regContent = m_numberedRegisters->at( QString( reg ).toInt()-1 );
  } else if ( reg == '+' ) { // system clipboard register
      regContent = QApplication::clipboard()->text( QClipboard::Clipboard );
  } else if ( reg == '*' ) { // system selection register
      regContent = QApplication::clipboard()->text( QClipboard::Selection );
  } else { // regular, named register
    if ( !m_registers->contains( reg ) ) {
      error( QString ("Nothing in register ") + reg );
    } else {
      regContent = m_registers->value( reg );
    }
  }

  return regContent;
}

void KateViNormalMode::error( const QString &errorMsg ) const
{
  kError( 13070 ) << "\033[31m" << errorMsg << "\033[0m\n"; // FIXME
}

void KateViNormalMode::textRemoved( const QString &text )
{
  m_registerTemp.append( text );
}

void KateViNormalMode::removeDone()
{
  QChar reg;

  // multi-line deletes goes to register '1', small deletes (less than one line) to register '-'
  if ( m_registerTemp.indexOf( '\n' ) != -1 ) {
    reg = getRegister( '1' );
  } else {
    reg = getRegister( '-' );
  }

  fillRegister( reg, m_registerTemp );
  m_registerTemp.clear();
}

/**
 * (re)set to start configuration. This is done when a command is completed
 * executed or when a command is aborted
 */
void KateViNormalMode::reset()
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

KTextEditor::Cursor KateViNormalMode::findNextWordStart( int fromLine, int fromColumn ) const
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
      if ( l >= m_view->doc()->lines()-1 ) {
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

KTextEditor::Cursor KateViNormalMode::findNextWORDStart( int fromLine, int fromColumn ) const
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
      if ( l >= m_view->doc()->lines()-1 ) {
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

KTextEditor::Cursor KateViNormalMode::findPrevWordStart( int fromLine, int fromColumn ) const
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
      if ( l <= 0 ) {
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

KTextEditor::Cursor KateViNormalMode::findPrevWORDStart( int fromLine, int fromColumn ) const
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
      if ( l <= 0 ) {
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

KTextEditor::Cursor KateViNormalMode::findWordEnd( int fromLine, int fromColumn ) const
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
          if ( l >= m_view->doc()->lines()-1 ) {
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

KTextEditor::Cursor KateViNormalMode::findWORDEnd( int fromLine, int fromColumn ) const
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
          if ( l >= m_view->doc()->lines()-1 ) {
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

void KateViNormalMode::addToNumberedRegister( const QString &text )
{
  if ( m_numberedRegisters->size() == 9 ) {
    m_numberedRegisters->removeLast();
  }

  // register 0 is used for the last yank command, so insert at position 1
  m_numberedRegisters->prepend( text );

  kDebug( 13070 ) << "Register 1-9:";
  for ( int i = 0; i < m_numberedRegisters->size(); i++ ) {
      kDebug( 13070 ) << "\t Register " << i+1 << ": " << m_numberedRegisters->at( i );
  }
}

void KateViNormalMode::fillRegister( const QChar &reg, const QString &text )
{
  // the specified register is the "black hole register", don't do anything
  if ( reg == '_' ) {
    return;
  }

  if ( reg >= '1' && reg <= '9' ) { // "kill ring" registers
    addToNumberedRegister( text );
  } else if ( reg == '+' ) { // system clipboard register
      QApplication::clipboard()->setText( text,  QClipboard::Clipboard );
  } else if ( reg == '*' ) { // system selection register
      QApplication::clipboard()->setText( text, QClipboard::Selection );
  } else {
    m_registers->insert( reg, text );
  }

  kDebug( 13070 ) << "Register " << reg << " set to " << text;

  if ( reg == '0' || reg == '1' || reg == '-' ) {
    m_defaultRegister = reg;
    kDebug( 13070 ) << "Register " << '"' << " set to point to \"" << reg;
  }
}

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
  KTextEditor::Cursor cursor ( m_view->cursorPosition() );

  for ( unsigned int i = 0; i < getCount(); i++ ) {
    m_view->doc()->removeLine( cursor.line() );
  }

  removeDone();

  return true;
}

bool KateViNormalMode::commandDelete()
{
  KTextEditor::Cursor c( m_view->cursorPosition() );

  int line1 = ( m_commandRange.startLine != -1 ? m_commandRange.startLine : c.line() );
  int line2 = m_commandRange.endLine;

  bool r = false;

  if ( line1 == line2 ) { // characterwise
    int endColumn = m_commandRange.endColumn;
    if ( m_commandRange.isInclusive() )
      endColumn++;

    if ( m_commandRange.startColumn != -1 ) {
        c.setColumn( m_commandRange.startColumn );
    }

    KTextEditor::Range range( c, KTextEditor::Cursor( c.line(), endColumn ) );
    r = m_view->doc()->removeText( range );
  }
  else { // linewise FIXME: not necessarily true: w (word forward) and others are always characterwise
    for ( int i = ( line1 < line2 ? line1 : line2 ); i <= ( line1 < line2 ? line2 : line1 ); i++ ) {
      r = m_view->doc()->removeLine( ( line1 < line2 ? line1 : line2 ) );
    }
  }

  removeDone();

  return r;
}

bool KateViNormalMode::commandDeleteToEOL()
{
  KTextEditor::Cursor c( m_view->cursorPosition() );

  m_commandRange.startLine = -1;
  m_commandRange.startColumn = -1;
  m_commandRange.endLine = c.line();
  m_commandRange.endColumn = m_view->doc()->lineLength( c.line() )-1;

  bool r = commandDelete();

  c.setColumn( m_view->doc()->lineLength( c.line() )-1 );
  m_viewInternal->updateCursor( c );

  removeDone();

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

  for ( unsigned int i = 0; i < getCount(); i++ ) {
      m_view->doc()->insertLine( c.line()+1, "" );
  }

  c.setLine( c.line()+getCount() );
  c.setColumn( 0 );

  m_viewInternal->updateCursor( c );

  m_view->changeViMode( InsertMode );
  m_viewInternal->repaint ();

  return true;
}

bool KateViNormalMode::commandOpenNewLineOver()
{
  KTextEditor::Cursor c( m_view->cursorPosition() );

  for ( unsigned int i = 0; i < getCount(); i++ ) {
      m_view->doc()->insertLine( c.line(), "" );
  }

  c.setLine( c.line() );
  c.setColumn( 0 );

  m_viewInternal->updateCursor( c );

  m_view->changeViMode( InsertMode );
  m_viewInternal->repaint ();

  return true;
}

bool KateViNormalMode::commandJoinLines()
{
  KTextEditor::Cursor c( m_view->cursorPosition() );

  int n = getCount();

  if ( n > m_view->doc()->lines()-1-c.line() ) {
      n = m_view->doc()->lines()-1-c.line(); // FIXME
  }

  kDebug( 13070 ) << "Joining " << n << " lines from line " << c.line();

  m_view->doc()->joinLines( c.line(), c.line()+n );

  return true;
}

bool KateViNormalMode::commandChange()
{
  commandDelete();
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
  KTextEditor::Cursor c( m_view->cursorPosition() );
  c.setColumn( 0 );
  m_viewInternal->updateCursor( c );

  commandDeleteToEOL();
  commandEnterInsertModeAppend();

  return true;
}

bool KateViNormalMode::commandYank()
{
  KTextEditor::Cursor c( m_view->cursorPosition() );

  int line1 = ( m_commandRange.startLine != -1 ? m_commandRange.startLine : c.line() );
  int line2 = m_commandRange.endLine;

  bool r = false;
  QString yankedText;

  if ( line1 == line2 ) { // characterwise
    int endColumn = m_commandRange.endColumn;
    if ( m_commandRange.isInclusive() )
      endColumn++;

    int len = endColumn - c.column();

    yankedText = getLine().mid( c.column(), len );
  }
  else { // linewise
    for ( int i = ( line1 < line2 ? line1 : line2 ); i <= ( line1 < line2 ? line2 : line1 ); i++ ) {
      yankedText.append(getLine( i ) + '\n' );
    }
  }

  fillRegister( getRegister( '0' ), yankedText );

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
  fillRegister( getRegister( '0' ), lines );

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

bool KateViNormalMode::commandAddIndentation()
{
  return false;
}

bool KateViNormalMode::commandRemoveIndentation()
{
  return false;
}

bool KateViNormalMode::commandPaste()
{
  KTextEditor::Cursor c( m_view->cursorPosition() );
  QChar reg = getRegister( m_defaultRegister );

  QString textToInsert = getRegisterContent( reg );

  c.setColumn( c.column()+1 );

  for ( unsigned int i = 0; i < getCount(); i++ ) {
    if ( textToInsert.indexOf('\n') != -1 ) { // lines
      // remove the last \n
      textToInsert.chop( 1 );
      QStringList lines = textToInsert.split( '\n' );

      for (int i = 0; i < lines.size(); i++ ) {
        m_view->doc()->insertLine( c.line()+1+i, lines[ i ] );
      }
    } else {
      m_view->doc()->insertText( c, textToInsert );
    }
  }

  m_viewInternal->updateCursor( c );

  return true;
}

bool KateViNormalMode::commandPasteBefore()
{
  KTextEditor::Cursor c( m_view->cursorPosition() );
  QChar reg = getRegister( m_defaultRegister );

  QString textToInsert = getRegisterContent( reg );

  //c.setColumn( c.column() );

  for ( unsigned int i = 0; i < getCount(); i++ ) {
    if ( textToInsert.indexOf('\n') != -1 ) { // lines
      // remove the last \n
      textToInsert.chop( 1 );
      QStringList lines = textToInsert.split( '\n' );

      for (int i = 0; i < lines.size(); i++ ) {
        m_view->doc()->insertLine( c.line()+i, lines[ i ] );
      }
    } else {
      m_view->doc()->insertText( c, textToInsert );
    }
  }

  m_viewInternal->updateCursor( c );

  return true;
}

bool KateViNormalMode::commandDeleteChar()
{
    KTextEditor::Cursor c1( m_view->cursorPosition() );
    KTextEditor::Cursor c2( c1.line(), c1.column()+getCount() );

    if ( c2.column() > m_view->doc()->lineLength( c1.line() ) ) {
      c2.setColumn( m_view->doc()->lineLength( c1.line() ));
    }

    KTextEditor::Range range( c1, c2 );

    bool ret = m_view->doc()->removeText( range );

    removeDone();

    return ret;
}

bool KateViNormalMode::commandDeleteCharBackward()
{
    KTextEditor::Cursor c1( m_view->cursorPosition() );
    KTextEditor::Cursor c2( c1.line(), c1.column()-getCount() );

    if ( c2.column() < 0 ) {
      c2.setColumn( 0 );
    }

    KTextEditor::Range range( c2, c1 );

    removeDone();

    return m_view->doc()->removeText( range );
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

KateViRange KateViNormalMode::motionDown()
{
  KTextEditor::Cursor cursor ( m_view->cursorPosition() );

  KateViRange r( cursor.line(), cursor.column(), ViMotion::InclusiveMotion );
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

KateViRange KateViNormalMode::motionToFirstCharacterOfLine()
{
  if ( m_stickyColumn != -1 )
    m_stickyColumn = -1;

  KTextEditor::Cursor cursor ( m_view->cursorPosition() );

  KateViRange r( cursor.line(), 0, ViMotion::ExclusiveMotion );

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
    return KateViRange( getCount()-1, 0, ViMotion::InclusiveMotion );
}

KateViRange KateViNormalMode::motionToLineLast()
{
    KateViRange r( m_view->doc()->lines()-1, 0, ViMotion::InclusiveMotion );

  if ( getCount()-1 != 0 ) {
    r.endLine = getCount()-1;
  }

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

KateViRange KateViNormalMode::motionToMatchingBracket()
{
  KateViRange r;
  r.valid = false;
  return r;
}

KateViRange KateViNormalMode::motionToMark()
{
  KateViRange r;

  KTextEditor::SmartCursor* cursor;
  if ( m_marks->contains( m_keys.at( m_keys.size()-1 ) ) ) {
    cursor = m_marks->value( m_keys.at( m_keys.size()-1 ) );
    r.endLine = cursor->line();
    r.endColumn = cursor->column();
  } else {
    error(QString("Mark not set: ") + m_keys.right( 1 ) );
    r.valid = false;
  }

  return r;
}

KateViRange KateViNormalMode::motionToMarkLine()
{
  KateViRange r = motionToMark();
  r.endColumn = 0; // FIXME: should be first non-blank on line

  return r;
}

KateViRange KateViNormalMode::textObjectInnerWord()
{
  KTextEditor::Cursor cursor ( m_view->cursorPosition() );
  QString line = getLine();

  int matchColumnRight = cursor.column();
  int matchColumnLeft = cursor.column();

  /*
  // left position
  for ( int i = cursor.column()-1; i >= 0; i-- ) {
    if ( line.at( i ) == m_keys.right( 1 ) ) {
      matchColumnLeft = i;
      break;
    }
  }
  */

  /*
  // right position
  for ( int i = cursor.column()+1; i < getLine().length()-1; i++ ) {
    if ( line.at( i ) == m_keys.right( 1 ) ) {
      matchColumnRight = i;
      break;
    }
  }
  */

  return KateViRange( cursor.line(), matchColumnLeft, cursor.line(), matchColumnRight, ViMotion::InclusiveMotion );
}

KateViRange KateViNormalMode::textObjectInnerQuoteDouble()
{
  KTextEditor::Cursor c( m_view->cursorPosition() );
  QString line = getLine();

  int col1 = line.lastIndexOf( '"', c.column() )+1;
  int col2 = line.indexOf( '"', c.column() )-1;

  KateViRange r( c.line(), col1, c.line(), col2, ViMotion::InclusiveMotion );
  if ( col1 == -1 || col2 == -1 ) {
      r.valid = false;
  }

  return r;
}

void KateViNormalMode::initializeCommands()
{
  m_commands.push_back( new KateViNormalModeCommand( this, "a", &KateViNormalMode::commandEnterInsertModeAppend, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "A", &KateViNormalMode::commandEnterInsertModeAppendEOL, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "i", &KateViNormalMode::commandEnterInsertMode, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "o", &KateViNormalMode::commandOpenNewLineUnder, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "O", &KateViNormalMode::commandOpenNewLineOver, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "J", &KateViNormalMode::commandJoinLines, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "c", &KateViNormalMode::commandChange, false, true ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "C", &KateViNormalMode::commandChangeToEOL, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "cc", &KateViNormalMode::commandChangeLine, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "dd", &KateViNormalMode::commandDeleteLine, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "d", &KateViNormalMode::commandDelete, false, true ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "D", &KateViNormalMode::commandDeleteToEOL, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "x", &KateViNormalMode::commandDeleteChar, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "X", &KateViNormalMode::commandDeleteCharBackward, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "gq", &KateViNormalMode::commandFormatLines, false, true ) );
//m_commands.push_back( new KateViNormalModeCommand( this, "gqgq", &KateViNormalMode::commandFormatLines, false, true ) );
//m_commands.push_back( new KateViNormalModeCommand( this, "gqq", &KateViNormalMode::commandFormatLines, false, true ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "gu", &KateViNormalMode::commandMakeLowercase, false, true ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "guu", &KateViNormalMode::commandMakeLowercaseLine, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "gU", &KateViNormalMode::commandMakeUppercase, false, true ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "gUU", &KateViNormalMode::commandMakeUppercaseLine, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "y", &KateViNormalMode::commandYank, false, true ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "yy", &KateViNormalMode::commandYankLine, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "Y", &KateViNormalMode::commandYankToEOL, false, true ) );
  m_commands.push_back( new KateViNormalModeCommand( this, ">", &KateViNormalMode::commandAddIndentation, false, true ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "<", &KateViNormalMode::commandRemoveIndentation, false, true ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "p", &KateViNormalMode::commandPaste, false, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "P", &KateViNormalMode::commandPasteBefore, false, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "r.", &KateViNormalMode::commandReplaceCharacter, true ) );
  m_commands.push_back( new KateViNormalModeCommand( this, ":", &KateViNormalMode::commandSwitchToCmdLine, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "/", &KateViNormalMode::commandSearch, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "u", &KateViNormalMode::commandUndo, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "<c-r>", &KateViNormalMode::commandRedo, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "U", &KateViNormalMode::commandRedo, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "m.", &KateViNormalMode::commandSetMark, true, false) );
  m_commands.push_back( new KateViNormalModeCommand( this, "n", &KateViNormalMode::commandFindNext, false) );
  m_commands.push_back( new KateViNormalModeCommand( this, "N", &KateViNormalMode::commandFindPrev, false) );

  // regular motions
  m_motions.push_back( new KateViMotion( this, "h", &KateViNormalMode::motionLeft ) );
  m_motions.push_back( new KateViMotion( this, "j", &KateViNormalMode::motionDown ) );
  m_motions.push_back( new KateViMotion( this, "k", &KateViNormalMode::motionUp ) );
  m_motions.push_back( new KateViMotion( this, "l", &KateViNormalMode::motionRight ) );
  m_motions.push_back( new KateViMotion( this, "$", &KateViNormalMode::motionToEOL ) );
  m_motions.push_back( new KateViMotion( this, "0", &KateViNormalMode::motionToFirstCharacterOfLine ) );
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
  m_motions.push_back( new KateViMotion( this, "%", &KateViNormalMode::motionToMatchingBracket ) );
  m_motions.push_back( new KateViMotion( this, "`.", &KateViNormalMode::motionToMark, true ) );
  m_motions.push_back( new KateViMotion( this, "'.", &KateViNormalMode::motionToMarkLine, true ) );

  // text objects
  m_motions.push_back( new KateViMotion( this, "iw", &KateViNormalMode::textObjectInnerWord ) );
  m_motions.push_back( new KateViMotion( this, "i\"", &KateViNormalMode::textObjectInnerQuoteDouble ) );
}
