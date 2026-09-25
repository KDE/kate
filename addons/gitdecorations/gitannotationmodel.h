/*
    SPDX-FileCopyrightText: 2026 Leo Ruggeri <leo5t@yahoo.it>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <KTextEditor/AnnotationInterface>

#include <QHash>

class VcsDiff;

class GitAnnotationModel : public KTextEditor::AnnotationModel
{
    Q_OBJECT

public:
    enum class ChangeType {
        None = 0,
        Modified = 1 << 0,
        RemovedAfter = 1 << 1,
        RemovedBefore = 1 << 2
    };

    Q_DECLARE_FLAGS(ChangeTypes, ChangeType)

    static constexpr Qt::ItemDataRole ChangeRole = static_cast<Qt::ItemDataRole>(Qt::UserRole + 1);

    explicit GitAnnotationModel(QObject *parent);
    void setDiff(const VcsDiff &diff);
    QVariant data(int line, Qt::ItemDataRole role) const override;

private:
    QHash<int, ChangeTypes> m_changes;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(GitAnnotationModel::ChangeTypes)
