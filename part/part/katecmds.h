/* This file is part of the KDE libraries
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
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

#ifndef _KATE_CMDS_H
#define _KATE_CMDS_H

#include "katecmd.h"

/**
 * this namespace will be maintained by Charles Samuels <charles@kde.org>
 * so we're going to be using this indentation style here.
 *
 * Respect my style, and I'll respect yours!
 **/
namespace KateCommands
{

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
class SedReplace : public KateCmdParser
{
  public:
    bool usable (const QString &cmd);

    bool exec (class KateView *view, const QString &cmd, QString &errorMsg);

    QStringList cmds () { return QStringList("s/search/replace/"); };

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
class Character : public KateCmdParser
{
  public:
    bool usable (const QString &cmd);

    bool exec (class KateView *view, const QString &cmd, QString &errorMsg);

    QStringList cmds () { return QStringList("char:"); };
};

class Goto : public KateCmdParser
{
  public:
    bool usable (const QString &cmd);

    bool exec (class KateView *view, const QString &cmd, QString &errorMsg);

    QStringList cmds () { return QStringList("goto:"); };
};

class Date : public KateCmdParser
{
  public:
    bool usable (const QString &cmd);

    bool exec (class KateView *view, const QString &cmd, QString &errorMsg);

    QStringList cmds () { return QStringList("date:"); };
};

}

// vim: noet

#endif
