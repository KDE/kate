/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2008 - 2009 Erlend Hamberg <ehamberg@gmail.com>
 *  Copyright (C) 2009 Paul Gideon Dann <pdgiddie@gmail.com>
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

#include "kateviinputmodemanager.h"

#include <QKeyEvent>
#include <QString>
#include <QCoreApplication>

#include <kconfig.h>
#include <kconfiggroup.h>

#include "kateconfig.h"
#include "kateglobal.h"
#include "kateviglobal.h"
#include "katevinormalmode.h"
#include "kateviinsertmode.h"
#include "katevivisualmode.h"
#include "katevireplacemode.h"
#include "katevikeyparser.h"

KateViInputModeManager::KateViInputModeManager(KateView* view, KateViewInternal* viewInternal)
{
  m_viNormalMode = new KateViNormalMode(this, view, viewInternal);
  m_viInsertMode = new KateViInsertMode(this, view, viewInternal);
  m_viVisualMode = new KateViVisualMode(this, view, viewInternal);
  m_viReplaceMode = new KateViReplaceMode(this, view, viewInternal);

  m_currentViMode = NormalMode;

  m_view = view;
  m_viewInternal = viewInternal;

  m_runningMacro = false;

  m_lastSearchBackwards = false;

  jump_list = new QList<KateViJump>;
  current_jump = jump_list->begin();
  m_temporaryNormalMode = false;

  m_mark_set_inside_viinputmodemanager = false;

  connect(m_view->doc(),
           SIGNAL(markChanged(KTextEditor::Document*, KTextEditor::Mark,
                              KTextEditor::MarkInterface::MarkChangeAction)),
           this,
           SLOT(markChanged(KTextEditor::Document*, KTextEditor::Mark,
                            KTextEditor::MarkInterface::MarkChangeAction)));
}

KateViInputModeManager::~KateViInputModeManager()
{
  delete m_viNormalMode;
  delete m_viInsertMode;
  delete m_viVisualMode;
  delete m_viReplaceMode;
  delete jump_list;
}

bool KateViInputModeManager::handleKeypress(const QKeyEvent *e)
{
  bool res;

  // record key press so that it can be repeated
  if (!isRunningMacro()) {
    QKeyEvent copy( e->type(), e->key(), e->modifiers(), e->text() );
    appendKeyEventToLog( copy );
  }

  // FIXME: I think we're making things difficult for ourselves here.  Maybe some
  //        more thought needs to go into the inheritance hierarchy.
  switch(m_currentViMode) {
  case NormalMode:
    res = m_viNormalMode->handleKeypress(e);
    break;
  case InsertMode:
    res = m_viInsertMode->handleKeypress(e);
    break;
  case VisualMode:
  case VisualLineMode:
  case VisualBlockMode:
    res = m_viVisualMode->handleKeypress(e);
    break;
  case ReplaceMode:
    res = m_viReplaceMode->handleKeypress(e);
    break;
  default:
    kDebug( 13070 ) << "WARNING: Unhandled keypress";
    res = false;
  }

  return res;
}

void KateViInputModeManager::feedKeyPresses(const QString &keyPresses) const
{
  int key;
  Qt::KeyboardModifiers mods;
  QString text;

  kDebug( 13070 ) << "Repeating change";
  foreach(const QChar &c, keyPresses) {
    QString decoded = KateViKeyParser::getInstance()->decodeKeySequence(QString(c));
    key = -1;
    mods = Qt::NoModifier;
    text.clear();

    kDebug( 13070 ) << "\t" << decoded;

    if (decoded.length() > 1 ) { // special key

      // remove the angle brackets
      decoded.remove(0, 1);
      decoded.remove(decoded.indexOf(">"), 1);
      kDebug( 13070 ) << "\t Special key:" << decoded;

      // check if one or more modifier keys where used
      if (decoded.indexOf("s-") != -1 || decoded.indexOf("c-") != -1
          || decoded.indexOf("m-") != -1 || decoded.indexOf("m-") != -1) {

        int s = decoded.indexOf("s-");
        if (s != -1) {
          mods |= Qt::ShiftModifier;
          decoded.remove(s, 2);
        }

        int c = decoded.indexOf("c-");
        if (c != -1) {
          mods |= Qt::ControlModifier;
          decoded.remove(c, 2);
        }

        int a = decoded.indexOf("a-");
        if (a != -1) {
          mods |= Qt::AltModifier;
          decoded.remove(a, 2);
        }

        int m = decoded.indexOf("m-");
        if (m != -1) {
          mods |= Qt::MetaModifier;
          decoded.remove(m, 2);
        }

        if (decoded.length() > 1 ) {
          key = KateViKeyParser::getInstance()->vi2qt(decoded);
        } else if (decoded.length() == 1) {
          key = int(decoded.at(0).toUpper().toAscii());
          text = decoded.at(0);
          kDebug( 13070 ) << "###########" << key;
        } else {
          kWarning( 13070 ) << "decoded is empty. skipping key press.";
        }
      } else { // no modifiers
        key = KateViKeyParser::getInstance()->vi2qt(decoded);
      }
    } else {
      key = decoded.at(0).unicode();
      text = decoded.at(0);
    }

    QKeyEvent k(QEvent::KeyPress, key, mods, text);

    QCoreApplication::sendEvent(m_viewInternal, &k);
  }
}

void KateViInputModeManager::appendKeyEventToLog(const QKeyEvent &e)
{
  if ( e.key() != Qt::Key_Shift && e.key() != Qt::Key_Control
      && e.key() != Qt::Key_Meta && e.key() != Qt::Key_Alt ) {
    m_keyEventsLog.append(e);
  }
}

void KateViInputModeManager::storeChangeCommand()
{
  m_lastChange.clear();

  for (int i = 0; i < m_keyEventsLog.size(); i++) {
    int keyCode = m_keyEventsLog.at(i).key();
    QString text = m_keyEventsLog.at(i).text();
    int mods = m_keyEventsLog.at(i).modifiers();
    QChar key;

   if ( text.length() > 0 ) {
     key = text.at(0);
   }

    if ( text.isEmpty() || ( text.length() ==1 && text.at(0) < 0x20 )
        || ( mods != Qt::NoModifier && mods != Qt::ShiftModifier ) ) {
      QString keyPress;

      keyPress.append( '<' );
      keyPress.append( ( mods & Qt::ShiftModifier ) ? "s-" : "" );
      keyPress.append( ( mods & Qt::ControlModifier ) ? "c-" : "" );
      keyPress.append( ( mods & Qt::AltModifier ) ? "a-" : "" );
      keyPress.append( ( mods & Qt::MetaModifier ) ? "m-" : "" );
      keyPress.append( keyCode <= 0xFF ? QChar( keyCode ) : KateViKeyParser::getInstance()->qt2vi( keyCode ) );
      keyPress.append( '>' );

      key = KateViKeyParser::getInstance()->encodeKeySequence( keyPress ).at( 0 );
    }

    m_lastChange.append(key);
  }
}

void KateViInputModeManager::repeatLastChange()
{
  m_runningMacro = true;
  feedKeyPresses(m_lastChange);
  m_runningMacro = false;
}

void KateViInputModeManager::changeViMode(ViMode newMode)
{
  m_currentViMode = newMode;
}

ViMode KateViInputModeManager::getCurrentViMode() const
{
  return m_currentViMode;
}

void KateViInputModeManager::viEnterNormalMode()
{
  bool moveCursorLeft = (m_currentViMode == InsertMode || m_currentViMode == ReplaceMode)
    && m_viewInternal->getCursor().column() > 0;

  changeViMode(NormalMode);

  if ( moveCursorLeft ) {
      m_viewInternal->cursorLeft();
  }
  m_viewInternal->repaint ();
}

void KateViInputModeManager::viEnterInsertMode()
{
  changeViMode(InsertMode);
  m_viewInternal->repaint ();
}

void KateViInputModeManager::viEnterVisualMode( ViMode mode )
{
  changeViMode( mode );

  m_viewInternal->repaint ();
  getViVisualMode()->setVisualModeType( mode );
  getViVisualMode()->init();
}

void KateViInputModeManager::viEnterReplaceMode()
{
  changeViMode(ReplaceMode);
  m_viewInternal->repaint ();
}


KateViNormalMode* KateViInputModeManager::getViNormalMode()
{
  return m_viNormalMode;
}

KateViInsertMode* KateViInputModeManager::getViInsertMode()
{
  return m_viInsertMode;
}

KateViVisualMode* KateViInputModeManager::getViVisualMode()
{
  return m_viVisualMode;
}

KateViReplaceMode* KateViInputModeManager::getViReplaceMode()
{
  return m_viReplaceMode;
}

const QString KateViInputModeManager::getVerbatimKeys() const
{
  QString cmd;

  switch (getCurrentViMode()) {
  case NormalMode:
    cmd = m_viNormalMode->getVerbatimKeys();
    break;
  case InsertMode:
  case ReplaceMode:
    // ...
    break;
  case VisualMode:
  case VisualLineMode:
  case VisualBlockMode:
    cmd = m_viVisualMode->getVerbatimKeys();
    break;
  }

  return cmd;
}

void KateViInputModeManager::readSessionConfig( const KConfigGroup& config )
{

    if ( KateGlobal::self()->viInputModeGlobal()->getRegisters()->size() > 0 ) {
        QStringList names = config.readEntry( "ViRegisterNames", QStringList() );
        QStringList contents = config.readEntry( "ViRegisterContents", QStringList() );
        QList<int> flags = config.readEntry( "ViRegisterFlags", QList<int>() );

        // sanity check
        if ( names.size() == contents.size() ) {
            for ( int i = 0; i < names.size(); i++ ) {
                KateGlobal::self()->viInputModeGlobal()->fillRegister( names.at( i ).at( 0 ), contents.at( i ), (OperationMode)( flags.at( i ) ) );
            }
        }
    }

  // Reading jump list
  // Format: jump1.line, jump1.column, jump2.line, jump2.column, jump3.line, ...
  jump_list->clear();
  QStringList jumps = config.readEntry( "JumpList", QStringList() );
  for (int i = 0; i + 1 < jumps.size(); i+=2) {
      KateViJump jump = {jumps.at(i).toInt(), jumps.at(i+1).toInt() } ;
      jump_list->push_back(jump);
  }
  current_jump = jump_list->end();
  PrintJumpList();

  // Reading marks
  QStringList marks = config.readEntry("ViMarks", QStringList() );
  for (int i = 0; i + 2 < marks.size(); i+=3) {
      addMark(m_view->doc(),marks.at(i).at(0), KTextEditor::Cursor(marks.at(i+1).toInt(),marks.at(i+2).toInt()));
  }
  syncViMarksAndBookmarks();
}

void KateViInputModeManager::writeSessionConfig( KConfigGroup& config )
{
  if ( KateGlobal::self()->viInputModeGlobal()->getRegisters()->size() > 0 ) {
    const QMap<QChar, KateViRegister>* regs = KateGlobal::self()->viInputModeGlobal()->getRegisters();
    QStringList names, contents;
    QList<int> flags;
    QMap<QChar, KateViRegister>::const_iterator i;
    for (i = regs->constBegin(); i != regs->constEnd(); ++i) {
      if ( i.value().first.length() <= 1000 ) {
        names << i.key();
        contents << i.value().first;
        flags << int(i.value().second);
      } else {
        kDebug( 13070 ) << "Did not save contents of register " << i.key() << ": contents too long ("
          << i.value().first.length() << " characters)";
      }
    }

    config.writeEntry( "ViRegisterNames", names );
    config.writeEntry( "ViRegisterContents", contents );
    config.writeEntry( "ViRegisterFlags", flags );
  }

  // Writing Jump List
  // Format: jump1.line, jump1.column, jump2.line, jump2.column, jump3.line, ...
  QStringList l;
  for (int i = 0; i < jump_list->size(); i++)
      l << QString::number(jump_list->at(i).line) << QString::number(jump_list->at(i).column);
  config.writeEntry("JumpList",l );

  // Writing marks;
  l.clear();
  foreach (QChar key, m_marks.keys()) {
      l << key << QString::number(m_marks.value( key )->line())
        << QString::number(m_marks.value( key )->column());
  }
  config.writeEntry("ViMarks",l);
}


void KateViInputModeManager::addJump(KTextEditor::Cursor cursor) {
   for (QList<KateViJump>::iterator iterator = jump_list->begin();
        iterator != jump_list->end();
        iterator ++){
       if ((*iterator).line == cursor.line()){
           jump_list->erase(iterator);
           break;
       }
   }

   KateViJump jump = { cursor.line(), cursor.column()};
   jump_list->push_back(jump);
   current_jump = jump_list->end();

   // DEBUG
   PrintJumpList();

}

KTextEditor::Cursor KateViInputModeManager::getNextJump(KTextEditor::Cursor cursor ) {
   if (current_jump != jump_list->end()) {
       KateViJump jump;
       if (current_jump + 1 != jump_list->end())
           jump = *(++current_jump);
       else
           jump = *(current_jump);

       cursor = KTextEditor::Cursor(jump.line, jump.column);
   }

   // DEBUG
   PrintJumpList();

   return cursor;
}

KTextEditor::Cursor KateViInputModeManager::getPrevJump(KTextEditor::Cursor cursor ) {
   if (current_jump == jump_list->end()) {
       addJump(cursor);
       current_jump--;
   }

   if (current_jump != jump_list->begin()) {
       KateViJump jump;
           jump = *(--current_jump);
       cursor = KTextEditor::Cursor(jump.line, jump.column);
   }

   // DEBUG
   PrintJumpList();

   return cursor;
}

void KateViInputModeManager::PrintJumpList(){
   qDebug() << "Jump List";
   for (  QList<KateViJump>::iterator iter = jump_list->begin();
        iter != jump_list->end();
        iter++){
           if (iter == current_jump)
               qDebug() << (*iter).line << (*iter).column << "<< Current Jump";
           else
               qDebug() << (*iter).line << (*iter).column;
   }
   if (current_jump == jump_list->end())
       qDebug() << "    << Current Jump";

}

void KateViInputModeManager::addMark( KateDocument* doc, const QChar& mark, const KTextEditor::Cursor& pos )
{
  m_mark_set_inside_viinputmodemanager = true;
  uint marktype = m_view->doc()->mark(pos.line());

  // delete old cursor if any
  if (KTextEditor::MovingCursor *oldCursor = m_marks.value( mark )) {

    int  number_of_marks = 0;

    foreach (QChar c, m_marks.keys()) {
      if  ( m_marks.value(c)->line() ==  oldCursor->line() )
        number_of_marks++;
    }

    if (number_of_marks == 1 && pos.line() != oldCursor->line()) {
      m_view->doc()->removeMark( oldCursor->line(), KTextEditor::MarkInterface::markType01 );
    }

    delete oldCursor;
  }

  // create and remember new one
  m_marks.insert( mark, doc->newMovingCursor( pos ) );

  if( !marktype & KTextEditor::MarkInterface::markType01 ) {
    m_view->doc()->addMark( pos.line(),
                           KTextEditor::MarkInterface::markType01 );
  }

  // Showing what mark we set:
  if (mark != '>' && mark != '<')
    m_viNormalMode->message("Mark set: " + mark);

  m_mark_set_inside_viinputmodemanager = false;
}

KTextEditor::Cursor KateViInputModeManager::getMarkPosition( const QChar& mark ) const
{
  if ( m_marks.contains(mark) ) {
    KTextEditor::MovingCursor* c = m_marks.value( mark );
    return KTextEditor::Cursor( c->line(), c->column() );
  } else {
    return KTextEditor::Cursor::invalid();
  }
}


void KateViInputModeManager::markChanged (KTextEditor::Document* doc,
                                          KTextEditor::Mark mark,
                                          KTextEditor::MarkInterface::MarkChangeAction action) {

    Q_UNUSED( doc )
  if (!m_mark_set_inside_viinputmodemanager) {
    if (action == 1) {
        foreach (QChar c, m_marks.keys()) {
            if  ( m_marks.value(c)->line() == mark.line )
                m_marks.remove(c);
        }
    } else if (action == 0) {
      bool char_exist = false;
      for( char c= 'a'; c <= 'z'; c++) {
        if (!m_marks.value(c) && (mark.type == KTextEditor::MarkInterface::Bookmark)) {
          addMark(m_view->doc(), c, KTextEditor::Cursor(mark.line,0));
          char_exist = true;
          break;
        }
      }
      if (!char_exist)
        m_viNormalMode->error("There no more chars for the next bookmark");
    }
  }
}

void KateViInputModeManager::syncViMarksAndBookmarks() {
  const QHash<int, KTextEditor::Mark*> &m = m_view->doc()->marks();

  //  Each bookmark should have a vi mark on the same line.
  for (QHash<int, KTextEditor::Mark*>::const_iterator it = m.constBegin(); it != m.constEnd(); ++it)
  {
    if (it.value()->type & KTextEditor::MarkInterface::markType01 ) {
      bool there_is_vi_mark_for_this_line = false;
      foreach( QChar key, m_marks.keys() ) {
        if ( m_marks.value(key)->line() == it.value()->line ){
          there_is_vi_mark_for_this_line = true;
          break;
        }
      }
      if ( ! there_is_vi_mark_for_this_line ) {
        for( char c= 'a'; c <= 'z'; c++ ) {
          if ( ! m_marks.value(c) ) {
            addMark( m_view->doc(), c, KTextEditor::Cursor( it.value()->line, 0 ) );
            break;
          }
        }
      }
    }
  }

  // For each vi mark line should be bookmarked.
  foreach( QChar key, m_marks.keys() ) {
    bool there_is_kate_mark_for_this_line = false;
    for (QHash<int, KTextEditor::Mark*>::const_iterator it = m.constBegin(); it != m.constEnd(); ++it) {
      if (it.value()->type & KTextEditor::MarkInterface::markType01 ) {
        if ( m_marks.value(key)->line() == it.value()->line ) {
          there_is_kate_mark_for_this_line = true;
          break;
        }
        if ( ! there_is_kate_mark_for_this_line) {
          m_view->doc()->addMark( m_marks.value(key)->line(), KTextEditor::MarkInterface::markType01 );
        }
      }
    }
  }
}


// Returns a string of marks and columns.
QString KateViInputModeManager::getMarksOnTheLine(int line) {
  QString res = "";

  if ( m_view->viInputMode() ) {
    foreach (QChar c, m_marks.keys()) {
      if  ( m_marks.value(c)->line() == line )
        res += c + ":" + QString::number(m_marks.value(c)->column()) + " ";
    }
  }

  return res;
}
