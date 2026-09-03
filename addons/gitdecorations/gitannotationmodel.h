#pragma once

#include <KTextEditor/AnnotationInterface>

#include <QHash>

class VcsDiff;

class GitAnnotationModel : public KTextEditor::AnnotationModel
{
public:
    enum class ChangeType {
        None = 0,
        Added = 1 << 0,
        Changed = 1 << 1,
        RemovedAfter = 1 << 2,
        RemovedBefore = 1 << 3
    };

    Q_DECLARE_FLAGS(ChangeTypes, ChangeType)

    static constexpr Qt::ItemDataRole ChangeRole = static_cast<Qt::ItemDataRole>(Qt::UserRole + 1);

    explicit GitAnnotationModel(QObject *parent = nullptr);
    void setDiff(const VcsDiff &diff);
    QVariant data(int line, Qt::ItemDataRole role) const override;

private:
    QHash<int, ChangeTypes> m_changes;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(GitAnnotationModel::ChangeTypes)