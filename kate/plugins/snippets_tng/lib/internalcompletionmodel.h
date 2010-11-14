/* This file is part of the KDE project
 * Copyright (C) 2010 Joseph Wenninger <jowenn@kde.org>
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

#ifndef __KTE_SNIPPETS_INTERNALCOMPLETIONMODEL_H__
#define __KTE_SNIPPETS_INTERNALCOMPLETIONMODEL_H__


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

class FilteredEntry;

//BEGIN: InternalCompletionModel
    class InternalCompletionModel: public KTextEditor::CodeCompletionModel2, public KTextEditor::CodeCompletionModelControllerInterface3 {
        Q_OBJECT
        Q_INTERFACES(KTextEditor::CodeCompletionModelControllerInterface3)
      public:
        InternalCompletionModel(QList<FilteredEntry> _entries);
        virtual ~InternalCompletionModel();
        virtual void completionInvoked(KTextEditor::View* view, const KTextEditor::Range& range, InvocationType invocationType);
        
        QVariant data( const QModelIndex & index, int role) const;
        QModelIndex parent(const QModelIndex& index) const;

        QModelIndex index(int row, int column, const QModelIndex& parent) const;
        int rowCount (const QModelIndex & parent) const;

        virtual void executeCompletionItem2(KTextEditor::Document* document,
          const KTextEditor::Range& word, const QModelIndex& index) const;



	  virtual void aborted(View* view);
	  
/*
         virtual bool shouldAbortCompletion(View* view, const Range &range, const QString &currentCompletion);
        virtual Range completionRange(View* view, const Cursor &position);
        virtual bool shouldStartCompletion(KTextEditor::View* view, const QString &insertedText, bool userInsertion, const KTextEditor::Cursor &position);
*/
        
      private:
        QList<FilteredEntry> entries;

    };

//END:  InternalCompletionModel

  }
#ifdef SNIPPET_EDITOR
 }
#endif
  
}

#endif