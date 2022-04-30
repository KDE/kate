/*
    SPDX-FileCopyrightText: 2019 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "session_manager_test.h"
#include "kateapp.h"
#include "katesessionmanager.h"

#include <KConfig>
#include <KConfigGroup>

#include <QCommandLineParser>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTestWidgets>

QTEST_MAIN(KateSessionManagerTest)

KateSessionManagerTest::KateSessionManagerTest()
{
    m_tempdir = new QTemporaryDir;
    QVERIFY(m_tempdir->isValid());

    // we need an application object, as session loading will trigger modifications to that
    static QCommandLineParser parser;
    m_app = new KateApp(parser, KateApp::ApplicationKate, m_tempdir->path());
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

void KateSessionManagerTest::deleteSession()
{
    m_manager->activateSession(QStringLiteral("foo"));
    KateSession::Ptr s = m_manager->activeSession();

    m_manager->activateSession(QStringLiteral("bar"));
    KateSession::Ptr s2 = m_manager->activeSession();

    QCOMPARE(m_manager->sessionList().size(), 2);

    m_manager->deleteSession(s);
    QCOMPARE(m_manager->sessionList().size(), 1);

    // cleanup again for next test
    QVERIFY(m_manager->activateAnonymousSession());
    QVERIFY(m_manager->deleteSession(s2));
    QCOMPARE(m_manager->sessionList().size(), 0);
}

void KateSessionManagerTest::deleteActiveSession()
{
    m_manager->activateSession(QStringLiteral("foo"));
    KateSession::Ptr s = m_manager->activeSession();

    QCOMPARE(m_manager->sessionList().size(), 1);
    m_manager->deleteSession(s);
    QCOMPARE(m_manager->sessionList().size(), 1);

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
    QCOMPARE(m_manager->sessionList().first(), s);

    // cleanup again for next test
    QVERIFY(m_manager->activateAnonymousSession());
    QVERIFY(m_manager->deleteSession(s));
    QCOMPARE(m_manager->sessionList().size(), 0);
}

void KateSessionManagerTest::saveActiveSessionWithAnynomous()
{
    QVERIFY(m_manager->activateAnonymousSession());
    QVERIFY(m_manager->activeSession()->isAnonymous());
    QVERIFY(m_manager->sessionList().empty());

    QCOMPARE(m_manager->saveActiveSession(), true);
    QCOMPARE(m_manager->activeSession()->isAnonymous(), true);
    QCOMPARE(m_manager->activeSession()->name(), QString());
    QCOMPARE(m_manager->sessionList().size(), 0);
}

void KateSessionManagerTest::deletingSessionFilesUnderRunningApp()
{
    m_manager->activateSession(QStringLiteral("foo"));
    m_manager->activateSession(QStringLiteral("bar"));

    QVERIFY(m_manager->sessionList().size() == 2);
    QVERIFY(m_manager->activeSession()->name() == QLatin1String("bar"));

    const QString file = m_tempdir->path() + QLatin1String("/foo.katesession");
    QVERIFY(QFile::remove(file));

    // wait for notification about external session list change
    QSignalSpy spy(m_manager, &KateSessionManager::sessionListChanged);
    QVERIFY(spy.wait());

    QCOMPARE(m_manager->activeSession()->name(), QLatin1String("bar"));
}

void KateSessionManagerTest::startNonEmpty()
{
    m_manager->activateSession(QStringLiteral("foo"));
    m_manager->activateSession(QStringLiteral("bar"));

    KateSessionManager m(this, m_tempdir->path());
    QCOMPARE(m.sessionList().size(), 2);
}
