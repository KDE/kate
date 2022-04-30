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

KateSessionsActionTest::KateSessionsActionTest()
{
    m_tempdir = new QTemporaryDir;
    QVERIFY(m_tempdir->isValid());

    // we need an application object, as session loading will trigger modifications to that
    static QCommandLineParser parser;
    m_app = new KateApp(parser, KateApp::ApplicationKate, m_tempdir->path());
    m_app->sessionManager()->activateAnonymousSession();
    m_manager = m_app->sessionManager();
    m_ac = new KateSessionsAction(QStringLiteral("menu"), this, m_manager, false);
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

void KateSessionsActionTest::limit()
{
    for (int i = 0; i < 14; i++) {
        m_manager->copySession(m_manager->activeSession(), QStringLiteral("session %1").arg(i));
    }

    QCOMPARE(m_manager->sessionList().size(), 14);
    QCOMPARE(m_ac->isEnabled(), true);

    m_ac->slotAboutToShow();

    QList<QAction *> actions = m_ac->sessionsGroup->actions();
    QCOMPARE(actions.size(), 10);
}
