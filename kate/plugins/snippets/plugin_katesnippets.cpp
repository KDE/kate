/* This file is part of the KDE project
   Copyright (C) 2004 Stephan Möres <Erdling@gmx.net>
   Copyright (C) 2008 Jakob Petsovits <jpetso@gmx.at>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "plugin_katesnippets.h"
#include "plugin_katesnippets.moc"

#include "katesnippetswidget.h"

#include <kaction.h>
#include <klocale.h>
#include <kpluginfactory.h>
#include <kpluginloader.h>
#include <kaboutdata.h>
#include <kiconloader.h>

// let the world know ...
K_PLUGIN_FACTORY(KateSnippetsFactory, registerPlugin<KatePluginSnippets>();)
K_EXPORT_PLUGIN(KateSnippetsFactory(KAboutData("katesnippets","katesnippets",ki18n("Snippets"), "0.1", ki18n("Insert code snippets int the document"), KAboutData::License_LGPL_V2)) )


KatePluginSnippets::KatePluginSnippets( QObject* parent, const QList<QVariant>& )
    : Kate::Plugin ( (Kate::Application*)parent, "KatePluginSnippets" ) {}

KatePluginSnippets::~KatePluginSnippets() {}

Kate::PluginView *KatePluginSnippets::createView( Kate::MainWindow *mainWindow )
{
  return new KatePluginSnippetsView( mainWindow );
}


KatePluginSnippetsView::KatePluginSnippetsView( Kate::MainWindow *mainWin )
    : Kate::PluginView( mainWin )
{
  setComponentData( KateSnippetsFactory::componentData() );
  setXMLFile( "plugins/katesnippets/plugin_katesnippets.rc" );
  m_dock = mainWindow()->createToolView(
    "kate_plugin_snippets",
    Kate::MainWindow::Left,
    SmallIcon("insert-text"),
    i18n("Snippets")
  );
  m_snippetsWidget = new KateSnippetsWidget( mainWindow(), m_dock, "snippetswidget" );

  // write the settings when the user clicks Save in the widget
  connect( m_snippetsWidget, SIGNAL( saveRequested() ), this, SLOT( writeConfig() ) );

  mainWindow()->guiFactory()->addClient( this );

  m_config = new KConfig( "katesnippetspluginrc" );
  readConfig();
}

KatePluginSnippetsView::~KatePluginSnippetsView()
{
  writeConfig();
  mainWindow()->guiFactory()->removeClient( this );
  delete m_dock; // includes m_snippetsWidget
  delete m_config;
}

void KatePluginSnippetsView::readConfig()
{
  kateSnippetsWidget()->readConfig( m_config, "Snippets" );
}

void KatePluginSnippetsView::writeConfig()
{
  kateSnippetsWidget()->writeConfig( m_config, "Snippets" );
}

// kate: space-indent on; indent-width 2; replace-tabs on;
