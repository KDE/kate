/* This file is part of the KDE libraries
   Copyright (C) 2003 Anders Lund <anders@alweb.dk>
   Copyright (C) 2001, 2003 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Charles Samuels <charles@kde.org>

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

#ifndef __KATE_CMDS_H__
#define __KATE_CMDS_H__

#include "../interfaces/document.h"
#include "../interfaces/view.h"

/**
 * this namespace will be maintained by Charles Samuels <charles@kde.org>
 * so we're going to be using this indentation style here.
 *
 * Respect my style, and I'll respect yours!
 **/
namespace KateCommands
{

/*
  This Kate::Command provides access to a lot of the core functionality
  of kate part, settings, utilities, navigation etc.
  it needs to get a kateview pointer, it will cast the kate::view pointer
  hard to kateview
*/
class CoreCommands : public Kate::Command
{
  public:
    bool exec( class Kate::View *view, const QString &cmd, QString &errorMsg );
  
    bool help( class Kate::View *, const QString &, QString & ) {return false;};
  
    QStringList cmds();
};

/**
 * -- Charles Samuels <charles@kde.org>
 * Support vim/sed find and replace
 * s/search/replace/ find search, replace with replace on this line
 * %s/search/replace/ do the same to the whole file
 * s/search/replace/i do the S. and R., but case insensitively
 * $s/search/replace/ do the search are replacement to the selection only
 *
 * $s/// is currently unsupported
 **/
class SedReplace : public Kate::Command
{
  public:
    bool exec (class Kate::View *view, const QString &cmd, QString &errorMsg);

    bool help (class Kate::View *, const QString &, QString &) { return false; };

    QStringList cmds () { return QStringList("s"); };

  private:
    static QString sedMagic(QString textLine, const QString &find, const QString &replace, bool noCase, bool repeat);
};

/**
 * insert a unicode or ascii character
 * base 9+1: 1234
 * hex: 0x1234 or x1234
 * octal: 01231
 *
 * prefixed with "char:"
 **/
class Character : public Kate::Command
{
  public:
    bool exec (class Kate::View *view, const QString &cmd, QString &errorMsg);

    bool help (class Kate::View *, const QString &, QString &) { return false; };

    QStringList cmds () { return QStringList("char"); };
};

class Goto : public Kate::Command
{
  public:
    bool exec (class Kate::View *view, const QString &cmd, QString &errorMsg);

    bool help (class Kate::View *, const QString &, QString &) { return false; };

    QStringList cmds () { return QStringList("goto"); };
};

class Date : public Kate::Command
{
  public:
    bool exec (class Kate::View *view, const QString &cmd, QString &errorMsg);

    bool help (class Kate::View *, const QString &, QString &) { return false; };

    QStringList cmds () { return QStringList("date"); };
};

}

// vim: noet

#endif
