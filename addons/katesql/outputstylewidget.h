/*
   SPDX-FileCopyrightText: 2010 Marco Mentasti <marcomentasti@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <QTreeWidget>

class OutputStyleWidget : public QTreeWidget
{
    Q_OBJECT

public:
    explicit OutputStyleWidget(QWidget *parent = nullptr);
    ~OutputStyleWidget() override;

    QTreeWidgetItem *addContext(const QString &key, const QString &name);

public Q_SLOTS:
    void readConfig();
    void writeConfig();

protected Q_SLOTS:
    void slotChanged();
    void updatePreviews();

    void readConfig(QTreeWidgetItem *item);
    void writeConfig(QTreeWidgetItem *item);

Q_SIGNALS:
    void changed();
};
