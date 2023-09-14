/***************************************************************************
 *   SPDX-FileCopyrightText: 2001-2002 Bernd Gehrmann *
 *   bernd@kdevelop.org                                                    *
 *                                                                         *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 ***************************************************************************/

#pragma once

#include <QString>

class CTagsKinds
{
public:
    static QString findKind(const char *kindChar, const QString &extension);
    static QString findKindNoi18n(const char *kindChar, const QStringView &extension);
};
