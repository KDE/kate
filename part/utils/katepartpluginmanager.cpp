/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2001-2010 Christoph Cullmann <cullmann@kde.org>
 *  Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
 *  Copyright (C) 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>
 *  Copyright (C) 2007 Dominik Haumann <dhaumann kde org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "katepartpluginmanager.h"
#include "katepartpluginmanager.moc"

#include "kateglobal.h"

#include <ktexteditor/plugin.h>
#include <ktexteditor/document.h>
#include <ktexteditor/view.h>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <kxmlguifactory.h>
#include <kplugininfo.h>

#include <kservicetypetrader.h>
#include <kdebug.h>

KatePartPluginInfo::KatePartPluginInfo(KService::Ptr service)
    : m_pluginInfo(service)
{
}

QString KatePartPluginInfo::saveName() const
{
  QString saveName = m_pluginInfo.pluginName();
  if (saveName.isEmpty())
    saveName = service()->library();
  return saveName;
}

KatePartPluginManager::KatePartPluginManager()
  : QObject(),
    m_config(new KConfig("katepartpluginsrc", KConfig::NoGlobals))
{
  setupPluginList ();
  loadConfig ();
}

KatePartPluginManager::~KatePartPluginManager()
{
  writeConfig();
  // than unload the plugins
  unloadAllPlugins ();
  delete m_config;
  m_config = 0;
}

KatePartPluginManager *KatePartPluginManager::self()
{
  return KateGlobal::self()->pluginManager ();
}

void KatePartPluginManager::setupPluginList ()
{
  KService::List traderList = KServiceTypeTrader::self()->
      query("KTextEditor/Plugin",
            "([X-KDE-Version] >= 4.0) and ([X-KDE-Version] <= " + QString("%1.%2").arg(KDE::versionMajor()).arg(KDE::versionMinor()) + ')');

  foreach(const KService::Ptr &ptr, traderList)
  {
    KatePartPluginInfo info(ptr);

    info.load = false;
    info.plugin = 0L;

    m_pluginList.push_back (info);
  }
}

void KatePartPluginManager::addDocument(KTextEditor::Document *doc)
{
  //kDebug() << doc;
  for (KatePartPluginList::iterator it = m_pluginList.begin();
      it != m_pluginList.end(); ++it)
  {
    if (it->load) {
      it->plugin->addDocument(doc);
    }
  }
}

void KatePartPluginManager::removeDocument(KTextEditor::Document *doc)
{
  //kDebug() << doc;
  for (KatePartPluginList::iterator it = m_pluginList.begin();
      it != m_pluginList.end(); ++it)
  {
    if (it->load) {
      it->plugin->removeDocument(doc);
    }
  }
}

void KatePartPluginManager::addView(KTextEditor::View *view)
{
  //kDebug() << view;
  for (KatePartPluginList::iterator it = m_pluginList.begin();
      it != m_pluginList.end(); ++it)
  {
    if (it->load) {
      it->plugin->addView(view);
    }
  }
}

void KatePartPluginManager::removeView(KTextEditor::View *view)
{
  //kDebug() << view;
  for (KatePartPluginList::iterator it = m_pluginList.begin();
      it != m_pluginList.end(); ++it)
  {
    if (it->load) {
      it->plugin->removeView(view);
    }
  }
}

void KatePartPluginManager::loadConfig ()
{
  // first: unload the plugins
  unloadAllPlugins ();

  KConfigGroup cg = KConfigGroup(m_config, "Kate Part Plugins");

  // disable all plugin if no config...
  foreach (const KatePartPluginInfo &plugin, m_pluginList) {
    bool enabledByDefault = plugin.isEnabledByDefault();
    plugin.load = cg.readEntry (plugin.saveName(), enabledByDefault);
  }

  loadAllPlugins();
}

void KatePartPluginManager::writeConfig()
{
  KConfigGroup cg = KConfigGroup( m_config, "Kate Part Plugins" );
  foreach(const KatePartPluginInfo &it, m_pluginList)
  {
    cg.writeEntry (it.saveName(), it.load);
  }
}

void KatePartPluginManager::loadAllPlugins ()
{
  for (KatePartPluginList::iterator it = m_pluginList.begin();
      it != m_pluginList.end(); ++it)
  {
    if (it->load)
    {
      loadPlugin(*it);
      enablePlugin(*it);
    }
  }
}

void KatePartPluginManager::unloadAllPlugins ()
{
  for (KatePartPluginList::iterator it = m_pluginList.begin();
       it != m_pluginList.end(); ++it)
  {
    if (it->plugin) {
      disablePlugin(*it);
      unloadPlugin(*it);
    }
  }
}

void KatePartPluginManager::loadPlugin (KatePartPluginInfo &item)
{
  if (item.plugin) return;

  // make sure all dependencies are loaded beforehand
  QStringList openDependencies = item.dependencies();
  if ( !openDependencies.empty() )
  {
    for (KatePartPluginList::iterator it = m_pluginList.begin();
      it != m_pluginList.end(); ++it)
    {
      if ( openDependencies.contains( it->saveName() ) )
      {
        loadPlugin( *it );
        openDependencies.removeAll( it->saveName() );
      }
    }
    Q_ASSERT( openDependencies.empty() );
  }

  item.plugin = KTextEditor::createPlugin (item.service(), this);
  Q_ASSERT(item.plugin);
  item.load = (item.plugin != 0);
}

void KatePartPluginManager::unloadPlugin (KatePartPluginInfo &item)
{
  if ( !item.plugin ) return;

  // make sure dependent plugins are unloaded beforehand
  for (KatePartPluginList::iterator it = m_pluginList.begin();
    it != m_pluginList.end(); ++it)
  {
    if ( !it->plugin ) continue;

    if ( it->dependencies().contains( item.saveName() ) )
    {
      unloadPlugin( *it );
    }
  }

  delete item.plugin;
  item.plugin = 0L;
  item.load = false;
}

void KatePartPluginManager::enablePlugin (KatePartPluginInfo &item)
{
  // plugin around at all?
  if (!item.plugin || !item.load)
    return;

  // register docs and views
  foreach (KTextEditor::Document *doc, KateGlobal::self()->documents()) {
    if (!doc)
      continue;

    foreach (KTextEditor::View *view, doc->views()) {
      if (!view)
        continue;

      KXMLGUIFactory *viewFactory = view->factory();
      if (viewFactory)
        viewFactory->removeClient(view);

      item.plugin->addView(view);

      if (viewFactory)
        viewFactory->addClient(view);
    }
  }
}

void KatePartPluginManager::disablePlugin (KatePartPluginInfo &item)
{
  // plugin around at all?
  if (!item.plugin || !item.load)
    return;

  // de-register docs and views
  foreach (KTextEditor::Document *doc, KateGlobal::self()->documents()) {
    if (!doc)
      continue;

    foreach (KTextEditor::View *view, doc->views()) {
      if (!view)
        continue;

      KXMLGUIFactory *viewFactory = view->factory();
      if (viewFactory)
        viewFactory->removeClient(view);

      item.plugin->removeView(view);

      if (viewFactory)
        viewFactory->addClient(view);
    }
  }
}

// kate: space-indent on; indent-width 2; replace-tabs on;
