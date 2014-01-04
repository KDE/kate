/* This file is part of the KDE and the Kate project
 *
 *   Copyright (C) 2013 Dominik Haumann <dhaumann@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "katefadeeffect.h"
#include "katefadeeffect.moc"

#include <QWidget>
#include <QTimeLine>
#include <QGraphicsOpacityEffect>
#include <QDebug>

KateFadeEffect::KateFadeEffect(QWidget* widget)
  : QObject(widget)
  , m_widget(widget)
  , m_effect(0) // effect only exists during fading animation
{
  m_timeLine = new QTimeLine(500, this);
  m_timeLine->setUpdateInterval(40);

  connect(m_timeLine, SIGNAL(valueChanged(qreal)), this, SLOT(opacityChanged(qreal)));
  connect(m_timeLine, SIGNAL(finished()), this, SLOT(animationFinished()));
}

void KateFadeEffect::fadeIn()
{
  // stop time line if still running
  if (m_timeLine->state() == QTimeLine::Running) {
    m_timeLine->stop();
  }

  // assign new graphics effect, old one is deleted in setGraphicsEffect()
  m_effect = new QGraphicsOpacityEffect(this);
  m_effect->setOpacity(0.0);
  m_widget->setGraphicsEffect(m_effect);

  // show widget and start fade in animation
  m_widget->show();
  m_timeLine->setDirection(QTimeLine::Forward);
  m_timeLine->start();
}

void KateFadeEffect::fadeOut()
{
  // stop time line if still running
  if (m_timeLine->state() == QTimeLine::Running) {
    m_timeLine->stop();
  }

  // assign new graphics effect, old one is deleted in setGraphicsEffect()
  m_effect = new QGraphicsOpacityEffect(this);
  m_effect->setOpacity(1.0);
  m_widget->setGraphicsEffect(m_effect);

  // start fade out animation
  m_timeLine->setDirection(QTimeLine::Backward);
  m_timeLine->start();
}

void KateFadeEffect::opacityChanged(qreal value)
{
  Q_ASSERT(m_effect);
  m_effect->setOpacity(value);
}

void KateFadeEffect::animationFinished()
{
  // fading finished: remove graphics effect, deletes the effect as well
  m_widget->setGraphicsEffect(0);
  Q_ASSERT(!m_effect);

  if (m_timeLine->direction() == QTimeLine::Backward) {
    m_widget->hide();
    emit widgetHidden();
  }
}

// kate: space-indent on; indent-width 2; replace-tabs on;
