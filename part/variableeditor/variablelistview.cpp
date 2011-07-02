/* This file is part of the KDE project

   Copyright (C) 2011 Dominik Haumann <dhaumann kde org>

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

#include "variableeditor.h"
#include "variablelistview.h"
#include "variableitem.h"

#include <QtCore/QDebug>
#include <QtGui/QPainter>
#include <QtGui/QResizeEvent>

VariableListView::VariableListView(QWidget* parent)
  : QScrollArea(parent)
{
  setBackgroundRole(QPalette::Base);

  setWidget(new QWidget(this));
}

VariableListView::~VariableListView()
{
}

void VariableListView::addItem(VariableItem* item)
{
  QWidget* editor = item->createEditor(widget());
  editor->setBackgroundRole((m_editors.size() % 2) ? QPalette::AlternateBase : QPalette::Base);

  m_editors << editor;
  m_items << item;

  connect(editor, SIGNAL(valueChanged()), this, SLOT(somethingChanged()));
}

QVector<VariableItem *> VariableListView::items()
{
  return m_items;
}

void VariableListView::resizeEvent(QResizeEvent* event)
{
  QScrollArea::resizeEvent(event);
  
  // calculate sum of all editor heights
  int listHeight = 0;
  foreach (QWidget* w, m_editors) {
    listHeight += w->sizeHint().height();
  }

  // resize scroll area widget
  QWidget* top = widget();
  top->resize(event->size().width(), listHeight);

  // set client geometries correctly
  int h = 0;
  foreach (QWidget* w, m_editors) {
    w->setGeometry(0, h, top->width(), w->sizeHint().height());
    h += w->sizeHint().height();
  }
}

void VariableListView::somethingChanged()
{
  qDebug() << "CHANGED";
}

// kate: indent-width 2; replace-tabs on;
