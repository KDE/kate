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
#include <KProcIO>
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
  {};

    KTextEditor::Document *document;
};


KateMwModOnHdDialog::KateMwModOnHdDialog( DocVector docs, QWidget *parent, const char *name )
    : KDialog( parent )
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

  connect( twDocuments, SIGNAL(currentItemChanged(QTreeWidget *, QTreeWidget *)), this, SLOT(slotSelectionChanged(QTreeWidget *, QTreeWidget *)) );

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
  m_tmpfile = 0;
}

KateMwModOnHdDialog::~KateMwModOnHdDialog()
{}

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

void KateMwModOnHdDialog::slotSelectionChanged(QTreeWidget *current, QTreeWidget *)
{
  // set the diff button enabled
  btnDiff->setEnabled( current &&
                       KateDocManager::self()->documentInfo( ((KateDocItem*)current)->document )->modifiedOnDiscReason != 3 );
}

// ### the code below is slightly modified from kdelibs/kate/part/katedialogs,
// class KateModOnHdPrompt.
void KateMwModOnHdDialog::slotDiff()
{
  if ( m_tmpfile ) // we are already somewhere in this process.
    return;

  if ( ! twDocuments->currentItem() )
    return;

  KTextEditor::Document *doc = ((KateDocItem*)twDocuments->currentItem())->document;

  // don't try o diff a deleted file
  if ( KateDocManager::self()->documentInfo( doc )->modifiedOnDiscReason == 3 )
    return;

  // Start a KProcess that creates a diff
  KProcIO *p = new KProcIO();
  p->setComm( KProcess::All );
  *p << "diff" << "-ub" << "-" <<  doc->url().path();
  connect( p, SIGNAL(processExited(KProcess*)), this, SLOT(slotPDone(KProcess*)) );
  connect( p, SIGNAL(readReady(KProcIO*)), this, SLOT(slotPRead(KProcIO*)) );

  setCursor( Qt::WaitCursor );

  p->start( KProcess::NotifyOnExit, true );

  uint lastln =  doc->lines();
  for ( uint l = 0; l <  lastln; l++ )
    p->writeStdin(  doc->line( l ), l < lastln );

  p->closeWhenDone();
}

void KateMwModOnHdDialog::slotPRead( KProcIO *p)
{
  // create a file for the diff if we haven't one already
  if ( ! m_tmpfile )
  {
    m_tmpfile = new KTemporaryFile();
    m_tmpfile->setAutoRemove(false);
    m_tmpfile->open();
  }

  // put all the data we have in it
  QString stmp;
  bool readData = false;
  QTextStream textStream ( m_tmpfile );
  while ( p->readln( stmp, false ) > -1 )
  {
    textStream << stmp << endl;
    readData = true;
  }
  textStream.flush();

  // dominik: only ackRead(), when we *really* read data, otherwise, this slot
  // is called infinity times, which leads to a crash (#123887)
  if (readData)
    p->ackRead();
}

void KateMwModOnHdDialog::slotPDone( KProcess *p )
{
  setCursor( Qt::ArrowCursor );

  // dominik: whitespace changes lead to diff with 0 bytes, so that slotPRead is
  // never called. Thus, m_tmpfile can be NULL
  if( m_tmpfile )
    m_tmpfile->close();

  if ( ! p->normalExit() /*|| p->exitStatus()*/ )
  {
    KMessageBox::sorry( this,
                        i18n("The diff command failed. Please make sure that "
                             "diff(1) is installed and in your PATH."),
                        i18n("Error Creating Diff") );
    delete m_tmpfile;
    m_tmpfile = 0;
    return;
  }

  if ( ! m_tmpfile )
  {
    KMessageBox::information( this,
                              i18n("Besides white space changes, the files are identical."),
                              i18n("Diff Output") );
    return;
  }

  KRun::runUrl( m_tmpfile->fileName(), "text/x-diff", this, true );
  delete m_tmpfile;
  m_tmpfile = 0;
}

// kate: space-indent on; indent-width 2; replace-tabs on;
