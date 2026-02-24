/*
    SPDX-FileCopyrightText: 2025 Dennis Lübke <kde@dennis2society.de>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

/**
 * @brief This is a convenience wrapper for GPGMepp/GpgMe++ calls.
 * We want returned datatypes in C++/Qt types+containers.
 */

#include <gpgme++/key.h>

#include "gpgkeydetails.hpp"

#include <QVector>
#include <QVersionNumber>

struct GPGOperationResult {
    QString resultString; // de- or encrypted string depending on operation
    bool keyFound = false;
    bool decryptionSuccess = false;
    QString errorMessage;
    QString keyIDUsedForDecryption;
};

class GPGMeWrapper
{
private:
    // The list of available GPG Keys
    QVector<GPGKeyDetails> m_keys;

    // for convenience reasons we want to know the currently selected key from the
    // UI
    uint m_selectedKeyIndex = 0;

    /**
     * @brief Gets all available GPG keys containing mail addresses
     *        with search pattern.
     * @param searchPattern_ The mail search pattern.
     * @return A list of matching GpgMe::Key
     */
    std::vector<GpgME::Key> listKeys(bool showOnlyPrivateKeys_, const QString &searchPattern_ = QLatin1String(""));

public:
    GPGMeWrapper();

    ~GPGMeWrapper();

    /**
     * @brief Gets the list of available GPG keys.
     * @return The list of available GPG keys.
     */
    const QVector<GPGKeyDetails> &getKeys() const;

    uint getNumKeys() const;

    /**
     * @brief This function reads all available keys and
     *        adds its details to the keys list.
     */
    void loadKeys(bool showOnlyPrivateKeys_, bool hideExpiredKeys_, const QString searchPattern_);

    /**
     * @brief This function attempts to decrypt a given input string
     *        using any of the available private keys. Will fail if the
     *        message was not encrypted to your private key.
     * @param inputString_ The encrypted input string.
     * @param fingerprint_ This is used for planning ahead when attempting
     *                     to decrypt a message encrypted to multiple
     *                     recipients.
     * @return The GPGOerationsResult (see above)
     */
    const GPGOperationResult decryptString(const QString &inputString_, const QString &fingerprint_);

    /**
     * @brief This function attempts to encrypt a given input string
     *        using the currently selected private key. Will fail if
     *        no fingerprint is selected in the ey table.
     * @param inputString_    The input string to be encrypted.
     * @param fingerprint_    The selected fingerprint/key.
     * @param recipientMail_ A recipient mail address matched with the
     *        fingerprint.
     * @param symmetricEncryption_ A bool to enable symmetric encryption
     *        (will ask for a symmetric passphrase that must be remembered).
     *        Default is false.
     * @param showOnlyPrivateKeys_ Will only display keys for which a private
     *        key is available.
     * @return The GPGOerationsResult (see above)
     */
    GPGOperationResult encryptString(const QString &inputString_,
                                     const QString &fingerprint_,
                                     const QString &recipientMail_,
                                     const bool useASCII,
                                     bool symmetricEncryption_ = false,
                                     bool showOnlyPrivateKeys_ = false);

    /**
     * @brief To test if a given QString is GPG encrypted already.
     * @param inputString_ The text to be tested
     * @return true if the inputString_ has sufficient indicators for being
     *         encrypted (by testing file extension and decryption success).
     */
    bool isEncrypted(const QString &inputString_);

    bool isPreferredKey(const GPGKeyDetails d_, const QString &mailAddress_);

    void setSelectedKeyIndex(uint newSelectedKeyIndex);
    uint selectedKeyIndex() const;
};
