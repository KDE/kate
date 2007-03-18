/* This file is part of the KDE project
   Copyright (C) 2007 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2005 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2007 Dominik Haumann <dhaumann@kde.org>

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

#ifndef _KATE_TOOLTIP_MENU_
#define _KATE_TOOLTIP_MENU_

#include <QMenu>

class QLabel;

/**
 * Menu with tooltips on the current hovered item.
 */
class KateToolTipMenu: public QMenu
{
    Q_OBJECT
  public:
    explicit KateToolTipMenu(QWidget *parent = 0);
    virtual ~KateToolTipMenu();

  protected:
    virtual bool event(QEvent*);

  private:
    QAction *m_currentAction;
    QLabel *m_toolTip;

  private Q_SLOTS:
    void slotHovered(QAction*);
    void hideToolTip();
};

#endif
// kate: space-indent on; indent-width 2; replace-tabs on;
