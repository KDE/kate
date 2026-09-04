/*
    SPDX-FileCopyrightText: 2026 Leo Ruggeri <leo5t@yahoo.it>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "gitannotationdelegate.h"
#include "gitannotationmodel.h"

#include <KTextEditor/AbstractAnnotationItemDelegate>
#include <KTextEditor/AnnotationInterface>
#include <KTextEditor/View>

#include <KColorScheme>
#include <QBrush>
#include <QColor>
#include <QHelpEvent>
#include <QPainter>

GitAnnotationDelegate::GitAnnotationDelegate(KTextEditor::View *parent)
    : KTextEditor::AbstractAnnotationItemDelegate(parent)
{
    auto initializeColors = [this]() {
        const KColorScheme scheme(QPalette::Active, KColorScheme::View);
        m_addedColor = scheme.foreground(KColorScheme::ActiveText).color();
        m_changedColor = scheme.foreground(KColorScheme::ActiveText).color().lighter(140);
        m_removedColor = scheme.foreground(KColorScheme::NegativeText).color();
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

    const auto change = static_cast<GitAnnotationModel::ChangeTypes>(value.toInt());
    bool added = change.testFlag(GitAnnotationModel::ChangeType::Added);
    bool removedAfter = change.testFlag(GitAnnotationModel::ChangeType::RemovedAfter);
    bool removedBefore = change.testFlag(GitAnnotationModel::ChangeType::RemovedBefore);
    bool changed = change.testFlag(GitAnnotationModel::ChangeType::Changed);

    if (added || changed) {
        painter->save();
        constexpr int barWidth = 3;
        QRect rect = option.rect;
        rect.setLeft(rect.right() - barWidth + 1);
        const QColor color = QColor(changed ? m_changedColor : m_addedColor);
        painter->fillRect(rect, color);
        painter->restore();
    }

    if (removedAfter || removedBefore) {
        painter->save();
        constexpr int barHeight = 2;
        QRect rect = option.rect;
        rect.setHeight(barHeight);
        rect.moveTop(removedBefore ? option.rect.top() : option.rect.bottom() - barHeight + 1);
        painter->fillRect(rect, m_removedColor);
        painter->restore();
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