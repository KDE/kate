/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2008 Erlend Hamberg <ehamberg@gmail.com>
 *  Copyright (C) 2011 Svyatoslav Kuzmich <svatoslav1@gmail.com>
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

#include "kateglobal.h"
#include "kateviglobal.h"
#include "katevivisualmode.h"
#include "katevirange.h"

using KTextEditor::Cursor;
using KTextEditor::Range;
using KTextEditor::View;

#define ADDCMD(STR,FUNC, FLGS) m_commands.push_back( \
    new KateViCommand( this, STR, &KateViNormalMode::FUNC, FLGS ) );

#define ADDMOTION(STR, FUNC, FLGS) m_motions.push_back( new \
    KateViMotion( this, STR, &KateViNormalMode::FUNC, FLGS ) );

KateViVisualMode::KateViVisualMode( KateViInputModeManager* viInputModeManager, KateView *view, KateViewInternal *viewInternal )
  : KateViNormalMode( viInputModeManager, view, viewInternal )
{
  m_start.setPosition( -1, -1 );

  m_mode = VisualMode;

  m_selection_is_changed_inside_ViMode = false;

  initializeCommands();
  connect(m_view, SIGNAL(selectionChanged(KTextEditor::View*)), this, SLOT(updateSelection()));
}

KateViVisualMode::~KateViVisualMode()
{
}

void KateViVisualMode::SelectInclusive(Cursor c1, Cursor c2) {
  m_selection_is_changed_inside_ViMode = true;

  if ( c1 >= c2 )
    m_view->setSelection(Range(c1.line(), c1.column() + 1,
                               c2.line(), c2.column()));
  else
    m_view->setSelection(Range(c1.line(), c1.column(),
                               c2.line(), c2.column() + 1));

  m_selection_is_changed_inside_ViMode = false;
}

void KateViVisualMode::SelectBlockInclusive(Cursor c1, Cursor c2) {
  m_selection_is_changed_inside_ViMode = true;

  m_view->setBlockSelection(true);
  if ( c1.column() >= c2.column() )
    m_view->setSelection(Range(c1.line(), c1.column() + 1,
                               c2.line(), c2.column()));
  else
    m_view->setSelection(Range(c1.line(), c1.column(),
                               c2.line(), c2.column() + 1));

  m_selection_is_changed_inside_ViMode = false;
}



void KateViVisualMode::SelectLines(Range range) {
  m_selection_is_changed_inside_ViMode = true;

    int startline = qMin(range.start().line(),range.end().line());
    int endline   = qMax(range.start().line(),range.end().line());
    m_view->setSelection(Range(Cursor(startline,0),
                               Cursor(endline,m_view->doc()->lineLength(endline)+1)));

    m_selection_is_changed_inside_ViMode = false;
}

void KateViVisualMode::goToPos( const KateViRange &r )
{
  Cursor c = m_view->cursorPosition();

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

  // Setting range for a command
  m_commandRange.startLine = m_start.line();
  m_commandRange.startColumn = m_start.column();
  m_commandRange.endLine = c.line();
  m_commandRange.endColumn = c.column();

  // If visual mode is blockwise
  if (isVisualBlock()){
    SelectBlockInclusive(m_start,c);

    // Need to correct command range to make it inclusive.
    if (  (c.line() < m_start.line() && c.column() > m_start.column()) ||
          (c.line() > m_start.line() && c.column() < m_start.column()) )
      qSwap(m_commandRange.endColumn, m_commandRange.startColumn);

    return;
  } else {
    m_view->setBlockSelection(false);
  }

  // If visual mode is linewise
  if (isVisualLine()){
    SelectLines(Range(m_start,c));
    return;
  }

  // If visual mode is charwise
  SelectInclusive(m_start,c);
}

void KateViVisualMode::reset()
{
    m_mode = VisualMode;

    // only switch to normal mode if still in visual mode. commands like c, s, ...
    // can have switched to insert mode
    if ( m_viInputModeManager->getCurrentViMode() == VisualMode
        || m_viInputModeManager->getCurrentViMode() == VisualLineMode
        || m_viInputModeManager->getCurrentViMode() == VisualBlockMode ) {

      saveRangeMarks();
      m_lastVisualMode = m_viInputModeManager->getCurrentViMode();

      // Return the cursor back to start of selection after.
      if (!m_pendingResetIsDueToExit)
      {
        Cursor c = m_view->cursorPosition();
        if (m_start.line() != -1 && m_start.column() != -1) {
          if (m_viInputModeManager->getCurrentViMode() == VisualLineMode) {
            if (m_start.line() < c.line()) {
              updateCursor(Cursor(m_start.line(),0));
              m_stickyColumn = -1;
            }
          } else {
            updateCursor(qMin(m_start,c));
            m_stickyColumn = -1;
          }
        }
      }

      startNormalMode();
    }

    m_selection_is_changed_inside_ViMode = true;

    if (!m_commandShouldKeepSelection)
        m_view->removeSelection();
    else
        m_commandShouldKeepSelection = false;

    m_selection_is_changed_inside_ViMode = false;

    m_start.setPosition( -1, -1 );

    m_pendingResetIsDueToExit = false;
}

void KateViVisualMode::saveRangeMarks()
{
    // DO NOT save these marks if the
    // action that exited visual mode deleted the selection
  if (m_deleteCommand == false) {
    m_view->getViInputModeManager()->addMark( doc(), '<', m_start );
    m_view->getViInputModeManager()->addMark( doc(), '>', m_view->cursorPosition() );
  }
}

void KateViVisualMode::init()
{
    // when using "gv" we already have a start position
    if ( !m_start.isValid() ) {
      m_start = m_view->cursorPosition();
    }

    if (isVisualLine()){
      Cursor c = m_view->cursorPosition();
      m_selection_is_changed_inside_ViMode = true;
      SelectLines(Range(c,c));
      m_selection_is_changed_inside_ViMode = false;

    }

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
}

void KateViVisualMode::setVisualBlock( bool l )
{
  if ( l ) {
    m_mode = VisualBlockMode;
  } else {
    m_mode = VisualMode;
  }
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
}

void KateViVisualMode::goToPos(const Cursor& c)
{
  KateViRange r;
  r.endColumn = c.column();
  r.endLine = c.line();
  goToPos(r);
}

void KateViVisualMode::updateSelection( ) {
  if(!m_view->viInputMode() || m_selection_is_changed_inside_ViMode)
    return;

  // If we are there it's already not VisualBlock mode.
  m_view->setBlockSelection(false);

  Range r = m_view->selectionRange();

  // If not valid going back to normal mode
  if (!r.isValid()) {
    reset();
    return;
  }

  // If alredy not in visual mode, it's time to go there.
  if (m_viInputModeManager->getCurrentViMode() != VisualMode)
    commandEnterVisualMode();

  if (m_view->cursorPosition() == r.start()) {
    m_start = r.end();
  } else {
    m_start = r.start();
  }

  // Set range for commands
  m_commandRange.startLine = r.start().line();
  m_commandRange.startColumn = r.start().column();
  m_commandRange.endLine = r.end().line();
  m_commandRange.endColumn = r.end().column();
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
  ADDCMD("S", commandChangeToEOL, IS_CHANGE );
  ADDCMD("d", commandDelete, IS_CHANGE );
  ADDCMD("<del>", commandDelete, IS_CHANGE );
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
  ADDCMD("/", commandSearchForward, 0 );
  ADDCMD("?", commandSearchBackward, 0 );
  ADDCMD("m.", commandSetMark, REGEX_PATTERN | SHOULD_NOT_RESET );
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
  ADDCMD("I", commandPrependToBlock, IS_CHANGE );
  ADDCMD("A", commandAppendToBlock, IS_CHANGE );
  ADDCMD("gq", commandFormatLines, IS_CHANGE );

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
  ADDMOTION("*", motionToNextOccurrence, 0 );
  ADDMOTION("#", motionToPrevOccurrence, 0 );
  ADDMOTION("<c-f>", motionPageDown, 0 );
  ADDMOTION("<pagedown>", motionPageDown, 0 );
  ADDMOTION("<c-b>", motionPageUp, 0 );
  ADDMOTION("<pageup>", motionPageUp, 0 );
  ADDMOTION("{", motionToBeforeParagraph, 0 );
  ADDMOTION("}", motionToAfterParagraph, 0 );

  // text objects
  ADDMOTION("iw", textObjectInnerWord, 0 );
  ADDMOTION("aw", textObjectAWord, 0 );
  ADDMOTION("i\"", textObjectInnerQuoteDouble, 0 );
  ADDMOTION("a\"", textObjectAQuoteDouble, 0 );
  ADDMOTION("i'", textObjectInnerQuoteSingle, 0 );
  ADDMOTION("a'", textObjectAQuoteSingle, 0 );
  ADDMOTION("i[()b]", textObjectInnerParen, REGEX_PATTERN );
  ADDMOTION("a[()b]", textObjectAParen, REGEX_PATTERN );
  ADDMOTION("i[{}B]", textObjectInnerCurlyBracket, REGEX_PATTERN | IS_NOT_LINEWISE);
  ADDMOTION("a[{}B]", textObjectACurlyBracket, REGEX_PATTERN | IS_NOT_LINEWISE);
  ADDMOTION("i[\\[\\]]", textObjectInnerBracket, REGEX_PATTERN );
  ADDMOTION("a[\\[\\]]", textObjectABracket, REGEX_PATTERN );
  ADDMOTION("i,", textObjectInnerComma, 0 );
  ADDMOTION("a,", textObjectAComma, 0 );
}
