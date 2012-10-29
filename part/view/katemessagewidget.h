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

namespace KTextEditor
{
  class Message;
}

class KMessageWidget;

/**
 * This class implements a message widget based on KMessageWidget.
 * It is used to show messages through the KTextEditior::MessageInterface.
 */
class KateMessageWidget : public QWidget
{
  Q_OBJECT

  public:
    KateMessageWidget(KTextEditor::Message* message, QWidget * parent = 0);

    int priority() const;

  protected:
    virtual bool eventFilter(QObject *obj, QEvent *event);
    virtual void showEvent(QShowEvent *event);

  private:
    KMessageWidget* m_messageWidget;
    KTextEditor::Message* m_message;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
