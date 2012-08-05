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

#include "plugin_kateproject.h"
#include "plugin_kateproject.moc"

#include "kateproject.h"

#include <kate/application.h>
#include <ktexteditor/view.h>

#include <kaction.h>
#include <kactioncollection.h>
#include <klocale.h>
#include <kpluginfactory.h>
#include <kpluginloader.h>
#include <kaboutdata.h>

#include <KFileDialog>
#include <QDialog>
#include <QTreeView>
 
K_PLUGIN_FACTORY(KatePluginProjectFactory, registerPlugin<KatePluginProject>();)
K_EXPORT_PLUGIN(KatePluginProjectFactory(KAboutData("kateproject","kateproject",ki18n("Hello World"), "0.1", ki18n("Example kate plugin"))) )

KatePluginProject::KatePluginProject( QObject* parent, const QList<QVariant>& )
    : Kate::Plugin( (Kate::Application*)parent, "kate-hello-world-plugin" )
{
}

KatePluginProject::~KatePluginProject()
{
}

Kate::PluginView *KatePluginProject::createView( Kate::MainWindow *mainWindow )
{
  return new KatePluginProjectView( mainWindow );
}


KatePluginProjectView::KatePluginProjectView( Kate::MainWindow *mainWin )
    : Kate::PluginView( mainWin ),
    Kate::XMLGUIClient(KatePluginProjectFactory::componentData())
{
  KAction *a = actionCollection()->addAction( "open_project" );
  a->setText( i18n("Open Project") );
  connect( a, SIGNAL(triggered(bool)), this, SLOT(slotInsertHello()) );

  mainWindow()->guiFactory()->addClient( this );
}

KatePluginProjectView::~KatePluginProjectView()
{
  mainWindow()->guiFactory()->removeClient( this );
}

void KatePluginProjectView::slotInsertHello()
{
  QString projectFile = KFileDialog::getOpenFileName ();
  
  KateProject *project = new KateProject ();
  project->load (projectFile);
  
  /**
   * show the model
   */
   QDialog *test = new QDialog ();
   QTreeView *tree = new QTreeView (test);
   tree->setModel (project->model ());
   test->show();
   test->raise();
   test->activateWindow();
}

void KatePluginProjectView::readSessionConfig( KConfigBase* config, const QString& groupPrefix )
{
  // If you have session-dependant settings, load them here.
  // If you have application wide settings, you have to read your own KConfig,
  // see the Kate::Plugin docs for more information.
  Q_UNUSED( config );
  Q_UNUSED( groupPrefix );
}

void KatePluginProjectView::writeSessionConfig( KConfigBase* config, const QString& groupPrefix )
{
  // If you have session-dependant settings, save them here.
  // If you have application wide settings, you have to create your own KConfig,
  // see the Kate::Plugin docs for more information.
  Q_UNUSED( config );
  Q_UNUSED( groupPrefix );
}

// kate: space-indent on; indent-width 2; replace-tabs on;

