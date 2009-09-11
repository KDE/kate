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

#ifndef __JOWENN_SNIPPETS_COMPLETIONMODEL_H__
#define __JOWENN_SNIPPETS_COMPLETIONMODEL_H__


#include <QAbstractListModel>
#include <ktexteditor/codecompletionmodel.h>
#include <ktexteditor/document.h>

namespace JoWenn {
  class KateSnippetCompletionEntry;
   
  class KateSnippetSelectorModel;
  
  class KateSnippetCompletionModel: public KTextEditor::CodeCompletionModel2 {
      Q_OBJECT
    public:
      friend class KateSnippetSelectorModel;
      KateSnippetCompletionModel(QStringList &snippetFiles);
      virtual ~KateSnippetCompletionModel();
      virtual void completionInvoked(KTextEditor::View* view, const KTextEditor::Range& range, InvocationType invocationType);
      virtual QVariant data (const QModelIndex & index, int role = Qt::DisplayRole) const;
      
      virtual QModelIndex index(int row, int column, const QModelIndex& parent) const;
      virtual QModelIndex parent(const QModelIndex& index) const;
      virtual int rowCount ( const QModelIndex & parent ) const;
      
      virtual void executeCompletionItem2(KTextEditor::Document* document,
        const KTextEditor::Range& word, const QModelIndex& index) const;

      static void loadHeader(const QString& filename, QString* name, QString* filetype, QString* authors, QString* license);

      KateSnippetSelectorModel *selectorModel();
      
    private:
      QList<KateSnippetCompletionEntry> m_entries;
      QList<const KateSnippetCompletionEntry*> m_matches;
      void loadEntries(const QString& filename);
  };
  
  class KateSnippetSelectorModel: public QAbstractItemModel {
      Q_OBJECT
    public:
      KateSnippetSelectorModel(KateSnippetCompletionModel* cmodel);
      virtual ~KateSnippetSelectorModel();
      virtual int columnCount(const QModelIndex& parent) const {return parent.isValid()?0:1;}
      virtual int rowCount(const QModelIndex& parent) const;
      virtual QModelIndex index ( int row, int column, const QModelIndex & parent = QModelIndex() ) const;
      virtual QVariant data(const QModelIndex &index, int role) const;
      virtual QModelIndex parent ( const QModelIndex & index) const {return QModelIndex();}
      QVariant headerData ( int section, Qt::Orientation orientation, int role) const;
      
      enum {FillInRole=Qt::UserRole+1};
    private:
        KateSnippetCompletionModel *m_cmodel;
  };
  
  
}
#endif
// kate: space-indent on; indent-width 2; replace-tabs on;