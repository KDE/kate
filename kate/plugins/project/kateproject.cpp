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
#include <QPlainTextDocumentLayout>
#include <qjson/parser.h>

KateProject::KateProject ()
  : QObject ()
  , m_worker (new KateProjectWorker (this))
  , m_thread (m_worker)
  , m_notesDocument(0)
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
  
  if (m_notesDocument) {
    QString notesFile=dataFile("notes.txt");
    if (!notesFile.isEmpty()) {
      QFile outFile(notesFile);
      outFile.open(QIODevice::Text | QIODevice::WriteOnly);
      if (outFile.isOpen()) {
        QTextStream outStream(&outFile);
        outStream<<m_notesDocument->toPlainText();
        outFile.close();
      }
    }
  }
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
   * move to our project
   */
  m_projectIndex = projectIndex;
}

QString KateProject::dataFile(const QString& filename) {
  QString dirPath(m_fileName+".user.d");
  QDir d;
  if (d.mkpath(dirPath)) {
    QString fullFilePath=dirPath+QDir::separator()+filename;
    QFile f(fullFilePath);
    if (!f.exists()) {
      f.open(QIODevice::Text | QIODevice::WriteOnly);
      f.close();
    }
    return fullFilePath;
  } //else error
  return QString();
 
}


QTextDocument* KateProject::notesDocument() {
    if (!m_notesDocument) {
      m_notesDocument=new QTextDocument(this);
      m_notesDocument->setDocumentLayout(new QPlainTextDocumentLayout(m_notesDocument));
      QString notesFile=dataFile("notes.txt");
      if (!notesFile.isEmpty()) {
        QFile inFile(notesFile);
        inFile.open(QIODevice::Text | QIODevice::ReadOnly);
        if (inFile.isOpen()) {
          QTextStream inStream(&inFile);
          m_notesDocument->setPlainText(inStream.readAll());
          inFile.close();
        }
      }
    }
    return m_notesDocument;
}
// kate: space-indent on; indent-width 2; replace-tabs on;
