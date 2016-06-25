/* This file is part of the KDE project
 *
 *  Copyright (C) 2005 Christoph Cullmann <cullmann@kde.org>
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

#include "katesessionsaction.h"

#include "kateapp.h"
#include "katesessionmanager.h"

#include <algorithm>
#include <QMenu>

KateSessionsAction::KateSessionsAction(const QString &text, QObject *parent, KateSessionManager *manager)
    : KActionMenu(text, parent)
{
    m_manager = manager ? manager : KateApp::self()->sessionManager();

    connect(menu(), SIGNAL(aboutToShow()), this, SLOT(slotAboutToShow()));

    sessionsGroup = new QActionGroup(menu());

    // reason for Qt::QueuedConnection: when switching session with N mainwindows
    // to e.g. 1 mainwindow, the last N - 1 mainwindows are deleted. Invoking
    // a session switch without queued connection deletes a mainwindow in which
    // the current code path is executed ---> crash. See bug #227008.
    connect(sessionsGroup, SIGNAL(triggered(QAction*)), this, SLOT(openSession(QAction*)), Qt::QueuedConnection);

    connect(m_manager, SIGNAL(sessionChanged()), this, SLOT(slotSessionChanged()));

    setDisabled(m_manager->sessionList().size() == 0);
}

void KateSessionsAction::slotAboutToShow()
{
    qDeleteAll(sessionsGroup->actions());

    KateSessionList slist = m_manager->sessionList();
    std::sort(slist.begin(), slist.end(), KateSession::compareByTimeDesc);

    slist = slist.mid(0, 10); // take first 10

    // sort the reduced list alphabetically (#364089)
    std::sort(slist.begin(), slist.end(), KateSession::compareByName);

    foreach(const KateSession::Ptr & session, slist) {
        QString sessionName = session->name();
        sessionName.replace(QStringLiteral("&"), QStringLiteral("&&"));
        QAction *action = new QAction(sessionName, sessionsGroup);
        action->setData(QVariant(session->name()));
        menu()->addAction(action);
    }
}

void KateSessionsAction::openSession(QAction *action)
{
    const QString name = action->data().toString();
    m_manager->activateSession(name);
}

void KateSessionsAction::slotSessionChanged()
{
    setDisabled(m_manager->sessionList().size() == 0);
}

