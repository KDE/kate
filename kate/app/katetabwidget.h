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

#ifndef __KATE_TABWIDGET_H__
#define __KATE_TABWIDGET_H__

#include <KTabWidget>

class KateTabWidget : public KTabWidget
{
    Q_OBJECT

  public:
    enum TabWidgetVisibility {
      AlwaysShowTabs         = 0,
      ShowWhenMoreThanOneTab = 1,
      NeverShowTabs          = 2
  };

  public:
    KateTabWidget(QWidget* parent);
    virtual ~KateTabWidget();

    virtual void addTab ( QWidget * child, const QString & label );

    virtual void addTab ( QWidget * child, const QIcon & iconset, const QString & label );

    virtual void insertTab ( QWidget * child, const QString & label, int index = -1 );

    virtual void insertTab ( QWidget * child, const QIcon & iconset, const QString & label, int index = -1 );

    virtual void removePage ( QWidget * w );

    TabWidgetVisibility tabWidgetVisibility() const;

    void setTabWidgetVisibility( TabWidgetVisibility );

  private Q_SLOTS:
    void closeTab(QWidget* w);

  private:
    void maybeShow();
    void setCornerWidgetVisibility(bool visible);

  private:
    TabWidgetVisibility m_visibility;
};

#endif
// kate: space-indent on; indent-width 2; replace-tabs on;

