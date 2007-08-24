/**
  * This file is part of the KDE project
  * Copyright (C) 2007, 2006 Rafael Fernández López <ereslibre@gmail.com>
  * Copyright (C) 2002-2003 Matthias Kretz <kretz@kde.org>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Library General Public
  * License version 2 as published by the Free Software Foundation.
  *
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Library General Public License for more details.
  *
  * You should have received a copy of the GNU Library General Public License
  * along with this library; see the file COPYING.LIB.  If not, write to
  * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  * Boston, MA 02110-1301, USA.
  */

#ifndef KATEPLUGINSELECTOR_H
#define KATEPLUGINSELECTOR_H

#include <QtGui/QWidget>

#include <QtCore/QList>

#include <ksharedconfig.h>

class KComponentData;
class KPluginInfo;


/**
  * @short A widget to select what plugins to load and configure the plugins.
  *
  * It shows the list of available plugins
  *
  * Since the user needs a way to know what a specific plugin does every plugin
  * sould install a desktop file containing a name, comment and category field.
  * The category is useful for applications that can use different kinds of
  * plugins like a playlist, skin or visualization
  *
  * The location of these desktop files is the
  * share/apps/&lt;instancename&gt;/&lt;plugindir&gt; directory. But if you need
  * you may use a different directory
  *
  * You can add plugins from different KConfig[group], by just calling all times
  * you want addPlugins method with the correct parameters
  *
  * Additionally, calls to constructor with same @p categoryName, will add new
  * items to the same category, even if plugins are from different categories
  *
  * @author Matthias Kretz <kretz@kde.org>
  * @author Rafael Fernández López <ereslibre@gmail.com>
  */
class KatePluginSelector
    : public QWidget
{
    Q_OBJECT

public:
    enum PluginLoadMethod {
        ReadConfigFile = 0,
        IgnoreConfigFile
    };

    /**
      * Create a new KatePluginSelector
      */
    KatePluginSelector(QWidget *parent = 0);

    /**
      * Destructor
      */
    ~KatePluginSelector();

    /**
      * Add a list of KParts plugins
      *
      * The information about the plugins will be loaded from the
      * share/apps/&lt;instancename&gt;/kpartplugins directory
      *
      * @param componentName The name of the KComponentData of the plugin's parent.
      * @param categoryName  The translated name of the category. This is the
      *                      name that is shown in the title. If the category
      *                      did exist before because of another call to
      *                      addPlugins, then they will be shown in that
      *                      category. If @p categoryName is a new one, then
      *                      a new category will be shown on the plugin window,
      *                      and the list of plugins added to it
      * @param categoryKey   When you have different categories of KParts
      *                      plugins you distinguish between the plugins using
      *                      the Category key in the .desktop file. Use this
      *                      parameter to select only those KParts plugins
      *                      with the Category key == @p categoryKey. If
      *                      @p categoryKey is not set the Category key is
      *                      ignored and all plugins are shown. Not match case
      * @param config        The KConfig object that holds the state of the
      *                      plugins being enabled or not. By default it should
      *                      be componentData.config(). It is recommended to
      *                      always pass a KConfig object if you use
      *                      KSettings::PluginPage since you never know from where the
      *                      page will be called (think global config app).
      *                      For example KViewCanvas passes KConfig(
      *                      "kviewcanvas" )
      */
    void addPlugins(const QString &componentName,
                    const QString &categoryName = QString(),
                    const QString &categoryKey = QString(),
                    KSharedConfig::Ptr config = KSharedConfig::Ptr());

    /**
      * Add a list of KParts plugins. Convenience method for the one above.
      * If not set explicitly, @p config is set to componentData.config()
      */
    void addPlugins(const KComponentData &instance,
                    const QString &categoryName = QString(),
                    const QString &categoryKey = QString(),
                    const KSharedConfig::Ptr &config = KSharedConfig::Ptr());

    /**
      * Add a list of non-KParts plugins
      *
      * @param pluginInfoList   A list of KPluginInfo objects containing the
      *                         necessary information for the plugins you want to
      *                         add to the list
      * @param pluginLoadMethod If KatePluginSelector will try to load the
      *                         state of the plugin when loading the
      *                         dialog from the configuration file or not.
      *                         This is useful if for some reason you
      *                         called the setPluginEnabled() for each plugin
      *                         individually before loading the dialog, and
      *                         don't want KatePluginSelector to override them
      *                         when loading
      * @param categoryName     The translated name of the category. This is the
      *                         name that is shown in the title. If the category
      *                         did exist before because of another call to
      *                         addPlugins, then they will be shown in that
      *                         category. If @p categoryName is a new one, then
      *                         a new category will be shown on the plugin window,
      *                         and the list of plugins added to it
      * @param categoryKey      When you have different categories of KParts
      *                         plugins you distinguish between the plugins using
      *                         the Category key in the .desktop file. Use this
      *                         parameter to select only those KParts plugins
      *                         with the Category key == @p categoryKey. If
      *                         @p categoryKey is not set the Category key is
      *                         ignored and all plugins are shown. Not match case
      * @param config           The KConfig object that holds the state of the
      *                         plugins being enabled or not. By default it will
      *                         use KGlobal::config(). It is recommended to
      *                         always pass a KConfig object if you use
      *                         KSettings::PluginPage since you never know from
      *                         where the page will be called (think global
      *                         config app). For example KViewCanvas passes
      *                         KConfig("kviewcanvas")
      *
      * @note   All plugins that were set a config group using setConfig() method
      *         will load and save their information from there. For those that
      *         weren't any config object, @p config will be used
      */
    void addPlugins(const QList<KPluginInfo> &pluginInfoList,
                    PluginLoadMethod pluginLoadMethod = ReadConfigFile,
                    const QString &categoryName = QString(),
                    const QString &categoryKey = QString(),
                    const KSharedConfig::Ptr &config = KSharedConfig::Ptr());

    /**
      * Load the state of the plugins (selected or not) from the KPluginInfo
      * objects
      */
    void load();

    /**
      * Save the configuration
      */
    void save();

    /**
      * Change to applications defaults
      */
    void defaults();

    /**
      * Updates plugins state (enabled or not)
      *
      * This method won't save anything on any configuration file. It will just
      * be useful if you added plugins with the method:
      *
      * \code
      * void addPlugins(const QList<KPluginInfo> &pluginInfoList,
      *                 const QString &categoryName = QString(),
      *                 const QString &categoryKey = QString(),
      *                 const KSharedConfig::Ptr &config = KSharedConfig::Ptr());
      * \endcode
      *
      * To sum up, this method will update your plugins state depending if plugins
      * are ticked or not on the KatePluginSelector dialog, without saving anything
      * anywhere
      */
    void updatePluginsState();

Q_SIGNALS:
    /**
      * Tells you whether the configuration is changed or not.
      */
    void changed(bool hasChanged);

    /**
      * Emitted after the config of an embedded KCM has been saved. The
      * argument is the name of the parent component that needs to reload
      * its config
      */
    void configCommitted(const QByteArray &componentName);

private:
    class Private;
    Private * const d;
};

#endif
