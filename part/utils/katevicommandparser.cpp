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

  // if keyCode is a number
  if ( m_gettingCount && keyCode >= 0x30 && keyCode <= 0x39 ) {
    m_count *= 10;
    m_count += keyCode-0x30;
    kDebug( 13070 ) << "count: " << m_count;

    return false;
  }

  m_gettingCount = false;

  switch ( keyCode ) {
  case Qt::Key_A:
    if ( e->modifiers( ) == Qt::ShiftModifier ) {
      m_view->viEnterInsertModeAppendEOL( );
    } else {
      m_view->viEnterInsertModeAppend( );
    }
    break;
  case Qt::Key_H:
    if ( e->modifiers( ) == Qt::ShiftModifier ) {
      ;// TODO
    } else {
      m_view->cursorLeft( );
    }
    break;
  case Qt::Key_I:
    if ( e->modifiers( ) == Qt::ShiftModifier ) {
      ;// TODO
    } else {
      m_view->viEnterInsertMode( );
    }
    break;
  case Qt::Key_J:
    if ( e->modifiers( ) == Qt::ShiftModifier ) {
      ;// TODO
    } else {
      m_view->viLineDown( );
    }
    break;
  case Qt::Key_K:
    if ( e->modifiers( ) == Qt::ShiftModifier ) {
      ;// TODO
    } else {
      m_view->viLineUp( );
    }
    break;
  case Qt::Key_L:
    if ( e->modifiers( ) == Qt::ShiftModifier ) {
      ;// TODO
    } else {
      m_view->cursorRight( );
    }
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
  m_count = 0;
  m_gettingCount = true;
}
