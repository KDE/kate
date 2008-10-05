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

#ifndef KATE_VI_INPUT_MODE_MANAGER_INCLUDED
#define KATE_VI_INPUT_MODE_MANAGER_INCLUDED

#include <QList>

class KateView;
class KateViewInternal;
class KateViNormalMode;
class KateViInsertMode;
class KateViVisualMode;
class QKeyEvent;
class QString;

/**
 * The four vi modes supported by Kate's vi input mode
 */
enum ViMode {
  NormalMode,
  InsertMode,
  VisualMode,
  VisualLineMode
};

class KateViInputModeManager
{
  public:
    KateViInputModeManager(KateView* view, KateViewInternal* viewInternal);
    ~KateViInputModeManager();

  /**
   * feed key the given key press to the command parser
   * @return true if keypress was is [part of a] command, false otherwise
   */
  bool handleKeypress(QKeyEvent *e);

  /**
   * feed key the given list of key presses to the key handling code, one by one
   */
  void feedKeys(QList<QKeyEvent> keyPresses);

  /**
   * @return The current vi mode
   */
  ViMode getCurrentViMode() const;

  const QString getVerbatimKeys() const;

  /**
   * changes the current vi mode to the given mode
   */
  void changeViMode(ViMode newMode);

  /**
   * set normal mode to be the active vi mode and perform the needed setup work
   */
  void viEnterNormalMode();

  /**
   * set insert mode to be the active vi mode and perform the needed setup work
   */
  void viEnterInsertMode();

  /**
   * set visual mode to be the active vi mode and make the needed setup work
   * if visualLine is true, visual linewise mode is started
   */
  void viEnterVisualMode( bool visualLine = false);

  /**
   * @return the KateViNormalMode instance
   */
  KateViNormalMode* getViNormalMode();

  /**
   * @return the KateViInsertMode instance
   */
  KateViInsertMode* getViInsertMode();

  /**
   * @return the KateViVisualMode instance
   */
  KateViVisualMode* getViVisualMode();

  private:
    KateViNormalMode* m_viNormalMode;
    KateViInsertMode* m_viInsertMode;
    KateViVisualMode* m_viVisualMode;

    ViMode m_currentViMode;

    KateView *m_view;
    KateViewInternal *m_viewInternal;
};

#endif
