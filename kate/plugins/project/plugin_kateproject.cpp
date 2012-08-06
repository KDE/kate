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
#include "kateprojectpluginview.h"

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
#include <QFileInfo>
 
KateProjectPlugin::KateProjectPlugin (QObject* parent, const QList<QVariant>&)
  : Kate::Plugin ((Kate::Application*)parent)
{
}

KateProjectPlugin::~KateProjectPlugin()
{
  /**
   * cleanup open projects
   */
  qDeleteAll (m_fileName2Project);
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
   * remember project and return it
   */
  m_fileName2Project[canonicalFilePath] = project;
  return project;
}

// kate: space-indent on; indent-width 2; replace-tabs on;
