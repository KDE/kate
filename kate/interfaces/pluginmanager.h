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

#include <kate_export.h>

#include <QtCore/QObject>
#include <kurl.h>

namespace Kate
{
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

    public:
      /**
       * Get a plugin with identifier \p name.
       * \param name the plugin's name
       * \return pointer to the plugin if a plugin with \p name is loaded,
       *         otherwise NULL
       */
      class Plugin *plugin(const QString &name);

      /**
       * Check, whether a plugin with \p name exists.
       *
       * \return \e true if the plugin exists, otherwise \e false
       * \todo This method is not used yet, i.e. returns always \e false.
       */
      bool pluginAvailable(const QString &name);

      /**
       * Load the plugin \p name.
       *
       * If the plugin does not exist the return value is NULL.
       * \param name the plugin name
       * \param permanent if \e true the plugin will be loaded at the next Kate
       *        startup, otherwise it will only be loaded temporarily during the
       *        current session.
       * \return pointer to the plugin on success, otherwise NULL
       * \see unloadPlugin()
       * \todo This method is not used yet, i.e. returns always NULL.
       */
      class Plugin *loadPlugin(const QString &name, bool permanent = true);

      /**
       * Unload the plugin \p name.
       *
       * \param name the plugin name
       * \param permanent if \e true the plugin will \e not be loaded on the
       *        next Kate startup, \e even if it was loaded with permanent set
       *        to \e true.
       * \see loadPlugin()
       * \todo This method is not used yet, i.e. does nothing.
       */
      void unloadPlugin(const QString &name, bool permanent = true);

    private:
      class PrivatePluginManager *d;
  };

}

#endif
// kate: space-indent on; indent-width 2; replace-tabs on;

