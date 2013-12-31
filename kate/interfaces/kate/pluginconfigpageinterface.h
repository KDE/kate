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

#ifndef kate_pluginconfigpageinterface_h
#define kate_pluginconfigpageinterface_h

#include <kateinterfaces_export.h>

#include <ktexteditor/configpage.h>

#include <QIcon>
#include <QPixmap>
#include <QWidget>

namespace Kate
{

  /**
   * \brief Plugin config page widget.
   *
   * The class PluginConfigPage is a base class for config pages for plugins.
   * If you want to add support for a config page to your Kate plugin you have
   * to derive a class from PluginConfigPage and overwrite apply(), reset()
   * and default() as <b>plublic slots</b>.
   *
   * Whenever settings were changed in your config page emit the signal
   * changed().
   *
   * \see Plugin, PluginConfigPageInterface
   * \author Christoph Cullmann \<cullmann@kde.org\>
   */
  typedef KTextEditor::ConfigPage PluginConfigPage;

  /**
   * \brief Plugin config page extension interface.
   *
   * The class PluginConfigPageInterface is an extension interface for plugins.
   * If you want to add config pages to a plugin you have to make sure you
   *  -# derive your plugin from this interface (multiple inheritance) and
   *     overwrite the abstract methods
   *  -# return the number of config pages your plugin supports in configPages()
   *  -# return an instance the requested config pages in configPage()
   *
   * Your plugin header then looks like this:
   * \code
   * class MyPlugin : public Kate::Plugin,
   *                  public Kate::PluginConfigPageInterface
   * {
   *     Q_OBJECT
   *     Q_INTERFACES(Kate::PluginConfigPageInterface)
   *
   * public:
   *     // other methods etc...
   * };
   * \endcode
   * The line \p Q_INTERFACES... is important, otherwise the qobject_cast from
   * Plugin to PluginConfigPageInterface fails so that your config page interface
   * will not be recognized.
   *
   * \see Plugin, PluginConfigPage
   * \author Christoph Cullmann \<cullmann@kde.org\>
   */
  class KATEINTERFACES_EXPORT PluginConfigPageInterface
  {
      friend class PrivatePluginConfigPageInterface;

    public:
      /**
       * Constructor.
       */
      PluginConfigPageInterface();
      /**
       * Virtual destructor.
       */
      virtual ~PluginConfigPageInterface();

      //
      // slots !!!
      //
    public:
      /**
       * Reimplement this function and return the number of config pages your
       * plugin supports.
       * \return number of config pages
       * \see configPage()
       */
      virtual uint configPages () const = 0;

      /**
       * Return the config page with the given \p number and \p parent.
       * Assume you return \c N in configPages(), then you have to return
       * config pages for the numbers 0 to \p N-1.
       *
       * \note This function will only be called if configPages() > 0.
       *
       * \param number the config page for the given \p number
       * \param parent use this widget as parent widget for the config page
       * \param name config page identifier
       * \return a config page
       * \see configPages()
       */
      virtual PluginConfigPage *configPage (uint number = 0, QWidget *parent = 0, const char *name = 0 ) = 0;

      /**
       * Return a short name for the config page with the given \p number, for
       * example 'Autobookmarker'.
       * \param number the config page for the given \p number
       * \return a short name
       */
      virtual QString configPageName (uint number = 0) const = 0;
      /**
       * Return the full name for the config page with the given \p number, for
       * example 'Configure Autobookmarker'.
       * \param number the config page for the given \p number
       * \return a more descriptive name
       */
      virtual QString configPageFullName (uint number = 0) const = 0;
      /**
       * Return an icon for for the config page with the given \p number.
       * \param number the config page for the given \p number
       * \return the icon for the config page
       */
      virtual QIcon configPageIcon (uint number = 0) const = 0;

    private:
      class PrivatePluginConfigPageInterface *d;
  };

  class Plugin;
  /**
   * Helper function that returns the PluginConfigPageInterface of the \p plugin
   * or NULL if the \p plugin does not support the interface.
   * \param plugin the plugin for which the plugin config page interface should
   *        be returned
   * \return the plugin config page interface or NULL if the plugin does not
   *        support the interface
   */
  KATEINTERFACES_EXPORT PluginConfigPageInterface *pluginConfigPageInterface (Plugin *plugin);

}

Q_DECLARE_INTERFACE(Kate::PluginConfigPageInterface, "org.kde.Kate.PluginConfigPageInterface")
#endif
// kate: space-indent on; indent-width 2; replace-tabs on;

