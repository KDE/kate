#ifndef KATE_SEARCH_MATCH_PROXY_MODEL_H
#define KATE_SEARCH_MATCH_PROXY_MODEL_H

#include <QSortFilterProxyModel>

class MatchProxyModel final : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    using QSortFilterProxyModel::QSortFilterProxyModel;

    bool filterAcceptsRow(int sourceRow, const QModelIndex &parent) const override;

    Q_SLOT void setFilterText(const QString &text)
    {
        beginResetModel();
        m_text = text;
        endResetModel();
    }

private:
    bool isMatchItem(const QModelIndex &index) const;

    bool parentAcceptsRow(const QModelIndex &source_parent) const;

    QString m_text;
};

#endif
