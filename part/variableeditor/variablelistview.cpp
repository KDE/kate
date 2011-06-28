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

class ContainerProxyWidget : public QWidget
{
public:
  ContainerProxyWidget(VariableListView* parent = 0)
    : QWidget(parent), m_listView(parent)
  {
  }

  virtual QSize sizeHint() const
  {
    int h = 0;
    foreach (QWidget* w, m_listView->m_editors) {
      h += w->sizeHint().height();
    }

    return QSize(-1, h); // the width is not used
  }
  
private:
  VariableListView* m_listView;
};

VariableListView::VariableListView(QWidget* parent)
  : QScrollArea(parent)
{
  setBackgroundRole(QPalette::Base);

  QWidget* top = new QWidget(this);
  setWidget(top);

  VariableItem* item = new VariableUintItem("indent-width", 4);
  item->setHelpText("Set the indentation depth for each level.");
  item->setActive(true);
  
  m_editors << item->createEditor(top);
  m_items << item;

  item = new VariableBoolItem("replace-tabs", true);
  item->setHelpText("Replace tabs with spaces.");
  item->setActive(true);
  
  m_editors << item->createEditor(top);
  m_items << item;

  item = new VariableStringListItem("indent-style", QStringList() << "cstyle" << "lisp" << "python" << "none", "none");
  item->setHelpText("Set the indentation style.");
  item->setActive(false);
  
  m_editors << item->createEditor(top);
  m_items << item;
  
  for (int i = 0; i < m_editors.size(); ++i) {
    m_editors[i]->setBackgroundRole((i % 2) ? QPalette::AlternateBase : QPalette::Base);
    m_editors[i]->show();

    connect(m_editors[i], SIGNAL(valueChanged()), this, SLOT(somethingChanged()));
  }
}

VariableListView::~VariableListView()
{
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
