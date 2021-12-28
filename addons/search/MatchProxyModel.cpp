#include "MatchProxyModel.h"
#include "MatchModel.h"

bool MatchProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &parent) const
{
    // root item always visible
    if (!parent.isValid()) {
        return true;
    }

    // nothing to filter
    if (m_text.isEmpty()) {
        return true;
    }

    const auto index = sourceModel()->index(sourceRow, 0, parent);
    if (!index.isValid()) {
        return false;
    }

    const QString text = index.data(MatchModel::PlainTextRole).toString();

    // match text;
    if (text.contains(m_text, Qt::CaseInsensitive)) {
        return true;
    }

    // text didn't match. Check if this is a match item & its parent is accepted
    if (isMatchItem(index) && parentAcceptsRow(parent)) {
        return true;
    }

    // filter it out
    return false;
}

bool MatchProxyModel::isMatchItem(const QModelIndex &index) const
{
    return index.parent().isValid() && index.parent().parent().isValid();
}

bool MatchProxyModel::parentAcceptsRow(const QModelIndex &source_parent) const
{
    if (source_parent.isValid()) {
        const QModelIndex index = source_parent.parent();
        if (filterAcceptsRow(source_parent.row(), index)) {
            return true;
        }
        // we don't want to recurse because our root item is always accepted
    }
    return false;
}
