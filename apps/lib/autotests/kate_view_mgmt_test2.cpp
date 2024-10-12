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
    void testViewCursorPositionIsRestored();

private:
    std::unique_ptr<QTemporaryDir> m_tempdir;
    std::unique_ptr<KateApp> app;
};

KateViewManagementTest2::KateViewManagementTest2(QObject *)
{
    // ensure ui file can be found and the translation domain is set to avoid warnings
    qApp->setApplicationName(QStringLiteral("kate"));
    KLocalizedString::setApplicationDomain(QByteArrayLiteral("kate"));

    m_tempdir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_tempdir->isValid());

    // ensure we use some dummy config
    KConfig::setMainConfigName(m_tempdir->path() + QStringLiteral("/testconfigfilerc"));

    // create KWrite variant to avoid plugin loading!
    static QCommandLineParser parser;
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

QTEST_MAIN(KateViewManagementTest2)

#include "kate_view_mgmt_test2.moc"
