/* This file is part of the KDE libraries
 * Copyright (C) 2008 Erlend Hamberg <ehamberg@gmail.com>
 * Copyright (C) 2008 Evgeniy Ivanov <powerfox@kde.ru>
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
#include "kateviinputmodemanager.h"
#include "katesmartmanager.h"
#include "katesmartrange.h"
#include <QApplication>
#include <QList>

KateViNormalMode::KateViNormalMode( KateViInputModeManager *viInputModeManager, KateView * view, KateViewInternal * viewInternal )
{
  m_view = view;
  m_doc = view->doc();
  m_viewInternal = viewInternal;
  m_viInputModeManager = viInputModeManager;
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
bool KateViNormalMode::handleKeypress( const QKeyEvent *e )
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

  QChar key = m_keyParser->KeyEventToQChar( e->key(), e->text(), e->modifiers(), e->nativeScanCode() );
  //kDebug( 13070 ) << key << "(" << keyCode << ")";
  m_keysVerbatim.append( m_keyParser->decodeKeySequence( key ) );

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

  // Special case: "cw" and "cW" work the same as "ce" and "cE" if the cursor is
  // on a non-blank.  This is because Vim interprets "cw" as change-word, and a
  // word does not include the following white space. (:help cw in vim)
  if ( ( m_keys == "cw" || m_keys == "cW" ) && !getCharUnderCursor().isSpace() ) {
    // Special case of the special case: :-)
    // If the cursor is at the end of the current word rewrite to "cl"
    KTextEditor::Cursor c1( m_view->cursorPosition() ); // current position
    KTextEditor::Cursor c2 = findWordEnd(c1.line(), c1.column()-1, true); // word end

    if ( c1 == c2 ) { // the cursor is at the end of a word
      m_keys = "cl";
    } else {
      if ( m_keys.at(1) == 'w' ) {
        m_keys = "ce";
      } else {
        m_keys = "cE";
      }
    }
  }

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
        //kDebug( 13070 ) << "removing " << m_commands.at( m_matchingCommands.at( i ) )->pattern() << ", size before remove is " << m_matchingCommands.size();
        if ( m_commands.at( m_matchingCommands.at( i ) )->needsMotion() ) {
          // "cache" command needing a motion for later
          //kDebug( 13070 ) << "m_motionOperatorIndex set to " << m_motionOperatorIndex;
          m_motionOperatorIndex = m_matchingCommands.at( i );
        }
        m_matchingCommands.remove( i );
      }
    }

    // check if any of the matching commands need a motion/text object, if so
    // push the current command length to m_awaitingMotionOrTextObject so one
    // knows where to split the command between the operator and the motion
    for ( int i = 0; i < m_matchingCommands.size(); i++ ) {
      if ( m_commands.at( m_matchingCommands.at( i ) )->needsMotion() ) {
        m_awaitingMotionOrTextObject.push( m_keys.size() );
        break;
      }
    }
  } else {
    // go through all registered commands and put possible matches in m_matchingCommands
    for ( int i = 0; i < m_commands.size(); i++ ) {
      if ( m_commands.at( i )->matches( m_keys ) ) {
        m_matchingCommands.push_back( i );
        if ( m_commands.at( i )->needsMotion() && m_commands.at( i )->pattern().length() == m_keys.size() ) {
          m_awaitingMotionOrTextObject.push( m_keys.size() );
        }
      }
    }
  }

  // this indicates where in the command string one should start looking for a motion command
  int checkFrom = ( m_awaitingMotionOrTextObject.isEmpty() ? 0 : m_awaitingMotionOrTextObject.top() );

  //kDebug( 13070 ) << "checkFrom: " << checkFrom;

  // look for matching motion commands from position 'checkFrom'
  // FIXME: if checkFrom hasn't changed, only motions whose index is in
  // m_matchingMotions shold be checked
  if ( checkFrom < m_keys.size() ) {
    for ( int i = 0; i < m_motions.size(); i++ ) {
      //kDebug( 13070 )  << "\tchecking " << m_keys.mid( checkFrom )  << " against " << m_motions.at( i )->pattern();
      if ( m_motions.at( i )->matches( m_keys.mid( checkFrom ) ) ) {
        //kDebug( 13070 )  << m_keys.mid( checkFrom ) << " matches!";
        m_matchingMotions.push_back( i );

        // if it matches exact, we have found the motion command to execute
        if ( m_motions.at( i )->matchesExact( m_keys.mid( checkFrom ) ) ) {
          if ( checkFrom == 0 ) {
            // no command given before motion, just move the cursor to wherever
            // the motion says it should go to
            KateViRange r = m_motions.at( i )->execute();

            // make sure the position is valid before moving the cursor there
            if ( r.valid
                && r.endLine >= 0
                && ( r.endLine == 0 || r.endLine <= m_doc->lines()-1 )
                && r.endColumn >= 0
                && ( r.endColumn == 0 || r.endColumn < m_doc->lineLength( r.endLine ) ) ) {
              kDebug( 13070 ) << "No command given, going to position ("
                << r.endLine << "," << r.endColumn << ")";
              goToPos( r );
              m_viInputModeManager->clearLog();
            } else {
              kDebug( 13070 ) << "Invalid position: (" << r.endLine << "," << r.endColumn << ")";
            }

            resetParser();
            return true;
          } else {
            // execute the specified command and supply the position returned from
            // the motion

            m_commandRange = m_motions.at( i )->execute();

            // if we didn't get an explicit start position, use the current cursor position
            if ( m_commandRange.startLine == -1 ) {
              KTextEditor::Cursor c( m_view->cursorPosition() );
              m_commandRange.startLine = c.line();
              m_commandRange.startColumn = c.column();
            }

            // Special case: "word motions" should never cross a line boundary when they are the
            // input to a command
            if ( ( m_keys.right(1) == "w" || m_keys.right(1) == "W" )
                && m_commandRange.endLine > m_commandRange.startLine ) {
              m_commandRange = motionToEOL();

              KTextEditor::Cursor c( m_view->cursorPosition() );
              m_commandRange.startLine = c.line();
              m_commandRange.startColumn = c.column();
            }

            if ( m_commandRange.valid ) {
              kDebug( 13070 ) << "Run command" << m_commands.at( m_motionOperatorIndex )->pattern()
                << "from (" << m_commandRange.startLine << "," << m_commandRange.endLine << ")"
                << "to (" << m_commandRange.endLine << "," << m_commandRange.endColumn << ")";
              executeCommand( m_commands.at( m_motionOperatorIndex ) );
            } else {
              kDebug( 13070 ) << "Invalid range: "
                << "from (" << m_commandRange.startLine << "," << m_commandRange.endLine << ")"
                << "to (" << m_commandRange.endLine << "," << m_commandRange.endColumn << ")";
            }

            reset();
            return true;
          }
        }
      }
    }
  }

  //kDebug( 13070 ) << "'" << m_keys << "' MATCHING COMMANDS: " << m_matchingCommands.size();
  //kDebug( 13070 ) << "'" << m_keys << "' MATCHING MOTIONS: " << m_matchingMotions.size();
  //kDebug( 13070 ) << "'" << m_keys << "' AWAITING MOTION OR TO (INDEX): " << ( m_awaitingMotionOrTextObject.isEmpty() ? 0 : m_awaitingMotionOrTextObject.top() );

  // if we have only one match, check if it is a perfect match and if so, execute it
  // if it's not waiting for a motion or a text object
  if ( m_matchingCommands.size() == 1 ) {
    if ( m_commands.at( m_matchingCommands.at( 0 ) )->matchesExact( m_keys )
        && !m_commands.at( m_matchingCommands.at( 0 ) )->needsMotion() ) {
      //kDebug( 13070 ) << "Running command at index " << m_matchingCommands.at( 0 );

      KateViCommand *cmd = m_commands.at( m_matchingCommands.at( 0 ) );
      executeCommand( cmd );

      // check if reset() should be called. some commands in visual mode should not end visual mode
      if ( cmd->shouldReset() ) {
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

void KateViNormalMode::goToPos( const KateViRange &r )
{
  KTextEditor::Cursor c;
  c.setLine( r.endLine );
  c.setColumn( r.endColumn );

  if ( r.jump ) {
    addCurrentPositionToJumpList();
  }

  if ( c.line() >= m_doc->lines() ) {
    c.setLine( m_doc->lines()-1 );
  }

  updateCursor( c );
}

void KateViNormalMode::executeCommand( const KateViCommand* cmd )
{
  cmd->execute();

  // if the command was a change, and it didn't enter insert mode, store the key presses so that
  // they can be repeated with '.'
  if ( m_viInputModeManager->getCurrentViMode() != InsertMode ) {
    if ( cmd->isChange() && !m_viInputModeManager->isRunningMacro() ) {
      m_viInputModeManager->storeChangeCommand();
    }

    m_viInputModeManager->clearLog();
  }

  // make sure the cursor does not end up after the end of the line
  KTextEditor::Cursor c( m_view->cursorPosition() );
  if ( m_viInputModeManager->getCurrentViMode() == NormalMode ) {
    int lineLength = m_doc->lineLength( c.line() );

    if ( c.column() >= lineLength ) {
      if ( lineLength == 0 ) {
        c.setColumn( 0 );
      } else {
        c.setColumn( lineLength-1 );
      }
    }
    updateCursor( c );
  }
}

void KateViNormalMode::addCurrentPositionToJumpList()
{
    KTextEditor::Cursor c( m_view->cursorPosition() );

    KateSmartCursor *cursor = m_doc->smartManager()->newSmartCursor( c );

    m_marks->insert( '\'', cursor );
}

////////////////////////////////////////////////////////////////////////////////
// COMMANDS AND OPERATORS
////////////////////////////////////////////////////////////////////////////////

/**
 * enter insert mode at the cursor position
 */

bool KateViNormalMode::commandEnterInsertMode()
{
  return startInsertMode();
}

/**
 * enter insert mode after the current character
 */

bool KateViNormalMode::commandEnterInsertModeAppend()
{
  KTextEditor::Cursor c( m_view->cursorPosition() );
  c.setColumn( c.column()+1 );

  // if empty line, the cursor should start at column 0
  if ( getLine( c.line() ).length() == 0 ) {
    c.setColumn( 0 );
  }

  updateCursor( c );

  return startInsertMode();
}

/**
 * start insert mode after the last character of the line
 */

bool KateViNormalMode::commandEnterInsertModeAppendEOL()
{
  KTextEditor::Cursor c( m_view->cursorPosition() );
  c.setColumn( m_doc->lineLength( c.line() ) );
  updateCursor( c );

  return startInsertMode();
}

bool KateViNormalMode::commandEnterInsertModeBeforeFirstCharacterOfLine()
{
  KTextEditor::Cursor cursor( m_view->cursorPosition() );
  QRegExp nonSpace( "\\S" );
  int c = getLine().indexOf( nonSpace );
  if ( c == -1 ) {
    c = 0;
  }
  cursor.setColumn( c );
  updateCursor( cursor );

  return startInsertMode();
}

bool KateViNormalMode::commandEnterVisualLineMode()
{
  if ( m_viInputModeManager->getCurrentViMode() == VisualLineMode ) {
    reset();
    return true;
  }

  return startVisualLineMode();
}

bool KateViNormalMode::commandEnterVisualMode()
{
  if ( m_viInputModeManager->getCurrentViMode() == VisualMode ) {
    reset();
    return true;
  }

  return startVisualMode();
}

bool KateViNormalMode::commandToOtherEnd()
{
  if ( m_viInputModeManager->getCurrentViMode() == VisualLineMode
      || m_viInputModeManager->getCurrentViMode() == VisualMode ) {
    m_viInputModeManager->getViVisualMode()->switchStartEnd();
    return true;
  }

  return false;
}

KateViRange KateViNormalMode::motionWordForward()
{
  KTextEditor::Cursor c( m_view->cursorPosition() );
  KateViRange r( c.line(), c.column(), ViMotion::ExclusiveMotion );

  // Special case: If we're already on the very last character in the document, the motion should be
  // inclusive so the last character gets included
  if ( c.line() == m_doc->lines()-1 && c.column() == m_doc->lineLength( c.line() )-1 ) {
    r.motionType = ViMotion::InclusiveMotion;
  } else {
    for ( unsigned int i = 0; i < getCount(); i++ ) {
      c = findNextWordStart( c.line(), c.column() );

      // stop when at the last char in the document
      if ( c.line() == m_doc->lines()-1 && c.column() == m_doc->lineLength( c.line() )-1 ) {
        // if we still haven't "used up the count", make the motion inclusive, so that the last char
        // is included
        if ( i < getCount() ) {
          r.motionType = ViMotion::InclusiveMotion;
        }
        break;
      }
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
    if ( c.line() == m_doc->lines()-1 && c.column() == m_doc->lineLength( c.line() )-1 ) {
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

KateViRange KateViNormalMode::motionToEndOfPrevWord()
{
    KTextEditor::Cursor c( m_view->cursorPosition() );
    KateViRange r( c.line(), c.column(), ViMotion::InclusiveMotion );

    for ( unsigned int i = 0; i < getCount(); i++ ) {
      c = findPrevWordEnd( c.line(), c.column() );

      // stop when at the first char in the document
      if ( c.line() == 0 && c.column() == 0 ) {
        break;
      }
    }

    r.endColumn = c.column();
    r.endLine = c.line();

    return r;
}

KateViRange KateViNormalMode::motionToEndOfPrevWORD()
{
    KTextEditor::Cursor c( m_view->cursorPosition() );
    KateViRange r( c.line(), c.column(), ViMotion::InclusiveMotion );

    for ( unsigned int i = 0; i < getCount(); i++ ) {
      c = findPrevWORDEnd( c.line(), c.column() );

      // stop when at the first char in the document
      if ( c.line() == 0 && c.column() == 0 ) {
        break;
      }
    }

    r.endColumn = c.column();
    r.endLine = c.line();

    return r;
}

bool KateViNormalMode::commandDeleteLine()
{
  KTextEditor::Cursor c( m_view->cursorPosition() );

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

  if ( c.line() > m_doc->lines()-1 ) {
    c.setLine( m_doc->lines()-1 );
  }

  c.setColumn( column );
  updateCursor( c );

  return ret;
}

bool KateViNormalMode::commandDelete()
{
  bool linewise = m_viInputModeManager->getCurrentViMode() == VisualLineMode
    || ( m_commandRange.startLine != m_commandRange.endLine
      && m_viInputModeManager->getCurrentViMode() != VisualMode );

  return deleteRange( m_commandRange, linewise );
}

bool KateViNormalMode::commandDeleteToEOL()
{
  KTextEditor::Cursor c( m_view->cursorPosition() );

  m_commandRange.endLine = c.line();
  m_commandRange.endColumn = m_doc->lineLength( c.line() );

  if ( m_viInputModeManager->getCurrentViMode() == NormalMode ) {
    m_commandRange.startLine = c.line();
    m_commandRange.startColumn = c.column();
  }

  bool linewise = ( m_viInputModeManager->getCurrentViMode() == VisualMode
      || m_viInputModeManager->getCurrentViMode() == VisualLineMode );

  bool r = deleteRange( m_commandRange, linewise );

  if ( !linewise ) {
    c.setColumn( m_doc->lineLength( c.line() )-1 );
  } else {
    c.setLine( m_commandRange.startLine-1 );
    c.setColumn( m_commandRange.startColumn );
  }

  // make sure cursor position is valid after deletion
  if ( c.line() < 0 ) {
    c.setLine( 0 );
  }
  if ( c.column() > m_doc->lineLength( c.line() )-1 ) {
    c.setColumn( m_doc->lineLength( c.line() )-1 );
  }
  if ( c.column() < 0 ) {
    c.setColumn( 0 );
  }

  updateCursor( c );

  return r;
}

bool KateViNormalMode::commandMakeLowercase()
{
  KTextEditor::Cursor c( m_view->cursorPosition() );

  int line1 = m_commandRange.startLine;
  int line2 = m_commandRange.endLine;

  if ( line1 == line2 ) { // characterwise
    int endColumn = ( m_commandRange.endColumn > c.column() ) ? m_commandRange.endColumn : c.column();
    int startColumn = ( m_commandRange.startColumn < c.column() ) ? m_commandRange.startColumn : c.column();
    if ( m_commandRange.isInclusive() )
      endColumn++;

    KTextEditor::Range range( KTextEditor::Cursor( c.line(), startColumn ), KTextEditor::Cursor( c.line(), endColumn ) );
    m_doc->replaceText( range, getLine().mid( startColumn, endColumn-startColumn ).toLower() );
  }
  else { // linewise
    QString s;
    for ( int i = ( line1 < line2 ? line1 : line2 ); i <= ( line1 < line2 ? line2 : line1 ); i++ ) {
       s.append( m_doc->line( i ).toLower() + '\n' );
    }

    // remove the laste \n
    s.chop( 1 );

    KTextEditor::Range range( KTextEditor::Cursor( c.line(), 0 ),
        KTextEditor::Cursor( line2, m_doc->lineLength( line2 ) ) );

    m_doc->replaceText( range, s );
  }

  updateCursor( c );

  return true;
}

bool KateViNormalMode::commandMakeLowercaseLine()
{
  KTextEditor::Cursor c( m_view->cursorPosition() );

  m_commandRange.startLine = c.line();
  m_commandRange.endLine = c.line();
  m_commandRange.startColumn = 0;
  m_commandRange.endColumn = m_doc->lineLength( c.line() );

  return commandMakeLowercase();
}

bool KateViNormalMode::commandMakeUppercase()
{
  KTextEditor::Cursor c( m_view->cursorPosition() );

  int line1 = m_commandRange.startLine;
  int line2 = m_commandRange.endLine;

  if ( line1 == line2 ) { // characterwise
    int endColumn = ( m_commandRange.endColumn > c.column() ) ? m_commandRange.endColumn : c.column();
    int startColumn = ( m_commandRange.startColumn < c.column() ) ? m_commandRange.startColumn : c.column();
    if ( m_commandRange.isInclusive() )
      endColumn++;

    KTextEditor::Range range( KTextEditor::Cursor( c.line(), startColumn ), KTextEditor::Cursor( c.line(), endColumn ) );
    m_doc->replaceText( range, getLine().mid( startColumn, endColumn-startColumn ).toUpper() );
  }
  else { // linewise
    QString s;
    for ( int i = ( line1 < line2 ? line1 : line2 ); i <= ( line1 < line2 ? line2 : line1 ); i++ ) {
       s.append( m_doc->line( i ).toUpper() + '\n' );
    }

    // remove the laste \n
    s.chop( 1 );

    KTextEditor::Range range( KTextEditor::Cursor( c.line(), 0 ),
        KTextEditor::Cursor( line2, m_doc->lineLength( line2 ) ) );

    m_doc->replaceText( range, s );
  }

  updateCursor( c );

  return true;
}

bool KateViNormalMode::commandMakeUppercaseLine()
{
  KTextEditor::Cursor c( m_view->cursorPosition() );

  m_commandRange.startLine = c.line();
  m_commandRange.endLine = c.line();
  m_commandRange.startColumn = 0;
  m_commandRange.endColumn = m_doc->lineLength( c.line() );

  return commandMakeUppercase();
}

bool KateViNormalMode::commandChangeCase()
{
  QString text;
  KTextEditor::Range range;
  KTextEditor::Cursor c( m_view->cursorPosition() );

  // in visual mode, the range is from start position to end position...
  if ( m_viInputModeManager->getCurrentViMode() == VisualMode ) {
    KTextEditor::Cursor c2 = m_viInputModeManager->getViVisualMode()->getStart();

    if ( c2 > c ) {
      c2.setColumn( c2.column()+1 );
    } else {
      c.setColumn( c.column()+1 );
    }

    range.setRange( c, c2 );
  // ... in visual line mode, the range is from column 0 on the first line to
  // the line length of the last line...
  } else if ( m_viInputModeManager->getCurrentViMode() == VisualLineMode ) {
    KTextEditor::Cursor c2 = m_viInputModeManager->getViVisualMode()->getStart();

    if ( c2 > c ) {
      c2.setColumn( m_doc->lineLength( c2.line() ) );
      c.setColumn( 0 );
    } else {
      c.setColumn( m_doc->lineLength( c.line() ) );
      c2.setColumn( 0 );
    }

    range.setRange( c, c2 );
  // ... and in normal mode the range is from the current positon to the
  // current position + count
  } else {
    KTextEditor::Cursor c2 = c;
    c2.setColumn( c.column()+getCount() );

    if ( c2.column() > m_doc->lineLength( c.line() ) ) {
      c2.setColumn( m_doc->lineLength( c.line() ) );
    }

    range.setRange( c, c2 );
  }

  // get the text the command should operate on
  text = m_doc->text ( range );

  // for every character, switch its case
  for ( int i = 0; i < text.length(); i++ ) {
    if ( text.at(i).isUpper() ) {
      text[i] = text.at(i).toLower();
    } else if ( text.at(i).isLower() ) {
      text[i] = text.at(i).toUpper();
    }
  }

  // replace the old text with the modified text
  m_doc->replaceText( range, text );

  // in normal mode, move the cursor to the right, in visual mode move the
  // cursor to the start of the selection
  if ( m_viInputModeManager->getCurrentViMode() == NormalMode ) {
    updateCursor( range.end() );
  } else {
    updateCursor( range.start() );
  }

  return true;
}

bool KateViNormalMode::commandOpenNewLineUnder()
{
  KTextEditor::Cursor c( m_view->cursorPosition() );

  c.setColumn( getLine().length() );
  updateCursor( c );

  for ( unsigned int i = 0; i < getCount(); i++ ) {
    m_doc->newLine( m_view );
  }

  startInsertMode();
  m_viewInternal->repaint ();

  return true;
}

bool KateViNormalMode::commandOpenNewLineOver()
{
  KTextEditor::Cursor c( m_view->cursorPosition() );

  if ( c.line() == 0 ) {
    for (unsigned int i = 0; i < getCount(); i++ ) {
      m_doc->insertLine( 0, QString() );
    }
    c.setColumn( 0 );
    c.setLine( 0 );
    updateCursor( c );
  } else {
    c.setLine( c.line()-1 );
    c.setColumn( getLine( c.line() ).length() );
    updateCursor( c );
    for ( unsigned int i = 0; i < getCount(); i++ ) {
        m_doc->newLine( m_view );
    }

    if ( getCount() > 1 ) {
      c = m_view->cursorPosition();
      c.setLine( c.line()-(getCount()-1 ) );
      updateCursor( c );
    }
    //c.setLine( c.line()-getCount() );
  }

  startInsertMode();
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
  if ( n > m_doc->lines()-1-c.line() ) {
      n = m_doc->lines()-1-c.line();
  }

  m_doc->joinLines( c.line(), c.line()+n );

  return true;
}

bool KateViNormalMode::commandChange()
{
  KTextEditor::Cursor c( m_view->cursorPosition() );

  bool linewise = ( m_commandRange.startLine != m_commandRange.endLine
      && m_viInputModeManager->getCurrentViMode() != VisualMode );

  m_doc->editStart();
  commandDelete();

  // if we deleted several lines, insert an empty line and put the cursor there
  if ( linewise ) {
    m_doc->insertLine( m_commandRange.startLine, QString() );
    c.setLine( m_commandRange.startLine );
    updateCursor( c );
  }
  m_doc->editEnd();

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
  updateCursor( c );

  m_doc->editStart();

  // if count >= 2 start by deleting the whole lines
  if ( getCount() >= 2 ) {
    KateViRange r( c.line(), 0, c.line()+getCount()-2, 0, ViMotion::InclusiveMotion );
    deleteRange( r );
  }

  // ... then delete the _contents_ of the last line, but keep the line
  KateViRange r( c.line(), c.column(), c.line(), m_doc->lineLength( c.line() )-1,
      ViMotion::InclusiveMotion );
  deleteRange( r, false, true );
  m_doc->editEnd();

  // ... then enter insert mode
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

  bool linewise = ( m_commandRange.startLine != m_commandRange.endLine
      && m_viInputModeManager->getCurrentViMode() != VisualMode );

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

  m_doc->insertText( c, textToInsert );

  updateCursor( cAfter );

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

  m_doc->insertText( c, textToInsert );

  updateCursor( cAfter );

  return true;
}

bool KateViNormalMode::commandDeleteChar()
{
    KTextEditor::Cursor c( m_view->cursorPosition() );
    KateViRange r( c.line(), c.column(), c.line(), c.column()+getCount(), ViMotion::ExclusiveMotion );

    if ( m_commandRange.startLine != -1 && m_commandRange.startColumn != -1 ) {
      r = m_commandRange;
    } else {
      if ( r.endColumn > m_doc->lineLength( r.startLine ) ) {
        r.endColumn = m_doc->lineLength( r.startLine );
      }
    }

    // should only delete entire lines if in visual line mode
    bool linewise = m_viInputModeManager->getCurrentViMode() == VisualLineMode;

    return deleteRange( r, linewise );
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

    // should only delete entire lines if in visual line mode
    bool linewise = m_viInputModeManager->getCurrentViMode() == VisualLineMode;

    return deleteRange( r, linewise );
}

bool KateViNormalMode::commandReplaceCharacter()
{
  KTextEditor::Cursor c1( m_view->cursorPosition() );
  KTextEditor::Cursor c2( m_view->cursorPosition() );

  c2.setColumn( c2.column()+1 );

  bool r = m_doc->replaceText( KTextEditor::Range( c1, c2 ), m_keys.right( 1 ) );

  updateCursor( c1 );

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
    m_doc->undo();
    return true;
}

bool KateViNormalMode::commandRedo()
{
    m_doc->redo();
    return true;
}

bool KateViNormalMode::commandSetMark()
{
  KTextEditor::Cursor c( m_view->cursorPosition() );

  KateSmartCursor *cursor = m_doc->smartManager()->newSmartCursor( c );

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
        m_doc->indent( m_view, c.line()+i, 1 );
    }

    return true;
}

bool KateViNormalMode::commandUnindentLine()
{
    KTextEditor::Cursor c( m_view->cursorPosition() );

    for ( unsigned int i = 0; i < getCount(); i++ ) {
        m_doc->indent( m_view, c.line()+i, -1 );
    }

    return true;
}

bool KateViNormalMode::commandIndentLines()
{
  KTextEditor::Cursor c( m_view->cursorPosition() );

  m_commandRange.normalize();

  int line1 = m_commandRange.startLine;
  int line2 = m_commandRange.endLine;

  m_doc->editStart();
  for ( int i = line1; i <= line2; i++ ) {
      m_doc->indent( m_view, i, 1 );
  }
  m_doc->editEnd();

  return true;
}

bool KateViNormalMode::commandUnindentLines()
{
  KTextEditor::Cursor c( m_view->cursorPosition() );

  m_commandRange.normalize();

  int line1 = m_commandRange.startLine;
  int line2 = m_commandRange.endLine;

  m_doc->editStart();
  for ( int i = line1; i <= line2; i++ ) {
      m_doc->indent( m_view, i, -1 );
  }
  m_doc->editEnd();

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
  QChar ch = getCharUnderCursor();

  if ( ch == QChar::Null ) {
      message( QString( "NUL" ) );
  } else {

    int code = ch.unicode();

    message( QString("<%1> %2,  Hex %3,  Octal %4")
        .arg( ch )                          // char
        .arg( code )                        // decimal
        .arg( code, 0, 16 )                 // hexadecimal
        .arg( code, 3, 8, QChar('0') ) );   // octal
  }

  return true;
}

bool KateViNormalMode::commandRepeatLastChange()
{
  resetParser();
  m_viInputModeManager->repeatLastChange();

  return true;
}

bool KateViNormalMode::commandAlignLine()
{
  const int line = m_view->cursorPosition().line();
  KTextEditor::Range alignRange( KTextEditor::Cursor(line, 0), KTextEditor::Cursor(line, 0) );

  m_doc->align( m_view, alignRange );

  return true;
}

bool KateViNormalMode::commandAlignLines()
{
  KTextEditor::Cursor c( m_view->cursorPosition() );
  m_commandRange.normalize();

  KTextEditor::Cursor start(m_commandRange.startLine, 0);
  KTextEditor::Cursor end(m_commandRange.endLine, 0);

  m_doc->align( m_view, KTextEditor::Range( start, end ) );

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
  m_stickyColumn = -1;
  KateViRange r( cursor.line(), cursor.column(), ViMotion::ExclusiveMotion );
  r.endColumn -= getCount();

  return r;
}

KateViRange KateViNormalMode::motionRight()
{
  KTextEditor::Cursor cursor ( m_view->cursorPosition() );
  m_stickyColumn = -1;
  KateViRange r( cursor.line(), cursor.column(), ViMotion::ExclusiveMotion );
  r.endColumn += getCount();

  return r;
}

KateViRange KateViNormalMode::motionToEOL()
{
  m_stickyColumn = -1;
  KTextEditor::Cursor c( m_view->cursorPosition() );
  KateViRange r( c.line(), getLine().length()-1, ViMotion::InclusiveMotion );

  return r;
}

KateViRange KateViNormalMode::motionToColumn0()
{
  m_stickyColumn = -1;
  KTextEditor::Cursor cursor ( m_view->cursorPosition() );
  KateViRange r( cursor.line(), 0, ViMotion::ExclusiveMotion );

  return r;
}

KateViRange KateViNormalMode::motionToFirstCharacterOfLine()
{
  m_stickyColumn = -1;

  KTextEditor::Cursor cursor ( m_view->cursorPosition() );
  QRegExp nonSpace( "\\S" );
  int c = getLine().indexOf( nonSpace );

  KateViRange r( cursor.line(), c, ViMotion::ExclusiveMotion );

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

  r.startColumn = cursor.column();
  r.startLine = cursor.line();
  r.endColumn = matchColumn;
  r.endLine = cursor.line();

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

  r.endColumn = matchColumn;
  r.endLine = cursor.line();

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

  r.endColumn = matchColumn-1;
  r.endLine = cursor.line();

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

  r.endColumn = matchColumn+1;
  r.endLine = cursor.line();

  return r;
}

KateViRange KateViNormalMode::motionToLineFirst()
{
    KateViRange r( getCount()-1, 0, ViMotion::InclusiveMotion );

    if ( r.endLine > m_doc->lines() - 1 ) {
      r.endLine = m_doc->lines() - 1;
    }
    r.jump = true;

    return r;
}

KateViRange KateViNormalMode::motionToLineLast()
{
  KateViRange r( m_doc->lines()-1, 0, ViMotion::InclusiveMotion );

  if ( getCount()-1 != 0 ) {
    r.endLine = getCount()-1;
  }

  if ( r.endLine > m_doc->lines() - 1 ) {
    r.endLine = m_doc->lines() - 1;
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

  if ( m_doc->lineLength( c.line() )-1 < (int)getCount()-1 ) {
      column = m_doc->lineLength( c.line() )-1;
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
  int lines = m_doc->lines();
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

    // move the cursor to the first bracket
    c.setColumn( n1+1 );
    updateCursor( c );

    // find the matching one
    c = m_viewInternal->findMatchingBracket();

    if ( c > m_view->cursorPosition() ) {
      c.setColumn( c.column()-1 );
    }
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

    c2.setColumn( c2.column()-1 ); // don't include the first char of the following word
    KateViRange r( c.line(), c.column(), ViMotion::InclusiveMotion );

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
  m_commands.push_back( new KateViCommand( this, "a", &KateViNormalMode::commandEnterInsertModeAppend, IS_CHANGE ) );
  m_commands.push_back( new KateViCommand( this, "A", &KateViNormalMode::commandEnterInsertModeAppendEOL, IS_CHANGE ) );
  m_commands.push_back( new KateViCommand( this, "i", &KateViNormalMode::commandEnterInsertMode, IS_CHANGE ) );
  m_commands.push_back( new KateViCommand( this, "I", &KateViNormalMode::commandEnterInsertModeBeforeFirstCharacterOfLine, IS_CHANGE ) );
  m_commands.push_back( new KateViCommand( this, "v", &KateViNormalMode::commandEnterVisualMode ) );
  m_commands.push_back( new KateViCommand( this, "V", &KateViNormalMode::commandEnterVisualLineMode ) );
  m_commands.push_back( new KateViCommand( this, "o", &KateViNormalMode::commandOpenNewLineUnder, IS_CHANGE ) );
  m_commands.push_back( new KateViCommand( this, "O", &KateViNormalMode::commandOpenNewLineOver, IS_CHANGE ) );
  m_commands.push_back( new KateViCommand( this, "J", &KateViNormalMode::commandJoinLines, IS_CHANGE ) );
  m_commands.push_back( new KateViCommand( this, "c", &KateViNormalMode::commandChange, IS_CHANGE | NEEDS_MOTION ) );
  m_commands.push_back( new KateViCommand( this, "C", &KateViNormalMode::commandChangeToEOL, IS_CHANGE ) );
  m_commands.push_back( new KateViCommand( this, "cc", &KateViNormalMode::commandChangeLine, IS_CHANGE ) );
  m_commands.push_back( new KateViCommand( this, "s", &KateViNormalMode::commandSubstituteChar, IS_CHANGE ) );
  m_commands.push_back( new KateViCommand( this, "S", &KateViNormalMode::commandSubstituteLine, IS_CHANGE ) );
  m_commands.push_back( new KateViCommand( this, "dd", &KateViNormalMode::commandDeleteLine, IS_CHANGE ) );
  m_commands.push_back( new KateViCommand( this, "d", &KateViNormalMode::commandDelete, IS_CHANGE | NEEDS_MOTION ) );
  m_commands.push_back( new KateViCommand( this, "D", &KateViNormalMode::commandDeleteToEOL, IS_CHANGE ) );
  m_commands.push_back( new KateViCommand( this, "x", &KateViNormalMode::commandDeleteChar, IS_CHANGE ) );
  m_commands.push_back( new KateViCommand( this, "X", &KateViNormalMode::commandDeleteCharBackward, IS_CHANGE ) );
  m_commands.push_back( new KateViCommand( this, "gu", &KateViNormalMode::commandMakeLowercase, IS_CHANGE | NEEDS_MOTION ) );
  m_commands.push_back( new KateViCommand( this, "guu", &KateViNormalMode::commandMakeLowercaseLine, IS_CHANGE ) );
  m_commands.push_back( new KateViCommand( this, "gU", &KateViNormalMode::commandMakeUppercase, IS_CHANGE | NEEDS_MOTION ) );
  m_commands.push_back( new KateViCommand( this, "gUU", &KateViNormalMode::commandMakeUppercaseLine, IS_CHANGE ) );
  m_commands.push_back( new KateViCommand( this, "y", &KateViNormalMode::commandYank, NEEDS_MOTION ) );
  m_commands.push_back( new KateViCommand( this, "yy", &KateViNormalMode::commandYankLine ) );
  m_commands.push_back( new KateViCommand( this, "Y", &KateViNormalMode::commandYankToEOL, NEEDS_MOTION ) );
  m_commands.push_back( new KateViCommand( this, "p", &KateViNormalMode::commandPaste, IS_CHANGE ) );
  m_commands.push_back( new KateViCommand( this, "P", &KateViNormalMode::commandPasteBefore, IS_CHANGE ) );
  m_commands.push_back( new KateViCommand( this, "r.", &KateViNormalMode::commandReplaceCharacter, IS_CHANGE | REGEX_PATTERN ) );
  m_commands.push_back( new KateViCommand( this, ":", &KateViNormalMode::commandSwitchToCmdLine ) );
  m_commands.push_back( new KateViCommand( this, "/", &KateViNormalMode::commandSearch ) );
  m_commands.push_back( new KateViCommand( this, "u", &KateViNormalMode::commandUndo) );
  m_commands.push_back( new KateViCommand( this, "<c-r>", &KateViNormalMode::commandRedo ) );
  m_commands.push_back( new KateViCommand( this, "U", &KateViNormalMode::commandRedo ) );
  m_commands.push_back( new KateViCommand( this, "m.", &KateViNormalMode::commandSetMark, REGEX_PATTERN ) );
  m_commands.push_back( new KateViCommand( this, "n", &KateViNormalMode::commandFindNext ) );
  m_commands.push_back( new KateViCommand( this, "N", &KateViNormalMode::commandFindPrev ) );
  m_commands.push_back( new KateViCommand( this, ">>", &KateViNormalMode::commandIndentLine, IS_CHANGE ) );
  m_commands.push_back( new KateViCommand( this, "<<", &KateViNormalMode::commandUnindentLine, IS_CHANGE ) );
  m_commands.push_back( new KateViCommand( this, ">", &KateViNormalMode::commandIndentLines, IS_CHANGE | NEEDS_MOTION ) );
  m_commands.push_back( new KateViCommand( this, "<", &KateViNormalMode::commandUnindentLines, IS_CHANGE | NEEDS_MOTION ) );
  m_commands.push_back( new KateViCommand( this, "<c-f>", &KateViNormalMode::commandScrollPageDown ) );
  m_commands.push_back( new KateViCommand( this, "<c-b>", &KateViNormalMode::commandScrollPageUp ) );
  m_commands.push_back( new KateViCommand( this, "ga", &KateViNormalMode::commandPrintCharacterCode, SHOULD_NOT_RESET ) );
  m_commands.push_back( new KateViCommand( this, ".", &KateViNormalMode::commandRepeatLastChange ) );
  m_commands.push_back( new KateViCommand( this, "==", &KateViNormalMode::commandAlignLine, IS_CHANGE ) );
  m_commands.push_back( new KateViCommand( this, "=", &KateViNormalMode::commandAlignLines, IS_CHANGE | NEEDS_MOTION) );
  m_commands.push_back( new KateViCommand( this, "~", &KateViNormalMode::commandChangeCase, IS_CHANGE ) );

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
  m_motions.push_back( new KateViMotion( this, "f.", &KateViNormalMode::motionFindChar, REGEX_PATTERN ) );
  m_motions.push_back( new KateViMotion( this, "F.", &KateViNormalMode::motionFindCharBackward, REGEX_PATTERN ) );
  m_motions.push_back( new KateViMotion( this, "t.", &KateViNormalMode::motionToChar, REGEX_PATTERN ) );
  m_motions.push_back( new KateViMotion( this, "T.", &KateViNormalMode::motionToCharBackward, REGEX_PATTERN ) );
  m_motions.push_back( new KateViMotion( this, "gg", &KateViNormalMode::motionToLineFirst ) );
  m_motions.push_back( new KateViMotion( this, "G", &KateViNormalMode::motionToLineLast ) );
  m_motions.push_back( new KateViMotion( this, "w", &KateViNormalMode::motionWordForward ) );
  m_motions.push_back( new KateViMotion( this, "W", &KateViNormalMode::motionWORDForward ) );
  m_motions.push_back( new KateViMotion( this, "b", &KateViNormalMode::motionWordBackward ) );
  m_motions.push_back( new KateViMotion( this, "B", &KateViNormalMode::motionWORDBackward ) );
  m_motions.push_back( new KateViMotion( this, "e", &KateViNormalMode::motionToEndOfWord ) );
  m_motions.push_back( new KateViMotion( this, "E", &KateViNormalMode::motionToEndOfWORD ) );
  m_motions.push_back( new KateViMotion( this, "ge", &KateViNormalMode::motionToEndOfPrevWord ) );
  m_motions.push_back( new KateViMotion( this, "gE", &KateViNormalMode::motionToEndOfPrevWORD ) );
  m_motions.push_back( new KateViMotion( this, "|", &KateViNormalMode::motionToScreenColumn ) );
  m_motions.push_back( new KateViMotion( this, "%", &KateViNormalMode::motionToMatchingItem ) );
  m_motions.push_back( new KateViMotion( this, "`[a-zA-Z]", &KateViNormalMode::motionToMark, REGEX_PATTERN ) );
  m_motions.push_back( new KateViMotion( this, "'[a-zA-Z]", &KateViNormalMode::motionToMarkLine, REGEX_PATTERN ) );
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
  m_motions.push_back( new KateViMotion( this, "i[()]", &KateViNormalMode::textObjectInnerParen, REGEX_PATTERN ) );
  m_motions.push_back( new KateViMotion( this, "a[()]", &KateViNormalMode::textObjectAParen, REGEX_PATTERN ) );
  m_motions.push_back( new KateViMotion( this, "i[\\[\\]]", &KateViNormalMode::textObjectInnerBracket, REGEX_PATTERN ) );
  m_motions.push_back( new KateViMotion( this, "a[\\[\\]]", &KateViNormalMode::textObjectABracket, REGEX_PATTERN ) );
}

QRegExp KateViNormalMode::generateMatchingItemRegex()
{
  QString pattern("\\[|\\]|\\{|\\}|\\(|\\)|");
  QList<QString> keys = m_matchingItems.keys();

  for ( int i = 0; i < keys.size(); i++ ) {
    QString s = m_matchingItems[ keys[ i ] ];
    s = s.replace( QRegExp( "^-" ), QChar() );
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
