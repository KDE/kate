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

#include "kateprojectplugin.h"
#include "kateprojectplugin.moc"

#include "kateproject.h"
#include "kateprojectpluginview.h"

#include <kate/application.h>
#include <kate/documentmanager.h>
#include <ktexteditor/document.h>

#include <QFileInfo>
#include <QTime>

#include "config.h"

#ifdef HAVE_CTERMID
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#endif

KateProjectPlugin::KateProjectPlugin (QObject* parent, const QList<QVariant>&)
  : Kate::Plugin ((Kate::Application*)parent)
  , m_completion (this)
{
  /**
   * register some data types
   */
  qRegisterMetaType<KateProjectSharedQStandardItem>("KateProjectSharedQStandardItem");
  qRegisterMetaType<KateProjectSharedQMapStringItem>("KateProjectSharedQMapStringItem");
  qRegisterMetaType<KateProjectSharedProjectIndex>("KateProjectSharedProjectIndex");
 
  /**
   * connect to important signals, e.g. for auto project loading
   */
  connect (application()->documentManager(), SIGNAL(documentCreated (KTextEditor::Document *)), this, SLOT(slotDocumentCreated (KTextEditor::Document *)));
  connect (&m_fileWatcher, SIGNAL(directoryChanged (const QString &)), this, SLOT(slotDirectoryChanged (const QString &)));
  
#ifdef HAVE_CTERMID
  /**
   * open project for our current working directory, if this kate has a terminal
   * http://stackoverflow.com/questions/1312922/detect-if-stdin-is-a-terminal-or-pipe-in-c-c-qt
   */
  char tty[L_ctermid+1] = {0};
  ctermid (tty);
  int fd = ::open(tty, O_RDONLY);
  if (fd >= 0) {
    /**
     * open project for working dir!
     */
    projectForDir (QDir::current ());
    
    /**
     * close again
     */
    ::close (fd);
  }
#endif
  
  /**
   * connect for all already existing documents
   */
  foreach (KTextEditor::Document *document, application()->documentManager()->documents())
    slotDocumentCreated (document);
}

KateProjectPlugin::~KateProjectPlugin()
{
  /**
   * cleanup open projects
   */
  foreach (KateProject *project, m_fileName2Project) {
    /**
     * remove path
     */
    m_fileWatcher.removePath (QFileInfo (project->fileName()).canonicalPath());

    /**
     * let events still be handled!
     */
    delete project;
  }

  /**
   * cleanup list
   */
  m_fileName2Project.clear ();
}

Kate::PluginView *KateProjectPlugin::createView( Kate::MainWindow *mainWindow )
{
  return new KateProjectPluginView ( this, mainWindow );
}

KateProject *KateProjectPlugin::projectForFileName (const QString &fileName)
{
  /**
   * canonicalize file path
   */
  QString canonicalFilePath = QFileInfo (fileName).canonicalFilePath ();

  /**
   * abort if empty
   */
  if (canonicalFilePath.isEmpty())
    return 0;

  /**
   * first: lookup in existing projects
   */
  if (m_fileName2Project.contains (canonicalFilePath))
    return m_fileName2Project.value (canonicalFilePath);

  /**
   * else: try to load or fail
   */
  KateProject *project = new KateProject ();
  if (!project->load (canonicalFilePath)) {
    delete project;
    return 0;
  }

  /**
   * remember project and emit & return it
   */
  m_fileName2Project[canonicalFilePath] = project;
  m_fileWatcher.addPath (QFileInfo (canonicalFilePath).canonicalPath());
  emit projectCreated (project);
  return project;
}

KateProject *KateProjectPlugin::projectForDir (QDir dir)
{
  /**
   * search projects upwards
   * with recursion guard
   */
  QSet<QString> seenDirectories;
  while (!seenDirectories.contains (dir.absolutePath ())) {
    /**
     * fill recursion guard
     */
    seenDirectories.insert (dir.absolutePath ());

    /**
     * check for project and load it if found
     */
    if (dir.exists (".kateproject"))
      return projectForFileName (dir.absolutePath () + "/.kateproject");

    /**
     * else: cd up, if possible or abort
     */
    if (!dir.cdUp())
      break;
  }

  /**
   * nothing there
   */
  return 0;
}

KateProject *KateProjectPlugin::projectForUrl (const KUrl &url)
{
  /**
   * abort if empty url or no local path
   */
  if (url.isEmpty() || !url.isLocalFile())
    return 0;

  /**
   * else get local filename and then the dir for it
   * pass this to right search function
   */
  return projectForDir (QFileInfo(url.toLocalFile ()).absoluteDir ());
}

void KateProjectPlugin::slotDocumentCreated (KTextEditor::Document *document)
{
  /**
   * connect to url changed, for auto load and destroyed
   */
  connect (document, SIGNAL(documentUrlChanged (KTextEditor::Document *)), this, SLOT(slotDocumentUrlChanged (KTextEditor::Document *)));
  connect (document, SIGNAL(destroyed (QObject *)), this, SLOT(slotDocumentDestroyed (QObject *)));

  /**
   * trigger slot once, for existing docs
   */
  slotDocumentUrlChanged (document);
}

void KateProjectPlugin::slotDocumentDestroyed (QObject *document)
{
  /**
   * remove mapping to project
   */
  m_document2Project.remove (document);
}

void KateProjectPlugin::slotDocumentUrlChanged (KTextEditor::Document *document)
{
  /**
   * search matching project
   */
  KateProject *project = projectForUrl (document->url());
  
  /**
   * update mapping document => project
   */
  if (!project)
    m_document2Project.remove (document);
  else
    m_document2Project[document] = project;
}

void KateProjectPlugin::slotDirectoryChanged (const QString &path)
{
  /**
   * auto-reload, if there
   */
  KateProject *project = projectForFileName (QFileInfo (path + "/.kateproject").canonicalFilePath ());
  if (project)
    project->reload ();
}

// kate: space-indent on; indent-width 2; replace-tabs on;
