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

KateCmd::KateCmd (KateDocument *doc) : QObject (doc)
{
  myDoc = doc;

  myParser.append (new KateCommands::InsertTime (myDoc));
  myParser.append (new KateCommands::SedReplace (myDoc));
  myParser.append (new KateCommands::Character (myDoc));
}

KateCmd::~KateCmd ()
{
    myParser.setAutoDelete(true);
    myParser.clear();
}

void KateCmd::execCmd (QString cmd, KateView *view)
{
  for (uint i=0; i<myParser.count(); i++)
  {
    if (myParser.at(i)->execCmd (cmd, view))
      break;
  }
}

KateCmdParser::KateCmdParser (KateDocument *doc)
{
  myDoc = doc;
}

KateCmdParser::~KateCmdParser()
{
}


