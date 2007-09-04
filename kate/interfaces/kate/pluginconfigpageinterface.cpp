/* This file is part of the KDE project
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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "pluginconfigpageinterface.h"
#include "pluginconfigpageinterface.moc"

#include "plugin.h"

namespace Kate
{

  class PrivatePluginConfigPageInterface
  {
    public:
      PrivatePluginConfigPageInterface()
      {}
      ~PrivatePluginConfigPageInterface()
      {}
  };

  PluginConfigPage::PluginConfigPage ( QWidget *parent, const char *name ) : QWidget (parent)
  {
    setObjectName( name );
  }

  PluginConfigPage::~PluginConfigPage ()
  { }

  unsigned int PluginConfigPageInterface::globalPluginConfigPageInterfaceNumber = 0;

  PluginConfigPageInterface::PluginConfigPageInterface()
  {
    globalPluginConfigPageInterfaceNumber++;
    myPluginConfigPageInterfaceNumber = globalPluginConfigPageInterfaceNumber++;

    d = new PrivatePluginConfigPageInterface();
  }

  PluginConfigPageInterface::~PluginConfigPageInterface()
  {
    delete d;
  }

  unsigned int PluginConfigPageInterface::pluginConfigPageInterfaceNumber () const
  {
    return myPluginConfigPageInterfaceNumber;
  }

  PluginConfigPageInterface *pluginConfigPageInterface (Plugin *plugin)
  {
    if (!plugin)
      return 0;

    return qobject_cast<PluginConfigPageInterface*>(plugin);
  }

}
// kate: space-indent on; indent-width 2; replace-tabs on;

