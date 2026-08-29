#pragma once

#include <KTextEditor/AnnotationInterface>

#include <QHash>

class VcsDiff;

class GitAnnotationModel : public KTextEditor::AnnotationModel
{
public:
    enum class ChangeType {
        Added,
        Removed
    };

    static constexpr Qt::ItemDataRole ChangeRole = static_cast<Qt::ItemDataRole>(Qt::UserRole + 1);

    explicit GitAnnotationModel(QObject *parent = nullptr);
    void setDiff(const VcsDiff &diff);
    QVariant data(int line, Qt::ItemDataRole role) const override;

private:
    QHash<int, ChangeType> m_changes;
};
