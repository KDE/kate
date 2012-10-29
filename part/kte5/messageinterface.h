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

/**
 * @brief This class holds the Message data used to display messages in View%s.
 *
 * \section message_intro Introduction
 *
 * The Message class holds the data used to display interactive message widgets
 * in the editor. To post a message, use the MessageInterface.
 *
 * \section message_creation Message Creation and Deletion
 * To create a new Message, use code like this:
 * \code
 * Message::Ptr message(new MessageData(Message::Information, "My information text"));
 * message->setWordWrap(true);
 * // ...
 * \endcode
 * Message%s are used through the shared pointer Message::Ptr. Thus, never
 * delete a message. Messages%s are ref-counted and thus deleted automatically
 * when no Message::Ptr instance exists anymore.
 *
 * @see MessageInterface
 * @author Dominik Haumann \<dhaumann@kde.org\>
 * @since KDE 4.10
 */
class Message
{
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

   /**
    * Shared data type for a Message. To create a new Message, please use:
    * \code
    * Message::Ptr message(new Message(Message::Information, "Information text"));
    * messageInterface->postMessage(message);
    * \endcode
    *
    * @see MessageInterface, Message
    */
    typedef QSharedPointer<Message> Ptr;


    /**
     * Constructor for new messages.
     * @param type the message type, e.g. MessageType::Information
     * @param richtext s
     */
    Message(MessageType type, const QString& richtext);

    /**
     * Destructor.
     * Deletes all QActions that were added with addAction().
     */
    ~Message();

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
     * Returns whether triggering @p action closes the message widget or not.
     * @
     */
    bool isCloseAction(QAction* action) const;

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

private:
    class MessagePrivate * const d;
    Q_DISABLE_COPY(Message)
};

/**
 * \brief Message interface for posting interactive Message%s to a Document and its View%s.
 *
 * \ingroup kte_group_document_extension
 *
 * This interface allows to post Message%s to a Document. The Message then
 * is shown either the specified View, or in all View%s of the Document.
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
 * Message::Ptr message(new Message(Message::Information, "text"));
 * message->setWordWrap(true);
 * message->addAction(...); // add your actions...
 * iface->postMessage(message);
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
     * is shown.
     * @warning Never post the same message twice.
     */
    virtual void postMessage(Message::Ptr message) = 0;

    /**
     * Remove @p message from the message queue, if it is still active.
     */
    virtual void removeMessage(Message::Ptr message) = 0;

    /**
     * Check, whether @p message was already processed.
     * @return \e true, if @p message is still in the message queue.
     */
    virtual bool isPending(Message::Ptr message);

  //
  // SIGNALS
  //
  public:
    /**
     * This signal is emitted whenever a message was processed.
     * The View pointer is always valid and represents the View that processed
     * this message. The View can be accessed through Message::view().
     *
     * This signal is not emitted if a text message was automatically hidden,
     * see Message::setAutoHide(). Further, if the message was processed by
     * the user, this signal is emitted exactly once, even if multiple View%s
     * show the message.
     *
     * @param message the message
     * @see Message::view()
     */
    void messageProcessed(Message::Ptr message);

  private:
    class MessageInterfacePrivate * const d;
};

}

Q_DECLARE_INTERFACE(KTextEditor::MessageInterface, "org.kde.KTextEditor.MessageInterface")

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
