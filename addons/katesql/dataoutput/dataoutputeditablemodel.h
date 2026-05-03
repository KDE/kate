/*
SPDX-FileCopyrightText: 2026 Artyom Kirnev <kirnevartem30@gmail.com>

SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include "dataoutputmodelbase.h"

#include <QSqlTableModel>

/// Editable model for a single database table with styling support.
/// All shared logic (data, clear, refresh, config, locale) is inherited
/// from DataOutputModelBase<QSqlTableModel>.
class DataOutputEditableModel : public DataOutputModelBase<QSqlTableModel>
{
    Q_OBJECT
    Q_INTERFACES(DataOutputModelInterface)
public:
    explicit DataOutputEditableModel(QObject *parent = nullptr, QSqlDatabase db = QSqlDatabase())
        : DataOutputModelBase<QSqlTableModel>(parent, db)
    {
    }

    ~DataOutputEditableModel() override = default;
};
