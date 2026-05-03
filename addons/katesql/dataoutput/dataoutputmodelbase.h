/*
SPDX-FileCopyrightText: 2026 Artyom Kirnev <kirnevartem30@gmail.com>

SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include "dataoutputmodelinterface.h"

#include <QSqlTableModel>

#include <type_traits>

/**
 * Template base that consolidates the shared model logic used by all
 * DataOutput model variants:
 *
 *   - DataOutputModel              (CachedSqlQueryModel, read-only)
 *   - DataOutputEditableModel      (QSqlTableModel, editable)
 *   - DataOutputEditableRelationalModel (QSqlRelationalTableModel, editable + FK)
 *
 * Uses compile-time branching (if constexpr) to accommodate differences
 * between the SQL base classes (e.g. isDirty() only exists on QSqlTableModel).
 *
 * Note: Q_OBJECT and Q_INTERFACES must appear only in the concrete
 * derived classes, not in this template.
 *
 * @tparam SqlModel  One of CachedSqlQueryModel, QSqlTableModel,
 *                   or QSqlRelationalTableModel.
 */
template<typename SqlModel>
class DataOutputModelBase : public SqlModel, public DataOutputModelInterface
{
public:
    /**
     * Constructor for all SQL model types.
     * CachedSqlQueryModel(QObject*), QSqlTableModel(QObject*), and
     * QSqlRelationalTableModel(QObject*) all accept this form.
     */
    explicit DataOutputModelBase(QObject *parent = nullptr)
        : SqlModel(parent)
    {
    }

    /**
     * Constructor for table models that accept a database connection.
     * Only enabled when SqlModel inherits from QSqlTableModel.
     */
    template<typename M = SqlModel, std::enable_if_t<std::is_base_of_v<QSqlTableModel, M>, int> = 0>
    DataOutputModelBase(QObject *parent, QSqlDatabase db)
        : SqlModel(parent, db)
    {
    }

    ~DataOutputModelBase() override = default;

    bool useSystemLocale() const override
    {
        return m_styleHelper ? m_styleHelper->useSystemLocale() : false;
    }

    void setUseSystemLocale(bool useSystemLocale) override
    {
        if (m_styleHelper) {
            m_styleHelper->setUseSystemLocale(useSystemLocale);
        }

        Q_EMIT SqlModel::dataChanged(SqlModel::index(0, 0), SqlModel::index(SqlModel::rowCount() - 1, SqlModel::columnCount() - 1));
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (role == Qt::EditRole) {
            return SqlModel::data(index, role);
        }

        QVariant value(SqlModel::data(index, Qt::DisplayRole));

        // isDirty() only exists on QSqlTableModel and subclasses.
        // For read-only query models, dirty is always false.
        bool dirty = false;
        if constexpr (std::is_base_of_v<QSqlTableModel, SqlModel>) {
            dirty = SqlModel::isDirty(index);
        }

        QVariant styled = applyStyle(value, role, dirty);
        if (styled.isValid()) {
            return styled;
        }

        return SqlModel::data(index, role);
    }

    void clear() override
    {
        // QSqlQueryModel::clear() does NOT emit beginResetModel/endResetModel,
        // so for non-table models we wrap the call to notify views properly.
        // QSqlTableModel::clear() handles this internally.
        if constexpr (!std::is_base_of_v<QSqlTableModel, SqlModel>) {
            SqlModel::beginResetModel();
        }

        SqlModel::clear();

        if constexpr (!std::is_base_of_v<QSqlTableModel, SqlModel>) {
            SqlModel::endResetModel();
        }
    }

    void refresh() override
    {
        // Only editable table models have select().  Read-only query models
        // should override this method with their own refresh logic.
        if constexpr (std::is_base_of_v<QSqlTableModel, SqlModel>) {
            SqlModel::select();
        }
    }

    void readConfig() override
    {
        Q_EMIT SqlModel::dataChanged(SqlModel::index(0, 0), SqlModel::index(SqlModel::rowCount() - 1, SqlModel::columnCount() - 1));
    }

    QObject *asQObject() override
    {
        return this;
    }
};
