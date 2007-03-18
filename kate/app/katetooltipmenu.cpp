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

#include "katetooltipmenu.h"
#include "katetooltipmenu.moc"

#include <QEvent>
#include <QLabel>
#include <QToolTip>
#include <QApplication>
#include <QDesktopWidget>
#include <kdebug.h>

KateToolTipMenu::KateToolTipMenu(QWidget *parent)
  : QMenu(parent), m_currentAction(0), m_toolTip(0)
{
  connect(this, SIGNAL(hovered(QAction*)), this, SLOT(slotHovered(QAction*)));
  connect(this, SIGNAL(aboutToHide()), this, SLOT(hideToolTip()));
}

KateToolTipMenu::~KateToolTipMenu()
{}

bool KateToolTipMenu::event(QEvent* event)
{
#if 0
  if (event->type() == QEvent::ToolTip)
  {
    if (m_currentAction)
    {

      //tmp->setSizePolicy(QSizePolicy(QSizePolicy::Preferred,QSizePolicy::Preferred));

      event->setAccepted(true);
      return true;
    }
  }
#endif
  return QMenu::event(event);
}

void KateToolTipMenu::slotHovered(QAction* a)
{
  m_currentAction = a;
  if (!a)
  {
    delete m_toolTip;
    m_toolTip = 0;
    return;
  }
  if (!m_toolTip)
  {
    m_toolTip = new QLabel(this, Qt::ToolTip);
    m_toolTip->setFrameStyle(QFrame::Box | QFrame::Plain);
  }
  m_toolTip->setText(m_currentAction->toolTip());

  QRect fg = actionGeometry(a);
  fg.moveTo(frameGeometry().topLeft() + fg.topLeft());
  m_toolTip->ensurePolished();
  m_toolTip->resize(m_toolTip->sizeHint());
  m_toolTip->setPalette(QToolTip::palette());

  QRect fgl = m_toolTip->frameGeometry();
  QRect dr = QApplication::desktop()->availableGeometry(this);
  int posx;
  if ( (fg.right() + fgl.width() + 1) > dr.right())
    posx = fg.left() - m_toolTip->width();
  else
    posx = fg.right() + 1;
  int posy;
  if ((fg.top() + fgl.height()) > dr.bottom())
    posy = fg.bottom() - fgl.height();
  else
    posy = fg.top();

  m_toolTip->move(posx, posy);
  m_toolTip->show();
}

void KateToolTipMenu::hideToolTip()
{
  delete m_toolTip;
  m_toolTip = 0;
}

// kate: space-indent on; indent-width 2; replace-tabs on;
