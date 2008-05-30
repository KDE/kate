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

#include <ktexteditor/cursor.h>

KateViNormalMode::KateViNormalMode( KateView * view, KateViewInternal * viewInternal )
{
  m_view = view;
  m_viewInternal = viewInternal;

  initializeCommands();
  reset(); // initialise with start configuration
}

KateViNormalMode::~KateViNormalMode()
{
}

/**
 * parses a key stroke to check if it's a valid (part of) a command
 * @return true if a command was completed and executed, false otherwise
 */
bool KateViNormalMode::handleKeypress( QKeyEvent *e )
{
  m_matches.clear();
  int keyCode = e->key();
  QString text = e->text();

  // don't act on ctrl/alt/meta keypresses
  if ( text.isEmpty() ) {
    return false;
  }

  QChar key = text.at( 0 );
  kDebug( 13070 ) << key << "(" << keyCode << ")";


  //if ( m_findWaitingForChar ) {
  //  switch ( m_keys[ m_keys.size() -1 ].toAscii() ) {
  //  case 'f':
  //    commandFindChar();
  //    break;
  //  case 't':
  //    commandToChar( key );
  //    break;
  //  case 'F':
  //    commandFindCharBackwards( key );
  //    break;
  //  case 'T':
  //    commandToCharBackwards( key );
  //    break;
  //  default:
  //    kError( 13070 ) << "Error: m_findWaitingForChar should not be true, m_keys=" << m_keys;
  //  }
  //  reset();
  //  return true;
  //}

  // if keyCode is a number, append it to m_count
  if ( m_gettingCount && keyCode >= Qt::Key_0 && keyCode <= Qt::Key_9 ) {
    m_count *= 10;
    m_count += keyCode-Qt::Key_0;
    kDebug( 13070 ) << "count: " << m_count;

    return false;
  }

  if ( m_gettingCount ) {
    m_gettingCount = false;
  }

  m_keys.append( key );

  for ( int i = 0; i < m_commands.size(); i++ ) {
    if ( m_commands.at( i )->matches( m_keys ) )
      m_matches.push_back( i );
  }

  kDebug( 13070 ) << "'" << m_keys << "' MATCHES: " << m_matches.size();

  if ( m_matches.size() == 1 ) {
    if ( m_commands.at( m_matches.at( 0 ) )->matchesExact( m_keys ) ) {
      m_commands.at( m_matches.at( 0 ) )->execute();
      reset();
      return true;
    }
  } else if ( m_matches.size() == 0 ) {
    reset();
    return false;
  }

  // deal with simple one-key commands quick'n'easy
  //switch ( key.toAscii() ) {
  //case 'a':
  //  enterInsertModeAppend();
  //  break;
  //case 'A':
  //  enterInsertModeAppendEOL();
  //  break;
  //case 'f':
  //case 'F':
  //case 't':
  //case 'T':
  //  m_findWaitingForChar = true;
  //  m_keys.append( key );
  //  return false;
  //  break;
  //case 'h':
  //  commandCursorLeft();
  //  break;
  //case 'i':
  //  enterInsertMode();
  //  break;
  //case 'j':
  //  commandCursorDown();
  //  break;
  //case 'k':
  //  commandCursorUp();
  //  break;
  //case 'l':
  //  commandCursorRight();
  //  break;
  //default:
  //  reset();
  //  return false;
  //}

  return false;
}

/**
 * (re)set to start configuration. This is done when a command is completed
 * executed or when a command is aborted
 */
void KateViNormalMode::reset()
{
  kDebug( 1301 ) << "***RESET***";
  m_keys = "";
  m_count = 0;
  m_gettingCount = true;
  m_findWaitingForChar = false;
  m_matches.clear();
}

QString KateViNormalMode::getLine( int lineNumber )
{
  QString line;

  if ( lineNumber == -1 ) {
    KTextEditor::Cursor cursor ( m_view->cursorPositionVirtual() );
    line = m_view->doc()->line( cursor.line() );
  } else {
    line = m_view->doc()->line( lineNumber );
  }

  return line;
}

/**
 * enter insert mode at the cursor position
 */

void KateViNormalMode::enterInsertMode()
{
  m_view->changeViMode( InsertMode );
  m_viewInternal->repaint ();

  //m_view->m_editActions->addAssociatedWidget( m_viewInternal );

  emit m_view->viewModeChanged( m_view );
  //emit m_view->viewEditModeChanged( this,viewEditMode() );
}

/**
 * enter insert mode after the current character
 */

void KateViNormalMode::enterInsertModeAppend()
{
  m_view->changeViMode( InsertMode );
  m_viewInternal->cursorRight();
  m_viewInternal->repaint ();

  emit m_view->viewModeChanged( m_view );
  //emit viewEditModeChanged( this,viewEditMode() );
}

/**
 * start insert mode after the last character of the line
 */

void KateViNormalMode::enterInsertModeAppendEOL()
{
  m_view->changeViMode( InsertMode );
  m_viewInternal->end();
  //m_viewInternal->repaint ();

  emit m_view->viewModeChanged( m_view );
  //emit viewEditModeChanged( this,viewEditMode() );
}

void KateViNormalMode::commandCursorDown()
{
  m_viewInternal->cursorDown();
  //m_viewInternal->repaint();
}

void KateViNormalMode::commandCursorUp()
{
  m_viewInternal->cursorUp();
  //m_viewInternal->repaint();
}

void KateViNormalMode::commandCursorLeft()
{
  m_view->cursorLeft();
  //m_viewInternal->repaint();
}

void KateViNormalMode::commandCursorRight()
{
  m_view->cursorRight();
  //m_viewInternal->repaint();
}

bool KateViNormalMode::commandFindChar()
{
  KTextEditor::Cursor cursor ( m_view->cursorPositionVirtual() );
  QString line = getLine();
  int matchColumn = line.indexOf( m_keys.at( m_keys.size()-1 ), cursor.column() );

  kDebug( 13070 ) << "Looking for '" << m_keys.at( m_keys.size()-1 ) << "' from column " << cursor.column();

  if ( matchColumn >= 0 ) {
    cursor.setColumn( matchColumn );
    m_viewInternal->updateCursor( cursor );
    return true;
  }

  return false;
}

bool KateViNormalMode::commandFindCharBackwards()
{
  KTextEditor::Cursor cursor ( m_view->cursorPositionVirtual() );
  QString line = getLine();

  kDebug( 13070 ) << "Looking for '" << m_keys.at( m_keys.size()-1 ) << "' from column " << cursor.column() << " (backwards)";

  int matchColumn = -1;
  for ( int i = cursor.column()-1; i > 0; i-- ) {
    if ( line.at( i ) == m_keys.at( m_keys.size()-1 ) ) {
      matchColumn = i;
      break;
    }
  }

  if ( matchColumn >= 0 ) {
    cursor.setColumn( matchColumn );
    m_viewInternal->updateCursor( cursor );
    return true;
  }

  return false;
}

bool KateViNormalMode::commandToChar()
{
  KTextEditor::Cursor cursor ( m_view->cursorPositionVirtual() );
  QString line = getLine();
  int matchColumn = line.indexOf( m_keys.at( m_keys.size()-1 ), cursor.column() );
  
  if ( matchColumn >= 0 ) {
    cursor.setColumn( matchColumn-1 );
    m_viewInternal->updateCursor( cursor );
    return true;
  }

  return false;
}

bool KateViNormalMode::commandToCharBackwards()
{
  KTextEditor::Cursor cursor ( m_view->cursorPositionVirtual() );
  QString line = getLine();

  int matchColumn = -1;
  for ( int i = cursor.column()-1; i > 0; i-- ) {
    if ( line.at( i ) == m_keys.at( m_keys.size()-1 ) ) {
      matchColumn = i;
      break;
    }
  }

  if ( matchColumn >= 0 ) {
    cursor.setColumn( matchColumn+1 );
    m_viewInternal->updateCursor( cursor );
    return true;
  }

  return false;
}

void KateViNormalMode::initializeCommands()
{
  m_commands.push_back( new KateViNormalModeCommand( this, "f.", &KateViNormalMode::commandFindChar, true ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "F.", &KateViNormalMode::commandFindCharBackwards, true ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "t.", &KateViNormalMode::commandToChar, true ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "T.", &KateViNormalMode::commandToCharBackwards, true ) );
}
