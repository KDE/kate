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

#ifndef _KATE_CMD_H
#define _KATE_CMD_H

#include <qobject.h>
#include <qptrlist.h>
#include <qstringlist.h>

class KateCmdParser
{
  public:
    KateCmdParser () {};
    virtual ~KateCmdParser () {};

    virtual bool usable (const QString &cmd) = 0;

    virtual bool exec (class KateView *view, const QString &cmd, QString &msg) = 0;

    virtual QStringList cmds () { return QStringList(); };
};

class KateCmd
{
  public:
    KateCmd ();
    ~KateCmd ();

    KateCmdParser *query (const QString &cmd);

    QStringList cmds ();

  private:
    QPtrList<KateCmdParser> m_parser;
    QStringList m_cmds;
};

#endif
