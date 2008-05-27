/* This file is part of the KDE libraries
   Copyright (C) 2008 Erlend Hamberg <ehamberg@gmail.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) version 3.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "katevicommandparser.h"

#include <kdebug.h>

KateViCommandParser:: KateViCommandParser( KateView * view )
: m_view( view )
{
  // initialise with start configuration
  reset( );
}

KateViCommandParser::~KateViCommandParser( )
{
}

/**
 * parses a key stroke to check if it's a valid (part of) a command
 * @return true if a command is completed, false otherwise
 */
bool KateViCommandParser::eatKey( QKeyEvent * e )
{
  int keyCode = e->key( );

  if ( m_findWaitingForChar ) {
    reset( );
    return true;
  }

  // if keyCode is a number, append a digit to m_count
  if ( m_gettingCount && keyCode >= Qt::Key_0 && keyCode <= Qt::Key_9 ) {
    m_count *= 10;
    m_count += keyCode-Qt::Key_0;
    kDebug( 13070 ) << "count: " << m_count;

    return false;
  }

  if ( m_gettingCount ) {
    m_gettingCount = false;
  }

  char key = ( char )( e->key( ) );
  if ( e->modifiers( ) != Qt::ShiftModifier ) {
    key += 0x20;
  }
  kDebug( 13070 ) << key;

  // deal with simple one-key commands quick'n'easy
  switch ( key ) {
  case 'a':
    m_view->viEnterInsertModeAppend( );
    break;
  case 'A':
    m_view->viEnterInsertModeAppendEOL( );
    break;
  case 'f':
  case 'F':
  case 't':
  case 'T':
    m_findWaitingForChar = true;
    m_keys.append( key );
    return false;
    break;
  case 'h':
    m_view->cursorLeft( );
    break;
  case 'i':
    m_view->viEnterInsertMode( );
    break;
  case 'j':
    m_view->viLineDown( );
    break;
  case 'k':
    m_view->viLineUp( );
    break;
  case 'l':
    m_view->cursorRight( );
    break;
  default:
    reset( );
    return false;
  }

  reset( );
  return true;
}

/**
 * (re)set to start configuration
 */
void KateViCommandParser::reset( )
{
  m_keys = "";
  m_count = 0;
  m_gettingCount = true;
  m_findWaitingForChar = false;
}
