#include "hostprocess.h"
#include "kateapp.h"
#include "katemainwindow.h"

#include <KActionCollection>
#include <KFontRequester>
#include <KLocalizedString>
#include <QCommandLineParser>
#include <QDialog>
#include <QDir>
#include <QLineEdit>
#include <QPushButton>
#include <QTest>

class BasicUiTests : public QObject
{
    Q_OBJECT
public:
    BasicUiTests();

private Q_SLOTS:
    void test_windowOpenClose();
    void test_openFontDialog();
    void test_settingPATH();

private:
    std::unique_ptr<QTemporaryDir> m_tempdir;
    std::unique_ptr<KateApp> app;
};

BasicUiTests::BasicUiTests()
{
    // ensure ui file can be found and the translation domain is set to avoid warnings
    qApp->setApplicationName(QStringLiteral("kate"));
    KLocalizedString::setApplicationDomain(QByteArrayLiteral("kate"));

    m_tempdir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_tempdir->isValid());

    // ensure we use some dummy config
    KConfig::setMainConfigName(m_tempdir->path() + QStringLiteral("/testconfigfilerc"));

    static QCommandLineParser parser;
    app.reset(); // needed as KateApp mimics a singleton
    app = std::make_unique<KateApp>(parser, KateApp::ApplicationKate, m_tempdir->path());
}

void BasicUiTests::test_windowOpenClose()
{
    app->sessionManager()->sessionNew();
    QCOMPARE(app->mainWindows().size(), 1); // expect a mainwindow to be there

    auto mainWindow = KateApp::newMainWindow();
    QVERIFY(mainWindow->wrapper()); // expect a valid KTextEditor::MainWindow
    mainWindow->show();
    delete mainWindow;

    QCOMPARE(app->mainWindows().size(), 1);
}

void BasicUiTests::test_openFontDialog()
{
    // This test tests that "Configure Kate..." works
    // and that clicking KFontRequester in Appearance tab
    // opens a dialog

    app->sessionManager()->sessionNew();
    QCOMPARE(app->mainWindows().size(), 1); // expect a mainwindow to be there

    auto mainWindow = app->activeKateMainWindow();
    QAction *act = mainWindow->actionCollection()->findChild<QAction *>(QStringLiteral("options_configure"));
    QVERIFY(act);

    auto dismissNextDialog = [this]() {
        QMetaObject::invokeMethod(
            this,
            [] {
                auto *w = qobject_cast<QDialog *>(qApp->activeModalWidget());
                QVERIFY(w);
                w->reject();
            },
            Qt::QueuedConnection);
    };

    auto dialogTest = [dismissNextDialog] {
        // expect a KateConfigDialog to be there
        auto w = qApp->activeModalWidget();
        QVERIFY(w);
        // expect to find a font requester
        auto fontRequester = w->findChild<KFontRequester *>();
        QVERIFY(fontRequester);
        // click the button
        dismissNextDialog();
        fontRequester->button()->click();

        // dismiss KateConfigDialog
        dismissNextDialog();
    };
    QMetaObject::invokeMethod(this, dialogTest, Qt::QueuedConnection);
    act->trigger();
}

void BasicUiTests::test_settingPATH()
{
    app->sessionManager()->sessionNew();
    QCOMPARE(app->mainWindows().size(), 1); // expect a mainwindow to be there

    auto mainWindow = app->activeKateMainWindow();

    auto acceptDialog = []() {
        auto *w = qobject_cast<QDialog *>(qApp->activeModalWidget());
        QVERIFY(w);
        w->accept();
    };

    // 2. Set the path in dialog
    auto dialogTest1 = [acceptDialog] {
        QTRY_VERIFY(qApp->activeModalWidget());
        auto w = qApp->activeModalWidget();
        QVERIFY(w);
        auto pathEdit = w->findChild<QLineEdit *>(QStringLiteral("katePATHedit"));
        QVERIFY(pathEdit);
        pathEdit->setText(qApp->applicationDirPath());

        // accept KateConfigDialog
        acceptDialog();
        QTRY_VERIFY(!qApp->activeModalWidget());
    };

    // 1. open config dialog
    QMetaObject::invokeMethod(this, dialogTest1, Qt::QueuedConnection);
    mainWindow->showPluginConfigPage(nullptr, 0);

    const QString first = qEnvironmentVariable("PATH").split(QDir::listSeparator()).constFirst();
    QCOMPARE(first, qApp->applicationDirPath());

    // verify that we are now able to find the test exe because its dir is in PATH
    QVERIFY(!safeExecutableName(QStringLiteral("basic_ui_tests")).isEmpty());

    // 3. Remove the path, ensure its properly gone
    auto dialogTest2 = [acceptDialog] {
        QTRY_VERIFY(qApp->activeModalWidget());
        auto w = qApp->activeModalWidget();
        QVERIFY(w);
        auto pathEdit = w->findChild<QLineEdit *>(QStringLiteral("katePATHedit"));
        QVERIFY(pathEdit);
        pathEdit->setText(QString());

        // dismiss KateConfigDialog
        acceptDialog();
        QTRY_VERIFY(!qApp->activeModalWidget());
    };
    QMetaObject::invokeMethod(this, dialogTest2, Qt::QueuedConnection);
    mainWindow->showPluginConfigPage(nullptr, 0);

    QVERIFY(qEnvironmentVariable("PATH").isEmpty() || qEnvironmentVariable("PATH").split(QDir::listSeparator()).constFirst() != qApp->applicationDirPath());
}

QTEST_MAIN(BasicUiTests)

#include "basic_ui_tests.moc"
