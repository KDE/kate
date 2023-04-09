/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "stashdialog.h"
#include "git/gitutils.h"
#include "gitwidget.h"
#include "kateprojectpluginview.h"

#include <QProcess>
#include <QTreeView>
#include <QWidget>

#include <KTextEditor/View>

#include <KLocalizedString>

#include <drawing_utils.h>

#include <gitprocess.h>

StashDialog::StashDialog(QWidget *parent, QWidget *window, const QString &gitPath)
    : HUDDialog(parent, window)
    , m_gitPath(gitPath)
{
}

void StashDialog::openDialog(StashMode m)
{
    setStringList({});

    switch (m) {
    case StashMode::Stash:
    case StashMode::StashKeepIndex:
    case StashMode::StashUntrackIncluded:
        m_lineEdit.setPlaceholderText(i18n("Stash message (optional). Enter to confirm, Esc to leave."));
        m_currentMode = m;
        break;
    case StashMode::StashPop:
    case StashMode::StashDrop:
    case StashMode::StashApply:
    case StashMode::ShowStashContent:
        m_lineEdit.setPlaceholderText(i18n("Type to filter, Enter to pop stash, Esc to leave."));
        m_currentMode = m;
        getStashList();
        break;
    case StashMode::StashApplyLast:
        applyStash({});
        return;
    case StashMode::StashPopLast:
        popStash({});
        return;
    default:
        return;
    }

    // trigger reselect first
    m_lineEdit.textChanged(QString());
    exec();
}

static QString getStashIndex(const QModelIndex &index)
{
    QString s = index.data().toString();
    if (s.isEmpty() || !s.startsWith(QStringLiteral("stash@{"))) {
        return {};
    }
    static QRegularExpression re(QStringLiteral("stash@{(.*)}"));
    const auto match = re.match(s);
    if (!match.hasMatch())
        return {};
    return match.captured(1);
}

void StashDialog::slotReturnPressed(const QModelIndex &index)
{
    switch (m_currentMode) {
    case StashMode::Stash:
        stash(false, false);
        break;
    case StashMode::StashKeepIndex:
        stash(true, false);
        break;
    case StashMode::StashUntrackIncluded:
        stash(false, true);
        break;
    default:
        break;
    }

    auto stashIndex = getStashIndex(index);
    if (stashIndex.isEmpty())
        return;

    switch (m_currentMode) {
    case StashMode::StashApply:
        applyStash(stashIndex);
        break;
    case StashMode::StashPop:
        popStash(stashIndex);
        break;
    case StashMode::StashDrop:
        dropStash(stashIndex);
        break;
    case StashMode::ShowStashContent:
        showStash(stashIndex);
        break;
    default:
        break;
    }

    hide();
}

QProcess *StashDialog::gitp(const QStringList &arguments)
{
    auto git = new QProcess(this);
    setupGitProcess(*git, m_gitPath, arguments);
    return git;
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

    auto git = gitp(args);
    connect(git, &QProcess::finished, this, [this, git](int exitCode, QProcess::ExitStatus es) {
        if (es != QProcess::NormalExit || exitCode != 0) {
            qWarning() << git->errorString();
            Q_EMIT message(i18n("Failed to stash changes %1", QString::fromUtf8(git->readAllStandardError())), true);
        } else {
            Q_EMIT message(i18n("Changes stashed successfully."), false);
        }
        Q_EMIT done();
        git->deleteLater();
    });
    startHostProcess(*git, QProcess::ReadOnly);
}

void StashDialog::getStashList()
{
    auto git = gitp({QStringLiteral("stash"), QStringLiteral("list")});
    startHostProcess(*git, QProcess::ReadOnly);

    QStringList stashList;
    if (git->waitForStarted() && git->waitForFinished(-1)) {
        if (git->exitStatus() == QProcess::NormalExit && git->exitCode() == 0) {
            stashList = QString::fromUtf8(git->readAllStandardOutput()).split(QLatin1Char('\n'));
            setStringList(stashList);
        } else {
            Q_EMIT message(i18n("Failed to get stash list. Error: ") + QString::fromUtf8(git->readAll()), true);
        }
    }
}

void StashDialog::popStash(const QString &index, const QString &command)
{
    QStringList args{QStringLiteral("stash"), command};
    if (!index.isEmpty()) {
        args.append(index);
    }
    auto git = gitp(args);

    connect(git, &QProcess::finished, this, [this, command, git](int exitCode, QProcess::ExitStatus es) {
        if (es != QProcess::NormalExit || exitCode != 0) {
            if (command == QLatin1String("apply")) {
                Q_EMIT message(i18n("Failed to apply stash. Error: ") + QString::fromUtf8(git->readAll()), true);
            } else if (command == QLatin1String("drop")) {
                Q_EMIT message(i18n("Failed to drop stash. Error: ") + QString::fromUtf8(git->readAll()), true);
            } else {
                Q_EMIT message(i18n("Failed to pop stash. Error: ") + QString::fromUtf8(git->readAll()), true);
            }
        } else {
            if (command == QLatin1String("apply")) {
                Q_EMIT message(i18n("Stash applied successfully."), false);
            } else if (command == QLatin1String("drop")) {
                Q_EMIT message(i18n("Stash dropped successfully."), false);
            } else {
                Q_EMIT message(i18n("Stash popped successfully."), false);
            }
        }
        Q_EMIT done();
        git->deleteLater();
    });
    startHostProcess(*git, QProcess::ReadOnly);
}

void StashDialog::applyStash(const QString &index)
{
    popStash(index, QStringLiteral("apply"));
}

void StashDialog::dropStash(const QString &index)
{
    popStash(index, QStringLiteral("drop"));
}

void StashDialog::showStash(const QString &index)
{
    if (index.isEmpty()) {
        return;
    }

    auto git = gitp({QStringLiteral("stash"), QStringLiteral("show"), QStringLiteral("-p"), index});

    connect(git, &QProcess::finished, this, [this, git](int exitCode, QProcess::ExitStatus es) {
        if (es != QProcess::NormalExit || exitCode != 0) {
            Q_EMIT message(i18n("Show stash failed. Error: ") + QString::fromUtf8(git->readAll()), true);
        } else {
            Q_EMIT showStashDiff(git->readAllStandardOutput());
        }
        Q_EMIT done();
        git->deleteLater();
    });

    startHostProcess(*git, QProcess::ReadOnly);
}
