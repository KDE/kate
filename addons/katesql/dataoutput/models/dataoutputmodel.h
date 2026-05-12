/*
   SPDX-FileCopyrightText: 2010 Marco Mentasti <marcomentasti@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include "../cachedsqlquerymodel.h"
#include "../dataoutputmodelbase.h"

/// Read-only model for SQL query results with styling support.
class DataOutputModel : public DataOutputModelBase<CachedSqlQueryModel>
{
    Q_OBJECT
    Q_INTERFACES(DataOutputModelInterface)
public:
    explicit DataOutputModel(QObject *parent = nullptr);
    ~DataOutputModel() override;

    /// Re-executes the query and refreshes the cache.
    void refresh() override;
};
