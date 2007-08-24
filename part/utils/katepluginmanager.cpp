/* This file is part of the KDE project
   Copyright 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>
   Copyright 2007 Dominik Haumann <dhaumann kde org>

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

#include "kateglobal.h"

#include <ktexteditor/plugin.h>
#include <ktexteditor/document.h>
#include <ktexteditor/view.h>
#include <kconfig.h>

//Added by qt3to4:
#include <QList>
#include <kservicetypetrader.h>
#include <kdebug.h>
#include <QFile>

QString KatePluginInfo::saveName() const
{
  QString saveName = service->property("X-KDE-PluginInfo-Name").toString();

  if (saveName.isEmpty())
    saveName = service->library();
  return saveName;
}

KatePluginManager::KatePluginManager()
  : QObject(),
    m_config(new KConfig("katepartpluginsrc", KConfig::NoGlobals))
{
  setupPluginList ();
  loadConfig ();
}

KatePluginManager::~KatePluginManager()
{
  writeConfig();
  // than unload the plugins
  unloadAllPlugins ();
  m_config->sync();
  delete m_config;
  m_config = 0;
}

KatePluginManager *KatePluginManager::self()
{
  return KateGlobal::self()->pluginManager ();
}

void KatePluginManager::setupPluginList ()
{
  // NOTE: adapt the interval each minor KDE version
  KService::List traderList = KServiceTypeTrader::self()->
      query("KTextEditor/Plugin", "([X-KDE-Version] >= 4.0) and ([X-KDE-Version] <= 4.0)");

  foreach(const KService::Ptr &ptr, traderList)
  {
    KatePluginInfo info;

    info.load = false;
    info.service = ptr;
    info.plugin = 0L;

    m_pluginList.push_back (info);
  }
}

void KatePluginManager::addDocument(KTextEditor::Document *doc)
{
  kDebug() << doc << endl;
  for (KatePluginList::iterator it = m_pluginList.begin();
      it != m_pluginList.end(); ++it)
  {
    if (it->load) {
      it->plugin->addDocument(doc);
    }
  }
}

void KatePluginManager::removeDocument(KTextEditor::Document *doc)
{
  kDebug() << doc << endl;
  for (KatePluginList::iterator it = m_pluginList.begin();
      it != m_pluginList.end(); ++it)
  {
    if (it->load) {
      it->plugin->removeDocument(doc);
    }
  }
}

void KatePluginManager::addView(KTextEditor::View *view)
{
  kDebug() << view << endl;
  for (KatePluginList::iterator it = m_pluginList.begin();
      it != m_pluginList.end(); ++it)
  {
    if (it->load) {
      it->plugin->addView(view);
    }
  }
}

void KatePluginManager::removeView(KTextEditor::View *view)
{
  kDebug() << view << endl;
  for (KatePluginList::iterator it = m_pluginList.begin();
      it != m_pluginList.end(); ++it)
  {
    if (it->load) {
      it->plugin->removeView(view);
    }
  }
}

void KatePluginManager::loadConfig ()
{
  // first: unload the plugins
  unloadAllPlugins ();

  KConfigGroup cg = KConfigGroup(m_config, "Kate Part Plugins");

  // disable all plugin if no config...
  foreach (const KatePluginInfo &plugin, m_pluginList)
    plugin.load = cg.readEntry (plugin.service->library(), false)
               || cg.readEntry (plugin.service->property("X-KDE-PluginInfo-Name").toString(), false);

  loadAllPlugins();
}

void KatePluginManager::writeConfig()
{
  KConfigGroup cg = KConfigGroup( m_config, "Kate Part Plugins" );
  foreach(const KatePluginInfo &it, m_pluginList)
  {
    cg.writeEntry (it.saveName(), it.load);
  }
}

void KatePluginManager::loadAllPlugins ()
{
  for (KatePluginList::iterator it = m_pluginList.begin();
      it != m_pluginList.end(); ++it)
  {
    if (it->load)
    {
      loadPlugin(*it);
      enablePlugin(*it);
    }
  }
}

void KatePluginManager::unloadAllPlugins ()
{
  for (KatePluginList::iterator it = m_pluginList.begin();
       it != m_pluginList.end(); ++it)
  {
    if (it->plugin) {
      disablePlugin(*it);
      unloadPlugin(*it);
    }
  }
}

void KatePluginManager::loadPlugin (KatePluginInfo &item)
{
  if (item.plugin) return;

  item.plugin = createPlugin (item.service);
  if (item.plugin) {
    item.load = true;
    item.plugin->readConfig(m_config);
  }
}

void KatePluginManager::unloadPlugin (KatePluginInfo &item)
{
  if (item.plugin) {
    item.plugin->writeConfig(m_config);
//    delete item.plugin;
    item.plugin = 0L;
  }
  item.load = false;
}

void KatePluginManager::enablePlugin (KatePluginInfo &item)
{
  // plugin around at all?
  if (!item.plugin || !item.load)
    return;

  // register docs and views
  foreach (KTextEditor::Document *doc, KateGlobal::self()->documents()) {
    item.plugin->addDocument(doc);
    foreach (KTextEditor::View *view, doc->views())
      item.plugin->addView(view);
  }
}

void KatePluginManager::disablePlugin (KatePluginInfo &item)
{
  // plugin around at all?
  if (!item.plugin || !item.load)
    return;

  // de-register docs and views
  foreach (KTextEditor::Document *doc, KateGlobal::self()->documents()) {
    foreach (KTextEditor::View *view, doc->views())
      item.plugin->removeView(view);
    item.plugin->removeDocument(doc);
  }
}

KTextEditor::Plugin *KatePluginManager::createPlugin(KService::Ptr service)
{
  return KTextEditor::createPlugin (QFile::encodeName(service->library()), 0);
}

// kate: space-indent on; indent-width 2; replace-tabs on;
