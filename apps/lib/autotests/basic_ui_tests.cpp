#include "kateapp.h"
#include "katemainwindow.h"

#include <KActionCollection>
#include <KFontRequester>
#include <KLocalizedString>
#include <QCommandLineParser>
#include <QDialog>
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
    app = std::make_unique<KateApp>(parser, KateApp::ApplicationKWrite, m_tempdir->path());
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
    QAction *act = mainWindow->actionCollection()->findChild<QAction *>("options_configure");
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

QTEST_MAIN(BasicUiTests)

#include "basic_ui_tests.moc"
