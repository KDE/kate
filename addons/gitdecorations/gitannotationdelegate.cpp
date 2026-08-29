#include "gitannotationdelegate.h"
#include "gitannotationmodel.h"

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

    const QVariant value = model->data(line, GitAnnotationModel::ChangeRole);
    if (!value.isValid()) {
        return;
    }

    const auto change = static_cast<GitAnnotationModel::ChangeType>(value.toInt());
    painter->save();

    switch (change) {
    case GitAnnotationModel::ChangeType::Added: {
        constexpr int barWidth = 3;
        QRect rect = option.rect;
        rect.setLeft(rect.right() - barWidth + 1);
        const QColor color = QColor(QStringLiteral("#4CAF50"));
        painter->fillRect(rect, color);
        break;
    }

    case GitAnnotationModel::ChangeType::Removed: {
        constexpr int triangleHeight = 8;
        int triangleWidth = option.rect.width();
        const int right = option.rect.right();
        const int centerY = option.rect.top();
        const int left = right - triangleWidth;
        QPolygon triangle;
        triangle << QPoint(left, centerY - triangleHeight / 2) << QPoint(left, centerY + triangleHeight / 2) << QPoint(right, centerY);
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(QStringLiteral("#F44336")));
        painter->drawPolygon(triangle);
        painter->restore();
        break;
    }
    }

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