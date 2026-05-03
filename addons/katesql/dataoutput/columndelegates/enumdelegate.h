/*
   SPDX-FileCopyrightText: 2026 Artyom Kirnev <kirnevartem30@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <QStringList>
#include <QStyledItemDelegate>

class EnumDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    /**
     * Creates an enum delegate with the given allowed values.
     *
     * @param enumValues The list of allowed enum values for the column.
     * @param parent Parent object for ownership.
     */
    explicit EnumDelegate(const QStringList &enumValues, QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    QStringList m_enumValues;
};
