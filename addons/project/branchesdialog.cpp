/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "branchesdialog.h"
#include "branchesdialogmodel.h"
#include "gitprocess.h"
#include "ktexteditor_utils.h"

#include <QPainter>
#include <QTreeView>
#include <QWidget>

#include <KFuzzyMatcher>
#include <KLocalizedString>
#include <KTextEditor/Message>
#include <KTextEditor/View>
#include <utility>

#include <drawing_utils.h>

class StyleDelegate : public HUDStyleDelegate
{
public:
    using HUDStyleDelegate::HUDStyleDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QStyleOptionViewItem options = option;
        initStyleOption(&options, index);

        auto name = index.data().toString();

        QList<QTextLayout::FormatRange> formats;
        QTextCharFormat fmt;
        fmt.setForeground(options.palette.link());
        fmt.setFontWeight(QFont::Bold);

        const auto itemType = (BranchesDialogModel::ItemType)index.data(BranchesDialogModel::ItemTypeRole).toInt();
        const bool branchItem = itemType == BranchesDialogModel::BranchItem;
        const int offset = branchItem ? 0 : 2;

        const auto ranges = KFuzzyMatcher::matchedRanges(m_filterString, name);
        std::transform(ranges.begin(), ranges.end(), std::back_inserter(formats), [offset, fmt](const KFuzzyMatcher::Range &fr) {
            return QTextLayout::FormatRange{.start = fr.start + offset, .length = fr.length, .format = fmt};
        });

        if (!branchItem) {
            name = QStringLiteral("+ ") + name;
        }

        const int nameLen = name.length();
        int len = 6;
        if (branchItem) {
            const auto refType = (GitUtils::RefType)index.data(BranchesDialogModel::RefType).toInt();
            using RefType = GitUtils::RefType;
            if (refType == RefType::Head) {
                name.append(QStringLiteral(" local"));
            } else if (refType == RefType::Remote) {
                name.append(QStringLiteral(" remote"));
                len = 7;
            }
        }
        QTextCharFormat lf;
        lf.setFontItalic(true);
        lf.setForeground(Qt::gray);
        formats.append({.start = nameLen, .length = len, .format = lf});

        painter->save();

        auto *style = options.widget->style();
        options.text = QString(); // clear old text
        style->drawControl(QStyle::CE_ItemViewItem, &options, painter, options.widget);

        // leave space for icon
        const int hMargin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, &option, option.widget);
        const int iconWidth = option.decorationSize.width() + (hMargin * 2);
        if (itemType == BranchesDialogModel::BranchItem) {
            options.rect.adjust(iconWidth, 0, 0, 0);
        } else {
            options.rect.adjust((hMargin * 2), 0, 0, 0);
        }
        Utils::paintItemViewText(painter, name, options, formats);

        painter->restore();
    }
};

BranchesDialog::BranchesDialog(QWidget *window, QString projectPath)
    : HUDDialog(window)
    , m_model(new BranchesDialogModel(this))
    , m_projectPath(std::move(projectPath))
{
    setModel(m_model, FilterType::ScoredFuzzy, 0, Qt::DisplayRole, BranchesDialogModel::FuzzyScore);
    setDelegate(new StyleDelegate(this));
}

void BranchesDialog::openDialog(GitUtils::RefType r)
{
    m_lineEdit.setPlaceholderText(i18n("Select Branch..."));

    QList<GitUtils::Branch> branches = GitUtils::getAllBranchesAndTags(m_projectPath, r);
    m_model->refresh(branches);

    reselectFirst();
    raise();
    show();
}

void BranchesDialog::sendMessage(const QString &plainText, bool warn)
{
    Utils::showMessage(plainText, gitIcon(), i18n("Git"), warn ? MessageType::Error : MessageType::Info);
}
