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

#include "viewstatusmsginterface.h"       
#include "viewstatusmsgdcopinterface.h" 
#include "view.h"

#include <qstring.h>

namespace KTextEditor
{

class PrivateViewStatusMsgInterface
{
  public:
    PrivateViewStatusMsgInterface() {interface=0;}
    ~PrivateViewStatusMsgInterface() {}
    ViewStatusMsgDCOPInterface  *interface;
};

};

using namespace KTextEditor;

unsigned int ViewStatusMsgInterface::globalViewStatusMsgInterfaceNumber = 0;

ViewStatusMsgInterface::ViewStatusMsgInterface()
{
  globalViewStatusMsgInterfaceNumber++;
  myViewStatusMsgInterfaceNumber = globalViewStatusMsgInterfaceNumber++;

  d = new PrivateViewStatusMsgInterface();
  ::QString name = "ViewStatusMsgInterface#" + ::QString::number(myViewStatusMsgInterfaceNumber);
  d->interface = new ViewStatusMsgDCOPInterface(this, name.latin1());
}

ViewStatusMsgInterface::~ViewStatusMsgInterface()
{
  delete d->interface;
  delete d;
}

unsigned int ViewStatusMsgInterface::viewStatusMsgInterfaceNumber () const
{
  return myViewStatusMsgInterfaceNumber;
}

void ViewStatusMsgInterface::setViewStatusMsgInterfaceDCOPSuffix (const QCString &suffix)
{
  d->interface->setObjId ("ViewStatusMsgInterface#"+suffix);
}

ViewStatusMsgInterface *KTextEditor::viewStatusMsgInterface (View *view)
{           
  if (!view)
    return 0;

  return static_cast<ViewStatusMsgInterface*>(view->qt_cast("KTextEditor::ViewStatusMsgInterface"));
}
