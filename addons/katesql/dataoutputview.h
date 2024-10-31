/*
   SPDX-FileCopyrightText: 2010 Marco Mentasti <marcomentasti@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <QTableView>

class DataOutputView : public QTableView
{
public:
    explicit DataOutputView(QWidget *parent = nullptr);

private:
    void slotCustomContextMenuRequested(const QPoint &pos);
};
