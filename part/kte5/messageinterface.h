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

#ifndef KTEXTEDITOR_MESSAGEINTERFACE_H
#define KTEXTEDITOR_MESSAGEINTERFACE_H

namespace KTextEditor {

class MessageData
{
  public:
    enum MessageType {
      Positive = 0,
      Information,
      Warning,
      Error
    };

    MessageData(MessageType type, const QString& richtext);
    ~MessageData();

    QString text() const;
    MessageType messageType() const;

    void addAction(QAction* action);
    const QVector<QAction*>& actions() const;

    void setAutoHide(int ms = 5000);
    int autoHide() const;

    void setWordWrap(bool wordWrap);
    bool wordWrap() const;

    bool isCloseButtonVisible() const;
    void setCloseButtonVisible(bool visible);

    void setPriority(int priority);
    int priority() const;

    void setView(KTextEditor::View* view);
    KTextEditor::View* view() const;

private:
    class MessageDataPrivate * const d;
    Q_DISABLE_COPY(MessageData)
};

typedef QSharedPointer<MessageData> Message;

class MessageInterface
{
  public:
    MessageInterface();
    virtual ~MessageInterface();

    void removeMessage(Message message) = 0;
    bool isPending(Message message);

    Q_SIGNALS:
      void messageProcessed(Message* message)
};

}

Q_DECLARE_INTERFACE(KTextEditor::MovingInterface, "org.kde.KTextEditor.MovingInterface")

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
