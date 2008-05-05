/* This file is part of the KDE libraries
   Copyright (C) 2008 Erlend Hamberg <ehamberg@gmail.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) version 3.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "katevicommandparser.h"

#include <kdebug.h>

KateViCommandParser:: KateViCommandParser(KateView* view)
: m_view(view)
{
}

KateViCommandParser::~KateViCommandParser()
{
}

bool KateViCommandParser::eatKey(int key)
{
  kDebug(13070) << "Yum! Key eaten: " << (char)key << "(" << key << ")";

  // FIXME: temporary hack
  if (key == Qt::Key_I)
    m_view->viEnterInsertMode();

  return false;
}
