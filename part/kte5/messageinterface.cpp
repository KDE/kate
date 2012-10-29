/* This file is part of the KDE project
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

#include "messageinterface.h"

namespace KTextEditor {

class MessagePrivate
{
  public:
    QVector<QAction*> actions;
    MessageType messageType;
    QString text;
    bool closeButtonVisible : 1;
    bool wordWrap : 1;
    int autoHide;
    int priority;
    KTextEditor::View* view;
};

Message::Message()
  : d(new MessagePrivate())
{
  d->messageType = Information;
  d->closeButtonVisible = true;
  d->wordWrap = false;
  d->autoHide = 0;
  d->priority = 0;
  d->view = 0;
}

Message::~Message()
{
  qDeleteAll(d->actions);
  delete d;
}

const QString& Message::text() const
{
  return d->text;
}

MessageType Message::messageType() const
{
  return d->type;
}

void Message::addAction(QAction* action)
{
  d->actions.append(action);
}

const QVector<QAction*>& actions() const
{
  return d->actions;
}

void Message::setAutoHide(int autoHideTimer = 5000)
{
  d->autoHide = autoHideTimer;
}

int Message::autoHide() const
{
  return d->autoHide;
}

void Message::setWordWrap(bool wordWrap)
{
  d->wordWrap = wordWrap;
}

bool Message::wordWrap() const
{
  return d->wordWrap;
}

bool Message::isCloseButtonVisible() const
{
  return d->closeButtonVisible;
}

void Message::setCloseButtonVisible(bool visible)
{
  d->closeButtonVisible = visible;
}

void Message::setPriority(int priority)
{
  d->priority = priority;
}

int Message::priority() const
{
  return d->priority;
}

void Message::setView(KTextEditor::View* view)
{
  d->view = view;
}

KTextEditor::View* Message::view() const
{
  return d->view;
}



MessageInterface::MessageInterface()
  : d (0)
{
}

MessageInterface::~MessageInterface()
{
}

}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
