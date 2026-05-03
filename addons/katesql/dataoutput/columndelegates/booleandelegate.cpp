/*
   SPDX-FileCopyrightText: 2026 Artyom Kirnev <kirnevartem30@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#include "booleandelegate.h"

#include <QApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QStyleOptionButton>

BooleanDelegate::BooleanDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void BooleanDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    painter->save();
    if (opt.backgroundBrush.style() != Qt::NoBrush) {
        painter->fillRect(opt.rect, opt.backgroundBrush);
    }
    if (opt.state & QStyle::State_Selected) {
        painter->fillRect(opt.rect, opt.palette.highlight());
    }

    // Draw a centred checkbox indicator.
    const bool checked = index.data(Qt::EditRole).toBool();

    QStyleOptionButton checkboxOption;
    checkboxOption.state = QStyle::State_Enabled | (checked ? QStyle::State_On : QStyle::State_Off);
    checkboxOption.direction = opt.direction;
    checkboxOption.fontMetrics = opt.fontMetrics;
    checkboxOption.palette = opt.palette;

    const int indicatorWidth = QApplication::style()->pixelMetric(QStyle::PM_IndicatorWidth, nullptr, nullptr);
    const int indicatorHeight = QApplication::style()->pixelMetric(QStyle::PM_IndicatorHeight, nullptr, nullptr);
    checkboxOption.rect = QRect(opt.rect.x() + (opt.rect.width() - indicatorWidth) / 2,
                                opt.rect.y() + (opt.rect.height() - indicatorHeight) / 2,
                                indicatorWidth,
                                indicatorHeight);

    QApplication::style()->drawControl(QStyle::CE_CheckBox, &checkboxOption, painter);
    painter->restore();
}

bool BooleanDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    bool changed = false;

    if (event->type() == QEvent::MouseButtonDblClick) {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        changed = mouseEvent->button() == Qt::LeftButton;
    } else if (event->type() == QEvent::MouseButtonRelease) {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            const int indicatorWidth = QApplication::style()->pixelMetric(QStyle::PM_IndicatorWidth, nullptr, nullptr);
            const int indicatorHeight = QApplication::style()->pixelMetric(QStyle::PM_IndicatorHeight, nullptr, nullptr);
            const QRect checkboxRect(option.rect.x() + (option.rect.width() - indicatorWidth) / 2,
                                     option.rect.y() + (option.rect.height() - indicatorHeight) / 2,
                                     indicatorWidth,
                                     indicatorHeight);
            changed = checkboxRect.contains(mouseEvent->pos());
        }
    } else if (event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        changed = keyEvent->key() == Qt::Key_Space;
    }

    if (changed) {
        const bool currentValue = index.data(Qt::EditRole).toBool();
        model->setData(index, !currentValue, Qt::EditRole);
        return true;
    }
    return QStyledItemDelegate::editorEvent(event, model, option, index);
}

QWidget *BooleanDelegate::createEditor(QWidget * /*parent*/, const QStyleOptionViewItem & /*option*/, const QModelIndex & /*index*/) const
{
    // No separate editor needed — the checkbox is drawn directly in the cell.
    return nullptr;
}

QSize BooleanDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    const int indicatorWidth = QApplication::style()->pixelMetric(QStyle::PM_IndicatorWidth, nullptr, nullptr);
    const int indicatorHeight = QApplication::style()->pixelMetric(QStyle::PM_IndicatorHeight, nullptr, nullptr);
    const int margin = 4;
    return QSize(indicatorWidth + margin, qMax(indicatorHeight + margin, QStyledItemDelegate::sizeHint(option, index).height()));
}
