/*
   SPDX-FileCopyrightText: 2010 Marco Mentasti <marcomentasti@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <QTableView>

class DataOutputView : public QTableView
{
    Q_OBJECT
public:
    explicit DataOutputView(QWidget *parent = nullptr);

    void commitCurrentEditorData();

Q_SIGNALS:
    void contextMenuAboutToShow(const QPoint &pos, int column);

private:
    void slotCustomContextMenuRequested(const QPoint &pos);
};
