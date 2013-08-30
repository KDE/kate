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

#include <ktexteditor/messageinterface.h>
#include <kmessagewidget.h>
#include <kateanimation.h>

#include <kdeversion.h>
#include <kdebug.h>

#include <QtCore/QEvent>
#include <QtCore/QTimer>
#include <QtGui/QVBoxLayout>
#include <QToolTip>
#include <QShowEvent>

static const int s_defaultAutoHideTime = 6 * 1000;

KateMessageWidget::KateMessageWidget(QWidget* parent, bool applyFadeEffect)
  : QWidget(parent)
  , m_animation(0)
  , m_autoHideTimer(new QTimer(this))
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

  // create animation controller, and connect widgetHidden() to showNextMessage()
  m_animation = new KateAnimation(m_messageWidget, applyFadeEffect ? KateAnimation::FadeEffect : KateAnimation::GrowEffect);
  connect(m_animation, SIGNAL(widgetHidden()), this, SLOT(showNextMessage()));

  // setup autoHide timer details
  m_autoHideTimer->setSingleShot(true);

#if KDE_IS_VERSION(4,10,60) // KMessageWidget::linkHovered() is new in KDE 4.11
  connect(m_messageWidget, SIGNAL(linkHovered(const QString&)), SLOT(linkHovered(const QString&))); // FIXME: TODO: this?
#endif
}

void KateMessageWidget::showNextMessage()
{
  // at this point, we should not have a currently shown message
  Q_ASSERT(m_currentMessage == 0);

  // if not message to show, just stop
  if (m_messageQueue.size() == 0) {
    hide();
    return;
  }

  // track current message
  m_currentMessage = m_messageQueue[0];

  // set text etc.
  m_messageWidget->setText(m_currentMessage->text());
#if KDE_VERSION >= KDE_MAKE_VERSION(4,10,60)   // KMessageWidget::setIcon() requires KDE >= 4.11
  m_messageWidget->setIcon(m_currentMessage->icon());
#endif

  // connect textChanged() and iconChanged(), so it's possible to change this on the fly
  connect(m_currentMessage, SIGNAL(textChanged(const QString&)),
          m_messageWidget, SLOT(setText(const QString&)), Qt::UniqueConnection);
  connect(m_currentMessage, SIGNAL(iconChanged(const QIcon&)),
          m_messageWidget, SLOT(setIcon(const QIcon&)), Qt::UniqueConnection);

  // the enums values do not necessarily match, hence translate with switch
  switch (m_currentMessage->messageType()) {
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
  foreach (QAction* a, m_currentMessage->actions())
    m_messageWidget->addAction(a);

  // set word wrap of the message
  setWordWrap(m_currentMessage);

  // setup auto-hide timer, and start if requested
  m_autoHideTime = m_currentMessage->autoHide();
  m_autoHideTimer->stop();
  if (m_autoHideTime >= 0) {
    connect(m_autoHideTimer, SIGNAL(timeout()), m_currentMessage, SLOT(deleteLater()), Qt::UniqueConnection);
    if (m_currentMessage->autoHideMode() == KTextEditor::Message::Immediate) {
      m_autoHideTimer->start(m_autoHideTime == 0 ? s_defaultAutoHideTime : m_autoHideTime);
    }
  }

  // finally show
  show();
  m_animation->show();
}

void KateMessageWidget::setWordWrap(KTextEditor::Message* message)
{
  // want word wrap anyway? -> ok
  if (message->wordWrap()) {
    m_messageWidget->setWordWrap(message->wordWrap());
    return;
  }

  // word wrap not wanted, that's ok if a parent widget does not exist
  if (!parentWidget()) {
    m_messageWidget->setWordWrap(false);
    return;
  }

  // word wrap not wanted -> enable word wrap if it breaks the layout otherwise
  int margin = 0;
  if (parentWidget()->layout()) {
    // get left/right margin of the layout, since we need to subtract these
    int leftMargin = 0, rightMargin = 0;
    parentWidget()->layout()->getContentsMargins(&leftMargin, 0, &rightMargin, 0);
    margin = leftMargin + rightMargin;
  }

  // if word wrap enabled, first disable it
  if (m_messageWidget->wordWrap())
    m_messageWidget->setWordWrap(false);

  // make sure the widget's size is up-to-date in its hidden state
  m_messageWidget->ensurePolished();
  m_messageWidget->adjustSize();

  // finally enable word wrap, if there is not enough free horizontal space
  const int freeSpace = (parentWidget()->width() - margin) - m_messageWidget->width();
  if (freeSpace < 0) {
//     kDebug() << "force word wrap to avoid breaking the layout" << freeSpace;
    m_messageWidget->setWordWrap(true);
  }
}

void KateMessageWidget::postMessage(KTextEditor::Message* message,
                           QList<QSharedPointer<QAction> > actions)
{
  Q_ASSERT(!m_messageHash.contains(message));
  m_messageHash[message] = actions;

  // insert message sorted after priority
  int i = 0;
  for (; i < m_messageQueue.count(); ++i) {
    if (message->priority() > m_messageQueue[i]->priority())
      break;
  }

  // queue message
  m_messageQueue.insert(i, message);

  // catch if the message gets deleted
  connect(message, SIGNAL(closed(KTextEditor::Message*)), SLOT(messageDestroyed(KTextEditor::Message*)));

  if (i == 0 && !m_animation->hideAnimationActive()) {
    // if message has higher priority than the one currently shown,
    // then hide the current one and then show the new one.
    if (m_currentMessage) {

      // autoHide timer may be running for currently shown message, therefore
      // simply disconnect autoHide timer to all timeout() receivers
      disconnect(m_autoHideTimer, SIGNAL(timeout()), 0, 0);
      m_autoHideTimer->stop();

      // if there is a current message, the message queue must contain 2 messages
      Q_ASSERT(m_messageQueue.size() > 1);
      Q_ASSERT(m_currentMessage == m_messageQueue[1]);

      // a bit unnice: disconnect textChanged() and iconChanged() signals of previously visible message
      disconnect(m_currentMessage, SIGNAL(textChanged(const QString&)),
                 m_messageWidget, SLOT(setText(const QString&)));
      disconnect(m_currentMessage, SIGNAL(iconChanged(const QIcon&)),
                 m_messageWidget, SLOT(setIcon(const QIcon&)));

      m_currentMessage = 0;
      m_animation->hide();
    } else {
      showNextMessage();
    }
  }
}

void KateMessageWidget::messageDestroyed(KTextEditor::Message* message)
{
  // last moment when message is valid, since KTE::Message is already in
  // destructor we have to do the following:
  // 1. remove message from m_messageQueue, so we don't care about it anymore
  // 2. activate hide animation or show a new message()

  // remove widget from m_messageQueue
  int i = 0;
  for (; i < m_messageQueue.count(); ++i) {
    if (m_messageQueue[i] == message) {
      break;
    }
  }

  // the message must be in the list
  Q_ASSERT(i < m_messageQueue.count());

  // remove message
  m_messageQueue.removeAt(i);

  // remove message from hash -> release QActions
  Q_ASSERT(m_messageHash.contains(message));
  m_messageHash.remove(message);

  // if deleted message is the current message, launch hide animation
  if (message == m_currentMessage) {
    m_currentMessage = 0;
    m_animation->hide();
  }
}

void KateMessageWidget::startAutoHideTimer()
{
  // message does not want autohide, or timer already running
  if ( !m_currentMessage           // no message, nothing to do
    || m_autoHideTime < 0          // message does not want auto-hide
    || m_autoHideTimer->isActive() // auto-hide timer is already active
    || m_animation->hideAnimationActive() // widget is in hide animation phase
    || m_animation->showAnimationActive() // widget is in show animation phase
  ) {
    return;
  }

  // safety checks: the message must still still be valid
  Q_ASSERT(m_messageQueue.size());
  Q_ASSERT(m_currentMessage->autoHide() == m_autoHideTime);

  // start autoHide timer as requrested
  m_autoHideTimer->start(m_autoHideTime == 0 ? s_defaultAutoHideTime : m_autoHideTime);
}

void KateMessageWidget::linkHovered(const QString& link)
{
  QToolTip::showText(QCursor::pos(), link, m_messageWidget);
}

QString KateMessageWidget::text() const
{
  return m_messageWidget->text();
}
// kate: space-indent on; indent-width 2; replace-tabs on;
