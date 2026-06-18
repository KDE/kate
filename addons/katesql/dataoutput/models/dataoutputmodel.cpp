/*
   SPDX-FileCopyrightText: 2010 Marco Mentasti <marcomentasti@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#include "dataoutputmodel.h"

DataOutputModel::DataOutputModel(QObject *parent)
    : DataOutputModelBase<CachedSqlQueryModel>(parent)
{
}

DataOutputModel::~DataOutputModel() = default;

void DataOutputModel::refresh()
{
    DataOutputModelBase::refresh();
    Q_EMIT DataOutputModelBase::dataChanged(DataOutputModelBase::index(0, 0),
                                            DataOutputModelBase::index(DataOutputModelBase::rowCount() - 1, DataOutputModelBase::columnCount() - 1));
}

#include "moc_dataoutputmodel.cpp"
