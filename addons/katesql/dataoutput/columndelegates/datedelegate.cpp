/*
   SPDX-FileCopyrightText: 2026 Artyom Kirnev <kirnevartem30@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#include "datedelegate.h"

#include <QDateEdit>

DateDelegate::DateDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

QWidget *DateDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem & /*option*/, const QModelIndex & /*index*/) const
{
    auto *editor = new QDateEdit(parent);
    editor->setCalendarPopup(true);
    editor->setDisplayFormat(QLocale().dateFormat(QLocale::ShortFormat));
    return editor;
}

void DateDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    const QDate value = index.data(Qt::EditRole).toDate();
    if (auto *dateEdit = qobject_cast<QDateEdit *>(editor)) {
        dateEdit->setDate(value);
    }
}

void DateDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    if (auto *dateEdit = qobject_cast<QDateEdit *>(editor)) {
        model->setData(index, dateEdit->date(), Qt::EditRole);
    }
}

void DateDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex & /*index*/) const
{
    editor->setGeometry(option.rect);
}
