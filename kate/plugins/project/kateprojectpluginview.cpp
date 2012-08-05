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

#include "kateprojectpluginview.h"
#include "kateprojectpluginview.moc"

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

K_PLUGIN_FACTORY(KateProjectPluginFactory, registerPlugin<KateProjectPlugin>();)
K_EXPORT_PLUGIN(KateProjectPluginFactory(KAboutData("kateproject","kateproject",ki18n("Hello World"), "0.1", ki18n("Example kate plugin"))) )

KateProjectPluginView::KateProjectPluginView( KateProjectPlugin *plugin, Kate::MainWindow *mainWin )
    : Kate::PluginView( mainWin ),
      Kate::XMLGUIClient(KateProjectPluginFactory::componentData())
      , m_plugin (plugin)
{
  KAction *a = actionCollection()->addAction( "open_project" );
  a->setText( i18n("Open Project") );
  connect( a, SIGNAL(triggered(bool)), this, SLOT(slotInsertHello()) );

  /**
   * add us to gui
   */
  mainWindow()->guiFactory()->addClient( this );

  /**
   * create toolview
   */
  m_toolView = mainWindow()->createToolView ("kateproject", Kate::MainWindow::Left, SmallIcon("project-open"), i18n("Projects"));
}

KateProjectPluginView::~KateProjectPluginView()
{
  /**
   * cu toolview
   */
  delete m_toolView;
  
  /**
   * cu gui client
   */
  mainWindow()->guiFactory()->removeClient( this );
}

void KateProjectPluginView::slotInsertHello()
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

void KateProjectPluginView::readSessionConfig( KConfigBase* config, const QString& groupPrefix )
{
  // If you have session-dependant settings, load them here.
  // If you have application wide settings, you have to read your own KConfig,
  // see the Kate::Plugin docs for more information.
  Q_UNUSED( config );
  Q_UNUSED( groupPrefix );
}

void KateProjectPluginView::writeSessionConfig( KConfigBase* config, const QString& groupPrefix )
{
  // If you have session-dependant settings, save them here.
  // If you have application wide settings, you have to create your own KConfig,
  // see the Kate::Plugin docs for more information.
  Q_UNUSED( config );
  Q_UNUSED( groupPrefix );
}

// kate: space-indent on; indent-width 2; replace-tabs on;

