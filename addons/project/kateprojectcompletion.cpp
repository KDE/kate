/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2012 Christoph Cullmann <cullmann@kde.org>
 *  Copyright (C) 2003 Anders Lund <anders.lund@lund.tdcadsl.dk>
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

#include "kateprojectcompletion.h"
#include "kateprojectplugin.h"

#include <klocalizedstring.h>

#include <QIcon>

KateProjectCompletion::KateProjectCompletion(KateProjectPlugin *plugin)
    : KTextEditor::CodeCompletionModel(nullptr)
    , m_plugin(plugin)
{
}

KateProjectCompletion::~KateProjectCompletion()
{
}

void KateProjectCompletion::saveMatches(KTextEditor::View *view, const KTextEditor::Range &range)
{
    m_matches.clear();
    allMatches(m_matches, view, range);
}

QVariant KateProjectCompletion::data(const QModelIndex &index, int role) const
{
    if (role == InheritanceDepth) {
        return 10010;    //Very high value, so the word-completion group and items are shown behind any other groups/items if there is multiple
    }

    if (!index.parent().isValid()) {
        //It is the group header
        switch (role) {
        case Qt::DisplayRole:
            return i18n("Project Completion");
        case GroupRole:
            return Qt::DisplayRole;
        }
    }

    if (index.column() == KTextEditor::CodeCompletionModel::Name && role == Qt::DisplayRole) {
        return m_matches.item(index.row())->data(Qt::DisplayRole);
    }

    if (index.column() == KTextEditor::CodeCompletionModel::Icon && role == Qt::DecorationRole) {
        static QIcon icon(QIcon::fromTheme(QStringLiteral("insert-text")).pixmap(QSize(16, 16)));
        return icon;
    }

    return QVariant();
}

QModelIndex KateProjectCompletion::parent(const QModelIndex &index) const
{
    if (index.internalId()) {
        return createIndex(0, 0, quintptr(0));
    } else {
        return QModelIndex();
    }
}

QModelIndex KateProjectCompletion::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        if (row == 0) {
            return createIndex(row, column, quintptr(0));
        } else {
            return QModelIndex();
        }

    } else if (parent.parent().isValid()) {
        return QModelIndex();
    }

    if (row < 0 || row >= m_matches.rowCount() || column < 0 || column >= ColumnCount) {
        return QModelIndex();
    }

    return createIndex(row, column, 1);
}

int KateProjectCompletion::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid() && !(m_matches.rowCount() == 0)) {
        return 1;    //One root node to define the custom group
    } else if (parent.parent().isValid()) {
        return 0;    //Completion-items have no children
    } else {
        return m_matches.rowCount();
    }
}

bool KateProjectCompletion::shouldStartCompletion(KTextEditor::View *view, const QString &insertedText, bool userInsertion, const KTextEditor::Cursor &position)
{
    if (!userInsertion) {
        return false;
    }
    if (insertedText.isEmpty()) {
        return false;
    }

    QString text = view->document()->line(position.line()).left(position.column());

    uint check = 3; //v->config()->wordCompletionMinimalWordLength();

    if (check <= 0) {
        return true;
    }
    int start = text.length();
    int end = text.length() - check;
    if (end < 0) {
        return false;
    }
    for (int i = start - 1; i >= end; i--) {
        QChar c = text.at(i);
        if (!(c.isLetter() || (c.isNumber()) || c == QLatin1Char('_'))) {
            return false;
        }
    }

    return true;
}

bool KateProjectCompletion::shouldAbortCompletion(KTextEditor::View *view, const KTextEditor::Range &range, const QString &currentCompletion)
{

    if (m_automatic) {
        if (currentCompletion.length() < 3 /*v->config()->wordCompletionMinimalWordLength()*/) {
            return true;
        }
    }

    return CodeCompletionModelControllerInterface::shouldAbortCompletion(view, range, currentCompletion);
}

void KateProjectCompletion::completionInvoked(KTextEditor::View *view, const KTextEditor::Range &range, InvocationType it)
{
    /**
     * auto invoke...
     */
    m_automatic = false;
    if (it == AutomaticInvocation) {
        m_automatic = true;

        if (range.columnWidth() >= 3 /*v->config()->wordCompletionMinimalWordLength()*/) {
            saveMatches(view, range);
        } else {
            m_matches.clear();
        }

        // done here...
        return;
    }

    // normal case ;)
    saveMatches(view, range);
}

// Scan throughout the entire document for possible completions,
// ignoring any dublets
void KateProjectCompletion::allMatches(QStandardItemModel &model, KTextEditor::View *view, const KTextEditor::Range &range) const
{
    /**
     * get project for this document, else fail
     */
    KateProject *project = m_plugin->projectForDocument(view->document());
    if (!project) {
        return;
    }

    /**
     * let project index fill the completion for this document
     */
    if (project->projectIndex()) {
        project->projectIndex()->findMatches(model, view->document()->text(range), KateProjectIndex::CompletionMatches);
    }
}

KTextEditor::CodeCompletionModelControllerInterface::MatchReaction KateProjectCompletion::matchingItem(const QModelIndex & /*matched*/)
{
    return HideListIfAutomaticInvocation;
}

// Return the range containing the word left of the cursor
KTextEditor::Range KateProjectCompletion::completionRange(KTextEditor::View *view, const KTextEditor::Cursor &position)
{
    int line = position.line();
    int col = position.column();

    KTextEditor::Document *doc = view->document();
    while (col > 0) {
        QChar c = (doc->characterAt(KTextEditor::Cursor(line, col - 1)));
        if (c.isLetterOrNumber() || c.isMark() || c == QLatin1Char('_')) {
            col--;
            continue;
        }

        break;
    }

    return KTextEditor::Range(KTextEditor::Cursor(line, col), position);
}

