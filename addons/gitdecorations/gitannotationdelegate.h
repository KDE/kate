#pragma once

#include <KTextEditor/AbstractAnnotationItemDelegate>

class GitAnnotationDelegate : public KTextEditor::AbstractAnnotationItemDelegate
{
public:
    explicit GitAnnotationDelegate(QObject *parent = nullptr);
    void paint(QPainter *painter, const KTextEditor::StyleOptionAnnotationItem &option, KTextEditor::AnnotationModel *model, int line) const override;
    QSize sizeHint(const KTextEditor::StyleOptionAnnotationItem &option, KTextEditor::AnnotationModel *model, int line) const override;
    bool helpEvent(QHelpEvent *event,
                   KTextEditor::View *view,
                   const KTextEditor::StyleOptionAnnotationItem &option,
                   KTextEditor::AnnotationModel *model,
                   int line) override;
    void hideTooltip(KTextEditor::View *view) override;
};