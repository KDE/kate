/*
SPDX-FileCopyrightText: 2026 Artyom Kirnev <kirnevartem30@gmail.com>

SPDX-License-Identifier: LGPL-2.0-only
*/

#include "dataoutputeditablemodel.h"

DataOutputEditableModel::DataOutputEditableModel(QObject *parent, QSqlDatabase db)
    : DataOutputModelBase<QSqlTableModel>(parent, db)
{
}

DataOutputEditableModel::~DataOutputEditableModel() = default;

#include "moc_dataoutputeditablemodel.cpp"
