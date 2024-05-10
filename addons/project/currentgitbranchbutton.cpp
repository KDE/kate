/*
    SPDX-FileCopyrightText: 2023 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "currentgitbranchbutton.h"

#include "gitprocess.h"
#include "hostprocess.h"

#include <KAcceleratorManager>
#include <KLocalizedString>
#include <KTextEditor/Document>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>
#include <QPointer>

#include <QFileInfo>
#include <QProcess>
#include <QtConcurrentRun>

static CurrentGitBranchButton::BranchResult getCurrentBranchName(const QString &workingDir)
{
    const QStringList argsList[3] = {
        {QStringLiteral("symbolic-ref"), QStringLiteral("--short"), QStringLiteral("HEAD")},
        {QStringLiteral("rev-parse"), QStringLiteral("--short"), QStringLiteral("HEAD")},
        {QStringLiteral("describe"), QStringLiteral("--exact-match"), QStringLiteral("HEAD")},
    };

    for (int i = 0; i < 3; ++i) {
        QProcess git;
        if (!setupGitProcess(git, workingDir, argsList[i])) {
            return {};
        }

        startHostProcess(git, QProcess::ReadOnly);
        if (git.waitForStarted() && git.waitForFinished(-1)) {
            if (git.exitStatus() == QProcess::NormalExit && git.exitCode() == 0) {
                return {
                    .branch = QString::fromUtf8(git.readAllStandardOutput().trimmed()),
                    .type = static_cast<CurrentGitBranchButton::BranchType>(i),
                };
            }
        }
    }

    // give up
    return {};
}

CurrentGitBranchButton::CurrentGitBranchButton(KTextEditor::MainWindow *mainWindow, QWidget *parent)
    : QToolButton(parent)
{
    setVisible(false);
    setAutoRaise(true);
    setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_viewChangedTimer.setSingleShot(true);
    m_viewChangedTimer.setInterval(1000);
    KAcceleratorManager::setNoAccel(this);

    auto mw = QPointer<KTextEditor::MainWindow>(mainWindow);
    connect(mainWindow, &KTextEditor::MainWindow::viewChanged, this, [this](KTextEditor::View *v) {
        if (!v || v->document()->url().toLocalFile().isEmpty()) {
            hideButton();
            m_viewChangedTimer.stop();
            return;
        }
        m_viewChangedTimer.start();
    });
    m_viewChangedTimer.callOnTimeout(this, [this, mw] {
        if (mw) {
            onViewChanged(mw->activeView());
        }
    });

    connect(&m_watcher, &QFutureWatcher<QString>::finished, this, &CurrentGitBranchButton::onBranchFetched);
    onViewChanged(mainWindow->activeView());
}

CurrentGitBranchButton::~CurrentGitBranchButton()
{
    m_viewChangedTimer.stop();
    if (m_watcher.isRunning()) {
        disconnect(&m_watcher, &QFutureWatcher<BranchResult>::finished, this, &CurrentGitBranchButton::onBranchFetched);
        m_watcher.cancel();
        m_watcher.waitForFinished();
    }
}

void CurrentGitBranchButton::hideButton()
{
    setText({});
    setVisible(false);
}

void CurrentGitBranchButton::refresh()
{
    m_viewChangedTimer.start();
}

void CurrentGitBranchButton::onViewChanged(KTextEditor::View *v)
{
    if (!v || v->document()->url().toLocalFile().isEmpty()) {
        hideButton();
        return;
    }

    const auto fi = QFileInfo(v->document()->url().toLocalFile());
    const auto workingDir = fi.absolutePath();
    auto future = QtConcurrent::run(&getCurrentBranchName, workingDir);
    m_watcher.setFuture(future);
}

void CurrentGitBranchButton::onBranchFetched()
{
    const BranchResult branch = m_watcher.result();
    if (branch.branch.isEmpty()) {
        hideButton();
        return;
    }

    setText(branch.branch);

    if (branch.type == Commit) {
        setToolTip(i18nc("Tooltip text, describing that '%1' commit is checked out", "HEAD at commit %1", branch.branch));
    } else if (branch.type == Tag) {
        setToolTip(i18nc("Tooltip text, describing that '%1' tag is checked out", "HEAD is at this tag %1", branch.branch));
    } else if (branch.type == Branch) {
        setToolTip(i18nc("Tooltip text, describing that '%1' branch is checked out", "Active branch: %1", branch.branch));
    }

    if (!isVisible()) {
        setVisible(true);
    }
}
