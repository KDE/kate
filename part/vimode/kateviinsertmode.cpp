/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2008-2011 Erlend Hamberg <ehamberg@gmail.com>
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

#include "kateviinsertmode.h"
#include "kateviinputmodemanager.h"
#include "kateview.h"
#include "kateviewinternal.h"
#include "kateconfig.h"
#include "katecompletionwidget.h"
#include "kateglobal.h"
#include "katevikeyparser.h"

using KTextEditor::Cursor;

KateViInsertMode::KateViInsertMode( KateViInputModeManager *viInputModeManager,
    KateView * view, KateViewInternal * viewInternal ) : KateViModeBase()
{
  m_view = view;
  m_viewInternal = viewInternal;
  m_viInputModeManager = viInputModeManager;

  m_blockInsert = None;
  m_eolPos = 0;
  m_count = 1;
  m_countedRepeatsBeginOnNewLine = false;
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

    return deleteRange( r, CharWise, false );
}

bool KateViInsertMode::commandDeleteCharBackward()
{
    kDebug( 13070 ) << "Char backward!\n";
    Cursor c( m_view->cursorPosition() );

    KateViRange r( c.line(), c.column()-getCount(), c.line(), c.column(), ViMotion::ExclusiveMotion );

    if (c.column() == 0) {
        if (c.line() == 0) {
            return true;
        } else {
            r.startColumn = doc()->line(c.line()-1).length();
            r.startLine--;
      }
    }

    return deleteRange( r, CharWise );
}

bool KateViInsertMode::commandNewLine()
{
    doc()->newLine( m_view );
    return true;
}

bool KateViInsertMode::commandIndent()
{
    Cursor c( m_view->cursorPosition() );
    doc()->indent( KTextEditor::Range( c.line(), 0, c.line(), 0), 1 );
    return true;
}

bool KateViInsertMode::commandUnindent()
{
    Cursor c( m_view->cursorPosition() );
    doc()->indent( KTextEditor::Range( c.line(), 0, c.line(), 0), -1 );
    return true;
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
  // Completion may yield different results when performed
  // other places in the document. Therefore, repeat the
  // inserted text instead of the typed keystrokes.
  m_viInputModeManager->setTextualRepeat();
  if(m_view->completionWidget()->isCompletionActive()) {
    m_view->completionWidget()->cursorDown();
  } else {
    m_view->userInvokedCompletion();
  }
  return true;
}

bool KateViInsertMode::commandCompletePrevious()
{
  m_viInputModeManager->setTextualRepeat();
  if(m_view->completionWidget()->isCompletionActive()) {
    m_view->completionWidget()->cursorUp();
  } else {
    m_view->userInvokedCompletion();
    m_view->completionWidget()->bottom();
  }
  return true;
}

bool KateViInsertMode::commandInsertContentOfRegister(){
    Cursor c( m_view->cursorPosition() );
    Cursor cAfter = c;
    QChar reg = getChosenRegister( m_register );

    OperationMode m = getRegisterFlag( reg );
    QString textToInsert = getRegisterContent( reg );

    if ( textToInsert.isNull() ) {
      error(i18n("Nothing in register %1", reg ));
      return false;
    }

    if ( m == LineWise ) {
      textToInsert.chop( 1 ); // remove the last \n
      c.setColumn( doc()->lineLength( c.line() ) ); // paste after the current line and ...
      textToInsert.prepend( QChar( '\n' ) ); // ... prepend a \n, so the text starts on a new line

      cAfter.setLine( cAfter.line()+1 );
      cAfter.setColumn( 0 );
    }

    doc()->insertText( c, textToInsert, m == Block );

    updateCursor( cAfter );

    return true;
}

// Start Normal mode just for one command and return to Insert mode
bool KateViInsertMode::commandSwitchToNormalModeForJustOneCommand(){
    m_viInputModeManager->setTemporaryNormalMode(true);
    m_viInputModeManager->changeViMode(NormalMode);
    const Cursor cursorPos = m_view->cursorPosition();
    // If we're at end of the line, move the cursor back one step, as in Vim.
    if (doc()->line(cursorPos.line()).length() == cursorPos.column())
    {
      m_view->setCursorPosition(Cursor(cursorPos.line(), cursorPos.column() - 1));
    }
    m_view->setCaretStyle( KateRenderer::Block, true );
    m_view->updateViModeBarMode();
    m_viewInternal->repaint();
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

  if(m_keys.isEmpty()){
  if ( e->modifiers() == Qt::NoModifier ) {
    switch ( e->key() ) {
    case Qt::Key_Escape:
      leaveInsertMode();
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
      leaveInsertMode();
      return true;
      break;
    case Qt::Key_Space:
      commandCompleteNext();
      return true;
      break;
    case Qt::Key_C:
      leaveInsertMode( true );
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
    case Qt::Key_J:
      commandNewLine();
      return true;
      break;
    case Qt::Key_H:
      commandDeleteCharBackward();
      return true;
      break;
    case Qt::Key_Y:
      commandInsertFromAbove();
      return true;
      break;
    case Qt::Key_O:
      commandSwitchToNormalModeForJustOneCommand();
      return true;
      break;
    case Qt::Key_Home:
      commandToFirstCharacterInFile();
      return true;
      break;
    case Qt::Key_R:
      m_keys = "cR";
      // Waiting for register
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
} else {

    // Was waiting for register for Ctrl-R
    if (m_keys == "cR"){
        QChar key = KateViKeyParser::self()->KeyEventToQChar(
                    e->key(),
                    e->text(),
                    e->modifiers(),
                    e->nativeScanCode() );
        key = key.toLower();

        // is it register ?
        if ( ( key >= '0' && key <= '9' ) || ( key >= 'a' && key <= 'z' ) ||
                key == '_' || key == '+' || key == '*' ) {
          m_register = key;
        } else {
          m_keys = "";
          return false;
        }
        commandInsertContentOfRegister();
        m_keys = "";
        return true;
    }
  }
  return false;
}

// leave insert mode when esc, etc, is pressed. if leaving block
// prepend/append, the inserted text will be added to all block lines. if
// ctrl-c is used to exit insert mode this is not done.
void KateViInsertMode::leaveInsertMode( bool force )
{
    if ( !force )
    {
      if ( m_blockInsert != None ) { // block append/prepend

          // make sure cursor haven't been moved
          if ( m_blockRange.startLine == m_view->cursorPosition().line() ) {
              int start, len;
              QString added;
              Cursor c;

              switch ( m_blockInsert ) {
              case Append:
              case Prepend:
                  if ( m_blockInsert == Append ) {
                      start = m_blockRange.endColumn+1;
                  } else {
                      start = m_blockRange.startColumn;
                  }

                  len = m_view->cursorPosition().column()-start;
                  added = getLine().mid( start, len );

                  c = Cursor( m_blockRange.startLine, start );
                  for ( int i = m_blockRange.startLine+1; i <= m_blockRange.endLine; i++ ) {
                      c.setLine( i );
                      doc()->insertText( c, added );
                  }
                  break;
              case AppendEOL:
                  start = m_eolPos;
                  len = m_view->cursorPosition().column()-start;
                  added = getLine().mid( start, len );

                  c = Cursor( m_blockRange.startLine, start );
                  for ( int i = m_blockRange.startLine+1; i <= m_blockRange.endLine; i++ ) {
                      c.setLine( i );
                      c.setColumn( doc()->lineLength( i ) );
                      doc()->insertText( c, added );
                  }
                  break;
              default:
                  error("not supported");
              }
          }

          m_blockInsert = None;
      }
      else
      {
          const QString added = doc()->text(Range(m_viInputModeManager->getMarkPosition('^'), m_view->cursorPosition()));

          if (m_count > 1)
          {
              for (unsigned int i = 0; i < m_count - 1; i++)
              {
                  if (m_countedRepeatsBeginOnNewLine)
                  {
                      doc()->newLine(m_view);
                  }
                  doc()->insertText( m_view->cursorPosition(), added );
              }
          }
          m_countedRepeatsBeginOnNewLine = false;
      }
    }
    startNormalMode();
}

void KateViInsertMode::setBlockPrependMode( KateViRange blockRange )
{
    // ignore if not more than one line is selected
    if ( blockRange.startLine != blockRange.endLine ) {
        m_blockInsert = Prepend;
        m_blockRange = blockRange;
    }
}

void KateViInsertMode::setBlockAppendMode( KateViRange blockRange, BlockInsert b )
{
    Q_ASSERT( b == Append || b == AppendEOL );

    // ignore if not more than one line is selected
    if ( blockRange.startLine != blockRange.endLine ) {
        m_blockRange = blockRange;
        m_blockInsert = b;
        if ( b == AppendEOL ) {
          m_eolPos = doc()->lineLength( m_blockRange.startLine );
        }
    } else {
        kDebug( 13070 ) << "cursor moved. ignoring block append/prepend";
    }
}
