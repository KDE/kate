/*
    SPDX-FileCopyrightText: 2021 Markus Ebner <info@ebner-markus.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "MatchExportDialog.h"
#include "plugin_search.h"

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
        QMenu menu;
        QSet<QAction *> actionList;
        KatePluginSearchView::addRegexHelperActionsForReplace(&actionList, &menu);
        auto &&action = menu.exec(QCursor::pos());
        KatePluginSearchView::regexHelperActOnAction(action, actionList, exportPatternText);
    });
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
            QRegularExpressionMatch match = m_regExp->match(matchIndex.data(MatchModel::MatchRole).toString());
            exportResult += MatchModel::generateReplaceString(match, exportPattern) + QLatin1String("\n");
        }
    }
    this->exportResultText->setPlainText(exportResult);
}
