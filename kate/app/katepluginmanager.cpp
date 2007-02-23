/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>

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

#include "katepluginmanager.h"
#include "katepluginmanager.moc"

#include "kateapp.h"
#include "katemainwindow.h"

#include <kate/application.h>

#include <KConfig>
#include <QStringList>

//Added by qt3to4:
#include <QList>
#include <KMessageBox>
#include <KServiceTypeTrader>
#include <kdebug.h>
#include <QFile>

QString KatePluginInfo::saveName() const
{
  QString saveName = service->property("X-Kate-PluginName").toString();

  if (saveName.isEmpty())
    saveName = service->library();
  return saveName;
}

KatePluginManager::KatePluginManager(QObject *parent) : QObject (parent)
{
  m_pluginManager = new Kate::PluginManager (this);
  setupPluginList ();
}

KatePluginManager::~KatePluginManager()
{
  // than unload the plugins
  unloadAllPlugins ();
}

KatePluginManager *KatePluginManager::self()
{
  return KateApp::self()->pluginManager ();
}

void KatePluginManager::setupPluginList ()
{
  KService::List traderList = KServiceTypeTrader::self()->query("Kate/Plugin", "(not ('Kate/ProjectPlugin' in ServiceTypes)) and (not ('Kate/InitPlugin' in ServiceTypes))");

  foreach(const KService::Ptr &ptr, traderList)
  {
    double pVersion = ptr->property("X-Kate-Version").toDouble();

    // don't use plugins out of 3.x release series
    if ((pVersion >= 2.8) && (pVersion <= KateApp::kateVersion(false).toDouble()))
    {
      KatePluginInfo info;

      info.load = false;
      info.service = ptr;
      info.plugin = 0L;

      m_pluginList.push_back (info);
    }
  }
}

void KatePluginManager::loadConfig (KConfig* config)
{
  // first: unload the plugins
  unloadAllPlugins ();

  if ( ! config ) return;

  KConfigGroup cg = KConfigGroup(config, "Kate Plugins");

  // disable all plugin if no config...
  foreach (const KatePluginInfo &plugin, m_pluginList)
    plugin.load =  cg.readEntry (plugin.service->library(), false) ||
                 cg.readEntry (plugin.service->property("X-Kate-PluginName").toString(), false);

  for (KatePluginList::iterator it = m_pluginList.begin();it != m_pluginList.end(); ++it)
  {
    if (it->load)
    {
      loadPlugin(&(*it));

      // restore config
      if (it->plugin)
        it->plugin->readSessionConfig(config, QString("Plugin:%1:").arg(it->saveName()));
    }
  }
}

void KatePluginManager::writeConfig(KConfig* config)
{
  Q_ASSERT( config );

  KConfigGroup cg = KConfigGroup( config, "Kate Plugins" );
  foreach(const KatePluginInfo &plugin, m_pluginList)
  {
    QString saveName = plugin.saveName();

    cg.writeEntry (saveName, plugin.load);

    if (plugin.plugin)
      plugin.plugin->writeSessionConfig(config, QString("Plugin:%1:").arg(saveName));
  }
}

void KatePluginManager::unloadAllPlugins ()
{
  for (KatePluginList::iterator it = m_pluginList.begin();it != m_pluginList.end(); ++it)
  {
    if (it->plugin)
      unloadPlugin(&(*it));
  }
}

void KatePluginManager::enableAllPluginsGUI (KateMainWindow *win, KConfigBase *config)
{
  for (KatePluginList::iterator it = m_pluginList.begin();it != m_pluginList.end(); ++it)
  {
    if (it->plugin)
      enablePluginGUI(&(*it), win, config);
  }
}

void KatePluginManager::disableAllPluginsGUI (KateMainWindow *win)
{
  for (KatePluginList::iterator it = m_pluginList.begin();it != m_pluginList.end(); ++it)
  {
    if (it->plugin)
      disablePluginGUI(&(*it), win);
  }
}

void KatePluginManager::loadPlugin (KatePluginInfo *item)
{
  QString pluginName = item->service->property("X-Kate-PluginName").toString();

  if (pluginName.isEmpty())
    pluginName = item->service->library();

  item->plugin = Kate::createPlugin (QFile::encodeName(item->service->library()), Kate::application(), QStringList(pluginName));
}

void KatePluginManager::unloadPlugin (KatePluginInfo *item)
{
  disablePluginGUI (item);
  if (item->plugin) delete item->plugin;
  item->plugin = 0L;
}

void KatePluginManager::enablePluginGUI (KatePluginInfo *item, KateMainWindow *win, KConfigBase *config)
{
  // plugin around at all?
  if (!item->plugin)
    return;

  // lookup if there is already a view for it..
  if (!win->pluginViews().contains(item->plugin))
  {
    // create the view
    Kate::PluginView *view = item->plugin->createView(win->mainWindow());
    win->pluginViews().insert (item->plugin, view);
  }

  // load session config if needed
  if (config && win->pluginViews().contains(item->plugin))
  {
    int winID = KateApp::self()->mainWindowID(win);
    win->pluginViews().value(item->plugin)->readSessionConfig(config, QString("Plugin:%1:MainWindow:%2").arg(item->saveName()).arg(winID));
  }
}

void KatePluginManager::enablePluginGUI (KatePluginInfo *item)
{
  // plugin around at all?
  if (!item->plugin)
    return;

  // enable the gui for all mainwindows...
  for (int i = 0; i < KateApp::self()->mainWindows(); i++)
    enablePluginGUI (item, KateApp::self()->mainWindow(i), 0);
}

void KatePluginManager::disablePluginGUI (KatePluginInfo *item, KateMainWindow *win)
{
  // plugin around at all?
  if (!item->plugin)
    return;

  // lookup if there is a view for it..
  if (!win->pluginViews().contains(item->plugin))
    return;

  // really delete the view of this plugin
  delete win->pluginViews().value(item->plugin);
  win->pluginViews().remove (item->plugin);
}

void KatePluginManager::disablePluginGUI (KatePluginInfo *item)
{
  // plugin around at all?
  if (!item->plugin)
    return;

  // disable the gui for all mainwindows...
  for (int i = 0; i < KateApp::self()->mainWindows(); i++)
    disablePluginGUI (item, KateApp::self()->mainWindow(i));
}

Kate::Plugin *KatePluginManager::plugin(const QString &name)
{
  foreach(const KatePluginInfo &info, m_pluginList)
  {
    QString pluginName = info.service->property("X-Kate-PluginName").toString();
    if (pluginName.isEmpty())
      pluginName = info.service->library();
    if  (pluginName == name)
    {
      if (info.plugin)
        return info.plugin;
      else
        break;
    }
  }
  return 0;
}

bool KatePluginManager::pluginAvailable(const QString &)
{
  return false;
}
class Kate::Plugin *KatePluginManager::loadPlugin(const QString &, bool )
{
    return 0;
}
void KatePluginManager::unloadPlugin(const QString &, bool)
{
  ;
}
// kate: space-indent on; indent-width 2; replace-tabs on;

