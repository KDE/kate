#pragma once

#include <KTextEditor/AnnotationInterface>

class GitAnnotationModel : public KTextEditor::AnnotationModel
{
public:
    explicit GitAnnotationModel(QObject *parent = nullptr);
    QVariant data(int line, Qt::ItemDataRole role) const override;
};