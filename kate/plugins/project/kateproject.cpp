/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2012 Christoph Cullmann <cullmann@kde.org>
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

#include "kateproject.h"
#include "kateprojectworker.h"

#include <ktexteditor/document.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <qjson/parser.h>

KateProject::KateProject ()
  : QObject ()
  , m_worker (new KateProjectWorker (this))
  , m_thread (m_worker)
{
  /**
   * move worker object over and start our worker thread
   * thread will delete worker on run() exit
   */
  m_worker->moveToThread (&m_thread);
  m_thread.start ();
}

KateProject::~KateProject ()
{
  /**
   * only do this once
   */
  Q_ASSERT (m_worker);

  /**
   * quit the thread event loop and wait for completion
   * will delete worker on thread run() exit
   */
  m_thread.quit ();
  m_thread.wait ();

  /**
   * marks as deleted
   */
  m_worker = 0;
}

bool KateProject::load (const QString &fileName)
{
  /**
   * bail out if already fileName set!
   */
  if (!m_fileName.isEmpty())
    return false;

  /**
   * set new filename
   */
  m_fileName = fileName;

  /**
   * trigger reload
   */
  return reload ();
}

bool KateProject::reload (bool force)
{
  /**
   * open the file for reading, bail out on error!
   */
  QFile file (m_fileName);
  if (!file.open (QFile::ReadOnly))
    return false;

  /**
   * parse the whole file, bail out again on error!
   */
  bool ok = true;
  QJson::Parser parser;
  QVariant project = parser.parse (&file, &ok);
  if (!ok)
    return false;

  /**
   * now: get global group
   */
  QVariantMap globalProject = project.toMap ();

  /**
   * no name, bad => bail out
   */
  if (globalProject["name"].toString().isEmpty())
    return false;

  /**
   * anything changed?
   * else be done without forced reload!
   */
  if (!force && (m_projectMap == globalProject))
    return true;

  /**
   * setup global attributes in this object
   */
  m_projectMap = globalProject;

  /**
   * emit that we changed stuff
   */
  emit projectMapChanged ();

  /**
   * trigger worker to REALLY load the project model and stuff
   */
  QMetaObject::invokeMethod (m_worker, "loadProject", Qt::QueuedConnection, Q_ARG(QString, m_fileName), Q_ARG(QVariantMap, m_projectMap));

  /**
   * done ok ;)
   */
  return true;
}

void KateProject::loadProjectDone (KateProjectSharedQStandardItem topLevel, KateProjectSharedQMapStringItem file2Item)
{
  /**
   * no worker any more, do nothing!
   * shared pointers will do deletions
   */
  if (!m_worker)
      return;

  /**
   * setup model data
   */
  m_model.clear ();
  m_model.invisibleRootItem()->appendColumn (topLevel->takeColumn (0));

  /**
   * setup file => item map
   */
  m_file2Item = file2Item;

  /**
   * model changed
   */
  emit modelChanged ();
}

void KateProject::loadIndexDone (KateProjectSharedProjectIndex projectIndex)
{
  /**
   * no worker any more, do nothing!
   * shared pointers will do deletions
   */
  if (!m_worker)
      return;

  /**
   * move to our project
   */
  m_projectIndex = projectIndex;
}

// kate: space-indent on; indent-width 2; replace-tabs on;
