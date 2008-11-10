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
  m_topRange = m_doc->newSmartRange(m_doc->documentRange());
  static_cast<KateSmartRange*>(m_topRange)->setInternal();
  m_topRange->setInsertBehavior(KTextEditor::SmartRange::ExpandLeft | KTextEditor::SmartRange::ExpandRight);

  m_view->addInternalHighlight(m_topRange);

  m_visualLine = false;

  KTextEditor::Range r;
  highlightRange = m_doc->newSmartRange( r, m_topRange );
  attribute = KTextEditor::Attribute::Ptr(new KTextEditor::Attribute());
  attribute->setBackground( m_viewInternal->palette().highlight() );
  attribute->setForeground( m_viewInternal->palette().highlightedText() );
  highlightRange->setInsertBehavior(KTextEditor::SmartRange::DoNotExpand);

  initializeCommands();
}

KateViVisualMode::~KateViVisualMode()
{
}

void KateViVisualMode::highlight() const
{
  // FIXME: HACK to avoid highlight bug: remove highlighing and re-set it
  highlightRange->setAttribute(static_cast<KTextEditor::Attribute::Ptr>(0));
  highlightRange->setAttribute(attribute);

  KTextEditor::Cursor c1 = m_start;
  KTextEditor::Cursor c2 = m_view->cursorPosition();

  if ( m_visualLine ) {
      c1.setColumn( ( c1 < c2 ) ? 0 : getLine( m_start.line() ).length() );
      c2.setColumn( ( c1 < c2  ? getLine().length() : 0 ) );
  } else if ( c1 > c2 && c1.column() != 0 ) {
    c1.setColumn( c1.column()+1 );
  }

  highlightRange->setRange( KTextEditor::Range( c1, c2 ) );
}

void KateViVisualMode::goToPos( const KateViRange &r )
{
  KTextEditor::Cursor c = m_view->cursorPosition();

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

  if ( c.line() >= m_doc->lines() ) {
    c.setLine( m_doc->lines()-1 );
  }

  updateCursor( c );

  m_commandRange.startLine = m_start.line();
  m_commandRange.startColumn = m_start.column();
  m_commandRange.endLine = r.endLine;
  m_commandRange.endColumn = r.endColumn;

  highlight();
}

void KateViVisualMode::reset()
{
    // remove highlighting
    highlightRange->setAttribute(static_cast<KTextEditor::Attribute::Ptr>(0));

    m_awaitingMotionOrTextObject.push_back( 0 ); // search for text objects/motion from char 0

    m_visualLine = false;

    // only switch to normal mode if still in visual mode. commands like c, s, ...
    // can have switched to insert mode
    if ( m_viInputModeManager->getCurrentViMode() == VisualMode
        || m_viInputModeManager->getCurrentViMode() == VisualLineMode ) {
      m_viInputModeManager->viEnterNormalMode();
    }
}

void KateViVisualMode::init()
{
    m_start = m_view->cursorPosition();
    highlightRange->setRange( KTextEditor::Range( m_start, m_view->cursorPosition() ) );
    highlightRange->setAttribute(attribute);
    highlight();

    m_awaitingMotionOrTextObject.push_back( 0 ); // search for text objects/motion from char 0

    m_commandRange.startLine = m_commandRange.endLine = m_start.line();
    m_commandRange.startColumn = m_commandRange.endColumn = m_start.column();
}


void KateViVisualMode::setVisualLine( bool l )
{
  m_visualLine = l;
  highlight();
}

void KateViVisualMode::switchStartEnd()
{
  KTextEditor::Cursor c = m_start;
  m_start = m_view->cursorPosition();

  updateCursor( c );

  highlight();
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
  m_commands.push_back( new KateViCommand( this, "gq", &KateViNormalMode::commandFormatLines, IS_CHANGE ) );
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
  m_commands.push_back( new KateViCommand( this, "ga", &KateViNormalMode::commandPrintCharacterCode, SHOULD_NOT_RESET ) );
  m_commands.push_back( new KateViCommand( this, "v", &KateViNormalMode::commandEnterVisualMode, SHOULD_NOT_RESET ) );
  m_commands.push_back( new KateViCommand( this, "V", &KateViNormalMode::commandEnterVisualLineMode, SHOULD_NOT_RESET ) );
  m_commands.push_back( new KateViCommand( this, "o", &KateViNormalMode::commandToOtherEnd, SHOULD_NOT_RESET ) );
  m_commands.push_back( new KateViCommand( this, "=", &KateViNormalMode::commandAlignLines, SHOULD_NOT_RESET ) );

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
}
