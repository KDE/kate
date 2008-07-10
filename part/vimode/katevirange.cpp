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

#include "katevirange.h"

    KateViRange::KateViRange( int slin, int scol, int elin, int ecol, ViMotion::MotionType inc )
  : startLine( slin ), startColumn( scol ), endLine( elin ), endColumn( ecol ), motionType( inc )
{
  valid = true;
}

// for motions which only return a position, in constrast to "text objects"
// which returns a range
    KateViRange::KateViRange( int elin, int ecol, ViMotion::MotionType inc )
  : endLine( elin ), endColumn( ecol ), motionType( inc )
{
  startLine = -1;
  startColumn = -1;
  valid = true;
}

KateViRange::KateViRange()
{
  startLine = -1;
  startColumn = -1;
  endLine = -1;
  endColumn = -1;
  valid = true;
  motionType = ViMotion::InclusiveMotion;
}
