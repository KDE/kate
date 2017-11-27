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

#ifndef __KATE_SESSION_MANAGER_H__
#define __KATE_SESSION_MANAGER_H__

#include "katesession.h"

#include <QObject>
#include <QHash>

typedef QList<KateSession::Ptr> KateSessionList;

class KATE_TESTS_EXPORT KateSessionManager : public QObject
{
    Q_OBJECT

public:
    KateSessionManager(QObject *parent = nullptr, const QString &sessionsDir = QString());
    ~KateSessionManager() override;

    /**
     * allow access to the session list
     * kept up to date by watching the dir
     */
    KateSessionList sessionList();

    /**
     * activate session by @name
     * first, it will look if a session with this name exists in list
     * if yes, it will use this session, else it will create a new session file
     * @param name name of the session to activate
     * @param closeAndSaveLast try to close and save last session or not?
     * @param loadNew load new session stuff?
     * @return false==session has been delegated, true==session has been activated in this distance
     */
    bool activateSession(const QString &name, const bool closeAndSaveLast = true, const bool loadNew = true);

    /**
     * activates new/anonymous session
     */
    bool activateAnonymousSession();

    /**
     * save current session
     * @param rememberAsLast remember this session as last used?
     * @return success
     */
    bool saveActiveSession(bool rememberAsLast = false);

    /**
     * return the current active session
     * sessionFile == empty means we have no session around for this instance of kate
     * @return session active atm
     */
    inline KateSession::Ptr activeSession() {
        return m_activeSession;
    }

    /**
     * session dir
     * @return global session dir
     */
    inline const QString &sessionsDir() const {
        return m_sessionsDir;
    }

    /**
     * initial session chooser, on app start
     * @return success, if false, app should exit
     */
    bool chooseSession();

public Q_SLOTS:
    /**
     * try to start a new session
     * asks user first for name
     */
    void sessionNew();

    /**
     * try to open a existing session
     */
    void sessionOpen();

    /**
     * try to save current session
     */
    void sessionSave();

    /**
     * try to save as current session
     */
    void sessionSaveAs();

    /**
     * show dialog to manage our sessions
     */
    void sessionManage();

Q_SIGNALS:
    /**
     * Emitted, whenever the session changes, e.g. when it was renamed.
     */
    void sessionChanged();

    /**
     * module internal APIs
     */
public:
    /**
     * return session with given name
     * if no existing session matches, create new one with this name
     * @param name session name
     */
    KateSession::Ptr giveSession(const QString &name);

    /**
     * deletes session file and removes the session from sessions list
     */
    void deleteSession(KateSession::Ptr session);

    /**
     * renames the session to @newName
     * @return true if succesfull
     */
    bool renameSession(KateSession::Ptr session, const QString &newName);

    /**
     * activate a session
     * first, it will look if a session with this name exists in list
     * if yes, it will use this session, else it will create a new session file
     * @param session session to activate
     * @param closeAndSaveLast try to close and save last session or not?
     * @param loadNew load new session stuff?
     * @return false==session has been delegated, true==session has been activated in this distance
     */
    bool activateSession(KateSession::Ptr session, const bool closeAndSaveLast = true, const bool loadNew = true);

private Q_SLOTS:
    /**
     * trigger update of session list
     */
    void updateSessionList();

private:
    /**
     * Asks the user for a new session name. Used by save as for example.
     */
    bool newSessionName();

    /**
     * returns session config file according to policy
     */
    QString sessionFileForName(const QString &name) const;

    /**
     * returns session file for anonymous session
     */
    QString anonymousSessionFile() const;

    /**
      * helper function to save the session to a given config object
      */
    void saveSessionTo(KConfig *sc) const;

    /**
     * restore sessions documents, windows, etc...
     */
    void loadSession(const KateSession::Ptr &session) const;

    /**
     * Writes sessions as jump list actions to the kate.desktop file
     */
    void updateJumpListActions(const QStringList &sessionList);

private:
    /**
     * absolute path to dir in home dir where to store the sessions
     */
    QString m_sessionsDir;

    /**
     * list of current available sessions
     */
    QHash<QString, KateSession::Ptr> m_sessions;

    /**
     * current active session
     */
    KateSession::Ptr m_activeSession;

    class KDirWatch *m_dirWatch;
};

#endif

