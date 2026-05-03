/*
SPDX-FileCopyrightText: 2026 Artyom Kirnev <kirnevartem30@gmail.com>

SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include "dataoutputmodelbase.h"

#include <QSqlRelationalTableModel>

/// Editable relational model with FK-aware header labels.
/// All shared logic (data, clear, refresh, config, locale) is inherited
/// from DataOutputModelBase<QSqlRelationalTableModel>.
class DataOutputEditableRelationalModel : public DataOutputModelBase<QSqlRelationalTableModel>
{
    Q_OBJECT
    Q_INTERFACES(DataOutputModelInterface)
public:
    explicit DataOutputEditableRelationalModel(QObject *parent = nullptr, QSqlDatabase db = QSqlDatabase())
        : DataOutputModelBase<QSqlRelationalTableModel>(parent, db)
    {
    }

    ~DataOutputEditableRelationalModel() override = default;

    /// Shows "tablename.displaycolumn" for FK columns instead of bare
    /// display column names, avoiding ambiguous headers like "name" / "name_2"
    /// when multiple FK columns resolve to the same display column name.
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
            const QSqlRelation rel = relation(section);
            if (rel.isValid()) {
                // postgres returns schema-qualified table names
                const auto tableNameWithSchema = rel.tableName();
                const auto tableNameWithoutSchema = tableNameWithSchema.contains(u'.') ? tableNameWithSchema.section(u'.', 1) : tableNameWithSchema;

                return QStringLiteral("%1.%2").arg(tableNameWithoutSchema, rel.displayColumn());
            }
        }
        return QSqlRelationalTableModel::headerData(section, orientation, role);
    }
};
