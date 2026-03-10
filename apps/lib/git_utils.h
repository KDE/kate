/*
 *  SPDX-FileCopyrightText: 2026 Leo Ruggeri <leo5t@yahoo.it>
 *  SPDX-License-Identifier: MIT
 */

#pragma once

#include <QString>

namespace GitUtils
{

/**
 * Convert a stash index to a stash reference.
 */
inline QString stashRefFromIndex(const QString &index)
{
    return QStringLiteral("stash@{%1}").arg(index);
}

/**
 * Check whether a string is a valid stash reference.
 */
inline bool isStashRef(const QString &stashRef)
{
    return stashRef.startsWith(QLatin1String("stash@{"));
}

}