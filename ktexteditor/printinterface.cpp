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

#include "printinterface.h"
#include "printdcopinterface.h"
#include "document.h"

namespace KTextEditor
{

class PrivatePrintInterface
{
  public:
    PrivatePrintInterface() {interface=0;}
    ~PrivatePrintInterface() {}
    PrintDCOPInterface *interface;
};

};

using namespace KTextEditor;

unsigned int PrintInterface::globalPrintInterfaceNumber = 0;

PrintInterface::PrintInterface()
{
  globalPrintInterfaceNumber++;
  myPrintInterfaceNumber = globalPrintInterfaceNumber++;

  d = new PrivatePrintInterface();
  QString name = "PrintInterface#" + QString::number(myPrintInterfaceNumber);
  d->interface = new PrintDCOPInterface(this, name.latin1());
}

PrintInterface::~PrintInterface()
{
  delete d->interface;
  delete d;
}

unsigned int PrintInterface::printInterfaceNumber () const
{
  return myPrintInterfaceNumber;
}

void PrintInterface::setPrintInterfaceDCOPSuffix (const QCString &suffix)
{
  d->interface->setObjId ("PrintInterface#"+suffix);
}

PrintInterface *KTextEditor::printInterface (Document *doc)
{           
  if (!doc)
    return 0;

  return static_cast<PrintInterface*>(doc->qt_cast("KTextEditor::PrintInterface"));
}
