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

#include "document.h"

#include <qdict.h>
#include <qstringlist.h>

class KATEPARTINTERFACES_EXPORT KateCmd
{
  private:
    KateCmd ();

  public:
    ~KateCmd ();

    static KateCmd *self ();

    bool registerCommand (Kate::Command *cmd);
    bool unregisterCommand (Kate::Command *cmd);
    Kate::Command *queryCommand (const QString &cmd);

    QStringList cmds ();
    void appendHistory( const QString &cmd );
    const QString fromHistory( uint i ) const;
    uint historyLength() const { return m_history.count(); }

  private:
    static KateCmd *s_self;
    QDict<Kate::Command> m_dict;
    QStringList m_cmds;
    QStringList m_history;
};

#endif
