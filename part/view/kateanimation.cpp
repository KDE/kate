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

#include "kateanimation.h"
#include "kateanimation.moc"
#include <katefadeeffect.h>

#include <KMessageWidget>
#include <QTimer>
#include <kglobalsettings.h>

KateAnimation::KateAnimation(KMessageWidget* widget, EffectType effect)
  : QObject(widget)
  , m_widget(widget)
  , m_fadeEffect(0)
{
  Q_ASSERT(m_widget != 0);

  // create wanted effect
  if (effect == FadeEffect) {
    m_fadeEffect = new KateFadeEffect(widget);
  }

  // create tracking timer for hiding the widget
  m_hideTimer = new QTimer(this);
  m_hideTimer->setInterval(550); // 500 from KMessageWidget + 50 to make sure the hide animation is really finished
  m_hideTimer->setSingleShot(true);
  connect(m_hideTimer, SIGNAL(timeout()), this, SIGNAL(widgetHidden()));

  // create tracking timer for showing the widget
  m_showTimer = new QTimer(this);
  m_showTimer->setInterval(550); // 500 from KMessageWidget + 50 to make sure the show animation is really finished
  m_showTimer->setSingleShot(true);
  connect(m_showTimer, SIGNAL(timeout()), this, SIGNAL(widgetShown()));
}

bool KateAnimation::hideAnimationActive() const
{
  return m_hideTimer->isActive();
}

bool KateAnimation::showAnimationActive() const
{
  return m_showTimer->isActive();
}

void KateAnimation::show()
{
  Q_ASSERT(m_widget != 0);

  // stop hide timer if needed
  if (m_hideTimer->isActive()) {
    m_hideTimer->stop();
  }

  // show according to effects config
  if (!(KGlobalSettings::graphicEffectsLevel() & KGlobalSettings::SimpleAnimationEffects)) {
    m_widget->show();
    emit widgetShown();
  } else {
    // launch show effect
    // NOTE: use a singleShot timer to avoid resizing issues when showing the message widget the first time (bug #316666)
    if (m_fadeEffect) {
      QTimer::singleShot(0, m_fadeEffect, SLOT(fadeIn()));
    } else {
      QTimer::singleShot(0, m_widget, SLOT(animatedShow()));
    }

    // start timer in order to track when showing is done (this effectively works
    // around the fact, that KMessageWidget does not have a hidden signal)
    m_showTimer->start();
  }
}

void KateAnimation::hide()
{
  Q_ASSERT(m_widget != 0);

  // stop show timer if needed
  if (m_showTimer->isActive()) {
    m_showTimer->stop();
  }
  
  // hide according to effects config
  if (!(KGlobalSettings::graphicEffectsLevel() & KGlobalSettings::SimpleAnimationEffects)) {
    m_widget->hide();
    emit widgetHidden();
  } else {
    // hide depending on effect
    if (m_fadeEffect) {
      m_fadeEffect->fadeOut();
    } else {
      m_widget->animatedHide();
    }

    // start timer in order to track when hiding is done (this effectively works
    // around the fact, that KMessageWidget does not have a hidden signal)
    m_hideTimer->start();
  }
}

// kate: space-indent on; indent-width 2; replace-tabs on;
