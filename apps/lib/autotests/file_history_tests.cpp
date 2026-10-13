/*
    SPDX-FileCopyrightText: 2024 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "kateapp.h"

#include "filehistorywidget.h"
#include "gitprocess.h"
#include "hostprocess.h"
#include "katemainwindow.h"
#include "ktexteditor_utils.h"

#include <KLocalizedString>
#include <KSharedConfig>
#include <KTextEditor/Editor>

#include <QCommandLineParser>
#include <QIcon>
#include <QLineEdit>
#include <QListWidget>
#include <QObject>
#include <QPointer>
#include <QProcess>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QSortFilterProxyModel>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QTest>
#include <QTextBrowser>
#include <QToolButton>
#include <memory>

class KateApp;
class KateViewSpace;
class KateViewManager;

// This class uses Kate as the test KateApp mode
class FileHistoryTest : public QObject
{
    Q_OBJECT

public:
    FileHistoryTest(QObject *parent = nullptr);

private Q_SLOTS:
    void smokeTest();
    void testFiltering();
    void testAuthorAndDateFiltering();
    void testLargeHistoryParsedCompletely();

private:
    QWidget *getToolview()
    {
        KateMainWindow *mw = app->activeKateMainWindow();
        const QString toolViewIdentifier = QStringLiteral("git_file_history_%1").arg(testFile);
        // Expect toolview
        return Utils::toolviewForName(mw->wrapper(), toolViewIdentifier);
    }

    bool waitForGitLogFinish()
    {
        auto fileHistoryWidget = getToolview()->findChild<QWidget *>(QStringLiteral("FileHistoryWidget"));
        Q_ASSERT(fileHistoryWidget);
        QSignalSpy spy(fileHistoryWidget, SIGNAL(gitLogDone()));
        return spy.count() == 1 || spy.wait();
    }

    QToolButton *getCloseButton(QWidget *toolview)
    {
        const auto childs = toolview->findChildren<QToolButton *>();
        for (QToolButton *child : childs) {
            if (child->icon().name() == QLatin1String("tab-close")) {
                return child;
            }
        }
        return nullptr;
    }

private:
    std::unique_ptr<QTemporaryDir> m_tempdir;
    std::unique_ptr<KateApp> app;
    QString testFile;
    bool haveGit = false;
};

FileHistoryTest::FileHistoryTest(QObject *)
{
    // ensure we are in test mode, for the part, too
    QStandardPaths::setTestModeEnabled(true);

    // ensure ui file can be found and the translation domain is set to avoid warnings
    qApp->setApplicationName(QStringLiteral("kate"));
    KLocalizedString::setApplicationDomain(QByteArrayLiteral("kate"));

    m_tempdir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_tempdir->isValid());

    // ensure we use some dummy config
    KConfig::setMainConfigName(m_tempdir->path() + QStringLiteral("/testconfigfilerc"));

    static QCommandLineParser parser;
    app = std::make_unique<KateApp>(parser, KateApp::ApplicationKate, m_tempdir->path());
    app->sessionManager()->sessionNew();

    QDir thisDir(QStringLiteral(QT_TESTCASE_SOURCEDIR));
    testFile = thisDir.absoluteFilePath(QStringLiteral("CMakeLists.txt"));
    qDebug("File is %ls", qUtf16Printable(testFile));

    KateMainWindow *mw = app->activeKateMainWindow();
    QVERIFY(mw->openUrl(QUrl::fromLocalFile(testFile)));

    auto version = getGitVersion(QFileInfo(testFile).absolutePath());
    haveGit = version.major != -1;
}

void FileHistoryTest::smokeTest()
{
    if (!haveGit) {
        QSKIP("Skipping the test, 'git' not found");
    }

    KateMainWindow *mw = app->activeKateMainWindow();
    FileHistory::showFileHistory(testFile, mw->wrapper());

    auto toolview = getToolview();
    QVERIFY(toolview);

    auto commitModel = toolview->findChild<QAbstractListModel *>();
    QVERIFY(commitModel);
    QTRY_VERIFY(commitModel->rowCount() != 0);

    auto filterLineEdit = toolview->findChild<QLineEdit *>();
    QVERIFY(filterLineEdit);
    QVERIFY(filterLineEdit->isVisible());

    auto filtersList = toolview->findChild<QListWidget *>();
    QVERIFY(filtersList);
    QVERIFY(!filtersList->isVisible());

    auto closeBtn = getCloseButton(toolview);
    QVERIFY(closeBtn);
    closeBtn->click();

    QTRY_VERIFY(getToolview() == nullptr);
}

void FileHistoryTest::testFiltering()
{
    if (!haveGit) {
        QSKIP("Skipping the test, 'git' not found");
    }

    KateMainWindow *mw = app->activeKateMainWindow();
    FileHistory::showFileHistory(testFile, mw->wrapper());
    // wait for git log to finish
    QVERIFY(waitForGitLogFinish());

    auto toolview = getToolview();
    QVERIFY(toolview);

    auto filtersList = toolview->findChild<QListWidget *>();
    auto filterLineEdit = toolview->findChild<QLineEdit *>();
    auto commitProxyModel = toolview->findChild<QSortFilterProxyModel *>();

    QVERIFY(filterLineEdit->text().isEmpty());
    QVERIFY(filtersList->count() == 0);

    QTRY_VERIFY(commitProxyModel->rowCount() != 0);
    const int preFilterRowCount = commitProxyModel->rowCount();

    // Add a filter
    filterLineEdit->setText(QStringLiteral("a:waqar"));
    filterLineEdit->returnPressed();
    // Expect line edit is emptied after return press
    QVERIFY(filterLineEdit->text().isEmpty());

    // Expect one filter button
    QCOMPARE(filtersList->count(), 1);
    // Expect different rowCount
    QVERIFY(commitProxyModel->rowCount() != preFilterRowCount);

    // Add the same filter, expect no change
    filterLineEdit->setText(QStringLiteral("a:waqar"));
    filterLineEdit->returnPressed();
    QCOMPARE(filtersList->count(), 1);

    // Click close button on a filter
    auto filterCloseBtn = filtersList->findChild<QToolButton *>();
    QVERIFY(filterCloseBtn);
    filterCloseBtn->click();
    // Expect filter list to be empty and hidden
    QCOMPARE(filtersList->count(), 0);
    QCOMPARE(filtersList->isVisible(), false);
    QVERIFY(commitProxyModel->rowCount() == preFilterRowCount);

    getCloseButton(toolview)->click();
    QTRY_VERIFY(getToolview() == nullptr);
}

void FileHistoryTest::testAuthorAndDateFiltering()
{
    if (!haveGit) {
        QSKIP("Skipping the test, 'git' not found");
    }

    KateMainWindow *mw = app->activeKateMainWindow();
    FileHistory::showFileHistory(testFile, mw->wrapper());
    // wait for git log to finish
    QVERIFY(waitForGitLogFinish());

    auto toolview = getToolview();
    QVERIFY(toolview);

    auto filtersList = toolview->findChild<QListWidget *>();
    auto filterLineEdit = toolview->findChild<QLineEdit *>();
    auto commitProxyModel = toolview->findChild<QSortFilterProxyModel *>();

    QVERIFY(filterLineEdit->text().isEmpty());
    QVERIFY(filtersList->count() == 0);

    const int preFilterRowCount = commitProxyModel->rowCount();

    // Add a filter
    filterLineEdit->setText(QStringLiteral("a:waqar"));
    filterLineEdit->returnPressed();
    // Expect line edit is emptied after return press
    QVERIFY(filterLineEdit->text().isEmpty());

    // Expect different rowCount
    QVERIFY(commitProxyModel->rowCount() != preFilterRowCount);
    const int rowCountAfterAuthorFilter = commitProxyModel->rowCount();

    // Add since filter
    filterLineEdit->setText(QStringLiteral("since:2025-01-01"));
    filterLineEdit->returnPressed();
    QCOMPARE(filtersList->count(), 2);

    // Expect different rowCount after applying since filter
    QVERIFY(commitProxyModel->rowCount() != rowCountAfterAuthorFilter);

    // close the toolview
    getCloseButton(toolview)->click();
    QTRY_VERIFY(getToolview() == nullptr);
}

// git log output arrives in chunks while git is still running. Records (commit + file name)
// and UTF-8 characters split across chunks must still be parsed correctly: every commit
// shows up exactly once with the right author.
void FileHistoryTest::testLargeHistoryParsedCompletely()
{
    if (!haveGit) {
        QSKIP("Skipping the test, 'git' not found");
    }

    QTemporaryDir repo;
    QVERIFY(repo.isValid());
    auto git = [&repo](const QStringList &args, const QByteArray &input = {}) {
        QProcess p;
        if (!setupGitProcess(p, repo.path(), args)) {
            return false;
        }
        startHostProcess(p, QProcess::ReadWrite);
        if (!input.isEmpty()) {
            p.write(input);
        }
        p.closeWriteChannel();
        return p.waitForFinished(60000) && p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0;
    };
    QVERIFY(git({QStringLiteral("init"), QStringLiteral("-q")}));

    // many commits with non-ASCII authors, some with very long messages: git writes a commit
    // with one write() if it fits into the pipe atomically (PIPE_BUF, 4K on Linux), larger
    // ones are split and arrive in several chunks
    constexpr int commitCount = 3000;
    QSet<QString> expectedTooltips;
    QByteArray stream;
    for (int i = 0; i < commitCount; ++i) {
        const QByteArray name = "Jürgen Äpfelbäum " + QByteArray::number(i);
        const QByteArray email = "user" + QByteArray::number(i) + "@example.com";
        const QByteArray body = (i % 7 == 0) ? QByteArray(20000, 'y') : QByteArray(150, 'x');
        const QByteArray msg = "Ändere Datei " + QByteArray::number(i) + " " + body + " ✓\n";
        const QByteArray content = "line " + QByteArray::number(i) + "\n";
        const QByteArray ident = name + " <" + email + "> " + QByteArray::number(1700000000 + i) + " +0000\n";
        stream += "commit refs/heads/master\nauthor " + ident + "committer " + ident;
        stream += "data " + QByteArray::number(msg.size()) + "\n" + msg;
        stream += "M 644 inline file.txt\ndata " + QByteArray::number(content.size()) + "\n" + content + "\n";
        expectedTooltips.insert(QString::fromUtf8(name) + QStringLiteral("<br>") + QString::fromUtf8(email));
    }
    QVERIFY(git({QStringLiteral("fast-import"), QStringLiteral("--quiet")}, stream));
    QVERIFY(git({QStringLiteral("checkout"), QStringLiteral("-q"), QStringLiteral("master")}));

    const QString file = repo.filePath(QStringLiteral("file.txt"));
    KateMainWindow *mw = app->activeKateMainWindow();
    FileHistory::showFileHistory(file, mw->wrapper());

    auto toolview = Utils::toolviewForName(mw->wrapper(), QStringLiteral("git_file_history_%1").arg(file));
    QVERIFY(toolview);
    auto fileHistoryWidget = toolview->findChild<QWidget *>(QStringLiteral("FileHistoryWidget"));
    QVERIFY(fileHistoryWidget);
    QSignalSpy spy(fileHistoryWidget, SIGNAL(gitLogDone()));
    QVERIFY(spy.count() == 1 || spy.wait(60000));

    auto commitModel = toolview->findChild<QAbstractListModel *>();
    QVERIFY(commitModel);
    QCOMPARE(commitModel->rowCount(), commitCount);

    QSet<QString> tooltips;
    for (int row = 0; row < commitModel->rowCount(); ++row) {
        tooltips.insert(commitModel->index(row).data(Qt::ToolTipRole).toString());
    }
    QCOMPARE(tooltips, expectedTooltips);

    auto closeBtn = getCloseButton(toolview);
    QVERIFY(closeBtn);
    closeBtn->click();
}

QTEST_MAIN(FileHistoryTest)

#include "file_history_tests.moc"
