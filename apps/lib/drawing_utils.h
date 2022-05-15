/*
 *  SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>
 *  SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>*
 *
 *  SPDX-License-Identifier: MIT
 */

#ifndef KATE_DRAW_UTILS_H
#define KATE_DRAW_UTILS_H

#include <QIcon>
#include <QPainter>
#include <QPixmap>
#include <QStyle>
#include <QStyleOptionViewItem>
#include <QTextLayout>

namespace Utils
{
/**
 * @brief colors the @p icon with @fgColor
 */
inline QPixmap colorIcon(const QIcon &icon, const QColor &fgColor, const QSize s = QSize(16, 16))
{
    auto p = icon.pixmap(s);
    if (p.isNull())
        return {};

    QPainter paint(&p);
    paint.setCompositionMode(QPainter::CompositionMode_SourceIn);
    paint.fillRect(p.rect(), fgColor);
    paint.end();

    return p;
}

/**
 * Draws text in an item view for e.g treeview
 * It assumes options.rect == textRect
 */
inline void paintItemViewText(QPainter *p, const QString &text, const QStyleOptionViewItem &options, QVector<QTextLayout::FormatRange> formats)
{
    // set formats
    QTextLayout textLayout(text, options.font);
    auto fmts = textLayout.formats();
    formats.append(fmts);
    textLayout.setFormats(formats);

    // set alignment, rtls etc
    QTextOption textOption;
    textOption.setTextDirection(options.direction);
    textOption.setAlignment(QStyle::visualAlignment(options.direction, options.displayAlignment));
    textLayout.setTextOption(textOption);

    // layout the text
    textLayout.beginLayout();

    QTextLine line = textLayout.createLine();
    if (!line.isValid())
        return;

    const int lineWidth = options.rect.width();
    line.setLineWidth(lineWidth);
    line.setPosition(QPointF(0, 0));

    textLayout.endLayout();

    int y = QStyle::alignedRect(Qt::LayoutDirectionAuto, Qt::AlignVCenter, textLayout.boundingRect().size().toSize(), options.rect).y();

    // draw the text
    const auto pos = QPointF(options.rect.x(), y);
    textLayout.draw(p, pos);
}

} // namespace Utils

#endif
