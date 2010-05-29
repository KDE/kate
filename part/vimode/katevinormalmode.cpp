/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2008-2009 Erlend Hamberg <ehamberg@gmail.com>
 *  Copyright (C) 2008 Evgeniy Ivanov <powerfox@kde.ru>
 *  Copyright (C) 2009 Paul Gideon Dann <pdgiddie@gmail.com>
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

#include "katevinormalmode.h"
#include "katevivisualmode.h"
#include "kateviinputmodemanager.h"
#include "kateviglobal.h"
#include "kateglobal.h"
#include "katebuffer.h"
#include "kateviewhelpers.h"

#include <ktexteditor/movingcursor.h>
#include <QApplication>
#include <QList>

using KTextEditor::Cursor;
using KTextEditor::Range;

#define ADDCMD(STR,FUNC, FLGS) m_commands.push_back( \
    new KateViCommand( this, STR, &KateViNormalMode::FUNC, FLGS ) );

#define ADDMOTION(STR, FUNC, FLGS) m_motions.push_back( new \
    KateViMotion( this, STR, &KateViNormalMode::FUNC, FLGS ) );

KateViNormalMode::KateViNormalMode( KateViInputModeManager *viInputModeManager, KateView * view,
    KateViewInternal * viewInternal ) : KateViModeBase()
{
  m_view = view;
  m_viewInternal = viewInternal;
  m_viInputModeManager = viInputModeManager;
  m_stickyColumn = -1;

  // FIXME: make configurable:
  m_extraWordCharacters = "";
  m_matchingItems["/*"] = "*/";
  m_matchingItems["*/"] = "-/*";

  m_matchItemRegex = generateMatchingItemRegex();

  m_defaultRegister = '"';

  m_timeoutlen = 1000; // FIXME: make configurable
  m_mappingKeyPress = false; // temporarily set to true when an aborted mapping sends key presses
  m_mappingTimer = new QTimer( this );
  connect(m_mappingTimer, SIGNAL(timeout()), this, SLOT(mappingTimerTimeOut()));

  initializeCommands();
  resetParser(); // initialise with start configuration
}

KateViNormalMode::~KateViNormalMode()
{
  // delete the text cursors
  qDeleteAll( m_marks );

  qDeleteAll( m_commands );
  qDeleteAll( m_motions) ;
}

void KateViNormalMode::mappingTimerTimeOut()
{
  kDebug( 13070 ) << "timeout! key presses: " << m_mappingKeys;
  m_mappingKeyPress = true;
  m_viInputModeManager->feedKeyPresses( m_mappingKeys );
  m_mappingKeyPress = false;
  m_mappingKeys.clear();
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

  QChar key = KateViKeyParser::getInstance()->KeyEventToQChar( keyCode, text, e->modifiers(), e->nativeScanCode() );

  // check for matching mappings
  if ( !m_mappingKeyPress && m_matchingCommands.size() == 0 ) {
    m_mappingKeys.append( key );

    foreach ( const QString &str, getMappings() ) {
      if ( str.startsWith( m_mappingKeys ) ) {
        if ( str == m_mappingKeys ) {
          m_viInputModeManager->feedKeyPresses( getMapping( str ) );
          m_mappingTimer->stop();
          return true;
        } else {
          m_mappingTimer->start( m_timeoutlen );
          m_mappingTimer->setSingleShot( true );
          return true;
        }
      }
    }
    m_mappingKeys.clear();
  } else {
    // FIXME:
    //m_mappingKeyPress = false; // key press ignored wrt mappings, re-set m_mappingKeyPress
  }

  m_keysVerbatim.append( KateViKeyParser::getInstance()->decodeKeySequence( key ) );

  QChar c = QChar::Null;
  if ( m_keys.size() > 0 ) {
    c = m_keys.at( m_keys.size()-1 ); // last char
  }

  if ( ( keyCode >= Qt::Key_0 && keyCode <= Qt::Key_9 && c != '"' )       // key 0-9
      && ( m_countTemp != 0 || keyCode != Qt::Key_0 )                     // first digit can't be 0
      && ( c != 'f' && c != 't' && c != 'F' && c != 'T' && c != 'r' ) ) { // "find char" motions

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
    Cursor c1( m_view->cursorPosition() ); // current position
    Cursor c2 = findWordEnd(c1.line(), c1.column()-1, true); // word end

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

            // jump over folding regions since we are just moving the cursor
            int currLine = m_view->cursorPosition().line();
            int delta = r.endLine - currLine;
            int vline = doc()->foldingTree()->getVirtualLine( currLine );
            r.endLine = doc()->foldingTree()->getRealLine( vline+delta );
            if ( r.endLine >= doc()->lines() ) r.endLine = doc()->lines()-1;

            // make sure the position is valid before moving the cursor there
            if ( r.valid
                && r.endLine >= 0
                && ( r.endLine == 0 || r.endLine <= doc()->lines()-1 )
                && r.endColumn >= 0
                && ( r.endColumn == 0 || r.endColumn < doc()->lineLength( r.endLine ) ) ) {
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
              Cursor c( m_view->cursorPosition() );
              m_commandRange.startLine = c.line();
              m_commandRange.startColumn = c.column();
            }

            // Special case: "word motions" should never cross a line boundary when they are the
            // input to a command
            if ( ( m_keys.right(1) == "w" || m_keys.right(1) == "W" )
                && m_commandRange.endLine > m_commandRange.startLine ) {
              m_commandRange = motionToEOL();

              Cursor c( m_view->cursorPosition() );
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
      resetParser();
      return false;
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
  Cursor c;
  c.setLine( r.endLine );
  c.setColumn( r.endColumn );

  if ( r.jump ) {
    addCurrentPositionToJumpList();
  }

  if ( c.line() >= doc()->lines() ) {
    c.setLine( doc()->lines()-1 );
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
  Cursor c( m_view->cursorPosition() );
  if ( m_viInputModeManager->getCurrentViMode() == NormalMode ) {
    int lineLength = doc()->lineLength( c.line() );

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
    Cursor c( m_view->cursorPosition() );

    // delete old cursor if any
    if (KTextEditor::MovingCursor *oldCursor = m_marks.value('\''))
      delete oldCursor;

    // create and remember new one
    KTextEditor::MovingCursor *cursor = doc()->newMovingCursor( c );
    m_marks.insert( '\'', cursor );
}

////////////////////////////////////////////////////////////////////////////////
// COMMANDS AND OPERATORS
////////////////////////////////////////////////////////////////////////////////

/**
 * enter insert mode at the cursor position
 */

bool KateViNormalMode::commandEnterInsertMode()
{
  m_stickyColumn = -1;
  return startInsertMode();
}

/**
 * enter insert mode after the current character
 */

bool KateViNormalMode::commandEnterInsertModeAppend()
{
  Cursor c( m_view->cursorPosition() );
  c.setColumn( c.column()+1 );

  // if empty line, the cursor should start at column 0
  if ( doc()->lineLength( c.line() ) == 0 ) {
    c.setColumn( 0 );
  }

  // cursor should never be in a column > number of columns
  if ( c.column() > doc()->lineLength( c.line() ) ) {
    c.setColumn( doc()->lineLength( c.line() ) );
  }

  updateCursor( c );

  m_stickyColumn = -1;
  return startInsertMode();
}

/**
 * start insert mode after the last character of the line
 */

bool KateViNormalMode::commandEnterInsertModeAppendEOL()
{
  Cursor c( m_view->cursorPosition() );
  c.setColumn( doc()->lineLength( c.line() ) );
  updateCursor( c );

  m_stickyColumn = -1;
  return startInsertMode();
}

bool KateViNormalMode::commandEnterInsertModeBeforeFirstNonBlankInLine()
{
  Cursor cursor( m_view->cursorPosition() );
  QRegExp nonSpace( "\\S" );
  int c = getLine().indexOf( nonSpace );
  if ( c == -1 ) {
    c = 0;
  }
  cursor.setColumn( c );
  updateCursor( cursor );

  m_stickyColumn = -1;
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

bool KateViNormalMode::commandEnterVisualBlockMode()
{
  if ( m_viInputModeManager->getCurrentViMode() == VisualBlockMode ) {
    reset();
    return true;
  }

  return startVisualBlockMode();
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
  if ( m_viInputModeManager->getCurrentViMode() == VisualMode
      || m_viInputModeManager->getCurrentViMode() == VisualLineMode
      || m_viInputModeManager->getCurrentViMode() == VisualBlockMode ) {
    m_viInputModeManager->getViVisualMode()->switchStartEnd();
    return true;
  }

  return false;
}

bool KateViNormalMode::commandEnterReplaceMode()
{
  return startReplaceMode();
}

bool KateViNormalMode::commandDeleteLine()
{
  Cursor c( m_view->cursorPosition() );

  KateViRange r;

  r.startLine = c.line();
  r.endLine = c.line()+getCount()-1;

  int column = c.column();

  bool ret = deleteRange( r, true );

  c = m_view->cursorPosition();
  if ( column > doc()->lineLength( c.line() )-1 ) {
    column = doc()->lineLength( c.line() )-1;
  }
  if ( column < 0 ) {
    column = 0;
  }

  if ( c.line() > doc()->lines()-1 ) {
    c.setLine( doc()->lines()-1 );
  }

  c.setColumn( column );
  m_stickyColumn = -1;
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
  Cursor c( m_view->cursorPosition() );

  m_commandRange.endLine = c.line()+getCount()-1;
  m_commandRange.endColumn = doc()->lineLength( m_commandRange.endLine )-1;

  if ( m_viInputModeManager->getCurrentViMode() == NormalMode ) {
    m_commandRange.startLine = c.line();
    m_commandRange.startColumn = c.column();
  }

  bool linewise = ( m_viInputModeManager->getCurrentViMode() == VisualMode
      || m_viInputModeManager->getCurrentViMode() == VisualLineMode );

  bool r = deleteRange( m_commandRange, linewise );

  if ( !linewise ) {
    c.setColumn( doc()->lineLength( c.line() )-1 );
  } else {
    c.setLine( m_commandRange.startLine-1 );
    c.setColumn( m_commandRange.startColumn );
  }

  // make sure cursor position is valid after deletion
  if ( c.line() < 0 ) {
    c.setLine( 0 );
  }
  if ( c.column() > doc()->lineLength( c.line() )-1 ) {
    c.setColumn( doc()->lineLength( c.line() )-1 );
  }
  if ( c.column() < 0 ) {
    c.setColumn( 0 );
  }

  updateCursor( c );

  return r;
}

bool KateViNormalMode::commandMakeLowercase()
{
  bool linewise = ( m_commandRange.startLine != m_commandRange.endLine
      && m_viInputModeManager->getCurrentViMode() != VisualMode );

  QString text = getRange( m_commandRange, linewise );
  QString lowerCase = text.toLower();

  Cursor start( m_commandRange.startLine, m_commandRange.startColumn );
  Cursor end( m_commandRange.endLine, m_commandRange.endColumn );
  Range range( start, end );

  doc()->replaceText( range, lowerCase );

  return true;
}

bool KateViNormalMode::commandMakeLowercaseLine()
{
  Cursor c( m_view->cursorPosition() );

  m_commandRange.startLine = c.line();
  m_commandRange.endLine = c.line();
  m_commandRange.startColumn = 0;
  m_commandRange.endColumn = doc()->lineLength( c.line() );

  return commandMakeLowercase();
}

bool KateViNormalMode::commandMakeUppercase()
{
  bool linewise = ( m_commandRange.startLine != m_commandRange.endLine
      && m_viInputModeManager->getCurrentViMode() != VisualMode );

  QString text = getRange( m_commandRange, linewise );
  QString upperCase = text.toUpper();

  Cursor start( m_commandRange.startLine, m_commandRange.startColumn );
  Cursor end( m_commandRange.endLine, m_commandRange.endColumn );
  Range range( start, end );

  doc()->replaceText( range, upperCase );

  return true;
}

bool KateViNormalMode::commandMakeUppercaseLine()
{
  Cursor c( m_view->cursorPosition() );

  m_commandRange.startLine = c.line();
  m_commandRange.endLine = c.line();
  m_commandRange.startColumn = 0;
  m_commandRange.endColumn = doc()->lineLength( c.line() );

  return commandMakeUppercase();
}

bool KateViNormalMode::commandChangeCase()
{
  QString text;
  Range range;
  Cursor c( m_view->cursorPosition() );

  // in visual mode, the range is from start position to end position...
  if ( m_viInputModeManager->getCurrentViMode() == VisualMode ) {
    Cursor c2 = m_viInputModeManager->getViVisualMode()->getStart();

    if ( c2 > c ) {
      c2.setColumn( c2.column()+1 );
    } else {
      c.setColumn( c.column()+1 );
    }

    range.setRange( c, c2 );
  // ... in visual line mode, the range is from column 0 on the first line to
  // the line length of the last line...
  } else if ( m_viInputModeManager->getCurrentViMode() == VisualLineMode ) {
    Cursor c2 = m_viInputModeManager->getViVisualMode()->getStart();

    if ( c2 > c ) {
      c2.setColumn( doc()->lineLength( c2.line() ) );
      c.setColumn( 0 );
    } else {
      c.setColumn( doc()->lineLength( c.line() ) );
      c2.setColumn( 0 );
    }

    range.setRange( c, c2 );
  // ... and in normal mode the range is from the current position to the
  // current position + count
  } else {
    Cursor c2 = c;
    c2.setColumn( c.column()+getCount() );

    if ( c2.column() > doc()->lineLength( c.line() ) ) {
      c2.setColumn( doc()->lineLength( c.line() ) );
    }

    range.setRange( c, c2 );
  }

  // get the text the command should operate on
  text = doc()->text ( range );

  // for every character, switch its case
  for ( int i = 0; i < text.length(); i++ ) {
    if ( text.at(i).isUpper() ) {
      text[i] = text.at(i).toLower();
    } else if ( text.at(i).isLower() ) {
      text[i] = text.at(i).toUpper();
    }
  }

  // replace the old text with the modified text
  doc()->replaceText( range, text );

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
  Cursor c( m_view->cursorPosition() );

  c.setColumn( doc()->lineLength( c.line() ) );
  updateCursor( c );

  for ( unsigned int i = 0; i < getCount(); i++ ) {
    doc()->newLine( m_view );
  }

  m_stickyColumn = -1;
  startInsertMode();
  m_viewInternal->repaint ();

  return true;
}

bool KateViNormalMode::commandOpenNewLineOver()
{
  Cursor c( m_view->cursorPosition() );

  if ( c.line() == 0 ) {
    for (unsigned int i = 0; i < getCount(); i++ ) {
      doc()->insertLine( 0, QString() );
    }
    c.setColumn( 0 );
    c.setLine( 0 );
    updateCursor( c );
  } else {
    c.setLine( c.line()-1 );
    c.setColumn( getLine( c.line() ).length() );
    updateCursor( c );
    for ( unsigned int i = 0; i < getCount(); i++ ) {
        doc()->newLine( m_view );
    }

    if ( getCount() > 1 ) {
      c = m_view->cursorPosition();
      c.setLine( c.line()-(getCount()-1 ) );
      updateCursor( c );
    }
    //c.setLine( c.line()-getCount() );
  }

  m_stickyColumn = -1;
  startInsertMode();
  m_viewInternal->repaint ();

  return true;
}

bool KateViNormalMode::commandJoinLines()
{
  Cursor c( m_view->cursorPosition() );

  // remember line length so the cursor can be put between the joined lines
  int l = doc()->lineLength( c.line() );

  int n = getCount();

  // if we were given a range of lines, this information overrides the previous
  if ( m_commandRange.startLine != -1 && m_commandRange.endLine != -1 ) {
    m_commandRange.normalize();
    c.setLine ( m_commandRange.startLine );
    n = m_commandRange.endLine-m_commandRange.startLine;
  }

  // make sure we don't try to join lines past the document end
  if ( n > doc()->lines()-1-c.line() ) {
      n = doc()->lines()-1-c.line();
  }

  doc()->joinLines( c.line(), c.line()+n );

  // position cursor between the joined lines
  c.setColumn( l );
  updateCursor( c );

  return true;
}

bool KateViNormalMode::commandChange()
{
  Cursor c( m_view->cursorPosition() );

  bool linewise = ( m_commandRange.startLine != m_commandRange.endLine
      && m_viInputModeManager->getCurrentViMode() != VisualMode );

  doc()->editStart();
  commandDelete();

  // if we deleted several lines, insert an empty line and put the cursor there
  if ( linewise ) {
    doc()->insertLine( m_commandRange.startLine, QString() );
    c.setLine( m_commandRange.startLine );
    c.setColumn(0);
  }
  doc()->editEnd();

  if ( linewise ) {
    updateCursor( c );
  }

  commandEnterInsertMode();

  // correct indentation level
  if ( linewise ) {
    m_view->align();
  }

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
  Cursor c( m_view->cursorPosition() );
  c.setColumn( 0 );
  updateCursor( c );

  doc()->editStart();

  // if count >= 2 start by deleting the whole lines
  if ( getCount() >= 2 ) {
    KateViRange r( c.line(), 0, c.line()+getCount()-2, 0, ViMotion::InclusiveMotion );
    deleteRange( r );
  }

  // ... then delete the _contents_ of the last line, but keep the line
  KateViRange r( c.line(), c.column(), c.line(), doc()->lineLength( c.line() )-1,
      ViMotion::InclusiveMotion );
  deleteRange( r, false, true );
  doc()->editEnd();

  // ... then enter insert mode
  commandEnterInsertModeAppend();

  // correct indentation level
  m_view->align();

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
  Cursor c( m_view->cursorPosition() );

  bool r = false;
  QString yankedText;

  bool linewise = m_viInputModeManager->getCurrentViMode() == VisualLineMode
    || ( m_commandRange.startLine != m_commandRange.endLine
      && m_viInputModeManager->getCurrentViMode() != VisualMode );

  yankedText = getRange( m_commandRange, linewise );

  fillRegister( getChosenRegister( '0' ), yankedText );

  return r;
}

bool KateViNormalMode::commandYankLine()
{
  Cursor c( m_view->cursorPosition() );
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
  Cursor c( m_view->cursorPosition() );

  bool r = false;
  QString yankedText;

  m_commandRange.endLine = c.line()+getCount()-1;
  m_commandRange.endColumn = doc()->lineLength( m_commandRange.endLine )-1;

  bool linewise = ( m_viInputModeManager->getCurrentViMode() == VisualMode
      || m_viInputModeManager->getCurrentViMode() == VisualLineMode );

  if ( m_viInputModeManager->getCurrentViMode() == NormalMode ) {
    m_commandRange.startLine = c.line();
    m_commandRange.startColumn = c.column();
  }

  yankedText = getRange( m_commandRange, linewise );

  fillRegister( getChosenRegister( '0' ), yankedText );

  return r;
}

// insert the text in the given register at the cursor position
// the cursor should end up at the beginning of what was pasted
bool KateViNormalMode::commandPaste()
{
  Cursor c( m_view->cursorPosition() );
  Cursor cAfter = c;
  QChar reg = getChosenRegister( m_defaultRegister );

  QString textToInsert = getRegisterContent( reg );

  if ( textToInsert.isNull() ) {
    error(i18n("Nothing in register %1", reg ));
    return false;
  }

  if ( getCount() > 1 ) {
    textToInsert = textToInsert.repeated( getCount() );
  }

  if ( textToInsert.endsWith('\n') ) { // line(s)
    textToInsert.chop( 1 ); // remove the last \n
    c.setColumn( doc()->lineLength( c.line() ) ); // paste after the current line and ...
    textToInsert.prepend( QChar( '\n' ) ); // ... prepend a \n, so the text starts on a new line

    cAfter.setLine( cAfter.line()+1 );
    cAfter.setColumn( 0 );
  } else {
    if ( getLine( c.line() ).length() > 0 ) {
      c.setColumn( c.column()+1 );
    }

    cAfter = c;
  }

  doc()->insertText( c, textToInsert );

  updateCursor( cAfter );

  return true;
}

// insert the text in the given register before the cursor position
// the cursor should end up at the beginning of what was pasted
bool KateViNormalMode::commandPasteBefore()
{
  Cursor c( m_view->cursorPosition() );
  Cursor cAfter = c;
  QChar reg = getChosenRegister( m_defaultRegister );

  QString textToInsert = getRegisterContent( reg );

  if ( getCount() > 1 ) {
    textToInsert = textToInsert.repeated( getCount() );
  }

  if ( textToInsert.endsWith('\n') ) { // lines
    c.setColumn( 0 );
    cAfter.setColumn( 0 );
  }

  doc()->insertText( c, textToInsert );

  updateCursor( cAfter );

  return true;
}

bool KateViNormalMode::commandDeleteChar()
{
    Cursor c( m_view->cursorPosition() );
    KateViRange r( c.line(), c.column(), c.line(), c.column()+getCount(), ViMotion::ExclusiveMotion );

    if ( m_commandRange.startLine != -1 && m_commandRange.startColumn != -1 ) {
      r = m_commandRange;
    } else {
      if ( r.endColumn > doc()->lineLength( r.startLine ) ) {
        r.endColumn = doc()->lineLength( r.startLine );
      }
    }

    // should only delete entire lines if in visual line mode
    bool linewise = m_viInputModeManager->getCurrentViMode() == VisualLineMode;

    return deleteRange( r, linewise );
}

bool KateViNormalMode::commandDeleteCharBackward()
{
    Cursor c( m_view->cursorPosition() );

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
  Cursor c1( m_view->cursorPosition() );
  Cursor c2( m_view->cursorPosition() );

  c2.setColumn( c2.column()+1 );

  bool r = doc()->replaceText( Range( c1, c2 ), m_keys.right( 1 ) );

  updateCursor( c1 );

  return r;
}

bool KateViNormalMode::commandSwitchToCmdLine()
{
    Cursor c( m_view->cursorPosition() );

    m_view->switchToCmdLine();

    // if a count is given, the range [current line] to [current line] + count should be prepended
    // to the command line
    if ( getCount() != 1 ) {
      m_view->cmdLineBar()->setText( ".,.+" +QString::number( getCount()-1 ), false);
    }

    return true;
}

bool KateViNormalMode::commandSearch()
{
    m_view->find();
    return true;
}

bool KateViNormalMode::commandUndo()
{
    doc()->undo();
    return true;
}

bool KateViNormalMode::commandRedo()
{
    doc()->redo();
    return true;
}

bool KateViNormalMode::commandSetMark()
{
  Cursor c( m_view->cursorPosition() );

  // delete old cursor if any
  if (KTextEditor::MovingCursor *oldCursor = m_marks.value(m_keys.at( m_keys.size()-1 )))
    delete oldCursor;

  // create and remember new one
  KTextEditor::MovingCursor *cursor = doc()->newMovingCursor( c );
  m_marks.insert( m_keys.at( m_keys.size()-1 ), cursor );

  kDebug( 13070 ) << "set mark at (" << c.line() << "," << c.column() << ")";

  return true;
}

bool KateViNormalMode::commandIndentLine()
{
    Cursor c( m_view->cursorPosition() );

    for ( unsigned int i = 0; i < getCount(); i++ ) {
        doc()->indent( KTextEditor::Range( c.line()+i, 0, c.line()+i, 0), 1 );
    }

    return true;
}

bool KateViNormalMode::commandUnindentLine()
{
    Cursor c( m_view->cursorPosition() );

    for ( unsigned int i = 0; i < getCount(); i++ ) {
        doc()->indent( KTextEditor::Range( c.line()+i, 0, c.line()+i, 0), -1 );
    }

    return true;
}

bool KateViNormalMode::commandIndentLines()
{
  Cursor c( m_view->cursorPosition() );

  m_commandRange.normalize();

  int line1 = m_commandRange.startLine;
  int line2 = m_commandRange.endLine;
  int col = getLine( line2 ).length();

  doc()->editStart();
  doc()->indent( KTextEditor::Range( line1, 0, line2, col ), 1 );
  doc()->editEnd();

  return true;
}

bool KateViNormalMode::commandUnindentLines()
{
  Cursor c( m_view->cursorPosition() );

  m_commandRange.normalize();

  int line1 = m_commandRange.startLine;
  int line2 = m_commandRange.endLine;

  doc()->editStart();
  doc()->indent( KTextEditor::Range( line1, 0, line2, 0), -1 );
  doc()->editEnd();

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

bool KateViNormalMode::commandCentreViewOnCursor()
{
  Cursor c( m_view->cursorPosition() );
  int linesToScroll = (m_viewInternal->endLine()-linesDisplayed()/2)-c.line();

  scrollViewLines( -linesToScroll );

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

    QString dec = QString::number( code );
    QString hex = QString::number( code, 16 );
    QString oct = QString::number( code, 8 );
    if ( oct.length() < 3 ) { oct.prepend( '0' ); }
    message( i18n("'%1' %2,  Hex %3,  Octal %4", ch, dec, hex, oct ) );
  }

  return true;
}

bool KateViNormalMode::commandRepeatLastChange()
{
  resetParser();
  doc()->editStart();
  m_viInputModeManager->repeatLastChange();
  doc()->editEnd();

  return true;
}

bool KateViNormalMode::commandAlignLine()
{
  const int line = m_view->cursorPosition().line();
  Range alignRange( Cursor(line, 0), Cursor(line, 0) );

  doc()->align( m_view, alignRange );

  return true;
}

bool KateViNormalMode::commandAlignLines()
{
  Cursor c( m_view->cursorPosition() );
  m_commandRange.normalize();

  Cursor start(m_commandRange.startLine, 0);
  Cursor end(m_commandRange.endLine, 0);

  doc()->align( m_view, Range( start, end ) );

  return true;
}

bool KateViNormalMode::commandAddToNumber()
{
    addToNumberUnderCursor( getCount() );

    return true;
}

bool KateViNormalMode::commandSubtractFromNumber()
{
    addToNumberUnderCursor( -getCount() );

    return true;
}

////////////////////////////////////////////////////////////////////////////////
// MOTIONS
////////////////////////////////////////////////////////////////////////////////

KateViRange KateViNormalMode::motionDown()
{
  return goLineDown();
}


KateViRange KateViNormalMode::motionUp()
{
  return goLineUp();
}

KateViRange KateViNormalMode::motionLeft()
{
  Cursor cursor ( m_view->cursorPosition() );
  m_stickyColumn = -1;
  KateViRange r( cursor.line(), cursor.column(), ViMotion::ExclusiveMotion );
  r.endColumn -= getCount();

  return r;
}

KateViRange KateViNormalMode::motionRight()
{
  Cursor cursor ( m_view->cursorPosition() );
  m_stickyColumn = -1;
  KateViRange r( cursor.line(), cursor.column(), ViMotion::ExclusiveMotion );
  r.endColumn += getCount();

  return r;
}

KateViRange KateViNormalMode::motionPageDown()
{
  Cursor c( m_view->cursorPosition() );
  int linesToScroll = linesDisplayed();

  KateViRange r( c.line()+linesToScroll, c.column(), ViMotion::InclusiveMotion );

  if ( r.endLine >= doc()->lines() ) {
    r.endLine = doc()->lines()-1;
  }

  return r;
}

KateViRange KateViNormalMode::motionPageUp()
{
  Cursor c( m_view->cursorPosition() );
  int linesToScroll = linesDisplayed();

  KateViRange r( c.line()-linesToScroll, c.column(), ViMotion::InclusiveMotion );

  if ( r.endLine < 0 ) {
    r.endLine = 0;
  }

  return r;
}

KateViRange KateViNormalMode::motionDownToFirstNonBlank()
{
  Cursor c( m_view->cursorPosition() );
  KateViRange r = goLineDown();

  r.endColumn = getLine( r.endLine ).indexOf( QRegExp( "\\S" ) );

  if ( r.endColumn < 0 ) {
    r.endColumn = 0;
  }

  return r;
}

KateViRange KateViNormalMode::motionUpToFirstNonBlank()
{
  Cursor c( m_view->cursorPosition() );
  KateViRange r = goLineUp();

  r.endColumn = getLine( r.endLine ).indexOf( QRegExp( "\\S" ) );

  if ( r.endColumn < 0 ) {
    r.endColumn = 0;
  }

  return r;
}

KateViRange KateViNormalMode::motionWordForward()
{
  Cursor c( m_view->cursorPosition() );
  KateViRange r( c.line(), c.column(), ViMotion::ExclusiveMotion );

  m_stickyColumn = -1;

  // Special case: If we're already on the very last character in the document, the motion should be
  // inclusive so the last character gets included
  if ( c.line() == doc()->lines()-1 && c.column() == doc()->lineLength( c.line() )-1 ) {
    r.motionType = ViMotion::InclusiveMotion;
  } else {
    for ( unsigned int i = 0; i < getCount(); i++ ) {
      c = findNextWordStart( c.line(), c.column() );

      // stop when at the last char in the document
      if ( c.line() == doc()->lines()-1 && c.column() == doc()->lineLength( c.line() )-1 ) {
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
  Cursor c( m_view->cursorPosition() );
  KateViRange r( c.line(), c.column(), ViMotion::ExclusiveMotion );

  m_stickyColumn = -1;

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
  Cursor c( m_view->cursorPosition() );
  KateViRange r( c.line(), c.column(), ViMotion::ExclusiveMotion );

  m_stickyColumn = -1;

  for ( unsigned int i = 0; i < getCount(); i++ ) {
    c = findNextWORDStart( c.line(), c.column() );

    // stop when at the last char in the document
    if ( c.line() == doc()->lines()-1 && c.column() == doc()->lineLength( c.line() )-1 ) {
      break;
    }
  }

  r.endColumn = c.column();
  r.endLine = c.line();

  return r;
}

KateViRange KateViNormalMode::motionWORDBackward()
{
  Cursor c( m_view->cursorPosition() );
  KateViRange r( c.line(), c.column(), ViMotion::ExclusiveMotion );

  m_stickyColumn = -1;

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
    Cursor c( m_view->cursorPosition() );
    KateViRange r( c.line(), c.column(), ViMotion::InclusiveMotion );

    m_stickyColumn = -1;

    for ( unsigned int i = 0; i < getCount(); i++ ) {
        c = findWordEnd( c.line(), c.column() );
    }

    r.endColumn = c.column();
    r.endLine = c.line();

    return r;
}

KateViRange KateViNormalMode::motionToEndOfWORD()
{
    Cursor c( m_view->cursorPosition() );
    KateViRange r( c.line(), c.column(), ViMotion::InclusiveMotion );

    m_stickyColumn = -1;

    for ( unsigned int i = 0; i < getCount(); i++ ) {
        c = findWORDEnd( c.line(), c.column() );
    }

    r.endColumn = c.column();
    r.endLine = c.line();

    return r;
}

KateViRange KateViNormalMode::motionToEndOfPrevWord()
{
    Cursor c( m_view->cursorPosition() );
    KateViRange r( c.line(), c.column(), ViMotion::InclusiveMotion );

    m_stickyColumn = -1;

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
    Cursor c( m_view->cursorPosition() );
    KateViRange r( c.line(), c.column(), ViMotion::InclusiveMotion );

    m_stickyColumn = -1;

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

KateViRange KateViNormalMode::motionToEOL()
{
  Cursor c( m_view->cursorPosition() );

  // set sticky column to a rediculously high value so that the cursor will stick to EOL,
  // but only if it's a regular motion
  if ( m_keys.size() == 1 ) {
    m_stickyColumn = 100000;
  }

  unsigned int line = c.line() + ( getCount() - 1 );
  KateViRange r( line, doc()->lineLength(line )-1, ViMotion::InclusiveMotion );

  return r;
}

KateViRange KateViNormalMode::motionToColumn0()
{
  m_stickyColumn = -1;
  Cursor cursor ( m_view->cursorPosition() );
  KateViRange r( cursor.line(), 0, ViMotion::ExclusiveMotion );

  return r;
}

KateViRange KateViNormalMode::motionToFirstCharacterOfLine()
{
  m_stickyColumn = -1;

  Cursor cursor ( m_view->cursorPosition() );
  QRegExp nonSpace( "\\S" );
  int c = getLine().indexOf( nonSpace );

  KateViRange r( cursor.line(), c, ViMotion::ExclusiveMotion );

  return r;
}

KateViRange KateViNormalMode::motionFindChar()
{
  m_lastTFcommand = m_keys;
  Cursor cursor ( m_view->cursorPosition() );
  QString line = getLine();

  m_stickyColumn = -1;

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
  m_lastTFcommand = m_keys;
  Cursor cursor ( m_view->cursorPosition() );
  QString line = getLine();

  m_stickyColumn = -1;

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
  m_lastTFcommand = m_keys;
  Cursor cursor ( m_view->cursorPosition() );
  QString line = getLine();

  m_stickyColumn = -1;

  int matchColumn = cursor.column()+1;

  for ( unsigned int i = 0; i < getCount(); i++ ) {
    matchColumn = line.indexOf( m_keys.right( 1 ), matchColumn );
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
  m_lastTFcommand = m_keys;
  Cursor cursor ( m_view->cursorPosition() );
  QString line = getLine();

  m_stickyColumn = -1;

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

  r.endColumn = matchColumn+1;
  r.endLine = cursor.line();

  return r;
}

KateViRange KateViNormalMode::motionRepeatlastTF()
{
  if ( !m_lastTFcommand.isEmpty() ) {
    m_keys = m_lastTFcommand;
    if ( m_keys.at( 0 ) == 'f' ) {
      return motionFindChar();
    }
    else if ( m_keys.at( 0 ) == 'F' ) {
      return motionFindCharBackward();
    }
    else if ( m_keys.at( 0 ) == 't' ) {
      return motionToChar();
    }
    else if ( m_keys.at( 0 ) == 'T' ) {
      return motionToCharBackward();
    }
  }

  // there was no previous t/f command

  KateViRange r;
  r.valid = false;

  return r;
}

KateViRange KateViNormalMode::motionRepeatlastTFBackward()
{
  if ( !m_lastTFcommand.isEmpty() ) {
    m_keys = m_lastTFcommand;
    if ( m_keys.at( 0 ) == 'f' ) {
      return motionFindCharBackward();
    }
    else if ( m_keys.at( 0 ) == 'F' ) {
      return motionFindChar();
    }
    else if ( m_keys.at( 0 ) == 't' ) {
      return motionToCharBackward();
    }
    else if ( m_keys.at( 0 ) == 'T' ) {
      return motionToChar();
    }
  }

  // there was no previous t/f command

  KateViRange r;
  r.valid = false;

  return r;
}

// FIXME: should honour the provided count
KateViRange KateViNormalMode::motionFindPrev()
{
  QString pattern = m_viInputModeManager->getLastSearchPattern();
  bool backwards = m_viInputModeManager->lastSearchBackwards();

  return findPattern( pattern, !backwards, getCount() );
}

KateViRange KateViNormalMode::motionFindNext()
{
  QString pattern = m_viInputModeManager->getLastSearchPattern();
  bool backwards = m_viInputModeManager->lastSearchBackwards();

  return findPattern( pattern, backwards, getCount() );
}


KateViRange KateViNormalMode::motionToLineFirst()
{
    KateViRange r( getCount()-1, 0, ViMotion::InclusiveMotion );
    m_stickyColumn = -1;

    if ( r.endLine > doc()->lines() - 1 ) {
      r.endLine = doc()->lines() - 1;
    }
    r.jump = true;

    return r;
}

KateViRange KateViNormalMode::motionToLineLast()
{
  KateViRange r( doc()->lines()-1, 0, ViMotion::InclusiveMotion );
  m_stickyColumn = -1;

  // don't use getCount() here, no count and a count of 1 is different here...
  if ( m_count != 0 ) {
    r.endLine = m_count-1;
  }

  if ( r.endLine > doc()->lines() - 1 ) {
    r.endLine = doc()->lines() - 1;
  }
  r.jump = true;

  return r;
}

KateViRange KateViNormalMode::motionToScreenColumn()
{
  m_stickyColumn = -1;

  Cursor c( m_view->cursorPosition() );

  int column = getCount()-1;

  if ( doc()->lineLength( c.line() )-1 < (int)getCount()-1 ) {
      column = doc()->lineLength( c.line() )-1;
  }

  return KateViRange( c.line(), column, ViMotion::ExclusiveMotion );
}

KateViRange KateViNormalMode::motionToMark()
{
  KateViRange r;

  m_stickyColumn = -1;

  QChar reg = m_keys.at( m_keys.size()-1 );

  // ` and ' is the same register (position before jump)
  if ( reg == '`' ) {
      reg = '\'';
  }

  if ( m_marks.contains( reg ) ) {
    KTextEditor::MovingCursor *cursor = m_marks.value( reg );
    r.endLine = cursor->line();
    r.endColumn = cursor->column();
  } else {
    error(i18n("Mark not set: %1",m_keys.right( 1 ) ));
    r.valid = false;
  }

  r.jump = true;

  return r;
}

KateViRange KateViNormalMode::motionToMarkLine()
{
  KateViRange r = motionToMark();
  r.endColumn = 0; // FIXME: should be first non-blank on line

  m_stickyColumn = -1;

  r.jump = true;

  return r;
}

KateViRange KateViNormalMode::motionToMatchingItem()
{
  KateViRange r;
  Cursor c( m_view->cursorPosition() );
  int lines = doc()->lines();
  QString l = getLine();
  int n1 = l.indexOf( m_matchItemRegex, c.column() );
  int n2;

  m_stickyColumn = -1;

  if ( n1 == -1 ) {
    r.valid = false;
    return r;
  } else {
    n2 = l.indexOf( QRegExp( "\\b|\\s|$" ), n1 );
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

  m_stickyColumn = -1;

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

  m_stickyColumn = -1;

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

  m_stickyColumn = -1;

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

  m_stickyColumn = -1;

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

KateViRange KateViNormalMode::motionToNextOccurrence()
{
  QString word = getWordUnderCursor();
  word.prepend("\\b").append("\\b");

  m_viInputModeManager->setLastSearchPattern( word );
  m_viInputModeManager->setLastSearchBackwards( false );

  return findPattern( word );
}

KateViRange KateViNormalMode::motionToPrevOccurrence()
{
  QString word = getWordUnderCursor();
  word.prepend("\\b").append("\\b");

  m_viInputModeManager->setLastSearchPattern( word );
  m_viInputModeManager->setLastSearchBackwards( true );

  return findPattern( word, true );
}


////////////////////////////////////////////////////////////////////////////////
// TEXT OBJECTS
////////////////////////////////////////////////////////////////////////////////

KateViRange KateViNormalMode::textObjectAWord()
{
    Cursor c( m_view->cursorPosition() );

    Cursor c1 = findPrevWordStart( c.line(), c.column()+1, true );
    Cursor c2( c );

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
    Cursor c( m_view->cursorPosition() );

    Cursor c1 = findPrevWordStart( c.line(), c.column()+1, true );
    // need to start search in column-1 because it might be a one-character word
    Cursor c2( c.line(), c.column()-1 );

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
    Cursor c( m_view->cursorPosition() );

    Cursor c1 = findPrevWORDStart( c.line(), c.column()+1, true );
    Cursor c2( c );

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
    Cursor c( m_view->cursorPosition() );

    Cursor c1 = findPrevWORDStart( c.line(), c.column()+1, true );
    Cursor c2( c );

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

KateViRange KateViNormalMode::textObjectAComma()
{
    KateViRange r = findSurrounding( ',', ',', false );

    if ( !r.valid ) {
      r = findSurrounding( QRegExp(","), QRegExp("[\\])}]"), false );
    }

    if ( !r.valid ) {
      r = findSurrounding( QRegExp("[\\[({]"), QRegExp(","), false );
    }

    return r;
}

KateViRange KateViNormalMode::textObjectInnerComma()
{
    KateViRange r = findSurrounding( ',', ',', true );

    if ( !r.valid ) {
      r = findSurrounding( QRegExp(","), QRegExp("[\\])}]"), true );
    }

    if ( !r.valid ) {
      r = findSurrounding( QRegExp("[\\[({]"), QRegExp(","), true );
    }

    return r;
}

// add commands
// when adding commands here, remember to add them to visual mode too (if applicable)
void KateViNormalMode::initializeCommands()
{
  ADDCMD("a", commandEnterInsertModeAppend, IS_CHANGE );
  ADDCMD("A", commandEnterInsertModeAppendEOL, IS_CHANGE );
  ADDCMD("i", commandEnterInsertMode, IS_CHANGE );
  ADDCMD("I", commandEnterInsertModeBeforeFirstNonBlankInLine, IS_CHANGE );
  ADDCMD("v", commandEnterVisualMode, 0 );
  ADDCMD("V", commandEnterVisualLineMode, 0 );
//ADDCMD("<c-v>", commandEnterVisualBlockMode, 0 );
  ADDCMD("o", commandOpenNewLineUnder, IS_CHANGE );
  ADDCMD("O", commandOpenNewLineOver, IS_CHANGE );
  ADDCMD("J", commandJoinLines, IS_CHANGE );
  ADDCMD("c", commandChange, IS_CHANGE | NEEDS_MOTION );
  ADDCMD("C", commandChangeToEOL, IS_CHANGE );
  ADDCMD("cc", commandChangeLine, IS_CHANGE );
  ADDCMD("s", commandSubstituteChar, IS_CHANGE );
  ADDCMD("S", commandSubstituteLine, IS_CHANGE );
  ADDCMD("dd", commandDeleteLine, IS_CHANGE );
  ADDCMD("d", commandDelete, IS_CHANGE | NEEDS_MOTION );
  ADDCMD("D", commandDeleteToEOL, IS_CHANGE );
  ADDCMD("x", commandDeleteChar, IS_CHANGE );
  ADDCMD("X", commandDeleteCharBackward, IS_CHANGE );
  ADDCMD("gu", commandMakeLowercase, IS_CHANGE | NEEDS_MOTION );
  ADDCMD("guu", commandMakeLowercaseLine, IS_CHANGE );
  ADDCMD("gU", commandMakeUppercase, IS_CHANGE | NEEDS_MOTION );
  ADDCMD("gUU", commandMakeUppercaseLine, IS_CHANGE );
  ADDCMD("y", commandYank, NEEDS_MOTION );
  ADDCMD("yy", commandYankLine, 0 );
  ADDCMD("Y", commandYankToEOL, 0 );
  ADDCMD("p", commandPaste, IS_CHANGE );
  ADDCMD("P", commandPasteBefore, IS_CHANGE );
  ADDCMD("r.", commandReplaceCharacter, IS_CHANGE | REGEX_PATTERN );
  ADDCMD("R", commandEnterReplaceMode, IS_CHANGE );
  ADDCMD(":", commandSwitchToCmdLine, 0 );
  ADDCMD("/", commandSearch, 0 );
  ADDCMD("u", commandUndo, 0);
  ADDCMD("<c-r>", commandRedo, 0 );
  ADDCMD("U", commandRedo, 0 );
  ADDCMD("m.", commandSetMark, REGEX_PATTERN );
  ADDCMD(">>", commandIndentLine, IS_CHANGE );
  ADDCMD("<<", commandUnindentLine, IS_CHANGE );
  ADDCMD(">", commandIndentLines, IS_CHANGE | NEEDS_MOTION );
  ADDCMD("<", commandUnindentLines, IS_CHANGE | NEEDS_MOTION );
  ADDCMD("<c-f>", commandScrollPageDown, 0 );
  ADDCMD("<pagedown>", commandScrollPageDown, 0 );
  ADDCMD("<c-b>", commandScrollPageUp, 0 );
  ADDCMD("<pageup>", commandScrollPageUp, 0 );
  ADDCMD("zz", commandCentreViewOnCursor, 0 );
  ADDCMD("ga", commandPrintCharacterCode, SHOULD_NOT_RESET );
  ADDCMD(".", commandRepeatLastChange, 0 );
  ADDCMD("==", commandAlignLine, IS_CHANGE );
  ADDCMD("=", commandAlignLines, IS_CHANGE | NEEDS_MOTION);
  ADDCMD("~", commandChangeCase, IS_CHANGE );
  ADDCMD("<c-a>", commandAddToNumber, IS_CHANGE );
  ADDCMD("<c-x>", commandSubtractFromNumber, IS_CHANGE );

  // regular motions
  ADDMOTION("h", motionLeft, 0 );
  ADDMOTION("<left>", motionLeft, 0 );
  ADDMOTION("<backspace>", motionLeft, 0 );
  ADDMOTION("j", motionDown, 0 );
  ADDMOTION("<down>", motionDown, 0 );
  ADDMOTION("<enter>", motionDownToFirstNonBlank, 0 );
  ADDMOTION("k", motionUp, 0 );
  ADDMOTION("<up>", motionUp, 0 );
  ADDMOTION("-", motionUpToFirstNonBlank, 0 );
  ADDMOTION("l", motionRight, 0 );
  ADDMOTION("<right>", motionRight, 0 );
  ADDMOTION(" ", motionRight, 0 );
  ADDMOTION("$", motionToEOL, 0 );
  ADDMOTION("<end>", motionToEOL, 0 );
  ADDMOTION("0", motionToColumn0, 0 );
  ADDMOTION("<home>", motionToColumn0, 0 );
  ADDMOTION("^", motionToFirstCharacterOfLine, 0 );
  ADDMOTION("f.", motionFindChar, REGEX_PATTERN );
  ADDMOTION("F.", motionFindCharBackward, REGEX_PATTERN );
  ADDMOTION("t.", motionToChar, REGEX_PATTERN );
  ADDMOTION("T.", motionToCharBackward, REGEX_PATTERN );
  ADDMOTION(";", motionRepeatlastTF, 0 );
  ADDMOTION(",", motionRepeatlastTFBackward, 0 );
  ADDMOTION("n", motionFindNext, 0 );
  ADDMOTION("N", motionFindPrev, 0 );
  ADDMOTION("gg", motionToLineFirst, 0 );
  ADDMOTION("G", motionToLineLast, 0 );
  ADDMOTION("w", motionWordForward, 0 );
  ADDMOTION("W", motionWORDForward, 0 );
  ADDMOTION("b", motionWordBackward, 0 );
  ADDMOTION("B", motionWORDBackward, 0 );
  ADDMOTION("e", motionToEndOfWord, 0 );
  ADDMOTION("E", motionToEndOfWORD, 0 );
  ADDMOTION("ge", motionToEndOfPrevWord, 0 );
  ADDMOTION("gE", motionToEndOfPrevWORD, 0 );
  ADDMOTION("|", motionToScreenColumn, 0 );
  ADDMOTION("%", motionToMatchingItem, 0 );
  ADDMOTION("`[a-zA-Z]", motionToMark, REGEX_PATTERN );
  ADDMOTION("'[a-zA-Z]", motionToMarkLine, REGEX_PATTERN );
  ADDMOTION("[[", motionToPreviousBraceBlockStart, 0 );
  ADDMOTION("]]", motionToNextBraceBlockStart, 0 );
  ADDMOTION("[]", motionToPreviousBraceBlockEnd, 0 );
  ADDMOTION("][", motionToNextBraceBlockEnd, 0 );
  ADDMOTION("*", motionToNextOccurrence, 0 );
  ADDMOTION("#", motionToPrevOccurrence, 0 );

  // text objects
  ADDMOTION("iw", textObjectInnerWord, 0 );
  ADDMOTION("aw", textObjectAWord, 0 );
  ADDMOTION("i\"", textObjectInnerQuoteDouble, 0 );
  ADDMOTION("a\"", textObjectAQuoteDouble, 0 );
  ADDMOTION("i'", textObjectInnerQuoteSingle, 0 );
  ADDMOTION("a'", textObjectAQuoteSingle, 0 );
  ADDMOTION("i[()]", textObjectInnerParen, REGEX_PATTERN );
  ADDMOTION("a[()]", textObjectAParen, REGEX_PATTERN );
  ADDMOTION("i[\\[\\]]", textObjectInnerBracket, REGEX_PATTERN );
  ADDMOTION("a[\\[\\]]", textObjectABracket, REGEX_PATTERN );
  ADDMOTION("i,", textObjectInnerComma, 0 );
  ADDMOTION("a,", textObjectAComma, 0 );
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
      pattern.append( '|' );
    }
  }

  return QRegExp( pattern );
}

void KateViNormalMode::addMapping( const QString &from, const QString &to )
{
    KateGlobal::self()->viInputModeGlobal()->addMapping( NormalMode, from, to );
}

const QString KateViNormalMode::getMapping( const QString &from ) const
{
    return KateGlobal::self()->viInputModeGlobal()->getMapping( NormalMode, from );
}

const QStringList KateViNormalMode::getMappings() const
{
    return KateGlobal::self()->viInputModeGlobal()->getMappings( NormalMode );
}

