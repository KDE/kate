/*
   SPDX-FileCopyrightText: 2026 Artyom Kirnev <kirnevartem30@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#include "datetimedelegate.h"

#include <QDateTimeEdit>

DateTimeDelegate::DateTimeDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

QWidget *DateTimeDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem & /*option*/, const QModelIndex & /*index*/) const
{
    auto *editor = new QDateTimeEdit(parent);
    editor->setCalendarPopup(true);
    editor->setDisplayFormat(QLocale().dateTimeFormat(QLocale::ShortFormat));

    return editor;
}

void DateTimeDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    const QDateTime value = index.data(Qt::EditRole).toDateTime();
    if (auto *dateTimeEdit = qobject_cast<QDateTimeEdit *>(editor)) {
        dateTimeEdit->setDateTime(value);
    }
}

void DateTimeDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    if (auto *dateTimeEdit = qobject_cast<QDateTimeEdit *>(editor)) {
        model->setData(index, dateTimeEdit->dateTime(), Qt::EditRole);
    }
}

void DateTimeDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex & /*index*/) const
{
    editor->setGeometry(option.rect);
}
