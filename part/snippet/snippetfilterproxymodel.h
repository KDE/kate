/***************************************************************************
 *   Copyright 2007 Robert Gruber <rgruber@users.sourceforge.net>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef __SNIPPETFILTERPROXYMODEL_H__
#define __SNIPPETFILTERPROXYMODEL_H__

#include <QSortFilterProxyModel>

/**
 * @author Robert Gruber <rgruber@users.sourceforge.net>
 */
class SnippetFilterProxyModel : public QSortFilterProxyModel {
	Q_OBJECT

public:
    SnippetFilterProxyModel(QObject *parent);
    ~SnippetFilterProxyModel();

    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

public slots:
    void changeFilter(const QString& filter);

private slots:
    void dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);

private:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

    QString filter_;
};

#endif

