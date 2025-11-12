/*  This file is part of the Kate project.
 *  Based on the snippet plugin from KDevelop 4.
 *
 *  SPDX-FileCopyrightText: 2008 Andreas Pakulat <apaku@gmx.de>
 *  SPDX-FileCopyrightText: 2012 Christoph Cullmann <cullmann@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "snippetcompletionmodel.h"

#include <KTextEditor/Document>
#include <ktexteditor/view.h>

#include "snippet.h"
#include "snippetcompletionitem.h"
#include "snippetrepository.h"
#include "snippetstore.h"

#include <KLocalizedString>

SnippetCompletionModel::SnippetCompletionModel()
    : KTextEditor::CodeCompletionModel(nullptr)
{
    setHasGroups(false);
}

SnippetCompletionModel::~SnippetCompletionModel()
{
    qDeleteAll(m_snippets);
    m_snippets.clear();
}

QVariant SnippetCompletionModel::data(const QModelIndex &idx, int role) const
{
    if (role == KTextEditor::CodeCompletionModel::InheritanceDepth) {
        return 11000;
    }

    // grouping of snippets
    if (!idx.parent().isValid()) {
        if (role == Qt::DisplayRole) {
            return i18n("Snippets");
        }
        if (role == KTextEditor::CodeCompletionModel::GroupRole) {
            return Qt::DisplayRole;
        }
        return {};
    }
    // snippets
    if (!idx.isValid() || idx.row() < 0 || idx.row() >= m_snippets.count()) {
        return {};
    } else {
        return m_snippets.at(idx.row())->data(idx, role, nullptr);
    }
}

void SnippetCompletionModel::executeCompletionItem(KTextEditor::View *view, const KTextEditor::Range &word, const QModelIndex &index) const
{
    if (index.parent().isValid()) {
        m_snippets[index.row()]->execute(view, word);
    }
}

void SnippetCompletionModel::completionInvoked(KTextEditor::View *view, const KTextEditor::Range &range, InvocationType invocationType)
{
    Q_UNUSED(range);
    Q_UNUSED(invocationType);
    initData(view);
}

void SnippetCompletionModel::initData(KTextEditor::View *view)
{
    QString posMode = view->document()->highlightingModeAt(view->cursorPosition());
    QString docMode = view->document()->highlightingMode();
    if (docMode.isEmpty() && posMode.isEmpty()) {
        qWarning("SnippetCompletionModel::initData Unexpected empty modes");
        return;
    }

    beginResetModel();

    qDeleteAll(m_snippets);
    m_snippets.clear();
    SnippetStore *store = SnippetStore::self();
    for (int i = 0; i < store->rowCount(); i++) {
        if (store->item(i, 0)->checkState() != Qt::Checked) {
            continue;
        }
        SnippetRepository *repo = SnippetRepository::fromItem(store->item(i, 0));
        if (!repo) {
            continue;
        }
        const QStringList fileTypes = repo->fileTypes();
        if (fileTypes.isEmpty() || fileTypes.contains(docMode) || fileTypes.contains(posMode)) {
            for (int j = 0; j < repo->rowCount(); ++j) {
                if (Snippet *snippet = Snippet::fromItem(repo->child(j))) {
                    m_snippets << new SnippetCompletionItem(snippet, repo);
                }
            }
        }
    }

    endResetModel();
}

QModelIndex SnippetCompletionModel::parent(const QModelIndex &index) const
{
    if (index.internalId()) {
        return createIndex(0, 0, quintptr(0));
    } else {
        return {};
    }
}

QModelIndex SnippetCompletionModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        if (row == 0) {
            return createIndex(row, column, quintptr(0)); // header  index
        } else {
            return {};
        }
    } else if (parent.parent().isValid()) { // we only have header and children, no subheaders
        return {};
    }

    if (row < 0 || row >= m_snippets.count() || column < 0 || column >= ColumnCount) {
        return {};
    }

    return createIndex(row, column, 1); // normal item index
}

int SnippetCompletionModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid() && !m_snippets.isEmpty()) {
        return 1; // one toplevel node (group header)
    } else if (parent.parent().isValid()) {
        return 0; // we don't have sub children
    } else {
        return m_snippets.count(); // only the children
    }
}

static int minimalCompletionLength(KTextEditor::View *view)
{
    bool valueFound = false;
    const int length = view->configValue(QStringLiteral("word-completion-minimal-word-length")).toInt(&valueFound);

    // handle bogus values or old versions that don't export that setting
    return valueFound ? length : 3;
}

bool SnippetCompletionModel::shouldStartCompletion(KTextEditor::View *view, const QString &insertedText, bool userInsertion, const KTextEditor::Cursor &)
{
    if (userInsertion && QStringView(insertedText).trimmed().size() >= minimalCompletionLength(view)) {
        // last must be a letter to avoid some annoyance
        return insertedText.back().isLetter();
    }
    return false;
}

bool SnippetCompletionModel::shouldAbortCompletion(KTextEditor::View *view, const KTextEditor::Range &range, const QString &currentCompletion)
{
    if (view->cursorPosition() < range.start() || view->cursorPosition() > range.end()) {
        return true; // Always abort when the completion-range has been left
    }

    for (const auto token : currentCompletion) {
        if (token.isSpace()) {
            return true;
        }
    }
    // else it's valid
    return false;
}

#include "moc_snippetcompletionmodel.cpp"
