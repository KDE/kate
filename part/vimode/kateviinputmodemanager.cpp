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

#include "katevinormalmode.h"
#include "kateviinsertmode.h"
#include "katevivisualmode.h"

KateViInputModeManager::KateViInputModeManager(KateView* view, KateViewInternal* viewInternal)
{
  m_viNormalMode = new KateViNormalMode(this, view, viewInternal);
  m_viInsertMode = new KateViInsertMode(this, view, viewInternal);
  m_viVisualMode = new KateViVisualMode(this, view, viewInternal);

  m_currentViMode = NormalMode;

  m_view = view;
  m_viewInternal = viewInternal;
}

KateViInputModeManager::~KateViInputModeManager()
{
  delete m_viNormalMode;
  delete m_viInsertMode;
  delete m_viVisualMode;
}

bool KateViInputModeManager::handleKeypress(QKeyEvent *e)
{
  bool res;

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
  bool moveCursorRight = m_currentViMode == InsertMode && m_viewInternal->getCursor().column() > 0;

  changeViMode(NormalMode);

  if ( moveCursorRight ) {
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
