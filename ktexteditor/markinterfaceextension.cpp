/* This file is part of the KDE libraries
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>

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

#include "markinterfaceextension.h"
#include "document.h"

namespace KTextEditor
{

class PrivateMarkInterfaceExtension
{
  public:
    PrivateMarkInterfaceExtension() {}
    ~PrivateMarkInterfaceExtension() {}
};

};

using namespace KTextEditor;

unsigned int MarkInterfaceExtension::globalMarkInterfaceExtensionNumber = 0;

MarkInterfaceExtension::MarkInterfaceExtension()
{
  globalMarkInterfaceExtensionNumber++;
  myMarkInterfaceExtensionNumber = globalMarkInterfaceExtensionNumber++;

  d = new PrivateMarkInterfaceExtension();
}

MarkInterfaceExtension::~MarkInterfaceExtension()
{
  delete d;
}

unsigned int MarkInterfaceExtension::markInterfaceExtensionNumber () const
{
  return myMarkInterfaceExtensionNumber;
}

void MarkInterfaceExtension::setMarkInterfaceExtensionDCOPSuffix (const QCString &/*suffix*/)
{
  //d->interface->setObjId ("MarkInterfaceExtension#"+suffix);
}

MarkInterfaceExtension *KTextEditor::markInterfaceExtension (Document *doc)
{                   
  if (!doc)
    return 0;

  return static_cast<MarkInterfaceExtension*>(doc->qt_cast("KTextEditor::MarkInterfaceExtension"));
}
