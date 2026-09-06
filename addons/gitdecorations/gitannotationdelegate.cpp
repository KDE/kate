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

GitAnnotationDelegate::GitAnnotationDelegate(KTextEditor::View *parent)
    : KTextEditor::AbstractAnnotationItemDelegate(parent)
    , m_view(parent)
{
    auto createHatchBrush = [](QColor color) {
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
    };

    auto initializeColors = [this, createHatchBrush]() {
        const KColorScheme scheme(QPalette::Active, KColorScheme::View);
        m_modifiedColor = scheme.foreground(KColorScheme::ActiveText).color();
        m_modifiedColorOutOfSync = createHatchBrush(m_modifiedColor);
        m_removedColor = scheme.foreground(KColorScheme::NegativeText).color();
        m_removedColorOutOfSync = m_removedColor.lighter(150);
    };

    initializeColors();
    // TODO: Update colors when theme changes
}

void GitAnnotationDelegate::paint(QPainter *painter, const KTextEditor::StyleOptionAnnotationItem &option, KTextEditor::AnnotationModel *model, int line) const
{
    if (!painter || !model) {
        return;
    }

    const QVariant value = model->data(line, GitAnnotationModel::ChangeRole);
    if (!value.isValid()) {
        return;
    }

    if (!m_view || !m_view->document()) {
        return;
    }

    const auto change = static_cast<GitAnnotationModel::ChangeTypes>(value.toInt());
    bool added = change.testFlag(GitAnnotationModel::ChangeType::Added);
    bool removedAfter = change.testFlag(GitAnnotationModel::ChangeType::RemovedAfter);
    bool removedBefore = change.testFlag(GitAnnotationModel::ChangeType::RemovedBefore);
    bool changed = change.testFlag(GitAnnotationModel::ChangeType::Changed);
    bool isDocumentModified = m_view->document()->isModified();

    if (added || changed) {
        constexpr int barWidth = 3;
        QRect rect = option.rect;
        rect.setLeft(rect.right() - barWidth + 1);
        painter->fillRect(rect, m_modifiedColor);
        if (isDocumentModified) {
            painter->fillRect(rect, m_modifiedColorOutOfSync);
        }
    }

    if (removedAfter || removedBefore) {
        constexpr int barHeight = 2;
        QRect rect = option.rect;
        rect.setHeight(barHeight);
        rect.moveTop(removedBefore ? option.rect.top() : option.rect.bottom() - barHeight + 1);
        painter->fillRect(rect, isDocumentModified ? m_removedColorOutOfSync : m_removedColor);
    }
}

QSize GitAnnotationDelegate::sizeHint(const KTextEditor::StyleOptionAnnotationItem &, KTextEditor::AnnotationModel *, int) const
{
    return QSize(6, 0); // TODO: Remove workaround and fix KateIconBorder
}

bool GitAnnotationDelegate::helpEvent(QHelpEvent *, KTextEditor::View *, const KTextEditor::StyleOptionAnnotationItem &, KTextEditor::AnnotationModel *, int)
{
    return false;
}

void GitAnnotationDelegate::hideTooltip(KTextEditor::View *)
{
}
