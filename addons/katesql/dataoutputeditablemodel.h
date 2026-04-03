/*
SPDX-FileCopyrightText: 2010 Artyom Kirnev <kirnevartem30@gmail.com>

SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include "dataoutputmodelinterface.h"

#include <QSqlTableModel>

class DataOutputStyleHelper;

class DataOutputEditableModel : public QSqlTableModel, public DataOutputModelInterface
{
    Q_OBJECT
    // Inform the meta-object system that this class implements the interface
    Q_INTERFACES(DataOutputModelInterface)
public:
    explicit DataOutputEditableModel(QObject *parent = nullptr, QSqlDatabase db = QSqlDatabase());
    ~DataOutputEditableModel() override;

    bool useSystemLocale() const override;
    void setUseSystemLocale(bool useSystemLocale) override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void clear() override;
    void refresh() override;
    void readConfig() override;

    QObject *asQObject() override {return this;};

    void setStyleHelper(DataOutputStyleHelper *helper);

private:
    DataOutputStyleHelper *m_styleHelper;
};
