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

#ifndef KATE_SESSION_MANAGER_TEST_H
#define KATE_SESSION_MANAGER_TEST_H

#include <QObject>

class KateSessionManagerTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void cleanup();
    void initTestCase();
    void cleanupTestCase();

    void basic();
    void activateNewNamedSession();
    void anonymousSessionFile();
    void urlizeSessionFile();
    void renameSession();
    void deleteActiveSession();
    void deleteSession();
    void saveActiveSessionWithAnynomous();

    void deletingSessionFilesUnderRunningApp();
    void startNonEmpty();

private:
    class QTemporaryDir *m_tempdir;
    class KateSessionManager *m_manager;
    class KateApp *m_app; // dependency, sigh...
};

#endif

