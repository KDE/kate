/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2007 Mirko Stocker <me@misto.ch>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __KATE_CONFIGPLUGINDIALOGPAGE_H__
#define __KATE_CONFIGPLUGINDIALOGPAGE_H__

#include <QFrame>
#include <QTreeWidget>

class KatePluginListItem;

class KatePluginListView : public QTreeWidget
{
    Q_OBJECT

public:
    KatePluginListView(QWidget *parent = nullptr);

Q_SIGNALS:
    void stateChange(KatePluginListItem *, bool);

private Q_SLOTS:
    void stateChanged(QTreeWidgetItem *);
};

class KateConfigPluginPage: public QFrame
{
    Q_OBJECT

public:
    KateConfigPluginPage(QWidget *parent, class KateConfigDialog *dialog);

private:
    class KateConfigDialog *myDialog;

Q_SIGNALS:
    void changed();

private Q_SLOTS:
    void stateChange(KatePluginListItem *, bool);

    void loadPlugin(KatePluginListItem *);
    void unloadPlugin(KatePluginListItem *);
};

#endif

