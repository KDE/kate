/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "stashdialog.h"
#include "git/gitutils.h"
#include "gitwidget.h"
#include "kateprojectpluginview.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPainter>
#include <QProcess>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QTemporaryFile>
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

constexpr int StashIndexRole = Qt::UserRole + 2;

class StashFilterModel final : public QSortFilterProxyModel
{
public:
    StashFilterModel(QObject *parent = nullptr)
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
        return sourceLeft.data(FuzzyScore).toInt() < sourceRight.data(FuzzyScore).toInt();
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
        sourceModel()->setData(idx, score, FuzzyScore);
        return res;
    }
private:
    static constexpr int FuzzyScore = Qt::UserRole + 1;
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

        QString name = index.data().toString();

        QVector<QTextLayout::FormatRange> formats;

        int colon = name.indexOf(QLatin1Char(':'));
        QString stashMessage = name.mid(colon + 1, name.length() - (colon + 1));
        ++colon;

        QTextCharFormat bold;
        bold.setFontWeight(QFont::Bold);
        formats.append({0, colon, bold});

        QTextCharFormat fmt;
        fmt.setForeground(options.palette.link());
        fmt.setFontWeight(QFont::Bold);
        auto resFmts = kfts::get_fuzzy_match_formats(m_filterString, stashMessage, colon, fmt);

        formats.append(resFmts);

        painter->save();

        //        // paint background
        if (option.state & QStyle::State_Selected) {
            painter->fillRect(option.rect, option.palette.highlight());
        } else {
            painter->fillRect(option.rect, option.palette.base());
        }

        options.text = QString(); // clear old text
        options.widget->style()->drawControl(QStyle::CE_ItemViewItem, &options, painter, options.widget);

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

StashDialog::StashDialog(GitWidget *gitwidget, QWidget *window)
    : QuickDialog(window)
    , m_gitwidget(gitwidget)
{
    m_model = new QStandardItemModel(this);
    m_proxyModel = new StashFilterModel(this);
    m_proxyModel->setSourceModel(m_model);
    m_treeView.setModel(m_proxyModel);

    StyleDelegate *delegate = new StyleDelegate(this);
    m_treeView.setItemDelegateForColumn(0, delegate);
    connect(&m_lineEdit, &QLineEdit::textChanged, delegate, [this, delegate](const QString &string) {
        m_proxyModel->setFilterString(string);
        delegate->setFilterString(string);
        // reselect first
        m_treeView.setCurrentIndex(m_proxyModel->index(0, 0));
    });
    m_proxyModel->setFilterRole(Qt::DisplayRole);
}

void StashDialog::openDialog(StashDialog::Mode m)
{
    m_model->clear();

    switch (m) {
    case Mode::Stash:
    case Mode::StashKeepIndex:
    case Mode::StashUntrackIncluded:
        m_lineEdit.setPlaceholderText(i18n("Stash message (optional). Enter to confirm, Esc to leave."));
        m_currentMode = m;
        break;
    case Mode::StashPop:
    case Mode::StashDrop:
    case Mode::StashApply:
    case Mode::ShowStashContent:
        m_lineEdit.setPlaceholderText(i18n("Type to filter, Enter to pop stash, Esc to leave."));
        m_currentMode = m;
        getStashList();
        break;
    case Mode::StashApplyLast:
        applyStash({});
        return;
    case Mode::StashPopLast:
        popStash({});
        return;
    default:
        return;
    }

    // trigger reselect first
    m_lineEdit.textChanged(QString());
    updateViewGeometry();
    setFocus();
    exec();
}

void StashDialog::slotReturnPressed()
{
    switch (m_currentMode) {
    case Mode::Stash:
        stash(false, false);
        break;
    case Mode::StashKeepIndex:
        stash(true, false);
        break;
    case Mode::StashUntrackIncluded:
        stash(false, true);
        break;
    case Mode::StashApply:
        applyStash(m_treeView.currentIndex().data(StashIndexRole).toByteArray());
        break;
    case Mode::StashPop:
        popStash(m_treeView.currentIndex().data(StashIndexRole).toByteArray());
        break;
    case Mode::StashDrop:
        dropStash(m_treeView.currentIndex().data(StashIndexRole).toByteArray());
        break;
    case Mode::ShowStashContent:
        showStash(m_treeView.currentIndex().data(StashIndexRole).toByteArray());
        break;
    default:
        break;
    }

    clearLineEdit();
    hide();
}

void StashDialog::sendMessage(const QString &message, bool warn)
{
    // just proxy to git widget
    m_gitwidget->sendMessage(message, warn);
}

void StashDialog::stash(bool keepIndex, bool includeUntracked)
{
    QStringList args{QStringLiteral("stash"), QStringLiteral("-q")};

    if (keepIndex) {
        args.append(QStringLiteral("--keep-index"));
    }
    if (includeUntracked) {
        args.append(QStringLiteral("-u"));
    }

    if (!m_lineEdit.text().isEmpty()) {
        args.append(QStringLiteral("-m"));
        args.append(m_lineEdit.text());
    }

    auto git = m_gitwidget->gitprocess();
    auto gitWidget = m_gitwidget;
    if (!git) {
        return;
    }

    disconnect(git, &QProcess::finished, nullptr, nullptr);
    connect(git, &QProcess::finished, m_gitwidget, [gitWidget](int exitCode, QProcess::ExitStatus es) {
        disconnect(gitWidget->gitprocess(), &QProcess::finished, nullptr, nullptr);
        if (es != QProcess::NormalExit || exitCode != 0) {
            gitWidget->sendMessage(i18n("Failed to stash changes"), true);
        } else {
            gitWidget->getStatus();
            gitWidget->sendMessage(i18n("Changes stashed successfully."), false);
        }
    });
    git->setArguments(args);
    git->start(QProcess::ReadOnly);
}

void StashDialog::getStashList()
{
    auto git = m_gitwidget->gitprocess();
    if (!git) {
        return;
    }

    git->setArguments({QStringLiteral("stash"), QStringLiteral("list")});
    git->start(QProcess::ReadOnly);

    QList<QByteArray> stashList;
    if (git->waitForStarted() && git->waitForFinished(-1)) {
        if (git->exitStatus() == QProcess::NormalExit && git->exitCode() == 0) {
            stashList = git->readAllStandardOutput().split('\n');
        } else {
            m_gitwidget->sendMessage(i18n("Failed to get stash list. Error:\n %1", QString::fromUtf8(git->readAllStandardError())), true);
        }
    }

    // format stash@{}: message
    for (const auto &stash : stashList) {
        if (!stash.startsWith("stash@{")) {
            continue;
        }
        int brackCloseIdx = stash.indexOf('}', 7);

        if (brackCloseIdx < 0) {
            continue;
        }

        QByteArray stashIdx = stash.mid(0, brackCloseIdx + 1);

        QStandardItem *item = new QStandardItem(QString::fromUtf8(stash));
        item->setData(stashIdx, StashIndexRole);
        m_model->appendRow(item);
    }
}

void StashDialog::popStash(const QByteArray &index, const QString &command)
{
    auto git = m_gitwidget->gitprocess();
    auto gitWidget = m_gitwidget;
    if (!git) {
        return;
    }

    disconnect(git, &QProcess::finished, nullptr, nullptr);
    QStringList args{QStringLiteral("stash"), command};
    if (!index.isEmpty()) {
        args.append(QString::fromUtf8(index));
    }

    connect(git, &QProcess::finished, gitWidget, [gitWidget, command](int exitCode, QProcess::ExitStatus es) {
        disconnect(gitWidget->gitprocess(), &QProcess::finished, nullptr, nullptr);
        if (es != QProcess::NormalExit || exitCode != 0) {
            auto git = gitWidget->gitprocess();
            if (command == QLatin1String("apply")) {
                gitWidget->sendMessage(i18n("Failed to apply stash. Error:\n%1", QString::fromUtf8(git->readAllStandardError())), true);
            } else if (command == QLatin1String("drop")) {
                gitWidget->sendMessage(i18n("Failed to drop stash. Error:\n%1", QString::fromUtf8(git->readAllStandardError())), true);
            } else {
                gitWidget->sendMessage(i18n("Failed to pop stash. Error:\n%1", QString::fromUtf8(git->readAllStandardError())), true);
            }
        } else {
            gitWidget->getStatus();
            if (command == QLatin1String("apply")) {
                gitWidget->sendMessage(i18n("Stash applied successfully."), false);
            } else if (command == QLatin1String("drop")) {
                gitWidget->sendMessage(i18n("Stash dropped successfully."), false);
            } else {
                gitWidget->sendMessage(i18n("Stash popped successfully."), false);
            }
        }
    });
    git->setArguments(args);
    git->start(QProcess::ReadOnly);
}

void StashDialog::applyStash(const QByteArray &index)
{
    popStash(index, QStringLiteral("apply"));
}

void StashDialog::dropStash(const QByteArray &index)
{
    popStash(index, QStringLiteral("drop"));
}

void StashDialog::showStash(const QByteArray &index)
{
    if (index.isEmpty()) {
        return;
    }

    auto git = m_gitwidget->gitprocess();
    auto gitWidget = m_gitwidget;
    if (!git) {
        return;
    }

    disconnect(git, &QProcess::finished, nullptr, nullptr);
    QStringList args{QStringLiteral("stash"), QStringLiteral("show"), QStringLiteral("-p"), QString::fromUtf8(index)};

    connect(git, &QProcess::finished, gitWidget, [gitWidget](int exitCode, QProcess::ExitStatus es) {
        disconnect(gitWidget->gitprocess(), &QProcess::finished, nullptr, nullptr);
        if (es != QProcess::NormalExit || exitCode != 0) {
            gitWidget->sendMessage(i18n("Show stash failed. Error:\n%1", QString::fromUtf8(gitWidget->gitprocess()->readAllStandardError())), true);
        } else {
            gitWidget->openTempFile(QString(), QStringLiteral("XXXXXX.diff"));
        }
    });

    git->setArguments(args);
    git->start(QProcess::ReadOnly);
}

