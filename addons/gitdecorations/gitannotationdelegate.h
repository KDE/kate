/*
    SPDX-FileCopyrightText: 2026 Leo Ruggeri <leo5t@yahoo.it>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <KTextEditor/AbstractAnnotationItemDelegate>
#include <QColor>

class GitAnnotationDelegate : public KTextEditor::AbstractAnnotationItemDelegate
{
public:
    explicit GitAnnotationDelegate(KTextEditor::View *parent = nullptr);
    void paint(QPainter *painter, const KTextEditor::StyleOptionAnnotationItem &option, KTextEditor::AnnotationModel *model, int line) const override;
    QSize sizeHint(const KTextEditor::StyleOptionAnnotationItem &option, KTextEditor::AnnotationModel *model, int line) const override;
    bool helpEvent(QHelpEvent *event,
                   KTextEditor::View *view,
                   const KTextEditor::StyleOptionAnnotationItem &option,
                   KTextEditor::AnnotationModel *model,
                   int line) override;
    void hideTooltip(KTextEditor::View *view) override;

private:
    QColor m_addedColor;
    QColor m_changedColor;
    QColor m_removedColor;
};