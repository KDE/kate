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

#include "katevivisualmode.h"
#include "katesmartrange.h"
#include "katevirange.h"

KateViVisualMode::KateViVisualMode( KateViInputModeManager* viInputModeManager, KateView *view, KateViewInternal *viewInternal )
  : KateViNormalMode( viInputModeManager, view, viewInternal )
{
  m_start.setPosition( -1, -1 );
  m_previous.setPosition( -1, -1 );

  m_mode = VisualMode;

  initializeCommands();
}

KateViVisualMode::~KateViVisualMode()
{
}

void KateViVisualMode::updateDirty( bool entireView ) const
{
  KTextEditor::Cursor c = m_view->cursorPosition();

  if ( entireView ) {
    m_view->tagLines(0, m_view->doc()->lastLine() );
  } else {
    // tag lines that might have changed their highlighting as dirty
    if ( c.line() >= m_start.line() ) { // selection in the "normal" direction
      if ( c.line() > m_previous.line() ) {
        m_view->tagLines(m_start.line(), c.line());
      } else {
        m_view->tagLines(m_start.line(), m_previous.line());
      }
    } else { // selection in the "opposite" direction, i.e., upward or to the left
      if ( c.line() < m_previous.line() ) {
        m_view->tagLines(c.line(), m_start.line());
      } else {
        m_view->tagLines(m_previous.line(), m_start.line());
      }
    }
  }
  m_view->updateView( true );
}

void KateViVisualMode::goToPos( const KateViRange &r )
{
  KTextEditor::Cursor c = m_view->cursorPosition();
  m_previous = c;

  if ( r.startLine != -1 && r.startColumn != -1 && c == m_start ) {
    m_start.setLine( r.startLine );
    m_start.setColumn( r.startColumn );
    c.setLine( r.endLine );
    c.setColumn( r.endColumn );
  } else if ( r.startLine != -1 && r.startColumn != -1 && c < m_start ) {
    c.setLine( r.startLine );
    c.setColumn( r.startColumn );
  } else {
    c.setLine( r.endLine );
    c.setColumn( r.endColumn );
  }

  if ( c.line() >= doc()->lines() ) {
    c.setLine( doc()->lines()-1 );
  }

  updateCursor( c );

  m_commandRange.startLine = m_start.line();
  m_commandRange.startColumn = m_start.column();
  m_commandRange.endLine = r.endLine;
  m_commandRange.endColumn = r.endColumn;

  updateDirty();
}

void KateViVisualMode::reset()
{
    m_awaitingMotionOrTextObject.push_back( 0 ); // search for text objects/motion from char 0

    m_mode = VisualMode;

    // only switch to normal mode if still in visual mode. commands like c, s, ...
    // can have switched to insert mode
    if ( m_viInputModeManager->getCurrentViMode() == VisualMode
        || m_viInputModeManager->getCurrentViMode() == VisualLineMode
        || m_viInputModeManager->getCurrentViMode() == VisualBlockMode ) {
      startNormalMode();
    }

    // TODO: set register < and > (see :help '< in vim)

    m_start.setPosition( -1, -1 );
    m_previous.setPosition( -1, -1 );

    // remove highlighting
    updateDirty( true );
}

void KateViVisualMode::init()
{
    m_start = m_view->cursorPosition();
    updateDirty();

    m_awaitingMotionOrTextObject.push_back( 0 ); // search for text objects/motion from char 0

    m_commandRange.startLine = m_commandRange.endLine = m_start.line();
    m_commandRange.startColumn = m_commandRange.endColumn = m_start.column();
}

void KateViVisualMode::setVisualLine( bool l )
{
  if ( l ) {
    m_mode = VisualLineMode;
  } else {
    m_mode = VisualMode;
  }
  updateDirty( true );
}

void KateViVisualMode::setVisualBlock( bool l )
{
  if ( l ) {
    m_mode = VisualBlockMode;
  } else {
    m_mode = VisualMode;
  }
  updateDirty( true );
}

void KateViVisualMode::setVisualModeType( ViMode mode )
{
  if ( mode != VisualMode && mode != VisualLineMode && mode != VisualBlockMode ) {
    kDebug( 13070 ) << "ERROR: invalid mode requested.";
    m_mode = VisualMode;
  } else {
    m_mode = mode;
  }
}

void KateViVisualMode::switchStartEnd()
{
  KTextEditor::Cursor c = m_start;
  m_start = m_view->cursorPosition();

  updateCursor( c );

  m_stickyColumn = -1;

  updateDirty();
}

KTextEditor::Range KateViVisualMode::getVisualRange() const
{
  KTextEditor::Cursor c = m_view->cursorPosition();

  int startLine = qMin( c.line(), m_start.line() );
  int endLine = qMax( c.line(), m_start.line() );
  int startCol;
  int endCol;

  if ( m_mode == VisualBlockMode ) {
    startCol = qMin( c.column(), m_start.column() );
    endCol = qMax( c.column(), m_start.column() );
  } else if ( m_mode == VisualLineMode ) {
    startCol = 0;
    endCol = m_view->doc()->lineLength( endLine );
  } else {
    if ( c.line() == endLine ) {
      startCol = m_start.column();
      endCol = c.column();
    } else {
      startCol = c.column();
      endCol = m_start.column();
    }
  }

  return KTextEditor::Range( startLine, startCol, endLine, endCol );
}

void KateViVisualMode::initializeCommands()
{
  m_commands.clear();
  m_motions.clear();
  m_commands.push_back( new KateViCommand( this, "J", &KateViNormalMode::commandJoinLines, IS_CHANGE ) );
  m_commands.push_back( new KateViCommand( this, "c", &KateViNormalMode::commandChange, IS_CHANGE ) );
  m_commands.push_back( new KateViCommand( this, "s", &KateViNormalMode::commandChange, IS_CHANGE ) );
  m_commands.push_back( new KateViCommand( this, "C", &KateViNormalMode::commandChangeToEOL, IS_CHANGE ) );
  m_commands.push_back( new KateViCommand( this, "d", &KateViNormalMode::commandDelete, IS_CHANGE ) );
  m_commands.push_back( new KateViCommand( this, "D", &KateViNormalMode::commandDeleteToEOL, IS_CHANGE ) );
  m_commands.push_back( new KateViCommand( this, "x", &KateViNormalMode::commandDeleteChar, IS_CHANGE ) );
  m_commands.push_back( new KateViCommand( this, "X", &KateViNormalMode::commandDeleteCharBackward, IS_CHANGE ) );
  m_commands.push_back( new KateViCommand( this, "gu", &KateViNormalMode::commandMakeLowercase, IS_CHANGE ) );
  m_commands.push_back( new KateViCommand( this, "gU", &KateViNormalMode::commandMakeUppercase, IS_CHANGE ) );
  m_commands.push_back( new KateViCommand( this, "y", &KateViNormalMode::commandYank ) );
  m_commands.push_back( new KateViCommand( this, "Y", &KateViNormalMode::commandYankToEOL ) );
  m_commands.push_back( new KateViCommand( this, "p", &KateViNormalMode::commandPaste, IS_CHANGE ) );
  m_commands.push_back( new KateViCommand( this, "P", &KateViNormalMode::commandPasteBefore, IS_CHANGE ) );
  m_commands.push_back( new KateViCommand( this, "r.", &KateViNormalMode::commandReplaceCharacter, IS_CHANGE | REGEX_PATTERN ) );
  m_commands.push_back( new KateViCommand( this, ":", &KateViNormalMode::commandSwitchToCmdLine ) );
  m_commands.push_back( new KateViCommand( this, "/", &KateViNormalMode::commandSearch ) );
  m_commands.push_back( new KateViCommand( this, "u", &KateViNormalMode::commandUndo ) );
  m_commands.push_back( new KateViCommand( this, "U", &KateViNormalMode::commandRedo ) );
  m_commands.push_back( new KateViCommand( this, "m.", &KateViNormalMode::commandSetMark, REGEX_PATTERN ) );
  m_commands.push_back( new KateViCommand( this, "n", &KateViNormalMode::commandFindNext ) );
  m_commands.push_back( new KateViCommand( this, "N", &KateViNormalMode::commandFindPrev ) );
  m_commands.push_back( new KateViCommand( this, ">", &KateViNormalMode::commandIndentLines ) );
  m_commands.push_back( new KateViCommand( this, "<", &KateViNormalMode::commandUnindentLines ) );
  m_commands.push_back( new KateViCommand( this, "<c-c>", &KateViNormalMode::commandAbort ) );
  m_commands.push_back( new KateViCommand( this, "<c-[>", &KateViNormalMode::commandAbort ) );
  m_commands.push_back( new KateViCommand( this, "ga", &KateViNormalMode::commandPrintCharacterCode, SHOULD_NOT_RESET ) );
  m_commands.push_back( new KateViCommand( this, "v", &KateViNormalMode::commandEnterVisualMode, SHOULD_NOT_RESET ) );
  m_commands.push_back( new KateViCommand( this, "V", &KateViNormalMode::commandEnterVisualLineMode, SHOULD_NOT_RESET ) );
  m_commands.push_back( new KateViCommand( this, "o", &KateViNormalMode::commandToOtherEnd, SHOULD_NOT_RESET ) );
  m_commands.push_back( new KateViCommand( this, "=", &KateViNormalMode::commandAlignLines, SHOULD_NOT_RESET ) );
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
  m_motions.push_back( new KateViMotion( this, "`.", &KateViNormalMode::motionToMark, REGEX_PATTERN ) );
  m_motions.push_back( new KateViMotion( this, "'.", &KateViNormalMode::motionToMarkLine, REGEX_PATTERN ) );
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
  m_motions.push_back( new KateViMotion( this, "i,", &KateViNormalMode::textObjectInnerComma ) );
  m_motions.push_back( new KateViMotion( this, "a,", &KateViNormalMode::textObjectAComma ) );
}
