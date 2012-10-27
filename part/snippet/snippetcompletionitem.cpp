/**
 * This file is part of KDevelop
 *
 * Copyright 2009 Milian Wolff <mail@milianw.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "snippetcompletionitem.h"

#include <KTextEditor/Document>
#include <KLocalizedString>
#include <QtGui/QTextEdit>

#include <language/codecompletion/codecompletionmodel.h>

#include <ktexteditor/templateinterface.h>
#include <ktexteditor/view.h>

#include "snippet.h"
#include "snippetrepository.h"

#include <ktexteditor/templateinterface2.h>

SnippetCompletionItem::SnippetCompletionItem( Snippet* snippet, SnippetRepository* repo )
    : CompletionTreeItem(), m_name(snippet->text()), m_snippet(snippet->snippet()), m_prefix(snippet->prefix()),
      m_arguments(snippet->arguments()), m_postfix(snippet->postfix()), m_repo(repo)
{
    Q_ASSERT(m_repo);
    m_name.prepend( repo->completionNamespace() );
}

SnippetCompletionItem::~SnippetCompletionItem()
{
}

QVariant SnippetCompletionItem::data( const QModelIndex& index, int role, const KDevelop::CodeCompletionModel* model ) const
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
                return m_prefix;
            case KTextEditor::CodeCompletionModel::Postfix:
                return m_postfix;
            case KTextEditor::CodeCompletionModel::Arguments:
                return m_arguments;
        }
        break;
    case KDevelop::CodeCompletionModel::IsExpandable:
        return QVariant(true);
    case KDevelop::CodeCompletionModel::ExpandingWidget:
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

void SnippetCompletionItem::execute( KTextEditor::Document* document, const KTextEditor::Range& word )
{
    if ( document->activeView() ) {
        QMap< QString, QString > values = QMap<QString, QString>();
        if ( document->activeView()->selection() ) {
            values["selection"] = document->text(document->activeView()->selectionRange());
        }
        document->removeText(word);
        KTextEditor::TemplateInterface2* templateIface2
            = qobject_cast<KTextEditor::TemplateInterface2*>(document->activeView());
        if ( templateIface2 )
        {
            if ( word != document->activeView()->selectionRange() ) {
                document->removeText(word);
            }
            templateIface2->insertTemplateText(word.start(), m_snippet, values, m_repo->registeredScript());
            return;
        }
        KTextEditor::TemplateInterface* templateIface
            = qobject_cast<KTextEditor::TemplateInterface*>(document->activeView());
        if ( templateIface ) {
            templateIface->insertTemplateText(word.start(), m_snippet, values);
            return;
        }
    }

    document->replaceText(word, m_snippet);
}
