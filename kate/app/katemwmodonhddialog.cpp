/*
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
 
    ---
    Copyright (C) 2004, Anders Lund <anders@alweb.dk>
*/

#include "katemwmodonhddialog.h"
#include "katemwmodonhddialog.moc"

#include "katedocmanager.h"

#include <KTextEditor/Document>

#include <KIconLoader>
#include <KLocale>
#include <KMessageBox>
#include <kprocess.h>
#include <KRun>
#include <KTemporaryFile>
#include <KPushButton>
#include <KVBox>

#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QTextStream>

class KateDocItem : public QTreeWidgetItem
{
  public:
    KateDocItem( KTextEditor::Document *doc, const QString &status, QTreeWidget *tw )
        : QTreeWidgetItem( tw ),
        document( doc )
    {
      setText( 0, doc->url().prettyUrl() );
      setText( 1, status );
      if ( ! doc->isModified() )
        setCheckState( 0, Qt::Checked );
      else
        setCheckState( 0, Qt::Unchecked );
    }
    ~KateDocItem()
  {}

    KTextEditor::Document *document;
};


KateMwModOnHdDialog::KateMwModOnHdDialog( DocVector docs, QWidget *parent, const char *name )
    : KDialog( parent ),
      m_proc( 0 ),
      m_diffFile( 0 )
{
  setCaption( i18n("Documents Modified on Disk") );
  setButtons( User1 | User2 | User3 );
  setButtonGuiItem( User1, KGuiItem (i18n("&Ignore"), "window-close") );
  setButtonGuiItem( User2, KStandardGuiItem::overwrite() );
  setButtonGuiItem( User3, KGuiItem (i18n("&Reload"), "view-refresh") );

  setObjectName( name );
  setModal( true );
  setDefaultButton( KDialog::User3 );

  setButtonWhatsThis( User1, i18n(
                        "Removes the modified flag from the selected documents and closes the "
                        "dialog if there are no more unhandled documents.") );
  setButtonWhatsThis( User2, i18n(
                        "Overwrite selected documents, discarding the disk changes and closes the "
                        "dialog if there are no more unhandled documents.") );
  setButtonWhatsThis( User3, i18n(
                        "Reloads the selected documents from disk and closes the dialog if there "
                        "are no more unhandled documents.") );

  KVBox *w = new KVBox( this );
  setMainWidget( w );
  w->setSpacing( KDialog::spacingHint() );

  KHBox *lo1 = new KHBox( w );

  // dialog text
  QLabel *icon = new QLabel( lo1 );
  icon->setPixmap( DesktopIcon("dialog-warning") );

  QLabel *t = new QLabel( i18n(
                            "<qt>The documents listed below has changed on disk.<p>Select one "
                            "or more at the time and press an action button until the list is empty.</qt>"), lo1 );
  lo1->setStretchFactor( t, 1000 );

  // document list
  twDocuments = new QTreeWidget( w );
  QStringList header;
  header << i18n("Filename") << i18n("Status on Disk");
  twDocuments->setHeaderLabels(header);
  twDocuments->setSelectionMode( QAbstractItemView::SingleSelection );

  QStringList l;
  l << "" << i18n("Modified") << i18n("Created") << i18n("Deleted");
  for ( uint i = 0; i < docs.size(); i++ )
  {
    new KateDocItem( docs[i], l[ (uint)KateDocManager::self()->documentInfo( docs[i] )->modifiedOnDiscReason ], twDocuments );
  }

  connect( twDocuments, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)), this, SLOT(slotSelectionChanged(QTreeWidgetItem *, QTreeWidgetItem *)) );

  // diff button
  KHBox *lo2 = new KHBox ( w );
  QWidget *d = new QWidget (lo2);
  lo2->setStretchFactor (d, 2);
  btnDiff = new KPushButton( KGuiItem (i18n("&View Difference"), "edit"), lo2 );

  btnDiff->setWhatsThis(i18n(
                          "Calculates the difference between the the editor contents and the disk "
                          "file for the selected document, and shows the difference with the "
                          "default application. Requires diff(1).") );
  connect( btnDiff, SIGNAL(clicked()), this, SLOT(slotDiff()) );
  connect( this, SIGNAL(user1Clicked()), this, SLOT(slotUser1()) );
  connect( this, SIGNAL(user2Clicked()), this, SLOT(slotUser2()) );
  connect( this, SIGNAL(user3Clicked()), this, SLOT(slotUser3()) );

  slotSelectionChanged(NULL, NULL);
}

KateMwModOnHdDialog::~KateMwModOnHdDialog()
{
  delete m_proc;
  m_proc = 0;
  if (m_diffFile) {
    m_diffFile->setAutoRemove(true);
    delete m_diffFile;
    m_diffFile = 0;
  }
}

void KateMwModOnHdDialog::slotUser1()
{
  handleSelected( Ignore );
}

void KateMwModOnHdDialog::slotUser2()
{
  handleSelected( Overwrite );
}

void KateMwModOnHdDialog::slotUser3()
{
  handleSelected( Reload );
}

void KateMwModOnHdDialog::handleSelected( int action )
{
  // collect all items we can remove
  QList<QTreeWidgetItem *> itemsToDelete;
  for ( QTreeWidgetItemIterator it ( twDocuments ); *it; ++it )
  {
    KateDocItem *item = (KateDocItem *) * it;
    if ( item->checkState(0) == Qt::Checked )
    {
      KTextEditor::ModificationInterface::ModifiedOnDiskReason reason = KateDocManager::self()->documentInfo( item->document )->modifiedOnDiscReason;
      bool success = true;

      if (KTextEditor::ModificationInterface *iface = qobject_cast<KTextEditor::ModificationInterface *>(item->document))
        iface->setModifiedOnDisk( KTextEditor::ModificationInterface::OnDiskUnmodified );

      switch ( action )
      {
        case Overwrite:
          success = item->document->save();
          if ( ! success )
          {
            KMessageBox::sorry( this,
                                i18n("Could not save the document \n'%1'",
                                     item->document->url().prettyUrl() ) );
          }
          break;

        case Reload:
          item->document->documentReload();
          break;

        default:
          break;
      }

      if ( success )
        itemsToDelete.append( item );
      else
      {
        if (KTextEditor::ModificationInterface *iface = qobject_cast<KTextEditor::ModificationInterface *>(item->document))
          iface->setModifiedOnDisk( reason );
      }
    }
  }

  // remove the marked items
  for (int i = 0; i < itemsToDelete.count(); ++i)
    delete itemsToDelete[i];

// any documents left unhandled?
  if ( ! twDocuments->topLevelItemCount() )
    done( Ok );
}

void KateMwModOnHdDialog::slotSelectionChanged(QTreeWidgetItem *current, QTreeWidgetItem *)
{
  // set the diff button enabled
  btnDiff->setEnabled( current &&
                       KateDocManager::self()->documentInfo( ((KateDocItem*)current)->document )->modifiedOnDiscReason != KTextEditor::ModificationInterface::OnDiskDeleted );
}

// ### the code below is slightly modified from kdelibs/kate/part/katedialogs,
// class KateModOnHdPrompt.
void KateMwModOnHdDialog::slotDiff()
{
  if ( !btnDiff->isEnabled()) // diff button already pressed, proc not finished yet
    return;

  if ( ! twDocuments->currentItem() )
    return;

  KTextEditor::Document *doc = ((KateDocItem*)twDocuments->currentItem())->document;

  // don't try to diff a deleted file
  if ( KateDocManager::self()->documentInfo( doc )->modifiedOnDiscReason == KTextEditor::ModificationInterface::OnDiskDeleted )
    return;

  if (m_diffFile)
    return;

  m_diffFile = new KTemporaryFile();
  m_diffFile->open();

  // Start a KProcess that creates a diff
  m_proc = new KProcess( this );
  m_proc->setOutputChannelMode( KProcess::MergedChannels );
  *m_proc << "diff" << "-ub" << "-" << doc->url().path();
  connect( m_proc, SIGNAL(readyRead()), this, SLOT(slotDataAvailable()) );
  connect( m_proc, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(slotPDone()) );

  setCursor( Qt::WaitCursor );
  btnDiff->setEnabled(false);

  m_proc->start();

  QTextStream ts(m_proc);
  int lastln = doc->lines();
  for ( int l = 0; l < lastln; ++l )
    ts << doc->line( l ) << '\n';
  ts.flush();
  m_proc->closeWriteChannel();
}

void KateMwModOnHdDialog::slotDataAvailable()
{
  m_diffFile->write(m_proc->readAll());
}

void KateMwModOnHdDialog::slotPDone()
{
  setCursor( Qt::ArrowCursor );
  slotSelectionChanged(twDocuments->currentItem(), 0);

  const QProcess::ExitStatus es = m_proc->exitStatus();
  delete m_proc;
  m_proc = 0;

  if ( es != QProcess::NormalExit )
  {
    KMessageBox::sorry( this,
                        i18n("The diff command failed. Please make sure that "
                             "diff(1) is installed and in your PATH."),
                        i18n("Error Creating Diff") );
    delete m_diffFile;
    m_diffFile = 0;
    return;
  }

  if ( m_diffFile->size() == 0 )
  {
    KMessageBox::information( this,
                              i18n("Besides white space changes, the files are identical."),
                              i18n("Diff Output") );
    delete m_diffFile;
    m_diffFile = 0;
    return;
  }

  m_diffFile->setAutoRemove(false);
  KUrl url = KUrl::fromPath(m_diffFile->fileName());
  delete m_diffFile;
  m_diffFile = 0;

  // KRun::runUrl should delete the file, once the client exits
  KRun::runUrl( url, "text/x-patch", this, true );
}

// kate: space-indent on; indent-width 2; replace-tabs on;
