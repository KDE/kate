/*
    SPDX-FileCopyrightText: 2026 The Kate Developers

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kateprojectworker.h"

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QSignalSpy>
#include <QString>
#include <QTemporaryDir>
#include <QTest>

class KateProjectWorkerTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testGitSparseCheckout();
};

void KateProjectWorkerTest::testGitSparseCheckout()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString repoPath = tempDir.path();

    // Helper to run git commands
    auto runGit = [&repoPath](const QStringList &args) -> bool {
        QProcess proc;
        proc.setWorkingDirectory(repoPath);
        proc.start(QStringLiteral("git"), args);
        if (!proc.waitForStarted() || !proc.waitForFinished()) {
            return false;
        }
        return proc.exitStatus() == QProcess::NormalExit && proc.exitCode() == 0;
    };

    QVERIFY(runGit({QStringLiteral("init")}));
    QVERIFY(runGit({QStringLiteral("config"), QStringLiteral("user.name"), QStringLiteral("Test User")}));
    QVERIFY(runGit({QStringLiteral("config"), QStringLiteral("user.email"), QStringLiteral("test@example.com")}));

    // Create test files
    QFile file1(repoPath + QStringLiteral("/tracked_normal.txt"));
    QVERIFY(file1.open(QIODevice::WriteOnly));
    file1.write("normal content");
    file1.close();

    QFile file2(repoPath + QStringLiteral("/tracked_sparse.txt"));
    QVERIFY(file2.open(QIODevice::WriteOnly));
    file2.write("sparse content");
    file2.close();

    QFile file3(repoPath + QStringLiteral("/untracked_file.txt"));
    QVERIFY(file3.open(QIODevice::WriteOnly));
    file3.write("untracked content");
    file3.close();

    QVERIFY(runGit({QStringLiteral("add"), QStringLiteral("tracked_normal.txt"), QStringLiteral("tracked_sparse.txt")}));
    QVERIFY(runGit({QStringLiteral("commit"), QStringLiteral("-m"), QStringLiteral("Initial commit")}));

    // Mark tracked_sparse.txt as skip-worktree (simulate sparse-checkout)
    QVERIFY(runGit({QStringLiteral("update-index"), QStringLiteral("--skip-worktree"), QStringLiteral("tracked_sparse.txt")}));

    // Setup project map
    QVariantMap projectMap;
    projectMap[QStringLiteral("name")] = QStringLiteral("TestProject");
    QVariantMap gitEntry;
    gitEntry[QStringLiteral("git")] = 1;
    projectMap[QStringLiteral("files")] = QVariantList{gitEntry};

    KateProjectWorker worker(repoPath, QString(), projectMap, false);
    QSignalSpy spyLoadDone(&worker, &KateProjectWorker::loadDone);

    worker.run();

    QCOMPARE(spyLoadDone.count(), 1);

    auto file2Item = qvariant_cast<KateProjectSharedQHashStringItem>(spyLoadDone.takeFirst().at(1));
    QVERIFY(file2Item != nullptr);

    const QString normalPath = QDir(repoPath).absoluteFilePath(QStringLiteral("tracked_normal.txt"));
    const QString sparsePath = QDir(repoPath).absoluteFilePath(QStringLiteral("tracked_sparse.txt"));
    const QString untrackedPath = QDir(repoPath).absoluteFilePath(QStringLiteral("untracked_file.txt"));

    // Tracked normal and untracked file should be in file2Item
    QVERIFY(file2Item->contains(normalPath));
    QVERIFY(file2Item->contains(untrackedPath));

    // Sparse checkout (skip-worktree) file MUST NOT be in file2Item (Bug 525771)
    QVERIFY(!file2Item->contains(sparsePath));
}

QTEST_MAIN(KateProjectWorkerTest)

#include "KateProjectWorkerTest.moc"
