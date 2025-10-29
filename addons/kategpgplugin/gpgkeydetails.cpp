/*
    SPDX-FileCopyrightText: 2025 Dennis Lübke <kde@dennis2society.de>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <QDateTime>
#include <gpgkeydetails.hpp>

GPGKeyDetails::GPGKeyDetails()
{
}

GPGKeyDetails::~GPGKeyDetails()
{
    m_uids.clear();
    m_mailAddresses.clear();
    m_subkeyIDs.clear();
}

QString GPGKeyDetails::fingerPrint() const
{
    return m_fingerPrint;
}

QString GPGKeyDetails::keyID() const
{
    return m_keyID;
}

QString GPGKeyDetails::keyType() const
{
    return m_keyType;
}

QString GPGKeyDetails::keyLength() const
{
    return m_keyLength;
}

QString GPGKeyDetails::creationDate() const
{
    return m_creationDate;
}

QString GPGKeyDetails::expiryDate() const
{
    return m_expiryDate;
}

const QVector<QString> &GPGKeyDetails::uids() const
{
    return m_uids;
}

const QVector<QString> &GPGKeyDetails::mailAdresses() const
{
    return m_mailAddresses;
}

const QVector<QString> &GPGKeyDetails::subkeyIDs() const
{
    return m_subkeyIDs;
}

size_t GPGKeyDetails::getNumUIds() const
{
    return m_uids.size();
}

const QString timestampToQString(const time_t timestamp_)
{
    QDateTime dt;
    dt.setSecsSinceEpoch(timestamp_);
    return dt.toString(QStringLiteral("yyyy-MM-dd"));
}

void GPGKeyDetails::loadFromGPGMeKey(GpgME::Key key_)
{
    m_fingerPrint = QString::fromUtf8(key_.primaryFingerprint());
    m_keyID = QString::fromUtf8(key_.shortKeyID());
    m_keyType = QString::fromStdString(key_.subkey(0).algoName());
    m_keyLength = QString::number(key_.subkey(0).length());
    m_creationDate = QString(timestampToQString(key_.subkey(0).creationTime()));
    m_expiryDate = QString(timestampToQString(key_.subkey(0).expirationTime()));
    const std::vector<GpgME::UserID> &ids = key_.userIDs();
    for (auto &id : ids) {
        m_uids.push_back(QString::fromUtf8(id.name()));
        m_mailAddresses.push_back(QString::fromUtf8(id.email()));
        m_subkeyIDs.push_back(QString::fromUtf8(key_.subkey(1).keyID()));
    }
}
