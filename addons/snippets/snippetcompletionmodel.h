/*  This file is part of the Kate project.
 *  Based on the snippet plugin from KDevelop 4.
 *
 *  Copyright (C) 2008 Andreas Pakulat <apaku@gmx.de>
 *  Copyright (C) 2012 Christoph Cullmann <cullmann@kde.org>
 *
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
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#ifndef SNIPPETCOMPLETIONMODEL_H
#define SNIPPETCOMPLETIONMODEL_H

#include <ktexteditor/codecompletionmodel.h>
#include <ktexteditor/codecompletionmodelcontrollerinterface.h>

#include <QPointer>

namespace KTextEditor
{
class View;
}

class SnippetCompletionItem;

class SnippetCompletionModel : public KTextEditor::CodeCompletionModel,
                               public KTextEditor::CodeCompletionModelControllerInterface
{
    Q_OBJECT
    Q_INTERFACES(KTextEditor::CodeCompletionModelControllerInterface)

public:
    SnippetCompletionModel();
    ~SnippetCompletionModel() override;

    QVariant data( const QModelIndex& idx, int role = Qt::DisplayRole ) const override;
    void completionInvoked(KTextEditor::View *view, const KTextEditor::Range &range,
                           KTextEditor::CodeCompletionModel::InvocationType invocationType) override;
    void executeCompletionItem (KTextEditor::View *view, const KTextEditor::Range &word, const QModelIndex &index) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& index) const override;

    KTextEditor::Range completionRange(KTextEditor::View* view, const KTextEditor::Cursor& position) override;
    bool shouldAbortCompletion(KTextEditor::View* view, const KTextEditor::Range& range, const QString& currentCompletion) override;
private:
    void initData(KTextEditor::View* view);
    QList<SnippetCompletionItem*> m_snippets;
};

#endif
