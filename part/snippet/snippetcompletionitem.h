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

#ifndef SNIPPETCOMPLETIONITEM_H
#define SNIPPETCOMPLETIONITEM_H

/// TODO: push this into kdevplatform/language/codecompletion so language plugins can reuse it's functionality

#include <language/codecompletion/codecompletionitem.h>

class Snippet;
class SnippetRepository;

class SnippetCompletionItem : public KDevelop::CompletionTreeItem
{
public:
    SnippetCompletionItem(Snippet* snippet, SnippetRepository* repo);
    ~SnippetCompletionItem();

    virtual void execute( KTextEditor::Document* document, const KTextEditor::Range& word );
    virtual QVariant data( const QModelIndex& index, int role, const KDevelop::CodeCompletionModel* model ) const;

private:
    // we copy since the snippet itself can be deleted at any time
    QString m_name;
    QString m_snippet;
    QString m_prefix;
    QString m_arguments;
    QString m_postfix;
    SnippetRepository* m_repo;
};

#endif // SNIPPETCOMPLETIONITEM_H
