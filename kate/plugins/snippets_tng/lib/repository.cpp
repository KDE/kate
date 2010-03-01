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
#include "dbus_helpers.h"
#include "ui_snippet_repository.h"
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
#include <kio/netaccess.h>
#include <knewstuff3/downloaddialog.h>
#include <QDBusConnectionInterface>
#include <QDBusConnection>
#include <QDBusMessage>
#include <kcolorscheme.h>
#include <QUuid>
#include <unistd.h>
#include <ktemporaryfile.h>

namespace KTextEditor {
  namespace CodesnippetsCore {
    
    class SnippetRepositoryEntry {
      public:
        SnippetRepositoryEntry(const QString& _name, const QString& _filename, const QString& _fileType, const QString& _authors, const QString& _license, const QString& _snippetLicense, bool _systemFile, bool _ghnsFile,bool _enabled):
          name(_name),filename(_filename), authors(_authors), license(_license),snippetLicense(_snippetLicense),systemFile(_systemFile),ghnsFile(_ghnsFile),enabled(_enabled){
            setFileType(_fileType.split(";"));
          }
        ~SnippetRepositoryEntry(){}
        QString name;
        QString filename;      
        QString authors;
        QString license;
        QString snippetLicense;
        bool systemFile;
        bool ghnsFile;
        bool enabled;
        void setFileType(const QStringList &list) {
          m_fileType.clear();
          foreach(const QString &str,list) {
            m_fileType<<str.trimmed();
          }
          if (m_fileType.count()==0) m_fileType<<"*";
        }
        const QStringList& fileType() const {return m_fileType;}
      private:
        QStringList m_fileType;
    };
  
//BEGIN: Delegate  
    SnippetRepositoryItemDelegate::SnippetRepositoryItemDelegate(QAbstractItemView *itemView, QObject * parent):
        KWidgetItemDelegate(itemView,parent) {}
    
    SnippetRepositoryItemDelegate::~SnippetRepositoryItemDelegate(){}
    
    QList<QWidget*> SnippetRepositoryItemDelegate::createItemWidgets() const {
      QList<QWidget*> list;
      QCheckBox *checkbox=new QCheckBox();
      list<<checkbox;
      connect(checkbox,SIGNAL(stateChanged(int)),this,SLOT(enabledChanged(int)));
      list<<new QLabel();
      
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

    void SnippetRepositoryItemDelegate::enabledChanged(int state) {
      const QModelIndex idx=focusedIndex();
      if (!idx.isValid()) return;
      const_cast<QAbstractItemModel*>(idx.model())->setData(idx,(bool)state,SnippetRepositoryModel::EnabledRole);
    }

    void SnippetRepositoryItemDelegate::editEntry() {
      const QModelIndex idx=focusedIndex();
      if (!idx.isValid()) return;
      const_cast<QAbstractItemModel*>(idx.model())->setData(idx,qVariantFromValue(qobject_cast<QWidget*>(parent())),SnippetRepositoryModel::EditNowRole);
    }
    
    void SnippetRepositoryItemDelegate::deleteEntry() {
      const QModelIndex idx=focusedIndex();
      if (!idx.isValid()) return;
      const_cast<QAbstractItemModel*>(idx.model())->setData(idx,qVariantFromValue(qobject_cast<QWidget*>(parent())),SnippetRepositoryModel::DeleteNowRole);
    }

  #define SPACING 6
    void SnippetRepositoryItemDelegate::updateItemWidgets(const QList<QWidget*> widgets,
      const QStyleOptionViewItem &option,
      const QPersistentModelIndex &index) const {
      //CHECKBOX
      QCheckBox *checkBox = static_cast<QCheckBox*>(widgets[0]);
      checkBox->resize(checkBox->sizeHint());
      checkBox->move(SPACING, option.rect.height() / 2 - checkBox->sizeHint().height() / 2);

      //TYPE
      QLabel *typeicon=static_cast<QLabel*>(widgets[1]);
      typeicon->move(SPACING*2+checkBox->sizeHint().width(),option.rect.height()/2-checkBox->sizeHint().height() / 2);
      typeicon->resize(checkBox->sizeHint().height(),checkBox->sizeHint().height());
           
      //DELETE BUTTON
      KPushButton *btnDelete = static_cast<KPushButton*>(widgets[4]);
      btnDelete->resize(btnDelete->sizeHint());
      int btnW=btnDelete->sizeHint().width();
      btnDelete->move(option.rect.width()-SPACING-btnW, option.rect.height() / 2 - btnDelete->sizeHint().height() / 2);
      
      //EDIT BUTTON    
      KPushButton *btnEdit = static_cast<KPushButton*>(widgets[3]);
      btnEdit->resize(btnEdit->sizeHint());
      btnEdit->move(option.rect.width()-2*SPACING-btnW-btnEdit->sizeHint().width(), option.rect.height() / 2 - btnEdit->sizeHint().height() / 2);
      
      //NAME LABEL
      QLabel *label=static_cast<QLabel*>(widgets[2]);
      label->move(SPACING*3+2*checkBox->sizeHint().width(),option.rect.height()/2-option.fontMetrics.height()/* *2/2 */);
      label->resize(btnEdit->x()-SPACING-label->x(),option.fontMetrics.height()*4);
      
      //SETUP DATA
      if (!index.isValid()) {
          checkBox->setVisible(false);
          btnEdit->setVisible(false);
          btnDelete->setVisible(false);
          label->setVisible(false);
          typeicon->setVisible(false);
      } else {
          bool systemFile=index.model()->data(index, SnippetRepositoryModel::SystemFileRole).toBool();
          bool ghnsFile=index.model()->data(index, SnippetRepositoryModel::GhnsFileRole).toBool();
          checkBox->setVisible(true);
          btnEdit->setVisible(!systemFile);
          btnDelete->setVisible((!systemFile) && !(ghnsFile));
          label->setVisible(true);
          typeicon->setVisible(true);
          kDebug()<<"GHNSFILE==="<<ghnsFile;
          if (systemFile) {
            typeicon->setPixmap(KIcon("folder-red").pixmap(16,16));
          } else {
            if (ghnsFile) typeicon->setPixmap(KIcon("get-hot-new-stuff").pixmap(16,16));
            else typeicon->setPixmap(KIcon("user-home").pixmap(16,16));
          }
          checkBox->setChecked(index.model()->data(index, SnippetRepositoryModel::EnabledRole).toBool());
          //kDebug(13040)<<index.model()->data(index, KateSnippetRepositoryModel::NameRole).toString();
          QStringList fileType=index.model()->data(index, SnippetRepositoryModel::FiletypeRole).toStringList();
          QString displayFileType=fileType.join(";");
          if (fileType.contains("*")) displayFileType="all file types";
          QString snippetLicense=index.model()->data(index, SnippetRepositoryModel::SnippetLicenseRole).toString();
          bool tainted=false;
          if (snippetLicense!="public domain") {
            snippetLicense=i18n("!TAINTED!:%1",snippetLicense);
            tainted=true;   
            QPalette palette = label->palette();
            palette.setColor( label->foregroundRole(),
              KColorScheme(QPalette::Active, KColorScheme::Window).
                foreground(KColorScheme::NegativeText).color() );
            label->setPalette( palette );
            label->update();
            
          }
          label->setText(i18n("%1 (%2)\ncontent license: %3\nrepository license: %4 authors: %5",
            index.model()->data(index, SnippetRepositoryModel::NameRole).toString(),
            displayFileType,
            snippetLicense,
            index.model()->data(index, SnippetRepositoryModel::LicenseRole).toString(),
            index.model()->data(index, SnippetRepositoryModel::AuthorsRole).toString()
            )
          );
          QFont f=label->font();
          f.setItalic(tainted);
          label->setFont(f);
          //kDebug(13040)<<label->geometry();
          //kDebug(13040)<<btnEdit->x()-SPACING-label->x();
      }
    }

    void SnippetRepositoryItemDelegate::paint(QPainter * painter,
      const QStyleOptionViewItem & option, const QModelIndex & index) const {}
    
    QSize SnippetRepositoryItemDelegate::sizeHint(const QStyleOptionViewItem & option,
      const QModelIndex &) const {
      QSize size;
      size.setWidth(option.fontMetrics.height() * 8);
      size.setHeight(option.fontMetrics.height() * 4);
      return size;
    }
//END: Delegate

//BEGIN: Model
    long SnippetRepositoryModel::s_id=0;

    SnippetRepositoryModel::SnippetRepositoryModel(QObject *parent):
      QAbstractListModel(parent),m_connection(QDBusConnection::connectToBus(QDBusConnection::SessionBus,"KTECSCRepoConn"))
    {
      createOrUpdateList(false);
      kDebug()<<m_connection.lastError().message();
      Q_ASSERT(m_connection.isConnected());
      m_dbusServiceName=QString("org.kde.ktecodesnippetscore-%1-%2").arg(getpid()).arg(++s_id);
      kDebug()<<m_dbusServiceName;
      bool register_service=m_connection.registerService(m_dbusServiceName);
      kDebug()<<m_connection.lastError().message();
      Q_ASSERT(register_service);
      new SnippetRepositoryModelAdaptor(this);
      m_dbusObjectPath=QString("/Repository");
      m_connection.registerObject( m_dbusObjectPath, this );
    }
    
    SnippetRepositoryModel::~SnippetRepositoryModel() {}
    int SnippetRepositoryModel::rowCount(const QModelIndex &/*not used*/) const {
      return m_entries.count();
    }
    
       
    void SnippetRepositoryModel::createOrUpdateList(bool update) {
      kDebug()<<"BEGIN";
      KConfig config ("katesnippets_tngrc", KConfig::NoGlobals);

      createOrUpdateListSub(config,KGlobal::dirs()->findAllResources("data", "ktexteditor_snippets/data/*.xml",KStandardDirs::NoDuplicates),update,false);
      createOrUpdateListSub(config,KGlobal::dirs()->findAllResources("data", "ktexteditor_snippets/ghns/*.xml",KStandardDirs::NoDuplicates),update,true);

      config.sync();
      reset();
      emit typeChanged(QStringList("*"));
      kDebug()<<"END";
    }
    
    void SnippetRepositoryModel::createOrUpdateListSub(KConfig& config,QStringList list, bool update, bool ghnsFile) {
      foreach(const QString& filename,list) {
        QString groupName="SnippetRepositoryAndConfigCache "+ filename;
        KConfigGroup group(&config, groupName);

        QString name;
        QString filetype;
        QString authors;
        QString license;
        QString snippetlicense;
        bool systemFile=false;
        bool configRead=false;
        QFileInfo fi(filename);
        if (group.exists()) {         
          if (fi.lastModified()==group.readEntry("lastModified",QDateTime())) {
              name=group.readEntry("name");
              filetype=group.readEntry("filetypes");
              authors=group.readEntry("authors");
              license=group.readEntry("license");
              snippetlicense=group.readEntry("snippetlicense");
              if (!snippetlicense.isEmpty())
                configRead=true;
          }
        }
        if (!configRead) {
          SnippetCompletionModel::loadHeader(filename,&name,&filetype,&authors,&license,&snippetlicense);
          group.writeEntry("lastModified",fi.lastModified());
          group.writeEntry("name",name);
          group.writeEntry("filetypes",filetype);
          group.writeEntry("authors",authors);
          group.writeEntry("license",license);
          group.writeEntry("snippetlicense",snippetlicense);  
        }      
        name=i18nc("snippet name",name.toUtf8());
        if (update)
          updateEntry(name, filename, filetype, authors, license, snippetlicense, systemFile,ghnsFile);
        else
          addEntry(name, filename, filetype, authors, license, snippetlicense,systemFile,ghnsFile,/*enabled*/ false);
      }
    }
    
    
    
    QVariant SnippetRepositoryModel::data(const QModelIndex & index, int role) const {
      const SnippetRepositoryEntry& entry=m_entries[index.row()];
      switch(role) {
        case NameRole:
          return entry.name;
          break;
        case FilenameRole:
          return entry.filename;
          break;
        case FiletypeRole:
          return entry.fileType();
          break;
        case AuthorsRole:
          return entry.authors;
          break;
        case SnippetLicenseRole:
          return entry.snippetLicense;
          break;
        case LicenseRole:
          return entry.license;
          break;        
        case SystemFileRole:
          return entry.systemFile;
          break;
        case GhnsFileRole:
          return entry.ghnsFile;
          break;
        case EnabledRole:
          return entry.enabled;
          break;
        default:
          break;
      }
      return QVariant();
    }
    
    
    bool SnippetRepositoryModel::setData ( const QModelIndex & index, const QVariant & value, int role) {
        SnippetRepositoryEntry& entry=m_entries[index.row()];
        if (!index.isValid()) return false;
        switch (role) {
          case EnabledRole:
              kDebug()<<"setting enabled state for:"<<entry.filename;
              entry.enabled=value.toBool();
              emit dataChanged(index,index);
              emit typeChanged(entry.fileType());
              return true;
              break;
          case DeleteNowRole:
            if (KMessageBox::warningYesNo(value.value<QWidget*>(),
                            i18n("Do you really want to delete the file '%1' from the repository? This action is irreversible.",entry.name),
                            i18n("Deleting snippet file"))==KMessageBox::Yes) {
              KIO::NetAccess::del(KUrl::fromPath(entry.filename),value.value<QWidget*>());
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

    void SnippetRepositoryModel::newEntry(QWidget *dialogParent,const QString& type,bool add_after_creation) {
      QString tracking_params;
      if (add_after_creation) {
        QString token=QUuid::createUuid().toString();
        tracking_params=QString("?token=")+QUrl::toPercentEncoding(token)+
        "&service="+QUrl::toPercentEncoding(m_dbusServiceName)+
        "&object="+QUrl::toPercentEncoding(m_dbusObjectPath);
        m_newTokens.append(token);
      }
#ifdef Q_WS_X11
      if (tracking_params.isEmpty())
        tracking_params+="?";
      else
        tracking_params+="&";
      tracking_params+=QString("window=%1").arg(dialogParent->effectiveWinId());
#endif
      if (!KRun::runUrl(KUrl("new-file:///"+QUrl::toPercentEncoding(type)+tracking_params),"application/x-katesnippets_tng",dialogParent)) {
        KMessageBox::error(dialogParent,i18n("Editor application for new file with mimetype 'application/x-katesnippets_tng' could not be started"));
      }
    }

    void SnippetRepositoryModel::newEntry() {
      QWidget *widget=qobject_cast<QWidget*>(sender());
      if (!KRun::runUrl(KUrl("new-file:///"),"application/x-katesnippets_tng",widget)) {
        KMessageBox::error(widget,i18n("Editor application for new file with mimetype 'application/x-katesnippets_tng' could not be started"));
      }
    }

    void SnippetRepositoryModel::copyToRepository(const KUrl& src) {
      if (!src.isValid()) return;
      QString filename=src.fileName();
      if (filename.isEmpty()) {
        KMessageBox::error((QWidget*)0,i18n("No file specified"));
        return;
      }
      QString fileName=QUrl::toPercentEncoding(filename);
      QString outname=KGlobal::dirs()->locateLocal( "data", "ktexteditor_snippets/data/"+fileName);
      QFileInfo fiout(outname);
      if (fiout.exists()) {
        bool ok=false;
        for (int i=0;i<1000;i++) {
          outname=KGlobal::dirs()->locateLocal( "data", "ktexteditor_snippets/data/"+QString("%1_").arg(i)+fileName);
          QFileInfo fiout1(outname);
          if (!fiout1.exists()) {ok=true;break;}
        }
        if (!ok) {
          KMessageBox::error(0,i18n("It was not possible to create a unique file name for the imported file"));
          return;
        } else {
          KMessageBox::information(0,i18n("Imported file has been renamed because of a name conflict"));
        }
      }
      KUrl target;
      target.setPath(outname);
      
      if (!KIO::NetAccess::file_copy( src, target, (QWidget*)0)) {
        KMessageBox::error((QWidget*)0,i18n("File could not be copied to repository"));
      } else {
        // notify everybody
        notifyRepos();
      }
    }

    void SnippetRepositoryModel::updateEntry(const QString& name, const QString& filename, const QString& filetype, const QString& authors, const QString& license, const QString& snippetLicense, bool systemFile, bool ghnsFile)
    {
        for (int i=0;i<m_entries.count();i++) {
            SnippetRepositoryEntry& entry=m_entries[i];
            if (entry.filename==filename)
            {
              entry.name=name;
              entry.setFileType(filetype.split(";"));
              entry.authors=authors;
              entry.license=license;
              entry.systemFile=systemFile;
              entry.snippetLicense=snippetLicense;
              return;
            }
        }
        addEntry(name, filename, filetype, authors, license, snippetLicense, systemFile, ghnsFile, /*enabled*/ false);
    }
    void SnippetRepositoryModel::addEntry(const QString& name, const QString& filename, const QString& filetype, const QString& authors, const QString& license, const QString& snippetLicense, bool systemFile, bool ghnsFile, bool enabled) {
      beginInsertRows(QModelIndex(), m_entries.count(), m_entries.count());
      m_entries.append(SnippetRepositoryEntry(name,filename,filetype,authors,license,snippetLicense,systemFile,ghnsFile,enabled));
      endInsertRows();
    }

    SnippetCompletionModel* SnippetRepositoryModel::completionModel(const QString &filetype) {
      kDebug(13040)<<"Creating a new completion model";
      kDebug(13040)<<"**************************************************************************************************************************"<<filetype;
      QStringList l;
      foreach(const SnippetRepositoryEntry& entry, m_entries) {
        if ((entry.enabled==true) && ( (entry.fileType().contains("*")) || (entry.fileType().contains(filetype)))) {
          l<<entry.filename;
        }
      }
      return new SnippetCompletionModel(filetype,l);

    }
  
    void SnippetRepositoryModel::readSessionConfig (KConfigBase* config, const QString& groupPrefix) {
      QSet<QString> enabledSet;
      KConfigGroup group(config,groupPrefix+"enabled-snippets");
      int enabledCount=group.readEntry("count",0);
      for (int i=0;i<enabledCount;i++)
        enabledSet.insert(group.readPathEntry(QString("enabled_%1").arg(i),""));
      for (int i=0;i<m_entries.count();i++) {
        SnippetRepositoryEntry& entry=m_entries[i];
        entry.enabled=enabledSet.contains(entry.filename);
      }
    }
    
    void SnippetRepositoryModel::writeSessionConfig (KConfigBase* config, const QString& groupPrefix) {
      KConfigGroup group(config,groupPrefix+"enabled-snippets");
      group.deleteGroup();
      int enabledCount=0;
      for (int i=0;i<m_entries.count();i++) {
        const SnippetRepositoryEntry& entry=m_entries[i];
        if (entry.enabled) {
          group.writePathEntry(QString("enabled_%1").arg(enabledCount),entry.filename);
          enabledCount++;
        }
      }
      group.writeEntry("count",enabledCount);
      group.sync();
    }
        
  QModelIndex SnippetRepositoryModel::indexForFile(const QString& filename)
  {
    for (int i=0;i<m_entries.count();i++)
    {
      const SnippetRepositoryEntry& entry=m_entries[i];
      //kdDebug()<<"comparing entry.filename with filename:"<< entry.filename<<" -- "<<filename;
      if (entry.filename==filename) {         
        return index(i,0);
      }
    }
    return QModelIndex();
  }

  QModelIndex SnippetRepositoryModel::findFirstByName(const QString& name)
  {
    for (int i=0;i<m_entries.count();i++)
    {
      const SnippetRepositoryEntry& entry=m_entries[i];
      //kdDebug()<<"comparing entry.filename with filename:"<< entry.filename<<" -- "<<filename;
      if (entry.name==name) {         
        return index(i,0);
      }
    }
    return QModelIndex();
  }
  
  void SnippetRepositoryModel::tokenNewHandled(const QString& token, const QString& filepath)
  {
      if (!m_newTokens.contains(token)) return;
      m_newTokens.removeAll(token);
      if (!filepath.isEmpty()) {
        QModelIndex idx=indexForFile(filepath);
        if (idx.isValid())
          setData(idx,QVariant(true),EnabledRole);
      }
  }
  
  void SnippetRepositoryModel::addSnippetToFile(QWidget *dialogParent,const QString& snippet, const QString& filename)
  {
      KTemporaryFile tf;
      tf.setAutoRemove(false);
      tf.open();
      QString destFileName=tf.fileName();
      tf.write(snippet.toUtf8());
      tf.close();
            
      KUrl url=KUrl::fromPath(filename);
      url.addQueryItem("addthis",destFileName);
#ifdef Q_WS_X11      
      url.addQueryItem("window",QString("%1").arg(dialogParent->effectiveWinId()));
#endif
      kDebug()<<destFileName<<" --> "<<url.prettyUrl();      
      if (!KRun::runUrl(url,"application/x-katesnippets_tng",dialogParent)) {
        tf.remove();
        KMessageBox::error(dialogParent,i18n("Editor application for new file with mimetype 'application/x-katesnippets_tng' could not be started"));
      }                  
  }

void SnippetRepositoryModel::addSnippetToNewEntry(QWidget * dialogParent, const QString& snippet,
  const QString &repoTitle, const QString& type, bool add_after_creation)   {
      KTemporaryFile tf;
      tf.setAutoRemove(false);
      tf.open();
      QString destFileName=tf.fileName();
      tf.write(snippet.toUtf8());
      tf.close();
            
      KUrl url=KUrl(QString("new-file:///%1").arg(type));
      url.addQueryItem("addthis",destFileName);
      url.addQueryItem("repotitle",repoTitle);

      if (add_after_creation) {
        QString token=QUuid::createUuid().toString();
        url.addQueryItem("token",token);
        url.addQueryItem("service",m_dbusServiceName);
        url.addQueryItem("object",m_dbusObjectPath);
        m_newTokens.append(token);
      }           
#ifdef Q_WS_X11
      url.addQueryItem("window",QString("%1").arg(dialogParent->effectiveWinId()));
#endif
      if (!KRun::runUrl(url,"application/x-katesnippets_tng",dialogParent)) {
        KMessageBox::error(dialogParent,i18n("Editor application for new file with mimetype 'application/x-katesnippets_tng' could not be started"));
      }
  }


//END: Model

//BEGIN: Config Widget
  SnippetRepositoryConfigWidget::SnippetRepositoryConfigWidget( QWidget* parent, KTextEditor::CodesnippetsCore::SnippetRepositoryModel *repository )
    : QWidget( parent )
    , m_repository( repository )
  {
    m_ui=new Ui::KateSnippetRepository();
    m_ui->setupUi(this);
    m_ui->btnGHNS->setIcon(KIcon("get-hot-new-stuff"));    
    KTextEditor::CodesnippetsCore::SnippetRepositoryItemDelegate *delegate=new KTextEditor::CodesnippetsCore::SnippetRepositoryItemDelegate(m_ui->lstSnippetFiles,this);
    m_ui->lstSnippetFiles->setItemDelegate(delegate);
    
    m_ui->lstSnippetFiles->setModel(m_repository);
    connect(m_ui->btnNew,SIGNAL(clicked()),m_repository,SLOT(newEntry()));
    connect(m_ui->btnCopy,SIGNAL(clicked()),this,SLOT(slotCopy()));
    connect(m_ui->btnGHNS,SIGNAL(clicked()),this,SLOT(slotGHNS()));
  }

  void SnippetRepositoryConfigWidget::slotCopy()
  {
    KUrl url(m_ui->urlSource->url());
    if (!url.isValid()) return;
    m_repository->copyToRepository(url);
  }
  
  void SnippetRepositoryConfigWidget::slotGHNS()
  {
    KNS3::DownloadDialog dialog("ktexteditor_codesnippets_core.knsrc", this);
    dialog.exec();
    if (!dialog.changedEntries().isEmpty()) {
        notifyRepos();
    }
  }

  SnippetRepositoryConfigWidget::~SnippetRepositoryConfigWidget()
  {
    delete m_ui;
  }

//END: Config Widget


//BEGIN: DBus Adaptor

    SnippetRepositoryModelAdaptor::SnippetRepositoryModelAdaptor(SnippetRepositoryModel *repository):
      QDBusAbstractAdaptor(repository),m_repository(repository) {}
    
    SnippetRepositoryModelAdaptor::~SnippetRepositoryModelAdaptor() {}
    
    void SnippetRepositoryModelAdaptor::updateSnippetRepository()
    {
      m_repository->createOrUpdateList(true);
    }
 
    void SnippetRepositoryModelAdaptor::tokenNewHandled(const QString& token, const QString& filepath)
    {
      m_repository->tokenNewHandled(token,filepath);
    }
 
//END: DBus Adaptor

  }
}
// kate: space-indent on; indent-width 2; replace-tabs on;
