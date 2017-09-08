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

#ifndef __KATE_SESSION_H__
#define __KATE_SESSION_H__

#include <QExplicitlySharedDataPointer>
#include <QString>
#include <QDateTime>
#include "kateprivate_export.h"

class KConfig;

class KATE_TESTS_EXPORT KateSession : public QSharedData
{
public:
    /**
     * Define a Shared-Pointer type
     */
    typedef QExplicitlySharedDataPointer<KateSession> Ptr;

public:
    ~KateSession();

    /**
     * session name
     * @return name for this session
     */
    const QString &name() const {
        return m_name;
    }

    /**
     * session config
     * on first access, will create the config object, delete will be done automagic
     * return 0 if we have no file to read config from atm
     * @return correct KConfig, neverl null
     * @note never delete configRead(), because the return value might be
     *       KSharedConfig::openConfig(). Only delete the member variables directly.
     */
    KConfig *config();

    /**
     * count of documents in this session
     * @return documents count
     */
    unsigned int documents() const {
        return m_documents;
    }

    /**
     * update @number of openned documents in session
     */
    void setDocuments(const unsigned int number);

    /**
     * @return true if this is anonymouse/new session
     */
    bool isAnonymous() const {
        return m_anonymous;
    }

    /**
     * @return path to session file
     */
    const QString &file() const;

    /**
     * returns last save time of this session
     */
    const QDateTime &timestamp() const {
        return m_timestamp;
    }

    /**
     * Factories
     */
public:
    static KateSession::Ptr create(const QString &file, const QString &name);
    static KateSession::Ptr createFrom(const KateSession::Ptr &session, const QString &file, const QString &name);
    static KateSession::Ptr createAnonymous(const QString &file);
    static KateSession::Ptr createAnonymousFrom(const KateSession::Ptr &session, const QString &file);

    static bool compareByName(const KateSession::Ptr &s1, const KateSession::Ptr &s2);
    static bool compareByTimeDesc(const KateSession::Ptr &s1, const KateSession::Ptr &s2);

private:
    friend class KateSessionManager;
    friend class KateSessionTest;
    /**
     * set session name
     */
    void setName(const QString &name);

    /**
     * set's new session file to @filename
     */
    void setFile(const QString &filename);

    /**
     * create a session from given @file
     * @param file configuration file
     * @param name name of this session
     * @param anonymous anonymous flag
     * @param config if specified, the session will copy configuration from the KConfig instead of opening the file
     */
    KateSession(const QString &file, const QString &name, const bool anonymous, const KConfig *config = nullptr);

private:
    QString m_name;
    QString m_file;
    bool m_anonymous;
    unsigned int m_documents;
    KConfig *m_config;
    QDateTime m_timestamp;
};

#endif

