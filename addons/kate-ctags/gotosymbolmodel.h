/*
    SPDX-FileCopyrightText: 2020 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include <QAbstractTableModel>
#include <QIcon>
#include <QString>

struct SymbolItem {
    QString name;
    int line;
    QIcon icon;
};

class GotoSymbolModel : public QAbstractTableModel
{
public:
    explicit GotoSymbolModel(QObject *parent = nullptr);

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    void refresh(const QString &filePath);

private:
    QList<SymbolItem> m_rows;
};
