/*
SPDX-FileCopyrightText: 2010 Artyom Kirnev <kirnevartem30@gmail.com>

SPDX-License-Identifier: LGPL-2.0-only
*/

#include "dataoutputeditablemodel.h"
#include "dataoutputstylehelper.h"

#include <QVariant>

DataOutputEditableModel::DataOutputEditableModel(QObject *parent, QSqlDatabase db)
    : QSqlTableModel(parent, db)
    , m_styleHelper(nullptr)
{
}

DataOutputEditableModel::~DataOutputEditableModel() = default;

void DataOutputEditableModel::clear()
{
    QSqlTableModel::clear();
}

void DataOutputEditableModel::refresh()
{
    // Reload data from the database
    select();
}

void DataOutputEditableModel::readConfig()
{
    // Style helper is managed externally by DataOutputWidget
    // Just emit dataChanged to refresh the view with new styles
    Q_EMIT QSqlTableModel::dataChanged(QSqlTableModel::index(0, 0), QSqlTableModel::index(QSqlTableModel::rowCount() - 1, QSqlTableModel::columnCount() - 1));
}

bool DataOutputEditableModel::useSystemLocale() const
{
    return m_styleHelper ? m_styleHelper->useSystemLocale() : false;
}

void DataOutputEditableModel::setUseSystemLocale(bool useSystemLocale)
{
    if (m_styleHelper) {
        m_styleHelper->setUseSystemLocale(useSystemLocale);
    }

    Q_EMIT QSqlTableModel::dataChanged(QSqlTableModel::index(0, 0), QSqlTableModel::index(QSqlTableModel::rowCount() - 1, QSqlTableModel::columnCount() - 1));
}

QVariant DataOutputEditableModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::EditRole) {
        return QSqlTableModel::data(index, role);
    }

    QVariant value(QSqlTableModel::data(index, Qt::DisplayRole));

    if (m_styleHelper) {
        QVariant styled = m_styleHelper->styleData(value, role, QSqlTableModel::isDirty(index));
        if (styled.isValid()) {
            return styled;
        }
    }

    return QSqlTableModel::data(index, role);
}

void DataOutputEditableModel::setStyleHelper(DataOutputStyleHelper *helper)
{
    m_styleHelper = helper;
}
