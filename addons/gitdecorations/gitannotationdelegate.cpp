/*
    SPDX-FileCopyrightText: 2026 Leo Ruggeri <leo5t@yahoo.it>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "gitannotationdelegate.h"
#include "gitannotationmodel.h"

#include <KTextEditor/AbstractAnnotationItemDelegate>
#include <KTextEditor/AnnotationInterface>
#include <KTextEditor/Document>
#include <KTextEditor/View>

#include <KColorScheme>
#include <QBrush>
#include <QColor>
#include <QHelpEvent>
#include <QPainter>

namespace
{

QBrush createHatchBrush(const QColor &color)
{
    constexpr int hatchSize = 6;
    constexpr int hatchWidth = 2;
    QPixmap hatchPixmap(hatchSize, hatchSize);
    hatchPixmap.fill(Qt::transparent);
    QPainter hatchPainter(&hatchPixmap);
    hatchPainter.setRenderHint(QPainter::Antialiasing, false);
    QPen pen(color.lighter(170));
    pen.setWidth(hatchWidth);
    pen.setCapStyle(Qt::SquareCap);
    hatchPainter.setPen(pen);
    hatchPainter.drawLine(0, hatchSize, hatchSize, 0);
    hatchPainter.drawLine(-hatchSize, hatchSize, 0, 0);
    hatchPainter.drawLine(hatchSize, hatchSize, 2 * hatchSize, 0);
    return QBrush(hatchPixmap);
}

}

GitAnnotationDelegate::GitAnnotationDelegate(KTextEditor::View *parent)
    : KTextEditor::AbstractAnnotationItemDelegate(parent)
    , m_view(parent)
{
    initializeColors();
    m_view->installEventFilter(this);
}

void GitAnnotationDelegate::initializeColors()
{
    const KColorScheme scheme(QPalette::Active, KColorScheme::View);
    m_modifiedColor = m_view->palette().highlight().color();
    m_modifiedColorOutOfSync = createHatchBrush(m_modifiedColor);
    m_removedColor = scheme.foreground(KColorScheme::NegativeText).color();
    m_removedColorOutOfSync = m_removedColor.lighter(150);
}

void GitAnnotationDelegate::paint(QPainter *painter, const KTextEditor::StyleOptionAnnotationItem &option, KTextEditor::AnnotationModel *model, int line) const
{
    const QVariant value = model->data(line, GitAnnotationModel::ChangeRole);
    if (!value.isValid()) {
        return;
    }

    if (!m_view || !m_view->document()) {
        return;
    }

    const auto change = static_cast<GitAnnotationModel::ChangeTypes>(value.toInt());
    const bool modified = change.testFlag(GitAnnotationModel::ChangeType::Modified);
    const bool removedAfter = change.testFlag(GitAnnotationModel::ChangeType::RemovedAfter);
    const bool removedBefore = change.testFlag(GitAnnotationModel::ChangeType::RemovedBefore);
    const bool isDocumentModified = m_view->document()->isModified();

    constexpr int vBarWidth = 3;
    constexpr int hBarHeight = 3;

    if (modified) {
        QRect rect = option.rect;
        rect.setLeft(rect.right() - vBarWidth + 1);
        if (removedAfter || removedBefore) {
            rect.moveTop(option.rect.top() + hBarHeight + 1); // Reserve 1px margin between the markers
        }
        painter->fillRect(rect, m_modifiedColor);
        if (isDocumentModified) {
            painter->fillRect(rect, m_modifiedColorOutOfSync);
        }
    }

    if (removedAfter || removedBefore) {
        QRect rect = option.rect;
        rect.setHeight(hBarHeight);
        rect.moveTop(removedBefore ? option.rect.top() : option.rect.bottom() - hBarHeight + 1);
        painter->fillRect(rect, isDocumentModified ? m_removedColorOutOfSync : m_removedColor);
    }
}

QSize GitAnnotationDelegate::sizeHint(const KTextEditor::StyleOptionAnnotationItem &, KTextEditor::AnnotationModel *, int) const
{
    return QSize(6, 0); // KateIconBorder ignores lower values
}

bool GitAnnotationDelegate::helpEvent(QHelpEvent *, KTextEditor::View *, const KTextEditor::StyleOptionAnnotationItem &, KTextEditor::AnnotationModel *, int)
{
    return false;
}

void GitAnnotationDelegate::hideTooltip(KTextEditor::View *)
{
}

bool GitAnnotationDelegate::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_view && event->type() == QEvent::PaletteChange) {
        initializeColors();
    }

    return QObject::eventFilter(watched, event);
}
