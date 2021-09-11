/*
    SPDX-FileCopyrightText: 2021 Ilia Kats <ilia-kats@gmx.net>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KATELATEXCOMPLETIONMODEL_H
#define KATELATEXCOMPLETIONMODEL_H

#include <QModelIndex>

#include <KTextEditor/CodeCompletionModel>
#include <KTextEditor/CodeCompletionModelControllerInterface>
#include <KTextEditor/Cursor>
#include <KTextEditor/Range>

namespace KTextEditor
{
class View;
}

struct Completion;
class LatexCompletionModel : public KTextEditor::CodeCompletionModel, public KTextEditor::CodeCompletionModelControllerInterface
{
    Q_OBJECT
    Q_INTERFACES(KTextEditor::CodeCompletionModelControllerInterface)
public:
    LatexCompletionModel(QObject *parent);
    KTextEditor::Range completionRange(KTextEditor::View *view, const KTextEditor::Cursor &position) override;
    bool shouldStartCompletion(KTextEditor::View *view, const QString &insertedText, bool userInsertion, const KTextEditor::Cursor &position) override;
    bool shouldAbortCompletion(KTextEditor::View *view, const KTextEditor::Range &range, const QString &currentCompletion) override;
    void completionInvoked(KTextEditor::View *view, const KTextEditor::Range &range, InvocationType invocationType) override;
    void executeCompletionItem(KTextEditor::View *view, const KTextEditor::Range &word, const QModelIndex &index) const override;
    inline KTextEditor::CodeCompletionModelControllerInterface::MatchReaction matchingItem(const QModelIndex &) override
    {
        return None;
    };
    QVariant data(const QModelIndex &index, int role) const override;

private:
    QPair<int, int> m_matches = {0, 0};
};
#endif
