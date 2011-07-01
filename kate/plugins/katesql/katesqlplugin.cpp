/*
   Copyright (C) 2010  Marco Mentasti  <marcomentasti@gmail.com>

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

#include "katesqlplugin.h"
#include "katesqlconfigpage.h"
#include "katesqlview.h"

#include <kate/plugin.h>
#include <kate/mainwindow.h>
#include <kate/documentmanager.h>
#include <ktexteditor/document.h>

#include <kpluginloader.h>
#include <kaboutdata.h>
#include <kdebug.h>
#include <klocale.h>

K_PLUGIN_FACTORY_DEFINITION(KateSQLFactory, registerPlugin<KateSQLPlugin>();)
K_EXPORT_PLUGIN(KateSQLFactory(KAboutData("katesql", "katesqlplugin",
                                          ki18n("SQL Plugin"), "0.3", ki18n("Execute query on SQL databases"), KAboutData::License_LGPL_V2)))

//BEGIN KateSQLPLugin
KateSQLPlugin::KateSQLPlugin(QObject *parent, const QList<QVariant>&)
: Kate::Plugin ((Kate::Application*)parent, "katesql")
{
}


KateSQLPlugin::~KateSQLPlugin()
{
}


Kate::PluginView *KateSQLPlugin::createView (Kate::MainWindow *mainWindow)
{
  KateSQLView *view = new KateSQLView(mainWindow);

  connect(this, SIGNAL(globalSettingsChanged()), view, SLOT(slotGlobalSettingsChanged()));

  return view;
}


Kate::PluginConfigPage* KateSQLPlugin::configPage(uint number, QWidget *parent, const char *name)
{
  Q_UNUSED(name)

  if (number != 0)
    return 0;

  KateSQLConfigPage *page = new KateSQLConfigPage(parent);

  connect(page, SIGNAL(settingsChanged()), this, SIGNAL(globalSettingsChanged()));

  return page;
}


QString KateSQLPlugin::configPageName (uint number) const
{
  if (number != 0) return QString();
    return i18nc("@title", "SQL");
}


QString KateSQLPlugin::configPageFullName (uint number) const
{
  if (number != 0) return QString();
    return i18nc("@title:window", "SQL Plugin Settings");
}


KIcon KateSQLPlugin::configPageIcon (uint number) const
{
  if (number != 0) return KIcon();
  return KIcon("server-database");
}

//END KateSQLPlugin

