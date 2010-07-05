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

#ifndef __KTE_SNIPPETS_COMPLETIONMODEL_H__
#define __KTE_SNIPPETS_COMPLETIONMODEL_H__


#include <QAbstractListModel>
#include <ktexteditor/codecompletionmodel.h>
#include <ktexteditor/templateinterface2.h>
#include <ktexteditor/document.h>
#include <ktexteditor_codesnippets_core_export.h>
#include <ktexteditor/smartrange.h>
#include <ktexteditor/codecompletionmodelcontrollerinterface.h>

namespace KTextEditor {
  namespace CodesnippetsCore {
#ifdef SNIPPET_EDITOR
  namespace Editor {
#endif
    class SnippetCompletionEntry;
    class FilteredEntry;
    
    class SnippetSelectorModel;

    class SnippetCompletionModelPrivate;

    class KTEXTEDITOR_CODESNIPPETS_CORE_EXPORT SnippetCompletionModel: public KTextEditor::CodeCompletionModel2, public KTextEditor::CodeCompletionModelControllerInterface3 {
        Q_OBJECT
        Q_INTERFACES(KTextEditor::CodeCompletionModelControllerInterface3)
      public:
        friend class SnippetSelectorModel;
        SnippetCompletionModel(const QString& fileType, QStringList &snippetFiles,KTextEditor::TemplateScriptRegistrar* scriptRegistrar);
        virtual ~SnippetCompletionModel();
        virtual void completionInvoked(KTextEditor::View* view, const KTextEditor::Range& range, InvocationType invocationType);
        virtual QVariant data (const QModelIndex & index, int role = Qt::DisplayRole) const;

        virtual QModelIndex index(int row, int column, const QModelIndex& parent) const;
        virtual QModelIndex parent(const QModelIndex& index) const;
        virtual int rowCount ( const QModelIndex & parent ) const;

        virtual void executeCompletionItem2(KTextEditor::Document* document,
          const KTextEditor::Range& word, const QModelIndex& index) const;

        static bool loadHeader(const QString& filename, QString* name, QString* filetype, QString* authors, QString* license, QString *snippetlicense,QString *nameSpace);

        virtual bool shouldAbortCompletion(View* view, const Range &range, const QString &currentCompletion);
        virtual Range completionRange(View* view, const Cursor &position);
        virtual bool shouldStartCompletion(KTextEditor::View* view, const QString &insertedText, bool userInsertion, const KTextEditor::Cursor &position);

        
        SnippetSelectorModel *selectorModel();
        QString fileType();
  #ifdef SNIPPET_EDITOR
        bool save(const QString& filename, const QString& name, const QString& license, const QString& filetype, const QString& authors, const QString& snippetlicense, const QString& nameSpace);
        static QString createNew(const QString& name, const QString& license,const QString& authors,const QString& filetypes);
  #endif

      private:
        SnippetCompletionModelPrivate  * const d;
        friend class CategorizedSnippetModel;
        friend class InternalCompletionModel;
        friend bool lessThanFilteredEntry(const FilteredEntry& first,const FilteredEntry& second);          

  #ifdef SNIPPET_EDITOR
      public:
          QString script();
          void setScript(const QString& script);
  #endif

    };

    
    class CategorizedSnippetModelPrivate;

    class KTEXTEDITOR_CODESNIPPETS_CORE_EXPORT CategorizedSnippetModel: public QAbstractItemModel {
      Q_OBJECT
      public:
        CategorizedSnippetModel(const QList<SnippetSelectorModel*>& models);
        virtual ~CategorizedSnippetModel();
        virtual int columnCount(const QModelIndex&) const {return 1;}
        virtual int rowCount(const QModelIndex& parent) const;
        virtual QModelIndex index ( int row, int column, const QModelIndex & parent = QModelIndex() ) const;
        virtual QVariant data(const QModelIndex &index, int role) const;
        virtual QModelIndex parent ( const QModelIndex & index) const;
        virtual QVariant headerData ( int section, Qt::Orientation orientation, int role) const;
                
        KActionCollection *actionCollection();
      public Q_SLOTS:
        void subDestroyed(QObject*);
        void actionTriggered();
      private:
        QList<SnippetSelectorModel*> m_models;
        KActionCollection *m_actionCollection;
        CategorizedSnippetModelPrivate *d;
      Q_SIGNALS:
        void needView(KTextEditor::View **);
    };

    class SnippetSelectorModelPrivate;

    class KTEXTEDITOR_CODESNIPPETS_CORE_EXPORT SnippetSelectorModel: public QAbstractItemModel {
        Q_OBJECT
      public:
        SnippetSelectorModel(SnippetCompletionModel* cmodel);
        virtual ~SnippetSelectorModel();
        virtual int columnCount(const QModelIndex& parent) const {return parent.isValid()?0:1;}
        virtual int rowCount(const QModelIndex& parent) const;
        virtual QModelIndex index ( int row, int column, const QModelIndex & parent = QModelIndex() ) const;
        virtual QVariant data(const QModelIndex &index, int role) const;
        virtual QModelIndex parent ( const QModelIndex & index) const {return QModelIndex();}
        virtual QVariant headerData ( int section, Qt::Orientation orientation, int role) const;

        QString fileType();

  #ifdef SNIPPET_EDITOR
        // for editor only
        virtual bool setData ( const QModelIndex & index, const QVariant & value, int role = Qt::EditRole );
        virtual bool removeRows ( int row, int count, const QModelIndex & parent = QModelIndex() );
        QModelIndex newItem();
        enum {FillInRole=Qt::UserRole+1,ScriptTokenRole,MergedFilesRole,PrefixRole,MatchRole,PostfixRole,ArgumentsRole,ShortcutRole,ForExtension=Qt::UserRole+100};
        //#warning SNIPPET_EDITOR IS SET
  #else
        enum {FillInRole=Qt::UserRole+1,ScriptTokenRole,MergedFilesRole,ForExtension=Qt::UserRole+100};
        //#warning SNIPPET_EDITOR IS NOT SET
  #endif
      private:
          SnippetCompletionModel *m_cmodel;
          SnippetSelectorModelPrivate *d;
          QList<QAction*> actions();
          void entriesForShortcut(const QString &shortcut, void* list);
          friend class CategorizedSnippetModel;    
    };

#ifdef SNIPPET_EDITOR
  }
#endif

  }
}
#endif
// kate: space-indent on; indent-width 2; replace-tabs on;
