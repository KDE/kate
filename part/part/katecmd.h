/***************************************************************************
                          katecmd.h  -  description
                             -------------------
    begin                : Mon Feb 5 2001
    copyright            : (C) 2001 by Christoph Cullmann
    email                : cullmann@kde.org
 ***************************************************************************/

/***************************************************************************
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

