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

#include <algorithm>

KateProject::KateProject ()
  : QObject ()
  , m_worker (new KateProjectWorker (this))
  , m_file2Item (new QMap<QString, QStandardItem *>())
  , m_completionInfo (new QStringList ())
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
  delete m_completionInfo;
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

void KateProject::loadProjectDone (KateProjectSharedQStandardItem topLevel, void *file2Item)
{
  /**
   * convert to right types
   */
  QMap<QString, QStandardItem *> *file2ItemMap = static_cast<QMap<QString, QStandardItem *>*> (file2Item);

  /**
   * no worker any more, only cleanup the stuff we got from invoke!
   */
  if (!m_worker) {
      delete file2ItemMap;
      return;
  }

  /**
   * setup model data
   */
  m_model.clear ();
  m_model.invisibleRootItem()->appendColumn (topLevel->takeColumn (0));

  /**
   * setup file => item map
   */
  delete m_file2Item;
  m_file2Item = file2ItemMap;

  /**
   * model changed
   */
  emit modelChanged ();
}

void KateProject::loadCompletionDone (void *completionInfo)
{
  /**
   * convert to right types
   */
  QStringList *completionInfoList = static_cast<QStringList *> (completionInfo);

  /**
   * no worker any more, only cleanup the stuff we got from invoke!
   */
  if (!m_worker) {
      delete completionInfoList;
      return;
  }

  /**
   * setup
   */
  delete m_completionInfo;
  m_completionInfo = completionInfoList;
}

void KateProject::completionMatches (QStandardItemModel &model, KTextEditor::View *view, const KTextEditor::Range & range)
{
  /**
   * word to complete
   */
  QString word = view->document()->text(range);
  
  /**
   * get all matching things
   * use binary search for prefix
   */
  QStringList::iterator lowerBound = std::lower_bound (m_completionInfo->begin(), m_completionInfo->end(), word);
  while (lowerBound != m_completionInfo->end()) {
    if (lowerBound->startsWith (word)) {
      model.appendRow (new QStandardItem (*lowerBound));
      ++lowerBound;
      continue;
    }    
    break;
  }
}

// kate: space-indent on; indent-width 2; replace-tabs on;
