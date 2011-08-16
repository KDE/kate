/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2010 Christoph Cullmann <cullmann@kde.org>
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

#include "plugin_katehelloworld.h"
#include "plugin_katehelloworld.moc"

#include <kate/application.h>
#include <ktexteditor/view.h>

#include <kaction.h>
#include <kactioncollection.h>
#include <klocale.h>
#include <kpluginfactory.h>
#include <kpluginloader.h>
#include <kaboutdata.h>

K_PLUGIN_FACTORY(KatePluginHelloWorldFactory, registerPlugin<KatePluginHelloWorld>();)
K_EXPORT_PLUGIN(KatePluginHelloWorldFactory(KAboutData("katehelloworld","katehelloworld",ki18n("Hello World"), "0.1", ki18n("Example kate plugin"))) )

KatePluginHelloWorld::KatePluginHelloWorld( QObject* parent, const QList<QVariant>& )
    : Kate::Plugin( (Kate::Application*)parent, "kate-hello-world-plugin" )
{
}

KatePluginHelloWorld::~KatePluginHelloWorld()
{
}

Kate::PluginView *KatePluginHelloWorld::createView( Kate::MainWindow *mainWindow )
{
  return new KatePluginHelloWorldView( mainWindow );
}


KatePluginHelloWorldView::KatePluginHelloWorldView( Kate::MainWindow *mainWin )
    : Kate::PluginView( mainWin ),
    Kate::XMLGUIClient(KatePluginHelloWorldFactory::componentData())
{
  KAction *a = actionCollection()->addAction( "edit_insert_helloworld" );
  a->setText( i18n("Insert Hello World") );
  connect( a, SIGNAL(triggered(bool)), this, SLOT(slotInsertHello()) );

  mainWindow()->guiFactory()->addClient( this );
}

KatePluginHelloWorldView::~KatePluginHelloWorldView()
{
  mainWindow()->guiFactory()->removeClient( this );
}

void KatePluginHelloWorldView::slotInsertHello()
{
  if (!mainWindow()) {
    return;
  }

  KTextEditor::View *kv = mainWindow()->activeView();

  if (kv) {
    kv->insertText( "Hello World" );
  }
}

void KatePluginHelloWorldView::readSessionConfig( KConfigBase* config, const QString& groupPrefix )
{
  // If you have session-dependant settings, load them here.
  // If you have application wide settings, you have to read your own KConfig,
  // see the Kate::Plugin docs for more information.
  Q_UNUSED( config );
  Q_UNUSED( groupPrefix );
}

void KatePluginHelloWorldView::writeSessionConfig( KConfigBase* config, const QString& groupPrefix )
{
  // If you have session-dependant settings, save them here.
  // If you have application wide settings, you have to create your own KConfig,
  // see the Kate::Plugin docs for more information.
  Q_UNUSED( config );
  Q_UNUSED( groupPrefix );
}

// kate: space-indent on; indent-width 2; replace-tabs on;

