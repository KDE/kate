/*
    SPDX-FileCopyrightText: 2001 Christoph Cullmann <cullmann@kde.org>
    SPDX-FileCopyrightText: 2002 Joseph Wenninger <jowenn@kde.org>
    SPDX-FileCopyrightText: 2007 Mirko Stocker <me@misto.ch>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QFrame>
#include <QTreeWidget>

class KatePluginListItem;

class KatePluginListView : public QTreeWidget
{
    Q_OBJECT

public:
    explicit KatePluginListView(QWidget *parent = nullptr);

Q_SIGNALS:
    void stateChange(KatePluginListItem *, bool);

private:
    void stateChanged(QTreeWidgetItem *);
};

class KateConfigPluginPage : public QFrame
{
    Q_OBJECT

public:
    KateConfigPluginPage(QWidget *parent, class KateConfigDialog *dialog);

public:
    void slotApply();

private:
    class KateConfigDialog *myDialog;

Q_SIGNALS:
    void changed();

private:
    void stateChange(KatePluginListItem *, bool);

    void loadPlugin(KatePluginListItem *);
    void unloadPlugin(KatePluginListItem *);

private:
    std::vector<class KatePluginListItem *> m_pluginItems;
};
