/* This file is part of the KDE libraries
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>

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
#include "katecmd.moc"

#include "katedocument.h"
#include "kateview.h"

KateCmd::KateCmd (KateDocument *doc) : QObject (doc),
  m_doc (doc)
{
  m_parser.append (new KateCommands::InsertTime (m_doc));
  m_parser.append (new KateCommands::SedReplace (m_doc));
  m_parser.append (new KateCommands::Character (m_doc));
}

KateCmd::~KateCmd ()
{
    m_parser.setAutoDelete(true);
    m_parser.clear();
}

void KateCmd::execCmd (QString cmd, KateView *view)
{
  for (uint i=0; i<m_parser.count(); i++)
  {
    if (m_parser.at(i)->execCmd (cmd, view))
      break;
  }
}

KateCmdParser::KateCmdParser (KateDocument *doc)
: m_doc (doc)
{
}

KateCmdParser::~KateCmdParser()
{
}


