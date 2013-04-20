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

#ifndef __KTE_SNIPPETS_REPOSITORY_H__
#define __KTE_SNIPPETS_REPOSITORY_H__

#include "kwidgetitemdelegate.h"
#include <QAbstractListModel>
#include <ktexteditor/codecompletionmodel.h>
#include <ktexteditor/document.h>
#include <ktexteditor/templateinterface2.h>
#include <qdbusabstractadaptor.h>
#include <QDBusConnection>
#include <QSortFilterProxyModel>
#include "katedialogs.h"
#include "ui_snippet_repository.h"

class KConfigBase;
class KConfig;


class KatePartSnippetsConfigPage : public KateConfigPage {
    Q_OBJECT
  public:
    explicit KatePartSnippetsConfigPage( QWidget* parent = 0 );
    virtual ~KatePartSnippetsConfigPage()
    {}

    virtual void apply();
    virtual void reset();
    virtual void defaults(){}
  private:
};


namespace KTextEditor {
  namespace CodesnippetsCore {
    class SnippetRepositoryEntry;
    class SnippetCompletionEntry;
    class SnippetCompletionModel;
    
    class SnippetRepositoryItemDelegatePrivate;
    
    class SnippetRepositoryItemDelegate: public KWidgetItemDelegate
    {
        Q_OBJECT
      public:
        explicit SnippetRepositoryItemDelegate(QAbstractItemView *itemView, QObject * parent = 0);
        virtual ~SnippetRepositoryItemDelegate();

        virtual QList<QWidget*> createItemWidgets() const;

        virtual void updateItemWidgets(const QList<QWidget*> widgets,
          const QStyleOptionViewItem &option,
          const QPersistentModelIndex &index) const;
          
        virtual void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const;
        virtual QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const;
      public Q_SLOTS:
        void enabledChanged(int state);
        void editEntry();
        void deleteEntry();
        void uploadEntry();
      private:
        SnippetRepositoryItemDelegatePrivate* d;
    };
    

    class SnippetRepositoryModelPrivate;
    
    class SnippetRepositoryModel: public QAbstractListModel
    {
        Q_OBJECT
      public:
        explicit SnippetRepositoryModel(QObject *parent=0,KTextEditor::TemplateScriptRegistrar *scriptRegistrar=0);
        virtual ~SnippetRepositoryModel();
        enum Roles {
          NameRole = Qt::UserRole,
          FilenameRole,
          FiletypeRole,
          AuthorsRole,
          LicenseRole,
          SnippetLicenseRole,
          SystemFileRole,
          GhnsFileRole,
          EnabledRole,
          DeleteNowRole,
          EditNowRole,
          UploadNowRole,
          ForExtension=Qt::UserRole+100
        };
        
        //for the kdevelop based view
        enum ItemType {
          NullItem=0,
          RepositoryItem=1,
          SnippetItem=2
        };
      private:
        void createOrUpdateList(bool update);
        void tokenNewHandled(const QString& token, const QString& filepath);
        QString m_dbusServiceName;
        QString m_dbusObjectPath;
        friend class SnippetRepositoryModelAdaptor;
        static long s_id;
        QDBusConnection m_connection;
      public:
        void typeAndDisplay(const QModelIndex &index, enum ItemType* itemType,QVariant *display);
        
        virtual int rowCount(const QModelIndex & parent = QModelIndex()) const;
        virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
        virtual bool setData ( const QModelIndex & index, const QVariant & value, int role = Qt::EditRole );
        virtual QModelIndex parent ( const QModelIndex & index ) const;

        void updateEntry(const QString& name, const QString& filename, const QString& filetype, const QString& authors, const QString& license, const QString& snippetlicense, bool systemFile, bool ghnsFile);
        void addEntry(const QString& name, const QString& filename, const QString& filetype, const QString& authors, const QString& license, const QString& snippetlicense, bool systemFile, bool ghnsFile, bool enabled);
        SnippetCompletionModel* completionModel(const QString &filetype);
        void readSessionConfig (KConfigBase* config, const QString& groupPrefix);
        void writeSessionConfig (KConfigBase* config, const QString& groupPrefix);
        
        bool hasChildren ( const QModelIndex & parent = QModelIndex() ) const;
        QModelIndex index ( int row, int column = 0, const QModelIndex & parent = QModelIndex() ) const;

        int columnCount ( const QModelIndex & parent = QModelIndex() ) const {
          return 1;
          
        }
        
        QModelIndex indexForFile(const QString&);
        QModelIndex findFirstByName(const QString&);
        
        Qt::ItemFlags flags(const QModelIndex & index) const;
        
      Q_SIGNALS:
        void typeChanged(const QStringList& fileType);
      public:
        void newEntry(QWidget *dialogParent,const QString& type=QString(),bool add_after_creation=false);
        void addSnippetToFile(QWidget *dialogParent, const QString& snippet, const QString& filename);
        void addSnippetToNewEntry(QWidget * dialogParent, const QString& snippet, const QString &repoTitle, const QString& type, bool add_after_creation);
      public Q_SLOTS:
        void newEntry();
        void copyToRepository(const KUrl& src);
      private:
        QList<SnippetRepositoryEntry> m_entries;
        void createOrUpdateListSub(KConfig& config,QStringList list, bool update, bool ghnsFile);
        class SnippetRepositoryModelPrivate *d;
        QStringList m_newTokens;
        TemplateScriptRegistrar *m_scriptRegistrar;
    };
    
    
    class SnippetRepositoryConfigWidgetPrivate;
    class SnippetRepositoryConfigWidget : public QWidget {
      Q_OBJECT
    public:
      explicit SnippetRepositoryConfigWidget( QWidget* parent, SnippetRepositoryModel *repository );
      virtual ~SnippetRepositoryConfigWidget();

    public Q_SLOTS:
      void slotModeChanged();
      void slotCopy();
      void slotGHNS();
    private:
      SnippetRepositoryModel *m_repository;
      Ui::KTESnippetRepository m_ui;
      KTextEditor::CodesnippetsCore::SnippetRepositoryItemDelegate *m_delegate;
      SnippetRepositoryConfigWidgetPrivate *d;
  };

  
  
  // since the repository is now a tree to please the kdevelop like view, the config page now needs a filter
  class SnippetRepositoryViewFilterModel: public QSortFilterProxyModel {
    Q_OBJECT
  public:
    SnippetRepositoryViewFilterModel(QObject *parent);
    ~SnippetRepositoryViewFilterModel();
    void setSourceModel ( QAbstractItemModel * _sourceModel );

  private slots:
    void dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);

  private:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

  };
    

  }
}
#endif
// kate: space-indent on; indent-width 2; replace-tabs on;
