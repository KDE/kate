/*
    SPDX-FileCopyrightText: 2021 Ilia Kats <ilia-kats@gmx.net>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "completionmodel.h"
#include "completiontrie.h"
#include "logging.h"

#include <QIcon>
#include <QRegularExpression>

#include <KLocalizedString>
#include <KTextEditor/Document>
#include <KTextEditor/View>

static const QRegularExpression latexexpr(QStringLiteral("\\\\:?[\\w]+:?$"),
                                          QRegularExpression::DontCaptureOption); // no unicode here, LaTeX expressions are ASCII only

LatexCompletionModel::LatexCompletionModel(QObject *parent)
    : KTextEditor::CodeCompletionModel(parent)
{
    setHasGroups(false);
}
void LatexCompletionModel::completionInvoked(KTextEditor::View *view,
                                             const KTextEditor::Range &range,
                                             KTextEditor::CodeCompletionModel::InvocationType invocationType)
{
    Q_UNUSED(invocationType);
    beginResetModel();
    m_matches.clear();
    auto word = view->document()->text(range);
    if (!word.isEmpty() && word[0] == QLatin1Char('\\')) {
        try {
            auto prefixrange = completiontrie.equal_prefix_range(word.toStdString());
            for (auto it = prefixrange.first; it != prefixrange.second; ++it) {
                m_matches.push_back(QPair(QString::fromStdString(it.key()), &(*it)));
            }
        } catch (const std::exception &e) {
            qCCritical(LATEXCOMPLETION) << "caught exception while generating completions for " << word;
            qCCritical(LATEXCOMPLETION) << e.what();
        } catch (...) {
            qCCritical(LATEXCOMPLETION) << "caught exception while generating completions for " << word;
        }
    }
    endResetModel();
}

bool LatexCompletionModel::shouldAbortCompletion(KTextEditor::View *view, const KTextEditor::Range &range, const QString &currentCompletion)
{
    if (view->cursorPosition() < range.start() || view->cursorPosition() > range.end())
        return true;
    return !latexexpr.match(currentCompletion).hasMatch();
}

KTextEditor::Range LatexCompletionModel::completionRange(KTextEditor::View *view, const KTextEditor::Cursor &position)
{
    auto text = view->document()->line(position.line());
    KTextEditor::Cursor start = position;
    int pos = text.left(position.column()).lastIndexOf(latexexpr);
    if (pos >= 0)
        start.setColumn(pos);
    return KTextEditor::Range(start, position);
}

void LatexCompletionModel::executeCompletionItem(KTextEditor::View *view, const KTextEditor::Range &word, const QModelIndex &index) const
{
    view->document()->replaceText(word, data(index.sibling(index.row(), Postfix), Qt::DisplayRole).toString());
}

int LatexCompletionModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return 1; // One root node to define the custom group
    } else if (parent.parent().isValid()) {
        return 0; // Completion-items have no children
    } else {
        return m_matches.size();
    }
}

QModelIndex LatexCompletionModel::index(int row, int column, const QModelIndex &parent) const
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

    if (row < 0 || row >= m_matches.size() || column < 0 || column >= ColumnCount) {
        return QModelIndex();
    }

    return createIndex(row, column, 1);
}

QModelIndex LatexCompletionModel::parent(const QModelIndex &index) const
{
    if (index.internalId()) {
        return createIndex(0, 0, quintptr(0));
    } else {
        return QModelIndex();
    }
}
#include <iostream>
QVariant LatexCompletionModel::data(const QModelIndex &index, int role) const
{
    if (role == UnimportantItemRole)
        return false;
    else if (role == InheritanceDepth)
        return 1;

    if (!index.parent().isValid()) { // header
        switch (role) {
        case Qt::DisplayRole:
            return i18n("LaTeX completion");
        case GroupRole:
            return Qt::DisplayRole;
        }
    }

    if (index.isValid() && m_matches.size()) {
        auto symbol = m_matches[index.row()];
        if (role == IsExpandable)
            return true; // if it's not expandable, the description will often be cut off
                         // because apprarently the ItemSelected role is not taken into account
                         // when determining the completion widget width. So expanding is
                         // the only way to make sure that the complete description is available.
        else if (role == ItemSelected || role == ExpandingWidget)
            return QStringLiteral("<table><tr><td>%1</td><td>%2</td></tr></table>").arg(symbol.second->codepoint, symbol.second->name);
        else if (role == Qt::DisplayRole) {
            if (index.column() == Name)
                return symbol.first;
            else if (index.column() == Postfix)
                return symbol.second->chars;
        } else if (index.column() == Icon && role == Qt::DecorationRole) {
            static const QIcon icon(QIcon::fromTheme(QStringLiteral("texcompiler")).pixmap(QSize(16, 16)));
            return icon;
        }
    }
    return QVariant();
}
