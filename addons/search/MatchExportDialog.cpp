/*
    SPDX-FileCopyrightText: 2021 Markus Ebner <info@ebner-markus.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "MatchExportDialog.h"
#include "SearchPlugin.h"

#include <QAction>
#include <QMenu>
#include <QRegularExpression>

MatchExportDialog::MatchExportDialog(QWidget *parent, QAbstractItemModel *matchModel, QRegularExpression *regExp)
    : QDialog(parent)
    , m_matchModel(matchModel)
    , m_regExp(regExp)
{
    setupUi(this);
    setWindowTitle(i18n("Export Search Result Matches"));

    QAction *exportPatternTextActionForInsertRegexButton =
        exportPatternText->addAction(QIcon::fromTheme(QStringLiteral("code-context")), QLineEdit::TrailingPosition);

    connect(exportPatternTextActionForInsertRegexButton, &QAction::triggered, this, [this]() {
        QPoint pos = exportPatternText->pos();
        pos.rx() += exportPatternText->width() - ((16 + 4) * devicePixelRatioF());
        pos.ry() += exportPatternText->height();
        QMenu menu(this);
        QSet<QAction *> actionList;
        KatePluginSearchView::addRegexHelperActionsForReplace(&actionList, &menu);
        auto action = menu.exec(mapToGlobal(pos));
        KatePluginSearchView::regexHelperActOnAction(action, actionList, exportPatternText);
    });

    connect(pushButton, &QPushButton::clicked, this, &MatchExportDialog::generateMatchExport);
}

MatchExportDialog::~MatchExportDialog()
{
}

void MatchExportDialog::generateMatchExport()
{
    QString exportPattern = this->exportPatternText->text();
    QString exportResult;
    QModelIndex rootIndex = m_matchModel->index(0, 0);

    int fileCount = m_matchModel->rowCount(rootIndex);
    for (int i = 0; i < fileCount; ++i) {
        QModelIndex fileIndex = m_matchModel->index(i, 0, rootIndex);
        int matchCount = m_matchModel->rowCount(fileIndex);
        for (int j = 0; j < matchCount; ++j) {
            QModelIndex matchIndex = m_matchModel->index(j, 0, fileIndex);
            const KTextEditor::Document* doc = matchIndex.data(MatchModel::DocumentRole).value<KTextEditor::Document*>();
            if (doc == nullptr) { continue; }
            const KateSearchMatch& matchItem = matchIndex.data(MatchModel::MatchItemRole).value<KateSearchMatch>();
            QString matchLines = doc->text(matchItem.range);
            QRegularExpressionMatch match = MatchModel::rangeTextMatches(matchLines, *m_regExp);
        }
    }
    this->exportResultText->setPlainText(exportResult);
}
