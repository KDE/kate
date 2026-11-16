/*
    SPDX-FileCopyrightText: 2026 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: MIT
*/

#include "acpunifieddiff.h"

#include <QList>
#include <QStringList>

namespace
{

/**
 * @brief Largest table the longest common subsequence is computed with
 *
 * The table grows with the product of the line counts, so a modification of
 * two large files would need a lot of memory for a result nobody reads. Such a
 * modification is reported as a replacement of the whole file instead.
 */
constexpr qint64 maxTableSize = 4 * 1000 * 1000;

/** @brief One line of the result, as it appears in a unified diff */
struct DiffLine {
    QChar marker; ///< ' ' for an unchanged line, '-' for a removed one, '+' for an added one
    QString text; ///< The line itself, without its marker
};

/**
 * @brief Split a text into lines
 *
 * The newline at the end of a text terminates the last line rather than
 * starting an empty one, which would otherwise be reported as a change.
 */
QStringList splitLines(const QString &text)
{
    if (text.isEmpty()) {
        return {};
    }

    QStringList lines = text.split(QLatin1Char('\n'));
    if (!lines.isEmpty() && lines.constLast().isEmpty()) {
        lines.removeLast();
    }

    // Texts that use carriage returns would otherwise differ in every line
    for (QString &line : lines) {
        if (line.endsWith(QLatin1Char('\r'))) {
            line.chop(1);
        }
    }

    return lines;
}

/**
 * @brief Describe how to turn one list of lines into another
 *
 * The lines they share at the beginning and at the end are kept out of the
 * comparison, so that a small change in a large file costs little.
 */
QList<DiffLine> compareLines(const QStringList &oldLines, const QStringList &newLines)
{
    QList<DiffLine> result;

    int prefix = 0;
    while (prefix < oldLines.size() && prefix < newLines.size() && oldLines.at(prefix) == newLines.at(prefix)) {
        ++prefix;
    }

    int suffix = 0;
    while (suffix < oldLines.size() - prefix && suffix < newLines.size() - prefix
           && oldLines.at(oldLines.size() - 1 - suffix) == newLines.at(newLines.size() - 1 - suffix)) {
        ++suffix;
    }

    for (int i = 0; i < prefix; ++i) {
        result.append({QLatin1Char(' '), oldLines.at(i)});
    }

    const int oldCount = oldLines.size() - prefix - suffix;
    const int newCount = newLines.size() - prefix - suffix;

    if (static_cast<qint64>(oldCount) * static_cast<qint64>(newCount) > maxTableSize) {
        // Too large to compare line by line, report it as a replacement
        for (int i = 0; i < oldCount; ++i) {
            result.append({QLatin1Char('-'), oldLines.at(prefix + i)});
        }
        for (int i = 0; i < newCount; ++i) {
            result.append({QLatin1Char('+'), newLines.at(prefix + i)});
        }
    } else if (oldCount > 0 || newCount > 0) {
        // Length of the longest common subsequence of the lines from i and j
        // on, which says which of the two ways forward keeps more lines
        QList<int> common((oldCount + 1) * (newCount + 1), 0);
        const auto at = [newCount](int i, int j) {
            return i * (newCount + 1) + j;
        };

        for (int i = oldCount - 1; i >= 0; --i) {
            for (int j = newCount - 1; j >= 0; --j) {
                if (oldLines.at(prefix + i) == newLines.at(prefix + j)) {
                    common[at(i, j)] = common.at(at(i + 1, j + 1)) + 1;
                } else {
                    common[at(i, j)] = qMax(common.at(at(i + 1, j)), common.at(at(i, j + 1)));
                }
            }
        }

        int i = 0;
        int j = 0;
        while (i < oldCount || j < newCount) {
            if (i < oldCount && j < newCount && oldLines.at(prefix + i) == newLines.at(prefix + j)) {
                result.append({QLatin1Char(' '), oldLines.at(prefix + i)});
                ++i;
                ++j;
            } else if (i < oldCount && (j == newCount || common.at(at(i + 1, j)) >= common.at(at(i, j + 1)))) {
                // A replaced line is reported as its removal followed by its
                // addition, which is the order a unified diff is read in
                result.append({QLatin1Char('-'), oldLines.at(prefix + i)});
                ++i;
            } else {
                result.append({QLatin1Char('+'), newLines.at(prefix + j)});
                ++j;
            }
        }
    }

    for (int i = 0; i < suffix; ++i) {
        result.append({QLatin1Char(' '), oldLines.at(oldLines.size() - suffix + i)});
    }

    return result;
}

/** @brief Format the position of a hunk, the way a unified diff states it */
QString formatRange(int start, int count)
{
    // A hunk that adds to an empty side points at the line it follows
    if (count == 0) {
        return QStringLiteral("%1,0").arg(start - 1);
    }

    // A single line states no count, the way a unified diff is usually written
    if (count == 1) {
        return QString::number(start);
    }

    return QStringLiteral("%1,%2").arg(start).arg(count);
}

}

namespace ACP
{

QByteArray createUnifiedDiff(const QString &path, const QString &oldText, const QString &newText, bool isNewFile, int contextLines)
{
    if (oldText == newText) {
        return {};
    }

    contextLines = qMax(0, contextLines);

    const QList<DiffLine> lines = compareLines(splitLines(oldText), splitLines(newText));

    // The line each entry is at, so that the hunks can state their position
    QList<int> oldNumber(lines.size(), 0);
    QList<int> newNumber(lines.size(), 0);
    int oldLine = 1;
    int newLine = 1;
    bool changed = false;
    for (int i = 0; i < lines.size(); ++i) {
        oldNumber[i] = oldLine;
        newNumber[i] = newLine;

        if (lines.at(i).marker != QLatin1Char('+')) {
            ++oldLine;
        }
        if (lines.at(i).marker != QLatin1Char('-')) {
            ++newLine;
        }
        if (lines.at(i).marker != QLatin1Char(' ')) {
            changed = true;
        }
    }

    if (!changed) {
        return {};
    }

    // A path is absolute, while the a/ and b/ prefixes of a unified diff are
    // followed by a relative one, so the separator is reused as the one of the
    // prefix instead of being repeated
    QString name = path;
    while (name.startsWith(QLatin1Char('/'))) {
        name.remove(0, 1);
    }

    QStringList out;
    out.append(isNewFile ? QStringLiteral("--- /dev/null") : QStringLiteral("--- a/%1").arg(name));
    out.append(QStringLiteral("+++ b/%1").arg(name));

    int index = 0;
    while (index < lines.size()) {
        if (lines.at(index).marker == QLatin1Char(' ')) {
            ++index;
            continue;
        }

        // Everything from here that is close enough to the next change to
        // share its hunk, which keeps nearby changes together
        int begin = qMax(0, index - contextLines);
        int end = index;
        while (end < lines.size()) {
            if (lines.at(end).marker != QLatin1Char(' ')) {
                end = end + 1;
                continue;
            }

            // A run of unchanged lines ends the hunk once it is longer than
            // the context of this change and of the one that may follow
            int unchanged = 0;
            while (end + unchanged < lines.size() && lines.at(end + unchanged).marker == QLatin1Char(' ')) {
                ++unchanged;
            }

            if (unchanged > 2 * contextLines || end + unchanged >= lines.size()) {
                end += qMin(unchanged, contextLines);
                break;
            }

            end += unchanged;
        }
        end = qMin(end, lines.size());

        int oldCount = 0;
        int newCount = 0;
        for (int i = begin; i < end; ++i) {
            if (lines.at(i).marker != QLatin1Char('+')) {
                ++oldCount;
            }
            if (lines.at(i).marker != QLatin1Char('-')) {
                ++newCount;
            }
        }

        out.append(QStringLiteral("@@ -%1 +%2 @@").arg(formatRange(oldNumber.at(begin), oldCount), formatRange(newNumber.at(begin), newCount)));

        for (int i = begin; i < end; ++i) {
            out.append(lines.at(i).marker + lines.at(i).text);
        }

        index = end;
    }

    out.append(QString());

    return out.join(QLatin1Char('\n')).toUtf8();
}

}
