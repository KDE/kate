/***************************************************************************
                          katecmds.h  -  description
                             -------------------
    begin                : Mon Feb 5 2001
    copyright            : (C) 2001 by Christoph Cullmann
	                         Charles Samuels
    email                : cullmann@kde.org
                           charles@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _KATE_CMDS_H
#define _KATE_CMDS_H

#include "kateglobal.h"
#include "katecmd.h"

/**
 * this namespace will be maintained by Charles Samuels <charles@kde.org>
 * so we're going to be using this indentation style here.
 *
 * Respect my style, and I'll respect your's!
 **/
namespace KateCommands
{

/**
 * This is by Christoph Cullmann
 **/
class InsertTime : public KateCmdParser
{
public:
	InsertTime(KateDocument *doc=0) : KateCmdParser(doc) { }

	bool execCmd(QString cmd=0, KateView *view=0);
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
class SedReplace : public KateCmdParser
{
public:
	SedReplace(KateDocument *doc=0) : KateCmdParser(doc) { }

	bool execCmd(QString cmd=0, KateView *view=0);
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
	Character(KateDocument *doc=0) : KateCmdParser(doc) { }

	bool execCmd(QString cmd=0, KateView *view=0);
};

class Fifo : public KateCmdParser
{
public:
	Fifo(KateDocument *doc=0) : KateCmdParser(doc) { }

	bool execCmd(QString cmd=0, KateView *view=0);
};

}

// vim: noet

#endif

