/* This file is part of the KDE project
   Copyright (C) 2004 Joseph Wenninger <jowenn@kde.org>
 
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

#include "katesavemodifieddialog.h"
#include "katesavemodifieddialog.moc"

#include <QList>
#include <QTreeWidget>
#include <QLabel>
#include <QPushButton>

#include <KLocale>
#include <KGuiItem>
#include <KStandardGuiItem>
#include <KVBox>
#include <KIconLoader>
#include <KMessageBox>
#include <kdebug.h>
#include <KEncodingFileDialog>

class AbstractKateSaveModifiedDialogCheckListItem: public QTreeWidgetItem
{
  public:
    AbstractKateSaveModifiedDialogCheckListItem(QTreeWidgetItem *parent, const QString& title, const QString& url): QTreeWidgetItem(parent)
    {
      setFlags(flags() | Qt::ItemIsUserCheckable);
      setText(0, title);
      setText(1, url);
      setCheckState(0, Qt::Checked);
      setState(InitialState);
    }
    virtual ~AbstractKateSaveModifiedDialogCheckListItem()
    {}
    virtual bool synchronousSave(QWidget *dialogParent) = 0;
    enum STATE{InitialState, SaveOKState, SaveFailedState};
    STATE state() const
    {
      return m_state;
    }
    void setState(enum STATE state)
    {
      m_state = state;
      KIconLoader *loader = KIconLoader::global();
      switch (state)
      {
        case InitialState:
          setIcon(0, QIcon());
          break;
        case SaveOKState:
          setIcon(0, loader->loadIcon("dialog-ok", KIconLoader::NoGroup,/*height()*/16));
          // "ok" icon should probably be "dialog-success", but we don't have that icon in KDE 4.0
          break;
        case SaveFailedState:
          setIcon(0, loader->loadIcon("dialog-error", KIconLoader::NoGroup,/*height()*/16));
          break;
      }
    }
  private:
    STATE m_state;
};

class KateSaveModifiedDocumentCheckListItem: public AbstractKateSaveModifiedDialogCheckListItem
{
  public:
    KateSaveModifiedDocumentCheckListItem(QTreeWidgetItem *parent, KTextEditor::Document *document)
        : AbstractKateSaveModifiedDialogCheckListItem(parent, document->documentName(), document->url().prettyUrl())
    {
      m_document = document;
    }
    virtual ~KateSaveModifiedDocumentCheckListItem()
    {}
    virtual bool synchronousSave(QWidget *dialogParent)
    {
      if (m_document->url().isEmpty() )
      {
        KEncodingFileDialog::Result r = KEncodingFileDialog::getSaveUrlAndEncoding(
                                          m_document->encoding(), QString(), QString(), dialogParent, i18n("Save As (%1)", m_document->documentName()));

        m_document->setEncoding( r.encoding );

        if (!r.URLs.isEmpty())
        {
          KUrl tmp = r.URLs.first();
          if ( !m_document->saveAs( tmp ) )
          {
            setState(SaveFailedState);
            setText(1, m_document->url().prettyUrl());
            return false;
          }
          else
          {
            bool sc = m_document->waitSaveComplete();
            setText(1, m_document->url().prettyUrl());
            if (!sc)
            {
              setState(SaveFailedState);
              return false;
            }
            else
            {
              setState(SaveOKState);
              return true;
            }
          }
        }
        else
        {
          setState(SaveFailedState);
          return false;
        }
      }
      else
      { //document has an exising location
        if ( !m_document->save() )
        {
          setState(SaveFailedState);
          setText(1, m_document->url().prettyUrl());
          return false;
        }
        else
        {
          bool sc = m_document->waitSaveComplete();
          setText(1, m_document->url().prettyUrl());
          if (!sc)
          {
            setState(SaveFailedState);
            return false;
          }
          else
          {
            setState(SaveOKState);
            return true;
          }
        }

      }

      return false;

    }
  private:
    KTextEditor::Document *m_document;
};

KateSaveModifiedDialog::KateSaveModifiedDialog(QWidget *parent, QList<KTextEditor::Document*> documents):
    KDialog( parent)
{

  setCaption( i18n("Save Documents") );
  setButtons( Yes | No | Cancel );
  setObjectName( "KateSaveModifiedDialog" );
  setModal( true );

  KGuiItem saveItem = KStandardGuiItem::save();
  saveItem.setText(i18n("&Save Selected"));
  setButtonGuiItem(KDialog::Yes, saveItem);
  connect(this, SIGNAL(yesClicked()), this, SLOT(slotSaveSelected()));

  setButtonGuiItem(KDialog::No, KStandardGuiItem::dontSave());
  connect(this, SIGNAL(noClicked()), this, SLOT(slotDoNotSave()));

  KGuiItem cancelItem = KStandardGuiItem::cancel();
  cancelItem.setText(i18n("&Abort Closing"));
  setButtonGuiItem(KDialog::Cancel, cancelItem);

  KVBox *box = new KVBox(this);
  setMainWidget(box);
  new QLabel(i18n("<qt>The following documents have been modified. Do you want to save them before closing?</qt>"), box);
  m_list = new QTreeWidget(box);
  /*m_list->addColumn(i18n("Title"));
  m_list->addColumn(i18n("Location"));*/
  m_list->setColumnCount(2);
  m_list->setHeaderLabels(QStringList() << i18n("Title") << i18n("Location"));
  m_list->setRootIsDecorated(true);
  //m_list->setResizeMode(Q3ListView::LastColumn);
  if (0)
  {
    m_projectRoot = new QTreeWidgetItem(m_list);
    m_projectRoot->setText(0, i18n("Projects"));
  }
  else m_projectRoot = 0;
  if (documents.count() > 0)
  {
    m_documentRoot = new QTreeWidgetItem(m_list);
    m_documentRoot->setText(0, i18n("Documents"));
    const uint docCnt = documents.count();
    for (uint i = 0;i < docCnt;i++)
    {
      new KateSaveModifiedDocumentCheckListItem(m_documentRoot, documents.at(i));
    }
    m_list->expandItem(m_documentRoot);
    m_list->resizeColumnToContents(0);
  }
  else m_documentRoot = 0;
  //FIXME - Is this the best way?
  connect(m_list, SIGNAL(itemActivated(QTreeWidgetItem *, int)), SLOT(slotItemActivated(QTreeWidgetItem *, int)));
  connect(m_list, SIGNAL(itemClicked(QTreeWidgetItem *, int)), SLOT(slotItemActivated(QTreeWidgetItem *, int)));
  connect(m_list, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), SLOT(slotItemActivated(QTreeWidgetItem *, int)));
  if(documents.count() > 3)
  { //For 3 or less, it would be quicker just to tick or untick them yourself, so don't clutter the gui.
    connect(new QPushButton(i18n("Se&lect All"), box), SIGNAL(clicked()), this, SLOT(slotSelectAll()));
  }
}

KateSaveModifiedDialog::~KateSaveModifiedDialog()
{}

void KateSaveModifiedDialog::slotItemActivated(QTreeWidgetItem*, int)
{
  kDebug(13001) << "slotItemSelected()";
  for(int cc = 0;cc < m_documentRoot->childCount();cc++)
  {
    if (m_documentRoot->child(cc)->checkState(0) == Qt::Checked)
    {
      enableButton(KDialog::Yes, true);
      return;
    }
  }
  enableButton(KDialog::Yes, false);
}

static void selectItems(QTreeWidgetItem *root)
{
  if (!root) return;
  for(int cc = 0;cc < root->childCount();cc++)
  {
    root->child(cc)->setCheckState(0, Qt::Checked);
  }
}

void KateSaveModifiedDialog::slotSelectAll()
{
  selectItems(m_documentRoot);
  slotItemActivated(0, 0);
}


void KateSaveModifiedDialog::slotSaveSelected()
{
  if (doSave(m_documentRoot)) done(QDialog::Accepted);
}

void KateSaveModifiedDialog::slotDoNotSave()
{
  done(QDialog::Accepted);
}

bool KateSaveModifiedDialog::doSave(QTreeWidgetItem *root)
{
kDebug() << root;
  if (root)
  {
kDebug() << root->childCount();
    for (int i = 0; i < root->childCount(); ++i)
    {
      AbstractKateSaveModifiedDialogCheckListItem * cit = (AbstractKateSaveModifiedDialogCheckListItem*)root->child(i);
      kDebug() << cit->checkState(0)<<  Qt::Checked<<  cit->state()<<  AbstractKateSaveModifiedDialogCheckListItem::SaveOKState;
      if (cit->checkState(0) == Qt::Checked && (cit->state() != AbstractKateSaveModifiedDialogCheckListItem::SaveOKState))
      {
        if (!cit->synchronousSave(this /*perhaps that should be the kate mainwindow*/))
        {
          KMessageBox::sorry( this, i18n("Data you requested to be saved could not be written. Please choose how you want to proceed."));
          return false;
        }
      }
      else if ((cit->checkState(0) != Qt::Checked) && (cit->state() == AbstractKateSaveModifiedDialogCheckListItem::SaveFailedState))
      {
        cit->setState(AbstractKateSaveModifiedDialogCheckListItem::InitialState);
      }

    }
  }
  return true;
}

bool KateSaveModifiedDialog::queryClose(QWidget *parent, QList<KTextEditor::Document*> documents)
{
  KateSaveModifiedDialog d(parent, documents);
  return (d.exec() != QDialog::Rejected);
}
// kate: space-indent on; indent-width 2; replace-tabs on;

