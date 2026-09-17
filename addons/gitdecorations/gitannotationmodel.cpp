/*
    SPDX-FileCopyrightText: 2026 Leo Ruggeri <leo5t@yahoo.it>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "gitannotationmodel.h"
#include "gitdiff.h"

#include <QBrush>
#include <QColor>
#include <QVariant>

GitAnnotationModel::GitAnnotationModel(QObject *parent)
    : KTextEditor::AnnotationModel()
{
    setParent(parent);
}

void GitAnnotationModel::setDiff(const VcsDiff &diff)
{
    m_changes.clear();
    const auto lines = diff.diff().split(u'\n');

    int addedCount = 0;
    int removedCount = 0;
    int firstAddedLine = -1;
    int lastContextLine = -1;

    for (int i = 0; i < lines.size(); ++i) {
        const auto &line = lines.at(i);

        // TODO: handle conflict markers

        // File header
        if (line.startsWith(u"--- ") || line.startsWith(u"+++ ")) {
            continue;
        }

        // No newline at EOF marker
        if (line.startsWith(u'\\')) {
            continue;
        }

        // Removed line
        if (line.startsWith(u'-')) {
            ++removedCount;
            continue;
        }

        const int targetLine = diff.diffLineToTargetLine(i);

        // Added line
        if (line.startsWith(u'+')) {
            if (targetLine >= 0) {
                ++addedCount;
                m_changes[targetLine] |= ChangeType::Modified;
                if (firstAddedLine < 0) {
                    firstAddedLine = targetLine;
                }
            }

            continue;
        }

        // Context line
        if (removedCount > 0 || addedCount > 0) {
            if (addedCount == 0 && targetLine >= 0) {
                m_changes[targetLine] |= ChangeType::RemovedBefore;
            } else if (addedCount > 0 && removedCount > addedCount) {
                m_changes[firstAddedLine] |= ChangeType::RemovedBefore;
            }

            addedCount = 0;
            removedCount = 0;
            firstAddedLine = -1;
        }

        if (targetLine >= 0) {
            lastContextLine = targetLine;
        }
    }

    // The diff ended with removed lines
    if (removedCount > 0 && addedCount == 0 && lastContextLine >= 0) {
        m_changes[lastContextLine] |= ChangeType::RemovedAfter;
    }

    Q_EMIT reset();
}

QVariant GitAnnotationModel::data(int line, Qt::ItemDataRole role) const
{
    const auto it = m_changes.constFind(line);
    if (it == m_changes.constEnd()) {
        return {};
    }

    if (role == ChangeRole) {
        return static_cast<int>(it.value());
    }

    return {};
}
