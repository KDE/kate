/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2008 Erlend Hamberg <ehamberg@gmail.com>
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

#include "katevivisualmode.h"
#include "katevirange.h"

using KTextEditor::Cursor;
using KTextEditor::Range;

#define ADDCMD(STR,FUNC, FLGS) m_commands.push_back( \
    new KateViCommand( this, STR, &KateViNormalMode::FUNC, FLGS ) );

#define ADDMOTION(STR, FUNC, FLGS) m_motions.push_back( new \
    KateViMotion( this, STR, &KateViNormalMode::FUNC, FLGS ) );

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
  Cursor c = m_view->cursorPosition();

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
  Cursor c = m_view->cursorPosition();
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
  Q_ASSERT( mode == VisualMode || mode == VisualLineMode || mode == VisualBlockMode );
  m_mode = mode;
}

void KateViVisualMode::switchStartEnd()
{
  Cursor c = m_start;
  m_start = m_view->cursorPosition();

  updateCursor( c );

  m_stickyColumn = -1;

  updateDirty();
}

Range KateViVisualMode::getVisualRange() const
{
  Cursor c = m_view->cursorPosition();

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

  return Range( startLine, startCol, endLine, endCol );
}

void KateViVisualMode::initializeCommands()
{
  // Remove the commands put in here by KateViNormalMode
  qDeleteAll(m_commands);
  m_commands.clear();

  // Remove the motions put in here by KateViNormalMode
  qDeleteAll(m_motions);
  m_motions.clear();

  ADDCMD("J", commandJoinLines, IS_CHANGE );
  ADDCMD("c", commandChange, IS_CHANGE );
  ADDCMD("s", commandChange, IS_CHANGE );
  ADDCMD("C", commandChangeToEOL, IS_CHANGE );
  ADDCMD("d", commandDelete, IS_CHANGE );
  ADDCMD("D", commandDeleteToEOL, IS_CHANGE );
  ADDCMD("x", commandDeleteChar, IS_CHANGE );
  ADDCMD("X", commandDeleteCharBackward, IS_CHANGE );
  ADDCMD("gu", commandMakeLowercase, IS_CHANGE );
  ADDCMD("u", commandMakeLowercase, IS_CHANGE );
  ADDCMD("gU", commandMakeUppercase, IS_CHANGE );
  ADDCMD("U", commandMakeUppercase, IS_CHANGE );
  ADDCMD("y", commandYank, 0 );
  ADDCMD("Y", commandYankToEOL, 0 );
  ADDCMD("p", commandPaste, IS_CHANGE );
  ADDCMD("P", commandPasteBefore, IS_CHANGE );
  ADDCMD("r.", commandReplaceCharacter, IS_CHANGE | REGEX_PATTERN );
  ADDCMD(":", commandSwitchToCmdLine, 0 );
  ADDCMD("/", commandSearch, 0 );
  ADDCMD("m.", commandSetMark, REGEX_PATTERN );
  ADDCMD(">", commandIndentLines, 0 );
  ADDCMD("<", commandUnindentLines, 0 );
  ADDCMD("<c-c>", commandAbort, 0 );
  ADDCMD("<c-[>", commandAbort, 0 );
  ADDCMD("ga", commandPrintCharacterCode, SHOULD_NOT_RESET );
  ADDCMD("v", commandEnterVisualMode, SHOULD_NOT_RESET );
  ADDCMD("V", commandEnterVisualLineMode, SHOULD_NOT_RESET );
  ADDCMD("o", commandToOtherEnd, SHOULD_NOT_RESET );
  ADDCMD("=", commandAlignLines, SHOULD_NOT_RESET );
  ADDCMD("~", commandChangeCase, IS_CHANGE );

  // regular motions
  ADDMOTION("h", motionLeft, 0 );
  ADDMOTION("<left>", motionLeft, 0 );
  ADDMOTION("<backspace>", motionLeft, 0 );
  ADDMOTION("j", motionDown, 0 );
  ADDMOTION("<down>", motionDown, 0 );
  ADDMOTION("k", motionUp, 0 );
  ADDMOTION("<up>", motionUp, 0 );
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
  ADDMOTION("`.", motionToMark, REGEX_PATTERN );
  ADDMOTION("'.", motionToMarkLine, REGEX_PATTERN );
  ADDMOTION("[[", motionToPreviousBraceBlockStart, 0 );
  ADDMOTION("]]", motionToNextBraceBlockStart, 0 );
  ADDMOTION("[]", motionToPreviousBraceBlockEnd, 0 );
  ADDMOTION("][", motionToNextBraceBlockEnd, 0 );
  ADDMOTION("<c-f>", motionPageDown, 0 );
  ADDMOTION("<pagedown>", motionPageDown, 0 );
  ADDMOTION("<c-b>", motionPageUp, 0 );
  ADDMOTION("<pageup>", motionPageUp, 0 );

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
