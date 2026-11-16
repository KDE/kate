/*
    SPDX-FileCopyrightText: 2026 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: MIT
*/

#pragma once

#include <QByteArray>
#include <QString>

namespace ACP
{

/**
 * @brief Describe the difference between two texts as a unified diff
 * @param path Absolute path of the file that was modified
 * @param oldText Content before the modification
 * @param newText Content after the modification
 * @param isNewFile true if the file did not exist before
 * @param contextLines Unchanged lines kept around each change
 * @return A unified diff, empty if the two texts are equal
 *
 * Agents report a modification as the text before and after it, while the
 * tools that display a modification expect a unified diff, so one is computed
 * here.
 *
 * The result is accepted by Kate's diff widget, which recognizes a file by the
 * "--- a/" and "+++ b/" lines and a change by the "@@" line, and needs no
 * "diff --git" header.
 */
QByteArray createUnifiedDiff(const QString &path, const QString &oldText, const QString &newText, bool isNewFile, int contextLines = 3);

}
