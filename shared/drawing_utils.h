#ifndef KATE_DRAW_UTILS_H
#define KATE_DRAW_UTILS_H

#include <QIcon>
#include <QPainter>
#include <QPixmap>

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

} // namespace Utils

#endif
