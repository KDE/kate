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

    const auto change = static_cast<GitAnnotationModel::ChangeTypes>(value.toInt());
    bool added = change.testFlag(GitAnnotationModel::ChangeType::Added);
    bool removed = change.testFlag(GitAnnotationModel::ChangeType::Removed);
    bool changed = change.testFlag(GitAnnotationModel::ChangeType::Changed);

    if (added || changed) {
        painter->save();
        constexpr int barWidth = 3;
        QRect rect = option.rect;
        rect.setLeft(rect.right() - barWidth + 1);
        const QColor color = QColor(changed ? QStringLiteral("#82cded") : QStringLiteral("#05a1fa"));
        painter->fillRect(rect, color);
        painter->restore();
    }

    if (removed) {
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