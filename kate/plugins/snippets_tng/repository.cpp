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

#include "repository.h"
#include "repository.moc"
#include "completionmodel.h"
#include <qcheckbox.h>
#include <qlabel.h>
#include <QFileInfo>
#include <kpushbutton.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <qtimer.h>
#include <qdbusconnection.h>
#include <ktexteditor/templateinterface.h>
#include <ktexteditor/view.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <kconfiggroup.h>
#include <krun.h>

namespace JoWenn {

  class KateSnippetRepositoryEntry {
    public:
      KateSnippetRepositoryEntry(const QString& _name, const QString& _filename, const QString& _fileType, const QString& _authors, const QString& _license, bool _systemFile, bool _enabled):
        name(_name),filename(_filename),fileType(_fileType), authors(_authors), license(_license),systemFile(_systemFile),enabled(_enabled){}
      ~KateSnippetRepositoryEntry(){}
      QString name;
      QString filename;
      QString fileType;
      QString authors;
      QString license;
      bool systemFile;
      bool enabled;    
  };
  
//BEGIN: Delegate  
  KateSnippetRepositoryItemDelegate::KateSnippetRepositoryItemDelegate(QAbstractItemView *itemView, QObject * parent):
      KWidgetItemDelegate(itemView,parent) {}
  
  KateSnippetRepositoryItemDelegate::~KateSnippetRepositoryItemDelegate(){}
  
  QList<QWidget*> KateSnippetRepositoryItemDelegate::createItemWidgets() const {
    QList<QWidget*> list;
    QCheckBox *checkbox=new QCheckBox();
    list<<checkbox;
    connect(checkbox,SIGNAL(stateChanged(int)),this,SLOT(enabledChanged(int)));
    
    list<<new QLabel();
    
    KPushButton *btn=new KPushButton();
    btn->setIcon(KIcon("document-edit"));
    list<<btn;
    connect(btn,SIGNAL(clicked()),this,SLOT(editEntry()));
    
    btn=new KPushButton();
    btn->setIcon(KIcon("edit-delete-page"));
    list<<btn;
    connect(btn,SIGNAL(clicked()),this,SLOT(deleteEntry()));
    return list;
  }

  void KateSnippetRepositoryItemDelegate::enabledChanged(int state) {
    const QModelIndex idx=focusedIndex();
    if (!idx.isValid()) return;
    const_cast<QAbstractItemModel*>(idx.model())->setData(idx,(bool)state,KateSnippetRepositoryModel::EnabledRole);
  }

  void KateSnippetRepositoryItemDelegate::editEntry() {
    const QModelIndex idx=focusedIndex();
    if (!idx.isValid()) return;
    const_cast<QAbstractItemModel*>(idx.model())->setData(idx,qVariantFromValue(qobject_cast<QWidget*>(parent())),KateSnippetRepositoryModel::EditNowRole);
  }
  
  void KateSnippetRepositoryItemDelegate::deleteEntry() {
    const QModelIndex idx=focusedIndex();
    if (!idx.isValid()) return;
    const_cast<QAbstractItemModel*>(idx.model())->setData(idx,qVariantFromValue(qobject_cast<QWidget*>(parent())),KateSnippetRepositoryModel::DeleteNowRole);
  }

#define SPACING 6
  void KateSnippetRepositoryItemDelegate::updateItemWidgets(const QList<QWidget*> widgets,
    const QStyleOptionViewItem &option,
    const QPersistentModelIndex &index) const {
    //CHECKBOX
    QCheckBox *checkBox = static_cast<QCheckBox*>(widgets[0]);
    checkBox->resize(checkBox->sizeHint());
    checkBox->move(SPACING, option.rect.height() / 2 - checkBox->sizeHint().height() / 2);

    //DELETE BUTTON
    KPushButton *btnDelete = static_cast<KPushButton*>(widgets[3]);
    btnDelete->resize(btnDelete->sizeHint());
    int btnW=btnDelete->sizeHint().width();
    btnDelete->move(option.rect.width()-SPACING-btnW, option.rect.height() / 2 - btnDelete->sizeHint().height() / 2);
    
    //EDIT BUTTON    
    KPushButton *btnEdit = static_cast<KPushButton*>(widgets[2]);
    btnEdit->resize(btnEdit->sizeHint());
    btnEdit->move(option.rect.width()-2*SPACING-btnW-btnEdit->sizeHint().width(), option.rect.height() / 2 - btnEdit->sizeHint().height() / 2);
    
    //NAME LABEL
    QLabel *label=static_cast<QLabel*>(widgets[1]);
    label->move(SPACING*2+checkBox->sizeHint().width(),option.rect.height()/2-option.fontMetrics.height()/* *2/2 */);
    label->resize(btnEdit->x()-SPACING-label->x(),option.fontMetrics.height()*2);
    
    //SETUP DATA
    if (!index.isValid()) {
        checkBox->setVisible(false);
        btnEdit->setVisible(false);
        btnDelete->setVisible(false);
        label->setVisible(false);
    } else {
        bool systemFile=index.model()->data(index, KateSnippetRepositoryModel::SystemFileRole).toBool();
        checkBox->setVisible(true);
        btnEdit->setVisible(!systemFile);
        btnDelete->setVisible(!systemFile);
        label->setVisible(true);
        checkBox->setChecked(index.model()->data(index, KateSnippetRepositoryModel::EnabledRole).toBool());
        //kDebug(13040)<<index.model()->data(index, KateSnippetRepositoryModel::NameRole).toString();
        QString fileType=index.model()->data(index, KateSnippetRepositoryModel::FiletypeRole).toString();
        if (fileType=="*") fileType="all file types";
        label->setText(i18n("%1 (%2)\nlicense: %3, authors: %4",
          index.model()->data(index, KateSnippetRepositoryModel::NameRole).toString(),
          fileType,
          index.model()->data(index, KateSnippetRepositoryModel::LicenseRole).toString(),
          index.model()->data(index, KateSnippetRepositoryModel::AuthorsRole).toString()
          )
        );
        //kDebug(13040)<<label->geometry();
        //kDebug(13040)<<btnEdit->x()-SPACING-label->x();
    }
  }

  void KateSnippetRepositoryItemDelegate::paint(QPainter * painter,
    const QStyleOptionViewItem & option, const QModelIndex & index) const {}
  
  QSize KateSnippetRepositoryItemDelegate::sizeHint(const QStyleOptionViewItem & option,
    const QModelIndex &) const {
    QSize size;
    size.setWidth(option.fontMetrics.height() * 8);
    size.setHeight(option.fontMetrics.height() * 2);
    return size;
  }
//END: Delegate

//BEGIN: Model
  KateSnippetRepositoryModel::KateSnippetRepositoryModel(QObject *parent):
    QAbstractListModel(parent) {
    createOrUpdateList(false);
    new KateSnippetRepositoryModelAdaptor(this);
    QString dbusObjectPath ("/Plugin/SnippetsTNG/Repository");
    QDBusConnection::sessionBus().registerObject( dbusObjectPath, this );
  }
  
  KateSnippetRepositoryModel::~KateSnippetRepositoryModel() {}
  int KateSnippetRepositoryModel::rowCount(const QModelIndex &/*not used*/) const {
    return m_entries.count();
  }
  
  void KateSnippetRepositoryModel::createOrUpdateList(bool update) {
        KConfig config ("katesnippets_tngrc", KConfig::NoGlobals);
    const QStringList list = KGlobal::dirs()->findAllResources("data",
      "kate/plugins/katesnippets_tng/data/*.xml",KStandardDirs::NoDuplicates);
    foreach(const QString& filename,list) {
      QString groupName="SnippetRepositoryAndConfigCache "+ filename;
      KConfigGroup group(&config, groupName);

      QString name;
      QString filetype;
      QString authors;
      QString license;
      bool systemFile=false;
      bool configRead=false;
      QFileInfo fi(filename);
      if (group.exists()) {         
         if (fi.lastModified()==group.readEntry("lastModified",QDateTime())) {
            name=group.readEntry("name");
            filetype=group.readEntry("filetype");
            authors=group.readEntry("authors");
            license=group.readEntry("license");         
            configRead=true;
         }
      }
      if (!configRead) {
         KateSnippetCompletionModel::loadHeader(filename,&name,&filetype,&authors,&license);
         group.writeEntry("lastModified",fi.lastModified());
         group.writeEntry("name",name);
         group.writeEntry("filetype",filetype);
         group.writeEntry("authors",authors);
         group.writeEntry("license",license);  
      }      
      name=i18nc("snippet name",name.toUtf8());
      if (update)
        updateEntry(name, filename, filetype, authors, license, systemFile);
      else
        addEntry(name, filename, filetype, authors, license, systemFile,/*enabled*/ false);
    }
    config.sync();
    reset();
    emit typeChanged("*");
  }
  
  
  
  QVariant KateSnippetRepositoryModel::data(const QModelIndex & index, int role) const {
    const KateSnippetRepositoryEntry& entry=m_entries[index.row()];
    switch(role) {
      case NameRole:
        return entry.name;
        break;
      case FilenameRole:
        return entry.filename;
        break;
      case FiletypeRole:
        return entry.fileType;
        break;
      case AuthorsRole:
        return entry.authors;
        break;
      case LicenseRole:
        return entry.license;
        break;        
      case SystemFileRole:
        return entry.systemFile;
        break;
      case EnabledRole:
        return entry.enabled;
        break;
      default:
        break;
    }
    return QVariant();
  }
  
  
  bool KateSnippetRepositoryModel::setData ( const QModelIndex & index, const QVariant & value, int role) {
      KateSnippetRepositoryEntry& entry=m_entries[index.row()];
      if (!index.isValid()) return false;
      switch (role) {
        case EnabledRole:
            entry.enabled=value.toBool();
            emit dataChanged(index,index);
            emit typeChanged(entry.fileType);
            return true;
            break;
        case DeleteNowRole:
          if (KMessageBox::warningYesNo(value.value<QWidget*>(),
                          i18n("Do you really want to delete the file '%1' from the repository? This action is irreversible!",entry.name),
                          i18n("Deleting snippet file"))==KMessageBox::Yes) {
            int remove=index.row();
            m_entries.removeAt(remove);
            reset(); //make the KWidgetItemDelegate happy, otherwise the widgets are not relayouted correctly with begin/endRemoveRows
          }
          return false;
          break;
        case EditNowRole:
          if (!KRun::runUrl(KUrl::fromPath(entry.filename),"application/x-katesnippets_tng",value.value<QWidget*>())) {
            KMessageBox::error(value.value<QWidget*>(),i18n("Editor application for file '%1' with mimetype 'application/x-katesnippets_tng' could not be started",entry.filename));
          }
          return false;
          break;
        default:
          break;
      }
      return QAbstractListModel::setData(index,value,role);
  }

  void KateSnippetRepositoryModel::newEntry() {
    QWidget *widget=qobject_cast<QWidget*>(sender());
    if (!KRun::runUrl(KUrl("new-file://"),"application/x-katesnippets_tng",widget)) {
      KMessageBox::error(widget,i18n("Editor application for new file with mimetype 'application/x-katesnippets_tng' could not be started"));
    }

  }

  void KateSnippetRepositoryModel::updateEntry(const QString& name, const QString& filename, const QString& filetype, const QString& authors, const QString& license, bool systemFile)
  {
      for (int i=0;i<m_entries.count();i++) {
          KateSnippetRepositoryEntry& entry=m_entries[i];
          if (entry.filename==filename)
          {
            entry.name=name;
            entry.fileType=filetype;
            entry.authors=authors;
            entry.license=license;
            entry.systemFile=systemFile;
            return;
          }
      }
      addEntry(name, filename, filetype, authors, license, systemFile, /*enabled*/ false);
  }
  void KateSnippetRepositoryModel::addEntry(const QString& name, const QString& filename, const QString& filetype, const QString& authors, const QString& license, bool systemFile, bool enabled) {
    beginInsertRows(QModelIndex(), m_entries.count(), m_entries.count());
    m_entries.append(KateSnippetRepositoryEntry(name,filename,filetype,authors,license,systemFile,enabled));
    endInsertRows();
  }

  KTextEditor::CodeCompletionModel2* KateSnippetRepositoryModel::completionModel(const QString &filetype) {
    kDebug(13040)<<"Creating a new completion model";
    kDebug(13040)<<"**************************************************************************************************************************"<<filetype;
    QStringList l;
    foreach(const KateSnippetRepositoryEntry& entry, m_entries) {
      if ((entry.enabled==true) && ( (entry.fileType=="*") || (entry.fileType==filetype))) {
        l<<entry.filename;
      }
    }
    return new KateSnippetCompletionModel(l);

  }
 
  void KateSnippetRepositoryModel::readSessionConfig (KConfigBase* config, const QString& groupPrefix) {
    QSet<QString> enabledSet;
    KConfigGroup group(config,groupPrefix+"enabled-snippets");
    int enabledCount=group.readEntry("count",0);
    for (int i=0;i<enabledCount;i++)
      enabledSet.insert(group.readPathEntry(QString("enabled_%1").arg(i),""));
    for (int i=0;i<m_entries.count();i++) {
      KateSnippetRepositoryEntry& entry=m_entries[i];
      entry.enabled=enabledSet.contains(entry.filename);
    }
  }
  
  void KateSnippetRepositoryModel::writeSessionConfig (KConfigBase* config, const QString& groupPrefix) {
    KConfigGroup group(config,groupPrefix+"enabled-snippets");
    group.deleteGroup();
    int enabledCount=0;
    for (int i=0;i<m_entries.count();i++) {
      const KateSnippetRepositoryEntry& entry=m_entries[i];
      if (entry.enabled) {
        group.writePathEntry(QString("enabled_%1").arg(enabledCount),entry.filename);
        enabledCount++;
      }
    }
    group.writeEntry("count",enabledCount);
    group.sync();
  }
//END: Model


//BEGIN: DBus Adaptor

  KateSnippetRepositoryModelAdaptor::KateSnippetRepositoryModelAdaptor(KateSnippetRepositoryModel *repository):
    QDBusAbstractAdaptor(repository),m_repository(repository) {}
  
  KateSnippetRepositoryModelAdaptor::~KateSnippetRepositoryModelAdaptor() {}
  
  void KateSnippetRepositoryModelAdaptor::updateSnippetRepository()
  {
    m_repository->createOrUpdateList(true);
  }
 

//END: DBus Adaptor

}
// kate: space-indent on; indent-width 2; replace-tabs on;
