/*  This file is part of the Kate project.
 *  Based on the snippet plugin from KDevelop 4.
 *
 *  Copyright (C) 2007 Robert Gruber <rgruber@users.sourceforge.net> 
 *  Copyright (C) 2012 Christoph Cullmann <cullmann@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */
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

public Q_SLOTS:
    void changeFilter(const QString& filter);

private Q_SLOTS:
    void dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);

private:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

    QString filter_;
};

#endif

