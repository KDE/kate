/* This file is part of the KDE libraries
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>

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

// $Id$

#include "viewcursorinterface.h"

#include <qobject.h>

namespace KTextEditor
{

class PrivateViewCursorInterface
{
  public:
    PrivateViewCursorInterface() {}
    ~PrivateViewCursorInterface() {}
};

};

using namespace KTextEditor;

unsigned int ViewCursorInterface::globalViewCursorInterfaceNumber = 0;

ViewCursorInterface::ViewCursorInterface()
{
  globalViewCursorInterfaceNumber++;
  myViewCursorInterfaceNumber = globalViewCursorInterfaceNumber++;

  d = new PrivateViewCursorInterface();
}

ViewCursorInterface::~ViewCursorInterface()
{
  delete d;
}

unsigned int ViewCursorInterface::viewCursorInterfaceNumber () const
{
  return myViewCursorInterfaceNumber;
}

ViewCursorInterface *KTextEditor::viewCursorInterface (QObject *obj)
{
  return static_cast<ViewCursorInterface*>(obj->qt_cast("KTextEditor::ViewCursorInterface"));
}
