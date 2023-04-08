/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <QTreeView>

class AsmView : public QTreeView
{
    Q_OBJECT
public:
    explicit AsmView(QWidget *parent);

protected:
    void contextMenuEvent(QContextMenuEvent *e) override;

Q_SIGNALS:
    void scrollToLineRequested(int line);
};
