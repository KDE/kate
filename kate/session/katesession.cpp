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

#include "katesession.h"

#include "katesessionmanager.h"
#include "katedebug.h"

#include <KConfig>
#include <KConfigGroup>

#include <QCollator>
#include <QFile>
#include <QFileInfo>

static const QLatin1String opGroupName("Open Documents");
static const QLatin1String keyCount("Count");

KateSession::KateSession(const QString &file, const QString &name, const bool anonymous, const KConfig *_config)
    : m_name(name)
    , m_file(file)
    , m_anonymous(anonymous)
    , m_documents(0)
    , m_config(nullptr)
    , m_timestamp()
{
    Q_ASSERT(!m_file.isEmpty());

    if (_config) { // copy data from config instead
        m_config = _config->copyTo(m_file);
    } else if (!QFile::exists(m_file)) { // given file exists, use it to load some stuff
        qCDebug(LOG_KATE) << "Warning, session file not found: " << m_file;
        return;
    }

    m_timestamp = QFileInfo(m_file).lastModified();

    // get the document count
    m_documents = config()->group(opGroupName).readEntry(keyCount, 0);
}

KateSession::~KateSession()
{
    delete m_config;
}

const QString &KateSession::file() const
{
    return m_file;
}

void KateSession::setDocuments(const unsigned int number)
{
    config()->group(opGroupName).writeEntry(keyCount, number);
    m_documents = number;
}

void KateSession::setFile(const QString &filename)
{
    if (m_config) {
        KConfig *cfg = m_config->copyTo(filename);
        delete m_config;
        m_config = cfg;
    }

    m_file = filename;
}

void KateSession::setName(const QString &name)
{
    m_name = name;
}

KConfig *KateSession::config()
{
    if (m_config) {
        return m_config;
    }

    // reread documents number?
    return m_config = new KConfig(m_file, KConfig::SimpleConfig);
}

KateSession::Ptr KateSession::create(const QString &file, const QString &name)
{
    return Ptr(new KateSession(file, name, false));
}

KateSession::Ptr KateSession::createFrom(const KateSession::Ptr &session, const QString &file, const QString &name)
{
    return Ptr(new KateSession(file, name, false, session->config()));
}

KateSession::Ptr KateSession::createAnonymous(const QString &file)
{
    return Ptr(new KateSession(file, QString(), true));
}

KateSession::Ptr KateSession::createAnonymousFrom(const KateSession::Ptr &session, const QString &file)
{
    return Ptr(new KateSession(file, QString(), true, session->config()));
}

bool KateSession::compareByName(const KateSession::Ptr &s1, const KateSession::Ptr &s2)
{
    return QCollator().compare(s1->name(), s2->name()) == -1;
}

bool KateSession::compareByTimeDesc(const KateSession::Ptr &s1, const KateSession::Ptr &s2)
{
    return s1->timestamp() > s2->timestamp();
}

