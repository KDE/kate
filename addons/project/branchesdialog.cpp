/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "branchesdialog.h"
#include "branchesdialogmodel.h"
#include "git/gitutils.h"
#include "kateprojectpluginview.h"

#include <QCoreApplication>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPainter>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>
#include <QTextDocument>
#include <QTreeView>
#include <QVBoxLayout>
#include <QWidget>
#include <QtConcurrentRun>

#include <KTextEditor/MainWindow>
#include <KTextEditor/Message>
#include <KTextEditor/View>

#include <KLocalizedString>

#include <kfts_fuzzy_match.h>

class BranchFilterModel : public QSortFilterProxyModel
{
public:
    BranchFilterModel(QObject *parent = nullptr)
        : QSortFilterProxyModel(parent)
    {
    }

    Q_SLOT void setFilterString(const QString &string)
    {
        beginResetModel();
        m_pattern = string;
        endResetModel();
    }

protected:
    bool lessThan(const QModelIndex &sourceLeft, const QModelIndex &sourceRight) const override
    {
        if (m_pattern.isEmpty()) {
            const int l = sourceLeft.data(BranchesDialogModel::OriginalSorting).toInt();
            const int r = sourceRight.data(BranchesDialogModel::OriginalSorting).toInt();
            return l > r;
        }
        const int l = sourceLeft.data(BranchesDialogModel::FuzzyScore).toInt();
        const int r = sourceRight.data(BranchesDialogModel::FuzzyScore).toInt();
        return l < r;
    }

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override
    {
        if (m_pattern.isEmpty()) {
            return true;
        }

        int score = 0;
        const auto idx = sourceModel()->index(sourceRow, 0, sourceParent);
        const QString string = idx.data().toString();
        const bool res = kfts::fuzzy_match(m_pattern, string, score);
        sourceModel()->setData(idx, score, BranchesDialogModel::FuzzyScore);
        return res;
    }

private:
    QString m_pattern;
};

class StyleDelegate : public QStyledItemDelegate
{
public:
    StyleDelegate(QObject *parent = nullptr)
        : QStyledItemDelegate(parent)
    {
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QStyleOptionViewItem options = option;
        initStyleOption(&options, index);

        auto name = index.data().toString();

        QVector<QTextLayout::FormatRange> formats;
        QTextCharFormat fmt;
        fmt.setForeground(options.palette.link());
        fmt.setFontWeight(QFont::Bold);

        const auto itemType = (BranchesDialogModel::ItemType)index.data(BranchesDialogModel::ItemTypeRole).toInt();
        const bool branchItem = itemType == BranchesDialogModel::BranchItem;
        const int offset = branchItem ? 0 : 2;

        formats = kfts::get_fuzzy_match_formats(m_filterString, name, offset, fmt);

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
        formats.append({nameLen, len, lf});

        painter->save();

        // paint background
        if (option.state & QStyle::State_Selected) {
            painter->fillRect(option.rect, option.palette.highlight());
        } else {
            painter->fillRect(option.rect, option.palette.base());
        }

        options.text = QString(); // clear old text
        options.widget->style()->drawControl(QStyle::CE_ItemViewItem, &options, painter, options.widget);

        // leave space for icon
        if (itemType == BranchesDialogModel::BranchItem) {
            painter->translate(25, 0);
        }
        kfts::paintItemViewText(painter, name, options, formats);

        painter->restore();
    }

public Q_SLOTS:
    void setFilterString(const QString &text)
    {
        m_filterString = text;
    }

private:
    QString m_filterString;
};

BranchesDialog::BranchesDialog(QWidget *window, KateProjectPluginView *pluginView, QString projectPath)
    : QuickDialog(window)
    , m_pluginView(pluginView)
    , m_projectPath(projectPath)
{
    m_model = new BranchesDialogModel(this);
    m_proxyModel = new BranchFilterModel(this);
    m_proxyModel->setSourceModel(m_model);
    m_treeView.setModel(m_proxyModel);

    auto delegate = new StyleDelegate(this);

    connect(&m_lineEdit, &QLineEdit::textChanged, this, [this, delegate](const QString &s) {
        m_proxyModel->setFilterString(s);
        delegate->setFilterString(s);
    });

    connect(&m_checkoutWatcher, &QFutureWatcher<GitUtils::CheckoutResult>::finished, this, &BranchesDialog::onCheckoutDone);
}

BranchesDialog::~BranchesDialog()
{
    if (m_checkoutWatcher.isRunning()) {
        onCheckoutDone();
    }
}

void BranchesDialog::resetValues()
{
    m_checkoutBranchName.clear();
    m_checkingOutFromBranch = false;
    m_lineEdit.setPlaceholderText(i18n("Select branch to checkout. Press 'Esc' to cancel."));
}

void BranchesDialog::openDialog()
{
    resetValues();
    GitUtils::Branch newBranch;
    newBranch.name = i18n("Create New Branch");
    GitUtils::Branch newBranchFrom;
    newBranchFrom.name = i18n("Create New Branch From...");
    QVector<GitUtils::Branch> branches{newBranch, newBranchFrom};
    /*QVector<GitUtils::Branch> */ branches << GitUtils::getAllBranches(m_projectPath);
    m_model->refresh(branches);

    reselectFirst();
    updateViewGeometry();
    setFocus();
    exec();
}

void BranchesDialog::onCheckoutDone()
{
    const GitUtils::CheckoutResult res = m_checkoutWatcher.result();
    auto msgType = KTextEditor::Message::Positive;
    QString msgStr = i18n("Branch %1 checked out", res.branch);
    if (res.returnCode > 0) {
        msgType = KTextEditor::Message::Warning;
        msgStr = i18n("Failed to checkout to branch %1, Error: %2", res.branch, res.error);
    }

    sendMessage(msgStr, msgType == KTextEditor::Message::Warning);
}

void BranchesDialog::slotReturnPressed()
{
    // we cleared the model to checkout new branch
    if (m_model->rowCount() == 0) {
        createNewBranch(m_lineEdit.text(), m_checkoutBranchName);
        return;
    }

    // branch is selected, do actual checkout
    if (m_checkingOutFromBranch) {
        m_checkingOutFromBranch = false;
        const auto fromBranch = m_proxyModel->data(m_treeView.currentIndex(), BranchesDialogModel::CheckoutName).toString();
        m_checkoutBranchName = fromBranch;
        m_model->clear();
        clearLineEdit();
        m_lineEdit.setPlaceholderText(i18n("Enter new branch name. Press 'Esc' to cancel."));
        return;
    }

    const auto branch = m_proxyModel->data(m_treeView.currentIndex(), BranchesDialogModel::CheckoutName).toString();
    const auto itemType = (BranchesDialogModel::ItemType)m_proxyModel->data(m_treeView.currentIndex(), BranchesDialogModel::ItemTypeRole).toInt();

    if (itemType == BranchesDialogModel::BranchItem) {
        QFuture<GitUtils::CheckoutResult> future = QtConcurrent::run(&GitUtils::checkoutBranch, m_projectPath, branch);
        m_checkoutWatcher.setFuture(future);
    } else if (itemType == BranchesDialogModel::CreateBranch) {
        m_model->clear();
        m_lineEdit.setPlaceholderText(i18n("Enter new branch name. Press 'Esc' to cancel."));
        return;
    } else if (itemType == BranchesDialogModel::CreateBranchFrom) {
        m_model->clearBranchCreationItems();
        clearLineEdit();
        m_lineEdit.setPlaceholderText(i18n("Select branch to checkout from. Press 'Esc' to cancel."));
        m_checkingOutFromBranch = true;
        return;
    }

    clearLineEdit();
    hide();
}

void BranchesDialog::reselectFirst()
{
    QModelIndex index = m_proxyModel->index(0, 0);
    m_treeView.setCurrentIndex(index);
}

void BranchesDialog::sendMessage(const QString &plainText, bool warn)
{
    // use generic output view
    QVariantMap genericMessage;
    genericMessage.insert(QStringLiteral("type"), warn ? QStringLiteral("Error") : QStringLiteral("Info"));
    genericMessage.insert(QStringLiteral("category"), i18n("Git"));
    genericMessage.insert(QStringLiteral("categoryIcon"), QIcon(QStringLiteral(":/icons/icons/sc-apps-git.svg")));
    genericMessage.insert(QStringLiteral("text"), plainText);
    Q_EMIT m_pluginView->message(genericMessage);
}

void BranchesDialog::createNewBranch(const QString &branch, const QString &fromBranch)
{
    if (branch.isEmpty()) {
        clearLineEdit();
        hide();
        return;
    }

    // the branch name might be invalid, let git handle it
    const GitUtils::CheckoutResult r = GitUtils::checkoutNewBranch(m_projectPath, branch, fromBranch);
    const bool warn = true;
    if (r.returnCode == 0) {
        sendMessage(i18n("Checked out to new branch: %1", r.branch), !warn);
    } else {
        sendMessage(i18n("Failed to create new branch. Error \"%1\"", r.error), warn);
    }

    clearLineEdit();
    hide();
}
