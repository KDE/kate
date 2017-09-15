//
// Description: Widget that local variables of the gdb inferior
//
// Copyright (c) 2010 Kåre Särs <kare.sars@iki.fi>
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Library General Public
//  License version 2 as published by the Free Software Foundation.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Library General Public License for more details.
//
//  You should have received a copy of the GNU Library General Public License
//  along with this library; see the file COPYING.LIB.  If not, write to
//  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
//  Boston, MA 02110-1301, USA.

#ifndef LOCALSVIEW_H
#define LOCALSVIEW_H

#include <QTreeWidget>
#include <QTreeWidgetItem>


class LocalsView : public QTreeWidget
{
Q_OBJECT
public:
    LocalsView(QWidget *parent = nullptr);
    ~LocalsView() override;

public Q_SLOTS:
    // An empty value string ends the locals
    void addLocal(const QString &vString);
    void addStruct(QTreeWidgetItem *parent, const QString &vString);
    void addArray(QTreeWidgetItem *parent, const QString &vString);

Q_SIGNALS:
    void localsVisible(bool visible);

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private:
    void createWrappedItem(QTreeWidgetItem *parent, const QString &name, const QString &value);
    void createWrappedItem(QTreeWidget *parent, const QString &name, const QString &value);
    bool    m_allAdded;
    QString m_local;
};

#endif
