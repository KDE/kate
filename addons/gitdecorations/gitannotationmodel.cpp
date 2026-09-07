/*
    SPDX-FileCopyrightText: 2026 Leo Ruggeri <leo5t@yahoo.it>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "gitannotationmodel.h"
#include "gitdiff.h"

#include <QBrush>
#include <QColor>

GitAnnotationModel::GitAnnotationModel(QObject *parent)
    : KTextEditor::AnnotationModel()
{
    setParent(parent);
}

void GitAnnotationModel::setDiff(const VcsDiff &diff)
{
    m_changes.clear();
    const auto lines = diff.diff().split(u'\n');

    int removedCount = 0;
    int addedCount = 0;
    int firstAddedLine = -1;
    int lastUnchangedLine = -1;

    auto processBlock = [&](const int targetLineAfterBlock) {
        if (removedCount == 0 && addedCount == 0) {
            return;
        }

        if (addedCount == 0) {
            if (targetLineAfterBlock >= 0) {
                m_changes[targetLineAfterBlock] |= ChangeType::RemovedBefore;
            } else if (lastUnchangedLine >= 0) {
                m_changes[lastUnchangedLine] |= ChangeType::RemovedAfter;
            }
        } else if (removedCount > addedCount) {
            m_changes[firstAddedLine] |= ChangeType::RemovedBefore;
        }

        removedCount = 0;
        addedCount = 0;
        firstAddedLine = -1;
    };

    for (int i = 0; i < lines.size(); ++i) {
        const auto &line = lines.at(i);

        // TODO: handle conflict markers

        if (line.startsWith(u"--- ") || line.startsWith(u"+++ ")) {
            continue;
        }

        if (line.startsWith(u'\\')) { // No newline at EOF marker
            continue;
        }

        if (line.startsWith(u'-')) {
            ++removedCount;
            continue;
        }

        if (line.startsWith(u'+')) {
            const int targetLine = diff.diffLineToTargetLine(i);
            if (targetLine >= 0) {
                ++addedCount;
                m_changes[targetLine] |= ChangeType::Modified;
                if (firstAddedLine < 0) {
                    firstAddedLine = targetLine;
                }
            }

            continue;
        }

        const int targetLine = diff.diffLineToTargetLine(i);
        processBlock(targetLine);

        if (targetLine >= 0) {
            lastUnchangedLine = targetLine;
        }
    }

    processBlock(-1);

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