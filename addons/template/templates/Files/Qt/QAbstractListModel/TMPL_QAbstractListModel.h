#pragma once

#include <QAbstractListModel>
#include <QObject>

#include <optional>

class TMPL_QAbstractListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    struct Data {
        QString data;
    };

    enum Roles {
        DataRole = Qt::UserRole + 1,
    };
    Q_ENUM(Roles)

    explicit TMPL_QAbstractListModel(QObject *parent = nullptr);

    void setList(const QList<Data> &list);
    bool setAt(int row, const Data &item);
    bool insertAt(int row, const Data &item);
    std::optional<Data> takeAt(int row);
    bool removeAt(int row);
    void clear();

public:
    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

private:
    QList<Data> m_list;
};
