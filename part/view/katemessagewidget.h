/* This file is part of the KDE and the Kate project
 *
 *   Copyright (C) 2012 Dominik Haumann <dhaumann@kde.org>
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

#ifndef KATE_MESSAGE_WIDGET_H
#define KATE_MESSAGE_WIDGET_H

#include <QtGui/QWidget>
#include <QtCore/QHash>

namespace KTextEditor
{
  class Message;
}

class KMessageWidget;
class KateFadeEffect;

/**
 * This class implements a message widget based on KMessageWidget.
 * It is used to show messages through the KTextEditior::MessageInterface.
 */
class KateMessageWidget : public QWidget
{
  Q_OBJECT

  public:
    /**
     * Constructor. By default, the widget is hidden.
     */
    KateMessageWidget(QWidget* parent, bool applyFadeEffect = false);

    /**
     * Post a new incoming message. Show either directly, or queue
     */
    void postMessage(KTextEditor::Message* message, QList<QSharedPointer<QAction> > actions);

  protected:
    /**
     * Show the @p message and launch show animation
     */
    void showMessage(KTextEditor::Message* message);

  protected:
    /**
     * Event filter, needed to catch the end of the hide animation of the KMessageWidget.
     */
    virtual bool eventFilter(QObject *obj, QEvent *event);

  private Q_SLOTS:
    /**
     * catch when a message is deleted, then show next one, if applicable.
     */
    void messageDestroyed(KTextEditor::Message* message);
    /**
     * Start autoHide timer if requested
     */
    void startAutoHideTimer();

  private:
    // sorted list of pending messages
    QList<KTextEditor::Message*> m_messageList;
    // shared pointers to QActions as guard
    QHash<KTextEditor::Message*, QList<QSharedPointer<QAction> > > m_messageHash;
    // the message widget, showing the actual contents
    KMessageWidget* m_messageWidget;
    // the fade effect to show/hide the widget, if wanted
    KateFadeEffect* m_fadeEffect;

  private: // some state variables
    // flag: hide animation is running. needed to avoid flickering
    // when showMessage() is called during hide-animation
    bool m_hideAnimationRunning : 1;
    // flag: start autohide only once user interaction took place
    bool m_autoHideTimerRunning : 1;
    // flag: save message's autohide time
    int m_autoHideTime;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
