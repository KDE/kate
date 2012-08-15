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
#include <ktexteditor/document.h>

#include <kaction.h>
#include <kactioncollection.h>
#include <klocale.h>
#include <kpluginfactory.h>
#include <kpluginloader.h>
#include <kaboutdata.h>

#include <KFileDialog>
#include <QDialog>
#include <QVBoxLayout>

K_PLUGIN_FACTORY(KateProjectPluginFactory, registerPlugin<KateProjectPlugin>();)
K_EXPORT_PLUGIN(KateProjectPluginFactory(KAboutData("project","kateprojectplugin",ki18n("Hello World"), "0.1", ki18n("Example kate plugin"))) )

KateProjectPluginView::KateProjectPluginView( KateProjectPlugin *plugin, Kate::MainWindow *mainWin )
    : Kate::PluginView( mainWin )
    , Kate::XMLGUIClient(KateProjectPluginFactory::componentData())
    , m_plugin (plugin)
{
  /**
   * add us to gui
   */
  mainWindow()->guiFactory()->addClient( this );

  /**
   * create toolview
   */
  m_toolView = mainWindow()->createToolView ("kateproject", Kate::MainWindow::Left, SmallIcon("project-open"), i18n("Projects"));

  /**
   * populate the toolview
   */
  m_toolBox = new QToolBox (m_toolView);

  /**
   * create views for all already existing projects
   */
  foreach (KateProject *project, m_plugin->projects())
    viewForProject (project);
  
  /**
   * connect to important signals, e.g. for auto project view creation
   */
  connect (m_plugin, SIGNAL(projectCreated (KateProject *)), this, SLOT(viewForProject (KateProject *)));
  connect (mainWindow(), SIGNAL(viewChanged ()), this, SLOT(slotViewChanged ()));
  connect (m_toolBox, SIGNAL(currentChanged (int)), this, SLOT(slotCurrentChanged (int)));
  
  /**
   * trigger once view change, to highlight right document
   */
  slotViewChanged ();
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

KateProjectView *KateProjectPluginView::viewForProject (KateProject *project)
{
  /**
   * needs valid project
   */
  Q_ASSERT (project);

  /**
   * existing view?
   */
  if (m_project2View.contains (project))
    return m_project2View.value (project);

  /**
   * create new view
   */
   KateProjectView *view = new KateProjectView (this, project);

   /**
    * attach to toolbox
    */
   m_toolBox->addItem (view, project->name());

   /**
    * remember and return it
    */
   m_project2View[project] = view;
   return view;
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

QString KateProjectPluginView::projectFileName () const
{
  QWidget *active = m_toolBox->currentWidget ();
  if (!active)
    return QString ();

  return static_cast<KateProjectView *> (active)->project()->fileName ();
}

QVariantMap KateProjectPluginView::projectMap () const
{
  QWidget *active = m_toolBox->currentWidget ();
  if (!active)
    return QVariantMap ();

  return static_cast<KateProjectView *> (active)->project()->projectMap ();
}

QStringList KateProjectPluginView::projectFiles () const
{
  KateProjectView *active = static_cast<KateProjectView *> (m_toolBox->currentWidget ());
  if (!active)
    return QStringList ();

  return active->project()->files ();
}

void KateProjectPluginView::slotViewChanged ()
{
  /**
   * get active view
   */
  KTextEditor::View *activeView = mainWindow()->activeView ();
  
  /**
   * update pointer, maybe disconnect before
   */
  if (m_activeTextEditorView)
    m_activeTextEditorView->document()->disconnect (this);
  m_activeTextEditorView = activeView;

  /**
   * no current active view, return
   */
  if (!m_activeTextEditorView)
    return;
  
  /**
   * connect to url changed, for auto load
   */
  connect (m_activeTextEditorView->document(), SIGNAL(documentUrlChanged (KTextEditor::Document *)), this, SLOT(slotDocumentUrlChanged (KTextEditor::Document *)));

  /**
   * trigger slot once
   */
  slotDocumentUrlChanged (m_activeTextEditorView->document());
}

void KateProjectPluginView::slotCurrentChanged (int)
{
  /**
   * project file name might have changed
   */
  emit projectFileNameChanged ();
}

void KateProjectPluginView::slotDocumentUrlChanged (KTextEditor::Document *document)
{
  /**
   * abort if empty url or no local path
   */
  if (document->url().isEmpty() || !document->url().isLocalFile())
    return;
  
  /**
   * search matching project
   */
  KateProject *project = m_plugin->projectForUrl (document->url());
  if (!project)
    return;
  
  /**
   * get active project view and switch it, if it is for a different project
   */
  KateProjectView *active = static_cast<KateProjectView *> (m_toolBox->currentWidget ());
  if (active != m_project2View.value (project))
      m_toolBox->setCurrentWidget (m_project2View.value (project));
  
  /**
   * get local filename and then select it
   */
  active->selectFile (document->url().toLocalFile ());
}

// kate: space-indent on; indent-width 2; replace-tabs on;
