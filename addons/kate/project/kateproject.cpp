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

#include <klocale.h>

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
  , m_fileLastModified()
  , m_notesDocument (0)
  , m_documentsParent (0)
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
  
  /**
   * save notes document, if any
   */
  saveNotesDocument ();
}

bool KateProject::load (const QString &fileName)
{
  /**
   * bail out if already fileName set!
   */
  if (!m_fileName.isEmpty())
    return false;

  /**
   * set new filename and base directory
   */
  m_fileName = fileName;
  m_baseDir = QFileInfo(m_fileName).canonicalPath();

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
  m_fileLastModified = QDateTime();
  QFile file (m_fileName);
  if (!file.open (QFile::ReadOnly))
    return false;

  m_fileLastModified = QFileInfo(m_fileName).lastModified();
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
   * support out-of-source project files
   */
  if (!globalProject["directory"].toString().isEmpty()) 
    m_baseDir = QFileInfo (globalProject["directory"].toString()).canonicalFilePath ();

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
  QMetaObject::invokeMethod (m_worker, "loadProject", Qt::QueuedConnection, Q_ARG(QString, m_baseDir), Q_ARG(QVariantMap, m_projectMap));

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
   * readd the documents that are open atm
   */
  m_documentsParent = 0;
  foreach (KTextEditor::Document *document, m_documents.keys ())
    registerDocument (document);

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

  /**
   * notify external world that data is available
   */
  emit indexChanged ();
}

QFile *KateProject::projectLocalFile (const QString &file) const
{
  /**
   * nothing on empty file names for project
   * should not happen
   */
  if (m_fileName.isEmpty())
    return 0;
  
  /**
   * create dir to store local files, else fail
   */
  if (!QDir().mkpath (m_fileName+".d"))
    return 0;
  
  /**
   * try to open file read-write
   */
  QFile *readWriteFile = new QFile (m_fileName + ".d" + QDir::separator() + file);
  if (!readWriteFile->open (QIODevice::ReadWrite)) {
    delete readWriteFile;
    return 0;
  }
  
  /**
   * all fine, return file
   */
  return readWriteFile;
}

QTextDocument* KateProject::notesDocument ()
{
  /**
   * already there?
   */
  if (m_notesDocument)
    return m_notesDocument;
  
  /**
   * else create it
   */
  m_notesDocument = new QTextDocument (this);
  m_notesDocument->setDocumentLayout (new QPlainTextDocumentLayout (m_notesDocument));
  
  /**
   * and load text if possible
   */
  if (QFile *inFile = projectLocalFile ("notes.txt")) {
    {
      QTextStream inStream (inFile);
      inStream.setCodec ("UTF-8");
      m_notesDocument->setPlainText (inStream.readAll ());
    }
    delete inFile;
  }

  /**
   * and be done
   */
  return m_notesDocument;
}

void KateProject::saveNotesDocument ()
{
  /**
   * no notes document, nothing to do
   */
  if (!m_notesDocument)
    return;
  
  /**
   * try to get file to save to
   */
  if (QFile *outFile = projectLocalFile ("notes.txt")) {
    outFile->resize (0);
    {
      QTextStream outStream (outFile);
      outStream.setCodec ("UTF-8");
      outStream << m_notesDocument->toPlainText ();
    }
    delete outFile;
  }
}


void KateProject::slotModifiedChanged(KTextEditor::Document* document) {
  KateProjectItem *item = itemForFile (m_documents.value (document));
  
  if (!item) return;
  
  item->slotModifiedChanged(document);
}

void KateProject::slotModifiedOnDisk (KTextEditor::Document *document,
      bool isModified, KTextEditor::ModificationInterface::ModifiedOnDiskReason reason) {
  
  KateProjectItem *item = itemForFile (m_documents.value (document));
    
  if (!item) return;
  
  item->slotModifiedOnDisk(document,isModified, reason);
  
}


void KateProject::registerDocument (KTextEditor::Document *document)
{
  // remember the document, if not already there
  if (!m_documents.contains(document))
    m_documents[document] = document->url().toLocalFile ();
  
  // try to get item for the document
  KateProjectItem *item = itemForFile (document->url().toLocalFile ());
  
  // if we got one, we are done, else create a dummy!
  if (item) {
    disconnect(document,SIGNAL(modifiedChanged(KTextEditor::Document *)),this,SLOT(slotModifiedChanged(KTextEditor::Document *)));
    disconnect(document,SIGNAL(modifiedOnDisk(KTextEditor::Document*,bool,KTextEditor::ModificationInterface::ModifiedOnDiskReason)),this,SLOT(slotModifiedOnDisk(KTextEditor::Document*,bool,KTextEditor::ModificationInterface::ModifiedOnDiskReason)));
    item->slotModifiedChanged(document);
    
/*FIXME    item->slotModifiedOnDisk(document,document->isModified(),qobject_cast<KTextEditor::ModificationInterface*>(document)->modifiedOnDisk()); FIXME*/
    
    connect(document,SIGNAL(modifiedChanged(KTextEditor::Document *)),this,SLOT(slotModifiedChanged(KTextEditor::Document *)));
    connect(document,SIGNAL(modifiedOnDisk(KTextEditor::Document*,bool,KTextEditor::ModificationInterface::ModifiedOnDiskReason)),this,SLOT(slotModifiedOnDisk(KTextEditor::Document*,bool,KTextEditor::ModificationInterface::ModifiedOnDiskReason)));

    return;
  }
  
  // perhaps create the parent item
  if (!m_documentsParent) {
    m_documentsParent = new KateProjectItem (KateProjectItem::Directory, i18n ("<untracked>"));
    m_model.insertRow (0, m_documentsParent);
  }
  
  // create document item
  QFileInfo fileInfo (document->url().toLocalFile ());
  KateProjectItem *fileItem = new KateProjectItem (KateProjectItem::File, fileInfo.fileName());
  fileItem->setData(document->url().toLocalFile (), Qt::ToolTipRole);
  fileItem->slotModifiedChanged(document);
  connect(document,SIGNAL(modifiedChanged(KTextEditor::Document *)),this,SLOT(slotModifiedChanged(KTextEditor::Document *)));
  connect(document,SIGNAL(modifiedOnDisk(KTextEditor::Document*,bool,KTextEditor::ModificationInterface::ModifiedOnDiskReason)),this,SLOT(slotModifiedOnDisk(KTextEditor::Document*,bool,KTextEditor::ModificationInterface::ModifiedOnDiskReason)));

  
  bool inserted = false;
  for (int i = 0; i < m_documentsParent->rowCount(); ++i) {
        if (m_documentsParent->child (i)->data(Qt::UserRole).toString() > document->url().toLocalFile ()) {
          m_documentsParent->insertRow (i, fileItem);
          inserted = true;
          break;
        }
  }
  if (!inserted)
    m_documentsParent->appendRow (fileItem);
  
  fileItem->setData (document->url().toLocalFile (), Qt::UserRole);
  fileItem->setData (QVariant (true), Qt::UserRole + 3);
  
  if (!m_file2Item)
    m_file2Item = KateProjectSharedQMapStringItem (new QMap<QString, KateProjectItem *> ());
  (*m_file2Item)[document->url().toLocalFile ()] = fileItem;
}
    
void KateProject::unregisterDocument (KTextEditor::Document *document)
{
  // skip if no works
  if (!m_documents.contains (document))
    return;
  
  // perhaps kill the item we have generated
  bool empty = false;
  if (KateProjectItem *item = (KateProjectItem*)itemForFile (m_documents.value (document))) {
    disconnect(document,SIGNAL(modifiedChanged(KTextEditor::Document *)),this,SLOT(slotModifiedChanged(KTextEditor::Document *)));
    if (m_documentsParent && item->data (Qt::UserRole + 3).toBool ()) {
      for (int i = 0; i < m_documentsParent->rowCount(); ++i) {
        if (m_documentsParent->child (i) == item) {
          m_documentsParent->removeRow (i);
          break;
        }
      }
      
      empty = !m_documentsParent->rowCount();
      
      m_file2Item->remove (m_documents.value (document));
    }
  }
  
  // forget the document
  m_documents.remove (document);
  
  // perhaps remove parent item
  if (m_documentsParent && empty) {
    m_model.removeRow (0);
    m_documentsParent = 0;
  }
}
    
// kate: space-indent on; indent-width 2; replace-tabs on;
