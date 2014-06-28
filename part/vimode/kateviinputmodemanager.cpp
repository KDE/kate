/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2008 - 2009 Erlend Hamberg <ehamberg@gmail.com>
 *  Copyright (C) 2009 Paul Gideon Dann <pdgiddie@gmail.com>
 *  Copyright (C) 2011 Svyatoslav Kuzmich <svatoslav1@gmail.com>
 *  Copyright (C) 2012 - 2013 Simon St James <kdedevel@etotheipiplusone.com>
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
#include <QApplication>

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
#include "katevikeymapper.h"
#include "kateviemulatedcommandbar.h"

using KTextEditor::Cursor;
using KTextEditor::Document;
using KTextEditor::Mark;
using KTextEditor::MarkInterface;
using KTextEditor::MovingCursor;

KateViInputModeManager::KateViInputModeManager(KateView* view, KateViewInternal* viewInternal)
{
  m_viNormalMode = new KateViNormalMode(this, view, viewInternal);
  m_viInsertMode = new KateViInsertMode(this, view, viewInternal);
  m_viVisualMode = new KateViVisualMode(this, view, viewInternal);
  m_viReplaceMode = new KateViReplaceMode(this, view, viewInternal);

  m_currentViMode = NormalMode;
  m_previousViMode = NormalMode;

  m_view = view;
  m_viewInternal = viewInternal;

  m_view->setCaretStyle( KateRenderer::Block, true );

  m_insideHandlingKeyPressCount = 0;

  m_isReplayingLastChange = false;

  m_isRecordingMacro = false;
  m_macrosBeingReplayedCount = 0;
  m_lastPlayedMacroRegister = QChar::Null;

  m_keyMapperStack.push(QSharedPointer<KateViKeyMapper>(new KateViKeyMapper(this, m_view->doc(), m_view)));

  m_lastSearchBackwards = false;
  m_lastSearchCaseSensitive = false;
  m_lastSearchPlacedCursorAtEndOfMatch = false;

  jump_list = new QList<KateViJump>;
  current_jump = jump_list->begin();
  m_temporaryNormalMode = false;

  m_markSetInsideViInputModeManager = false;

  connect(m_view->doc(),
           SIGNAL(markChanged(KTextEditor::Document*, KTextEditor::Mark,
                              KTextEditor::MarkInterface::MarkChangeAction)),
           this,
           SLOT(markChanged(KTextEditor::Document*, KTextEditor::Mark,
                            KTextEditor::MarkInterface::MarkChangeAction)));
  // We have to do this outside of KateViNormalMode, as we don't want
  // KateViVisualMode (which inherits from KateViNormalMode) to respond
  // to changes in the document as well.
  m_viNormalMode->beginMonitoringDocumentChanges();

  if (view->selection())
  {
    changeViMode(VisualMode);
    m_view->setCursorPosition(Cursor(view->selectionRange().end().line(), view->selectionRange().end().column() - 1));
    m_viVisualMode->updateSelection();
  }
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
  m_insideHandlingKeyPressCount++;
  bool res = false;
  bool keyIsPartOfMapping = false;
  const bool isSyntheticSearchCompletedKeyPress = m_view->viModeEmulatedCommandBar()->isSendingSyntheticSearchCompletedKeypress();

  // With macros, we want to record the keypresses *before* they are mapped, but if they end up *not* being part
  // of a mapping, we don't want to record them when they are played back by m_keyMapper, hence
  // the "!isPlayingBackRejectedKeys()". And obviously, since we're recording keys before they are mapped, we don't
  // want to also record the executed mapping, as when we replayed the macro, we'd get duplication!
  if (isRecordingMacro() && !isReplayingMacro() && !isSyntheticSearchCompletedKeyPress && !keyMapper()->isExecutingMapping() && !keyMapper()->isPlayingBackRejectedKeys())
  {
    QKeyEvent copy( e->type(), e->key(), e->modifiers(), e->text() );
    m_currentMacroKeyEventsLog.append(copy);
  }

  if (!isReplayingLastChange() && !isSyntheticSearchCompletedKeyPress)
  {
    if (e->key() == Qt::Key_AltGr) {
      return true; // do nothing:)
    }

    // Hand off to the key mapper, and decide if this key is part of a mapping.
    if (e->key() != Qt::Key_Control && e->key() != Qt::Key_Shift && e->key() != Qt::Key_Alt && e->key() != Qt::Key_Meta)
    {
      const QChar key = KateViKeyParser::self()->KeyEventToQChar(*e);
      if (keyMapper()->handleKeypress(key))
      {
        keyIsPartOfMapping = true;
        res = true;
      }
    }
  }

  if (!keyIsPartOfMapping)
  {
    if (!isReplayingLastChange() && !isSyntheticSearchCompletedKeyPress) {
      // record key press so that it can be repeated via "."
      QKeyEvent copy( e->type(), e->key(), e->modifiers(), e->text() );
      appendKeyEventToLog( copy );
    }

    if (m_view->viModeEmulatedCommandBar()->isActive())
    {
      res = m_view->viModeEmulatedCommandBar()->handleKeyPress(e);
    }
    else
    {
      res = getCurrentViModeHandler()->handleKeypress(e);
    }
  }

  m_insideHandlingKeyPressCount--;
  Q_ASSERT(m_insideHandlingKeyPressCount >= 0);

  return res;
}

void KateViInputModeManager::feedKeyPresses(const QString &keyPresses) const
{
  int key;
  Qt::KeyboardModifiers mods;
  QString text;

  foreach(const QChar &c, keyPresses) {
    QString decoded = KateViKeyParser::self()->decodeKeySequence(QString(c));
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
          || decoded.indexOf("m-") != -1 || decoded.indexOf("a-") != -1) {

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
          key = KateViKeyParser::self()->vi2qt(decoded);
        } else if (decoded.length() == 1) {
          key = int(decoded.at(0).toUpper().toAscii());
          text = decoded.at(0);
          kDebug( 13070 ) << "###########" << key;
        } else {
          kWarning( 13070 ) << "decoded is empty. skipping key press.";
        }
      } else { // no modifiers
        key = KateViKeyParser::self()->vi2qt(decoded);
      }
    } else {
      key = decoded.at(0).unicode();
      text = decoded.at(0);
    }

    // We have to be clever about which widget we dispatch to, as we can trigger
    // shortcuts if we're not careful (even if Vim mode is configured to steal shortcuts).
    QKeyEvent k(QEvent::KeyPress, key, mods, text);
    QWidget *destWidget = NULL;
    if (QApplication::activePopupWidget())
    {
      // According to the docs, the activePopupWidget, if present, takes all events.
      destWidget = QApplication::activePopupWidget();
    }
    else if (QApplication::focusWidget())
    {
      if (QApplication::focusWidget()->focusProxy())
      {
        destWidget = QApplication::focusWidget()->focusProxy();
      }
      else
      {
        destWidget = QApplication::focusWidget();
      }
    }
    else
    {
      destWidget = m_view->focusProxy();
    }
    QApplication::sendEvent(destWidget, &k);
  }
}

bool KateViInputModeManager::isHandlingKeypress() const
{
  return m_insideHandlingKeyPressCount > 0;
}

void KateViInputModeManager::appendKeyEventToLog(const QKeyEvent &e)
{
  if ( e.key() != Qt::Key_Shift && e.key() != Qt::Key_Control
      && e.key() != Qt::Key_Meta && e.key() != Qt::Key_Alt ) {
    m_currentChangeKeyEventsLog.append(e);
  }
}

void KateViInputModeManager::storeLastChangeCommand()
{
  m_lastChange.clear();

  QList<QKeyEvent> keyLog = m_currentChangeKeyEventsLog;

  for (int i = 0; i < keyLog.size(); i++) {
    int keyCode = keyLog.at(i).key();
    QString text = keyLog.at(i).text();
    int mods = keyLog.at(i).modifiers();
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
      keyPress.append( keyCode <= 0xFF ? QChar( keyCode ) : KateViKeyParser::self()->qt2vi( keyCode ) );
      keyPress.append( '>' );

      key = KateViKeyParser::self()->encodeKeySequence( keyPress ).at( 0 );
    }

    m_lastChange.append(key);
  }
  m_lastChangeCompletionsLog = m_currentChangeCompletionsLog;
}

void KateViInputModeManager::repeatLastChange()
{
  m_isReplayingLastChange = true;
  m_nextLoggedLastChangeComplexIndex = 0;
  feedKeyPresses(m_lastChange);
  m_isReplayingLastChange = false;
}

void KateViInputModeManager::startRecordingMacro(QChar macroRegister)
{
  Q_ASSERT(!m_isRecordingMacro);
  kDebug(13070) << "Recording macro: " << macroRegister;
  m_isRecordingMacro = true;
  m_recordingMacroRegister = macroRegister;
  KateGlobal::self()->viInputModeGlobal()->clearMacro(macroRegister);
  m_currentMacroKeyEventsLog.clear();
  m_currentMacroCompletionsLog.clear();
}

void KateViInputModeManager::finishRecordingMacro()
{
  Q_ASSERT(m_isRecordingMacro);
  m_isRecordingMacro = false;
  KateGlobal::self()->viInputModeGlobal()->storeMacro(m_recordingMacroRegister, m_currentMacroKeyEventsLog, m_currentMacroCompletionsLog);
}

bool KateViInputModeManager::isRecordingMacro()
{
  return m_isRecordingMacro;
}

void KateViInputModeManager::replayMacro(QChar macroRegister)
{
  if (macroRegister == '@')
  {
    macroRegister = m_lastPlayedMacroRegister;
  }
  m_lastPlayedMacroRegister = macroRegister;
  kDebug(13070) << "Replaying macro: " << macroRegister;
  const QString macroAsFeedableKeypresses = KateGlobal::self()->viInputModeGlobal()->getMacro(macroRegister);
  kDebug(13070) << "macroAsFeedableKeypresses:  " << macroAsFeedableKeypresses;

  m_macrosBeingReplayedCount++;
  m_nextLoggedMacroCompletionIndex.push(0);
  m_macroCompletionsToReplay.push(KateGlobal::self()->viInputModeGlobal()->getMacroCompletions(macroRegister));
  m_keyMapperStack.push(QSharedPointer<KateViKeyMapper>(new KateViKeyMapper(this, m_view->doc(), m_view)));
  feedKeyPresses(macroAsFeedableKeypresses);
  m_keyMapperStack.pop();
  m_macroCompletionsToReplay.pop();
  m_nextLoggedMacroCompletionIndex.pop();
  m_macrosBeingReplayedCount--;
  kDebug(13070) << "Finished replaying: " << macroRegister;
}

bool KateViInputModeManager::isReplayingMacro()
{
  return m_macrosBeingReplayedCount > 0;
}

void KateViInputModeManager::logCompletionEvent(const KateViInputModeManager::Completion& completion)
{
  // Ctrl-space is a special code that means: if you're replaying a macro, fetch and execute
  // the next logged completion.
  QKeyEvent ctrlSpace( QKeyEvent::KeyPress, Qt::Key_Space, Qt::ControlModifier, " ");
  if (isRecordingMacro())
  {
    m_currentMacroKeyEventsLog.append(ctrlSpace);
    m_currentMacroCompletionsLog.append(completion);
  }
  m_currentChangeKeyEventsLog.append(ctrlSpace);
  m_currentChangeCompletionsLog.append(completion);
}

KateViInputModeManager::Completion KateViInputModeManager::nextLoggedCompletion()
{
  Q_ASSERT(isReplayingLastChange() || isReplayingMacro());
  if (isReplayingLastChange())
  {
    if (m_nextLoggedLastChangeComplexIndex >= m_lastChangeCompletionsLog.length())
    {
      kDebug(13070) << "Something wrong here: requesting more completions for last change than we actually have.  Returning dummy.";
      return Completion("", false, Completion::PlainText);
    }
    return m_lastChangeCompletionsLog[m_nextLoggedLastChangeComplexIndex++];
  }
  else
  {
    if (m_nextLoggedMacroCompletionIndex.top() >= m_macroCompletionsToReplay.top().length())
    {
      kDebug(13070) << "Something wrong here: requesting more completions for macro than we actually have.  Returning dummy.";
      return Completion("", false, Completion::PlainText);
    }
    return m_macroCompletionsToReplay.top()[m_nextLoggedMacroCompletionIndex.top()++];
  }
}

void KateViInputModeManager::doNotLogCurrentKeypress()
{
  if (m_isRecordingMacro)
  {
    Q_ASSERT(!m_currentMacroKeyEventsLog.isEmpty());
    m_currentMacroKeyEventsLog.pop_back();
  }
  Q_ASSERT(!m_currentChangeKeyEventsLog.isEmpty());
  m_currentChangeKeyEventsLog.pop_back();
}

const QString KateViInputModeManager::getLastSearchPattern() const
{
  if (!KateViewConfig::global()->viInputModeEmulateCommandBar())
  {
    return m_view->searchPattern();
  }
  else
  {
    return m_lastSearchPattern;
  }
}

void KateViInputModeManager::setLastSearchPattern( const QString &p )
{
  if (!KateViewConfig::global()->viInputModeEmulateCommandBar())
  {
    // This actually triggers a search, so we definitely don't want it to be called
    // if we are handle searches ourselves in the Emulated Command Bar.
    m_view->setSearchPattern(p);
  }
  m_lastSearchPattern = p;
}

void KateViInputModeManager::changeViMode(ViMode newMode)
{
  m_previousViMode = m_currentViMode;
  m_currentViMode = newMode;
}

ViMode KateViInputModeManager::getCurrentViMode() const
{
  return m_currentViMode;
}

ViMode KateViInputModeManager::getPreviousViMode() const
{
  return m_previousViMode;
}

bool KateViInputModeManager::isAnyVisualMode() const
{
  return ((m_currentViMode == VisualMode) || (m_currentViMode == VisualLineMode) || (m_currentViMode == VisualBlockMode));
}

KateViModeBase* KateViInputModeManager::getCurrentViModeHandler() const
{
  switch(m_currentViMode) {
    case NormalMode:
      return m_viNormalMode;
    case InsertMode:
      return m_viInsertMode;
    case VisualMode:
    case VisualLineMode:
    case VisualBlockMode:
      return m_viVisualMode;
    case ReplaceMode:
      return m_viReplaceMode;
  }
  kDebug( 13070 ) << "WARNING: Unknown Vi mode.";
  return NULL;
}

void KateViInputModeManager::viEnterNormalMode()
{
  bool moveCursorLeft = (m_currentViMode == InsertMode || m_currentViMode == ReplaceMode)
    && m_viewInternal->getCursor().column() > 0;

  if ( !isReplayingLastChange() && m_currentViMode == InsertMode ) {
    // '^ is the insert mark and "^ is the insert register,
    // which holds the last inserted text
    Range r( m_view->cursorPosition(), getMarkPosition( '^' ) );

    if ( r.isValid() ) {
      QString insertedText = m_view->doc()->text( r );
      KateGlobal::self()->viInputModeGlobal()->fillRegister( '^', insertedText );
    }

    addMark( m_view->doc(), '^', Cursor( m_view->cursorPosition() ), false, false );
  }

  changeViMode(NormalMode);

  if ( moveCursorLeft ) {
    m_viewInternal->cursorPrevChar();
  }
  m_view->setCaretStyle( KateRenderer::Block, true );
  m_viewInternal->update ();
}

void KateViInputModeManager::viEnterInsertMode()
{
  changeViMode(InsertMode);
  addMark( m_view->doc(), '^', Cursor( m_view->cursorPosition() ), false, false );
  if (getTemporaryNormalMode())
  {
    // Ensure the key log contains a request to re-enter Insert mode, else the keystrokes made
    // after returning from temporary normal mode will be treated as commands!
    m_currentChangeKeyEventsLog.append(QKeyEvent(QEvent::KeyPress, QString("i")[0].unicode(), Qt::NoModifier, "i"));
  }
  m_view->setCaretStyle( KateRenderer::Line, true );
  setTemporaryNormalMode(false);
  m_viewInternal->update ();
}

void KateViInputModeManager::viEnterVisualMode( ViMode mode )
{
  changeViMode( mode );

  // If the selection is inclusive, the caret should be a block.
  // If the selection is exclusive, the caret should be a line.
  m_view->setCaretStyle( KateRenderer::Block, true );
  m_viewInternal->update();
  getViVisualMode()->setVisualModeType( mode );
  getViVisualMode()->init();
}

void KateViInputModeManager::viEnterReplaceMode()
{
  changeViMode(ReplaceMode);
  m_view->setCaretStyle( KateRenderer::Underline, true );
  m_viewInternal->update();
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
        if ( names.size() == contents.size() && contents.size() == flags.size() ) {
            for ( int i = 0; i < names.size(); i++ ) {
		if (!names.at(i).isEmpty())
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
      addMark(m_view->doc(),marks.at(i).at(0), Cursor(marks.at(i+1).toInt(),marks.at(i+2).toInt()));
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

void KateViInputModeManager::reset() {
    if ( m_viVisualMode)
        m_viVisualMode->reset();
}

void KateViInputModeManager::addJump(Cursor cursor) {
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

Cursor KateViInputModeManager::getNextJump(Cursor cursor ) {
   if (current_jump != jump_list->end()) {
       KateViJump jump;
       if (current_jump + 1 != jump_list->end())
           jump = *(++current_jump);
       else
           jump = *(current_jump);

       cursor = Cursor(jump.line, jump.column);
   }

   // DEBUG
   PrintJumpList();

   return cursor;
}

Cursor KateViInputModeManager::getPrevJump(Cursor cursor ) {
   if (current_jump == jump_list->end()) {
       addJump(cursor);
       current_jump--;
   }

   if (current_jump != jump_list->begin()) {
       KateViJump jump;
           jump = *(--current_jump);
       cursor = Cursor(jump.line, jump.column);
   }

   // DEBUG
   PrintJumpList();

   return cursor;
}

void KateViInputModeManager::PrintJumpList(){
   kDebug( 13070 ) << "Jump List";
   for (  QList<KateViJump>::iterator iter = jump_list->begin();
        iter != jump_list->end();
        iter++){
           if (iter == current_jump)
               kDebug( 13070 ) << (*iter).line << (*iter).column << "<< Current Jump";
           else
               kDebug( 13070 ) << (*iter).line << (*iter).column;
   }
   if (current_jump == jump_list->end())
       kDebug( 13070 ) << "    << Current Jump";

}

void KateViInputModeManager::addMark( KateDocument* doc, const QChar& mark, const Cursor& pos,
                                      const bool moveoninsert, const bool showmark )
{
  m_markSetInsideViInputModeManager = true;
  uint marktype = m_view->doc()->mark(pos.line());

  // delete old cursor if any
  if (MovingCursor *oldCursor = m_marks.value( mark )) {

    int  number_of_marks = 0;

    foreach (QChar c, m_marks.keys()) {
      if  ( m_marks.value(c)->line() ==  oldCursor->line() )
        number_of_marks++;
    }

    if (number_of_marks == 1 && pos.line() != oldCursor->line()) {
      m_view->doc()->removeMark( oldCursor->line(), MarkInterface::markType01 );
    }

    delete oldCursor;
  }

  MovingCursor::InsertBehavior behavior = moveoninsert ? MovingCursor::MoveOnInsert
                                                                    : MovingCursor::StayOnInsert;
  // create and remember new one
  m_marks.insert( mark, doc->newMovingCursor( pos, behavior ) );

  // Showing what mark we set:
  if ( showmark && mark != '>' && mark != '<' && mark != '[' && mark != '.' && mark != ']') {
    if( !marktype & MarkInterface::markType01 ) {
      m_view->doc()->addMark( pos.line(),
          MarkInterface::markType01 );
    }

    // only show message for active view
    if (m_view->doc()->activeView() == m_view) {
      m_viNormalMode->message(i18n ("Mark set: %1", mark));
    }
  }

  m_markSetInsideViInputModeManager = false;
}

Cursor KateViInputModeManager::getMarkPosition( const QChar& mark ) const
{
  if ( m_marks.contains(mark) ) {
    MovingCursor* c = m_marks.value( mark );
    return Cursor( c->line(), c->column() );
  } else {
    return Cursor::invalid();
  }
}


void KateViInputModeManager::markChanged (Document* doc,
                                          Mark mark,
                                          MarkInterface::MarkChangeAction action) {

  Q_UNUSED( doc )
  if (mark.type != MarkInterface::Bookmark || m_markSetInsideViInputModeManager)
  {
    return;
  }
  if (action == MarkInterface::MarkRemoved) {
      foreach (QChar markerChar, m_marks.keys()) {
          if  ( m_marks.value(markerChar)->line() == mark.line )
              m_marks.remove(markerChar);
      }
  } else if (action == MarkInterface::MarkAdded) {
    bool freeMarkerCharFound = false;
    for( char markerChar = 'a'; markerChar <= 'z'; markerChar++) {
      if (!m_marks.value(markerChar)) {
        addMark(m_view->doc(), markerChar, Cursor(mark.line, 0));
        freeMarkerCharFound = true;
        break;
      }
    }
    if (!freeMarkerCharFound)
      m_viNormalMode->error(i18n ("There are no more chars for the next bookmark."));
  }
}

void KateViInputModeManager::syncViMarksAndBookmarks() {
  const QHash<int, Mark*> &m = m_view->doc()->marks();

  //  Each bookmark should have a vi mark on the same line.
  for (QHash<int, Mark*>::const_iterator it = m.constBegin(); it != m.constEnd(); ++it)
  {
    if (it.value()->type & MarkInterface::markType01 ) {
      bool thereIsViMarkForThisLine = false;
      foreach( QChar markerChar, m_marks.keys() ) {
        if ( m_marks.value(markerChar)->line() == it.value()->line ){
          thereIsViMarkForThisLine = true;
          break;
        }
      }
      if ( !thereIsViMarkForThisLine ) {
        for( char markerChar = 'a'; markerChar <= 'z'; markerChar++ ) {
          if ( ! m_marks.value(markerChar) ) {
            addMark( m_view->doc(), markerChar, Cursor( it.value()->line, 0 ) );
            break;
          }
        }
      }
    }
  }

  // For specific vi mark line should be bookmarked.
  QList<QChar> marksToSync;

  foreach(QChar markerChar, m_marks.keys()) {
    if (QLatin1Char('a') <= markerChar && markerChar <= QLatin1Char('z')) {
      marksToSync << markerChar;
    }
  }

  foreach(QChar markerChar, marksToSync) {
    bool thereIsKateMarkForThisLine = false;
    for (QHash<int, Mark*>::const_iterator it = m.constBegin(); it != m.constEnd(); ++it) {
      if (it.value()->type & MarkInterface::markType01 ) {
        if ( m_marks.value(markerChar)->line() == it.value()->line ) {
          thereIsKateMarkForThisLine = true;
          break;
        }
        if ( !thereIsKateMarkForThisLine) {
          m_view->doc()->addMark( m_marks.value(markerChar)->line(), MarkInterface::markType01 );
        }
      }
    }
  }
}


// Returns a string of marks and columns.
QString KateViInputModeManager::getMarksOnTheLine(int line) {
  QString res = "";

  if ( m_view->viInputMode() ) {
    foreach (QChar markerChar, m_marks.keys()) {
      if  ( m_marks.value(markerChar)->line() == line )
        res += markerChar + ":" + QString::number(m_marks.value(markerChar)->column()) + " ";
    }
  }

  return res;
}


QString KateViInputModeManager::modeToString(ViMode mode)
{
  QString modeStr;
  switch (mode) {
    case InsertMode:
      modeStr = i18n("VI: INSERT MODE");
      break;
    case NormalMode:
      modeStr = i18n("VI: NORMAL MODE");
      break;
    case VisualMode:
      modeStr = i18n("VI: VISUAL");
      break;
    case VisualBlockMode:
      modeStr = i18n("VI: VISUAL BLOCK");
      break;
    case VisualLineMode:
      modeStr = i18n("VI: VISUAL LINE");
      break;
    case ReplaceMode:
      modeStr = i18n("VI: REPLACE");
      break;
  }

  return modeStr;
}

KateViKeyMapper* KateViInputModeManager::keyMapper()
{
  return m_keyMapperStack.top().data();
}

KateViInputModeManager::Completion::Completion(const QString& completedText, bool removeTail, CompletionType completionType)
    : m_completedText(completedText),
      m_removeTail(removeTail),
      m_completionType(completionType)
{
  if (m_completionType == FunctionWithArgs || m_completionType == FunctionWithoutArgs)
  {
    kDebug(13070) << "Completing a function while not removing tail currently unsupported; will remove tail instead";
    m_removeTail = true;
  }
}
QString KateViInputModeManager::Completion::completedText() const
{
  return m_completedText;
}
bool KateViInputModeManager::Completion::removeTail() const
{
  return m_removeTail;
}
KateViInputModeManager::Completion::CompletionType KateViInputModeManager::Completion::completionType() const
{
  return m_completionType;
}

