/***************************************************************************
                          kateglobal.h  -  description
                             -------------------
    begin                : Mon Jan 15 2001
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

#ifndef kate_global_h
#define kate_global_h

#include <config.h>

class KateTextCursor
{
  public:
    int col;
    int line;
};

class KateLineRange
{
  public:
    int start;
    int end;
};

#endif


