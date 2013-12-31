/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef _KATE_PLUGINMANAGER_INCLUDE_
#define _KATE_PLUGINMANAGER_INCLUDE_

#include <kateinterfaces_export.h>

#include <QObject>

namespace Kate
{

class Plugin;
  
  /**
   * \brief Interface to the plugin manager.
   *
   * This interface provides access to Kate's plugin manager. To load a plugin
   * call loadPlugin(), to unload a plugin call unloadPlugin(). To check,
   * whether a plugin is loaded use plugin(), and to check a plugin's
   * availability use pluginAvailable().
   *
   * To access the plugin manager use Application::pluginManager().
   * You should never have to create an instance of this class yourself.
   *
   * \note Usually the Kate application itself loads/unloads the plugins, so
   *       plugins actually never have to call loadPlugin() or unloadPlugin().
   *
   *
   * \author Christoph Cullmann \<cullmann@kde.org\>
   * \see Plugin
   */
  class KATEINTERFACES_EXPORT PluginManager : public QObject
  {
      friend class PrivatePluginManager;

      Q_OBJECT

    public:
      /**
       * Constructor.
       *
       * Create a new plugin manager. You as a plugin developer should never
       * have to create a plugin manager yourself. Get the plugin manager
       * with the Application instance.
       * \param pluginManager internal usage
       */
      PluginManager ( void *pluginManager  );
      
      /**
       * Virtual destructor.
       */
      virtual ~PluginManager ();

    Q_SIGNALS:
      /**
       * New plugin loaded.
       * @param name name of new loaded plugin
       * @param plugin new loaded plugin
       */
      void pluginLoaded (const QString &name, Kate::Plugin *plugin);
      
      /**
       * This signal is emitted when the plugin has been unloaded, aka deleted.
       *
       *  Warning !!! DO NOT ACCESS THE DATA REFERENCED BY THE POINTER, IT IS ALREADY INVALID
       *  Use the pointer only to remove mappings in hash or maps
       * 
       * @param name name of unloaded plugin
       * @param plugin unloaded plugin, already deleted
       */
      void pluginUnloaded (const QString &name, Kate::Plugin *plugin);
      
    private:
      class PrivatePluginManager *d;
  };
}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
