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

#include "katecmds.h"

KateCmd::KateCmd ()
{
  m_parser.setAutoDelete(true);
  m_parser.append (new KateCommands::SedReplace ());
  m_parser.append (new KateCommands::Character ());
  m_parser.append (new KateCommands::Goto ());
  m_parser.append (new KateCommands::Date ());

  for (uint i=0; i<m_parser.count(); i++)
  {
    QStringList l = m_parser.at(i)->cmds ();

    for (uint z=0; z<l.count(); z++)
      m_dict.insert (l[z], m_parser.at(i));

    m_cmds += l;
  }
}

KateCmd::~KateCmd ()
{
}

KateCmdParser *KateCmd::query (const QString &cmd)
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
