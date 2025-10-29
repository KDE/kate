/*
    SPDX-FileCopyrightText: 2025 Dennis Lübke <kde@dennis2society.de>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

/**
 * @brief This class contains the details for a GPG key
 **/

#include <gpgme++/key.h>

#include <QString>
#include <QVector>

class GPGKeyDetails
{
public:
    GPGKeyDetails();

    ~GPGKeyDetails();

    QString fingerPrint() const;
    QString keyID() const;
    QString keyType() const;
    QString keyLength() const;
    QString creationDate() const;
    QString expiryDate() const;
    const QVector<QString> &uids() const; // this returns a list of all names per key
    const QVector<QString> &mailAdresses() const; // this returns a list of all email addresses associated with this
                                                  // key
    const QVector<QString> &subkeyIDs() const; // this returns a list of all "IDs" per key

    size_t getNumUIds() const;

    void loadFromGPGMeKey(GpgME::Key key_);

private:
    QString m_fingerPrint;
    QString m_keyID;
    QString m_keyType;
    QString m_keyLength;
    QString m_creationDate;
    QString m_expiryDate;
    QVector<QString> m_uids;
    QVector<QString> m_mailAddresses;
    QVector<QString> m_subkeyIDs;
};
