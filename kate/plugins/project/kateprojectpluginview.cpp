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
#include <ktexteditor/codecompletioninterface.h>

#include <kaction.h>
#include <kactioncollection.h>
#include <klocale.h>
#include <kpluginfactory.h>
#include <kpluginloader.h>
#include <kaboutdata.h>

#include <KFileDialog>
#include <QDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>

K_PLUGIN_FACTORY(KateProjectPluginFactory, registerPlugin<KateProjectPlugin>();)
K_EXPORT_PLUGIN(KateProjectPluginFactory(KAboutData("project","kateproject",ki18n("Hello World"), "0.1", ki18n("Example kate plugin"))) )

KateProjectPluginView::KateProjectPluginView( KateProjectPlugin *plugin, Kate::MainWindow *mainWin )
    : Kate::PluginView( mainWin )
    , Kate::XMLGUIClient(KateProjectPluginFactory::componentData())
    , m_plugin (plugin)
{
  /**
   * create toolviews
   */
  m_toolView = mainWindow()->createToolView ("kateproject", Kate::MainWindow::Left, SmallIcon("project-open"), i18n("Projects"));
  m_toolInfoView = mainWindow()->createToolView ("kateprojectinfo", Kate::MainWindow::Bottom, SmallIcon("view-choose"), i18n("Current Project"));

  /**
   * create the combo + buttons for the toolViews + stacked widgets
   */
  m_projectsCombo = new QComboBox (m_toolView);
  m_reloadButton = new QToolButton (m_toolView);
  m_reloadButton->setIcon (SmallIcon("view-refresh"));
  QHBoxLayout *layout = new QHBoxLayout ();
  layout->setSpacing (0);
  layout->addWidget (m_projectsCombo);
  layout->addWidget (m_reloadButton);
  m_toolView->layout()->addItem (layout);

  m_stackedProjectViews = new QStackedWidget (m_toolView);
  m_stackedProjectInfoViews = new QStackedWidget (m_toolInfoView);

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
  connect (m_projectsCombo, SIGNAL(currentIndexChanged (int)), this, SLOT(slotCurrentChanged (int)));
  connect (mainWindow(), SIGNAL(viewCreated (KTextEditor::View *)), this, SLOT(slotViewCreated (KTextEditor::View *)));
  connect (m_reloadButton, SIGNAL(clicked (bool)), this, SLOT(slotProjectReload ()));

  /**
   * connect for all already existing views
   */
  foreach (KTextEditor::View *view, mainWindow()->views())
    slotViewCreated (view);

  /**
   * trigger once view change, to highlight right document
   */
  slotViewChanged ();

  /**
   * back + forward
   */
  actionCollection()->addAction (KStandardAction::Back, "projects_prev_project", this, SLOT(slotProjectPrev()))->setShortcut (Qt::CTRL | Qt::ALT | Qt::Key_Left);
  actionCollection()->addAction (KStandardAction::Forward, "projects_next_project", this, SLOT(slotProjectNext()))->setShortcut (Qt::CTRL | Qt::ALT | Qt::Key_Right);

  /**
   * add us to gui
   */
  mainWindow()->guiFactory()->addClient( this );
}

KateProjectPluginView::~KateProjectPluginView()
{
  /**
   * cleanup for all views
   */
  foreach (QObject *view, m_textViews) {
    KTextEditor::CodeCompletionInterface *cci = qobject_cast<KTextEditor::CodeCompletionInterface *>(view);
    if (cci)
      cci->unregisterCompletionModel (m_plugin->completion());
  }

  /**
   * cu toolviews
   */
  delete m_toolView;
  delete m_toolInfoView;

  /**
   * cu gui client
   */
  mainWindow()->guiFactory()->removeClient( this );
}

QPair<KateProjectView *,KateProjectInfoView *> KateProjectPluginView::viewForProject (KateProject *project)
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
   * create new views
   */
   KateProjectView *view = new KateProjectView (this, project);
   KateProjectInfoView *infoView = new KateProjectInfoView (this, project);

   /**
    * attach to toolboxes
    * first the views, then the combo, that triggers signals
    */
   m_stackedProjectViews->addWidget (view);
   m_stackedProjectInfoViews->addWidget (infoView);
   m_projectsCombo->addItem (SmallIcon("project-open"), project->name(), project->fileName());

   /**
    * remember and return it
    */
   return (m_project2View[project] = QPair<KateProjectView *,KateProjectInfoView *> (view, infoView));
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
  QWidget *active = m_stackedProjectViews->currentWidget ();
  if (!active)
    return QString ();

  return static_cast<KateProjectView *> (active)->project()->fileName ();
}

QString KateProjectPluginView::projectName () const
{
  QWidget *active = m_stackedProjectViews->currentWidget ();
  if (!active)
    return QString ();

  return static_cast<KateProjectView *> (active)->project()->name ();
}

QString KateProjectPluginView::projectBaseDir () const
{
  QWidget *active = m_stackedProjectViews->currentWidget ();
  if (!active)
    return QString ();

  return static_cast<KateProjectView *> (active)->project()->baseDir ();
}

QVariantMap KateProjectPluginView::projectMap () const
{
  QWidget *active = m_stackedProjectViews->currentWidget ();
  if (!active)
    return QVariantMap ();

  return static_cast<KateProjectView *> (active)->project()->projectMap ();
}

QStringList KateProjectPluginView::projectFiles () const
{
  KateProjectView *active = static_cast<KateProjectView *> (m_stackedProjectViews->currentWidget ());
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

void KateProjectPluginView::slotCurrentChanged (int index)
{
  /**
   * trigger change of stacked widgets
   */
  m_stackedProjectViews->setCurrentIndex (index);
  m_stackedProjectInfoViews->setCurrentIndex (index);

  /**
   * open currently selected document
   */
  if (QWidget *current = m_stackedProjectViews->currentWidget ())
    static_cast<KateProjectView *> (current)->openSelectedDocument ();

  /**
   * project file name might have changed
   */
  emit projectFileNameChanged ();
  emit projectMapChanged ();
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
   * select the file FIRST
   */
  m_project2View.value (project).first->selectFile (document->url().toLocalFile ());

  /**
   * get active project view and switch it, if it is for a different project
   * do this AFTER file selection
   */
  KateProjectView *active = static_cast<KateProjectView *> (m_stackedProjectViews->currentWidget ());
  if (active != m_project2View.value (project).first) {
    int index = m_projectsCombo->findData (project->fileName());
    if (index >= 0)
      m_projectsCombo->setCurrentIndex (index);
  }
}

void KateProjectPluginView::slotViewCreated (KTextEditor::View *view)
{
  /**
   * connect to destroyed
   */
  connect (view, SIGNAL(destroyed (QObject *)), this, SLOT(slotViewDestroyed (QObject *)));

  /**
   * add completion model if possible
   */
  KTextEditor::CodeCompletionInterface *cci = qobject_cast<KTextEditor::CodeCompletionInterface *>(view);
  if (cci)
    cci->registerCompletionModel (m_plugin->completion());

  /**
   * remember for this view we need to cleanup!
   */
  m_textViews.insert (view);
}

void KateProjectPluginView::slotViewDestroyed (QObject *view)
{
  /**
   * remove remembered views for which we need to cleanup on exit!
   */
  m_textViews.remove (view);
}

void KateProjectPluginView::slotProjectPrev ()
{
  if (!m_projectsCombo->count())
    return;

  if (m_projectsCombo->currentIndex () == 0)
    m_projectsCombo->setCurrentIndex (m_projectsCombo->count()-1);
  else
    m_projectsCombo->setCurrentIndex (m_projectsCombo->currentIndex () - 1);
}

void KateProjectPluginView::slotProjectNext ()
{
  if (!m_projectsCombo->count())
    return;

  if (m_projectsCombo->currentIndex () + 1 == m_projectsCombo->count())
    m_projectsCombo->setCurrentIndex (0);
  else
    m_projectsCombo->setCurrentIndex (m_projectsCombo->currentIndex () + 1);
}

void KateProjectPluginView::slotProjectReload ()
{
  /**
   * force reload if any active project
   */
  if (QWidget *current = m_stackedProjectViews->currentWidget ())
    static_cast<KateProjectView *> (current)->project()->reload (true);
}

// kate: space-indent on; indent-width 2; replace-tabs on;
