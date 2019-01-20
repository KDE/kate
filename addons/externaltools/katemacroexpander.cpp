/* This file is part of the KDE project
 *
 *  Copyright 2019 Dominik Haumann <dhaumann@kde.org>
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
#include "katemacroexpander.h"

#include <KTextEditor/Editor>
#include <KTextEditor/Application>
#include <KTextEditor/Document>
#include <KTextEditor/View>

MacroExpander::MacroExpander(KTextEditor::View * view)
    : KWordMacroExpander()
    , m_view(view)
{
}

bool MacroExpander::expandMacro(const QString& str, QStringList& ret)
{
    KTextEditor::View* view = m_view;
    if (!view)
        return false;

    KTextEditor::Document* doc = view->document();
    QUrl url = doc->url();

    if (str == QStringLiteral("URL"))
        ret += url.url();
    else if (str == QStringLiteral("directory")) // directory of current doc
        ret += url.toString(QUrl::RemoveScheme | QUrl::RemoveFilename);
    else if (str == QStringLiteral("filename"))
        ret += url.fileName();
    else if (str == QStringLiteral("line")) // cursor line of current doc
        ret += QString::number(view->cursorPosition().line());
    else if (str == QStringLiteral("col")) // cursor col of current doc
        ret += QString::number(view->cursorPosition().column());
    else if (str == QStringLiteral("selection")) // selection of current doc if any
        ret += view->selectionText();
    else if (str == QStringLiteral("text")) // text of current doc
        ret += doc->text();
    else if (str == QStringLiteral("URLs")) {
        foreach (KTextEditor::Document* it, KTextEditor::Editor::instance()->application()->documents())
            if (!it->url().isEmpty())
                ret += it->url().url();
    } else
        return false;

    return true;
}

// kate: space-indent on; indent-width 4; replace-tabs on;
