/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2008-2009 Erlend Hamberg <ehamberg@gmail.com>
 *  Copyright (C) 2008 Evgeniy Ivanov <powerfox@kde.ru>
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

#include "katevinormalmode.h"
#include "katevivisualmode.h"
#include "kateviinsertmode.h"
#include "kateviinputmodemanager.h"
#include "kateviglobal.h"
#include "katevikeymapper.h"
#include "kateviemulatedcommandbar.h"
#include "kateglobal.h"
#include "kateconfig.h"
#include "katebuffer.h"
#include "kateviewhelpers.h"
#include <kateundomanager.h>
#include <ktexteditor/attribute.h>
#include <katecompletionwidget.h>
#include "kateconfig.h"

#include <QApplication>
#include <QList>

using KTextEditor::Cursor;
using KTextEditor::Range;

#define ADDCMD(STR, FUNC, FLGS) m_commands.push_back( \
    new KateViCommand( this, STR, &KateViNormalMode::FUNC, FLGS ) );

#define ADDMOTION(STR, FUNC, FLGS) m_motions.push_back( \
    new KateViMotion( this, STR, &KateViNormalMode::FUNC, FLGS ) );

KateViNormalMode::KateViNormalMode( KateViInputModeManager *viInputModeManager, KateView * view,
    KateViewInternal * viewInternal ) : KateViModeBase()
{
  m_view = view;
  m_viewInternal = viewInternal;
  m_viInputModeManager = viInputModeManager;
  m_stickyColumn = -1;
  m_lastMotionWasVisualLineUpOrDown = false;
  m_currentMotionWasVisualLineUpOrDown = false;

  // FIXME: make configurable
  m_extraWordCharacters = "";
  m_matchingItems["/*"] = "*/";
  m_matchingItems["*/"] = "-/*";

  m_matchItemRegex = generateMatchingItemRegex();

  m_defaultRegister = '"';

  m_scroll_count_limit = 1000; // Limit of count for scroll commands.

  initializeCommands();
  m_pendingResetIsDueToExit = false;
  m_isRepeatedTFcommand = false;
  m_lastMotionWasLinewiseInnerBlock = false;
  m_motionCanChangeWholeVisualModeSelection = false;
  resetParser(); // initialise with start configuration

  m_isUndo = false;
  connect(doc()->undoManager(), SIGNAL(undoStart(KTextEditor::Document*)),
          this, SLOT(undoBeginning()));
  connect(doc()->undoManager(), SIGNAL(undoEnd(KTextEditor::Document*)),
          this, SLOT(undoEnded()));

  updateYankHighlightAttrib();
  connect(view, SIGNAL(configChanged()),
          this, SLOT(updateYankHighlightAttrib()));
  connect(doc(), SIGNAL(aboutToInvalidateMovingInterfaceContent(KTextEditor::Document*)),
          this, SLOT(clearYankHighlight()));
  connect(doc(), SIGNAL(aboutToDeleteMovingInterfaceContent(KTextEditor::Document*)),
          this, SLOT(aboutToDeleteMovingInterfaceContent()));
}

KateViNormalMode::~KateViNormalMode()
{
  qDeleteAll( m_commands );
  qDeleteAll( m_motions) ;
  qDeleteAll(m_highlightedYanks);
}

/**
 * parses a key stroke to check if it's a valid (part of) a command
 * @return true if a command was completed and executed, false otherwise
 */
bool KateViNormalMode::handleKeypress( const QKeyEvent *e )
{
  const int keyCode = e->key();
  const QString text = e->text();

  // ignore modifier keys alone
  if ( keyCode == Qt::Key_Shift || keyCode == Qt::Key_Control
      || keyCode == Qt::Key_Alt || keyCode == Qt::Key_Meta ) {
    return false;
  }

  clearYankHighlight();

  if ( keyCode == Qt::Key_Escape || (keyCode == Qt::Key_C && e->modifiers() == Qt::ControlModifier) || (keyCode == Qt::Key_BracketLeft && e->modifiers() == Qt::ControlModifier)) {
    m_view->setCaretStyle( KateRenderer::Block, true );
    m_pendingResetIsDueToExit = true;
    // Vim in weird as if we e.g. i<ctrl-o><ctrl-c> it claims (in the status bar) to still be in insert mode,
    // but behaves as if it's in normal mode. I'm treating the status bar thing as a bug and just exiting
    // insert mode altogether.
    m_viInputModeManager->setTemporaryNormalMode(false);
    reset();
    return true;
  }

  const QChar key = KateViKeyParser::self()->KeyEventToQChar(*e);

  const QChar lastChar = m_keys.isEmpty() ?  QChar::Null : m_keys.at(m_keys.size() - 1);
  const bool waitingForRegisterOrCharToSearch = this->waitingForRegisterOrCharToSearch();

  // Use replace caret when reading a character for "r"
  if ( key == 'r' && !waitingForRegisterOrCharToSearch) {
    m_view->setCaretStyle( KateRenderer::Underline, true );
  }

  m_keysVerbatim.append( KateViKeyParser::self()->decodeKeySequence( key ) );

  if ( ( keyCode >= Qt::Key_0 && keyCode <= Qt::Key_9 && lastChar != '"' )       // key 0-9
      && ( m_countTemp != 0 || keyCode != Qt::Key_0 )                     // first digit can't be 0
      && (!waitingForRegisterOrCharToSearch) // Not in the middle of "find char" motions or replacing char.
      && e->modifiers() == Qt::NoModifier) {

    m_countTemp *= 10;
    m_countTemp += keyCode-Qt::Key_0;

    return true;
  } else if ( m_countTemp != 0 ) {
    m_count = getCount() * m_countTemp;
    m_countTemp = 0;
    m_iscounted = true;

    kDebug( 13070 ) << "count = " << getCount();
  }

  m_keys.append( key );

  if (m_viInputModeManager->isRecordingMacro() && key == 'q')
  {
    // Need to special case this "finish macro" q, as the "begin macro" q
    // needs a parameter whereas the finish macro does not.
    m_viInputModeManager->finishRecordingMacro();
    resetParser();
    return true;
  }

  if ((key == '/' || key == '?') && !waitingForRegisterOrCharToSearch)
  {
    // Special case for "/" and "?": these should be motions, but this is complicated by
    // the fact that the user must interact with the search bar before the range of the
    // motion can be determined.
    // We hack around this by showing the search bar immediately, and, when the user has
    // finished interacting with it, have the search bar send a "synthetic" keypresses
    // that will either abort everything (if the search was aborted) or "complete" the motion
    // otherwise.
    m_positionWhenIncrementalSearchBegan = m_view->cursorPosition();
    if (key == '/')
    {
      commandSearchForward();
    }
    else
    {
      commandSearchBackward();
    }
    return true;
  }

  // Special case: "cw" and "cW" work the same as "ce" and "cE" if the cursor is
  // on a non-blank.  This is because Vim interprets "cw" as change-word, and a
  // word does not include the following white space. (:help cw in vim)
  if ( ( m_keys == "cw" || m_keys == "cW" ) && !getCharUnderCursor().isSpace() ) {
    // Special case of the special case: :-)
    // If the cursor is at the end of the current word rewrite to "cl"
    const bool isWORD = (m_keys.at(1) == 'W');
    const Cursor currentPosition( m_view->cursorPosition() );
    const Cursor endOfWordOrWORD = (isWORD ? findWORDEnd(currentPosition.line(), currentPosition.column()-1, true) :
                                             findWordEnd(currentPosition.line(), currentPosition.column()-1, true));

    if ( currentPosition == endOfWordOrWORD ) {
      m_keys = "cl";
    } else {
      if (isWORD) {
        m_keys = "cE";
      } else {
        m_keys = "ce";
      }
    }
  }

  if ( m_keys[ 0 ] == Qt::Key_QuoteDbl ) {
    if ( m_keys.size() < 2 ) {
      return true; // waiting for a register
    }
    else {
      QChar r = m_keys[ 1 ].toLower();

      if ( ( r >= '0' && r <= '9' ) || ( r >= 'a' && r <= 'z' ) ||
           r == '_' || r == '+' || r == '*' || r == '#' || r == '^' ) {
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

  // Use operator-pending caret when reading a motion for an operator
  // in normal mode. We need to check that we are indeed in normal mode
  // since visual mode inherits from it.
  if( m_viInputModeManager->getCurrentViMode() == NormalMode &&
      !m_awaitingMotionOrTextObject.isEmpty() ) {
    m_view->setCaretStyle( KateRenderer::Half, true );
  }

  //kDebug( 13070 ) << "checkFrom: " << checkFrom;

  // look for matching motion commands from position 'checkFrom'
  // FIXME: if checkFrom hasn't changed, only motions whose index is in
  // m_matchingMotions should be checked
  bool motionExecuted = false;
  if ( checkFrom < m_keys.size() ) {
    for ( int i = 0; i < m_motions.size(); i++ ) {
      //kDebug( 13070 )  << "\tchecking " << m_keys.mid( checkFrom )  << " against " << m_motions.at( i )->pattern();
      if ( m_motions.at( i )->matches( m_keys.mid( checkFrom ) ) ) {
        m_lastMotionWasLinewiseInnerBlock = false;
        //kDebug( 13070 )  << m_keys.mid( checkFrom ) << " matches!";
        m_matchingMotions.push_back( i );

        // if it matches exact, we have found the motion command to execute
        if ( m_motions.at( i )->matchesExact( m_keys.mid( checkFrom ) ) ) {
          m_currentMotionWasVisualLineUpOrDown = false;
          motionExecuted = true;
          if ( checkFrom == 0 ) {
            // no command given before motion, just move the cursor to wherever
            // the motion says it should go to
            KateViRange r = m_motions.at( i )->execute();
            m_motionCanChangeWholeVisualModeSelection = m_motions.at( i )->canChangeWholeVisualModeSelection();

            // jump over folding regions since we are just moving the cursor
            int currLine = m_view->cursorPosition().line();
            int delta = r.endLine - currLine;
            int vline = m_view->textFolding().lineToVisibleLine( currLine );
            r.endLine = m_view->textFolding().visibleLineToLine( qMax (vline+delta, 0) /* ensure we have a valid line */ );
            if ( r.endLine >= doc()->lines() ) r.endLine = doc()->lines()-1;

            // make sure the position is valid before moving the cursor there
            // TODO: can this be simplified? :/
            if ( r.valid
                && r.endLine >= 0
                && ( r.endLine == 0 || r.endLine <= doc()->lines()-1 )
                && r.endColumn >= 0 ) {
               if ( r.endColumn >= doc()->lineLength( r.endLine )
                   && doc()->lineLength( r.endLine ) > 0 ) {
                   r.endColumn = doc()->lineLength( r.endLine ) - 1;
               }

              kDebug( 13070 ) << "No command given, going to position ("
                << r.endLine << "," << r.endColumn << ")";
              goToPos( r );

              // in the case of VisualMode we need to remember the motion commands as well.
              if (!m_viInputModeManager->isAnyVisualMode())
                m_viInputModeManager->clearCurrentChangeLog();
            } else {
              kDebug( 13070 ) << "Invalid position: (" << r.endLine << "," << r.endColumn << ")";
            }

            resetParser();

            // if normal mode was started by using Ctrl-O in insert mode,
            // it's time to go back to insert mode.
            if (m_viInputModeManager->getTemporaryNormalMode()) {
                startInsertMode();
                m_viewInternal->repaint();
            }

            m_lastMotionWasVisualLineUpOrDown = m_currentMotionWasVisualLineUpOrDown;

            break;
          } else {
            // execute the specified command and supply the position returned from
            // the motion

            m_commandRange = m_motions.at( i )->execute();
            m_linewiseCommand = m_motions.at( i )->isLineWise();

            // if we didn't get an explicit start position, use the current cursor position
            if ( m_commandRange.startLine == -1 ) {
              Cursor c( m_view->cursorPosition() );
              m_commandRange.startLine = c.line();
              m_commandRange.startColumn = c.column();
            }

            // special case: When using the "w" motion in combination with an operator and
            // the last word moved over is at the end of a line, the end of that word
            // becomes the end of the operated text, not the first word in the next line.
            if (m_motions.at(i)->pattern() == "w" || m_motions.at(i)->pattern() == "W") {
               if (m_commandRange.endLine != m_commandRange.startLine &&
                   m_commandRange.endColumn == getFirstNonBlank(m_commandRange.endLine)) {
                     m_commandRange.endLine--;
                     m_commandRange.endColumn = doc()->lineLength(m_commandRange.endLine );
                   }
            }

            m_commandWithMotion = true;

            if ( m_commandRange.valid ) {
              kDebug( 13070 ) << "Run command" << m_commands.at( m_motionOperatorIndex )->pattern()
                << "from (" << m_commandRange.startLine << "," << m_commandRange.startColumn << ")"
                << "to (" << m_commandRange.endLine << "," << m_commandRange.endColumn << ")";
              executeCommand( m_commands.at( m_motionOperatorIndex ) );
            } else {
              kDebug( 13070 ) << "Invalid range: "
                << "from (" << m_commandRange.startLine << "," << m_commandRange.startColumn << ")"
                << "to (" << m_commandRange.endLine << "," << m_commandRange.endColumn << ")";
            }

            if( m_viInputModeManager->getCurrentViMode() == NormalMode ) {
              m_view->setCaretStyle( KateRenderer::Block, true );
            }
            m_commandWithMotion = false;
            reset();
            break;
          }
        }
      }
    }
  }

  if (this->waitingForRegisterOrCharToSearch())
  {
    // If we are waiting for a char to search or a new register,
    // don't translate next character; we need the actual character so that e.g.
    // 'ab' is translated to 'fb' if the mappings 'a' -> 'f' and 'b' -> something else
    // exist.
    m_viInputModeManager->keyMapper()->setDoNotMapNextKeypress();
  }

  if (motionExecuted)
  {
    return true;
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

      if( m_viInputModeManager->getCurrentViMode() == NormalMode ) {
        m_view->setCaretStyle( KateRenderer::Block, true );
      }

      KateViCommand *cmd = m_commands.at( m_matchingCommands.at( 0 ) );
      executeCommand( cmd );

      // check if reset() should be called. some commands in visual mode should not end visual mode
      if ( cmd->shouldReset() ) {
        reset();
        m_view->setBlockSelection(false);
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
//  kDebug( 13070 ) << "***RESET***";
  m_keys.clear();
  m_keysVerbatim.clear();
  m_count = 0;
  m_oneTimeCountOverride = -1;
  m_iscounted = false;
  m_countTemp = 0;
  m_register = QChar::Null;
  m_findWaitingForChar = false;
  m_matchingCommands.clear();
  m_matchingMotions.clear();
  m_awaitingMotionOrTextObject.clear();
  m_motionOperatorIndex = 0;

  m_commandWithMotion = false;
  m_linewiseCommand = true;
  m_deleteCommand = false;

  m_commandShouldKeepSelection = false;

  m_currentChangeEndMarker = Cursor::invalid();
}

// reset the command parser
void KateViNormalMode::reset()
{
    resetParser();
    m_commandRange.startLine = -1;
    m_commandRange.startColumn = -1;
}

void KateViNormalMode::beginMonitoringDocumentChanges()
{
  connect(doc(), SIGNAL(textInserted(KTextEditor::Document*,KTextEditor::Range)),
          this, SLOT(textInserted(KTextEditor::Document*,KTextEditor::Range)));
  connect(doc(), SIGNAL(textRemoved(KTextEditor::Document*,KTextEditor::Range)),
          this, SLOT(textRemoved(KTextEditor::Document*,KTextEditor::Range)));
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
  const ViMode originalViMode = m_viInputModeManager->getCurrentViMode();

  cmd->execute();

  // if normal mode was started by using Ctrl-O in insert mode,
  // it's time to go back to insert mode.
  if (m_viInputModeManager->getTemporaryNormalMode()) {
      startInsertMode();
      m_viewInternal->repaint();
  }

  // if the command was a change, and it didn't enter insert mode, store the key presses so that
  // they can be repeated with '.'
  if ( m_viInputModeManager->getCurrentViMode() != InsertMode ) {
    if ( cmd->isChange() && !m_viInputModeManager->isReplayingLastChange() ) {
      m_viInputModeManager->storeLastChangeCommand();
    }

      // when we transition to visual mode, remember the command in the keys history (V, v, ctrl-v, ...)
      // this will later result in buffer filled with something like this "Vjj>" which we can use later with repeat "."
      const bool commandSwitchedToVisualMode = ((originalViMode == NormalMode) && m_viInputModeManager->isAnyVisualMode());
      if (!commandSwitchedToVisualMode)
        m_viInputModeManager->clearCurrentChangeLog();
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

void KateViNormalMode::addCurrentPositionToJumpList() {
    m_viInputModeManager->addJump(m_view->cursorPosition());
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
  m_viInputModeManager->getViInsertMode()->setCount(getCount());
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
  m_viInputModeManager->getViInsertMode()->setCount(getCount());
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
  m_viInputModeManager->getViInsertMode()->setCount(getCount());
  return startInsertMode();
}

bool KateViNormalMode::commandEnterInsertModeBeforeFirstNonBlankInLine()
{
  Cursor cursor( m_view->cursorPosition() );
  int c = getFirstNonBlank();
  cursor.setColumn( c );
  updateCursor( cursor );

  m_stickyColumn = -1;
  m_viInputModeManager->getViInsertMode()->setCount(getCount());
  return startInsertMode();
}

/**
 * enter insert mode at the last insert position
 */

bool KateViNormalMode::commandEnterInsertModeLast()
{
  Cursor c = m_view->getViInputModeManager()->getMarkPosition( '^' );
  if ( c.isValid() ) {
    updateCursor( c );
  }

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

bool KateViNormalMode::commandReselectVisual()
{
  // start last visual mode and set start = `< and cursor = `>
  Cursor c1 = m_view->getViInputModeManager()->getMarkPosition( '<' );
  Cursor c2 = m_view->getViInputModeManager()->getMarkPosition( '>' );

  // we should either get two valid cursors or two invalid cursors
  Q_ASSERT( c1.isValid() == c2.isValid() );

  if ( c1.isValid() && c2.isValid() ) {
    m_viInputModeManager->getViVisualMode()->setStart( c1 );
    bool returnValue = false;

    switch ( m_viInputModeManager->getViVisualMode()->getLastVisualMode() ) {
    case VisualMode:
      returnValue = commandEnterVisualMode();
      break;
    case VisualLineMode:
      returnValue = commandEnterVisualLineMode();
      break;
    case VisualBlockMode:
      returnValue = commandEnterVisualBlockMode();
      break;
    default:
      Q_ASSERT( "invalid visual mode" );
    }
    m_viInputModeManager->getViVisualMode()->goToPos(c2);
    return returnValue;
  } else {
    error("No previous visual selection");
  }

  return false;
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
  if (m_viInputModeManager->isAnyVisualMode()) {
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

  bool ret = deleteRange( r, LineWise );

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

  m_deleteCommand = true;
  return ret;
}

bool KateViNormalMode::commandDelete()
{
  m_deleteCommand = true;
  return deleteRange( m_commandRange, getOperationMode() );
}

bool KateViNormalMode::commandDeleteToEOL()
{
    Cursor c(m_view->cursorPosition());
    OperationMode m = CharWise;

    m_commandRange.endColumn = KateVi::EOL;
    switch (m_viInputModeManager->getCurrentViMode()) {
    case NormalMode:
        m_commandRange.startLine = c.line();
        m_commandRange.startColumn = c.column();
        m_commandRange.endLine = c.line() + getCount() - 1;
        break;
    case VisualMode:
    case VisualLineMode:
        m = LineWise;
        break;
    case VisualBlockMode:
        m_commandRange.normalize();
        m = Block;
        break;
    default:
        /* InsertMode and ReplaceMode will never call this method. */
        Q_ASSERT(false);
    }

    bool r = deleteRange(m_commandRange, m);

    switch (m) {
    case CharWise:
        c.setColumn(doc()->lineLength(c.line()) - 1);
        break;
    case LineWise:
        c.setLine(m_commandRange.startLine);
        c.setColumn(getFirstNonBlank(m_commandRange.startLine));
        break;
    case Block:
        c.setLine(m_commandRange.startLine);
        c.setColumn(m_commandRange.startColumn - 1);
        break;
    }

    // make sure cursor position is valid after deletion
    if (c.line() < 0) {
        c.setLine(0);
    }
    if (c.line() > doc()->lastLine()) {
        c.setLine(doc()->lastLine());
    }
    if (c.column() > doc()->lineLength(c.line()) - 1) {
        c.setColumn(doc()->lineLength(c.line()) - 1);
    }
    if (c.column() < 0) {
        c.setColumn(0);
    }

    updateCursor(c);

    m_deleteCommand = true;
    return r;
}

bool KateViNormalMode::commandMakeLowercase()
{
  Cursor c = m_view->cursorPosition();

  OperationMode m = getOperationMode();
  QString text = getRange( m_commandRange, m );
  if (m == LineWise)
    text = text.left(text.size() - 1); // don't need '\n' at the end;
  QString lowerCase = text.toLower();

  m_commandRange.normalize();
  Cursor start( m_commandRange.startLine, m_commandRange.startColumn );
  Cursor end( m_commandRange.endLine, m_commandRange.endColumn );
  Range range( start, end );

  doc()->replaceText( range, lowerCase, m == Block );

  if (m_viInputModeManager->getCurrentViMode() == NormalMode)
    updateCursor( start );
  else
    updateCursor(c);

  return true;
}

bool KateViNormalMode::commandMakeLowercaseLine()
{
  Cursor c( m_view->cursorPosition() );

  if (doc()->lineLength(c.line()) == 0)
  {
    // Nothing to do.
    return true;
  }

  m_commandRange.startLine = c.line();
  m_commandRange.endLine = c.line() + getCount() - 1;
  m_commandRange.startColumn = 0;
  m_commandRange.endColumn = doc()->lineLength( c.line() )-1;

  return commandMakeLowercase();
}

bool KateViNormalMode::commandMakeUppercase()
{
  kDebug(13070) << "Heere!";
  if (!m_commandRange.valid)
  {
    kDebug(13070) << "Here2";
    return false;
  }
  Cursor c = m_view->cursorPosition();
  OperationMode m = getOperationMode();
  QString text = getRange( m_commandRange, m );
  if (m == LineWise)
    text = text.left(text.size() - 1); // don't need '\n' at the end;
  QString upperCase = text.toUpper();

  m_commandRange.normalize();
  Cursor start( m_commandRange.startLine, m_commandRange.startColumn );
  Cursor end( m_commandRange.endLine, m_commandRange.endColumn );
  Range range( start, end );

  doc()->replaceText( range, upperCase, m == Block );
  if (m_viInputModeManager->getCurrentViMode() == NormalMode)
    updateCursor( start );
  else
    updateCursor(c);

  return true;
}

bool KateViNormalMode::commandMakeUppercaseLine()
{
  Cursor c( m_view->cursorPosition() );

  if (doc()->lineLength(c.line()) == 0)
  {
    // Nothing to do.
    return true;
  }

  m_commandRange.startLine = c.line();
  m_commandRange.endLine = c.line() + getCount() - 1;
  m_commandRange.startColumn = 0;
  m_commandRange.endColumn = doc()->lineLength( c.line() )-1;

  return commandMakeUppercase();
}

bool KateViNormalMode::commandChangeCase()
{
  switchView();
  QString text;
  Range range;
  Cursor c( m_view->cursorPosition() );

  // in visual mode, the range is from start position to end position...
  if ( m_viInputModeManager->getCurrentViMode() == VisualMode
    || m_viInputModeManager->getCurrentViMode() == VisualBlockMode ) {
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

  bool block = m_viInputModeManager->getCurrentViMode() == VisualBlockMode;

  // get the text the command should operate on
  text = doc()->text ( range, block );

  // for every character, switch its case
  for ( int i = 0; i < text.length(); i++ ) {
    if ( text.at(i).isUpper() ) {
      text[i] = text.at(i).toLower();
    } else if ( text.at(i).isLower() ) {
      text[i] = text.at(i).toUpper();
    }
  }

  // replace the old text with the modified text
  doc()->replaceText( range, text, block );

  // in normal mode, move the cursor to the right, in visual mode move the
  // cursor to the start of the selection
  if ( m_viInputModeManager->getCurrentViMode() == NormalMode ) {
    updateCursor( range.end() );
  } else {
    updateCursor( range.start() );
  }

  return true;
}

bool KateViNormalMode::commandChangeCaseRange()
{
  OperationMode m = getOperationMode();
  QString changedCase = getRange( m_commandRange, m );
  if (m == LineWise)
    changedCase = changedCase.left(changedCase.size() - 1); // don't need '\n' at the end;
  Range range = Range(m_commandRange.startLine, m_commandRange.startColumn, m_commandRange.endLine, m_commandRange.endColumn);
  // get the text the command should operate on
  // for every character, switch its case
  for ( int i = 0; i < changedCase.length(); i++ ) {
    if ( changedCase.at(i).isUpper() ) {
      changedCase[i] = changedCase.at(i).toLower();
    } else if ( changedCase.at(i).isLower() ) {
      changedCase[i] = changedCase.at(i).toUpper();
    }
  }
  doc()->replaceText( range, changedCase, m == Block );
  return true;
}

bool KateViNormalMode::commandChangeCaseLine()
{
  Cursor c( m_view->cursorPosition() );

  if (doc()->lineLength(c.line()) == 0)
  {
    // Nothing to do.
    return true;
  }

  m_commandRange.startLine = c.line();
  m_commandRange.endLine = c.line() + getCount() - 1;
  m_commandRange.startColumn = 0;
  m_commandRange.endColumn = doc()->lineLength( c.line() )-1; // -1 is for excluding '\0'

  if ( !commandChangeCaseRange() )
    return false;

  Cursor start( m_commandRange.startLine, m_commandRange.startColumn );
  if (getCount() > 1)
    updateCursor(c);
  else
    updateCursor(start);
  return true;

}

bool KateViNormalMode::commandOpenNewLineUnder()
{
  doc()->setUndoMergeAllEdits(true);

  Cursor c( m_view->cursorPosition() );

  c.setColumn( doc()->lineLength( c.line() ) );
  updateCursor( c );

  doc()->newLine( m_view );

  m_stickyColumn = -1;
  startInsertMode();
  m_viInputModeManager->getViInsertMode()->setCount(getCount());
  m_viInputModeManager->getViInsertMode()->setCountedRepeatsBeginOnNewLine(true);
  m_viewInternal->repaint ();

  return true;
}

bool KateViNormalMode::commandOpenNewLineOver()
{
  doc()->setUndoMergeAllEdits(true);

  Cursor c( m_view->cursorPosition() );

  if ( c.line() == 0 ) {
    doc()->insertLine(0, QString());
    c.setColumn( 0 );
    c.setLine( 0 );
    updateCursor( c );
  } else {
    c.setLine( c.line()-1 );
    c.setColumn( getLine( c.line() ).length() );
    updateCursor( c );
    doc()->newLine( m_view );
  }

  m_stickyColumn = -1;
  startInsertMode();
  m_viInputModeManager->getViInsertMode()->setCount(getCount());
  m_viInputModeManager->getViInsertMode()->setCountedRepeatsBeginOnNewLine(true);
  m_viewInternal->repaint ();

  return true;
}

bool KateViNormalMode::commandJoinLines()
{
  Cursor c( m_view->cursorPosition() );

  unsigned int from = c.line();
  unsigned int to = c.line() + ((getCount() == 1) ? 1 : getCount() - 1);

  // if we were given a range of lines, this information overrides the previous
  if ( m_commandRange.startLine != -1 && m_commandRange.endLine != -1 ) {
    m_commandRange.normalize();
    c.setLine ( m_commandRange.startLine );
    from = m_commandRange.startLine;
    to = m_commandRange.endLine;
  }

  if (to >= (unsigned int)doc()->lines())
  {
    return false;
  }

  bool nonEmptyLineFound = false;
  for (unsigned int lineNum = from; lineNum <= to; lineNum++)
  {
    if (!doc()->line(lineNum).isEmpty())
    {
      nonEmptyLineFound = true;
    }
  }

  const int firstNonWhitespaceOnLastLine = doc()->kateTextLine(to)->firstChar();
  QString leftTrimmedLastLine;
  if (firstNonWhitespaceOnLastLine != -1)
  {
    leftTrimmedLastLine = doc()->line(to).mid(firstNonWhitespaceOnLastLine);
  }

  joinLines( from, to );

  if (nonEmptyLineFound && leftTrimmedLastLine.isEmpty())
  {
    // joinLines won't have added a trailing " ", whereas Vim does - follow suit.
    doc()->insertText(Cursor(from, doc()->lineLength(from)), " ");
  }

  // Position cursor just before first non-whitesspace character of what was the last line joined.
  c.setColumn(doc()->lineLength(from) - leftTrimmedLastLine.length() - 1);
  if (c.column() >= 0) {
    updateCursor( c );
  }

  m_deleteCommand = true;
  return true;
}

bool KateViNormalMode::commandChange()
{
  Cursor c( m_view->cursorPosition() );

  OperationMode m = getOperationMode();

  doc()->setUndoMergeAllEdits(true);

  commandDelete();

  if ( m == LineWise ) {
    // if we deleted several lines, insert an empty line and put the cursor there.
    doc()->insertLine( m_commandRange.startLine, QString() );
    c.setLine( m_commandRange.startLine );
    c.setColumn(0);
  } else if (m == Block) {
    // block substitute can be simulated by first deleting the text
    // (done above) and then starting block prepend.
    return commandPrependToBlock();
  } else {
    if (m_commandRange.startLine < m_commandRange.endLine) {
      c.setLine(m_commandRange.startLine);
    }
    c.setColumn(m_commandRange.startColumn);
  }

  updateCursor(c);
  setCount(0); // The count was for the motion, not the insertion.
  commandEnterInsertMode();

  // correct indentation level
  if ( m == LineWise ) {
    m_view->align();
  }

  m_deleteCommand = true;
  return true;
}

bool KateViNormalMode::commandChangeToEOL()
{
  commandDeleteToEOL();

  if ( getOperationMode() == Block ) {
    return commandPrependToBlock();
  }

  m_deleteCommand = true;
  return commandEnterInsertModeAppend();
}

bool KateViNormalMode::commandChangeLine()
{
  m_deleteCommand = true;
  Cursor c( m_view->cursorPosition() );
  c.setColumn( 0 );
  updateCursor( c );

  doc()->setUndoMergeAllEdits(true);

  // if count >= 2 start by deleting the whole lines
  if ( getCount() >= 2 ) {
    KateViRange r( c.line(), 0, c.line()+getCount()-2, 0, ViMotion::InclusiveMotion );
    deleteRange( r );
  }

  // ... then delete the _contents_ of the last line, but keep the line
  KateViRange r( c.line(), c.column(), c.line(), doc()->lineLength( c.line() )-1,
      ViMotion::InclusiveMotion );
  deleteRange( r, CharWise, true );

  // ... then enter insert mode
  if ( getOperationMode() == Block ) {
    return commandPrependToBlock();
  }
  commandEnterInsertModeAppend();

  // correct indentation level
  m_view->align();

  return true;
}

bool KateViNormalMode::commandSubstituteChar()
{
  if ( commandDeleteChar() ) {
    // The count is only used for deletion of chars; the inserted text is not repeated
    setCount(0);
    return commandEnterInsertMode();
  }

  m_deleteCommand = true;
  return false;
}

bool KateViNormalMode::commandSubstituteLine()
{
  m_deleteCommand = true;
  return commandChangeLine();
}

bool KateViNormalMode::commandYank()
{
  Cursor c( m_view->cursorPosition() );

  bool r = false;
  QString yankedText;

  OperationMode m = getOperationMode();
  yankedText = getRange( m_commandRange, m );

  highlightYank(m_commandRange, m);

  QChar  chosen_register =  getChosenRegister( '0' );
  fillRegister(chosen_register, yankedText, m );
  yankToClipBoard(chosen_register, yankedText);

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

  KateViRange yankRange(linenum, 0, linenum + getCount() - 1, getLine(linenum + getCount() - 1).length(), ViMotion::InclusiveMotion);
  highlightYank(yankRange);

  QChar  chosen_register =  getChosenRegister( '0' );
  fillRegister(chosen_register, lines, LineWise );
  yankToClipBoard(chosen_register, lines);

  return true;
}

bool KateViNormalMode::commandYankToEOL()
{
    OperationMode m = CharWise;
    Cursor c(m_view->cursorPosition());

    ViMotion::MotionType motion = m_commandRange.motionType;
    m_commandRange.endLine = c.line() + getCount() - 1;
    m_commandRange.endColumn = doc()->lineLength(m_commandRange.endLine) - 1;
    m_commandRange.motionType = ViMotion::InclusiveMotion;

    switch (m_viInputModeManager->getCurrentViMode()) {
    case NormalMode:
        m_commandRange.startLine = c.line();
        m_commandRange.startColumn = c.column();
        break;
    case VisualMode:
    case VisualLineMode:
        m = LineWise;
        {
            KateViVisualMode *visual = static_cast<KateViVisualMode *>(this);
            visual->setStart(Cursor(visual->getStart().line(), 0));
        }
        break;
    case VisualBlockMode:
        m = Block;
        break;
    default:
        /* InsertMode and ReplaceMode will never call this method. */
        Q_ASSERT(false);
    }

    const QString &yankedText = getRange(m_commandRange, m);
    m_commandRange.motionType = motion;
    highlightYank(m_commandRange);

    QChar chosen_register =  getChosenRegister(QLatin1Char('0'));
    fillRegister(chosen_register,  yankedText, m);
    yankToClipBoard(chosen_register, yankedText);

    return true;
}

// Insert the text in the given register after the cursor position.
// This is the non-g version of paste, so the cursor will usually
// end up on the last character of the pasted text, unless the text
// was multi-line or linewise in which case it will end up
// on the *first* character of the pasted text(!)
// If linewise, will paste after the current line.
bool KateViNormalMode::commandPaste()
{
  return paste(AfterCurrentPosition, false, false);
}

// As with commandPaste, except that the text is pasted *at* the cursor position
bool KateViNormalMode::commandPasteBefore()
{
  return paste(AtCurrentPosition, false, false);
}

// As with commandPaste, except that the cursor will generally be placed *after* the
// last pasted character (assuming the last pasted character is not at  the end of the line).
// If linewise, cursor will be at the beginning of the line *after* the last line of pasted text,
// unless that line is the last line of the document; then it will be placed at the beginning of the
// last line pasted.
bool KateViNormalMode::commandgPaste()
{
  return paste(AfterCurrentPosition, true, false);
}

// As with commandgPaste, except that it pastes *at* the current cursor position or, if linewise,
// at the current line.
bool KateViNormalMode::commandgPasteBefore()
{
  return paste(AtCurrentPosition, true, false);
}

bool KateViNormalMode::commandIndentedPaste()
{
  return paste(AfterCurrentPosition, false, true);
}

bool KateViNormalMode::commandIndentedPasteBefore()
{
  return paste(AtCurrentPosition, false, true);
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

    // should delete entire lines if in visual line mode and selection in visual block mode
    OperationMode m = CharWise;
    if ( m_viInputModeManager->getCurrentViMode() == VisualLineMode ) {
      m = LineWise;
    } else if ( m_viInputModeManager->getCurrentViMode() == VisualBlockMode ) {
      m = Block;
    }

    m_deleteCommand = true;
    return deleteRange( r, m );
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

    // should delete entire lines if in visual line mode and selection in visual block mode
    OperationMode m = CharWise;
    if ( m_viInputModeManager->getCurrentViMode() == VisualLineMode ) {
      m = LineWise;
    } else if ( m_viInputModeManager->getCurrentViMode() == VisualBlockMode ) {
      m = Block;
    }

    m_deleteCommand = true;
    return deleteRange( r, m );
}

bool KateViNormalMode::commandReplaceCharacter()
{
    QString key = KateViKeyParser::self()->decodeKeySequence(m_keys.right(1));

    // Filter out some special keys.
    const int keyCode = KateViKeyParser::self()->encoded2qt(m_keys.right(1));
    switch (keyCode) {
        case Qt::Key_Left: case Qt::Key_Right: case Qt::Key_Up:
        case Qt::Key_Down: case Qt::Key_Home: case Qt::Key_End:
        case Qt::Key_PageUp: case Qt::Key_PageDown: case Qt::Key_Delete:
        case Qt::Key_Insert: case Qt::Key_Backspace: case Qt::Key_CapsLock:
            return true;
        case Qt::Key_Return: case Qt::Key_Enter:
            key = QLatin1String("\n");
    }

  bool r;
  if ( m_viInputModeManager->isAnyVisualMode()) {

    OperationMode m = getOperationMode();
    QString text = getRange( m_commandRange, m );

    if (m == LineWise)
      text = text.left(text.size() - 1); // don't need '\n' at the end;

    text.replace( QRegExp( "[^\n]" ), key );

    m_commandRange.normalize();
    Cursor start( m_commandRange.startLine, m_commandRange.startColumn );
    Cursor end( m_commandRange.endLine, m_commandRange.endColumn );
    Range range( start, end );

    r = doc()->replaceText( range, text, m == Block );

  } else {
    Cursor c1( m_view->cursorPosition() );
    Cursor c2( m_view->cursorPosition() );

    c2.setColumn( c2.column() + getCount() );

    if (c2.column() > doc()->lineLength(m_view->cursorPosition().line()))
    {
      return false;
    }

    r = doc()->replaceText( Range( c1, c2 ), key.repeated(getCount()) );
    updateCursor( c1 );

  }
  return r;
}

bool KateViNormalMode::commandSwitchToCmdLine()
{
    Cursor c( m_view->cursorPosition() );


    QString initialText;
    if ( m_viInputModeManager->isAnyVisualMode()) {
      // if in visual mode, make command range == visual selection
      m_viInputModeManager->getViVisualMode()->saveRangeMarks();
      initialText = "'<,'>";
    }
    else if ( getCount() != 1 ) {
      // if a count is given, the range [current line] to [current line] +
      // count should be prepended to the command line
      initialText = ".,.+" +QString::number( getCount()-1 );
    }
    if (!KateViewConfig::global()->viInputModeEmulateCommandBar())
    {
      m_view->switchToCmdLine();
      m_view->cmdLineBar()->setText( initialText, false );
    }
    else
    {
      m_view->showViModeEmulatedCommandBar();
      m_view->viModeEmulatedCommandBar()->init( KateViEmulatedCommandBar::Command, initialText );
    }

    m_commandShouldKeepSelection = true;

    return true;
}

bool KateViNormalMode::commandSearchBackward()
{
    if (!KateViewConfig::global()->viInputModeEmulateCommandBar())
    {
      m_viInputModeManager->setLastSearchBackwards(true);
      m_view->find();
    }
    else
    {
      m_view->showViModeEmulatedCommandBar();
      m_view->viModeEmulatedCommandBar()->init(KateViEmulatedCommandBar::SearchBackward);
    }
    return true;
}

bool KateViNormalMode::commandSearchForward()
{
    if (!KateViewConfig::global()->viInputModeEmulateCommandBar())
    {
      m_view->find();
    }
    else
    {
      m_view->showViModeEmulatedCommandBar();
      m_view->viModeEmulatedCommandBar()->init(KateViEmulatedCommandBar::SearchForward);
    }
    m_viInputModeManager->setLastSearchBackwards( false );
    return true;
}

bool KateViNormalMode::commandUndo()
{
  // See BUG #328277
  m_viInputModeManager->clearCurrentChangeLog();

  if (doc()->undoCount() > 0) {
    const bool mapped = m_viInputModeManager->keyMapper()->isExecutingMapping();

    if (mapped)
      doc()->editEnd();
    doc()->undo();
    if (mapped)
      doc()->editBegin();
    return true;
  }
  return false;
}

bool KateViNormalMode::commandRedo()
{
  if (doc()->redoCount() > 0) {
    const bool mapped = m_viInputModeManager->keyMapper()->isExecutingMapping();

    if (mapped)
      doc()->editEnd();
    doc()->redo();
    if (mapped)
      doc()->editBegin();
    return true;
  }
  return false;
}

bool KateViNormalMode::commandSetMark()
{
  Cursor c( m_view->cursorPosition() );

  m_view->getViInputModeManager()->addMark( doc(), m_keys.at( m_keys.size()-1 ), c );
  kDebug( 13070 ) << "set mark at (" << c.line() << "," << c.column() << ")";

  return true;
}

bool KateViNormalMode::commandIndentLine()
{
    Cursor c( m_view->cursorPosition() );

    doc()->indent( KTextEditor::Range( c.line(), 0, c.line() + getCount(), 0), 1 );

    return true;
}

bool KateViNormalMode::commandUnindentLine()
{
    Cursor c( m_view->cursorPosition() );

    doc()->indent( KTextEditor::Range( c.line(), 0, c.line() + getCount(), 0), -1 );

    return true;
}

bool KateViNormalMode::commandIndentLines()
{
  Cursor c( m_view->cursorPosition() );

  m_commandRange.normalize();

  int line1 = m_commandRange.startLine;
  int line2 = m_commandRange.endLine;
  int col = getLine( line2 ).length();

  doc()->indent( KTextEditor::Range( line1, 0, line2, col ), getCount() );

  return true;
}

bool KateViNormalMode::commandUnindentLines()
{
  Cursor c( m_view->cursorPosition() );

  m_commandRange.normalize();

  int line1 = m_commandRange.startLine;
  int line2 = m_commandRange.endLine;

  doc()->indent( KTextEditor::Range( line1, 0, line2, doc()->lineLength( line2 ) ), -getCount() );

  return true;
}

bool KateViNormalMode::commandScrollPageDown()
{
  if ( getCount() < m_scroll_count_limit ) {

    for(uint i = 0; i < getCount(); i++)
      m_view->pageDown();
  }
  return true;
}

bool KateViNormalMode::commandScrollPageUp()
{
  if ( getCount() < m_scroll_count_limit ) {
    for(uint i=0; i < getCount(); i++)
      m_view->pageUp();
  }
  return true;

}

bool KateViNormalMode::commandScrollHalfPageUp()
{
  if ( getCount() < m_scroll_count_limit ) {

    for(uint i=0; i < getCount(); i++)
      m_viewInternal->pageUp(false, true);
  }
  return true;
}

bool KateViNormalMode::commandScrollHalfPageDown()
{
  if ( getCount() < m_scroll_count_limit ) {
    for(uint i=0; i < getCount(); i++)
      m_viewInternal->pageDown(false, true);
  }
  return true;
}

bool KateViNormalMode::commandCenterView(bool onFirst)
{
    Cursor c(m_view->cursorPosition());
    const int virtualCenterLine = m_viewInternal->startLine() + linesDisplayed() / 2;
    const int virtualCursorLine = m_view->textFolding().lineToVisibleLine(c.line());

    scrollViewLines(virtualCursorLine - virtualCenterLine);
    if (onFirst) {
        c.setColumn(getFirstNonBlank());
        updateCursor(c);
    }
    return true;
}

bool KateViNormalMode::commandCenterViewOnNonBlank()
{
    return commandCenterView(true);
}

bool KateViNormalMode::commandCenterViewOnCursor()
{
    return commandCenterView(false);
}

bool KateViNormalMode::commandTopView(bool onFirst)
{
    Cursor c(m_view->cursorPosition());
    const int virtualCenterLine = m_viewInternal->startLine();
    const int virtualCursorLine = m_view->textFolding().lineToVisibleLine(c.line());

    scrollViewLines(virtualCursorLine - virtualCenterLine);
    if (onFirst) {
        c.setColumn(getFirstNonBlank());
        updateCursor(c);
    }
    return true;
}

bool KateViNormalMode::commandTopViewOnNonBlank()
{
    return commandTopView(true);
}

bool KateViNormalMode::commandTopViewOnCursor()
{
    return commandTopView(false);
}

bool KateViNormalMode::commandBottomView(bool onFirst)
{
    Cursor c(m_view->cursorPosition());
    const int virtualCenterLine = m_viewInternal->endLine();
    const int virtualCursorLine = m_view->textFolding().lineToVisibleLine(c.line());

    scrollViewLines(virtualCursorLine - virtualCenterLine);
    if (onFirst) {
        c.setColumn(getFirstNonBlank());
        updateCursor(c);
    }
    return true;
}

bool KateViNormalMode::commandBottomViewOnNonBlank()
{
    return commandBottomView(true);
}

bool KateViNormalMode::commandBottomViewOnCursor()
{
    return commandBottomView(false);
}

bool KateViNormalMode::commandAbort()
{
  m_pendingResetIsDueToExit = true;
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
    if ( code > 0x80 && code < 0x1000 ) { hex.prepend( ( code < 0x100 ? "00" : "0" ) ); }
    message( i18n("'%1' %2,  Hex %3,  Octal %4", ch, dec, hex, oct ) );
  }

  return true;
}

bool KateViNormalMode::commandRepeatLastChange()
{
  const int repeatCount = getCount();
  resetParser();
  if (repeatCount > 1)
  {
    m_oneTimeCountOverride = repeatCount;
  }
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

bool KateViNormalMode::commandPrependToBlock()
{
  Cursor c( m_view->cursorPosition() );

  // move cursor to top left corner of selection
  m_commandRange.normalize();
  c.setColumn( m_commandRange.startColumn );
  c.setLine( m_commandRange.startLine );
  updateCursor( c );

  m_stickyColumn = -1;
  m_viInputModeManager->getViInsertMode()->setBlockPrependMode( m_commandRange );
  return startInsertMode();
}

bool KateViNormalMode::commandAppendToBlock()
{
  Cursor c( m_view->cursorPosition() );

  m_commandRange.normalize();
  if ( m_stickyColumn == (unsigned int)KateVi::EOL ) { // append to EOL
    // move cursor to end of first line
    c.setLine( m_commandRange.startLine );
    c.setColumn( doc()->lineLength( c.line() ) );
    updateCursor( c );
    m_viInputModeManager->getViInsertMode()->setBlockAppendMode( m_commandRange, AppendEOL );
  } else {
    m_viInputModeManager->getViInsertMode()->setBlockAppendMode( m_commandRange, Append );
    // move cursor to top right corner of selection
    c.setColumn( m_commandRange.endColumn+1 );
    c.setLine( m_commandRange.startLine );
    updateCursor( c );
  }

  m_stickyColumn = -1;

  return startInsertMode();
}

bool KateViNormalMode::commandGoToNextJump() {
    Cursor c = getNextJump(m_view->cursorPosition());
    updateCursor(c);

    return true;
}

bool KateViNormalMode::commandGoToPrevJump() {
    Cursor c = getPrevJump(m_view->cursorPosition());
    updateCursor(c);

    return true;
}

bool KateViNormalMode::commandSwitchToLeftView() {
    switchView(Left);
    return true;
}

bool KateViNormalMode::commandSwitchToDownView() {
    switchView(Down);
    return true;
}

bool KateViNormalMode::commandSwitchToUpView() {
    switchView(Up);
    return true;
}

bool KateViNormalMode::commandSwitchToRightView() {
    switchView(Right);
    return true;
}

bool KateViNormalMode::commandSwitchToNextView() {
    switchView(Next);
    return true;
}

bool KateViNormalMode::commandSplitHoriz() {
  m_view->cmdLineBar()->execute("split");
  return true;
}

bool KateViNormalMode::commandSplitVert() {
  m_view->cmdLineBar()->execute("vsplit");
  return true;
}

bool KateViNormalMode::commandSwitchToNextTab() {
  QString command = "bn";

  if ( m_iscounted )
    command = command + ' ' + QString::number(getCount());

  m_view->cmdLineBar()->execute(command);
  return true;
}

bool KateViNormalMode::commandSwitchToPrevTab() {
  QString command = "bp";

  if ( m_iscounted )
    command = command + ' ' + QString::number(getCount());

  m_view->cmdLineBar()->execute(command);
  return true;
}

bool KateViNormalMode::commandFormatLine()
{
  Cursor c( m_view->cursorPosition() );

  reformatLines( c.line(), c.line()+getCount()-1 );

  return true;
}

bool KateViNormalMode::commandFormatLines()
{
  reformatLines( m_commandRange.startLine, m_commandRange.endLine );
  return true;
}

bool KateViNormalMode::commandCollapseToplevelNodes()
{
#if 0
  //FIXME FOLDING
  doc()->foldingTree()->collapseToplevelNodes();
#endif
  return true;
}

bool KateViNormalMode::commandStartRecordingMacro()
{
  const QChar reg = m_keys[m_keys.size() - 1];
  m_viInputModeManager->startRecordingMacro(reg);
  return true;
}

bool KateViNormalMode::commandReplayMacro()
{
  // "@<registername>" will have been added to the log; it needs to be cleared
  // *before* we replay the macro keypresses, else it can cause an infinite loop
  // if the macro contains a "."
  m_viInputModeManager->clearCurrentChangeLog();
  const QChar reg = m_keys[m_keys.size() - 1];
  const unsigned int count = getCount();
  resetParser();
  doc()->editBegin();
  for ( unsigned int i = 0; i < count; i++ )
  {
    m_viInputModeManager->replayMacro(reg);
  }
  doc()->editEnd();
  return true;
}

bool KateViNormalMode::commandCloseNocheck()
{
  m_view->cmdLineBar()->execute("q!");
  return true;
}

bool KateViNormalMode::commandCloseWrite()
{
  m_view->cmdLineBar()->execute("wq");
  return true;
}

bool KateViNormalMode::commandCollapseLocal()
{
#if 0
  //FIXME FOLDING
  Cursor c( m_view->cursorPosition() );
  doc()->foldingTree()->collapseOne( c.line(), c.column() );
#endif
  return true;
}

bool KateViNormalMode::commandExpandAll() {
#if 0
  //FIXME FOLDING
  doc()->foldingTree()->expandAll();
#endif
  return true;
}

bool KateViNormalMode::commandExpandLocal()
{
#if 0
  //FIXME FOLDING
  Cursor c( m_view->cursorPosition() );
  doc()->foldingTree()->expandOne( c.line() + 1, c.column() );
#endif
  return true;
}

bool KateViNormalMode::commandToggleRegionVisibility()
{
#if 0
  //FIXME FOLDING
  Cursor c( m_view->cursorPosition() );
  doc()->foldingTree()->toggleRegionVisibility( c.line() );
#endif
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

  if ( r.endColumn < 0 ) {
    r.endColumn = 0;
  }

  return r;
}

KateViRange KateViNormalMode::motionRight()
{
  Cursor cursor ( m_view->cursorPosition() );
  m_stickyColumn = -1;
  KateViRange r( cursor.line(), cursor.column(), ViMotion::ExclusiveMotion );
  r.endColumn += getCount();

  // make sure end position isn't > line length
  if ( r.endColumn > doc()->lineLength( r.endLine ) ) {
    r.endColumn = doc()->lineLength( r.endLine );
  }

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
  r.endColumn = getFirstNonBlank(r.endLine);
  return r;
}

KateViRange KateViNormalMode::motionUpToFirstNonBlank()
{
  Cursor c( m_view->cursorPosition() );
  KateViRange r = goLineUp();
  r.endColumn = getFirstNonBlank(r.endLine);
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
      if (!c.isValid()) {
        c = doc()->documentEnd();
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

    if (!c.isValid())
    {
      c = Cursor(0, 0);
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

    if (!c.isValid())
    {
      c = Cursor(0, 0);
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

    if (!c.isValid())
    {
      c = doc()->documentEnd();
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

      if (c.isValid())
      {
        r.endColumn = c.column();
        r.endLine = c.line();
      }
      else
      {
        r.endColumn = 0;
        r.endLine = 0;
        break;
      }

    }

    return r;
}

KateViRange KateViNormalMode::motionToEndOfPrevWORD()
{
    Cursor c( m_view->cursorPosition() );
    KateViRange r( c.line(), c.column(), ViMotion::InclusiveMotion );

    m_stickyColumn = -1;

    for ( unsigned int i = 0; i < getCount(); i++ ) {
      c = findPrevWORDEnd( c.line(), c.column() );

      if (c.isValid())
      {
        r.endColumn = c.column();
        r.endLine = c.line();
      }
      else
      {
        r.endColumn = 0;
        r.endLine = 0;
        break;
      }
    }

    return r;
}

KateViRange KateViNormalMode::motionToEOL()
{
  Cursor c( m_view->cursorPosition() );

  // set sticky column to a ridiculously high value so that the cursor will stick to EOL,
  // but only if it's a regular motion
  if ( m_keys.size() == 1 ) {
    m_stickyColumn = KateVi::EOL;
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
  int c = getFirstNonBlank();

  Cursor cursor ( m_view->cursorPosition() );
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

  if (matchColumn != -1)
  {
    r.endColumn = matchColumn;
    r.endLine = cursor.line();
  }
  else
  {
    return KateViRange::invalid();
  }

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

  if (matchColumn != -1)
  {
    r.endColumn = matchColumn;
    r.endLine = cursor.line();
  }
  else
  {
    return KateViRange::invalid();
  }

  return r;
}

KateViRange KateViNormalMode::motionToChar()
{
  m_lastTFcommand = m_keys;
  Cursor cursor ( m_view->cursorPosition() );
  QString line = getLine();

  m_stickyColumn = -1;
  KateViRange r;
  r.endColumn = -1;
  r.endLine = -1;

  int matchColumn = cursor.column()+ (m_isRepeatedTFcommand ? 2 : 1);

  for ( unsigned int i = 0; i < getCount(); i++ ) {
    const int lastColumn = matchColumn;
    matchColumn = line.indexOf( m_keys.right( 1 ), matchColumn + ((i > 0) ? 1 : 0));
    if ( matchColumn == -1 )
    {
      if (m_isRepeatedTFcommand)
      {
        matchColumn = lastColumn;
      }
      else
      {
        return KateViRange::invalid();
      }
      break;
    }
  }


  r.endColumn = matchColumn-1;
  r.endLine = cursor.line();

  m_isRepeatedTFcommand = false;
  return r;
}

KateViRange KateViNormalMode::motionToCharBackward()
{
  m_lastTFcommand = m_keys;
  Cursor cursor ( m_view->cursorPosition() );
  QString line = getLine();

  const int originalColumn = cursor.column();
  m_stickyColumn = -1;

  int matchColumn = originalColumn - 1;

  unsigned int hits = 0;
  int i = cursor.column()- (m_isRepeatedTFcommand ? 2 : 1);

  KateViRange r;

  while ( hits != getCount() && i >= 0 ) {
    if ( line.at( i ) == m_keys.at( m_keys.size()-1 ) )
      hits++;

    if ( hits == getCount() )
      matchColumn = i;

    i--;
  }

  if (hits == getCount())
  {
    r.endColumn = matchColumn+1;
    r.endLine = cursor.line();
  }
  else
  {
    r.valid = false;
  }

  m_isRepeatedTFcommand = false;

  return r;
}

KateViRange KateViNormalMode::motionRepeatlastTF()
{
  if ( !m_lastTFcommand.isEmpty() ) {
    m_isRepeatedTFcommand = true;
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
  return KateViRange::invalid();
}

KateViRange KateViNormalMode::motionRepeatlastTFBackward()
{
  if ( !m_lastTFcommand.isEmpty() ) {
    m_isRepeatedTFcommand = true;
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
  return KateViRange::invalid();
}

// FIXME: should honour the provided count
KateViRange KateViNormalMode::motionFindPrev()
{
  QString pattern = m_viInputModeManager->getLastSearchPattern();
  bool backwards = m_viInputModeManager->lastSearchBackwards();
  const bool caseSensitive = m_viInputModeManager->lastSearchCaseSensitive();
  const bool placeCursorAtEndOfMatch = m_viInputModeManager->lastSearchPlacesCursorAtEndOfMatch();

  KateViRange match = findPatternForMotion( pattern, !backwards, caseSensitive, m_view->cursorPosition(), getCount() );

  if (!match.valid) {
    return match;
  }
  if (!placeCursorAtEndOfMatch)
  {
    return KateViRange(match.startLine, match.startColumn, ViMotion::ExclusiveMotion);
  }
  else
  {
    return KateViRange(match.endLine, match.endColumn - 1, ViMotion::ExclusiveMotion);
  }
}

KateViRange KateViNormalMode::motionFindNext()
{
  QString pattern = m_viInputModeManager->getLastSearchPattern();
  bool backwards = m_viInputModeManager->lastSearchBackwards();
  const bool caseSensitive = m_viInputModeManager->lastSearchCaseSensitive();
  const bool placeCursorAtEndOfMatch = m_viInputModeManager->lastSearchPlacesCursorAtEndOfMatch();

  KateViRange match = findPatternForMotion( pattern, backwards, caseSensitive, m_view->cursorPosition(), getCount() );

  if (!match.valid) {
    return match;
  }
  if (!placeCursorAtEndOfMatch)
  {
    return KateViRange(match.startLine, match.startColumn, ViMotion::ExclusiveMotion);
  }
  else
  {
    return KateViRange(match.endLine, match.endColumn - 1, ViMotion::ExclusiveMotion);
  }
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

  Cursor c = m_view->getViInputModeManager()->getMarkPosition( reg );
  if ( c.isValid() ) {
    r.endLine = c.line();
    r.endColumn = c.column();
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
  r.endColumn = getFirstNonBlank(r.endLine);

  m_stickyColumn = -1;

  r.jump = true;

  return r;
}

KateViRange KateViNormalMode::motionToMatchingItem()
{
  KateViRange r;
  int lines = doc()->lines();

  // If counted, then it's not a motion to matching item anymore,
  // but a motion to the N'th percentage of the document
  if( isCounted() ) {
    int count = getCount();
    if ( count > 100 ) {
      return r;
    }
    r.endLine = qRound(lines * count / 100.0) - 1;
    r.endColumn = 0;
    return r;
  }

  Cursor c( m_view->cursorPosition() );

  QString l = getLine();
  int n1 = l.indexOf( m_matchItemRegex, c.column() );

  m_stickyColumn = -1;

  if ( n1 < 0 ) {
    return KateViRange::invalid();
  }

  QRegExp brackets( "[(){}\\[\\]]" );

  // use Kate's built-in matching bracket finder for brackets
  if ( brackets.indexIn ( l, n1 ) == n1 ) {
    // findMatchingBracket requires us to move the cursor to the
    // first bracket, but we don't want the cursor to really move
    // in case this is e.g. a yank, so restore it to its original
    // position afterwards.
    c.setColumn( n1 + 1 );
    const Cursor oldCursorPos = m_view->cursorPosition();
    updateCursor(c);

    // find the matching one
    c = m_viewInternal->findMatchingBracket();
    if ( c > m_view->cursorPosition() ) {
      c.setColumn( c.column() - 1 );
    }
    m_view->setCursorPosition(oldCursorPos);
  } else {
    // text item we want to find a matching item for
    int n2 = l.indexOf( QRegExp( "\\b|\\s|$" ), n1 );
    QString item = l.mid( n1, n2 - n1 );
    QString matchingItem = m_matchingItems[ item ];

    int toFind = 1;
    int line = c.line();
    int column = n2 - item.length();
    bool reverse = false;

    if ( matchingItem.left( 1 ) == "-" ) {
      matchingItem.remove( 0, 1 ); // remove the '-'
      reverse = true;
    }

    // make sure we don't hit the text item we started the search from
    if ( column == 0 && reverse ) {
      column -= item.length();
    }

    int itemIdx;
    int matchItemIdx;

    while ( toFind > 0 ) {
      if ( reverse ) {
        itemIdx = l.lastIndexOf( item, column - 1 );
        matchItemIdx = l.lastIndexOf( matchingItem, column - 1 );

        if ( itemIdx != -1 && ( matchItemIdx == -1 || itemIdx > matchItemIdx ) ) {
          ++toFind;
        }
      } else {
        itemIdx = l.indexOf( item, column );
        matchItemIdx = l.indexOf( matchingItem, column );

        if ( itemIdx != -1 && ( matchItemIdx == -1 || itemIdx < matchItemIdx ) ) {
          ++toFind;
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
            --toFind;
            c.setLine( line );
            c.setColumn( column );
          }
      } else { // no match, advance one line if possible
        ( reverse ) ? --line : ++line;
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
    return KateViRange::invalid();
  }

  r.endLine = line;
  r.endColumn = 0;
  r.jump = true;

  if (motionWillBeUsedWithCommand())
  {
    // Delete from cursor (inclusive) to the '{' (exclusive).
    // If we are on the first column, then delete the entire current line.
    r.motionType = ViMotion::ExclusiveMotion;
    if (m_view->cursorPosition().column() != 0)
    {
      r.endLine--;
      r.endColumn = doc()->lineLength(r.endLine);
    }
  }

  return r;
}

KateViRange KateViNormalMode::motionToPreviousBraceBlockStart()
{
  KateViRange r;

  m_stickyColumn = -1;

  int line = findLineStartingWitchChar( '{', getCount(), false );

  if ( line == -1 ) {
    return KateViRange::invalid();
  }

  r.endLine = line;
  r.endColumn = 0;
  r.jump = true;

  if (motionWillBeUsedWithCommand())
  {
    // With a command, do not include the { or the cursor position.
    r.motionType = ViMotion::ExclusiveMotion;
  }

  return r;
}

KateViRange KateViNormalMode::motionToNextBraceBlockEnd()
{
  KateViRange r;

  m_stickyColumn = -1;

  int line = findLineStartingWitchChar( '}', getCount() );

  if ( line == -1 ) {
    return KateViRange::invalid();
  }

  r.endLine = line;
  r.endColumn = 0;
  r.jump = true;

  if (motionWillBeUsedWithCommand())
  {
    // Delete from cursor (inclusive) to the '}' (exclusive).
    // If we are on the first column, then delete the entire current line.
    r.motionType = ViMotion::ExclusiveMotion;
    if (m_view->cursorPosition().column() != 0)
    {
      r.endLine--;
      r.endColumn = doc()->lineLength(r.endLine);
    }
  }

  return r;
}

KateViRange KateViNormalMode::motionToPreviousBraceBlockEnd()
{
  KateViRange r;

  m_stickyColumn = -1;

  int line = findLineStartingWitchChar( '}', getCount(), false );

  if ( line == -1 ) {
    return KateViRange::invalid();
  }

  r.endLine = line;
  r.endColumn = 0;
  r.jump = true;

  if (motionWillBeUsedWithCommand())
  {
    r.motionType = ViMotion::ExclusiveMotion;
  }

  return r;
}

KateViRange KateViNormalMode::motionToNextOccurrence()
{
  QString word = getWordUnderCursor();
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("\\<" + word + "\\>");
  word.prepend("\\b").append("\\b");

  m_viInputModeManager->setLastSearchPattern( word );
  m_viInputModeManager->setLastSearchBackwards( false );
  m_viInputModeManager->setLastSearchCaseSensitive( false );
  m_viInputModeManager->setLastSearchPlacesCursorAtEndOfMatch( false );

  const KateViRange match = findPatternForMotion( word, false, false, getWordRangeUnderCursor().start(), getCount() );
  return KateViRange(match.startLine, match.startColumn, ViMotion::ExclusiveMotion);
}

KateViRange KateViNormalMode::motionToPrevOccurrence()
{
  QString word = getWordUnderCursor();
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("\\<" + word + "\\>");
  word.prepend("\\b").append("\\b");

  m_viInputModeManager->setLastSearchPattern( word );
  m_viInputModeManager->setLastSearchBackwards( true );
  m_viInputModeManager->setLastSearchCaseSensitive( false );
  m_viInputModeManager->setLastSearchPlacesCursorAtEndOfMatch( false );

  // Search from the beginning of the word under the cursor, so that the current word isn't found
  // first.
  const KateViRange match = findPatternForMotion( word, true, false, getWordRangeUnderCursor().start(), getCount() );
  return KateViRange(match.startLine, match.startColumn, ViMotion::ExclusiveMotion);
}

KateViRange KateViNormalMode::motionToFirstLineOfWindow() {
    int lines_to_go;
    if (linesDisplayed() <= (unsigned int) m_viewInternal->endLine())
       lines_to_go = m_viewInternal->endLine() - linesDisplayed()- m_view->cursorPosition().line() + 1;
    else
        lines_to_go = - m_view->cursorPosition().line();

    KateViRange r = goLineUpDown(lines_to_go);

    r.endColumn = getFirstNonBlank(r.endLine);
    return r;
}

KateViRange KateViNormalMode::motionToMiddleLineOfWindow() {
    int lines_to_go;
    if (linesDisplayed() <= (unsigned int) m_viewInternal->endLine())
       lines_to_go = m_viewInternal->endLine() - linesDisplayed()/2 - m_view->cursorPosition().line();
    else
        lines_to_go = m_viewInternal->endLine()/2 - m_view->cursorPosition().line();
    KateViRange r = goLineUpDown(lines_to_go);

    r.endColumn = getFirstNonBlank(r.endLine);
    return r;
}

KateViRange KateViNormalMode::motionToLastLineOfWindow() {
    int lines_to_go;
    if (linesDisplayed() <= (unsigned int) m_viewInternal->endLine())
       lines_to_go = m_viewInternal->endLine() - m_view->cursorPosition().line();
    else
        lines_to_go = m_viewInternal->endLine() - m_view->cursorPosition().line();

    KateViRange r = goLineUpDown(lines_to_go);

    r.endColumn = getFirstNonBlank(r.endLine);
    return r;
}

KateViRange KateViNormalMode::motionToNextVisualLine() {
  return goVisualLineUpDown( getCount() );
}

KateViRange KateViNormalMode::motionToPrevVisualLine() {
  return goVisualLineUpDown( -getCount() );
}

KateViRange KateViNormalMode::motionToPreviousSentence()
{
  Cursor c = findSentenceStart();
  int linenum = c.line(), column;
  const bool skipSpaces = doc()->line(linenum).isEmpty();

  if (skipSpaces) {
    linenum--;
    if (linenum >= 0) {
      column = doc()->line(linenum).size() - 1;
    }
  } else {
    column = c.column();
  }

  for (int i = linenum; i >= 0; i--) {
    const QString &line = doc()->line(i);

    if (line.isEmpty() && !skipSpaces) {
      return KateViRange(i, 0, ViMotion::InclusiveMotion);
    }

    if (column < 0 && line.size() > 0) {
      column = line.size() - 1;
    }

    for (int j = column; j >= 0; j--) {
      if (skipSpaces || QString::fromLatin1(".?!").indexOf(line.at(j)) != -1) {
        c.setLine(i);
        c.setColumn(j);
        updateCursor(c);
        c = findSentenceStart();
        return KateViRange(c.line(), c.column(), ViMotion::InclusiveMotion);
      }
    }
    column = line.size() - 1;
  }
  return KateViRange(0, 0, ViMotion::InclusiveMotion);
}

KateViRange KateViNormalMode::motionToNextSentence()
{
  Cursor c = findSentenceEnd();
  int linenum = c.line(), column = c.column() + 1;
  const bool skipSpaces = doc()->line(linenum).isEmpty();

  for (int i = linenum; i < doc()->lines(); i++) {
    const QString &line = doc()->line(i);

    if (line.isEmpty() && !skipSpaces) {
      return KateViRange(i, 0, ViMotion::InclusiveMotion);
    }

    for (int j = column; j < line.size(); j++) {
      if (!line.at(j).isSpace()) {
        return KateViRange(i, j, ViMotion::InclusiveMotion);
      }
    }
    column = 0;
  }

  c = doc()->documentEnd();
  return KateViRange(c.line(), c.column(), ViMotion::InclusiveMotion);
}

KateViRange KateViNormalMode::motionToBeforeParagraph()
{
    Cursor c( m_view->cursorPosition() );

    int line = c.line();

    m_stickyColumn = -1;

    for ( unsigned int i = 0; i < getCount(); i++ ) {
        // advance at least one line, but if there are consecutive blank lines
        // skip them all
        do {
            line--;
        } while (line >= 0 && getLine( line+1 ).length() == 0);
        while ( line > 0 && getLine( line ).length() != 0 ) {
            line--;
        }
    }

    if ( line < 0 ) {
        line = 0;
    }

    KateViRange r( line, 0, ViMotion::InclusiveMotion );

    return r;
}

KateViRange KateViNormalMode::motionToAfterParagraph()
{
    Cursor c( m_view->cursorPosition() );

    int line = c.line();

    m_stickyColumn = -1;

    for ( unsigned int i = 0; i < getCount(); i++ ) {
        // advance at least one line, but if there are consecutive blank lines
        // skip them all
        do {
            line++;
        } while (line <= doc()->lines()-1 && getLine( line-1 ).length() == 0);
        while ( line < doc()->lines()-1 && getLine( line ).length() != 0 ) {
            line++;
        }
    }

    if ( line >= doc()->lines() ) {
        line = doc()->lines()-1;
    }

    // if we ended up on the last line, the cursor should be placed on the last column
    int column = (line == doc()->lines()-1) ? qMax( getLine( line ).length()-1, 0 ) : 0;

    KateViRange r( line, column, ViMotion::InclusiveMotion );

    return r;
}

KateViRange KateViNormalMode::motionToIncrementalSearchMatch()
{
  return KateViRange(m_positionWhenIncrementalSearchBegan.line(),
                     m_positionWhenIncrementalSearchBegan.column(),
                     m_view->cursorPosition().line(),
                     m_view->cursorPosition().column(), ViMotion::ExclusiveMotion);
}

////////////////////////////////////////////////////////////////////////////////
// TEXT OBJECTS
////////////////////////////////////////////////////////////////////////////////

KateViRange KateViNormalMode::textObjectAWord()
{
    Cursor c( m_view->cursorPosition() );

    Cursor c1 = c;

    bool startedOnSpace = false;
    if (doc()->character(c).isSpace())
    {
      startedOnSpace = true;
    }
    else
    {
      c1 = findPrevWordStart( c.line(), c.column()+1, true );
      if (!c1.isValid())
      {
        c1 = Cursor(0, 0);
      }
    }
    Cursor c2 = Cursor(c.line(), c.column() - 1);
    for (unsigned int i = 1; i <= getCount(); i++)
    {
      c2 = findWordEnd(c2.line(), c2.column());
    }
    if (!c1.isValid() || !c2.isValid())
    {
      return KateViRange::invalid();
    }
    // Adhere to some of Vim's bizarre rules of whether to swallow ensuing spaces or not.
    // Don't ask ;)
    const Cursor nextWordStart = findNextWordStart(c2.line(), c2.column());
    if (nextWordStart.isValid() && nextWordStart.line() == c2.line())
    {
      if (!startedOnSpace)
      {
        c2 = Cursor(nextWordStart.line(), nextWordStart.column() - 1);
      }
    }
    else
    {
      c2 = Cursor(c2.line(), doc()->lineLength(c2.line()) - 1);
    }
    bool swallowCarriageReturnAtEndOfLine = false;
    if (c2.line() != c.line() && c2.column() == doc()->lineLength(c2.line()) - 1)
    {
      // Greedily descend to the next line, so as to swallow the carriage return on this line.
      c2 = Cursor(c2.line() + 1, 0);
      swallowCarriageReturnAtEndOfLine = true;
    }
    const bool swallowPrecedingSpaces = (c2.column() == doc()->lineLength(c2.line()) - 1 && !doc()->character(c2).isSpace() ) || startedOnSpace || swallowCarriageReturnAtEndOfLine;
    if (swallowPrecedingSpaces)
    {
      if (c1.column() != 0)
      {
        const Cursor previousNonSpace = findPrevWordEnd(c.line(), c.column());
        if (previousNonSpace.isValid() && previousNonSpace.line() == c1.line())
        {
          c1 = Cursor(previousNonSpace.line(), previousNonSpace.column() + 1);
        }
        else if (startedOnSpace || swallowCarriageReturnAtEndOfLine)
        {
          c1 = Cursor(c1.line(), 0);
        }
      }
    }

    KateViRange r( c.line(), c.column(), !swallowCarriageReturnAtEndOfLine ? ViMotion::InclusiveMotion : ViMotion::ExclusiveMotion );

    r.startLine = c1.line();
    r.endLine = c2.line();
    r.startColumn = c1.column();
    r.endColumn = c2.column();

    return r;
}

KateViRange KateViNormalMode::textObjectInnerWord()
{
    Cursor c( m_view->cursorPosition() );

    Cursor c1 = findPrevWordStart( c.line(), c.column()+1, true );
    if (!c1.isValid())
    {
      c1 = Cursor(0, 0);
    }
    // need to start search in column-1 because it might be a one-character word
    Cursor c2( c.line(), c.column()-1 );

    for ( unsigned int i = 0; i < getCount(); i++ ) {
        c2 = findWordEnd( c2.line(), c2.column(), true );
    }

    if (!c2.isValid())
    {
      c2 = doc()->documentEnd();
    }

    KateViRange r;

    // sanity check
    if ( c1.line() != c2.line() || c1.column() > c2.column() ) {
      return KateViRange::invalid();
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

    Cursor c1 = c;

    bool startedOnSpace = false;
    if (doc()->character(c).isSpace())
    {
      startedOnSpace = true;
    }
    else
    {
      c1 = findPrevWORDStart( c.line(), c.column()+1, true );
      if (!c1.isValid())
      {
        c1 = Cursor(0, 0);
      }
    }
    Cursor c2 = Cursor(c.line(), c.column() - 1);
    for (unsigned int i = 1; i <= getCount(); i++)
    {
      c2 = findWORDEnd(c2.line(), c2.column());
    }
    if (!c1.isValid() || !c2.isValid())
    {
      return KateViRange::invalid();
    }
    // Adhere to some of Vim's bizarre rules of whether to swallow ensuing spaces or not.
    // Don't ask ;)
    const Cursor nextWordStart = findNextWordStart(c2.line(), c2.column());
    if (nextWordStart.isValid() && nextWordStart.line() == c2.line())
    {
      if (!startedOnSpace)
      {
        c2 = Cursor(nextWordStart.line(), nextWordStart.column() - 1);
      }
    }
    else
    {
      c2 = Cursor(c2.line(), doc()->lineLength(c2.line()) - 1);
    }
    bool swallowCarriageReturnAtEndOfLine = false;
    if (c2.line() != c.line() && c2.column() == doc()->lineLength(c2.line()) - 1)
    {
      // Greedily descend to the next line, so as to swallow the carriage return on this line.
      c2 = Cursor(c2.line() + 1, 0);
      swallowCarriageReturnAtEndOfLine = true;
    }
    const bool swallowPrecedingSpaces = (c2.column() == doc()->lineLength(c2.line()) - 1 && !doc()->character(c2).isSpace() ) || startedOnSpace || swallowCarriageReturnAtEndOfLine;
    if (swallowPrecedingSpaces)
    {
      if (c1.column() != 0)
      {
        const Cursor previousNonSpace = findPrevWORDEnd(c.line(), c.column());
        if (previousNonSpace.isValid() && previousNonSpace.line() == c1.line())
        {
          c1 = Cursor(previousNonSpace.line(), previousNonSpace.column() + 1);
        }
        else if (startedOnSpace || swallowCarriageReturnAtEndOfLine)
        {
          c1 = Cursor(c1.line(), 0);
        }
      }
    }

    KateViRange r( c.line(), c.column(), !swallowCarriageReturnAtEndOfLine ? ViMotion::InclusiveMotion : ViMotion::ExclusiveMotion );

    r.startLine = c1.line();
    r.endLine = c2.line();
    r.startColumn = c1.column();
    r.endColumn = c2.column();

    return r;
}

KateViRange KateViNormalMode::textObjectInnerWORD()
{
    Cursor c( m_view->cursorPosition() );

    Cursor c1 = findPrevWORDStart( c.line(), c.column()+1, true );
    if (!c1.isValid())
    {
      c1 = Cursor(0, 0);
    }
    Cursor c2( c );

    for ( unsigned int i = 0; i < getCount(); i++ ) {
        c2 = findWORDEnd( c2.line(), c2.column(), true );
    }

    if (!c2.isValid())
    {
      c2 = doc()->documentEnd();
    }

    KateViRange r;

    // sanity check
    if ( c1.line() != c2.line() || c1.column() > c2.column() ) {
      return KateViRange::invalid();
    } else {
        r.startLine = c1.line();
        r.endLine = c2.line();
        r.startColumn = c1.column();
        r.endColumn = c2.column();
    }

    return r;
}

Cursor KateViNormalMode::findSentenceStart()
{
  Cursor c(m_view->cursorPosition());
  int linenum = c.line(), column = c.column();
  int prev = column;

  for (int i = linenum; i >= 0; i--) {
    const QString &line = doc()->line(i);
    if (i != linenum) {
      column = line.size() - 1;
    }

    // An empty line is the end of a paragraph.
    if (line.isEmpty()) {
      return Cursor((i != linenum) ? i + 1 : i, prev);
    }

    prev = column;
    for (int j = column; j >= 0; j--) {
      if (line.at(j).isSpace()) {
        int lastSpace = j--;
        for (; j >= 0 && QString::fromLatin1("\"')]").indexOf(line.at(j)) != -1; j--);
        if (j >= 0 && QString::fromLatin1(".!?").indexOf(line.at(j)) != -1) {
          return Cursor(i, prev);
        }
        j = lastSpace;
      } else
        prev = j;
    }
  }

  return Cursor(0, 0);
}

Cursor KateViNormalMode::findSentenceEnd()
{
  Cursor c(m_view->cursorPosition());
  int linenum = c.line(), column = c.column();
  int j = 0, prev = 0;

  for (int i = linenum; i < doc()->lines(); i++) {
    const QString &line = doc()->line(i);

    // An empty line is the end of a paragraph.
    if (line.isEmpty()) {
      return Cursor(linenum, j);
    }

    // Iterating over the line to reach any '.', '!', '?'
    for (j = column; j < line.size(); j++) {
      if (QString::fromLatin1(".!?").indexOf(line.at(j)) != -1) {
        prev = j++;
        // Skip possible closing characters.
        for (; j < line.size() && QString::fromLatin1("\"')]").indexOf(line.at(j)) != -1; j++);

        if (j >= line.size()) {
          return Cursor(i, j - 1);
        }

        // And hopefully we're done...
        if (line.at(j).isSpace()) {
          return Cursor(i, j - 1);
        }
        j = prev;
      }
    }
    linenum = i;
    prev = column;
    column = 0;
  }

  return Cursor(linenum, j - 1);
}

Cursor KateViNormalMode::findParagraphStart()
{
  Cursor c(m_view->cursorPosition());
  const bool firstBlank = doc()->line(c.line()).isEmpty();
  int prev = c.line();

  for (int i = prev; i >= 0; i--) {
    if (doc()->line(i).isEmpty()) {
      if (i != prev) {
        prev = i + 1;
      }

      /* Skip consecutive empty lines. */
      if (firstBlank) {
        i--;
        for (; i >= 0 && doc()->line(i).isEmpty(); i--, prev--);
      }
      return Cursor(prev, 0);
    }
  }
  return Cursor(0, 0);
}

Cursor KateViNormalMode::findParagraphEnd()
{
  Cursor c(m_view->cursorPosition());
  int prev = c.line(), lines = doc()->lines();
  const bool firstBlank = doc()->line(prev).isEmpty();

  for (int i = prev; i < lines; i++) {
    if (doc()->line(i).isEmpty()) {
      if (i != prev) {
        prev = i - 1;
      }

      /* Skip consecutive empty lines. */
      if (firstBlank) {
        i++;
        for (; i < lines && doc()->line(i).isEmpty(); i++, prev++);
      }
      int length = doc()->lineLength(prev);
      return Cursor(prev, (length <= 0) ? 0 : length - 1);
    }
  }
  return doc()->documentEnd();
}

KateViRange KateViNormalMode::textObjectInnerSentence()
{
  KateViRange r;
  Cursor c1 = findSentenceStart();
  Cursor c2 = findSentenceEnd();
  updateCursor(c1);

  r.startLine = c1.line();
  r.startColumn = c1.column();
  r.endLine = c2.line();
  r.endColumn = c2.column();
  return r;
}

KateViRange KateViNormalMode::textObjectASentence()
{
  int i;
  KateViRange r = textObjectInnerSentence();
  const QString &line = doc()->line(r.endLine);

  // Skip whitespaces and tabs.
  for (i = r.endColumn + 1; i < line.size(); i++) {
    if (!line.at(i).isSpace()) {
      break;
    }
  }
  r.endColumn = i - 1;

  // Remove preceding spaces.
  if (r.startColumn != 0) {
    if (r.endColumn == line.size() - 1 && !line.at(r.endColumn).isSpace()) {
      const QString &line = doc()->line(r.startLine);
      for (i = r.startColumn - 1; i >= 0; i--) {
        if (!line.at(i).isSpace()) {
          break;
        }
      }
      r.startColumn = i + 1;
    }
  }
  return r;
}

KateViRange KateViNormalMode::textObjectInnerParagraph()
{
  KateViRange r;
  Cursor c1 = findParagraphStart();
  Cursor c2 = findParagraphEnd();
  updateCursor(c1);

  r.startLine = c1.line();
  r.startColumn = c1.column();
  r.endLine = c2.line();
  r.endColumn = c2.column();
  return r;
}

KateViRange KateViNormalMode::textObjectAParagraph()
{
  Cursor original(m_view->cursorPosition());
  KateViRange r = textObjectInnerParagraph();
  int lines = doc()->lines();

  if (r.endLine + 1 < lines) {
    // If the next line is empty, remove all subsequent empty lines.
    // Otherwise we'll grab the next paragraph.
    if (doc()->line(r.endLine + 1).isEmpty()) {
      for (int i = r.endLine + 1; i < lines && doc()->line(i).isEmpty(); i++) {
        r.endLine++;
      }
      r.endColumn = 0;
    } else {
      Cursor prev = m_view->cursorPosition();
      Cursor c(r.endLine + 1, 0);
      updateCursor(c);
      c = findParagraphEnd();
      updateCursor(prev);
      r.endLine = c.line();
      r.endColumn = c.column();
    }
  } else if (doc()->lineLength(r.startLine) > 0) {
    // We went too far, but maybe we can grab previous empty lines.
    for (int i = r.startLine - 1; i >= 0 && doc()->line(i).isEmpty(); i--) {
      r.startLine--;
    }
    r.startColumn = 0;
    updateCursor(Cursor(r.startLine, r.startColumn));
  } else {
    // We went too far and we're on empty lines, do nothing.
    return KateViRange::invalid();
  }
  return r;
}

KateViRange KateViNormalMode::textObjectAQuoteDouble()
{
    return findSurroundingQuotes( '"', false );
}

KateViRange KateViNormalMode::textObjectInnerQuoteDouble()
{
    return findSurroundingQuotes( '"', true );
}

KateViRange KateViNormalMode::textObjectAQuoteSingle()
{
    return findSurroundingQuotes( '\'', false );
}

KateViRange KateViNormalMode::textObjectInnerQuoteSingle()
{
    return findSurroundingQuotes( '\'', true );
}

KateViRange KateViNormalMode::textObjectABackQuote()
{
    return findSurroundingQuotes( '`', false );
}

KateViRange KateViNormalMode::textObjectInnerBackQuote()
{
    return findSurroundingQuotes( '`', true );
}


KateViRange KateViNormalMode::textObjectAParen()
{

    return findSurroundingBrackets( '(', ')', false,  '(', ')' );
}

KateViRange KateViNormalMode::textObjectInnerParen()
{

    return findSurroundingBrackets( '(', ')', true, '(', ')');
}

KateViRange KateViNormalMode::textObjectABracket()
{

    return findSurroundingBrackets( '[', ']', false,  '[', ']' );
}

KateViRange KateViNormalMode::textObjectInnerBracket()
{

    return findSurroundingBrackets( '[', ']', true, '[', ']' );
}

KateViRange KateViNormalMode::textObjectACurlyBracket()
{

    return findSurroundingBrackets( '{', '}', false,  '{', '}' );
}

KateViRange KateViNormalMode::textObjectInnerCurlyBracket()
{
  const KateViRange allBetweenCurlyBrackets = findSurroundingBrackets( '{', '}', true, '{', '}' );
  // Emulate the behaviour of vim, which tries to leave the closing bracket on its own line
  // if it was originally on a line different to that of the opening bracket.
  KateViRange innerCurlyBracket(allBetweenCurlyBrackets);

  if (innerCurlyBracket.startLine != innerCurlyBracket.endLine)
  {
    const bool openingBraceIsLastCharOnLine = innerCurlyBracket.startColumn == doc()->line(innerCurlyBracket.startLine).length();
    const bool stuffToDeleteIsAllOnEndLine = openingBraceIsLastCharOnLine &&
    innerCurlyBracket.endLine == innerCurlyBracket.startLine + 1;
    const QString textLeadingClosingBracket = doc()->line(innerCurlyBracket.endLine).mid(0, innerCurlyBracket.endColumn + 1);
    const bool closingBracketHasLeadingNonWhitespace = !textLeadingClosingBracket.trimmed().isEmpty();
    if (stuffToDeleteIsAllOnEndLine)
    {
      if (!closingBracketHasLeadingNonWhitespace)
      {
        // Nothing there to select - abort.
        return KateViRange::invalid();
      }
      else
      {
        // Shift the beginning of the range to the start of the line containing the closing bracket.
        innerCurlyBracket.startLine++;
        innerCurlyBracket.startColumn = 0;
      }
    }
    else
    {
      if (openingBraceIsLastCharOnLine && !closingBracketHasLeadingNonWhitespace)
      {
        innerCurlyBracket.startLine++;
        innerCurlyBracket.startColumn = 0;
        m_lastMotionWasLinewiseInnerBlock = true;
      }
      {
        // The line containing the end bracket is left alone if the end bracket is preceded by just whitespace,
        // else we need to delete everything (i.e. end up with "{}")
        if (!closingBracketHasLeadingNonWhitespace)
        {
          // Shrink the endpoint of the range so that it ends at the end of the line above,
          // leaving the closing bracket on its own line.
          innerCurlyBracket.endLine--;
          innerCurlyBracket.endColumn = doc()->line(innerCurlyBracket.endLine).length();
        }
      }
    }

  }
  return innerCurlyBracket;
}

KateViRange KateViNormalMode::textObjectAInequalitySign()
{

    return findSurroundingBrackets( '<', '>', false,  '<', '>' );
}

KateViRange KateViNormalMode::textObjectInnerInequalitySign()
{

    return findSurroundingBrackets( '<', '>', true, '<', '>' );
}

KateViRange KateViNormalMode::textObjectAComma()
{
  return textObjectComma(false);
}

KateViRange KateViNormalMode::textObjectInnerComma()
{
  return textObjectComma(true);
}

// add commands
// when adding commands here, remember to add them to visual mode too (if applicable)
void KateViNormalMode::initializeCommands()
{
  ADDCMD("a", commandEnterInsertModeAppend, IS_CHANGE );
  ADDCMD("A", commandEnterInsertModeAppendEOL, IS_CHANGE );
  ADDCMD("i", commandEnterInsertMode, IS_CHANGE );
  ADDCMD("<insert>", commandEnterInsertMode, IS_CHANGE );
  ADDCMD("I", commandEnterInsertModeBeforeFirstNonBlankInLine, IS_CHANGE );
  ADDCMD("gi", commandEnterInsertModeLast, IS_CHANGE );
  ADDCMD("v", commandEnterVisualMode, 0 );
  ADDCMD("V", commandEnterVisualLineMode, 0 );
  ADDCMD("<c-v>", commandEnterVisualBlockMode, 0 );
  ADDCMD("gv", commandReselectVisual, SHOULD_NOT_RESET );
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
  ADDCMD("<delete>", commandDeleteChar, IS_CHANGE );
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
  ADDCMD("gp", commandgPaste, IS_CHANGE );
  ADDCMD("gP", commandgPasteBefore, IS_CHANGE );
  ADDCMD("]p", commandIndentedPaste, IS_CHANGE );
  ADDCMD("[p", commandIndentedPasteBefore, IS_CHANGE );
  ADDCMD("r.", commandReplaceCharacter, IS_CHANGE | REGEX_PATTERN );
  ADDCMD("R", commandEnterReplaceMode, IS_CHANGE );
  ADDCMD(":", commandSwitchToCmdLine, 0 );
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
  ADDCMD("<c-u>", commandScrollHalfPageUp, 0 );
  ADDCMD("<c-d>", commandScrollHalfPageDown, 0 );
  ADDCMD("z.", commandCenterViewOnNonBlank, 0);
  ADDCMD("zz", commandCenterViewOnCursor, 0);
  ADDCMD("z<return>", commandTopViewOnNonBlank, 0);
  ADDCMD("zt", commandTopViewOnCursor, 0);
  ADDCMD("z-", commandBottomViewOnNonBlank, 0);
  ADDCMD("zb", commandBottomViewOnCursor, 0);
  ADDCMD("ga", commandPrintCharacterCode, SHOULD_NOT_RESET );
  ADDCMD(".", commandRepeatLastChange, 0 );
  ADDCMD("==", commandAlignLine, IS_CHANGE );
  ADDCMD("=", commandAlignLines, IS_CHANGE | NEEDS_MOTION);
  ADDCMD("~", commandChangeCase, IS_CHANGE );
  ADDCMD("g~", commandChangeCaseRange, IS_CHANGE | NEEDS_MOTION );
  ADDCMD("g~~", commandChangeCaseLine, IS_CHANGE );
  ADDCMD("<c-a>", commandAddToNumber, IS_CHANGE );
  ADDCMD("<c-x>", commandSubtractFromNumber, IS_CHANGE );
  ADDCMD("<c-o>", commandGoToPrevJump, 0);
  ADDCMD("<c-i>", commandGoToNextJump, 0);

  ADDCMD("<c-w>h",commandSwitchToLeftView,0);
  ADDCMD("<c-w><c-h>",commandSwitchToLeftView,0);
  ADDCMD("<c-w><left>",commandSwitchToLeftView,0);
  ADDCMD("<c-w>j",commandSwitchToDownView,0);
  ADDCMD("<c-w><c-j>",commandSwitchToDownView,0);
  ADDCMD("<c-w><down>",commandSwitchToDownView,0);
  ADDCMD("<c-w>k",commandSwitchToUpView,0);
  ADDCMD("<c-w><c-k>",commandSwitchToUpView,0);
  ADDCMD("<c-w><up>",commandSwitchToUpView,0);
  ADDCMD("<c-w>l",commandSwitchToRightView,0);
  ADDCMD("<c-w><c-l>",commandSwitchToRightView,0);
  ADDCMD("<c-w><right>",commandSwitchToRightView,0);
  ADDCMD("<c-w>w",commandSwitchToNextView,0);
  ADDCMD("<c-w><c-w>",commandSwitchToNextView,0);

  ADDCMD("<c-w>s", commandSplitHoriz, 0);
  ADDCMD("<c-w>S", commandSplitHoriz, 0);
  ADDCMD("<c-w><c-s>", commandSplitHoriz, 0);
  ADDCMD("<c-w>v", commandSplitVert, 0);
  ADDCMD("<c-w><c-v>", commandSplitVert, 0);

  ADDCMD("gt", commandSwitchToNextTab,0);
  ADDCMD("gT", commandSwitchToPrevTab,0);

  ADDCMD("gqq", commandFormatLine, IS_CHANGE);
  ADDCMD("gq", commandFormatLines, IS_CHANGE | NEEDS_MOTION);

  ADDCMD("zo", commandExpandLocal, 0 );
  ADDCMD("zc", commandCollapseLocal, 0 );
  ADDCMD("za", commandToggleRegionVisibility, 0 );
  ADDCMD("zr", commandExpandAll, 0 );
  ADDCMD("zm", commandCollapseToplevelNodes, 0 );

  ADDCMD("q.", commandStartRecordingMacro, REGEX_PATTERN);
  ADDCMD("@.", commandReplayMacro, REGEX_PATTERN);

  ADDCMD("ZZ", commandCloseWrite, 0);
  ADDCMD("ZQ", commandCloseNocheck, 0);

  // regular motions
  ADDMOTION("h", motionLeft, 0 );
  ADDMOTION("<left>", motionLeft, 0 );
  ADDMOTION("<backspace>", motionLeft, 0 );
  ADDMOTION("j", motionDown, 0 );
  ADDMOTION("<down>", motionDown, 0 );
  ADDMOTION("<enter>", motionDownToFirstNonBlank, 0 );
  ADDMOTION("<return>", motionDownToFirstNonBlank, 0 );
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
  ADDMOTION("w", motionWordForward, IS_NOT_LINEWISE );
  ADDMOTION("W", motionWORDForward, IS_NOT_LINEWISE );
  ADDMOTION("<c-right>", motionWordForward, IS_NOT_LINEWISE);
  ADDMOTION("<c-left>", motionWordBackward, IS_NOT_LINEWISE);
  ADDMOTION("b", motionWordBackward, 0 );
  ADDMOTION("B", motionWORDBackward, 0 );
  ADDMOTION("e", motionToEndOfWord, 0 );
  ADDMOTION("E", motionToEndOfWORD, 0 );
  ADDMOTION("ge", motionToEndOfPrevWord, 0 );
  ADDMOTION("gE", motionToEndOfPrevWORD, 0 );
  ADDMOTION("|", motionToScreenColumn, 0 );
  ADDMOTION("%", motionToMatchingItem, IS_NOT_LINEWISE );
  ADDMOTION("`[a-zA-Z^><\\.\\[\\]]", motionToMark, REGEX_PATTERN );
  ADDMOTION("'[a-zA-Z^><]", motionToMarkLine, REGEX_PATTERN );
  ADDMOTION("[[", motionToPreviousBraceBlockStart, IS_NOT_LINEWISE );
  ADDMOTION("]]", motionToNextBraceBlockStart, IS_NOT_LINEWISE );
  ADDMOTION("[]", motionToPreviousBraceBlockEnd, IS_NOT_LINEWISE );
  ADDMOTION("][", motionToNextBraceBlockEnd, IS_NOT_LINEWISE );
  ADDMOTION("*", motionToNextOccurrence, 0 );
  ADDMOTION("#", motionToPrevOccurrence, 0 );
  ADDMOTION("H", motionToFirstLineOfWindow, 0 );
  ADDMOTION("M", motionToMiddleLineOfWindow, 0 );
  ADDMOTION("L", motionToLastLineOfWindow, 0 );
  ADDMOTION("gj", motionToNextVisualLine, 0 );
  ADDMOTION("gk", motionToPrevVisualLine, 0 );
  ADDMOTION("(", motionToPreviousSentence, 0 );
  ADDMOTION(")", motionToNextSentence, 0 );
  ADDMOTION("{", motionToBeforeParagraph, 0 );
  ADDMOTION("}", motionToAfterParagraph, 0 );

  // text objects
  ADDMOTION("iw", textObjectInnerWord, 0 );
  ADDMOTION("aw", textObjectAWord, IS_NOT_LINEWISE );
  ADDMOTION("iW", textObjectInnerWORD, 0 );
  ADDMOTION("aW", textObjectAWORD, IS_NOT_LINEWISE );
  ADDMOTION("is", textObjectInnerSentence, IS_NOT_LINEWISE );
  ADDMOTION("as", textObjectASentence, IS_NOT_LINEWISE );
  ADDMOTION("ip", textObjectInnerParagraph, IS_NOT_LINEWISE );
  ADDMOTION("ap", textObjectAParagraph, IS_NOT_LINEWISE );
  ADDMOTION("i\"", textObjectInnerQuoteDouble, IS_NOT_LINEWISE );
  ADDMOTION("a\"", textObjectAQuoteDouble, IS_NOT_LINEWISE );
  ADDMOTION("i'", textObjectInnerQuoteSingle, IS_NOT_LINEWISE );
  ADDMOTION("a'", textObjectAQuoteSingle, IS_NOT_LINEWISE );
  ADDMOTION("i`", textObjectInnerBackQuote, IS_NOT_LINEWISE );
  ADDMOTION("a`", textObjectABackQuote, IS_NOT_LINEWISE );
  ADDMOTION("i[()b]", textObjectInnerParen, REGEX_PATTERN | IS_NOT_LINEWISE);
  ADDMOTION("a[()b]", textObjectAParen, REGEX_PATTERN | IS_NOT_LINEWISE);
  ADDMOTION("i[{}B]", textObjectInnerCurlyBracket, REGEX_PATTERN | IS_NOT_LINEWISE);
  ADDMOTION("a[{}B]", textObjectACurlyBracket, REGEX_PATTERN | IS_NOT_LINEWISE);
  ADDMOTION("i[><]", textObjectInnerInequalitySign, REGEX_PATTERN | IS_NOT_LINEWISE);
  ADDMOTION("a[><]", textObjectAInequalitySign, REGEX_PATTERN | IS_NOT_LINEWISE);
  ADDMOTION("i[\\[\\]]", textObjectInnerBracket, REGEX_PATTERN | IS_NOT_LINEWISE);
  ADDMOTION("a[\\[\\]]", textObjectABracket, REGEX_PATTERN  | IS_NOT_LINEWISE);
  ADDMOTION("i,", textObjectInnerComma, IS_NOT_LINEWISE );
  ADDMOTION("a,", textObjectAComma, IS_NOT_LINEWISE);

  ADDMOTION("/<enter>", motionToIncrementalSearchMatch, IS_NOT_LINEWISE);
  ADDMOTION("?<enter>", motionToIncrementalSearchMatch, IS_NOT_LINEWISE);
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

// returns the operation mode that should be used. this is decided by using the following heuristic:
// 1. if we're in visual block mode, it should be Block
// 2. if we're in visual line mode OR the range spans several lines, it should be LineWise
// 3. if neither of these is true, CharWise is returned
// 4. there are some motion that makes all operator charwise, if we have one of them mode will be CharWise
OperationMode KateViNormalMode::getOperationMode() const
{
  OperationMode m = CharWise;

  if ( m_viInputModeManager->getCurrentViMode() == VisualBlockMode ) {
    m = Block;
  } else if ( m_viInputModeManager->getCurrentViMode() == VisualLineMode
    || ( m_commandRange.startLine != m_commandRange.endLine
      && m_viInputModeManager->getCurrentViMode() != VisualMode )) {
    m = LineWise;
  }

  if ( m_commandWithMotion && !m_linewiseCommand )
        m = CharWise;

  if (m_lastMotionWasLinewiseInnerBlock)
    m = LineWise;

  return m;
}

bool KateViNormalMode::paste(PasteLocation pasteLocation, bool isgPaste, bool isIndentedPaste)
{
  Cursor pasteAt( m_view->cursorPosition() );
  Cursor cursorAfterPaste = pasteAt;
  QChar reg = getChosenRegister( m_defaultRegister );

  OperationMode m = getRegisterFlag( reg );
  QString textToInsert = getRegisterContent( reg );
  const bool isTextMultiLine = textToInsert.split("\n").count() > 1;

  // In temporary normal mode, p/P act as gp/gP.
  isgPaste |= m_viInputModeManager->getTemporaryNormalMode();

  if ( textToInsert.isEmpty() ) {
    error(i18n("Nothing in register %1", reg ));
    return false;
  }

  if ( getCount() > 1 ) {
    textToInsert = textToInsert.repeated( getCount() ); // FIXME: does this make sense for blocks?
  }


  if ( m == LineWise ) {
    pasteAt.setColumn( 0 );
    if (isIndentedPaste)
    {
      // Note that this does indeed work if there is no non-whitespace on the current line or if
      // the line is empty!
      const QString leadingWhiteSpaceOnCurrentLine = doc()->line(pasteAt.line()).mid(0, doc()->line(pasteAt.line()).indexOf(QRegExp("[^\\s]")));
      const QString leadingWhiteSpaceOnFirstPastedLine = textToInsert.mid(0, textToInsert.indexOf(QRegExp("[^\\s]")));
      // QString has no "left trim" method, bizarrely.
      while (textToInsert[0].isSpace())
      {
        textToInsert = textToInsert.mid(1);
      }
      textToInsert.prepend(leadingWhiteSpaceOnCurrentLine);
      // Remove the last \n, temporarily: we're going to alter the indentation of each pasted line
      // by doing a search and replace on '\n's, but don't want to alter this one.
      textToInsert.chop( 1 );
      textToInsert.replace(QString('\n') + leadingWhiteSpaceOnFirstPastedLine, QString('\n') + leadingWhiteSpaceOnCurrentLine);
      textToInsert.append('\n'); // Re-add the temporarily removed last '\n'.
    }
    if (pasteLocation == AfterCurrentPosition)
    {
      textToInsert.chop( 1 ); // remove the last \n
      pasteAt.setColumn( doc()->lineLength( pasteAt.line() ) ); // paste after the current line and ...
      textToInsert.prepend( QChar( '\n' ) ); // ... prepend a \n, so the text starts on a new line

      cursorAfterPaste.setLine( cursorAfterPaste.line()+1 );
    }
    if (isgPaste)
    {
      cursorAfterPaste.setLine(cursorAfterPaste.line() + textToInsert.split("\n").length() - 1);
    }
  } else {
    if (pasteLocation == AfterCurrentPosition)
    {
      // Move cursor forward one before we paste.  The position after the paste must also
      // be updated accordingly.
      if ( getLine( pasteAt.line() ).length() > 0 ) {
        pasteAt.setColumn( pasteAt.column()+1 );
      }
      cursorAfterPaste = pasteAt;
    }
    const bool leaveCursorAtStartOfPaste = isTextMultiLine && !isgPaste;
    if (!leaveCursorAtStartOfPaste)
    {
      cursorAfterPaste = cursorPosAtEndOfPaste(pasteAt, textToInsert);
      if (!isgPaste)
      {
        cursorAfterPaste.setColumn(cursorAfterPaste.column() - 1);
      }
    }
  }

  doc()->editBegin();
  if (m_view->selection())
  {
    pasteAt = m_view->selectionRange().start();
    doc()->removeText(m_view->selectionRange());
  }
  doc()->insertText( pasteAt, textToInsert, m == Block );
  doc()->editEnd();

  if (cursorAfterPaste.line() >= doc()->lines())
  {
    cursorAfterPaste.setLine(doc()->lines() - 1);
  }
  updateCursor( cursorAfterPaste );

  return true;
}

Cursor KateViNormalMode::cursorPosAtEndOfPaste(const Cursor& pasteLocation, const QString& pastedText)
{
  Cursor cAfter = pasteLocation;
  const QStringList textLines = pastedText.split("\n");
  if (textLines.length() == 1)
  {
    cAfter.setColumn(cAfter.column() + pastedText.length());
  }
  else
  {
    cAfter.setColumn(textLines.last().length() - 0);
    cAfter.setLine(cAfter.line() + textLines.length() - 1);
  }
  return cAfter;
}

void KateViNormalMode::joinLines(unsigned int from, unsigned int to) const
{
  // make sure we don't try to join lines past the document end
  if ( to >= (unsigned int)(doc()->lines()) ) {
    to = doc()->lines()-1;
  }

  // joining one line is a no-op
  if ( from == to ) return;

  doc()->joinLines( from, to );
}

void KateViNormalMode::reformatLines(unsigned int from, unsigned int to) const
{
  joinLines( from, to );
  doc()->wrapText( from, to );
}

int KateViNormalMode::getFirstNonBlank(int line) const
{
    if (line < 0) {
        line = m_view->cursorPosition().line();
    }

    // doc()->plainKateTextLine returns NULL if the line is out of bounds.
    Kate::TextLine l = doc()->plainKateTextLine(line);
    Q_ASSERT(l);

    int c = l->firstChar();
    return (c < 0) ? 0 : c;
}

// Tries to shrinks toShrink so that it fits tightly around rangeToShrinkTo.
void KateViNormalMode::shrinkRangeAroundCursor(KateViRange& toShrink, const KateViRange& rangeToShrinkTo)
{
  if (!toShrink.valid || !rangeToShrinkTo.valid)
  {
    return;
  }
  Cursor cursorPos = m_view->cursorPosition();
  if (rangeToShrinkTo.startLine >= cursorPos.line())
  {
    if (rangeToShrinkTo.startLine > cursorPos.line())
    {
      // Does not surround cursor; aborting.
      return;
    }
    Q_ASSERT(rangeToShrinkTo.startLine == cursorPos.line());
    if (rangeToShrinkTo.startColumn > cursorPos.column())
    {
      // Does not surround cursor; aborting.
      return;
    }
  }
  if (rangeToShrinkTo.endLine <= cursorPos.line())
  {
    if (rangeToShrinkTo.endLine < cursorPos.line())
    {
      // Does not surround cursor; aborting.
      return;
    }
    Q_ASSERT(rangeToShrinkTo.endLine == cursorPos.line());
    if (rangeToShrinkTo.endColumn < cursorPos.column())
    {
      // Does not surround cursor; aborting.
      return;
    }
  }

  if (toShrink.startLine <= rangeToShrinkTo.startLine)
  {
    if (toShrink.startLine < rangeToShrinkTo.startLine)
    {
    toShrink.startLine = rangeToShrinkTo.startLine;
    toShrink.startColumn = rangeToShrinkTo.startColumn;
    }
    Q_ASSERT(toShrink.startLine == rangeToShrinkTo.startLine);
    if (toShrink.startColumn < rangeToShrinkTo.startColumn)
    {
      toShrink.startColumn = rangeToShrinkTo.startColumn;
    }
  }
  if (toShrink.endLine >= rangeToShrinkTo.endLine)
  {
    if (toShrink.endLine > rangeToShrinkTo.endLine)
    {
    toShrink.endLine = rangeToShrinkTo.endLine;
    toShrink.endColumn = rangeToShrinkTo.endColumn;
    }
    Q_ASSERT(toShrink.endLine == rangeToShrinkTo.endLine);
    if (toShrink.endColumn > rangeToShrinkTo.endColumn)
    {
      toShrink.endColumn = rangeToShrinkTo.endColumn;
    }
  }
}

KateViRange KateViNormalMode::textObjectComma(bool inner)
{
  // Basic algorithm: look left and right of the cursor for all combinations
  // of enclosing commas and the various types of brackets, and pick the pair
  // closest to the cursor that surrounds the cursor.
  KateViRange r(0, 0, m_view->doc()->lines(), m_view->doc()->line(m_view->doc()->lastLine()).length(), ViMotion::InclusiveMotion);

  shrinkRangeAroundCursor(r, findSurroundingQuotes( ',', inner ));
  shrinkRangeAroundCursor(r, findSurroundingBrackets( '(', ')', inner, '(', ')' ));
  shrinkRangeAroundCursor(r, findSurroundingBrackets( '{', '}', inner, '{', '}' ));
  shrinkRangeAroundCursor(r, findSurroundingBrackets( ',', ')', inner, '(', ')' ));
  shrinkRangeAroundCursor(r, findSurroundingBrackets( ',', ']', inner, '[', ']' ));
  shrinkRangeAroundCursor(r, findSurroundingBrackets( ',', '}', inner, '{', '}' ));
  shrinkRangeAroundCursor(r, findSurroundingBrackets( '(', ',', inner, '(', ')'  ));
  shrinkRangeAroundCursor(r, findSurroundingBrackets( '[', ',', inner, '[', ']' ));
  shrinkRangeAroundCursor(r, findSurroundingBrackets( '{', ',', inner, '{', '}' ));
  return r;
}

void KateViNormalMode::updateYankHighlightAttrib()
{
  if (!m_highlightYankAttribute)
  {
    m_highlightYankAttribute = new KTextEditor::Attribute;
  }
  const QColor& yankedColor = m_view->renderer()->config()->savedLineColor();
  m_highlightYankAttribute->setBackground(yankedColor);
  KTextEditor::Attribute::Ptr mouseInAttribute(new KTextEditor::Attribute());
  mouseInAttribute->setFontBold(true);
  m_highlightYankAttribute->setDynamicAttribute (KTextEditor::Attribute::ActivateMouseIn, mouseInAttribute);
  m_highlightYankAttribute->dynamicAttribute (KTextEditor::Attribute::ActivateMouseIn)->setBackground(yankedColor);
}

void KateViNormalMode::highlightYank(const KateViRange& range, const OperationMode mode)
{
  clearYankHighlight();

  // current MovingRange doesn't support block mode selection so split the
  // block range into per-line ranges
  if (mode == Block)
  {
    for (int i = range.startLine; i <= range.endLine; i++)
      addHighlightYank( Range(i, range.startColumn, i, range.endColumn) );
  }
  else
    addHighlightYank( Range(range.startLine, range.startColumn, range.endLine, range.endColumn) );
}

void KateViNormalMode::addHighlightYank(const Range& yankRange)
{
  KTextEditor::MovingRange *highlightedYank = m_view->doc()->newMovingRange(yankRange, Kate::TextRange::DoNotExpand);
  highlightedYank->setView(m_view); // show only in this view
  highlightedYank->setAttributeOnlyForViews(true);
  // use z depth defined in moving ranges interface
  highlightedYank->setZDepth (-10000.0);
  highlightedYank->setAttribute(m_highlightYankAttribute);

  highlightedYankForDocument().insert(highlightedYank);
}

void KateViNormalMode::clearYankHighlight()
{
  QSet<KTextEditor::MovingRange *> &pHighlightedYanks = highlightedYankForDocument();
  qDeleteAll(pHighlightedYanks);
  pHighlightedYanks.clear();
}

void KateViNormalMode::aboutToDeleteMovingInterfaceContent()
{
  QSet<KTextEditor::MovingRange *> &pHighlightedYanks = highlightedYankForDocument();
  // Prevent double-deletion in case this KateViNormalMode is deleted.
  pHighlightedYanks.clear();
}

QSet<KTextEditor::MovingRange *> &KateViNormalMode::highlightedYankForDocument()
{
  // Work around the fact that both Normal and Visual mode will have their own m_highlightedYank -
  // make Normal's the canonical one.
  return m_viInputModeManager->getViNormalMode()->m_highlightedYanks;
}

bool KateViNormalMode::waitingForRegisterOrCharToSearch()
{
  if (m_keys.size() != 1) {
    return false;
  }

  QChar lastChar = m_keys[0];
  return (lastChar == 'f' || lastChar == 't' || lastChar == 'F' || lastChar == 'T' || lastChar == 'r' || lastChar == 'q' || lastChar == '@');
}

void KateViNormalMode::textInserted(KTextEditor::Document* document, Range range)
{
  Q_UNUSED(document);
  const bool isInsertMode = m_viInputModeManager->getCurrentViMode() == InsertMode;
  const bool continuesInsertion = range.start().line() == m_currentChangeEndMarker.line() && range.start().column() == m_currentChangeEndMarker.column();
  const bool beginsWithNewline = doc()->text(range)[0] == '\n';
  if (!continuesInsertion)
  {
    Cursor newBeginMarkerPos = range.start();
    if (beginsWithNewline && !isInsertMode)
    {
      // Presumably a linewise paste, in which case we ignore the leading '\n'
      newBeginMarkerPos = Cursor(newBeginMarkerPos.line() + 1, 0);
    }
    m_viInputModeManager->addMark(doc(), '[', newBeginMarkerPos, false);
  }
  m_viInputModeManager->addMark(doc(), '.', range.start());
  Cursor editEndMarker = range.end();
  if (!isInsertMode)
  {
    editEndMarker.setColumn(editEndMarker.column() - 1);
  }
  m_viInputModeManager->addMark(doc(), ']', editEndMarker);
  m_currentChangeEndMarker = range.end();
  if (m_isUndo)
  {
    const bool addsMultipleLines = range.start().line() != range.end().line();
    m_viInputModeManager->addMark(doc(), '[', Cursor(m_viInputModeManager->getMarkPosition('[').line(), 0));
    if (addsMultipleLines)
    {
      m_viInputModeManager->addMark(doc(), ']', Cursor(m_viInputModeManager->getMarkPosition(']').line() + 1, 0));
      m_viInputModeManager->addMark(doc(), '.', Cursor(m_viInputModeManager->getMarkPosition('.').line() + 1, 0));
    }
    else
    {
      m_viInputModeManager->addMark(doc(), ']', Cursor(m_viInputModeManager->getMarkPosition(']').line(), 0));
      m_viInputModeManager->addMark(doc(), '.', Cursor(m_viInputModeManager->getMarkPosition('.').line(), 0));
    }
  }
}

void KateViNormalMode::textRemoved(KTextEditor::Document* document , Range range)
{
  Q_UNUSED(document);
  const bool isInsertMode = m_viInputModeManager->getCurrentViMode() == InsertMode;
  m_viInputModeManager->addMark(doc(), '.', range.start());
  if (!isInsertMode)
  {
    // Don't go resetting [ just because we did a Ctrl-h!
    m_viInputModeManager->addMark(doc(), '[', range.start());
  }
  else
  {
    // Don't go disrupting our continued insertion just because we did a Ctrl-h!
    m_currentChangeEndMarker = range.start();
  }
  m_viInputModeManager->addMark(doc(), ']', range.start());
  if (m_isUndo)
  {
    // Slavishly follow Vim's weird rules: if an undo removes several lines, then all markers should
    // be at the beginning of the line after the last line removed, else they should at the beginning
    // of the line above that.
    const int markerLineAdjustment = (range.start().line() != range.end().line()) ? 1 : 0;
    m_viInputModeManager->addMark(doc(), '[', Cursor(m_viInputModeManager->getMarkPosition('[').line() + markerLineAdjustment, 0));
    m_viInputModeManager->addMark(doc(), ']', Cursor(m_viInputModeManager->getMarkPosition(']').line() + markerLineAdjustment, 0));
    m_viInputModeManager->addMark(doc(), '.', Cursor(m_viInputModeManager->getMarkPosition('.').line() + markerLineAdjustment, 0));
  }
}

void KateViNormalMode::undoBeginning()
{
  m_isUndo = true;
}

void KateViNormalMode::undoEnded()
{
  m_isUndo = false;
}

