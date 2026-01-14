/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "pushpulldialog.h"

#include <QFile>
#include <QProcess>
#include <QSettings>

#include <KConfigGroup>
#include <KSharedConfig>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>

#include <gitprocess.h>
#include <hostprocess.h>
#include <ktexteditor_utils.h>

PushPullDialog::PushPullDialog(KTextEditor::MainWindow *mainWindow, const QString &repoPath)
    : HUDDialog(mainWindow->window())
    , m_repo(repoPath)
{
    m_lineEdit.setFont(Utils::editorFont());
    m_treeView.setFont(Utils::editorFont());
    setFilteringEnabled(false);
    loadLastExecutedCommands();
    detectGerrit();
}

void PushPullDialog::openDialog(PushPullDialog::Mode m)
{
    // build the string
    QStringList builtStrings;
    if (m == Push && m_isGerrit) {
        builtStrings << QStringLiteral("git push origin HEAD:refs/for/%1").arg(m_gerritBranch);
    } else {
        builtStrings = buildCmdStrings(m);
    }
    // find if we have a last executed push/pull command
    QString lastCmd = getLastPushPullCmd(m);

    QStringList lastExecCmds = m_lastExecutedCommands;

    // if found, bring it up
    if (!lastCmd.isEmpty()) {
        lastExecCmds.removeAll(lastCmd);
        lastExecCmds.push_front(lastCmd);
    }

    for (const auto &s : std::as_const(builtStrings)) {
        lastExecCmds.removeAll(s);
        lastExecCmds.push_front(s);
    }

    setStringList(lastExecCmds);

    connect(m_treeView.selectionModel(), &QItemSelectionModel::currentChanged, this, [this](const QModelIndex &current, const QModelIndex &) {
        m_lineEdit.setText(current.data().toString());
    });

    reselectFirst();

    raise();
    show();
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
    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("kategit"));
    m_lastExecutedCommands = config.readEntry("lastExecutedGitCmds", QStringList());
}

void PushPullDialog::saveCommand(const QString &command)
{
    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("kategit"));
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
    if (!setupGitProcess(git, repo, {QStringLiteral("symbolic-ref"), QStringLiteral("--short"), QStringLiteral("HEAD")})) {
        return {};
    }

    startHostProcess(git, QIODevice::ReadOnly);
    if (git.waitForStarted() && git.waitForFinished(-1)) {
        if (git.exitStatus() == QProcess::NormalExit && git.exitCode() == 0) {
            return QString::fromUtf8(git.readAllStandardOutput().trimmed());
        }
    }
    // give up
    return {};
}

static QStringList remotesList(const QString &repo)
{
    QProcess git;
    if (!setupGitProcess(git, repo, {QStringLiteral("remote")})) {
        return {};
    }

    startHostProcess(git, QIODevice::ReadOnly);
    if (git.waitForStarted() && git.waitForFinished(-1)) {
        if (git.exitStatus() == QProcess::NormalExit && git.exitCode() == 0) {
            return QString::fromUtf8(git.readAllStandardOutput()).split(QLatin1Char('\n'), Qt::SkipEmptyParts);
        }
    }
    return {};
}

static QString getRemoteForCurrentBranch(const QString &repo, const QString &branch)
{
    QProcess git;
    const QStringList args{QStringLiteral("config"), QStringLiteral("branch.%1.remote").arg(branch)};
    if (!setupGitProcess(git, repo, args)) {
        return {};
    }

    startHostProcess(git, QIODevice::ReadOnly);
    if (git.waitForStarted() && git.waitForFinished(-1)) {
        if (git.exitStatus() == QProcess::NormalExit && git.exitCode() == 0) {
            return QString::fromUtf8(git.readAllStandardOutput().trimmed());
        }
    }
    return {};
}

QStringList PushPullDialog::buildCmdStrings(Mode m)
{
    const QString arg = m == Push ? QStringLiteral("push") : QStringLiteral("pull");
    const auto br = currentBranchName(m_repo);
    if (br.isEmpty()) {
        return {QStringLiteral("git %1").arg(arg)};
    }

    auto remoteForBranch = getRemoteForCurrentBranch(m_repo, br);
    if (remoteForBranch.isEmpty()) {
        const auto remotes = remotesList(m_repo);
        if (remotes.isEmpty()) {
            return {QStringLiteral("git %1").arg(arg)};
        }
        QStringList cmds;
        // reverse traversal as later, these commands will be pushed in front of the
        // list displayed to user, so we invert the order here and it will appear in
        // the same order that git shows
        for (auto ri = remotes.crbegin(); ri != remotes.crend(); ++ri) {
            cmds << QStringLiteral("git %1 %2 %3").arg(arg, *ri, br);
        }
        return cmds;
    } else {
        // if we found a remote, only offer that
        return {QStringLiteral("git %1 %2 %3").arg(arg, remoteForBranch, br)};
    }
}

void PushPullDialog::slotReturnPressed(const QModelIndex &)
{
    if (!m_lineEdit.text().isEmpty()) {
        auto args = m_lineEdit.text().split(QLatin1Char(' '));
        if (args.first() == QLatin1String("git")) {
            saveCommand(m_lineEdit.text());
            args.pop_front();
            Q_EMIT runGitCommand(args);
        }
    }

    hide();
    deleteLater();
}

void PushPullDialog::detectGerrit()
{
    if (QFile::exists(m_repo + QLatin1String(".gitreview"))) {
        m_isGerrit = true;
        QSettings s(m_repo + QLatin1String("/.gitreview"), QSettings::IniFormat);
        m_gerritBranch = s.value(QStringLiteral("gerrit/defaultbranch")).toString();
    }
}

#include "moc_pushpulldialog.cpp"
