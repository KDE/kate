/*
    SPDX-FileCopyrightText: 2021 Ilia Kats <ilia-kats@gmx.net>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "completionmodel.h"
#include "completiontable.h"

#include <algorithm>
#include <cstring>

#include <QIcon>
#include <QRegularExpression>

#include <KTextEditor/Document>
#include <KTextEditor/View>

bool startsWith(const Completion &comp, const std::string &prefix)
{
    if (prefix.size() <= comp.completion_strlen)
        return std::strncmp(prefix.data(), comp.completion, prefix.size()) == 0;
    return false;
}

LatexCompletionModel::LatexCompletionModel(QObject *parent)
    : KTextEditor::CodeCompletionModel(parent)
{
}

void LatexCompletionModel::completionInvoked(KTextEditor::View *view,
                                             const KTextEditor::Range &range,
                                             KTextEditor::CodeCompletionModel::InvocationType invocationType)
{
    Q_UNUSED(invocationType);
    beginResetModel();
    m_matches.first = m_matches.second = -1;
    auto word = view->document()->text(range).toStdString();
    const Completion *beginit = (Completion *)&completiontable;
    const Completion *endit = beginit + n_completions;
    if (!word.empty() && word[0] == QLatin1Char('\\')) {
        auto prefixrangestart = std::lower_bound(beginit, endit, word, [](const Completion &a, const std::string &b) -> bool {
            return startsWith(a, b) ? false : a.completion < b;
        });
        auto prefixrangeend = std::upper_bound(beginit, endit, word, [](const std::string &a, const Completion &b) -> bool {
            return startsWith(b, a) ? false : a < b.completion;
        });
        if (prefixrangestart != endit) {
            m_matches.first = prefixrangestart - beginit;
            m_matches.second = prefixrangeend - beginit;
        }
    }
    setRowCount(m_matches.second - m_matches.first);
    endResetModel();
}

bool LatexCompletionModel::shouldStartCompletion(KTextEditor::View *view, const QString &insertedText, bool userInsertion, const KTextEditor::Cursor &position)
{
    Q_UNUSED(view);
    Q_UNUSED(position);
    return userInsertion && latexexpr.match(insertedText).hasMatch();
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

QVariant LatexCompletionModel::data(const QModelIndex &index, int role) const
{
    if (role == UnimportantItemRole)
        return false;
    else if (role == InheritanceDepth)
        return 1;

    if (index.isValid() && index.row() < m_matches.second - m_matches.first) {
        const Completion &completion = completiontable[m_matches.first + index.row()];
        if (role == IsExpandable)
            return true; // if it's not expandable, the description will often be cut off
                         // because apprarently the ItemSelected role is not taken into account
                         // when determining the completion widget width. So expanding is
                         // the only way to make sure that the complete description is available.
        else if (role == ItemSelected || role == ExpandingWidget)
            return QStringLiteral("<table><tr><td>%1</td><td>%2</td></tr></table>")
                .arg(QString::fromUtf8(completion.codepoint), QString::fromUtf8(completion.name));
        else if (role == Qt::DisplayRole) {
            if (index.column() == Name)
                return QString::fromUtf8(completion.completion);
            else if (index.column() == Postfix)
                return QString::fromUtf8(completion.chars);
        } else if (index.column() == Icon && role == Qt::DecorationRole) {
            static const QIcon icon(QIcon::fromTheme(QStringLiteral("texcompiler")));
            return icon;
        }
    }
    return QVariant();
}
