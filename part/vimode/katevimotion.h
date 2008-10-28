/* This file is part of the KDE libraries
 * Copyright (C) 2008 Erlend Hamberg <ehamberg@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) version 3.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef KATE_VI_MOTION_INCLUDED
#define KATE_VI_MOTION_INCLUDED

#include "katevicommand.h"
#include "katevinormalmode.h"
#include "katevirange.h"

class KateViNormalMode;

/**
 * combined class for motions and text objects. execute() returns a KateViRange.
 * For motions the returned range is only a position (start pos is (-1, -1) to
 * indicate this), for motions a range (startx, starty), (endx, endy) is
 * returned
 */

class KateViMotion : public KateViCommand
{
  public:
    KateViMotion( KateViNormalMode *parent, const QString &pattern,
        KateViRange (KateViNormalMode::*commandMethod)(), unsigned int flags = 0 );
    KateViRange execute() const;

  protected:
    KateViRange (KateViNormalMode::*m_ptr2commandMethod)();
};

#endif
