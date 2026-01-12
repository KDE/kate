/*
    SPDX-FileCopyrightText: 2019 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "sessions_action_test.h"
#include "kateapp.h"
#include "katesessionmanager.h"
#include "katesessionsaction.h"

#include <QActionGroup>
#include <QCommandLineParser>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTestWidgets>

#include <KLocalizedString>

QTEST_MAIN(KateSessionsActionTest)

KateSessionsActionTest::KateSessionsActionTest()
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
    m_ac = new KateSessionsAction(QIcon::fromTheme(QStringLiteral("application-menu")), QStringLiteral("menu"), this, m_manager, false);
}

KateSessionsActionTest::~KateSessionsActionTest()
{
    delete m_ac;
    delete m_app;
    delete m_tempdir;
}

void KateSessionsActionTest::basic()
{
    m_ac->slotAboutToShow();
    QCOMPARE(m_ac->isEnabled(), false);

    QList<QAction *> actions = m_ac->sessionsGroup->actions();
    QCOMPARE(actions.size(), 0);
}

void KateSessionsActionTest::deleteActiveSession()
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

void KateSessionsActionTest::deleteSession()
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

void KateSessionsActionTest::saveActiveSessionWithAnynomous()
{
    QVERIFY(m_manager->activateAnonymousSession());
    QVERIFY(m_manager->activeSession()->isAnonymous());
    QVERIFY(m_manager->sessionList().empty());

    QCOMPARE(m_manager->saveActiveSession(), true);
    QCOMPARE(m_manager->activeSession()->isAnonymous(), true);
    QCOMPARE(m_manager->activeSession()->name(), QString());
    QCOMPARE(m_manager->sessionList().size(), 0);
}

void KateSessionsActionTest::deletingSessionFilesUnderRunningApp()
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

void KateSessionsActionTest::startNonEmpty()
{
    m_manager->activateSession(QStringLiteral("foo"));
    m_manager->activateSession(QStringLiteral("bar"));

    KateSessionManager m(this, m_tempdir->path());
    QCOMPARE(m.sessionList().size(), 2);
}

void KateSessionsActionTest::limit()
{
    for (int i = 0; i < 14; i++) {
        m_manager->copySession(m_manager->activeSession(), QStringLiteral("session %1").arg(i));
    }

    QVERIFY(m_manager->sessionList().size() > 10);
    QCOMPARE(m_ac->isEnabled(), true);

    m_ac->slotAboutToShow();

    QList<QAction *> actions = m_ac->sessionsGroup->actions();
    QCOMPARE(actions.size(), 10);
}

#include "moc_sessions_action_test.cpp"
