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

KateViNormalMode::KateViNormalMode( KateView * view, KateViewInternal * viewInternal )
{
  m_view = view;
  m_viewInternal = viewInternal;
  m_stickyColumn = -1;

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


  // if keyCode is a number, append it to m_count
  if ( m_gettingCount && keyCode >= Qt::Key_0 && keyCode <= Qt::Key_9 ) {
    // the first digit can't be 0 ('0' is a command)
    if ( m_count != 0 || keyCode != Qt::Key_0 ) {
      m_count *= 10;
      m_count += keyCode-Qt::Key_0;
      kDebug( 13070 ) << "count: " << m_count;

      return false;
    }
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

QString KateViNormalMode::getLine( int lineNumber ) const
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

void KateViNormalMode::goToStickyColumn( KTextEditor::Cursor &c )
{
  kDebug( 13070 ) << m_stickyColumn;

  // if sticky is set, check if we can co back to the sticky column in the new
  // line. if line is longer than previous line, but shorter than sticky, go
  // to the last column
  if ( m_stickyColumn != -1 ) {
    if ( getLine( c.line() ).length()-1 > m_stickyColumn )
      c.setColumn( m_stickyColumn );
    else if ( getLine( c.line() ).length()-1 > c.column() )
      c.setColumn( getLine( c.line() ).length()-1 );
  }

}

/**
 * enter insert mode at the cursor position
 */

bool KateViNormalMode::commandEnterInsertMode()
{
  m_view->changeViMode( InsertMode );
  m_viewInternal->repaint ();

  //m_view->m_editActions->addAssociatedWidget( m_viewInternal );

  emit m_view->viewModeChanged( m_view );
  //emit m_view->viewEditModeChanged( this,viewEditMode() );

  return true;
}

/**
 * enter insert mode after the current character
 */

bool KateViNormalMode::commandEnterInsertModeAppend()
{
  m_view->changeViMode( InsertMode );
  m_viewInternal->cursorRight();
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
  m_view->changeViMode( InsertMode );
  m_viewInternal->end();
  //m_viewInternal->repaint ();

  emit m_view->viewModeChanged( m_view );
  //emit viewEditModeChanged( this,viewEditMode() );

  return true;
}

bool KateViNormalMode::commandGoDownDisplayLine()
{
  m_viewInternal->cursorDown();
  //m_viewInternal->repaint();
}

bool KateViNormalMode::commandGoDown()
{
  KTextEditor::Cursor cursor ( m_view->cursorPosition() );

  // don't go below the last line
  if ( m_viewInternal->m_doc->numVisLines()-1 > cursor.line() ) {
    cursor.setLine( cursor.line()+1 );

    goToStickyColumn( cursor );

    // if the line below is shorter than the current cursor column, set sticky
    // to the old value and go to the last character of the previous line
    if ( getLine( cursor.line() ).length()-1 < cursor.column() ) {
      if ( m_stickyColumn == -1 )
        m_stickyColumn = cursor.column();
      cursor.setColumn( getLine( cursor.line() ).length()-1 );
    }

    m_viewInternal->updateCursor( cursor );
    return true;
  }

  return false;
  //m_viewInternal->repaint();
}

bool KateViNormalMode::commandGoUpDisplayLine()
{
  m_viewInternal->cursorUp();
  //m_viewInternal->repaint();
}

bool KateViNormalMode::commandGoUp()
{
  KTextEditor::Cursor cursor ( m_view->cursorPosition() );

  // don't go above the first line
  if ( cursor.line() > 0 ) {
    cursor.setLine( cursor.line()-1 );

    goToStickyColumn( cursor );

    // if the line above is shorter than the current cursor column, set sticky
    // to the old value and go to the last character of the previous line
    if ( getLine( cursor.line() ).length()-1 < cursor.column() ) {
      if ( m_stickyColumn == -1 )
        m_stickyColumn = cursor.column();
      cursor.setColumn( getLine( cursor.line() ).length()-1 );
    }

    m_viewInternal->updateCursor( cursor );
    return true;
  }

  return false;
  //m_viewInternal->repaint();
}

bool KateViNormalMode::commandGoLeft()
{
  KTextEditor::Cursor cursor ( m_view->cursorPositionVirtual() );

  if ( m_stickyColumn != -1 )
    m_stickyColumn = -1;

  if ( cursor.column() != 0 ) {
    m_view->cursorLeft();
    return true;
  }
  //m_viewInternal->repaint();
  return false;
}

bool KateViNormalMode::commandGoRight()
{
  KTextEditor::Cursor cursor ( m_view->cursorPositionVirtual() );

  if ( m_stickyColumn != -1 )
    m_stickyColumn = -1;

  if ( cursor.column() != getLine().length()-1 ) {
    m_view->cursorRight();
    return true;
  }
  //m_viewInternal->repaint();
  return false;
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

bool KateViNormalMode::commandFindCharBackward()
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

bool KateViNormalMode::commandToCharBackward()
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

bool KateViNormalMode::commandGoToEOL()
{
  KTextEditor::Cursor cursor ( m_view->cursorPositionVirtual() );

  cursor.setColumn( getLine().length()-1 );
  m_viewInternal->updateCursor( cursor );
  return true;
}

bool KateViNormalMode::commandGoToFirstCharacterOfLine()
{
  KTextEditor::Cursor cursor ( m_view->cursorPositionVirtual() );

  cursor.setColumn( 0 );
  m_viewInternal->updateCursor( cursor );
  return true;
}

bool KateViNormalMode::commandGoWordForward()
{
  return true;
}

bool KateViNormalMode::commandGoWordBackward()
{
  return true;
}

bool KateViNormalMode::commandGoWORDForward()
{
  return true;
}

bool KateViNormalMode::commandGoWORDBackward()
{
  return true;
}


void KateViNormalMode::initializeCommands()
{
  m_commands.push_back( new KateViNormalModeCommand( this, "f.", &KateViNormalMode::commandFindChar, true ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "F.", &KateViNormalMode::commandFindCharBackward, true ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "t.", &KateViNormalMode::commandToChar, true ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "T.", &KateViNormalMode::commandToCharBackward, true ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "h", &KateViNormalMode::commandGoLeft, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "j", &KateViNormalMode::commandGoDown, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "k", &KateViNormalMode::commandGoUp, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "l", &KateViNormalMode::commandGoRight, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "gj", &KateViNormalMode::commandGoDownDisplayLine, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "gk", &KateViNormalMode::commandGoUpDisplayLine, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "0", &KateViNormalMode::commandGoToFirstCharacterOfLine, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "$", &KateViNormalMode::commandGoToEOL, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "i", &KateViNormalMode::commandEnterInsertMode, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "a", &KateViNormalMode::commandEnterInsertModeAppend, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "A", &KateViNormalMode::commandEnterInsertModeAppendEOL, false ) );
}
