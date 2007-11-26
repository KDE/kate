/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2007 Flavio Castelli <flavio.castelli@gmail.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "katedocmanager.h"
#include "katedocmanager.moc"
#include "kateapp.h"
#include "katemainwindow.h"
#include "kateviewmanager.h"
#include "katedocmanageradaptor.h"

#include <KTextEditor/View>
#include <KTextEditor/SessionConfigInterface>
#include <KTextEditor/EditorChooser>

#include <KParts/Factory>

#include <KLocale>
#include <kdebug.h>
#include <KConfig>
#include <KLibLoader>
#include <KCodecs>
#include <KMessageBox>
#include <KEncodingFileDialog>
#include <KIO/DeleteJob>
#include <KIconLoader>
#include <KProgressDialog>

#include <QDateTime>
#include <QTextCodec>
#include <QByteArray>
#include <QHash>
#include <QListView>
#include <QStandardItem>
#include <QTimer>

KateDocManager::KateDocManager (QObject *parent)
    : QStandardItemModel (parent)
    , m_saveMetaInfos(true)
    , m_daysMetaInfos(0)
    , m_documentStillToRestore (0)
    , m_suppressOpeningErrorDialogs (false)
{
  // Constructed the beloved editor ;)
  m_editor = KTextEditor::EditorChooser::editor();

  // read in editor config
  m_editor->readConfig(KGlobal::config().data());

  m_documentManager = new Kate::DocumentManager (this);

  ( void ) new KateDocManagerAdaptor( this );
  m_dbusObjectPath = "/DocumentManager";
  QDBusConnection::sessionBus().registerObject( m_dbusObjectPath, this );

  m_metaInfos = new KConfig("metainfos", KConfig::NoGlobals, "appdata" );

  createDoc ();
}

KateDocManager::~KateDocManager ()
{
  // write editor config
  m_editor->writeConfig(KGlobal::config().data());

  // write metainfos?
  if (m_saveMetaInfos)
  {
    // saving meta-infos when file is saved is not enough, we need to do it once more at the end
    foreach (KTextEditor::Document *doc, m_docList)
    saveMetaInfos(doc);

    // purge saved filesessions
    if (m_daysMetaInfos > 0)
    {
      const QStringList groups = m_metaInfos->groupList();
      QDateTime def(QDate(1970, 1, 1));
      for (QStringList::const_iterator it = groups.begin(); it != groups.end(); ++it)
      {
        QDateTime last = m_metaInfos->group(*it).readEntry("Time", def);
        if (last.daysTo(QDateTime::currentDateTime()) > m_daysMetaInfos)
          m_metaInfos->deleteGroup(*it);
      }
    }
  }

  delete m_metaInfos;
}

KateDocManager *KateDocManager::self ()
{
  return KateApp::self()->documentManager ();
}

void KateDocManager::setSuppressOpeningErrorDialogs (bool suppress)
{
  m_suppressOpeningErrorDialogs = suppress;
}

QVariant KateDocManager::data( const QModelIndex & index, int role ) const
{
  if ( role == OpeningOrderRole)
    return m_docList.indexOf( data(index, DocumentRole).value<KTextEditor::Document*>() );
  return QStandardItemModel::data( index, role );
}

KTextEditor::Document *KateDocManager::createDoc ()
{
  KTextEditor::Document *doc = (KTextEditor::Document *) m_editor->createDocument(this);

  // turn of the editorpart's own modification dialog, we have our own one, too!
  if (qobject_cast<KTextEditor::ModificationInterface *>(doc))
    qobject_cast<KTextEditor::ModificationInterface *>(doc)->setModifiedOnDiskWarning (false);

  m_docList.append(doc);
  m_docInfos.insert (doc, new KateDocumentInfo ());

  QStandardItem *modelitem = new QStandardItem(doc->documentName());
  modelitem->setData(QVariant::fromValue(doc), DocumentRole);
//   modelitem->setData(m_docList.count()-1, OpeningOrderRole);
  modelitem->setEditable(false);
  modelitem->setIcon(KIcon("null"));
  modelitem->setToolTip(doc->url().prettyUrl());
  appendRow(modelitem);
  m_documentItemMapping.insert(doc, modelitem);

  // connect internal signals...
  connect(doc, SIGNAL(documentUrlChanged ( KTextEditor::Document *)), this, SLOT(slotDocumentUrlChanged(KTextEditor::Document *)));
  connect(doc, SIGNAL(modifiedChanged(KTextEditor::Document *)), this, SLOT(slotModChanged1(KTextEditor::Document *)));
  connect(doc, SIGNAL(documentNameChanged ( KTextEditor::Document * )), SLOT(slotDocumentNameChanged(KTextEditor::Document *)));
  connect(doc, SIGNAL(modifiedOnDisk(KTextEditor::Document *, bool, KTextEditor::ModificationInterface::ModifiedOnDiskReason)),
          this, SLOT(slotModifiedOnDisc(KTextEditor::Document *, bool, KTextEditor::ModificationInterface::ModifiedOnDiskReason)));

  // we have a new document, show it the world
  emit documentCreated (doc);
  emit m_documentManager->documentCreated (doc);

  // return our new document
  return doc;
}

void KateDocManager::slotDocumentNameChanged(KTextEditor::Document* doc)
{
  int rows = rowCount();
  for (int i = 0;i < rows;i++)
  {
    QStandardItem *it = item(i);
    if (it->data(KateDocManager::DocumentRole).value<KTextEditor::Document*>() == doc)
    {
      it->setText(doc->documentName());
      break;
    }
  }
}

void KateDocManager::deleteDoc (KTextEditor::Document *doc)
{
  int rows = rowCount();
  m_documentItemMapping.remove(doc);
  for (int i = 0;i < rows;i++)
  {
    QStandardItem *it = item(i);
    if (it->data(KateDocManager::DocumentRole).value<KTextEditor::Document*>() == doc)
    {
      removeRow(i);
      break;
    }
  }

  // document will be deleted, soon
  emit documentWillBeDeleted (doc);
  emit m_documentManager->documentWillBeDeleted (doc);

  // really delete the document and it's infos
  delete m_docInfos.take (doc);
  delete m_docList.takeAt (m_docList.indexOf(doc));

  // document is gone, emit our signals
  emit documentDeleted (doc);
  emit m_documentManager->documentDeleted (doc);
}

KTextEditor::Document *KateDocManager::document (uint n)
{
  return m_docList.at(n);
}

const KateDocumentInfo *KateDocManager::documentInfo (KTextEditor::Document *doc)
{
  return m_docInfos[doc];
}

int KateDocManager::findDocument (KTextEditor::Document *doc)
{
  return m_docList.indexOf (doc);
}

uint KateDocManager::documents ()
{
  return m_docList.count ();
}

KTextEditor::Document *KateDocManager::findDocument (const KUrl &url) const
{
  foreach (KTextEditor::Document* it, m_docList)
  {
    if ( it->url() == url)
      return it;
  }

  return 0;
}

bool KateDocManager::isOpen(KUrl url)
{
  // return just if we found some document with this url
  return findDocument (url) != 0;
}

KTextEditor::Document *KateDocManager::openUrl (const KUrl& url, const QString &encoding, bool isTempFile)
{
  // special handling if still only the first initial doc is there
  if (!documentList().isEmpty() && (documentList().count() == 1) && (!documentList().at(0)->isModified() && documentList().at(0)->url().isEmpty()))
  {
    KTextEditor::Document* doc = documentList().first();

    doc->setEncoding(encoding);

    if (!url.isEmpty())
    {
      doc->setSuppressOpeningErrorDialogs (m_suppressOpeningErrorDialogs);

      if (!loadMetaInfos(doc, url))
        doc->openUrl (url);

      doc->setSuppressOpeningErrorDialogs (false);

      if ( isTempFile && url.isLocalFile() )
      {
        QFileInfo fi( url.path() );
        if ( fi.exists() )
        {
          m_tempFiles[ doc] = qMakePair(url, fi.lastModified());
          kDebug(13001) << "temporary file will be deleted after use unless modified: " << url.prettyUrl();
        }
      }
    }

    connect(doc, SIGNAL(modifiedChanged(KTextEditor::Document *)), this, SLOT(slotModChanged(KTextEditor::Document *)));

    emit initialDocumentReplaced();

    return doc;
  }

  KTextEditor::Document *doc = 0;

  // always new document if url is empty...
  if (!url.isEmpty())
    doc = findDocument (url);

  if ( !doc )
  {
    doc = createDoc ();

    doc->setEncoding(encoding);

    if (!url.isEmpty())
    {
      doc->setSuppressOpeningErrorDialogs (m_suppressOpeningErrorDialogs);

      if (!loadMetaInfos(doc, url))
        doc->openUrl (url);

      doc->setSuppressOpeningErrorDialogs (false);
    }
  }

  return doc;
}

bool KateDocManager::closeDocument(class KTextEditor::Document *doc, bool closeUrl)
{
  if (!doc) return false;

  saveMetaInfos(doc);
  if (closeUrl && !doc->closeUrl()) return false;

  for (int i = 0; i < KateApp::self()->mainWindows (); i++ )
    KateApp::self()->mainWindow(i)->viewManager()->closeViews(doc);

  if ( closeUrl && m_tempFiles.contains( doc ) )
  {
    QFileInfo fi( m_tempFiles[ doc ].first.path() );
    if ( fi.lastModified() <= m_tempFiles[ doc ].second ||
         KMessageBox::questionYesNo( KateApp::self()->activeMainWindow(),
                                     i18n("The supposedly temporary file %1 has been modified. "
                                          "Do you want to delete it anyway?", m_tempFiles[ doc ].first.prettyUrl()),
                                     i18n("Delete File?") ) == KMessageBox::Yes )
    {
      KIO::del( m_tempFiles[ doc ].first, KIO::HideProgressInfo );
      kDebug(13001) << "Deleted temporary file " << m_tempFiles[ doc ].first;
      m_tempFiles.remove( doc );
    }
    else
    {
      m_tempFiles.remove(doc);
      kDebug(13001) << "The supposedly temporary file " << m_tempFiles[ doc ].first.prettyUrl() << " have been modified since loaded, and has not been deleted.";
    }
  }

  deleteDoc (doc);

  // never ever empty the whole document list
  if (m_docList.isEmpty())
    createDoc ();

  return true;
}

bool KateDocManager::closeDocument(uint n)
{
  return closeDocument(document(n));
}

bool KateDocManager::closeOtherDocuments(uint n)
{
  return closeOtherDocuments (document(n));
}

bool KateDocManager::closeAllDocuments(bool closeUrl)
{
  bool res = true;

  QList<KTextEditor::Document*> docs = m_docList;

  for (int i = 0; i < KateApp::self()->mainWindows (); i++ )
    KateApp::self()->mainWindow(i)->viewManager()->setViewActivationBlocked(true);

  while (!docs.isEmpty() && res)
    if (! closeDocument(docs.at(0), closeUrl) )
      res = false;
    else
      docs.removeFirst();

  for (int i = 0; i < KateApp::self()->mainWindows (); i++ )
  {
    KateApp::self()->mainWindow(i)->viewManager()->setViewActivationBlocked(false);
    KateApp::self()->mainWindow(i)->viewManager()->activateView (m_docList.at(0));
  }

  return res;
}

bool KateDocManager::closeOtherDocuments(KTextEditor::Document* doc)
{
  bool res = true;

  QList<KTextEditor::Document*> docs = m_docList;

  for (int i = 0; i < KateApp::self()->mainWindows (); i++ )
    KateApp::self()->mainWindow(i)->viewManager()->setViewActivationBlocked(true);

  for (int i = 0; (i < docs.size()) && res; i++)
  {
    if (docs.at(i) != doc)
        if (! closeDocument(docs.at(i), false) )
          res = false;
  }

  for (int i = 0; i < KateApp::self()->mainWindows (); i++ )
  {
    KateApp::self()->mainWindow(i)->viewManager()->setViewActivationBlocked(false);
    KateApp::self()->mainWindow(i)->viewManager()->activateView (m_docList.at(0));
  }

  return res;
}

/**
 * Find all modified documents, excluding @p excludedDoc.
 * @param excludedDoc a document that will never be included into the returned
 * list (also if it has been modified).
 * @return Return the list of all modified documents.
 */
QList<KTextEditor::Document*> KateDocManager::modifiedDocumentList(KTextEditor::Document* excludedDoc)
{
  QList<KTextEditor::Document*> modified;
  foreach (KTextEditor::Document* doc, m_docList)
  {
    if ((excludedDoc != doc) && (doc->isModified()))
    {
      modified.append(doc);
    }
  }
  return modified;
}


bool KateDocManager::queryCloseDocuments(KateMainWindow *w)
{
  int docCount = m_docList.count();
  foreach (KTextEditor::Document *doc, m_docList)
  {
    if (doc->url().isEmpty() && doc->isModified())
    {
      int msgres = KMessageBox::warningYesNoCancel( w,
                   i18n("<p>The document '%1' has been modified, but not saved.</p>"
                        "<p>Do you want to save your changes or discard them?</p>", doc->documentName() ),
                   i18n("Close Document"), KStandardGuiItem::save(), KStandardGuiItem::discard() );

      if (msgres == KMessageBox::Cancel)
        return false;

      if (msgres == KMessageBox::Yes)
      {
        KEncodingFileDialog::Result r = KEncodingFileDialog::getSaveUrlAndEncoding( doc->encoding(), QString(), QString(), w, i18n("Save As"));

        doc->setEncoding( r.encoding );

        if (!r.URLs.isEmpty())
        {
          KUrl tmp = r.URLs.first();

          if ( !doc->saveAs( tmp ) )
            return false;
        }
        else
          return false;
      }
    }
    else
    {
      if (!doc->queryClose())
        return false;
    }
  }

  // document count changed while queryClose, abort and notify user
  if (m_docList.count() > docCount)
  {
    KMessageBox::information (w,
                              i18n ("New file opened while trying to close Kate, closing aborted."),
                              i18n ("Closing Aborted"));
    return false;
  }

  return true;
}


void KateDocManager::saveAll()
{
  foreach ( KTextEditor::Document *doc, m_docList )
  if ( doc->isModified() )
    doc->documentSave();
}

void KateDocManager::reloadAll()
{
  foreach ( KTextEditor::Document *doc, m_docList )
    doc->documentReload();
}

void KateDocManager::saveDocumentList (KConfig* config)
{
  KConfigGroup openDocGroup(config, "Open Documents");

  openDocGroup.writeEntry ("Count", m_docList.count());

  int i = 0;
  foreach ( KTextEditor::Document *doc, m_docList)
  {
    KConfigGroup cg( config, QString("Document %1").arg(i) );

    if (KTextEditor::SessionConfigInterface *iface = qobject_cast<KTextEditor::SessionConfigInterface *>(doc))
      iface->writeSessionConfig(cg);

    i++;
  }
}

void KateDocManager::restoreDocumentList (KConfig* config)
{
  KConfigGroup openDocGroup(config, "Open Documents");
  unsigned int count = openDocGroup.readEntry("Count", 0);

  if (count == 0)
  {
    return;
  }

  KProgressDialog *pd = new KProgressDialog(0,
                        i18n("Starting Up"),
                        i18n("Reopening files from the last session..."));
  pd->setModal(true);
  pd->setAllowCancel(false);
  pd->progressBar()->setRange(0, count);

  bool first = true;
  const int countM1 = count - 1;
  m_documentStillToRestore = count;
  m_restoringDocumentList = true;
  m_openingErrors.clear();
  for (unsigned int i = 0; i < count; i++)
  {
    KConfigGroup cg( config, QString("Document %1").arg(i));
    KTextEditor::Document *doc = 0;

    if (first)
    {
      first = false;
      doc = document (0);
    }
    else
      doc = createDoc ();
    doc->setSuppressOpeningErrorDialogs(true);
    connect(doc, SIGNAL(completed()), this, SLOT(documentOpened()));
    connect(doc, SIGNAL(canceled(const QString&)), this, SLOT(documentOpened()));
    if (KTextEditor::SessionConfigInterface *iface = qobject_cast<KTextEditor::SessionConfigInterface *>(doc))
      iface->readSessionConfig(cg);

    pd->progressBar()->setValue(pd->progressBar()->value() + 1);
  }
  m_restoringDocumentList = false;
  delete pd;
}

void KateDocManager::slotModifiedOnDisc (KTextEditor::Document *doc, bool b, KTextEditor::ModificationInterface::ModifiedOnDiskReason reason)
{
  if (m_docInfos.contains(doc))
  {
    m_docInfos[doc]->modifiedOnDisc = b;
    m_docInfos[doc]->modifiedOnDiscReason = reason;
    slotModChanged1(doc);
  }
}

/**
 * Load file and file's meta-information if the MD5 didn't change since last time.
 */
bool KateDocManager::loadMetaInfos(KTextEditor::Document *doc, const KUrl &url)
{
  if (!m_saveMetaInfos)
    return false;

  if (!m_metaInfos->hasGroup(url.prettyUrl()))
    return false;

  QByteArray md5;
  bool ok = true;

  if (computeUrlMD5(url, md5))
  {
    KConfigGroup urlGroup( m_metaInfos, url.prettyUrl() );
    const QString old_md5 = urlGroup.readEntry("MD5");

    if ((const char *)md5 == old_md5)
    {
      if (KTextEditor::SessionConfigInterface *iface = qobject_cast<KTextEditor::SessionConfigInterface *>(doc))
        iface->readSessionConfig(urlGroup);
    }
    else
    {
      urlGroup.deleteGroup();
      ok = false;
    }

    m_metaInfos->sync();
  }

  return ok && doc->url() == url;
}

/**
 * Save file's meta-information if doc is in 'unmodified' state
 */
void KateDocManager::saveMetaInfos(KTextEditor::Document *doc)
{
  QByteArray md5;

  if (!m_saveMetaInfos)
    return;

  if (doc->isModified())
  {
//     kDebug (13020) << "DOC MODIFIED: no meta data saved";
    return;
  }

  if (computeUrlMD5(doc->url(), md5))
  {
    KConfigGroup urlGroup( m_metaInfos, doc->url().prettyUrl() );

    if (KTextEditor::SessionConfigInterface *iface = qobject_cast<KTextEditor::SessionConfigInterface *>(doc))
      iface->writeSessionConfig(urlGroup);

    urlGroup.writeEntry("MD5", (const char *)md5);
    urlGroup.writeEntry("Time", QDateTime::currentDateTime());
    m_metaInfos->sync();
  }
}

bool KateDocManager::computeUrlMD5(const KUrl &url, QByteArray &result)
{
  QFile f(url.path());

  if (f.open(QIODevice::ReadOnly))
  {
    KMD5 md5;

    if (!md5.update(f))
      return false;

    md5.hexDigest(result);
    f.close();
  }
  else
    return false;

  return true;
}

void KateDocManager::slotModChanged(KTextEditor::Document * doc)
{
  saveMetaInfos(doc);
}


void KateDocManager::slotDocumentUrlChanged(KTextEditor::Document *doc)
{
  if (!m_documentItemMapping.contains(doc)) return;
  m_documentItemMapping[doc]->setToolTip(doc->url().prettyUrl());
}

void KateDocManager::slotModChanged1(KTextEditor::Document * doc)
{
  kDebug() << "KateDocManager::slotModChanged (1)";
  if (!m_documentItemMapping.contains(doc)) return;
  kDebug() << "KateDocManager::slotModChanged (2)";
  QStandardItem *item = m_documentItemMapping[doc];
  const KateDocumentInfo *info = KateDocManager::self()->documentInfo(doc);
  item->setIcon(QIcon());
  if (info && info->modifiedOnDisc)
    if (doc->isModified()) item->setIcon(KIcon("modmod"));
    else item->setIcon(KIcon("modonhd"));
  else
    if (doc->isModified()) item->setIcon(KIcon("modified"));
    else item->setIcon(KIcon("null"));
  /*  if ( column == 0) {
      static QPixmap noPm = SmallIcon ("null");
      static QPixmap modPm = SmallIcon("modified");
      static QPixmap discPm = SmallIcon("modonhd");
      static QPixmap modmodPm = SmallIcon("modmod");

      const KateDocumentInfo *info = KateDocManager::self()->documentInfo(doc);

      if (info && info->modifiedOnDisc)
        return doc->isModified() ? &modmodPm : &discPm;
      else
        return doc->isModified() ? &modPm : &noPm;
    }

    return 0; */
}


QModelIndex KateDocManager::indexForDocument(KTextEditor::Document *document)
{
  int row = m_docList.indexOf(document);
  if (row == -1) return index(-1, -1);
  else return index(row, 0);
}

void KateDocManager::documentOpened()
{
  KTextEditor::Document *doc = qobject_cast<KTextEditor::Document*>(sender());
  if (!doc) return; // should never happen, but who knows
  doc->setSuppressOpeningErrorDialogs(false);
  disconnect(doc, SIGNAL(completed()), this, SLOT(documentOpened()));
  disconnect(doc, SIGNAL(canceled(const QString&)), this, SLOT(documentOpened()));
  if (doc->openingError())
    m_openingErrors += '\n' + doc->openingErrorMessage();
  --m_documentStillToRestore;

  if (m_documentStillToRestore == 0)
    QTimer::singleShot(0, this, SLOT(showRestoreErrors()));
}

void KateDocManager::showRestoreErrors ()
{
  if (!m_openingErrors.isEmpty())
  {
    KMessageBox::information (0,
                              m_openingErrors,
                              i18n ("Errors/Warnings while opening documents"));

    // clear errors
    m_openingErrors.clear ();
  }
}

// kate: space-indent on; indent-width 2; replace-tabs on;


