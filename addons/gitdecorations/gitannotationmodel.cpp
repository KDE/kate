#include "gitannotationmodel.h"

#include <QBrush>
#include <QColor>

GitAnnotationModel::GitAnnotationModel(QObject *parent)
    : KTextEditor::AnnotationModel()
{
    setParent(parent);
}

QVariant GitAnnotationModel::data(int line, Qt::ItemDataRole role) const
{
    if (role == Qt::BackgroundRole) {
        if (line > 20 && line < 100) {
            return QBrush(QColor(QStringLiteral("#4CAF50")));
        }
    }

    return {};
}