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

#include "configinterface.h"    

#include "document.h"
#include "plugin.h"

namespace KTextEditor
{

class PrivateConfigInterface
{
  public:
    PrivateConfigInterface() {}
    ~PrivateConfigInterface() {}
};

};

using namespace KTextEditor;

unsigned int ConfigInterface::globalConfigInterfaceNumber = 0;

ConfigInterface::ConfigInterface()
{
  globalConfigInterfaceNumber++;
  myConfigInterfaceNumber = globalConfigInterfaceNumber++;

  d = new PrivateConfigInterface();
}

ConfigInterface::~ConfigInterface()
{
  delete d;
}

unsigned int ConfigInterface::configInterfaceNumber () const
{
  return myConfigInterfaceNumber;
}

ConfigInterface *KTextEditor::configInterface (Document *doc)
{                       
  if (!doc)
    return 0;

  return static_cast<ConfigInterface*>(doc->qt_cast("KTextEditor::ConfigInterface"));
}      

ConfigInterface *KTextEditor::configInterface (Plugin *plugin)
{                       
  if (!plugin)
    return 0;

  return static_cast<ConfigInterface*>(plugin->qt_cast("KTextEditor::ConfigInterface"));
}
