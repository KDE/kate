#include "gitannotationdelegate.h"

#include <KTextEditor/AbstractAnnotationItemDelegate>
#include <KTextEditor/AnnotationInterface>
#include <KTextEditor/View>

#include <QBrush>
#include <QColor>
#include <QHelpEvent>
#include <QPainter>

GitAnnotationDelegate::GitAnnotationDelegate(QObject *parent)
    : KTextEditor::AbstractAnnotationItemDelegate(parent)
{
}

void GitAnnotationDelegate::paint(QPainter *painter, const KTextEditor::StyleOptionAnnotationItem &option, KTextEditor::AnnotationModel *model, int line) const
{
    if (!painter || !model) {
        return;
    }

    // TODO: Read line state instead of bg color
    const QVariant background = model->data(line, Qt::BackgroundRole);
    if (!background.isValid()) {
        return;
    }

    constexpr int barWidth = 2;
    QRect rect = option.rect;
    rect.setLeft(rect.right() - barWidth + 1); // Right aligned

    painter->save();
    painter->fillRect(rect, background.value<QBrush>());
    painter->restore();
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