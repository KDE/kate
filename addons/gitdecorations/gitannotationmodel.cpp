#include "gitannotationmodel.h"
#include "gitdiff.h"

#include <QBrush>
#include <QColor>

GitAnnotationModel::GitAnnotationModel(QObject *parent)
    : KTextEditor::AnnotationModel()
{
    setParent(parent);
}

void GitAnnotationModel::setDiff(const VcsDiff &diff)
{
    m_changes.clear();

    const auto lines = diff.diff().split(u'\n');
    bool hasDeletion = false;

    // TODO: Improve change type calculation and check for edge cases

    for (int i = 0; i < lines.size(); ++i) {
        const auto &line = lines.at(i);

        if (line.startsWith(u"--- ") || line.startsWith(u"+++ ")) {
            continue;
        }

        if (line.startsWith(u'-')) {
            hasDeletion = true;
            continue;
        }

        if (line.startsWith(u'+')) {
            const int targetLine = diff.diffLineToTargetLine(i);
            if (targetLine >= 0) {
                m_changes[targetLine] = ChangeType::Added;
            }

            hasDeletion = false;
            continue;
        }

        const int targetLine = diff.diffLineToTargetLine(i);
        if (hasDeletion && targetLine >= 0) {
            m_changes[targetLine] = ChangeType::Removed;
            hasDeletion = false;
        }
    }

    Q_EMIT reset();
}

QVariant GitAnnotationModel::data(int line, Qt::ItemDataRole role) const
{
    const auto it = m_changes.constFind(line);
    if (it == m_changes.constEnd()) {
        return {};
    }

    if (role == ChangeRole) {
        return static_cast<int>(it.value());
    }

    return {};
}