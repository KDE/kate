/* This file is part of the KDE project
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "sessions_action_test.h"
#include "katesessionsaction.h"
#include "katesessionmanager.h"
#include "kateapp.h"

#include <QtTestWidgets>
#include <QTemporaryDir>
#include <QActionGroup>
#include <QCommandLineParser>

QTEST_MAIN(KateSessionsActionTest)

void KateSessionsActionTest::initTestCase()
{
    m_app = new KateApp(QCommandLineParser()); // FIXME: aaaah, why, why, why?!
}

void KateSessionsActionTest::cleanupTestCase()
{
    delete m_app;
}

void KateSessionsActionTest::init()
{
    m_tempdir = new QTemporaryDir;
    QVERIFY(m_tempdir->isValid());

    m_manager = new KateSessionManager(this, m_tempdir->path());
    m_ac = new KateSessionsAction(QLatin1String("menu"), this, m_manager);
}

void KateSessionsActionTest::cleanup()
{
    delete m_ac;
    delete m_manager;
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
        m_manager->activateSession(QString::fromLatin1("session %1").arg(i));
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


