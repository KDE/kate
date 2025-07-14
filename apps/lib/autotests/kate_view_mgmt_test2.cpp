/*
    SPDX-FileCopyrightText: 2024 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "kateapp.h"

#include "kate_view_mgmt_tests.h" // for shared stuff
#include "katemainwindow.h"
#include "kateviewspace.h"
#include "ktexteditor_utils.h"

#include <KLocalizedString>
#include <KSharedConfig>
#include <KTextEditor/Editor>

#include <QCommandLineParser>
#include <QObject>
#include <QPointer>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QTest>
#include <memory>

class KateApp;
class KateViewSpace;
class KateViewManager;

// This class uses Kate as the test KateApp mode
class KateViewManagementTest2 : public QObject
{
    Q_OBJECT

public:
    KateViewManagementTest2(QObject *parent = nullptr);

private Q_SLOTS:
    void init();
    void testViewCursorPositionIsRestored();
    void testTabsKeepOrderOnRestore();
    void testNewlyCreatedUnsavedFilesStashed();
    void testMultipleViewCursorPositionIsRestored();
    void testTabsKeepOrderOnRestore2();

private:
    std::unique_ptr<QTemporaryDir> m_tempdir;
    std::unique_ptr<KateApp> app;
};

KateViewManagementTest2::KateViewManagementTest2(QObject *)
{
    // ensure ui file can be found and the translation domain is set to avoid warnings
    qApp->setApplicationName(QStringLiteral("kate"));
    KLocalizedString::setApplicationDomain(QByteArrayLiteral("kate"));
}

void KateViewManagementTest2::init()
{
    m_tempdir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_tempdir->isValid());

    // ensure we use some dummy config
    KConfig::setMainConfigName(m_tempdir->path() + QStringLiteral("/testconfigfilerc"));

    static QCommandLineParser parser;
    app.reset(); // needed as KateApp mimics a singleton
    app = std::make_unique<KateApp>(parser, KateApp::ApplicationKate, m_tempdir->path());
}

void KateViewManagementTest2::testViewCursorPositionIsRestored()
{
    // Test that cursor is restored to the correct position
    // if a doc is closed and reopened during the same session.

    app->sessionManager()->sessionNew();
    KateMainWindow *mw = app->activeKateMainWindow();
    clearAllDocs(mw);

    QTemporaryFile f1;
    QVERIFY(f1.open());
    const QString text = QStringList(100, QStringLiteral("Hello")).join(QStringLiteral("\n"));
    f1.write(text.toUtf8().data());
    f1.close();

    QVERIFY(app->sessionManager()->activeSession());
    app->sessionManager()->saveActiveSession();

    // open view and set cursor position
    const KTextEditor::Cursor expectedPos(23, 2);
    auto v = mw->openUrl(QUrl::fromLocalFile(f1.fileName()));
    QCOMPARE(v->document()->text(), text);
    v->setCursorPosition(expectedPos);
    mw->viewManager()->slotDocumentClose();

    // open view and expect cursor position to be still correct
    v = mw->openUrl(QUrl::fromLocalFile(f1.fileName()));
    QCOMPARE(v->document()->text(), text);
    QCOMPARE(v->cursorPosition(), expectedPos);
}

void KateViewManagementTest2::testTabsKeepOrderOnRestore()
{
    // test that the tabs keep their order on restore

    // use new test session
    app->sessionManager()->activateSession(QStringLiteral("testTabsKeepOrderOnRestore"), false, true);
    KateMainWindow *mw = app->activeKateMainWindow();

    // open two files aka tabs in order
    auto tab1 = mw->openUrl(QUrl::fromLocalFile(QStringLiteral(":/kxmlgui5/kate/kateui.rc")));
    auto tab2 = mw->openUrl(QUrl::fromLocalFile(QStringLiteral(":/kxmlgui5/kwrite/kateui.rc")));
    const auto file1 = tab1->document()->url();
    const auto file2 = tab2->document()->url();

    // check tab order, we need try compare for delayed initial doc closing
    QTRY_COMPARE(mw->viewManager()->activeViewSpace()->documentList().size(), 2);
    auto tabs = mw->viewManager()->activeViewSpace()->documentList();
    QCOMPARE(tabs.at(0).doc()->url(), file1);
    QCOMPARE(tabs.at(1).doc()->url(), file2);

    // trigger that the LRU order is no longer the tab order
    mw->activateView(tab1->document());

    // save the session
    app->sessionManager()->saveActiveSession();

    // open new empty session
    app->sessionManager()->sessionNew();
    mw = app->activeKateMainWindow();
    QTRY_COMPARE(mw->viewManager()->activeViewSpace()->documentList().size(), 1);

    // back to our session
    app->sessionManager()->activateSession(QStringLiteral("testTabsKeepOrderOnRestore"));
    mw = app->activeKateMainWindow();

    // tabs shall have right order, not the LRU one
    QTRY_COMPARE(mw->viewManager()->activeViewSpace()->documentList().size(), 2);
    tabs = mw->viewManager()->activeViewSpace()->documentList();
    QCOMPARE(tabs.at(0).doc()->url(), file1);
    QCOMPARE(tabs.at(1).doc()->url(), file2);
}

void KateViewManagementTest2::testNewlyCreatedUnsavedFilesStashed()
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig();
    KConfigGroup cgGeneral = KConfigGroup(config, QStringLiteral("General"));
    bool oldValue = cgGeneral.readEntry("Stash new unsaved files", true);
    cgGeneral.writeEntry("Stash new unsaved files", true);

    app->sessionManager()->activateSession(QStringLiteral("testNewlyCreatedUnsavedFilesStashed"), false, true);
    KateMainWindow *mw = app->activeKateMainWindow();

    auto v1 = mw->viewManager()->createView();
    auto v2 = mw->viewManager()->createView();
    v1->document()->setText(QStringLiteral("A\n"));
    v2->document()->setText(QStringLiteral("B\n"));

    QVERIFY(app->sessionManager()->saveActiveSession());

    // open new empty session
    app->sessionManager()->sessionNew();
    mw = app->activeKateMainWindow();
    QTRY_COMPARE(mw->viewManager()->activeViewSpace()->documentList().size(), 1);

    // back to our session
    app->sessionManager()->activateSession(QStringLiteral("testNewlyCreatedUnsavedFilesStashed"));
    mw = app->activeKateMainWindow();

    QTRY_COMPARE(mw->viewManager()->activeViewSpace()->documentList().size(), 2);
    auto docs = mw->viewManager()->activeViewSpace()->documentList();
    QCOMPARE(docs[0].doc()->text(), QStringLiteral("A\n"));
    QCOMPARE(docs[1].doc()->text(), QStringLiteral("B\n"));

    cgGeneral.writeEntry("Stash new unsaved files", oldValue);
}

void KateViewManagementTest2::testMultipleViewCursorPositionIsRestored()
{
    // Open App with 2 docs, set a cursor position
    const QString sessionName = QStringLiteral("testMultipleViewCursorPositionIsRestored");
    {
        app->sessionManager()->activateSession(sessionName, false, true);
        KateMainWindow *mw = app->activeKateMainWindow();

        // open two files aka tabs in order
        auto tab1 = mw->openUrl(QUrl::fromLocalFile(QStringLiteral(":/kxmlgui5/kate/kateui.rc")));
        auto tab2 = mw->openUrl(QUrl::fromLocalFile(QStringLiteral(":/kxmlgui5/kwrite/kateui.rc")));
        const auto file1 = tab1->document()->url();
        const auto file2 = tab2->document()->url();

        auto view1 = mw->activateView(tab1->document());
        auto view2 = mw->activateView(tab2->document());
        QVERIFY(view1);
        QVERIFY(view2);

        view1->setCursorPosition({3, 5});
        view2->setCursorPosition({7, 2});
        QCOMPARE(view1->cursorPosition(), KTextEditor::Cursor(3, 5));
        QCOMPARE(view2->cursorPosition(), KTextEditor::Cursor(7, 2));

        // save the session
        app->sessionManager()->saveActiveSession();
    }

    // Close app
    app.reset();

    // Open app again, expect correct cursor positions
    {
        QCommandLineParser parser;
        // TODO: reuse option from kate/main.cpp properly
        const QCommandLineOption startSessionOption(QStringList() << QStringLiteral("s") << QStringLiteral("start"),
                                                    i18n("Start Kate with a given session."),
                                                    i18n("session"));
        parser.addOption(startSessionOption);
        parser.process({qApp->applicationFilePath(), QStringLiteral("--start=%1").arg(sessionName)});

        app = std::make_unique<KateApp>(parser, KateApp::ApplicationKate, m_tempdir->path());
        QVERIFY(app->init());
        // Expect the right session
        QCOMPARE(app->sessionManager()->activeSession()->name(), sessionName);

        KateMainWindow *mw = app->activeKateMainWindow();

        // Expect 2 docs
        const auto docs = app->documents();
        QCOMPARE(docs.size(), 2);
        QCOMPARE(mw->viewManager()->activeViewSpace()->numberOfRegisteredDocuments(), 2);

        // Only 1 view, second doc hasn't been activated yet
        auto views = mw->viewManager()->views();
        QCOMPARE(views.size(), 1);

        // Expect correct position
        QCOMPARE(views[0]->cursorPosition(), KTextEditor::Cursor(7, 2));

        // simulate some wait
        QTest::qWait(200);

        // activate second document
        mw->activateView(docs[0]);

        // Expect 2 views now
        views = mw->viewManager()->views();
        QCOMPARE(views.size(), 2);
        // Expect correct cursor position
        QCOMPARE(views[0]->cursorPosition(), KTextEditor::Cursor(3, 5));
    }
}

void KateViewManagementTest2::testTabsKeepOrderOnRestore2()
{
    const QString sessionName = QStringLiteral("testTabsKeepOrderOnRestore2");

    KSharedConfig::Ptr config = KSharedConfig::openConfig();
    KConfigGroup cgGeneral = KConfigGroup(config, QStringLiteral("General"));
    const int oldValue = cgGeneral.readEntry("Tabbar Tab Limit", 0);
    cgGeneral.writeEntry("Tabbar Tab Limit", 3);
    auto _ = qScopeGuard([&cgGeneral, oldValue] {
        cgGeneral.writeEntry("Tabbar Tab Limit", oldValue);
    });

    // Tab bar with limit of 3 tabs
    QCOMPARE(cgGeneral.readEntry("Tabbar Tab Limit", 0), 3);

    app->sessionManager()->activateSession(sessionName, false, true);
    KateMainWindow *mw = app->activeKateMainWindow();

    // open 5 files aka tabs in order
    auto tab1 = mw->openUrl(QUrl::fromLocalFile(QStringLiteral(":/kxmlgui5/kate/kateui.rc")));
    auto tab2 = mw->openUrl(QUrl::fromLocalFile(QStringLiteral(":/kxmlgui5/kwrite/kateui.rc")));
    auto tab3 = mw->openUrl(QUrl::fromLocalFile(QStringLiteral(":/kxmlgui5/textfilter/ui.rc")));
    auto tab4 = mw->openUrl(QUrl::fromLocalFile(QStringLiteral(":/kxmlgui5/kategitblameplugin/ui.rc")));
    auto tab5 = mw->openUrl(QUrl::fromLocalFile(QStringLiteral(":/kxmlgui5/katesearch/ui.rc")));
    const QUrl file1 = tab1->document()->url();
    const QUrl file2 = tab2->document()->url();
    const QUrl file3 = tab3->document()->url();
    const QUrl file4 = tab4->document()->url();
    const QUrl file5 = tab5->document()->url();

    // check tab order, we need try compare for delayed initial doc closing
    QTRY_COMPARE(mw->viewManager()->activeViewSpace()->documentList().size(), 3);
    auto tabs = mw->viewManager()->activeViewSpace()->documentList();
    // Tab order is:
    QCOMPARE(tabs.at(0).doc()->url(), file4);
    QCOMPARE(tabs.at(1).doc()->url(), file5);
    QCOMPARE(tabs.at(2).doc()->url(), file3);

    // activate tab1 so it gets a view
    QVERIFY(mw->activateView(tab1->document()));

    // Tab order is:
    tabs = mw->viewManager()->activeViewSpace()->documentList();
    QCOMPARE(tabs.at(0).doc()->url(), file4);
    QCOMPARE(tabs.at(1).doc()->url(), file5);
    QCOMPARE(tabs.at(2).doc()->url(), file1); // got replaced

    // Move tab 0 to tab 1
    auto tabBar = mw->viewManager()->activeViewSpace()->m_tabBar;
    tabBar->moveTab(0, 1);

    // Tab order is:
    tabs = mw->viewManager()->activeViewSpace()->documentList();
    QCOMPARE(tabs.at(0).doc()->url(), file5);
    QCOMPARE(tabs.at(1).doc()->url(), file4);
    QCOMPARE(tabs.at(2).doc()->url(), file1);

    // open new empty session
    app->sessionManager()->sessionNew();
    mw = app->activeKateMainWindow();
    QTRY_COMPARE(mw->viewManager()->activeViewSpace()->documentList().size(), 1);

    // back to our session
    app->sessionManager()->activateSession(sessionName);
    mw = app->activeKateMainWindow();

    // tabs shall have right order, not the LRU one
    QTRY_COMPARE(mw->viewManager()->activeViewSpace()->documentList().size(), 3);
    tabs = mw->viewManager()->activeViewSpace()->documentList();
    QCOMPARE(tabs.at(0).doc()->url(), file5);
    QCOMPARE(tabs.at(1).doc()->url(), file4);
    QCOMPARE(tabs.at(2).doc()->url(), file1);
}

QTEST_MAIN(KateViewManagementTest2)

#include "kate_view_mgmt_test2.moc"
