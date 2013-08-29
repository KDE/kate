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

#ifndef KATE_ANIMATION_H
#define KATE_ANIMATION_H

#include <QObject>
#include <QPointer>

class QTimer;

class KMessageWidget;
class KateFadeEffect;
/**
 * This class provides a fade in/out effect for KMessageWidget%s.
 * Example:
 * \code
 * KateAnimation* animation = new KateAnimation(someMessageWidget);
 * animation->show();
 * //...
 * animation->hide();
 * \endcode
 */
class KateAnimation : public QObject
{
  Q_OBJECT

  public:
    /**
     * The type of supported animation effects
     */
    enum EffectType{
      FadeEffect = 0, ///< fade in/out
      GrowEffect      ///< grow / shrink
    };

  public:
    /**
     * Constructor.
     */
    KateAnimation(KMessageWidget* widget, EffectType effect);

    /**
     * Returns true, if the hide animation is running, otherwise false.
     */
    bool hideAnimationActive() const;

    /**
     * Returns true, if the how animation is running, otherwise false.
     */
    bool showAnimationActive() const;

  public Q_SLOTS:
    /**
     * Call to hide the widget.
     */
    void hide();

    /**
     * Call to show and fade in the widget
     */
    void show();

  Q_SIGNALS:
    /**
     * This signal is emitted when the hiding animation is finished.
     * At this point, the associated widget is hidden.
     */
    void widgetHidden();

    /**
     * This signal is emitted when the showing animation is finished.
     * At this point, the associated widget is hidden.
     */
    void widgetShown();

  private:
    QPointer<KMessageWidget> m_widget; ///< the widget to animate
    KateFadeEffect * m_fadeEffect;     ///< the fade effect
    QTimer * m_hideTimer;              ///< timer to track hide animation
    QTimer * m_showTimer;              ///< timer to track show animation
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
