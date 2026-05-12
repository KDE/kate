/*
SPDX-FileCopyrightText: 2026 Artyom Kirnev <kirnevartem30@gmail.com>

SPDX-License-Identifier: LGPL-2.0-only
*/

#include "dataoutputeditablerelationalmodel.h"

DataOutputEditableRelationalModel::DataOutputEditableRelationalModel(QObject *parent, QSqlDatabase db)
    : DataOutputModelBase<QSqlRelationalTableModel>(parent, db)
{
}

DataOutputEditableRelationalModel::~DataOutputEditableRelationalModel() = default;

bool DataOutputEditableRelationalModel::setData(const QModelIndex &item, const QVariant &value, int role)
{
    if (role == Qt::EditRole) {
        const QSqlRelation rel = relation(item.column());

        if (rel.isValid()) {
            QSqlTableModel *relModel = relationModel(item.column());
            if (relModel) {
                const int displayIdx = relModel->fieldIndex(rel.displayColumn());
                const int indexIdx = relModel->fieldIndex(rel.indexColumn());

                for (int r = 0; r < relModel->rowCount(); ++r) {
                    if (relModel->data(relModel->index(r, displayIdx)).toString() == value.toString()) {
                        return DataOutputModelBase::setData(item, relModel->data(relModel->index(r, indexIdx)), role);
                    }
                }
            }
        }
    }
    return DataOutputModelBase::setData(item, value, role);
}

QVariant DataOutputEditableRelationalModel::headerData(int section, Qt::Orientation orientation, int role) const
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
