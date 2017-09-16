/*
   Copyright (C) 2010  Marco Mentasti  <marcomentasti@gmail.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "dataoutputview.h"

#include <QAction>

#include <qcursor.h>
#include <qmenu.h>

DataOutputView::DataOutputView(QWidget *parent)
: QTableView(parent)
{
  setContextMenuPolicy(Qt::CustomContextMenu);

  connect(this, &DataOutputView::customContextMenuRequested,
          this, &DataOutputView::slotCustomContextMenuRequested);
}

void DataOutputView::slotCustomContextMenuRequested(const QPoint &pos)
{
  Q_UNUSED(pos);

  QMenu menu;

  menu.addActions(actions());

  menu.exec(QCursor::pos());
}

