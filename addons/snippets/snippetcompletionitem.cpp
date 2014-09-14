/*  This file is part of the Kate project.
 *  Based on the snippet plugin from KDevelop 4.
 *
 *  Copyright (C) 2009 Milian Wolff <mail@milianw.de>
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

#include "snippetcompletionitem.h"

#include <ktexteditor/document.h>
#include <KLocalizedString>
#include <QTextEdit>
#include <QModelIndex>

#include <ktexteditor/view.h>
#include <ktexteditor/codecompletioninterface.h>
#include <ktexteditor/codecompletionmodel.h>

#include "snippet.h"
#include "snippetrepository.h"


SnippetCompletionItem::SnippetCompletionItem( Snippet* snippet, SnippetRepository* repo )
    : m_name(snippet->text()), m_snippet(snippet->snippet()), m_repo(repo)
{
    Q_ASSERT(m_repo);
    const auto& namespace_ = repo->completionNamespace();
    if ( !namespace_.isEmpty() ) {
        m_name.prepend( QLatin1String(":") );
        m_name.prepend( repo->completionNamespace() );
    }
}

SnippetCompletionItem::~SnippetCompletionItem()
{
}

QVariant SnippetCompletionItem::data( const QModelIndex& index, int role, const KTextEditor::CodeCompletionModel* model ) const
{
    // as long as the snippet completion model is not a kdevelop code completion model,
    // model will usually be 0. hence don't use it.
    Q_UNUSED(model);

    switch ( role ) {
    case Qt::DisplayRole:
        switch ( index.column() ) {
            case KTextEditor::CodeCompletionModel::Name:
                return m_name;
            case KTextEditor::CodeCompletionModel::Prefix:
                return QString();
            case KTextEditor::CodeCompletionModel::Postfix:
                return QString();
            case KTextEditor::CodeCompletionModel::Arguments:
                return QString();
        }
        break;
    case KTextEditor::CodeCompletionModel::IsExpandable:
        return QVariant(true);
    case KTextEditor::CodeCompletionModel::ExpandingWidget:
        {
        QTextEdit *textEdit = new QTextEdit();
        ///TODO: somehow make it possible to scroll like in other expanding widgets
        // don't make it too large, only show a few lines
        textEdit->resize(textEdit->width(), 100);
        textEdit->setPlainText(m_snippet);
        textEdit->setReadOnly(true);
        textEdit->setLineWrapMode(QTextEdit::NoWrap);

        QVariant v;
        v.setValue<QWidget*>(textEdit);
        return v;
        }
    }

    return QVariant();
}

void SnippetCompletionItem::execute(KTextEditor::View* view, const KTextEditor::Range& word)
{
    // insert snippet content
    view->insertTemplate(view->cursorPosition(), m_snippet, m_repo->script());
    view->document()->removeText(word);
}
