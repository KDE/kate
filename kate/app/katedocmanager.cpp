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
#include "katecontainer.h"
#include "katesavemodifieddialog.h"
#include "katedebug.h"

#include <KTextEditor/View>
#include <KTextEditor/SessionConfigInterface>
#include <KTextEditor/Editor>
#include <KTextEditor/ContainerInterface>

#include <KCodecs>
#include <KMessageBox>
#include <KEncodingFileDialog>
#include <KIO/DeleteJob>
#include <KIconLoader>
#include <KColorScheme>
#include <klocalizedstring.h>
#include <kconfiggroup.h>

#include <QtCore/QByteArray>
#include <QtCore/QCryptographicHash>
#include <QtCore/QDateTime>
#include <QtCore/QHash>
#include <QtCore/QTextCodec>
#include <QtCore/QTimer>
#include <QtWidgets/QListView>
#include <QtWidgets/QProgressDialog>

KateDocManager::KateDocManager (QObject *parent)
    : QObject(parent)
    , m_saveMetaInfos(true)
    , m_daysMetaInfos(0)
    , m_documentStillToRestore (0)
    , m_suppressOpeningErrorDialogs (false)
{
  // Constructed the beloved editor ;)
  m_editor = KTextEditor::editor();
  
  if ( !m_editor )
  {
    KMessageBox::error(0, i18n("A KDE text-editor component could not be found.\n"
                                  "Please check your KDE installation."));
    exit(1);
  }

  KTextEditor::ContainerInterface * iface = qobject_cast<KTextEditor::ContainerInterface *>( m_editor );
  if (iface != NULL) {
    iface->setContainer( new KateContainer(KateApp::self()) );
  } else {
    qCDebug(LOG_KATE)<<"Editor does not support the container interface";
  }
  
  // read in editor config
  m_editor->readConfig(KSharedConfig::openConfig().data());

  m_documentManager = new Kate::DocumentManager (this);

  m_metaInfos = new KConfig("metainfos", KConfig::NoGlobals, QStandardPaths::DataLocation );

  createDoc ();
}

KateDocManager::~KateDocManager ()
{
  // write editor config
  m_editor->writeConfig(KSharedConfig::openConfig().data());

  // write metainfos?
  if (m_saveMetaInfos)
  {
    // saving meta-infos when file is saved is not enough, we need to do it once more at the end
    saveMetaInfos( m_docList );

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

  qDeleteAll( m_docInfos );
  delete m_metaInfos;
  delete m_documentManager;
  // delete m_editor; don't delete this here - it's cleaned up when the plugin is unloaded
}

KateDocManager *KateDocManager::self ()
{
  return KateApp::self()->documentManager ();
}

void KateDocManager::setSuppressOpeningErrorDialogs (bool suppress)
{
  m_suppressOpeningErrorDialogs = suppress;
}

KTextEditor::Document *KateDocManager::createDoc (const KateDocumentInfo& docInfo)
{
  qCDebug(LOG_KATE)<<"createDoc"<<endl;

  KTextEditor::Document *doc = (KTextEditor::Document *) m_editor->createDocument(this);
  
  // turn of the editorpart's own modification dialog, we have our own one, too!
  const KConfigGroup generalGroup(KSharedConfig::openConfig(), "General");
  bool ownModNotification = generalGroup.readEntry("Modified Notification", false);
  if (qobject_cast<KTextEditor::ModificationInterface *>(doc))
    qobject_cast<KTextEditor::ModificationInterface *>(doc)->setModifiedOnDiskWarning (!ownModNotification);

  m_docList.append(doc);
  m_docInfos.insert (doc, new KateDocumentInfo (docInfo));

  // connect internal signals...
  connect(doc, SIGNAL(modifiedChanged(KTextEditor::Document*)), this, SLOT(slotModChanged1(KTextEditor::Document*)));
  connect(doc, SIGNAL(modifiedOnDisk(KTextEditor::Document*,bool,KTextEditor::ModificationInterface::ModifiedOnDiskReason)),
          this, SLOT(slotModifiedOnDisc(KTextEditor::Document*,bool,KTextEditor::ModificationInterface::ModifiedOnDiskReason)));

  // we have a new document, show it the world
  
  //  m_documentManager->documentCreated must come first
  //  as to signal plugins and other api users about the change
  //  before the view manager gets a chance to signal a viewChanged
  emit m_documentManager->documentCreated (doc);
  emit documentCreated (doc);

  // return our new document
  return doc;
}

void KateDocManager::deleteDoc (KTextEditor::Document *doc)
{
  KateApp::self()->emitDocumentClosed(QString("%1").arg((qptrdiff)doc));
  qCDebug(LOG_KATE)<<"deleting document with name:"<<doc->documentName();

  // document will be deleted, soon
  emit m_documentManager->documentWillBeDeleted (doc);

  // really delete the document and its infos
  delete m_docInfos.take (doc);
  delete m_docList.takeAt (m_docList.indexOf(doc));
  
  //??????????????? AT THIS POINT THE REFERENCED POINTER IS INVALID
  // document is gone, emit our signals
  emit documentDeleted (doc);
  emit m_documentManager->documentDeleted (doc);
}

KTextEditor::Document *KateDocManager::document (uint n)
{
  return m_docList.at(n);
}

KateDocumentInfo *KateDocManager::documentInfo (KTextEditor::Document *doc)
{
  return m_docInfos.contains(doc) ? m_docInfos[doc] : 0;
}

int KateDocManager::findDocument (KTextEditor::Document *doc)
{
  return m_docList.indexOf (doc);
}

uint KateDocManager::documents ()
{
  return m_docList.count ();
}

KTextEditor::Document *KateDocManager::findDocument (const QUrl &url) const
{
  QUrl u( url.adjusted(QUrl::NormalizePathSegments) );

  foreach (KTextEditor::Document* it, m_docList)
  {
    if ( it->url() == u)
      return it;
  }

  return 0;
}

bool KateDocManager::isOpen(QUrl url)
{
  // return just if we found some document with this url
  return findDocument (url) != 0;
}

KTextEditor::Document *KateDocManager::openUrl (const QUrl& url, const QString &encoding, bool isTempFile, const KateDocumentInfo& docInfo)
{
  QUrl u( url.adjusted(QUrl::NormalizePathSegments) );

  // special handling if still only the first initial doc is there
  if (!documentList().isEmpty() && (documentList().count() == 1) && (!documentList().at(0)->isModified() && documentList().at(0)->url().isEmpty()))
  {
    KTextEditor::Document* doc = documentList().first();

    doc->setEncoding(encoding);

    if (!u.isEmpty())
    {
      doc->setSuppressOpeningErrorDialogs (m_suppressOpeningErrorDialogs);

      (*documentInfo(doc)) = docInfo;
      if (!loadMetaInfos(doc, u))
        doc->openUrl (u);
      else if (! encoding.isEmpty()) // set encoding again if provided, as metainfos sets it.
        doc->setEncoding(encoding);

      doc->setSuppressOpeningErrorDialogs (false);

      if ( isTempFile && u.isLocalFile() )
      {
        QFileInfo fi( u.toLocalFile() );
        if ( fi.exists() )
        {
          m_tempFiles[ doc] = qMakePair(u, fi.lastModified());
          qCDebug(LOG_KATE) << "temporary file will be deleted after use unless modified: " << u.url();
        }
      }
    }

    connect(doc, SIGNAL(modifiedChanged(KTextEditor::Document*)), this, SLOT(slotModChanged(KTextEditor::Document*)));

    emit initialDocumentReplaced();

    return doc;
  }

  KTextEditor::Document *doc = 0;

  // always new document if url is empty...
  if (!u.isEmpty())
    doc = findDocument (u);

  if ( !doc )
  {
    doc = createDoc (docInfo);

    doc->setEncoding(encoding);

    if (!u.isEmpty())
    {
      doc->setSuppressOpeningErrorDialogs (m_suppressOpeningErrorDialogs);

      if (!loadMetaInfos(doc, u))
        doc->openUrl (u);

      doc->setSuppressOpeningErrorDialogs (false);
    }
  }

  return doc;
}

bool KateDocManager::closeDocuments(const QList<KTextEditor::Document *> &documents, bool closeUrl)
{
  if (documents.isEmpty()) return false;

  saveMetaInfos( documents );

  foreach(KTextEditor::Document *doc, documents)
  {
    if (closeUrl && !doc->closeUrl())
      return false; // get out on first error

    for (int i = 0; i < KateApp::self()->mainWindows (); i++ ) {
      KateApp::self()->mainWindow(i)->viewManager()->closeViews(doc);
    }

    if ( closeUrl && m_tempFiles.contains( doc ) )
    {
      QFileInfo fi( m_tempFiles[ doc ].first.toLocalFile() );
      if ( fi.lastModified() <= m_tempFiles[ doc ].second ||
          KMessageBox::questionYesNo( KateApp::self()->activeMainWindow(),
                                      i18n("The supposedly temporary file %1 has been modified. "
                                      "Do you want to delete it anyway?", m_tempFiles[ doc ].first.url(QUrl::PreferLocalFile)),
                                      i18n("Delete File?") ) == KMessageBox::Yes )
      {
        KIO::del( m_tempFiles[ doc ].first, KIO::HideProgressInfo );
        qCDebug(LOG_KATE) << "Deleted temporary file " << m_tempFiles[ doc ].first;
        m_tempFiles.remove( doc );
      }
      else
      {
        m_tempFiles.remove(doc);
        qCDebug(LOG_KATE) << "The supposedly temporary file " << m_tempFiles[ doc ].first.url() << " have been modified since loaded, and has not been deleted.";
      }
    }

    deleteDoc (doc);
  }

  // never ever empty the whole document list
  if (m_docList.isEmpty())
    createDoc ();

  return true;
}

bool KateDocManager::closeDocument(KTextEditor::Document *doc, bool closeUrl)
{
  if (!doc) return false;

  QList<KTextEditor::Document *> documents;
  documents.append( doc );

  return closeDocuments( documents, closeUrl );
}

bool KateDocManager::closeOtherDocuments(uint n)
{
  return closeOtherDocuments (document(n));
}

bool KateDocManager::closeDocumentList(QList<KTextEditor::Document*> documents)
{
  bool res = true;

  for (int i = 0; i < KateApp::self()->mainWindows (); i++ )
    KateApp::self()->mainWindow(i)->viewManager()->setViewActivationBlocked(true);

  QList<KTextEditor::Document*> modifiedDocuments;
  foreach(KTextEditor::Document* document, documents) {
    if (document->isModified()) {
      modifiedDocuments.append(document);
    }
  }

  if(modifiedDocuments.size() > 0 && !KateSaveModifiedDialog::queryClose(0, modifiedDocuments)) {
    return false;
  }

  res = closeDocuments( documents, false ); // Do not show save/discard dialog

  for (int i = 0; i < KateApp::self()->mainWindows (); i++ )
  {
    KateApp::self()->mainWindow(i)->viewManager()->setViewActivationBlocked(false);
    if (!KateApp::self()->mainWindow(i)->viewManager()->activeView())
      KateApp::self()->mainWindow(i)->viewManager()->activateView (m_docList.at(0));
    else
      KateApp::self()->mainWindow(i)->viewManager()->reactivateActiveView();
  }

  return res;
}

bool KateDocManager::closeAllDocuments(bool closeUrl)
{
  bool res = true;

  QList<KTextEditor::Document*> docs = m_docList;

  for (int i = 0; i < KateApp::self()->mainWindows (); i++ )
    KateApp::self()->mainWindow(i)->viewManager()->setViewActivationBlocked(true);

  res = closeDocuments( docs, closeUrl );

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

  QList<KTextEditor::Document*> documents = m_docList;

  for (int i = 0; i < KateApp::self()->mainWindows (); i++ )
    KateApp::self()->mainWindow(i)->viewManager()->setViewActivationBlocked(true);

  documents.removeOne( doc );

  res = closeDocuments( documents );

  for (int i = 0; i < KateApp::self()->mainWindows (); i++ )
  {
    KateApp::self()->mainWindow(i)->viewManager()->setViewActivationBlocked(false);
    KateApp::self()->mainWindow(i)->viewManager()->activateView (m_docList.at(0));
  }

  return res;
}

/**
 * Find all modified documents.
 * @return Return the list of all modified documents.
 */
QList<KTextEditor::Document*> KateDocManager::modifiedDocumentList()
{
  QList<KTextEditor::Document*> modified;
  foreach (KTextEditor::Document* doc, m_docList)
  {
    if (doc->isModified())
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
        KEncodingFileDialog::Result r = KEncodingFileDialog::getSaveUrlAndEncoding( doc->encoding(), QUrl(), QString(), w, i18n("Save As"));

        doc->setEncoding( r.encoding );

        if (!r.URLs.isEmpty())
        {
          QUrl tmp = r.URLs.first();

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

void KateDocManager::saveSelected(const QList<KTextEditor::Document*> &docList)
{
  foreach(KTextEditor::Document* doc, docList)
  {
    if ( doc->isModified() )
      doc->documentSave();
  }
}

void KateDocManager::reloadAll()
{
  // reload all docs that are NOT modified on disk
  foreach ( KTextEditor::Document *doc, m_docList ) {
    if ( ! documentInfo( doc )->modifiedOnDisc )
      doc->documentReload();
  }

  // take care of all documents that ARE modified on disk
  KateApp::self()->activeMainWindow()->showModOnDiskPrompt();
}

void KateDocManager::closeOrphaned()
{
  QList<KTextEditor::Document *> documents;

  foreach ( KTextEditor::Document *doc, m_docList )
  {
    KateDocumentInfo* info = documentInfo( doc );
    if (info && !info->openSuccess)
      documents.append( doc );
  }

  closeDocuments( documents );
}

void KateDocManager::saveDocumentList (KConfig* config)
{
  KConfigGroup openDocGroup(config, "Open Documents");

  openDocGroup.writeEntry ("Count", m_docList.count());

  int i = 0;
  foreach ( KTextEditor::Document *doc, m_docList)
  {
    KConfigGroup cg( config, QString("Document %1").arg(i) );
    if (KTextEditor::ParameterizedSessionConfigInterface *iface =
      qobject_cast<KTextEditor::ParameterizedSessionConfigInterface*>(doc))
    {
      iface->writeParameterizedSessionConfig(cg, KTextEditor::ParameterizedSessionConfigInterface::SkipNone);
    }

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

  QProgressDialog *progress = new QProgressDialog();
  progress->setWindowTitle(i18n("Starting Up"));
  progress->setLabelText(i18n("Reopening files from the last session..."));
  progress->setModal(true);
  progress->setCancelButton(0);
  progress->setRange(0, count);

  m_documentStillToRestore = count;
  m_openingErrors.clear();
  for (unsigned int i = 0; i < count; i++)
  {
    KConfigGroup cg( config, QString("Document %1").arg(i));
    KTextEditor::Document *doc = 0;

    if (i == 0) {
      doc = document (0);
    }
    else
      doc = createDoc ();
    doc->setSuppressOpeningErrorDialogs(true);
    connect(doc, SIGNAL(completed()), this, SLOT(documentOpened()));
    connect(doc, SIGNAL(canceled(QString)), this, SLOT(documentOpened()));
    if (KTextEditor::ParameterizedSessionConfigInterface *iface =
      qobject_cast<KTextEditor::ParameterizedSessionConfigInterface *>(doc))
    {
      iface->readParameterizedSessionConfig(cg, KTextEditor::ParameterizedSessionConfigInterface::SkipNone);
    }

    progress->setValue(i);
  }
  delete progress;
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
bool KateDocManager::loadMetaInfos(KTextEditor::Document *doc, const QUrl &url)
{
  if (!m_saveMetaInfos)
    return false;

  if (!m_metaInfos->hasGroup(url.toDisplayString()))
    return false;

  QByteArray md5;
  bool ok = true;

  if (computeUrlMD5(url, md5))
  {
    KConfigGroup urlGroup( m_metaInfos, url.toDisplayString() );
    const QString old_md5 = urlGroup.readEntry("MD5");

    if (QString (md5) == old_md5)
    {
      if (KTextEditor::ParameterizedSessionConfigInterface *iface =
        qobject_cast<KTextEditor::ParameterizedSessionConfigInterface *>(doc))
      {
        KTextEditor::ParameterizedSessionConfigInterface::SessionConfigParameter flags =
          KTextEditor::ParameterizedSessionConfigInterface::SkipNone;
        if (documentInfo(doc)->openedByUser) {
          flags = KTextEditor::ParameterizedSessionConfigInterface::SkipEncoding;
        }
        iface->readParameterizedSessionConfig(urlGroup, flags);
      }
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

void KateDocManager::saveMetaInfos(const QList<KTextEditor::Document *> &documents)
{
  if (!m_saveMetaInfos)
    return;

  QByteArray md5;
  QDateTime now = QDateTime::currentDateTime();

  foreach(const KTextEditor::Document *doc, documents)
  {
    if (doc->isModified())
      continue;

    if (computeUrlMD5(doc->url(), md5))
    {
      KConfigGroup urlGroup( m_metaInfos, doc->url().toString() );

      if (KTextEditor::SessionConfigInterface *iface = qobject_cast<KTextEditor::SessionConfigInterface *>(doc))
        iface->writeSessionConfig(urlGroup);

      urlGroup.writeEntry("MD5", md5.constData());
      urlGroup.writeEntry("Time", now);
    }
  }

  m_metaInfos->sync();
}

// TODO: KDE 5: KateDocument computes the md5 digest when loading a file, and
//       when saving a file. Maybe add "QString KTextEditor::Document::md5sum()" const?
bool KateDocManager::computeUrlMD5(const QUrl &url, QByteArray &result)
{
  QFile f(url.toLocalFile());

  if (f.exists() && f.open(QIODevice::ReadOnly))
  {
    QCryptographicHash crypto(QCryptographicHash::Md5);
    while(!f.atEnd())
        crypto.addData (f.read(256 * 1024));
    result = crypto.result();
    f.close();
  }
  else
    return false;

  return true;
}

void KateDocManager::slotModChanged(KTextEditor::Document * doc)
{
  QList<KTextEditor::Document *> documents;
  documents.append( doc );
  saveMetaInfos( documents );
}

void KateDocManager::slotModChanged1(KTextEditor::Document * doc)
{
  const KateDocumentInfo *info = documentInfo(doc);

  if (info && info->modifiedOnDisc)
  {
    QMetaObject::invokeMethod(KateApp::self()->activeMainWindow(), "queueModifiedOnDisc",
            Qt::QueuedConnection, Q_ARG(KTextEditor::Document *, doc));
  }
}

void KateDocManager::documentOpened()
{
  KColorScheme colors(QPalette::Active);

  KTextEditor::Document *doc = qobject_cast<KTextEditor::Document*>(sender());
  if (!doc) return; // should never happen, but who knows
  doc->setSuppressOpeningErrorDialogs(false);
  disconnect(doc, SIGNAL(completed()), this, SLOT(documentOpened()));
  disconnect(doc, SIGNAL(canceled(QString)), this, SLOT(documentOpened()));
  if (doc->openingError())
  {
    m_openingErrors += '\n' + doc->openingErrorMessage()+"\n\n";
    KateDocumentInfo* info = documentInfo(doc);
    if (info) {
      info->openSuccess = false;
    }
  }
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


