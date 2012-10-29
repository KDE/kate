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

class MessageDataPrivate
{
  public:
    QVector<QAction*> actions;
    MessageType messageType;
    QString text;
    bool closeButtonVisible : 1;
    bool wordWrap : 1;
    int autoHide; // in milli seconds, < 0 means disable
    int priority;
    KTextEditor::View* view;
};

MessageData::MessageData()
  : d(new MessageDataPrivate())
{
  d->messageType = Information;
  d->closeButtonVisible = true;
  d->wordWrap = false;
  d->autoHide = 0;
  d->priority = 0;
  d->view = 0;
}

MessageData::~MessageData()
{
  qDeleteAll(d->actions);
  delete d;
}

const QString& MessageData::text() const
{
  return d->text;
}

MessageType MessageData::messageType() const
{
  return d->type;
}

void MessageData::addAction(QAction* action)
{
  d->actions.append(action);
}

const QVector<QAction*>& actions() const
{
  return d->actions;
}

void MessageData::setAutoHide(int autoHideTimer = 5000)
{
  d->autoHide = autoHideTimer;
}

int MessageData::autoHide() const
{
  return d->autoHide;
}

void MessageData::setWordWrap(bool wordWrap)
{
  d->wordWrap = wordWrap;
}

bool MessageData::wordWrap() const
{
  return d->wordWrap;
}

bool MessageData::isCloseButtonVisible() const
{
  return d->closeButtonVisible;
}

void MessageData::setCloseButtonVisible(bool visible)
{
  d->closeButtonVisible = visible;
}

void MessageData::setPriority(int priority)
{
  d->priority = priority;
}

int MessageData::priority() const
{
  return d->priority;
}

void MessageData::setView(KTextEditor::View* view)
{
  d->view = view;
}

KTextEditor::View* MessageData::view() const
{
  return d->view;
}

}

Q_DECLARE_INTERFACE(KTextEditor::MovingInterface, "org.kde.KTextEditor.MovingInterface")

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
