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
    m_cmds += m_parser.at(i)->cmds ();
}

KateCmd::~KateCmd ()
{
}

KateCmdParser *KateCmd::query (const QString &cmd)
{
  for (uint i=0; i<m_parser.count(); i++)
    if (m_parser.at(i)->usable (cmd))
      return m_parser.at(i);

  return 0;
}

QStringList KateCmd::cmds ()
{
  return m_cmds;
}
