/***************************************************************************
                          katecmd.cpp  -  description
                             -------------------
    begin                : Mon Feb 5 2001
    copyright            : (C) 2001 by Christoph Cullmann
    email                : cullmann@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "katecmd.h"
#include "katecmds.h"
#include "katecmd.moc"

#include "katedocument.h"

KateCmd::KateCmd (KateDocument *doc) : QObject (doc)
{
  myDoc = doc;

  myParser.append (new KateCommands::InsertTime (myDoc));
  myParser.append (new KateCommands::SedReplace (myDoc));
  myParser.append (new KateCommands::Character (myDoc));
}

KateCmd::~KateCmd ()
{
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


