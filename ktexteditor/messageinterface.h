/* This file is part of the KDE project
 *
 *   Copyright (C) 2012-2013 Dominik Haumann <dhaumann@kde.org>
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
#include <QtGui/QIcon>

#include <ktexteditor/ktexteditor_export.h>

namespace KTextEditor {

class View;
class Document;

/**
 * @brief This class holds a Message to display in View%s.
 *
 * @section message_intro Introduction
 *
 * The Message class holds the data used to display interactive message widgets
 * in the editor. To post a message, use the MessageInterface.
 *
 * @section message_creation Message Creation and Deletion
 *
 * To create a new Message, use code like this:
 * @code
 * QPointer<Message> message = new Message("My information text", Message::Information);
 * message->setWordWrap(true);
 * // ...
 * @endcode
 *
 * Although discouraged in general, the text of the Message can be changed
 * on the fly when it is already visible with setText().
 *
 * Once you posted the Message through MessageInterface::postMessage(), the
 * lifetime depends on the user interaction. The Message gets automatically
 * deleted either if the user clicks a closing action in the message, or for
 * instance if the document is reloaded.
 *
 * If you posted a message but want to remove it yourself again, just delete
 * the message. But beware of the following warning!
 *
 * @warning Always use QPointer\<Message\> to guard the message pointer from
 *          getting invalid, if you need to access the Message after you posted
 *          it.
 *
 * @section message_positioning Positioning
 *
 * By default, the Message appears right above of the View. However, if desired,
 * the position can be changed through setPosition(). For instance, the
 * search-and-replace code in Kate Part shows the number of performed replacements
 * in a message floating in the view. For further information, have a look at
 * the enum MessagePosition.
 *
 * @section message_hiding Autohiding Messages
 *
 * Message%s can be shown for only a short amount of time by using the autohide
 * feature. With setAutoHide() a timeout in milliseconds can be set after which
 * the Message is automatically hidden. Further, use setAutoHideMode() to either
 * trigger the autohide timer as soon as the widget is shown (AutoHideMode::Immediate),
 * or only after user interaction with the view (AutoHideMode::AfterUserInteraction).
 *
 * The default autohide mode is set to AutoHideMode::AfterUserInteraction.
 * This way, it is unlikely the user misses a notification.
 *
 * @see MessageInterface
 * @author Dominik Haumann \<dhaumann@kde.org\>
 * @since KDE 4.11
 */
class KTEXTEDITOR_EXPORT Message : public QObject
{
  Q_OBJECT

  //
  // public data types
  //
  public:
    /**
     * Message types used as visual indicator.
     * The message types match exactly the behavior of KMessageWidget::MessageType.
     * For simple notifications either use Positive or Information.
     */
    enum MessageType {
      Positive = 0, ///< positive information message
      Information,  ///< information message type
      Warning,      ///< warning message type
      Error         ///< error message type
    };

    /**
     * Message position used to place the message either above or below of the
     * KTextEditor::View.
     */
    enum MessagePosition {
      AboveView = 0, ///< show message above view
      BelowView,     ///< show message below view
      TopInView,     ///< show message as view overlay in the top right corner
      BottomInView   ///< show message as view overlay om the bottom right corner
    };

    /**
     * The AutoHideMode determines when to trigger the autoHide timer.
     * @see setAutoHide(), autoHide()
     */
    enum AutoHideMode {
      Immediate = 0,       ///< auto-hide is triggered as soon as the message is shown
      AfterUserInteraction ///< auto-hide is triggered only after the user interacted with the view
    };

  public:
    /**
     * Constructor for new messages.
     * @param type the message type, e.g. MessageType::Information
     * @param richtext text to be displayed
     */
    Message(const QString& richtext, MessageType type = Message::Information);

    /**
     * Destructor.
     */
    virtual ~Message();

    /**
     * Returns the text set in the constructor.
     */
    QString text() const;

    /**
     * Returns the icon of this message.
     * If the message has no icon set, a null icon is returned.
     */
    QIcon icon() const;

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
     * To connect to an action, use the following code:
     * @code
     * connect(action, SIGNAL(triggered()), receiver, SLOT(slotActionTriggered()));
     * @endcode
     *
     * @param action action to be added
     * @param closeOnTrigger when triggered, the message widget is closed
     *
     * @warning The added actions are deleted automatically.
     *          So do \em not delete the added actions yourself.
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
     * If @p autoHideTimer = 0, auto hide is enabled and set to a sane default
     * value of several seconds.
     *
     * By default, auto hide is disabled.
     *
     * @see autoHide(), setAutoHideMode()
     */
    void setAutoHide(int autoHideTimer = 0);

    /**
     * Returns the auto hide time in milliseconds.
     * Please refer to setAutoHide() for an explanation of the return value.
     *
     * @see setAutoHide(), autoHideMode()
     */
    int autoHide() const;

    /**
     * Sets the autoHide mode to @p mode.
     * The default mode is set to AutoHideMode::AfterUserInteraction.
     * @param mode autoHide mode
     * @see autoHideMode(), setAutoHide()
     */
    void setAutoHideMode(KTextEditor::Message::AutoHideMode mode);

    /**
     * Get the Message's autoHide mode.
     * The default mode is set to AutoHideMode::AfterUserInteraction.
     * @see setAutoHideMode(), autoHide()
     */
    KTextEditor::Message::AutoHideMode autoHideMode() const;

    /**
     * Enabled word wrap according to @p wordWrap.
     * By default, auto wrap is disabled.
     *
     * Word wrap is enabled automatically, if the Message's width is larger than
     * the parent widget's width to avoid breaking the gui layout.
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
     * @param view the associated view the message should be displayed in
     */
    void setView(KTextEditor::View* view);

    /**
     * This function returns the view you set by setView(). If setView() was
     * not called, the return value is 0.
     */
    KTextEditor::View* view() const;

    /**
     * Set the document pointer to @p document.
     * This is called by the implementation, as soon as you post a message
     * through MessageInterface::postMessage(), so that you do not have to
     * call this yourself.
     * @see MessageInterface, document()
     */
    void setDocument(KTextEditor::Document* document);

    /**
     * Returns the document pointer this message was posted in.
     * This pointer is 0 as long as the message was not posted.
     */
    KTextEditor::Document* document() const;

    /**
     * Sets the @p position either to AboveView or BelowView.
     * By default, the position is set to MessagePosition::AboveView.
     * @see MessagePosition
     */
    void setPosition(MessagePosition position);

    /**
     * Returns the desired message position of this message.
     */
    MessagePosition position() const;

  public Q_SLOTS:
    /**
     * Sets the notification contents to @p text.
     * If the message was already sent through MessageInterface::postMessage(),
     * the displayed text changes on the fly.
     * @note Change text on the fly with care, since changing the text may
     *       resize the notification widget, which may result in a distracting
     *       user experience.
     * @param richtext new notification text (rich text supported)
     * @see textChanged()
     */
    void setText(const QString& richtext);

    /**
     * Optionally set an icon for this notification.
     * The icon is shown next to the message text.
     * @param icon the icon to be displayed
     */
    void setIcon(const QIcon& icon);

  Q_SIGNALS:
    /**
     * This signal is emitted before the message is deleted. Afterwards, this
     * pointer is invalid.
     *
     * Use the function document() to access the associated Document.
     *
     * @param message closed/processed message
     */
    void closed(KTextEditor::Message* message);

    /**
     * This signal is emitted whenever setText() is called.
     * If the message was already sent through MessageInterface::postMessage(),
     * the displayed text changes on the fly.
     * @note Change text on the fly with care, since changing the text may
     *       resize the notification widget, which may result in a distracting
     *       user experience.
     *
     * @param text new notification text (rich text supported)
     * @see setText()
     */
    void textChanged(const QString& text);

    /**
     * This signal is emitted whenever setIcon() is called.
     * If the message was already sent through MessageInterface::postMessage(),
     * the displayed icon changes on the fly.
     * @note Change the icon on the fly with care, since changing the text may
     *       resize the notification widget, which may result in a distracting
     *       user experience.
     *
     * @param icon new notification icon
     * @see setIcon()
     */
    void iconChanged(const QIcon& icon);

private:
    class MessagePrivate * const d;
};

/**
 * \brief Message interface for posting interactive Message%s to a Document and its View%s.
 *
 * \ingroup kte_group_document_extension
 *
 * This interface allows to post Message%s to a Document. The Message then
 * is shown either the specified View if Message::setView() was called, or
 * in all View%s of the Document.
 *
 * \section message_interface Working with Messages
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
 * // always use a QPointer go guard your Message, if you keep a pointer
 * // after calling postMessage()
 * QPointer<Message> message = new Message("text", Message::Information);
 * message->setWordWrap(true);
 * message->addAction(...); // add your actions...
 * iface->postMessage(message);
 *
 * // The Message is deleted automatically if the Message gets closed,
 * // meaning that you usually can forget the pointer.
 * // If you really need to delete a message before the user processed it,
 * guard it with a QPointer!
 * \endcode
 *
 * @see Message
 * @author Dominik Haumann \<dhaumann@kde.org\>
 * @since KDE 4.11
 */
class KTEXTEDITOR_EXPORT MessageInterface
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
     *
     * Usually, you can simply forget the pointer, as the Message is deleted
     * automatically, once it is processed or the document gets closed.
     *
     * If the Document does not have a View yet, the Message is queued and
     * shown, once a View for the Document is created.
     *
     * @param message the message to show
     * @return @e true, if @p message was posted. @e false, if message == 0.
     */
    virtual bool postMessage(Message* message) = 0;

  private:
    class MessageInterfacePrivate * const d;
};

}

Q_DECLARE_INTERFACE(KTextEditor::MessageInterface, "org.kde.KTextEditor.MessageInterface")

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
