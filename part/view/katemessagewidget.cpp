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

#include <messageinterface.h>
#include <kmessagewidget.h>

#include <QtCore/QEvent>
#include <QtCore/QTimer>
#include <QtGui/QVBoxLayout>


/**
 * This class implements a message widget based on KMessageWidget.
 * It is used to show messages through the KTextEditior::MessageInterface.
 */
KateMessageWidget::KateMessageWidget(KTextEditor::Message* message, QWidget* parent)
  : QWidget(parent)
  , m_message(message)
{
  Q_ASSERT(message);

  QVBoxLayout* l = new QVBoxLayout();
  l->setMargin(0);

  m_messageWidget = new KMessageWidget();
  m_messageWidget->setText(message->text());
  m_messageWidget->setWordWrap(message->wordWrap());
  m_messageWidget->setCloseButtonVisible(false);

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

  // add all actions to the message wdiget
  foreach (QAction* a, message->actions())
    m_messageWidget->addAction(a);

  l->addWidget(m_messageWidget);
  setLayout(l);

  // tell the widget to always use the minimum size.
  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

  // install event filter so we catch the end of the hide animation
  m_messageWidget->installEventFilter(this);

  // by default, hide widgets
  m_messageWidget->hide();
  hide();
}

int KateMessageWidget::priority() const
{
  return m_message->priority();
}

KTextEditor::Message* KateMessageWidget::message()
{
  return m_message;
}

void KateMessageWidget::hideAndDeleteLater()
{
  m_message = 0;
  m_deleteLater = true;
  animatedHide();
}

void KateMessageWidget::animatedShow()
{
  if (!isVisible()) {
    show();
    m_messageWidget->animatedShow();
    //QTimer::singleShot(0, m_messageWidget, SLOT(animatedShow()));

    // start auto-hide timer, if requrested
    const int autoHide = m_message->autoHide();
    if (autoHide >= 0) {
      QTimer::singleShot(autoHide == 0 ? 5000 : autoHide, this, SLOT(animatedHide()));
    }
  }
}

void KateMessageWidget::animatedHide()
{
  if (m_messageWidget->isVisible())
    m_messageWidget->animatedHide();

  // hide this widget in eventFilter, when KMessageWidget's hide animation is done
}

bool KateMessageWidget::eventFilter(QObject *obj, QEvent *event)
{
  if (obj == m_messageWidget && event->type() == QEvent::Hide) {
    if (m_deleteLater) {
      // delete message. This triggers KTE::Message::destroyed(), which in turn
      // removes the messages widgets from all views through KateDocument::messageDestroyed()
      deleteLater();
    }
    // always hide message widget, if KMessageWidget is hidden
    hide();
  }

  return QWidget::eventFilter(obj, event);
}

// kate: space-indent on; indent-width 2; replace-tabs on;
