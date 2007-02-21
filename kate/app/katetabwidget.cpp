/* This file is part of the KDE project
   Copyright (C) 2005 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002,2003 Joseph Wenninger <jowenn@kde.org>
 
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

#include "katetabwidget.h"
#include "katetabwidget.moc"

#include <QTabBar>

//BEGIN KateTabWidget

KateTabWidget::KateTabWidget(QWidget* parent)
    : KTabWidget(parent)
    , m_visibility (ShowWhenMoreThanOneTab)
{
  tabBar()->hide();

  setHoverCloseButton(true);

  connect(this, SIGNAL(closeRequest(QWidget*)), this, SLOT(closeTab(QWidget*)));
}

KateTabWidget::~KateTabWidget()
{}

void KateTabWidget::closeTab(QWidget* w)
{
  w->close();
}

void KateTabWidget::addTab ( QWidget * child, const QString & label )
{
  KTabWidget::addTab(child, label);
  setCurrentIndex(indexOf(child));
  maybeShow();
}

void KateTabWidget::addTab ( QWidget * child, const QIcon & iconset, const QString & label )
{
  KTabWidget::addTab(child, iconset, label);
  setCurrentIndex(indexOf(child));
  maybeShow();
}

void KateTabWidget::insertTab ( QWidget * child, const QString & label, int index)
{
  KTabWidget::insertTab(index, child, label);
  setCurrentIndex(indexOf(child));
  maybeShow();
  tabBar()->repaint();
}

void KateTabWidget::insertTab ( QWidget * child, const QIcon & iconset, const QString & label, int index )
{
  KTabWidget::insertTab(index, child, iconset, label);
  setCurrentIndex(indexOf(child));
  maybeShow();
  tabBar()->repaint();
}

void KateTabWidget::removePage ( QWidget * w )
{
  KTabWidget::removeTab(indexOf(w));
  maybeShow();
}

void KateTabWidget::maybeShow()
{
  switch (m_visibility)
  {
    case AlwaysShowTabs:
      tabBar()->show();

      // show/hide corner widgets
      if (count() == 0)
        setCornerWidgetVisibility(false);
      else
        setCornerWidgetVisibility(true);

      break;

    case ShowWhenMoreThanOneTab:
      if (count() < 2) tabBar()->hide();
      else tabBar()->show();

      // show/hide corner widgets
      if (count() < 2)
        setCornerWidgetVisibility(false);
      else
        setCornerWidgetVisibility(true);

      break;

    case NeverShowTabs:
      tabBar()->hide();
      break;
  }
}

void KateTabWidget::setCornerWidgetVisibility(bool visible)
{
  // there are two corner widgets: on TopLeft and on TopTight!

  if (cornerWidget(Qt::TopLeftCorner) )
  {
    if (visible)
      cornerWidget(Qt::TopLeftCorner)->show();
    else
      cornerWidget(Qt::TopLeftCorner)->hide();
  }

  if (cornerWidget(Qt::TopRightCorner) )
  {
    if (visible)
      cornerWidget(Qt::TopRightCorner)->show();
    else
      cornerWidget(Qt::TopRightCorner)->hide();
  }
}

void KateTabWidget::setTabWidgetVisibility( TabWidgetVisibility visibility )
{
  m_visibility = visibility;
  maybeShow();
}

KateTabWidget::TabWidgetVisibility KateTabWidget::tabWidgetVisibility( ) const
{
  return m_visibility;
}

//END KateTabWidget
// kate: space-indent on; indent-width 2; replace-tabs on;

