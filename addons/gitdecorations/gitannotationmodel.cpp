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

    QVector<int> addedLines;
    int removedCount = 0;
    int previousTargetLine = -1;

    auto processBlock = [&](const int targetLineAfterBlock) {
        if (removedCount == 0 && addedLines.isEmpty()) {
            return;
        }

        if (addedLines.isEmpty()) {
            if (targetLineAfterBlock >= 0) {
                m_changes[targetLineAfterBlock] |= ChangeType::RemovedBefore;
            } else if (previousTargetLine >= 0) {
                m_changes[previousTargetLine] |= ChangeType::RemovedAfter;
            }
        } else {
            const int changedCount = qMin(removedCount, addedLines.size());
            for (int i = 0; i < changedCount; ++i) {
                m_changes[addedLines.at(i)] |= ChangeType::Changed;
            }

            if (removedCount > addedLines.size() && changedCount > 0) {
                m_changes[addedLines.first()] |= ChangeType::RemovedBefore;
            }

            for (int i = changedCount; i < addedLines.size(); ++i) {
                m_changes[addedLines.at(i)] |= ChangeType::Added;
            }
        }

        removedCount = 0;
        addedLines.clear();
    };

    for (int i = 0; i < lines.size(); ++i) {
        const auto &line = lines.at(i);

        // TODO: handle conflict markers

        if (line.startsWith(u"--- ") || line.startsWith(u"+++ ")) {
            continue;
        }

        if (line.startsWith(u'\\')) {
            continue;
        }

        if (line.startsWith(u'-')) {
            ++removedCount;
            continue;
        }

        if (line.startsWith(u'+')) {
            const int targetLine = diff.diffLineToTargetLine(i);
            if (targetLine >= 0) {
                addedLines.append(targetLine);
            }

            continue;
        }

        const int targetLine = diff.diffLineToTargetLine(i);
        processBlock(targetLine);

        if (targetLine >= 0) {
            previousTargetLine = targetLine;
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