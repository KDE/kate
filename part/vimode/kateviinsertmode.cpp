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

#include "kateviinsertmode.h"
#include "kateviinputmodemanager.h"
#include "kateview.h"
#include "kateviewinternal.h"
#include "kateconfig.h"
#include "katecompletionwidget.h"

using KTextEditor::Cursor;

KateViInsertMode::KateViInsertMode( KateViInputModeManager *viInputModeManager,
    KateView * view, KateViewInternal * viewInternal ) : KateViModeBase()
{
  m_view = view;
  m_viewInternal = viewInternal;
  m_viInputModeManager = viInputModeManager;
}

KateViInsertMode::~KateViInsertMode()
{
}


bool KateViInsertMode::commandInsertFromAbove()
{
  Cursor c( m_view->cursorPosition() );

  if ( c.line() <= 0 ) {
    return false;
  }

  QString line = doc()->line( c.line()-1 );
  int tabWidth = doc()->config()->tabWidth();
  QChar ch = getCharAtVirtualColumn( line, m_view->virtualCursorColumn(), tabWidth );

  if ( ch == QChar::Null ) {
    return false;
  }

  return doc()->insertText( c, ch  );
}

bool KateViInsertMode::commandInsertFromBelow()
{
  Cursor c( m_view->cursorPosition() );

  if ( c.line() >= doc()->lines()-1 ) {
    return false;
  }

  QString line = doc()->line( c.line()+1 );
  int tabWidth = doc()->config()->tabWidth();
  QChar ch = getCharAtVirtualColumn( line, m_view->virtualCursorColumn(), tabWidth );

  if ( ch == QChar::Null ) {
    return false;
  }

  return doc()->insertText( c, ch );
}

bool KateViInsertMode::commandDeleteWord()
{
    Cursor c1( m_view->cursorPosition() );
    Cursor c2;

    c2 = findPrevWordStart( c1.line(), c1.column() );

    if ( c2.line() != c1.line() ) {
        if ( c1.column() == 0 ) {
            c2.setColumn( doc()->line( c2.line() ).length() );
        } else {
            c2.setColumn( 0 );
            c2.setLine( c2.line()+1 );
        }
    }

    KateViRange r( c2.line(), c2.column(), c1.line(), c1.column(), ViMotion::ExclusiveMotion );

    return deleteRange( r, false, false );
}

bool KateViInsertMode::commandIndent()
{
  //return getViNormalMode()->commandIndentLine();
  return false;
}

bool KateViInsertMode::commandUnindent()
{
  //return getViNormalMode()->commandUnindentLine();
  return false;
}

bool KateViInsertMode::commandToFirstCharacterInFile()
{
  Cursor c;

  c.setLine( 0 );
  c.setColumn( 0 );

  updateCursor( c );

  return true;
}

bool KateViInsertMode::commandToLastCharacterInFile()
{
  Cursor c;

  int lines = doc()->lines()-1;
  c.setLine( lines );
  c.setColumn( doc()->line( lines ).length() );

  updateCursor( c );

  return true;
}

bool KateViInsertMode::commandMoveOneWordLeft()
{
  Cursor c( m_view->cursorPosition() );
  c = findPrevWordStart( c.line(), c.column() );

  updateCursor( c );
  return true;
}

bool KateViInsertMode::commandMoveOneWordRight()
{
  Cursor c( m_view->cursorPosition() );
  c = findNextWordStart( c.line(), c.column() );

  updateCursor( c );
  return true;
}

bool KateViInsertMode::commandCompleteNext()
{
  if(m_view->completionWidget()->isCompletionActive()) {
    m_view->completionWidget()->cursorDown();
  } else {
    m_view->userInvokedCompletion();
  }
  return true;
}

bool KateViInsertMode::commandCompletePrevious()
{
  if(m_view->completionWidget()->isCompletionActive()) {
    m_view->completionWidget()->cursorUp();
  } else {
    m_view->userInvokedCompletion();
    m_view->completionWidget()->bottom();
  }
  return true;
}

/**
 * checks if the key is a valid command
 * @return true if a command was completed and executed, false otherwise
 */
bool KateViInsertMode::handleKeypress( const QKeyEvent *e )
{
  // backspace should work even if the shift key is down
  if (e->modifiers() != Qt::ControlModifier && e->key() == Qt::Key_Backspace ) {
    m_view->backspace();
    return true;
  }

  if ( e->modifiers() == Qt::NoModifier ) {
    switch ( e->key() ) {
    case Qt::Key_Escape:
      startNormalMode();
      return true;
      break;
    case Qt::Key_Left:
      m_view->cursorLeft();
      return true;
    case Qt::Key_Right:
      m_view->cursorRight();
      return true;
    case Qt::Key_Up:
      m_view->up();
      return true;
    case Qt::Key_Down:
      m_view->down();
      return true;
    case Qt::Key_Delete:
      m_view->keyDelete();
      return true;
    case Qt::Key_Home:
      m_view->home();
      return true;
    case Qt::Key_End:
      m_view->end();
      return true;
    case Qt::Key_PageUp:
      m_view->pageUp();
      return true;
    case Qt::Key_PageDown:
      m_view->pageDown();
      return true;
    default:
      return false;
      break;
    }
  } else if ( e->modifiers() == Qt::ControlModifier ) {
    switch( e->key() ) {
    case Qt::Key_BracketLeft:
    case Qt::Key_3:
    case Qt::Key_C:
      startNormalMode();
      return true;
      break;
    case Qt::Key_D:
      commandUnindent();
      return true;
      break;
    case Qt::Key_E:
      commandInsertFromBelow();
      return true;
      break;
    case Qt::Key_N:
      commandCompleteNext();
      return true;
      break;
    case Qt::Key_P:
      commandCompletePrevious();
      return true;
      break;
    case Qt::Key_T:
      commandIndent();
      return true;
      break;
    case Qt::Key_W:
      commandDeleteWord();
      return true;
      break;
    case Qt::Key_Y:
      commandInsertFromAbove();
      return true;
      break;
    case Qt::Key_Home:
      commandToFirstCharacterInFile();
      return true;
      break;
    case Qt::Key_End:
      commandToLastCharacterInFile();
      return true;
      break;
    case Qt::Key_Left:
      commandMoveOneWordLeft();
      return true;
      break;
    case Qt::Key_Right:
      commandMoveOneWordRight();
      return true;
      break;
    default:
      return false;
    }
  }

  return false;
}
