/*
    SPDX-FileCopyrightText: 2019 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "session_manager_test.h"
#include "kateapp.h"
#include "katesessionmanager.h"

#include <KConfig>
#include <KLocalizedString>

#include <QCommandLineParser>
#include <QTemporaryDir>
#include <QtTestWidgets>

QTEST_MAIN(KateSessionManagerTest)

KateSessionManagerTest::KateSessionManagerTest()
{
    // ensure we are in test mode, for the part, too
    QStandardPaths::setTestModeEnabled(true);

    // ensure ui file can be found and the translation domain is set to avoid warnings
    qApp->setApplicationName(QStringLiteral("kate"));
    KLocalizedString::setApplicationDomain(QByteArrayLiteral("kate"));

    m_tempdir = new QTemporaryDir;
    QVERIFY(m_tempdir->isValid());

    // ensure we use some dummy config
    KConfig::setMainConfigName(m_tempdir->path() + QStringLiteral("/testconfigfilerc"));

    // create KWrite variant to avoid plugin loading!
    static QCommandLineParser parser;
    m_app = new KateApp(parser, KateApp::ApplicationKWrite, m_tempdir->path());
    m_app->sessionManager()->activateAnonymousSession();
    m_manager = m_app->sessionManager();
}

KateSessionManagerTest::~KateSessionManagerTest()
{
    delete m_app;
    delete m_tempdir;
}

void KateSessionManagerTest::basic()
{
    QCOMPARE(m_manager->sessionsDir(), m_tempdir->path());
    QCOMPARE(m_manager->sessionList().size(), 0);
    QVERIFY(m_manager->activateAnonymousSession());
    QVERIFY(m_manager->activeSession());
}

void KateSessionManagerTest::activateNewNamedSession()
{
    const QString sessionName = QStringLiteral("hello_world");

    QVERIFY(m_manager->activateSession(sessionName, false, false));
    QCOMPARE(m_manager->sessionList().size(), 1);

    KateSession::Ptr s = m_manager->activeSession();
    QCOMPARE(s->name(), sessionName);
    QCOMPARE(s->isAnonymous(), false);

    const QString sessionFile = m_tempdir->path() + QLatin1Char('/') + sessionName + QLatin1String(".katesession");
    QCOMPARE(s->config()->name(), sessionFile);

    // cleanup again for next test
    QVERIFY(m_manager->activateAnonymousSession());
    QVERIFY(m_manager->deleteSession(s));
    QCOMPARE(m_manager->sessionList().size(), 0);
}

void KateSessionManagerTest::anonymousSessionFile()
{
    const QString anonfile = QDir().cleanPath(m_tempdir->path() + QLatin1String("/../anonymous.katesession"));
    QVERIFY(m_manager->activateAnonymousSession());
    QVERIFY(m_manager->activeSession()->isAnonymous());
    QCOMPARE(m_manager->activeSession()->config()->name(), anonfile);
}

void KateSessionManagerTest::urlizeSessionFile()
{
    const QString sessionName = QStringLiteral("hello world/#");

    m_manager->activateSession(sessionName, false, false);
    KateSession::Ptr s = m_manager->activeSession();

    const QString sessionFile = m_tempdir->path() + QLatin1String("/hello%20world%2F%23.katesession");
    QCOMPARE(s->config()->name(), sessionFile);

    // cleanup again for next test
    QVERIFY(m_manager->activateAnonymousSession());
    QVERIFY(m_manager->deleteSession(s));
    QCOMPARE(m_manager->sessionList().size(), 0);
}

void KateSessionManagerTest::renameSession()
{
    m_manager->activateSession(QStringLiteral("foo"));
    KateSession::Ptr s = m_manager->activeSession();

    QCOMPARE(m_manager->sessionList().size(), 1);

    const QString newName = QStringLiteral("bar");
    m_manager->renameSession(s, newName); // non-collision path
    QCOMPARE(s->name(), newName);
    QCOMPARE(m_manager->sessionList().size(), 1);
    QCOMPARE(m_manager->sessionList().constFirst(), s);

    // cleanup again for next test
    QVERIFY(m_manager->activateAnonymousSession());
    QVERIFY(m_manager->deleteSession(s));
    QCOMPARE(m_manager->sessionList().size(), 0);
}

#include "moc_session_manager_test.cpp"
