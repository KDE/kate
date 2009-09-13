/* This file is part of the KDE project
 * Copyright (C) 2009 Joseph Wenninger <jowenn@kde.org>
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
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#include "snippeteditorwindow.h"
#include "snippeteditorwindow.moc"
#include "editorapp.h"
#include "snippeteditornewdialog.h"
#include "../completionmodel.h"
#include <kmessagebox.h>
#include <QDBusConnectionInterface>

using namespace JoWenn;

SnippetEditorWindow::SnippetEditorWindow(const QStringList &modes, const KUrl& url): KMainWindow(0), Ui::SnippetEditorView(),m_modified(false),m_url(url),m_snippetData(0),m_selectorModel(0)
{  
  if (!m_url.isLocalFile()) {
    m_ok=false;
    SnippetEditorNewDialog nd(this);
    if (nd.exec()==QDialog::Rejected) return;
    QString newPath=KateSnippetCompletionModel::createNew(nd.snippetCollectionName->text(),nd.snippetCollectionLicense->currentText(),nd.snippetCollectionAuthors->text());
    if (newPath.isEmpty()) return;
    m_url=KUrl::fromPath(newPath);
    notifyChange();
  }
  m_ok=true;
  QWidget *widget=new QWidget(this);
  setCentralWidget(widget);
  setupUi(widget);
  connect(buttonBox,SIGNAL(clicked(QAbstractButton*)),this,SLOT(slotClose(QAbstractButton*)));
  buttonBox->button(QDialogButtonBox::Save)->setEnabled(false);
  addSnippet->setIcon(KIcon("document-new"));
  delSnippet->setIcon(KIcon("edit-delete-page"));  
  connect(snippetCollectionAuthors,SIGNAL(textEdited(const QString&)),this,SLOT(modified()));
  connect(snippetPrefix,SIGNAL(textEdited(const QString&)),this,SLOT(modified()));
  connect(snippetPrefix,SIGNAL(textEdited(const QString&)),this,SLOT(modified()));
  connect(snippetMatch,SIGNAL(textEdited(const QString&)),this,SLOT(modified()));
  connect(snippetPostfix,SIGNAL(textEdited(const QString&)),this,SLOT(modified()));
  connect(snippetArguments,SIGNAL(textEdited(const QString&)),this,SLOT(modified()));
  connect(snippetContent,SIGNAL(textChanged()),this,SLOT(modified()));
  connect(delSnippet,SIGNAL(clicked()),this,SLOT(deleteSnippet()));
  connect(addSnippet,SIGNAL(clicked()),this,SLOT(newSnippet()));
  QString name;
  QString filetype;
  QString authors;
  QString license;

  KateSnippetCompletionModel::loadHeader(m_url.toLocalFile(),&name, &filetype, &authors, &license);    
  setCaption(m_url.fileName());  

  snippetCollectionName->setText(name);
  snippetCollectionAuthors->setText(authors);
  snippetCollectionLicense->setText(license);
  snippetCollectionFiletype->clear();
  snippetCollectionFiletype->addItem("*");
  snippetCollectionFiletype->addItems(modes);
  if (filetype=="*")
    snippetCollectionFiletype->setCurrentIndex(0);
  else
  {
    int idx=modes.indexOf(filetype);
    if (idx!=-1)
      snippetCollectionFiletype->setCurrentIndex(idx+1);
  }
  QStringList files;
  files<<m_url.toLocalFile();
  m_snippetData=new KateSnippetCompletionModel(files);
  m_selectorModel=m_snippetData->selectorModel();
  snippetListView->setModel(m_selectorModel);
  connect(snippetListView->selectionModel(),SIGNAL(currentChanged(const QModelIndex&,const QModelIndex&)),this,SLOT(currentChanged(const QModelIndex&, const QModelIndex&)));
  currentChanged(QModelIndex(),QModelIndex());
}

SnippetEditorWindow::~SnippetEditorWindow()
{
}

void SnippetEditorWindow::slotClose(QAbstractButton* button) {
  if (button==buttonBox->button(QDialogButtonBox::Save)) {
    QModelIndex previous=snippetListView->selectionModel()->currentIndex();
    if (previous.isValid()) {      
      m_selectorModel->setData(previous,snippetPrefix->text(),KateSnippetSelectorModel::PrefixRole);
      m_selectorModel->setData(previous,snippetMatch->text(),KateSnippetSelectorModel::MatchRole);
      m_selectorModel->setData(previous,snippetPostfix->text(),KateSnippetSelectorModel::PostfixRole);
      m_selectorModel->setData(previous,snippetArguments->text(),KateSnippetSelectorModel::ArgumentsRole);
      m_selectorModel->setData(previous,snippetContent->toPlainText(),KateSnippetSelectorModel::FillInRole);
    }  
    
    if (m_snippetData->save(m_url.toLocalFile(),snippetCollectionName->text(),snippetCollectionLicense->text(),snippetCollectionFiletype->currentText(),snippetCollectionAuthors->text())) {
      notifyChange();
      close();
    }
  } else if (button==buttonBox->button(QDialogButtonBox::Close)) {
    close();
  }
}

void SnippetEditorWindow::currentChanged(const QModelIndex& current, const QModelIndex& previous) {
  if (previous.isValid()) {
    m_selectorModel->setData(previous,snippetPrefix->text(),KateSnippetSelectorModel::PrefixRole);
    m_selectorModel->setData(previous,snippetMatch->text(),KateSnippetSelectorModel::MatchRole);
    m_selectorModel->setData(previous,snippetPostfix->text(),KateSnippetSelectorModel::PostfixRole);
    m_selectorModel->setData(previous,snippetArguments->text(),KateSnippetSelectorModel::ArgumentsRole);
    m_selectorModel->setData(previous,snippetContent->toPlainText(),KateSnippetSelectorModel::FillInRole);
  }
  snippetPrefix->setText(m_selectorModel->data(current,KateSnippetSelectorModel::PrefixRole).toString());
  snippetMatch->setText(m_selectorModel->data(current,KateSnippetSelectorModel::MatchRole).toString());
  snippetPostfix->setText(m_selectorModel->data(current,KateSnippetSelectorModel::PostfixRole).toString());
  snippetArguments->setText(m_selectorModel->data(current,KateSnippetSelectorModel::ArgumentsRole).toString());
  disconnect(snippetContent,SIGNAL(textChanged()),this,SLOT(modified()));
  snippetContent->setPlainText(m_selectorModel->data(current,KateSnippetSelectorModel::FillInRole).toString());
  connect(snippetContent,SIGNAL(textChanged()),this,SLOT(modified()));
  bool enabled=current.isValid();
  snippetPrefix->setEnabled(enabled);
  snippetMatch->setEnabled(enabled);
  snippetPostfix->setEnabled(enabled);
  snippetArguments->setEnabled(enabled);
  snippetContent->setEnabled(enabled);

}

void SnippetEditorWindow::modified() {  
  setCaption(m_url.fileName(),true);
  buttonBox->button(QDialogButtonBox::Save)->setEnabled(true);
}

void SnippetEditorWindow::deleteSnippet() {
    QModelIndex row=snippetListView->selectionModel()->currentIndex();
    if (row.isValid()) {
      m_selectorModel->removeRow(row.row(),QModelIndex());
      modified();
    }
}

void SnippetEditorWindow::newSnippet() {
  QModelIndex row=m_selectorModel->newItem();
  snippetListView->selectionModel()->setCurrentIndex(snippetListView->selectionModel()->currentIndex(),QItemSelectionModel::Clear);
  snippetListView->selectionModel()->setCurrentIndex(row,QItemSelectionModel::SelectCurrent);
  modified();
}

void SnippetEditorWindow::notifyChange() {
  QDBusConnectionInterface *interface=QDBusConnection::sessionBus().interface();
  if (!interface) return;
  QStringList serviceNames = interface->registeredServiceNames();
  foreach(const QString serviceName,serviceNames) {
    if (serviceName.startsWith("org.kde.kate-")) {
      QDBusMessage m = QDBusMessage::createMethodCall (serviceName,
      QLatin1String("/Plugin/SnippetsTNG/Repository"), "org.kde.Kate.Plugin.SnippetsTNG.Repository", "updateSnippetRepository");
      QDBusConnection::sessionBus().call (m);
    }
  }
}