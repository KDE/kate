/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef KATE_CMAKE_COMPLETION_H
#define KATE_CMAKE_COMPLETION_H

#include <ktexteditor/codecompletionmodel.h>
#include <ktexteditor/codecompletionmodelcontrollerinterface.h>
#include <ktexteditor/view.h>

#include <QStandardItemModel>

/**
 * Project wide completion support.
 */
class CMakeCompletion : public KTextEditor::CodeCompletionModel, public KTextEditor::CodeCompletionModelControllerInterface
{
    Q_OBJECT

    Q_INTERFACES(KTextEditor::CodeCompletionModelControllerInterface)

    using Controller = KTextEditor::CodeCompletionModelControllerInterface;

public:
    /**
     * Construct project completion.
     * @param plugin our plugin
     */
    explicit CMakeCompletion(QObject *parent = nullptr);

    /**
     * This function is responsible to generating / updating the list of current
     * completions. The default implementation does nothing.
     *
     * When implementing this function, remember to call setRowCount() (or implement
     * rowCount()), and to generate the appropriate change notifications (for instance
     * by calling QAbstractItemModel::reset()).
     * @param view The view to generate completions for
     * @param range The range of text to generate completions for
     * */
    void completionInvoked(KTextEditor::View *view, const KTextEditor::Range &range, InvocationType invocationType) override;

    bool shouldStartCompletion(KTextEditor::View *view, const QString &insertedText, bool userInsertion, const KTextEditor::Cursor &position) override;

    int rowCount(const QModelIndex &parent) const override;

    QVariant data(const QModelIndex &index, int role) const override;

    static bool isCMakeFile(const QUrl &url);

    //     KTextEditor::Range completionRange(KTextEditor::View *view, const KTextEditor::Cursor &position) override;

    //     void allMatches(QStandardItemModel &model, KTextEditor::View *view, const KTextEditor::Range &range) const;

    struct Completion {
        enum Kind {
            Compl_PROPERTY,
            Compl_VARIABLE,
            Compl_COMMAND,
        } kind;
        QByteArray text;
    };

private:
    std::vector<Completion> m_matches;

    bool m_hasData = false;
};

#endif
