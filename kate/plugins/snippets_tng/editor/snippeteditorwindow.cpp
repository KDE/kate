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
#include "../lib/completionmodel.h"
#include "../lib/dbus_helpers.h"
#include <kmessagebox.h>
#include <QDBusConnectionInterface>

#include <QStandardItem>

using namespace KTextEditor::CodesnippetsCore::Editor;


FiletypeListDropDown::FiletypeListDropDown(QWidget *parent,KLineEdit *lineEdit,KPushButton *btn,const QStringList &modes):
  QWidget(parent),Ui::FiletypeListCreatorview(), m_lineEdit(lineEdit),m_btn(btn),m_modes(modes)
{
  setupUi(this);
  hide();
  filetypeCombo->clear();
  filetypeCombo->addItems(modes);
  connect(filetypeAddButton,SIGNAL(clicked()),this,SLOT(addFileType()));
  filetypeUsedList->setModel(&m_model);
}

FiletypeListDropDown::~FiletypeListDropDown() {
}

void FiletypeListDropDown::parseAndShow() {
  if (isVisibleTo(parentWidget()))
  {
    m_lineEdit->removeEventFilter(this);
    hide();
    return;
  }
    
  int x=m_lineEdit->pos().x();
  int y=m_lineEdit->pos().y()+m_lineEdit->height();
  int width=m_btn->pos().x()+m_btn->width()-x;
  setGeometry(x,y,width,height());
  m_lineEdit->installEventFilter(this);
  
  m_model.clear();
  
  QString txt=m_lineEdit->text().trimmed();
  QString catchall("*");
  if (txt.isEmpty()) txt=catchall;
  if (txt!=catchall) {
    QStringList src=txt.split(";");
    QStringList dst;
    foreach(const QString &item_str,src) {
      QString item_str_trimmed=item_str.trimmed();      
      if (item_str_trimmed.isEmpty()) continue;
      //if (item_str_trimmed==catchall) continue;
        dst<<item_str_trimmed;
    }
    dst.removeDuplicates();
    txt=dst.join(";");
    if (txt!=catchall) {
      foreach (const QString &item_str,dst) {
        m_model.appendRow(new QStandardItem(item_str));
      }
    }
  }
  if (txt!=m_lineEdit->text().trimmed()) {
    m_lineEdit->setText(txt);
    emit modified();
  }
  show();
}

void FiletypeListDropDown::addFileType() {
  m_model.appendRow(new QStandardItem(filetypeCombo->currentText()));
  QStringList items;
  int rowCount=m_model.rowCount();
  for (int i=0;i<rowCount;i++) {
    items<<m_model.data(m_model.index(i,0)).toString();
  }
  items.removeDuplicates();
  m_lineEdit->setText(items.join(";"));
  emit modified();
}

bool FiletypeListDropDown::eventFilter(QObject *obj,QEvent *event) {
  if (event->type()==QEvent::FocusIn) parseAndShow();
  return false;
}


SnippetEditorWindow::SnippetEditorWindow(const QStringList &modes, const KUrl& url): KMainWindow(0), Ui::SnippetEditorView(),m_modified(false),m_url(url),m_snippetData(0),m_selectorModel(0)
{  
  if (!m_url.isLocalFile()) {
    kDebug()<<m_url.query();    
    QString token=m_url.queryItem("token");
    QString service=m_url.queryItem("service");
    QString object=m_url.queryItem("object");
    kDebug()<<token;
    kDebug()<<service;
    kDebug()<<object;
    m_ok=false;
    SnippetEditorNewDialog nd(this);
    if (nd.exec()==QDialog::Rejected) 
    {
      if (!token.isEmpty())
        notifyTokenNewHandled(token,service,object,"");
      return;
    }
    QString new_type=m_url.path();
    new_type=new_type.right(new_type.length()-1);
    QString newPath=KTextEditor::CodesnippetsCore::Editor::SnippetCompletionModel::createNew(nd.snippetCollectionName->text(),nd.snippetCollectionLicense->currentText(),nd.snippetCollectionAuthors->text(),new_type);
    if (newPath.isEmpty()) 
    {
      if (!token.isEmpty())
        notifyTokenNewHandled(token,service,object,"");
      return;
    }
    m_url=KUrl::fromPath(newPath);
    notifyRepos();
    if (!token.isEmpty())
      notifyTokenNewHandled(token,service,object,newPath);
  }
  m_ok=true;
  QWidget *widget=new QWidget(this);
  setCentralWidget(widget);
  setupUi(widget);
  m_filetypeDropDown=new FiletypeListDropDown(this,snippetCollectionFiletype,snippetCollectionFiletypeButton,modes);
  connect(snippetCollectionFiletypeButton,SIGNAL(clicked()),m_filetypeDropDown,SLOT(parseAndShow()));
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
  connect(snippetCollectionFiletype,SIGNAL(textEdited(const QString&)),this,SLOT(modified()));
  connect(m_filetypeDropDown,SIGNAL(modified()),this,SLOT(modified()));
  QString name;
  QString filetype;
  QString authors;
  QString license;

  KTextEditor::CodesnippetsCore::Editor::SnippetCompletionModel::loadHeader(m_url.toLocalFile(),&name, &filetype, &authors, &license,&m_snippetLicense);
  if (m_snippetLicense.isEmpty()) m_snippetLicense="public domain";
  setCaption(m_url.fileName());  

  snippetCollectionName->setText(name);
  snippetCollectionAuthors->setText(authors);
  snippetCollectionLicense->setText(license);
  snippetCollectionFiletype->setText(filetype);
  
  QStringList files;
  files<<m_url.toLocalFile();
  m_snippetData=new KTextEditor::CodesnippetsCore::Editor::SnippetCompletionModel("",files);
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
      m_selectorModel->setData(previous,snippetPrefix->text(),SnippetSelectorModel::PrefixRole);
      m_selectorModel->setData(previous,snippetMatch->text(),SnippetSelectorModel::MatchRole);
      m_selectorModel->setData(previous,snippetPostfix->text(),SnippetSelectorModel::PostfixRole);
      m_selectorModel->setData(previous,snippetArguments->text(),SnippetSelectorModel::ArgumentsRole);
      m_selectorModel->setData(previous,snippetContent->toPlainText(),SnippetSelectorModel::FillInRole);
    }  
    
    QString filetype=snippetCollectionFiletype->text();
    if (filetype.trimmed().isEmpty()) filetype=QString("*");
    if (m_snippetData->save(m_url.toLocalFile(),snippetCollectionName->text(),snippetCollectionLicense->text(),filetype,snippetCollectionAuthors->text(),m_snippetLicense)) {
      notifyRepos();
      close();
    }
  } else if (button==buttonBox->button(QDialogButtonBox::Close)) {
    close();
  }
}

void SnippetEditorWindow::currentChanged(const QModelIndex& current, const QModelIndex& previous) {
  if (previous.isValid()) {
    m_selectorModel->setData(previous,snippetPrefix->text(),SnippetSelectorModel::PrefixRole);
    m_selectorModel->setData(previous,snippetMatch->text(),SnippetSelectorModel::MatchRole);
    m_selectorModel->setData(previous,snippetPostfix->text(),SnippetSelectorModel::PostfixRole);
    m_selectorModel->setData(previous,snippetArguments->text(),SnippetSelectorModel::ArgumentsRole);
    m_selectorModel->setData(previous,snippetContent->toPlainText(),SnippetSelectorModel::FillInRole);
  }
  snippetPrefix->setText(m_selectorModel->data(current,SnippetSelectorModel::PrefixRole).toString());
  snippetMatch->setText(m_selectorModel->data(current,SnippetSelectorModel::MatchRole).toString());
  snippetPostfix->setText(m_selectorModel->data(current,SnippetSelectorModel::PostfixRole).toString());
  snippetArguments->setText(m_selectorModel->data(current,SnippetSelectorModel::ArgumentsRole).toString());
  disconnect(snippetContent,SIGNAL(textChanged()),this,SLOT(modified()));
  snippetContent->setPlainText(m_selectorModel->data(current,SnippetSelectorModel::FillInRole).toString());
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

// kate: space-indent on; indent-width 2; replace-tabs on;