/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "pushpulldialog.h"

#include <QProcess>
#include <QStandardItemModel>

#include <KConfigGroup>
#include <KSharedConfig>
#include <KTextEditor/ConfigInterface>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>

PushPullDialog::PushPullDialog(KTextEditor::MainWindow *mainWindow, const QString &repoPath)
    : QuickDialog(nullptr, mainWindow->window())
    , m_repo(repoPath)
{
    auto ciface = qobject_cast<KTextEditor::ConfigInterface *>(mainWindow->activeView());
    Q_ASSERT(ciface);
    m_lineEdit.setFont(ciface->configValue(QStringLiteral("font")).value<QFont>());

    loadLastExecutedCommands();
}

void PushPullDialog::openDialog(PushPullDialog::Mode m)
{
    // build the string
    QString builtString = m == Push ? buildPushString() : buildPullString();
    // find if we have a last executed push/pull command
    QString lastCmd = getLastPushPullCmd(m);

    QStringList lastExecCmds = m_lastExecutedCommands;

    if (!lastExecCmds.contains(builtString)) {
        lastExecCmds.push_front(builtString);
    }

    // if found, bring it up
    if (!lastCmd.isEmpty()) {
        lastExecCmds.removeAll(lastCmd);
        lastExecCmds.push_front(lastCmd);
    }

    auto *model = new QStandardItemModel(this);
    m_treeView.setModel(model);

    auto monoFont = m_lineEdit.font();

    for (const QString &cmd : std::as_const(lastExecCmds)) {
        auto *item = new QStandardItem(cmd);
        item->setFont(monoFont);
        model->appendRow(item);
    }

    connect(m_treeView.selectionModel(), &QItemSelectionModel::currentChanged, this, [this](const QModelIndex &current, const QModelIndex &) {
        m_lineEdit.setText(current.data().toString());
    });

    m_treeView.setCurrentIndex(model->index(0, 0));

    exec();
}

QString PushPullDialog::getLastPushPullCmd(Mode m) const
{
    const QString cmdToFind = m == Push ? QStringLiteral("git push") : QStringLiteral("git pull");
    QString found;
    for (const auto &cmd : m_lastExecutedCommands) {
        if (cmd.startsWith(cmdToFind)) {
            found = cmd;
            break;
        }
    }
    return found;
}

void PushPullDialog::loadLastExecutedCommands()
{
    KConfigGroup config(KSharedConfig::openConfig(), "kategit");
    m_lastExecutedCommands = config.readEntry("lastExecutedGitCmds", QStringList());
}

void PushPullDialog::saveCommand(const QString &command)
{
    KConfigGroup config(KSharedConfig::openConfig(), "kategit");
    QStringList cmds = m_lastExecutedCommands;
    cmds.removeAll(command);
    cmds.push_front(command);
    while (cmds.size() > 8) {
        cmds.pop_back();
    }
    config.writeEntry("lastExecutedGitCmds", cmds);
}

/**
 * This is not for display, hence not reusing gitutils here
 */
static QString currentBranchName(const QString &repo)
{
    QProcess git;
    git.setWorkingDirectory(repo);

    QStringList args{QStringLiteral("symbolic-ref"), QStringLiteral("--short"), QStringLiteral("HEAD")};

    git.start(QStringLiteral("git"), args, QProcess::ReadOnly);
    if (git.waitForStarted() && git.waitForFinished(-1)) {
        if (git.exitStatus() == QProcess::NormalExit && git.exitCode() == 0) {
            return QString::fromUtf8(git.readAllStandardOutput().trimmed());
        }
    }
    // give up
    return QString();
}

static QStringList remotesList(const QString &repo)
{
    QProcess git;
    git.setWorkingDirectory(repo);

    QStringList args{QStringLiteral("remote")};

    git.start(QStringLiteral("git"), args, QProcess::ReadOnly);
    if (git.waitForStarted() && git.waitForFinished(-1)) {
        if (git.exitStatus() == QProcess::NormalExit && git.exitCode() == 0) {
            return QString::fromUtf8(git.readAllStandardOutput()).split(QLatin1Char('\n'));
        }
    }
    return {};
}

QString PushPullDialog::buildPushString()
{
    auto br = currentBranchName(m_repo);
    if (br.isEmpty()) {
        return QStringLiteral("git push");
    }

    auto remotes = remotesList(m_repo);
    if (!remotes.contains(QStringLiteral("origin"))) {
        return QStringLiteral("git push");
    }

    return QStringLiteral("git push %1 %2").arg(QStringLiteral("origin")).arg(br);
}

QString PushPullDialog::buildPullString()
{
    auto br = currentBranchName(m_repo);
    if (br.isEmpty()) {
        return QStringLiteral("git pull");
    }

    auto remotes = remotesList(m_repo);
    if (!remotes.contains(QStringLiteral("origin"))) {
        return QStringLiteral("git pull");
    }

    return QStringLiteral("git pull %1 %2").arg(QStringLiteral("origin")).arg(br);
}

void PushPullDialog::slotReturnPressed()
{
    if (!m_lineEdit.text().isEmpty()) {
        auto args = m_lineEdit.text().split(QLatin1Char(' '));
        if (args.first() == QStringLiteral("git")) {
            saveCommand(m_lineEdit.text());
            args.pop_front();
            Q_EMIT runGitCommand(args);
        }
    }

    hide();
}
