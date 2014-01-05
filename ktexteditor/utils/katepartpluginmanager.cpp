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

#include "config.h"

#include "katepartpluginmanager.h"
#include "katepartpluginmanager.moc"

#include "kateglobal.h"
#include "katepartdebug.h"

#include <ktexteditor/plugin.h>
#include <ktexteditor/document.h>
#include <ktexteditor/view.h>
#include <KConfig>
#include <KConfigGroup>
#include <KXMLGUIFactory>
#include <KPluginInfo>

#include <KServiceTypeTrader>

//BEGIN KatePartPluginInfo

KatePartPluginInfo::KatePartPluginInfo(const KService::Ptr &service)
  : m_service(service)
{
  // FIXME: this should do the trick, but can fail terribly if file is not found or the lib has no info
  KPluginLoader loader(service->library());
  QVariantList vars = QVariantList() << loader.metaData().toVariantMap();
  m_pluginInfo = KPluginInfo(vars, loader.fileName());
}

QString KatePartPluginInfo::saveName() const
{
  QString saveName = m_pluginInfo.pluginName();
  if (saveName.isEmpty())
    saveName = m_service->library();
  return saveName;
}

void KatePartPluginInfo::setLoad(bool load)
{
  m_load = load;
  m_pluginInfo.setPluginEnabled(load);
}

KPluginInfo KatePartPluginInfo::getKPluginInfo() const
{
  return m_pluginInfo;
}

const KService::Ptr &KatePartPluginInfo::service() const
{
  return m_service;
}

QStringList KatePartPluginInfo::dependencies() const
{
  return m_pluginInfo.dependencies();
}

bool KatePartPluginInfo::isEnabledByDefault() const
{
  return m_pluginInfo.isPluginEnabledByDefault();
}

//END KatePartPluginInfo

//BEGIN KatePluginManager

KatePartPluginManager::KatePartPluginManager()
  : QObject(),
    m_config(new KConfig(QLatin1String("katepartpluginsrc"), KConfig::NoGlobals))
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

#define MAKE_VERSION( a,b,c ) (((a) << 16) | ((b) << 8) | (c))

static const unsigned KPPMVersion = MAKE_VERSION(KateVersionMajor,
                                                 KateVersionMinor,
                                                 KateVersionMicro);

static const unsigned MinPluginVersion = MAKE_VERSION(4, 0, 0);

void KatePartPluginManager::setupPluginList ()
{
  KService::List traderList = KServiceTypeTrader::self()->
      query(QLatin1String("KTextEditor/Plugin"));

  foreach(const KService::Ptr &ptr, traderList)
  {
    QVariant version = ptr->property(QLatin1String("X-KDE-Version"), QVariant::String);
    QStringList numbers = qvariant_cast<QString>(version).split(QLatin1Char('.'));
    unsigned int kdeVersion = MAKE_VERSION(numbers.value(0).toUInt(),
                                           numbers.value(1).toUInt(),
                                           numbers.value(2).toUInt());

    if (MinPluginVersion <= kdeVersion && kdeVersion <= KPPMVersion)
    {
      KatePartPluginInfo info(ptr);

      info.setLoad(false);
      info.plugin = 0L;

      m_pluginList.push_back (info);
    }
  }
}

void KatePartPluginManager::addDocument(KTextEditor::Document *doc)
{
  //qCDebug(LOG_PART) << doc;
  for (KatePartPluginList::iterator it = m_pluginList.begin();
      it != m_pluginList.end(); ++it)
  {
    if (it->isLoaded()) {
      it->plugin->addDocument(doc);
    }
  }
}

void KatePartPluginManager::removeDocument(KTextEditor::Document *doc)
{
  //qCDebug(LOG_PART) << doc;
  for (KatePartPluginList::iterator it = m_pluginList.begin();
      it != m_pluginList.end(); ++it)
  {
    if (it->isLoaded()) {
      it->plugin->removeDocument(doc);
    }
  }
}

void KatePartPluginManager::addView(KTextEditor::View *view)
{
  //qCDebug(LOG_PART) << view;
  for (KatePartPluginList::iterator it = m_pluginList.begin();
      it != m_pluginList.end(); ++it)
  {
    if (it->isLoaded()) {
      it->plugin->addView(view);
    }
  }
}

void KatePartPluginManager::removeView(KTextEditor::View *view)
{
  //qCDebug(LOG_PART) << view;
  for (KatePartPluginList::iterator it = m_pluginList.begin();
      it != m_pluginList.end(); ++it)
  {
    if (it->isLoaded()) {
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
  QListIterator<KatePartPluginInfo> it(m_pluginList);

  while (it.hasNext()) {
    KatePartPluginInfo plugin = it.next();
    bool enabledByDefault = plugin.isEnabledByDefault();
    plugin.setLoad(cg.readEntry(plugin.saveName(), enabledByDefault));
  }

  loadAllPlugins();
}

void KatePartPluginManager::writeConfig()
{
  KConfigGroup cg = KConfigGroup( m_config, "Kate Part Plugins" );
  foreach(const KatePartPluginInfo &it, m_pluginList)
  {
    cg.writeEntry (it.saveName(), it.isLoaded());
  }
}

void KatePartPluginManager::loadAllPlugins ()
{
  for (KatePartPluginList::iterator it = m_pluginList.begin();
      it != m_pluginList.end(); ++it)
  {
    if (it->isLoaded())
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

  // try to load, else reset load flag!
  QString error;
  item.plugin = item.service()->createInstance<KTextEditor::Plugin>(this, QVariantList(), &error);
  if ( !item.plugin )
    qCWarning(LOG_PART) << "failed to load plugin" << item.service()->name() << ":" << error;
  item.setLoad(item.plugin != 0);
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
  item.setLoad(false);
}

void KatePartPluginManager::enablePlugin (KatePartPluginInfo &item)
{
  // plugin around at all?
  if (!item.plugin || !item.isLoaded())
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
  if (!item.plugin || !item.isLoaded())
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
//END KatePluginManager

// kate: space-indent on; indent-width 2; replace-tabs on;
