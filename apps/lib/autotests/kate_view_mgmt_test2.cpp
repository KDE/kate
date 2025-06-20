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

QTEST_MAIN(KateViewManagementTest2)

#include "kate_view_mgmt_test2.moc"
