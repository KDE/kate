/*
   SPDX-FileCopyrightText: 2010 Marco Mentasti <marcomentasti@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#include "dataoutputmodel.h"
#include "dataoutputstylehelper.h"

#include <QVariant>

DataOutputModel::DataOutputModel(QObject *parent)
    : CachedSqlQueryModel(parent, 1000)
    , m_styleHelper(nullptr)
{
}

DataOutputModel::~DataOutputModel() = default;


void DataOutputModel::clear()
{
    beginResetModel();

    CachedSqlQueryModel::clear();

    endResetModel();
}

void DataOutputModel::refresh()
{
    CachedSqlQueryModel::refresh();
    Q_EMIT CachedSqlQueryModel::dataChanged(CachedSqlQueryModel::index(0, 0),
                                            CachedSqlQueryModel::index(CachedSqlQueryModel::rowCount() - 1, CachedSqlQueryModel::columnCount() - 1));
}

void DataOutputModel::readConfig()
{
    // Style helper is managed externally by DataOutputWidget
    // Just emit dataChanged to refresh the view with new styles
    Q_EMIT CachedSqlQueryModel::dataChanged(CachedSqlQueryModel::index(0, 0),
                                            CachedSqlQueryModel::index(CachedSqlQueryModel::rowCount() - 1, CachedSqlQueryModel::columnCount() - 1));
}

bool DataOutputModel::useSystemLocale() const
{
    return m_styleHelper ? m_styleHelper->useSystemLocale() : false;
}

void DataOutputModel::setUseSystemLocale(bool useSystemLocale)
{
    if (m_styleHelper) {
        m_styleHelper->setUseSystemLocale(useSystemLocale);
    }

    Q_EMIT CachedSqlQueryModel::dataChanged(CachedSqlQueryModel::index(0, 0),
                                            CachedSqlQueryModel::index(CachedSqlQueryModel::rowCount() - 1, CachedSqlQueryModel::columnCount() - 1));
}

QVariant DataOutputModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::EditRole) {
        return CachedSqlQueryModel::data(index, role);
    }

    QVariant value(CachedSqlQueryModel::data(index, Qt::DisplayRole));

    if (m_styleHelper) {
        QVariant styled = m_styleHelper->styleData(value, role);
        if (styled.isValid()) {
            return styled;
        }
    }

    return CachedSqlQueryModel::data(index, role);
}


void DataOutputModel::setStyleHelper(DataOutputStyleHelper *helper)
{
    m_styleHelper = helper;
}
