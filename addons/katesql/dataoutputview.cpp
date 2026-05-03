/*
   SPDX-FileCopyrightText: 2010 Marco Mentasti <marcomentasti@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#include "dataoutputview.h"

#include <QApplication>
#include <QMenu>

DataOutputView::DataOutputView(QWidget *parent)
    : QTableView(parent)
{
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &DataOutputView::customContextMenuRequested, this, &DataOutputView::slotCustomContextMenuRequested);
}

void DataOutputView::slotCustomContextMenuRequested(const QPoint &pos)
{
    const int column = columnAt(pos.x());
    Q_EMIT contextMenuAboutToShow(pos, column);

    QMenu menu(this);
    menu.addActions(actions());
    menu.exec(mapToGlobal(pos));
}

void DataOutputView::commitCurrentEditorData()
{
    if (state() != EditingState) {
        return;
    }

    QWidget *editor = QApplication::focusWidget();
    if (editor && (editor->parent() == viewport() || editor->parent() == this)) {
        QTableView::commitData(editor);
    }
}
