/* This file is part of the KDE libraries
   Copyright (C) 2003 Anders Lund <anders@alweb.dk>

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

#ifndef _KATE_CORE_COMMANDS_H_
#define _KATE_CORE_COMMANDS_H_

#include "../interfaces/document.h"
#include "../interfaces/view.h"

/*
  This Kate::Command provides access to a lot of the core functionality
  of kate part, settings, utilities, navigation etc.
*/
class KateCoreCommands : public Kate::Command
{
  public:
  QStringList cmds();
  bool exec( class Kate::View *view, const QString &cmd, QString &errorMsg );
  bool help( class Kate::View *view, const QString &cmd, QString &msg ) {return false;};

  static bool getBoolArg( QString s, bool *val  );
};

#endif //_KATE_CORE_COMMANDS_H_
