/* This file is part of the KDE libraries
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>

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

#include "documentinfo.h"
#include "documentdcopinfo.h"

namespace KTextEditor
{

class PrivateDocumentInfoInterface
{
  public:
    PrivateDocumentInfoInterface() {interface = 0;}
    ~PrivateDocumentInfoInterface() {}
    DocumentInfoDCOPInterface *interface;
};

};

using namespace KTextEditor;

unsigned int DocumentInfoInterface::globalDocumentInfoInterfaceNumber = 0;

DocumentInfoInterface::DocumentInfoInterface()
{
  globalDocumentInfoInterfaceNumber++;
  myDocumentInfoInterfaceNumber = globalDocumentInfoInterfaceNumber++;

  d = new PrivateDocumentInfoInterface();
  QString name = "DocumentInterface#" + QString::number(myDocumentInfoInterfaceNumber);
  d->interface = new DocumentInfoDCOPInterface(this, name.latin1());
}

DocumentInfoInterface::~DocumentInfoInterface()
{
  delete d->interface;
  delete d;
}

unsigned int DocumentInfoInterface::documentInfoInterfaceNumber () const
{
  return myDocumentInfoInterfaceNumber;
}

void DocumentInfoInterface::setDocumentInfoInterfaceDCOPSuffix (const QCString &suffix)
{
  d->interface->setObjId ("DocumentInfoInterface#"+suffix);
}
