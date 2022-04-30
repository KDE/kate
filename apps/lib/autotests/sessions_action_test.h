/*
    SPDX-FileCopyrightText: 2019 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KATE_SESSIONS_ACTION_TEST_H
#define KATE_SESSIONS_ACTION_TEST_H

#include <QObject>

class KateSessionsActionTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void basic();
    void limit();

    void init();
    void cleanup();

private:
    class QTemporaryDir *m_tempdir;
    class KateSessionManager *m_manager;
    class KateApp *m_app; // dependency, sigh...
    class KateSessionsAction *m_ac;
};

#endif
