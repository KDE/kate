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
#include <QTemporaryDir>
#include <QtTestWidgets>

QTEST_MAIN(KateSessionsActionTest)

void KateSessionsActionTest::init()
{
    m_tempdir = new QTemporaryDir;
    QVERIFY(m_tempdir->isValid());

    // we need an application object, as session loading will trigger modifications to that
    static QCommandLineParser parser;
    m_app = new KateApp(parser, KateApp::ApplicationKate, m_tempdir->path());
    m_app->sessionManager()->activateAnonymousSession();

    m_manager = new KateSessionManager(this, m_tempdir->path());
    m_ac = new KateSessionsAction(QStringLiteral("menu"), this, m_manager, false);
}

void KateSessionsActionTest::cleanup()
{
    delete m_ac;
    delete m_manager;
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

void KateSessionsActionTest::limit()
{
    for (int i = 0; i < 14; i++) {
        m_manager->activateSession(QStringLiteral("session %1").arg(i));
    }

    QCOMPARE(m_manager->activeSession()->isAnonymous(), false);
    QCOMPARE(m_manager->sessionList().size(), 14);
    QCOMPARE(m_ac->isEnabled(), true);

    m_ac->slotAboutToShow();

    QList<QAction *> actions = m_ac->sessionsGroup->actions();
    QCOMPARE(actions.size(), 10);
}

void KateSessionsActionTest::timesorted()
{
    /*
    m_manager->activateSession("one", false, false);
    m_manager->saveActiveSession();
    m_manager->activateSession("two", false, false);
    m_manager->saveActiveSession();
    m_manager->activateSession("three", false, false);
    m_manager->saveActiveSession();

    KateSessionList list = m_manager->sessionList();
    QCOMPARE(list.size(), 3);

    TODO: any idea how to test this simply and not calling utime()?
    */
}
