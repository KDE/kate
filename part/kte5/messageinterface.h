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

#include <QtCore/QObject>
#include <QtCore/QList>
#include <QtGui/QAction>

namespace KTextEditor {

class View;

/**
 * @brief This class holds a Message to display in View%s.
 *
 * \section message_intro Introduction
 *
 * The Message class holds the data used to display interactive message widgets
 * in the editor. To post a message, use the MessageInterface.
 *
 * \section message_creation Message Creation and Deletion
 * To create a new Message, use code like this:
 * \code
 * QPointer<Message> message = new Message(Message::Information, "My information text");
 * message->setWordWrap(true);
 * // ...
 * \endcode
 *
 * @see MessageInterface
 * @author Dominik Haumann \<dhaumann@kde.org\>
 * @since KDE 4.10
 */
class Message : public QObject
{
  Q_OBJECT

  //
  // public data types
  //
  public:
    /**
     * Message types used as visual indicator.
     * The message types match exactly the behavior of KMessageWidget::MessageType.
     */
    enum MessageType {
      Positive = 0, ///< positive information message
      Information,  ///< information message type
      Warning,      ///< warning message type
      Error         ///< error message type
    };

  public:
    /**
     * Constructor for new messages.
     * @param type the message type, e.g. MessageType::Information
     * @param richtext text to be displayed
     */
    Message(MessageType type, const QString& richtext);

    /**
     * Destructor.
     * Deletes all QActions that were added with addAction().
     */
    virtual ~Message();

    /**
     * Returns the text set in the constructor.
     */
    QString text() const;

    /**
     * Returns the message type set in the constructor.
     */
    MessageType messageType() const;

    /**
     * Adds an action to the message.
     *
     * By default (@p closeOnTrigger = true), the action closes the message
     * displayed in all View%s. If @p closeOnTrigger is @e false, the message
     * is stays open.
     *
     * The actions will be displayed in the order you added the actions.
     *
     * @param action action to be added
     * @param closeOnTrigger when triggered, the message widget is closed
     *
     * @warning The added actions are deleted automatically in the Message
     *          destructor, so do \em not delete the added actions yourself.
     */
    void addAction(QAction* action, bool closeOnTrigger = true);

    /**
     * Accessor to all actions, mainly used in the internal implementation
     * to add the actions into the gui.
     */
    QList<QAction*> actions() const;

    /**
     * Set the auto hide timer to @p autoHideTimer milliseconds.
     * If @p autoHideTimer < 0, auto hide is disabled.
     * If @p autoHideTimer = 0, auto hide is enabled and set to a default
     * value of several seconds.
     *
     * @see autoHide()
     */
    void setAutoHide(int autoHideTimer = 0);

    /**
     * Returns the auto hide time in milliseconds.
     * Please refer to setAutoHide() for an explanation of the return value.
     *
     * @see setAutoHide()
     */
    int autoHide() const;

    /**
     * Enabled word wrap according to @p wordWrap.
     *
     * @see wordWrap()
     */
    void setWordWrap(bool wordWrap);

    /**
     * Check, whether word wrap is enabled or not.
     *
     * @see setWordWrap()
     */
    bool wordWrap() const;

    /**
     * Set the priority of this message to @p priority.
     * Messages with higher priority are shown first.
     * The default priority is 0.
     *
     * @see priority()
     */
    void setPriority(int priority);

    /**
     * Returns the priority of the message. Default is 0.
     *
     * @see setPriority()
     */
    int priority() const;

    /**
     * Set the associated view of the message.
     * If @p view is 0, the message is shown in all View%s of the Document.
     * If @p view is given, i.e. non-zero, the message is shown only in this view.
     */
    void setView(KTextEditor::View* view);

    /**
     * This function returns the view the message was posted for.
     * This can be either be specified by setView() or will be automatically
     * set when the message is processed, see MessageInterface::messageProcessed().
     * Therefore, the return value may be 0, if you did not set a valid
     * view first.
     */
    KTextEditor::View* view() const;

  Q_SIGNALS:
    /**
     * This signal is emitted before the message is deleted.
     * Afterwards, this pointer is invalid.
     * @param message closed message
     *
     * @warning \em Never delete a message yourself!
     */
    void closed(Message* message);

private:
    class MessagePrivate * const d;
};

/**
 * \brief Message interface for posting interactive Message%s to a Document and its View%s.
 *
 * \ingroup kte_group_document_extension
 *
 * This interface allows to post Message%s to a Document. The Message then
 * is shown either the specified View, or in all View%s of the Document.
 *
 * \section message_interface Working with Message%s
 *
 * To post a message, you first have to cast the Document to this interface,
 * and then create a Message. Example:
 * \code
 * // doc is of type KTextEditor::Document*
 * KTextEditor::MessageInterface *iface =
 *     qobject_cast<KTextEditor::MessageInterface*>( doc );
 *
 * if( !iface ) {
 *     // the implementation does not support the interface
 *     return;
 * }
 *
 * QPointer<Message> message = new Message(Message::Information, "text");
 * message->setWordWrap(true);
 * message->addAction(...); // add your actions...
 * iface->postMessage(message);
 *
 * // To remove a message, just delete it (provided you use a QPointer!)
 * delete message;
 * \endcode
 *
 * @see Message
 * @author Dominik Haumann \<dhaumann@kde.org\>
 * @since KDE 4.10
 */
class MessageInterface
{
  public:
    /**
     * Default constructor, for internal use.
     */
    MessageInterface();
    /**
     * Destructor, for internal use.
     */
    virtual ~MessageInterface();

    /**
     * Post @p message to the Document and its View%s.
     * If multiple Message%s are posted, the one with the highest priority
     * is shown first.
     * @param message the message to show
     */
    virtual void postMessage(Message* message) = 0;

  private:
    class MessageInterfacePrivate * const d;
};

}

Q_DECLARE_INTERFACE(KTextEditor::MessageInterface, "org.kde.KTextEditor.MessageInterface")

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
