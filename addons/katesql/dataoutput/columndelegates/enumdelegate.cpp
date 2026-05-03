/*
   SPDX-FileCopyrightText: 2026 Artyom Kirnev <kirnevartem30@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#include "enumdelegate.h"

#include <QComboBox>

EnumDelegate::EnumDelegate(const QStringList &enumValues, QObject *parent)
    : QStyledItemDelegate(parent)
    , m_enumValues(enumValues)
{
}

QWidget *EnumDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem & /*option*/, const QModelIndex & /*index*/) const
{
    auto *editor = new QComboBox(parent);
    editor->addItems(m_enumValues);
    editor->setInsertPolicy(QComboBox::NoInsert);
    return editor;
}

void EnumDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    if (auto *comboBox = qobject_cast<QComboBox *>(editor)) {
        const QString value = index.data(Qt::EditRole).toString();
        int idx = comboBox->findText(value);
        if (idx >= 0) {
            comboBox->setCurrentIndex(idx);
        }
    }
}

void EnumDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    if (auto *comboBox = qobject_cast<QComboBox *>(editor)) {
        model->setData(index, comboBox->currentText(), Qt::EditRole);
    }
}

void EnumDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex & /*index*/) const
{
    editor->setGeometry(option.rect);
}
