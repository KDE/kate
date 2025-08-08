/*
    SPDX-FileCopyrightText: 2024 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "kateapp.h"

#include "filehistorywidget.h"
#include "gitprocess.h"
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

private:
    QWidget *getToolview()
    {
        KateMainWindow *mw = app->activeKateMainWindow();
        const QString toolViewIdentifier = QStringLiteral("git_file_history_%1").arg(testFile);
        // Expect toolview
        return Utils::toolviewForName(mw->wrapper(), toolViewIdentifier);
    }

    QToolButton *getCloseButton(QWidget *toolview)
    {
        const auto childs = toolview->findChildren<QToolButton *>();
        for (QToolButton *child : childs) {
            if (child->icon().name() == QStringLiteral("tab-close")) {
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

    QDir thisDir(QString::fromLatin1(QT_TESTCASE_SOURCEDIR));
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

    auto toolview = getToolview();
    QVERIFY(toolview);

    auto filtersList = toolview->findChild<QListWidget *>();
    auto filterLineEdit = toolview->findChild<QLineEdit *>();
    auto commitProxyModel = toolview->findChild<QSortFilterProxyModel *>();

    QVERIFY(filterLineEdit->text().isEmpty());
    QVERIFY(filtersList->count() == 0);

    QTRY_VERIFY(commitProxyModel->rowCount() != 0);
    const int preFilterRowCount = commitProxyModel->rowCount();

    filterLineEdit->setText(QStringLiteral("a:waqar"));
    filterLineEdit->returnPressed();

    QCOMPARE(filtersList->count(), 1);
    QVERIFY(commitProxyModel->rowCount() != preFilterRowCount);
}

QTEST_MAIN(FileHistoryTest)

#include "file_history_tests.moc"
