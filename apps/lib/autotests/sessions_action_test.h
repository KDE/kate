/*
    SPDX-FileCopyrightText: 2019 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QObject>

class KateSessionsActionTest : public QObject
{
    Q_OBJECT

public:
    KateSessionsActionTest();
    ~KateSessionsActionTest() override;

private Q_SLOTS:
    void basic();

    void deleteActiveSession();
    void deleteSession();
    void saveActiveSessionWithAnynomous();
    void deletingSessionFilesUnderRunningApp();
    void startNonEmpty();

    void limit();

private:
    class QTemporaryDir *m_tempdir;
    class KateSessionManager *m_manager;
    class KateApp *m_app; // dependency, sigh...
    class KateSessionsAction *m_ac;
};
