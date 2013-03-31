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

static const int frameRange = 40;

KateFadeEffect::KateFadeEffect(QWidget* widget)
  : QObject(widget)
  , m_widget(widget)
{
  m_timeLine = new QTimeLine(10000, this);
  m_timeLine->setFrameRange(0, frameRange);
  m_effect = new QGraphicsOpacityEffect(this);
  m_effect->setOpacity(1.0);
  m_widget->setGraphicsEffect(m_effect);

  connect(m_timeLine, SIGNAL(valueChanged(qreal)), this, SLOT(opacityChanged(qreal)));
  connect(m_timeLine, SIGNAL(finished()), this, SLOT(animationFinished()));
}

void KateFadeEffect::fadeIn()
{
  if (!m_widget || m_widget->isVisible()) {
    return;
  }

  m_timeLine->setDirection(QTimeLine::Forward);
  m_effect->setOpacity(0.0);
  m_widget->show();
  m_timeLine->start();
}

void KateFadeEffect::fadeOut()
{
  if (!m_widget || !m_widget->isVisible()) {
    return;
  }

  m_effect->setOpacity(1.0);
  m_timeLine->setDirection(QTimeLine::Backward);
  m_timeLine->start();
}

void KateFadeEffect::opacityChanged(qreal value)
{
  if (m_widget) {
    m_effect->setOpacity(value);
  }
}

void KateFadeEffect::animationFinished()
{
  if (!m_widget) {
    return;
  }

  if (m_timeLine->direction() == QTimeLine::Backward) {
    m_widget->hide();
  }
}

// kate: space-indent on; indent-width 2; replace-tabs on;
