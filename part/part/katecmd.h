/***************************************************************************
                          katecmd.h  -  description
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

#ifndef _KATE_CMD_H
#define _KATE_CMD_H

#include "kateglobal.h"

#include <qobject.h>
#include <qstring.h>
#include <qptrlist.h>

class KateCmdParser
{
  public:
    KateCmdParser (class KateDocument *doc=0L);
    virtual ~KateCmdParser ();

    virtual bool execCmd (QString cmd=0L, class KateView *view=0L)=0;

  private:
    class KateDocument *myDoc;
};

class KateCmd : public QObject
{
  Q_OBJECT

  public:
    KateCmd (class KateDocument *doc=0L);
    ~KateCmd ();

    void execCmd (QString cmd=0L, class KateView *view=0L);

  private:
    class KateDocument *myDoc;
    QPtrList<KateCmdParser> myParser;
};

#endif

