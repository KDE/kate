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
#include <messageinterface.h>
#include <kmessagewidget.h>

#include <QtCore/QEvent>
#include <QtCore/QTimer>
#include <QtGui/QVBoxLayout>
#include <QtGui/QShowEvent>

// TODO
// KTextEditor::View* view() const;
//
// // TODO: set the correct document somewhere
// void setDocument(KTextEditor::Document* document);




/**
 * This class implements a message widget based on KMessageWidget.
 * It is used to show messages through the KTextEditior::MessageInterface.
 */
KateMessageWidget::KateMessageWidget(KTextEditor::Message* message, QWidget * parent)
  : QWidget(parent)
  , m_message(message)
{
  Q_ASSERT(message);

  QVBoxLayout* l = new QVBoxLayout();

  m_messageWidget = new KMessageWidget();
  m_messageWidget->setText(message->text());
  m_messageWidget->setWordWrap(message->wordWrap());

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

  foreach (QAction* a, message->actions())
    m_messageWidget->addAction(a);

  connect(message, SIGNAL(closed(Message*)), m_messageWidget, SLOT(animatedHide()));

  l->addWidget(m_messageWidget);
  setLayout(l);

  m_messageWidget->installEventFilter(this);

  m_messageWidget->hide();
}

int KateMessageWidget::priority() const
{
  return m_message->priority();
}

bool KateMessageWidget::eventFilter(QObject *obj, QEvent *event)
{
  if (obj == m_messageWidget && event->type() == QEvent::Hide) {
    deleteLater();
  }

  return QWidget::eventFilter(obj, event);
}

void KateMessageWidget::showEvent(QShowEvent *event)
{
  if (!event->spontaneous()) {
    m_messageWidget->animatedShow();

    // enable auto hide if wanted
    const int autoHide = m_message->autoHide();
    if (autoHide >= 0) {
      QTimer::singleShot(autoHide == 0 ? 5000 : autoHide, m_messageWidget, SLOT(animatedHide()));
    }
  }

  QWidget::showEvent(event);
}

// kate: space-indent on; indent-width 2; replace-tabs on;
