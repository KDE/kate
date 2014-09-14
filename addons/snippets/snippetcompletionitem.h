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

#ifndef SNIPPETCOMPLETIONITEM_H
#define SNIPPETCOMPLETIONITEM_H

/// TODO: push this into kdevplatform/language/codecompletion so language plugins can reuse it's functionality

#include <QString>
#include <QVariant>

class Snippet;
class SnippetRepository;
class QModelIndex;

namespace KTextEditor {
  class View;
  class Range;
  class CodeCompletionModel;
}

class SnippetCompletionItem
{
public:
    SnippetCompletionItem(Snippet* snippet, SnippetRepository* repo);
    ~SnippetCompletionItem();

    void execute(KTextEditor::View* view, const KTextEditor::Range& word);
    QVariant data( const QModelIndex& index, int role, const KTextEditor::CodeCompletionModel* model ) const;

private:
    // we copy since the snippet itself can be deleted at any time
    QString m_name;
    QString m_snippet;
    SnippetRepository* m_repo;
};

#endif // SNIPPETCOMPLETIONITEM_H
