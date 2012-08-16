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

#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <qjson/parser.h>

KateProject::KateProject ()
  : QObject ()
  , m_worker (new KateProjectWorker (this))
  , m_file2Item (new QMap<QString, QStandardItem *>())
{
  /**
   * move worker object over and start our worker thread
   */
  m_worker->moveToThread (&m_thread);
  m_thread.start ();
}

KateProject::~KateProject ()
{
  /**
   * worker must be already gone!
   */
  Q_ASSERT (!m_worker);

  /**
   * delete other data
   */
  delete m_file2Item;
}

void KateProject::triggerDeleteLater ()
{
  /**
   * only do this once
   */
  Q_ASSERT (m_worker);
  
  /**
   * quit the thread event loop and wait for completion
   */
  m_thread.quit ();
  m_thread.wait ();
  
  /**
   * delete worker, before thread is deleted
   */
  delete m_worker;
  m_worker = 0;
  
  /**
   * trigger delete later
   */
  deleteLater ();
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

bool KateProject::reload ()
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
   * setup global attributes in this object
   */
  m_name = globalProject["name"].toString();
  m_projectMap = globalProject;

  /**
   * now, clear some stuff and before triggering worker thread to do the work ;)
   */
  m_model.clear ();
  m_file2Item->clear ();
  
  /**
   * trigger worker
   */
  QMetaObject::invokeMethod (m_worker, "loadProject", Qt::QueuedConnection, Q_ARG(QString, m_fileName), Q_ARG(QVariantMap, m_projectMap));

  /**
   * done ok ;)
   */
  return true;
}

void KateProject::loadProjectDone (void *topLevel, void *file2Item)
{ 
  /**
   * convert to right types
   */
  QStandardItem *topLevelItem = static_cast<QStandardItem*> (topLevel);
  QMap<QString, QStandardItem *> *file2ItemMap = static_cast<QMap<QString, QStandardItem *>*> (file2Item);

  /**
   * no worker any more, only cleanup the stuff we got from invoke!
   */
  if (!m_worker) {
      delete topLevelItem;
      delete file2ItemMap;
      return;
  }
  
  /**
   * setup model data
   */
  m_model.invisibleRootItem()->appendColumn (topLevelItem->takeColumn (0));
  delete topLevelItem;
  
  /**
   * setup file => item map
   */
  delete m_file2Item;
  m_file2Item = file2ItemMap;
}

// kate: space-indent on; indent-width 2; replace-tabs on;
