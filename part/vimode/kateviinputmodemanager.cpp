/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2008 - 2009 Erlend Hamberg <ehamberg@gmail.com>
 *  Copyright (C) 2009 Paul Gideon Dann <pdgiddie@gmail.com>
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
}

KateViInputModeManager::~KateViInputModeManager()
{
  delete m_viNormalMode;
  delete m_viInsertMode;
  delete m_viVisualMode;
  delete m_viReplaceMode;
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
        } else {
          key = int(decoded.at(0).toUpper().toAscii());
          text = decoded.at(0);
          kDebug( 13070 ) << "###########" << key;
          kDebug( 13070 ) << "###########" << Qt::Key_W;
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
  QStringList names = config.readEntry( "ViRegisterNames", QStringList() );
  QStringList contents = config.readEntry( "ViRegisterContents", QStringList() );

  // sanity check
  if ( names.size() == contents.size() ) {
    for ( int i = 0; i < names.size(); i++ ) {
      KateGlobal::self()->viInputModeGlobal()->fillRegister( names.at( i ).at( 0 ), contents.at( i ) );
    }
  }
}

void KateViInputModeManager::writeSessionConfig( KConfigGroup& config )
{
  const QMap<QChar, QString>* regs = KateGlobal::self()->viInputModeGlobal()->getRegisters();
  QStringList names, contents;
  QMap<QChar, QString>::const_iterator i;
  for (i = regs->constBegin(); i != regs->constEnd(); ++i) {
    if ( i.value().length() <= 1000 ) {
      names << i.key();
      contents << i.value();
    } else {
      kDebug( 13070 ) << "Did not save contents of register " << i.key() << ": contents too long ("
        << i.value().length() << " characters)";
    }
  }

  config.writeEntry( "ViRegisterNames", names );
  config.writeEntry( "ViRegisterContents", contents );
}
