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

#include "kateviinputmodemanager.h"

#include <QKeyEvent>
#include <QString>
#include <QCoreApplication>

#include "katevinormalmode.h"
#include "kateviinsertmode.h"
#include "katevivisualmode.h"
#include "katevikeysequenceparser.h"

KateViInputModeManager::KateViInputModeManager(KateView* view, KateViewInternal* viewInternal)
{
  m_viNormalMode = new KateViNormalMode(this, view, viewInternal);
  m_viInsertMode = new KateViInsertMode(this, view, viewInternal);
  m_viVisualMode = new KateViVisualMode(this, view, viewInternal);

  m_currentViMode = NormalMode;

  m_view = view;
  m_viewInternal = viewInternal;
  m_keyParser = new KateViKeySequenceParser();

  m_runningMacro = false;
}

KateViInputModeManager::~KateViInputModeManager()
{
  delete m_viNormalMode;
  delete m_viInsertMode;
  delete m_viVisualMode;
}

bool KateViInputModeManager::handleKeypress(const QKeyEvent *e)
{
  bool res;

  // record key press so that it can be repeated
  if (!isRunningMacro()) {
    QKeyEvent copy( e->type(), e->key(), e->modifiers(), e->text() );
    appendKeyEventToLog( copy );
  }

  switch(m_currentViMode) {
  case NormalMode:
    res = m_viNormalMode->handleKeypress(e);
    break;
  case InsertMode:
    res = m_viInsertMode->handleKeypress(e);
    break;
  case VisualMode:
  case VisualLineMode:
    res = m_viVisualMode->handleKeypress(e);
    break;
  default:
    res = false;
  }

  return res;
}

void KateViInputModeManager::feedKeyPresses(const QString &keyPresses) const
{
  QChar c;
  int key;
  Qt::KeyboardModifiers mods;
  QString text;

  kDebug( 13070 ) << "Repeating change";
  foreach(c, keyPresses) {
    QString decoded = m_keyParser->decodeKeySequence(QString(c));
    key = -1;
    mods = Qt::NoModifier;
    text = QString();

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
          key = m_keyParser->vi2qt(decoded);
        } else {
          key = int(decoded.at(0).toUpper().toAscii());
          text = decoded.at(0);
          kDebug( 13070 ) << "###########" << key;
          kDebug( 13070 ) << "###########" << Qt::Key_W;
        }
      } else { // no modifiers
        key = m_keyParser->vi2qt(decoded);
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
      keyPress.append( keyCode <= 0xFF ? QChar( keyCode ) : m_keyParser->qt2vi( keyCode ) );
      keyPress.append( '>' );

      key = m_keyParser->encodeKeySequence( keyPress ).at( 0 );
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
  bool moveCursorLeft = m_currentViMode == InsertMode && m_viewInternal->getCursor().column() > 0;

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

void KateViInputModeManager::viEnterVisualMode( bool visualLine )
{
  if ( !visualLine ) {
    changeViMode(VisualMode);
  } else {
    changeViMode(VisualLineMode);
  }

  m_viewInternal->repaint ();
  getViVisualMode()->setVisualLine( visualLine );
  getViVisualMode()->init();
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

const QString KateViInputModeManager::getVerbatimKeys() const
{
  QString cmd;

  switch (getCurrentViMode()) {
  case NormalMode:
    cmd = m_viNormalMode->getVerbatimKeys();
    break;
  case InsertMode:
    // ...
    break;
  case VisualMode:
  case VisualLineMode:
    cmd = m_viVisualMode->getVerbatimKeys();
    break;
  }

  return cmd;
}
