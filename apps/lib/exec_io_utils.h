/*
    SPDX-FileCopyrightText: 2025 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>

    SPDX-License-Identifier: MIT
*/

#pragma once

#include "kateprivate_export.h"

#include <QString>

#include <memory>

namespace Utils
{
class KATE_PRIVATE_EXPORT ExecPrefixManager
{
public:
    static ExecPrefixManager &instance();

    static QString scheme()
    {
        return QStringLiteral("kateexec");
    }

    ExecPrefixManager();
    ~ExecPrefixManager();

    void update(QString name, QStringList args);

    static QStringList load(QString name);

private:
    class ExecPrefixManagerPrivate;
    ExecPrefixManagerPrivate *const d;
};

} // Utils
