/*
SPDX-FileCopyrightText: 2026 Artyom Kirnev <kirnevartem30@gmail.com>

SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include "../dataoutputmodelbase.h"

#include <QSqlRelationalTableModel>

/// Editable relational model with FK-aware header labels.
/// All shared logic (data, clear, refresh, config, locale) is inherited
/// from DataOutputModelBase<QSqlRelationalTableModel>.
class DataOutputEditableRelationalModel : public DataOutputModelBase<QSqlRelationalTableModel>
{
    Q_OBJECT
    Q_INTERFACES(DataOutputModelInterface)
public:
    explicit DataOutputEditableRelationalModel(QObject *parent = nullptr, QSqlDatabase db = QSqlDatabase());
    ~DataOutputEditableRelationalModel() override;

    /// When pasting data on a relational column, resolves a display string
    /// (e.g. "John") to the corresponding FK ID by searching the relation
    /// model.  Falls through to the base class if no match is found (e.g.
    /// the value is already a valid FK ID).
    bool setData(const QModelIndex &item, const QVariant &value, int role = Qt::EditRole) override;

    /// Shows "tablename.displaycolumn" for FK columns instead of bare
    /// display column names, avoiding ambiguous headers like "name" / "name_2"
    /// when multiple FK columns resolve to the same display column name.
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
};
