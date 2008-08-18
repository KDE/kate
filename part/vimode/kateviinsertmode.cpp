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

#include "kateviinsertmode.h"
#include "katevinormalmode.h"
#include "kateview.h"
#include "kateviewinternal.h"
#include "katesmartrange.h"
#include "katecursor.h"

KateViInsertMode::KateViInsertMode( KateView * view, KateViewInternal * viewInternal )
{
  m_view = view;
  m_viewInternal = viewInternal;
}

KateViInsertMode::~KateViInsertMode()
{
}

QChar KateViInsertMode::getCharAtVirtualColumn( QString &line, int virtualColumn, int tabWidth ) const
{
  int column = 0;
  int tempCol = 0;

  while ( tempCol < virtualColumn ) {
    if ( line.at( column ) == '\t' ) {
      tempCol += tabWidth - ( tempCol % tabWidth );
    } else {
      tempCol++;
    }

    if ( tempCol <= virtualColumn ) {
      column++;

      if ( column >= line.length() ) {
        return false;
      }
    }
  }

  return line.at( column );
}

bool KateViInsertMode::commandInsertFromAbove()
{
  KTextEditor::Cursor c( m_view->cursorPosition() );

  if ( c.line() <= 0 ) {
    return false;
  }

  QString line = m_view->doc()->line( c.line()-1 );
  int tabWidth = m_view->doc()->config()->tabWidth();
  QChar ch = getCharAtVirtualColumn( line, m_view->virtualCursorColumn(), tabWidth );

  if ( ch == QChar::Null ) {
    return false;
  }

  return m_view->doc()->insertText( c, ch  );
}

bool KateViInsertMode::commandInsertFromBelow()
{
  KTextEditor::Cursor c( m_view->cursorPosition() );

  if ( c.line() >= m_view->doc()->lines()-1 ) {
    return false;
  }

  QString line = m_view->doc()->line( c.line()+1 );
  int tabWidth = m_view->doc()->config()->tabWidth();
  QChar ch = getCharAtVirtualColumn( line, m_view->virtualCursorColumn(), tabWidth );

  if ( ch == QChar::Null ) {
    return false;
  }

  return m_view->doc()->insertText( c, ch );
}

bool KateViInsertMode::commandDeleteWord()
{
    KTextEditor::Cursor c1( m_view->cursorPosition() );
    KTextEditor::Cursor c2;

    c2 = m_viewInternal->getViNormalMode()->findPrevWordStart( c1.line(), c1.column() );

    if ( c2.line() != c1.line() ) {
        if ( c1.column() == 0 ) {
            c2.setColumn( m_view->doc()->line( c2.line() ).length() );
        } else {
            c2.setColumn( 0 );
            c2.setLine( c2.line()+1 );
        }
    }

    KateViRange r( c2.line(), c2.column(), c1.line(), c1.column(), ViMotion::ExclusiveMotion );

    return m_viewInternal->getViNormalMode()->deleteRange( r, false, false );
}

bool KateViInsertMode::commandIndent()
{
  return m_viewInternal->getViNormalMode()->commandIndentLine();
}

bool KateViInsertMode::commandUnindent()
{
  return m_viewInternal->getViNormalMode()->commandUnindentLine();
}

/**
 * checks if the key is a valid command
 * @return true if a command was completed and executed, false otherwise
 */
bool KateViInsertMode::handleKeypress( const QKeyEvent *e )
{
    if ( e->key() == Qt::Key_Escape ) {
        m_view->viEnterNormalMode();
        return true;
    }

    if ( e->modifiers() == Qt::ControlModifier ) {
        switch( e->key() ) {
        case Qt::Key_BracketLeft:
        case Qt::Key_C:
            m_view->viEnterNormalMode();
            break;
        case Qt::Key_D:
            commandUnindent();
            break;
        case Qt::Key_E:
            commandInsertFromBelow();
            break;
        case Qt::Key_T:
            commandIndent();
            break;
        case Qt::Key_W:
            commandDeleteWord();
            break;
        case Qt::Key_Y:
            commandInsertFromAbove();
            break;
        default:
            return false;
        }

        return true;
    }

    return false;
}
