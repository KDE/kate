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

#include "katemessagewidget.h"
#include "katemessagewidget.moc"
#include "katefadeeffect.h"

#include <messageinterface.h>
#include <kmessagewidget.h>

#include <kdeversion.h>

#include <QtCore/QEvent>
#include <QtCore/QTimer>
#include <QtGui/QVBoxLayout>

KateMessageWidget::KateMessageWidget(QWidget* parent, bool applyFadeEffect)
  : QWidget(parent)
  , m_fadeEffect(0)
  , m_hideAnimationRunning(false)
  , m_autoHideTimerRunning(false)
  , m_autoHideTime(-1)
{
  QVBoxLayout* l = new QVBoxLayout();
  l->setMargin(0);

  m_messageWidget = new KMessageWidget(this);
  m_messageWidget->setCloseButtonVisible(false);

  l->addWidget(m_messageWidget);
  setLayout(l);

  // tell the widget to always use the minimum size.
  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

  // install event filter so we catch the end of the hide animation
  m_messageWidget->installEventFilter(this);

  // by default, hide widgets
  m_messageWidget->hide();
  hide();

  if (applyFadeEffect) {
    m_fadeEffect = new KateFadeEffect(m_messageWidget);
  }
}

bool KateMessageWidget::eventFilter(QObject *obj, QEvent *event)
{
  if (obj == m_messageWidget && event->type() == QEvent::Hide && !event->spontaneous()) {

    // hide animation is finished
    m_hideAnimationRunning = false;
    m_autoHideTimerRunning = false;
    m_autoHideTime = -1;

    // if there are other messages in the queue, show next one, else hide us
    if (m_messageList.count()) {
      showMessage(m_messageList[0]);
    } else {
      hide();
    }
  }

  return QWidget::eventFilter(obj, event);
}

void KateMessageWidget::showMessage(KTextEditor::Message* message)
{
  // set text etc.
  m_messageWidget->setText(message->text());
  m_messageWidget->setWordWrap(message->wordWrap());

  // connect textChanged(), so it's possible to change text on the fly
  connect(message, SIGNAL(textChanged(const QString&)),
          m_messageWidget, SLOT(setText(const QString&)));

  // the enums values do not necessarily match, hence translate with switch
  switch (message->messageType()) {
    case KTextEditor::Message::Positive:
      m_messageWidget->setMessageType(KMessageWidget::Positive);
      break;
    case KTextEditor::Message::Information:
      m_messageWidget->setMessageType(KMessageWidget::Information);
      break;
    case KTextEditor::Message::Warning:
      m_messageWidget->setMessageType(KMessageWidget::Warning);
      break;
    case KTextEditor::Message::Error:
      m_messageWidget->setMessageType(KMessageWidget::Error);
      break;
    default:
      m_messageWidget->setMessageType(KMessageWidget::Information);
      break;
  }

  // remove all actions from the message widget
  foreach (QAction* a, m_messageWidget->actions())
    m_messageWidget->removeAction(a);

  // add new actions to the message widget
  foreach (QAction* a, message->actions())
    m_messageWidget->addAction(a);

  // start auto-hide timer, if requested
  m_autoHideTime = message->autoHide();
  m_autoHideTimerRunning = false;

  if (m_autoHideTime >= 0 && message->autoHideMode() == KTextEditor::Message::Immediate) {
    QTimer::singleShot(m_autoHideTime == 0 ? (6*1000) : m_autoHideTime, message, SLOT(deleteLater()));
    m_autoHideTimerRunning = true;
  }

  // finally show us
  show();
  if (m_fadeEffect) {
    m_fadeEffect->fadeIn();
  } else {
#if KDE_VERSION >= KDE_MAKE_VERSION(4,10,0)   // work around KMessageWidget bugs
    m_messageWidget->animatedShow();
#else
    QTimer::singleShot(0, m_messageWidget, SLOT(animatedShow()));
#endif
  }
}

void KateMessageWidget::postMessage(KTextEditor::Message* message,
                           QList<QSharedPointer<QAction> > actions)
{
  Q_ASSERT(!m_messageHash.contains(message));
  m_messageHash[message] = actions;

  // insert message sorted after priority
  int i = 0;
  for (; i < m_messageList.count(); ++i) {
    if (message->priority() > m_messageList[i]->priority())
      break;
  }

  // queue message
  m_messageList.insert(i, message);

  if (i == 0 && !m_hideAnimationRunning) {
    // if message has higher priority than the one currently shown,
    // then hide the current one and then show the new one.
    if (m_messageWidget->isVisible()) {
      m_hideAnimationRunning = true;
      if (m_fadeEffect) {
        m_fadeEffect->fadeOut();
      } else {
        m_messageWidget->animatedHide();
      }
    } else {
      showMessage(m_messageList[0]);
    }
  }

  // catch if the message gets deleted
  connect(message, SIGNAL(closed(KTextEditor::Message*)), SLOT(messageDestroyed(KTextEditor::Message*)));
}

void KateMessageWidget::messageDestroyed(KTextEditor::Message* message)
{
  // last moment when message is valid, since KTE::Message is already in
  // destructor we have to do the following:
  // 1. remove message from m_messageList, so we don't care about it anymore
  // 2. activate hide animation or show a new message()

  // remove widget from m_messageList
  int i = 0;
  for (; i < m_messageList.count(); ++i) {
    if (m_messageList[i] == message) {
      break;
    }
  }

  // the message must be in the list
  Q_ASSERT(i < m_messageList.count());

  // remove message
  m_messageList.removeAt(i);

  // remove message from hash -> release QActions
  Q_ASSERT(m_messageHash.contains(message));
  m_messageHash.remove(message);

  // start hide animation, or show next message
  if (m_messageWidget->isVisible()) {
    m_hideAnimationRunning = true;
    if (m_fadeEffect) {
      m_fadeEffect->fadeOut();
    } else {
      m_messageWidget->animatedHide();
    }
  } else if (i == 0 && m_messageList.count()) {
    showMessage(m_messageList[0]);
  }
}

void KateMessageWidget::startAutoHideTimer()
{
  // message does not want autohide, or timer already running
  if (!isVisible()            // not visible, no message shown
    || m_autoHideTime < 0     // message does not want auto-hide
    || m_autoHideTimerRunning // auto-hide timer is already active
    || m_hideAnimationRunning // widget is in hide animation phase
  ) {
    return;
  }

  // switching KateViews may result isVisible() == true and still m_messageList.size() == 0.
  // The problem is that the hideEvent is never called for the KMessageWidget, if the
  // parent widget is hidden. In that case, we 'miss' that the notification is gone...
  if (m_messageList.size() == 0) {
    m_hideAnimationRunning = false;
    m_autoHideTimerRunning = false;
    m_autoHideTime = -1;

    if (isVisible()) {
      m_hideAnimationRunning = true;
      if (m_fadeEffect) {
        m_fadeEffect->fadeOut();
      } else {
        m_messageWidget->animatedHide();
      }
    }
    return;
  }

  // remember that auto hide timer is running
  m_autoHideTimerRunning = true;

  KTextEditor::Message* message = m_messageList[0];
  QTimer::singleShot(m_autoHideTime == 0 ? (6*1000) : m_autoHideTime, message, SLOT(deleteLater()));
}


// kate: space-indent on; indent-width 2; replace-tabs on;
