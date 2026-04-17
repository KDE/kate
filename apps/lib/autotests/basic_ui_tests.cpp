#include "hostprocess.h"
#include "kateapp.h"
#include "katemainwindow.h"

#include <KActionCollection>
#include <KFontRequester>
#include <KLocalizedString>
#include <KSharedConfig>
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
    void test_windowGeometrySaveRestore();
    void test_sessionGeometrySaveRestore();
    void test_openFontDialog();
    void test_settingPATH();

private:
    std::unique_ptr<QTemporaryDir> m_tempdir;
    std::unique_ptr<KateApp> app;
};

BasicUiTests::BasicUiTests()
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

void BasicUiTests::test_windowGeometrySaveRestore()
{
    // Ensure we start clean
    KConfigGroup cfg(KSharedConfig::openConfig(), QStringLiteral("MainWindow"));
    cfg.deleteGroup();
    cfg.sync();

    // Disable temp dir auto-removal and print out the path so it can be inspected
    m_tempdir->setAutoRemove(false);
    qDebug() << "\n\n>>> FAKE KATERC FILE CREATED AT:" << m_tempdir->path() << "/testconfigfilerc <<<\n\n";

    auto mainWindow = KateApp::newMainWindow();
    QVERIFY(mainWindow->wrapper());

    // Use sizeHint() as the base so we respect the 60% ratio and minimum sizes, preventing Wayland or Qt from overriding our bounds
    QSize targetSize = mainWindow->sizeHint().expandedTo(QSize(850, 550));
    mainWindow->resize(targetSize);
    mainWindow->move(10, 10);
    mainWindow->show();

    // Grab the actual QWindow size that Wayland/X11 settled on
    QSize savedWindowSize = mainWindow->windowHandle() ? mainWindow->windowHandle()->size() : mainWindow->size();
    qDebug() << ">>> Size BEFORE destruction (QWidget):" << mainWindow->size() << " QWindow:" << savedWindowSize;

    // Deleting the window saves to the "MainWindow" config group natively via destructor
    delete mainWindow;

    // Reparse config
    cfg.config()->reparseConfiguration();

    // We do not check exact positioning/values here because under Wayland
    // it's a no-op, but we verify the keys at least were touched or could
    // be populated in valid (e.g., X11) environments without failing on Wayland.
    const auto keys = cfg.keyList();

    bool hasWidth = false;
    bool hasHeight = false;
    bool hasX = false;
    bool hasY = false;

    for (const auto &key : keys) {
        if (key.endsWith(QLatin1String("Width")))
            hasWidth = true;
        if (key.endsWith(QLatin1String("Height")))
            hasHeight = true;
        if (key.endsWith(QLatin1String("XPosition")))
            hasX = true;
        if (key.endsWith(QLatin1String("YPosition")))
            hasY = true;
    }

    QVERIFY(hasWidth);
    QVERIFY(hasHeight);

    if (QGuiApplication::platformName() != QLatin1String("wayland")) {
        QVERIFY(hasX);
        QVERIFY(hasY);
    }

    // Now test the restoration by opening a new window
    auto restoredWindow = KateApp::newMainWindow();
    restoredWindow->show();

    QSize restoredWindowSize = restoredWindow->windowHandle() ? restoredWindow->windowHandle()->size() : restoredWindow->size();
    qDebug() << ">>> Size AFTER restoration (QWidget):" << restoredWindow->size() << " QWindow:" << restoredWindowSize;

    // The restored QWindow size should match the size it was saved at,
    // assuming the compositor respects the initial bounds.
    QCOMPARE(restoredWindowSize, savedWindowSize);

    delete restoredWindow;
}

void BasicUiTests::test_sessionGeometrySaveRestore()
{
    const QString sessionName = QStringLiteral("TestSessionGeometryRestore");

    // Activate a fresh named session with no prior windows
    // Use loadNew=true so that loadSession() is called and a window is created
    app->sessionManager()->activateSession(sessionName, false, true);
    QCOMPARE(app->mainWindows().size(), 1);

    auto mainWindow = app->activeKateMainWindow();
    QVERIFY(mainWindow->wrapper());

    // Use sizeHint() as the base so we respect the 60% ratio and minimum sizes
    QSize targetSize = mainWindow->sizeHint().expandedTo(QSize(900, 650));
    mainWindow->resize(targetSize);
    mainWindow->move(20, 20);
    mainWindow->show();

    // Visual delay so you can see it open at the desired size
    QTest::qWait(5000);

    // Grab the actual QWindow size that Wayland/X11 settled on
    QSize savedWindowSize = mainWindow->windowHandle() ? mainWindow->windowHandle()->size() : mainWindow->size();
    qDebug() << ">>> Session Size BEFORE save:" << mainWindow->size() << " QWindow:" << savedWindowSize;

    // Save the active session (writes MainWindow0 group with size to the session file)
    app->sessionManager()->saveActiveSession(true);

    // Switch to anonymous session first to change the identity of the active session,
    // so the subsequent re-activation of sessionName is not short-circuited
    // (activateSession returns early if the same session is already active)
    app->sessionManager()->activateAnonymousSession();

    // Now re-activate the named session WITH loadNew=true to trigger loadSession()
    // which calls newMainWindow(cfg, "MainWindow0") using the saved config
    app->sessionManager()->activateSession(sessionName, false, true);

    QCOMPARE(app->mainWindows().size(), 1);
    auto restoredWindow = app->activeKateMainWindow();
    QVERIFY(restoredWindow);
    restoredWindow->show();

    // Wait again so the user can see the restored window
    QTest::qWait(5000);

    QSize restoredWindowSize = restoredWindow->windowHandle() ? restoredWindow->windowHandle()->size() : restoredWindow->size();
    qDebug() << ">>> Session Size AFTER restoration:" << restoredWindow->size() << " QWindow:" << restoredWindowSize;

    // The restored QWindow size should match what was saved in the session
    QCOMPARE(restoredWindowSize, savedWindowSize);

    // Cleanup: switch back to anonymous and delete the test session
    auto s = app->sessionManager()->activeSession();
    app->sessionManager()->activateAnonymousSession();
    app->sessionManager()->deleteSession(s);
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
    QVERIFY(!safePrefixedExecutableNameInContainerIfAvailable(QStringLiteral("basic_ui_tests")).isEmpty());

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
