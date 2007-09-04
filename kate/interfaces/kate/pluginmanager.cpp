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

#include "pluginmanager.moc"

#include "plugin.h"
#include "documentmanager.h"
#include "pluginmanager.h"

#include "../app/katepluginmanager.h"
#include "../app/kateapp.h"

namespace Kate
{

  class PrivatePluginManager
  {
    public:
      PrivatePluginManager ()
      {}

      ~PrivatePluginManager ()
      {}

      KatePluginManager *pluginMan;
  };

  PluginManager::PluginManager (void *pluginManager) : QObject ((KatePluginManager*) pluginManager)
  {
    d = new PrivatePluginManager ();
    d->pluginMan = (KatePluginManager*) pluginManager;
  }

  PluginManager::~PluginManager ()
  {
    delete d;
  }

  Plugin *PluginManager::plugin(const QString &name)
  {
    return d->pluginMan->plugin(name);
  }

  bool PluginManager::pluginAvailable(const QString &name)
  {
    return d->pluginMan->pluginAvailable (name);
  }

  Plugin *PluginManager::loadPlugin(const QString &name, bool permanent)
  {
    return d->pluginMan->loadPlugin (name, permanent);
  }

  void PluginManager::unloadPlugin(const QString &name, bool permanent)
  {
    d->pluginMan->unloadPlugin (name, permanent);
  }

}

// kate: space-indent on; indent-width 2; replace-tabs on;

