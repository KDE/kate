/***************************************************************************
 *   This file is part of KDevelop                                         *
 *   Copyright 2008 Andreas Pakulat <apaku@gmx.de>                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef SNIPPETCOMPLETIONMODEL_H
#define SNIPPETCOMPLETIONMODEL_H

#include <ktexteditor/codecompletionmodel.h>
#include <ktexteditor/codecompletionmodelcontrollerinterface.h>

namespace KTextEditor
{
class Document;
class View;
}

class SnippetCompletionItem;

class SnippetCompletionModel : public KTextEditor::CodeCompletionModel2,
                               public KTextEditor::CodeCompletionModelControllerInterface3
{
    Q_OBJECT
    Q_INTERFACES(KTextEditor::CodeCompletionModelControllerInterface3)

public:
    SnippetCompletionModel();
    ~SnippetCompletionModel();

    QVariant data( const QModelIndex& idx, int role = Qt::DisplayRole ) const;
    void completionInvoked(KTextEditor::View *view, const KTextEditor::Range &range,
                           KTextEditor::CodeCompletionModel::InvocationType invocationType);
    virtual void executeCompletionItem2(KTextEditor::Document* document, const KTextEditor::Range& word,
                                        const QModelIndex& index) const;
    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
    virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex& index) const;

    virtual KTextEditor::Range completionRange(KTextEditor::View* view, const KTextEditor::Cursor& position);
    virtual bool shouldAbortCompletion(KTextEditor::View* view, const KTextEditor::Range& range, const QString& currentCompletion);
private:
    void initData(KTextEditor::View* view);
    QList<SnippetCompletionItem*> m_snippets;
};

#endif
