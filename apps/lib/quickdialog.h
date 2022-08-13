/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef QUICKDIALOG_H
#define QUICKDIALOG_H

#include <QLineEdit>
#include <QMenu>
#include <QPointer>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>
#include <QTreeView>

#include "kateprivate_export.h"

class QAbstractItemModel;
class QStyledItemDelegate;

namespace KTextEditor
{
class MainWindow;
}

class KATE_PRIVATE_EXPORT HUDStyleDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

public:
    void setFilterString(const QString &text)
    {
        m_filterString = text;
    }

    void setDisplayRole(int role)
    {
        m_displayRole = role;
    }

protected:
    QString m_filterString;
    int m_displayRole = Qt::DisplayRole;
};

class KATE_PRIVATE_EXPORT HUDDialog : public QMenu
{
    Q_OBJECT
public:
    HUDDialog(QWidget *parent, QWidget *mainWindow);
    ~HUDDialog();

    enum FilterType { Contains, Fuzzy, ScoredFuzzy };

    void setStringList(const QStringList &);

    void setModel(QAbstractItemModel *, FilterType, int filterKeyCol = 0, int filterRole = Qt::DisplayRole, int scoreRole = -1);

    void setFilteringEnabled(bool enabled);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void updateViewGeometry();
    void clearLineEdit();
    void setDelegate(HUDStyleDelegate *);
    void reselectFirst();

protected Q_SLOTS:
    virtual void slotReturnPressed(const QModelIndex &index);

protected:
    QTreeView m_treeView;
    QLineEdit m_lineEdit;

private:
    QPointer<QWidget> m_mainWindow;
    QPointer<QAbstractItemModel> m_model;
    QPointer<QSortFilterProxyModel> m_proxy;
    HUDStyleDelegate *m_delegate = nullptr;

Q_SIGNALS:
    void itemExecuted(const QModelIndex &index);
};

#endif // QUICKDIALOG_H
