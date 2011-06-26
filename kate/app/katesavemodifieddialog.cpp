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
    AbstractKateSaveModifiedDialogCheckListItem(const QString& title, const QString& url): QTreeWidgetItem()
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
    KateSaveModifiedDocumentCheckListItem(KTextEditor::Document *document)
        : AbstractKateSaveModifiedDialogCheckListItem(document->documentName(), document->url().prettyUrl())
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

        if (!r.URLs.isEmpty())
        {
          KUrl tmp = r.URLs.first();

          // check for overwriting a file
          if( tmp.isLocalFile() )
          {
            QFileInfo info( tmp.path() );
            if( info.exists() ) {
             if (KMessageBox::Cancel == KMessageBox::warningContinueCancel( dialogParent,
              i18n( "A file named \"%1\" already exists. "
                    "Are you sure you want to overwrite it?" ,  info.fileName() ),
              i18n( "Overwrite File?" ), KStandardGuiItem::overwrite(),
              KStandardGuiItem::cancel(), QString(), KMessageBox::Notify | KMessageBox::Dangerous ))
             {
                setState(SaveFailedState);
                return false;
             }
            }
          }
          
          m_document->setEncoding( r.encoding );

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
  setButtons( User1 | User2 | Cancel );
  setObjectName( "KateSaveModifiedDialog" );
  setModal( true );

  KGuiItem saveItem = KStandardGuiItem::save();
  saveItem.setText(i18n("&Save Selected"));
  setButtonGuiItem(KDialog::User1, saveItem);
  connect(this, SIGNAL(user1Clicked()), this, SLOT(slotSaveSelected()));

  setButtonGuiItem(KDialog::User2, KStandardGuiItem::dontSave());
  connect(this, SIGNAL(user2Clicked()), this, SLOT(slotDoNotSave()));

  KGuiItem cancelItem = KStandardGuiItem::cancel();
  cancelItem.setText(i18n("Do &Not Close"));
  setButtonGuiItem(KDialog::Cancel, cancelItem);
  setDefaultButton(KDialog::Cancel);
  setButtonFocus(KDialog::Cancel);
  
  KVBox *box = new KVBox(this);
  setMainWidget(box);
  new QLabel(i18n("<qt>The following documents have been modified. Do you want to save them before closing?</qt>"), box);
  m_list = new QTreeWidget(box);
  m_list->setColumnCount(2);
  m_list->setHeaderLabels(QStringList() << i18n("Documents") << i18n("Location"));
  m_list->setRootIsDecorated(true);

  foreach (KTextEditor::Document* doc, documents) {
    m_list->addTopLevelItem(new KateSaveModifiedDocumentCheckListItem(doc));
  }
  m_list->resizeColumnToContents(0);

  connect(m_list, SIGNAL(itemChanged(QTreeWidgetItem *, int)), SLOT(slotItemActivated(QTreeWidgetItem *, int)));
  connect(new QPushButton(i18n("Se&lect All"), box), SIGNAL(clicked()), this, SLOT(slotSelectAll()));
}

KateSaveModifiedDialog::~KateSaveModifiedDialog()
{}

void KateSaveModifiedDialog::slotItemActivated(QTreeWidgetItem*, int)
{
  bool enableSaveButton = false;

  for (int i = 0; i < m_list->topLevelItemCount(); ++i) {
    if (m_list->topLevelItem(i)->checkState(0) == Qt::Checked) {
      enableSaveButton = true;
      break;
    }
  }
  
  // enable or disable the Save button
  enableButton(KDialog::User1, enableSaveButton);
}

void KateSaveModifiedDialog::slotSelectAll()
{
  for (int i = 0; i < m_list->topLevelItemCount(); ++i) {
    m_list->topLevelItem(i)->setCheckState(0, Qt::Checked);
  }

  // enable Save button
  enableButton(KDialog::User1, true);
}


void KateSaveModifiedDialog::slotSaveSelected()
{
  if (doSave()) done(QDialog::Accepted);
}

void KateSaveModifiedDialog::slotDoNotSave()
{
  done(QDialog::Accepted);
}

bool KateSaveModifiedDialog::doSave()
{
  for (int i = 0; i < m_list->topLevelItemCount(); ++i) {
    AbstractKateSaveModifiedDialogCheckListItem * cit = (AbstractKateSaveModifiedDialogCheckListItem*)m_list->topLevelItem(i);

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

  return true;
}

bool KateSaveModifiedDialog::queryClose(QWidget *parent, QList<KTextEditor::Document*> documents)
{
  KateSaveModifiedDialog d(parent, documents);
  return (d.exec() != QDialog::Rejected);
}
// kate: space-indent on; indent-width 2; replace-tabs on;

