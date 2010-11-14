/* This file is part of the KDE libraries
   Copyright (C) 2010 Dominik Haumann <dhaumann kde org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KATE_SCRIPT_ACTION_H
#define KATE_SCRIPT_ACTION_H

#include "katecommandlinescript.h"

#include <kactionmenu.h>
#include <kaction.h>

class KateView;

/**
 * KateScriptAction is an action that executes a commandline-script
 * if triggered. It is shown in Tools > Scripts.
 */
class KateScriptAction : public KAction
{
  Q_OBJECT

  public:
    KateScriptAction(const ScriptActionInfo& info, KateView* view);
    virtual ~KateScriptAction();

  public Q_SLOTS:
    void exec();

  private:
    KateView* m_view;
    QString m_command;
    bool m_interactive;
};

/**
 * Tools > Scripts menu
 * This menu is filled with the command line scripts exported
 * via the scripting support.
 */
class KateScriptActionMenu : public KActionMenu
{
  Q_OBJECT

  public:
    KateScriptActionMenu(KateView* view, const QString& text);
    ~KateScriptActionMenu();
    
    void cleanup();

  public Q_SLOTS:
    void repopulate();

  private:
    KateView* m_view;
    QList<QMenu*> m_menus;
    QList<QAction*> m_actions;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
