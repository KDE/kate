/* This file is part of the KDE libraries
   Copyright (C) 2001, 2003 Christoph Cullmann <cullmann@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

// $Id$

#include "katecmd.h"

KateCmd::KateCmd ()
{
  m_parser.setAutoDelete(true);
}

KateCmd::~KateCmd ()
{
}

bool KateCmd::registerCommand (Kate::Command *cmd)
{
  m_parser.append (cmd);

  QStringList l = cmd->cmds ();

  for (uint z=0; z<l.count(); z++)
    m_dict.insert (l[z], cmd);

  m_cmds += l;

  return true;
}

bool KateCmd::unregisterCommand (Kate::Command *cmd)
{
  return true;
}

Kate::Command *KateCmd::queryCommand (const QString &cmd)
{
  uint f = 0;
  while (cmd[f].isLetterOrNumber())
    f++;

  return m_dict[cmd.left(f)];
}

QStringList KateCmd::cmds ()
{
  return m_cmds;
}

KateCmd *KateCmd::s_cmd = 0;

KateCmd *KateCmd::instance ()
{
  if (!s_cmd)
    s_cmd = new KateCmd ();

  return s_cmd;
}
