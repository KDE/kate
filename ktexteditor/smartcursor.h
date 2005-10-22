/* This file is part of the KDE project
   Copyright (C) 2003-2005 Hamish Rodda <rodda@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef KDELIBS_KTEXTEDITOR_SMARTCURSOR_H
#define KDELIBS_KTEXTEDITOR_SMARTCURSOR_H

#include <kdelibs_export.h>

#include <ktexteditor/cursor.h>

namespace KTextEditor
{
class SmartRange;

class SmartCursorWatcher;
class SmartCursorNotifier;

/**
 * \short A Cursor which is bound to a specific Document, and maintains its position.
 *
 * A SmartCursor is an extension of the basic Cursor class. It maintains its
 * position in the document and provides a number of convenience methods,
 * including those for accessing and manipulating the content of the associated
 * Document.  As a result of this, SmartCursors may not be copied, as they need
 * to maintain a connection to the assicated Document.
 *
 * To receive notifications when the position of the cursor changes, or other
 * similar notifications, see either SmartCursorNotifier for QObject signal
 * notification via notifier(), or SmartCursorWatcher for virtual inheritance
 * notification via setWatcher().
 *
 * To create a new SmartCursor:
 * \code
 *   // Retrieve the SmartInterface
 *   KTextEditor::SmartInterface* smart =
 *                   dynamic_cast<KTextEditor::SmartInterface*>( yourDocument );
 *
 *   if ( smart ) {
 *       KTextEditor::SmartCursor* cursor = smart->newSmartCursor();
 *   }
 * \endcode
 *
 * When finished with a SmartCursor, simply delete it.
 *
 * \sa Cursor, SmartCursorNotifier, SmartCursorWatcher, and SmartInterface.
 */
class KTEXTEDITOR_EXPORT SmartCursor : public Cursor
{
  friend class SmartRange;

  public:
    /**
     * Virtual destructor.
     */
    virtual ~SmartCursor();

    // BEGIN Functionality present from having this cursor associated with a Document
    /**
     * Returns the document to which this cursor is attached.
     */
    inline Document* document() const { return m_doc; }

    /**
     * \overload Cursor::isValid()
     * \sa Document::cursorInText()
     */
    virtual bool isValid() const;

    /**
     * Returns the character in the document immediately after this position,
     * ie. from this position to this position plus Cursor(0,1).
     */
    QChar character() const;

    /**
     * Insert @p text.
     *
     * @param text text to insert
     * @param block insert this text as a visual block of text rather than a linear sequence
     *
     * @return @e true on success, otherwise @e false
     */
    virtual bool insertText(const QStringList &text, bool block = false);
    // END

    /**
     * Returns whether this cursor is a SmartCursor.
     */
    virtual bool isSmart() const;

    /**
     * Returns the range that this cursor belongs to, if any.
     *
     * \sa Cursor::range()
     */
    SmartRange* smartRange() const;

    /**
     * Determine if this cursor is located at the end of the current line.
     *
     * @return @e true if the cursor is situated at the end of the line, otherwise @e false.
     */
    virtual bool atEndOfLine() const;

    /**
     * Determine if this cursor is located at the end of the document.
     *
     * @return @e true if the cursor is situated at the end of the document, otherwise @e false.
     */
    virtual bool atEndOfDocument() const;

    // BEGIN Behaviour methods
    /**
     * Returns how this cursor behaves when text is inserted at the cursor.
     * Defaults to moving on insert.
     */
    bool moveOnInsert() const;

    /**
     * Change the behavior of the cursor when text is inserted at the cursor.
     *
     * If @p moveOnInsert is true, the cursor will end up at the end of the insert.
     */
    void setMoveOnInsert(bool moveOnInsert);
    // END

    // BEGIN Notification methods
    /**
     * Determine if a notifier already exists for this smart cursor.
     *
     * \return \e true if a notifier already exists, otherwise \e false
     */
    virtual bool hasNotifier() const = 0;

    /**
     * Returns the current SmartCursorNotifier.  If one does not already exist,
     * it will be created.
     *
     * Connect to the notifier to receive signals indicating change of state of this cursor.
     * The notifier is created at the time it is first requested.  If you have finished with
     * notifications for a reasonable period of time you can save memory, and potentially
     * editor logic processing time, by calling deleteNotifier().
     *
     * \return a pointer to the current SmartCursorNotifier for this SmartCursor.
     *         If one does not already exist, it will be created.
     */
    virtual SmartCursorNotifier* notifier() = 0;

    /**
     * Deletes the current SmartCursorNotifier.
     *
     * When finished with a notifier, call this method to save memory, and potentially
     * editor logic processing time, by having the SmartCursorNotifier deleted.
     */
    virtual void deleteNotifier() = 0;

    /**
     * Returns a pointer to the current SmartCursorWatcher, if one has been set.
     *
     * \return the current SmartCursorWatcher pointer if one exists, otherwise null.
     */
    virtual SmartCursorWatcher* watcher() const = 0;

    /**
     * Provide a SmartCursorWatcher to receive calls indicating change of state of this cursor.
     * To finish receiving notifications, call this function with \p watcher set to 0L.
     *
     * \param watcher the class which will receive notifications about changes to this cursor.
     */
    virtual void setWatcher(SmartCursorWatcher* watcher = 0L) = 0;
    // END

    /**
     * Assignment operator. Assigns the current position of the provided cursor, \p c, only;
     * does not assign watchers, notifiers, behaviour etc.
     *
     * \note The assignment will be performed even if the provided cursor belongs to
     *       another Document.
     *
     * \param cursor the position to assign.
     *
     * \return a reference to this cursor, after assignment has occurred.
     *
     * \sa setPosition()
     */
    inline SmartCursor& operator=(const SmartCursor& c)
      { setPosition(c); return *this; }

  protected:
    /**
     * \internal
     *
     * Constructor for subclasses to utilise.  Protected to prevent direct
     * instantiation.
     *
     * \note 3rd party developers: you do not (and should not) need to subclass
     *       the Smart* classes; instead, use the SmartInterface to create instances.
     *
     * \param position the cursor position to assign
     * \param doc the Document this cursor is associated with
     * \param moveOnInsert the behaviour of this cursor when on the position of an insert.
     *        If \e true, move with the insert; if \e false, retain the current position.
     */
    SmartCursor(const Cursor& position, Document* doc, bool moveOnInsert);

  private:
    /**
     * \internal
     * Copy constructor: Disable copying of this class.
     */
    SmartCursor(const SmartCursor&);

    /**
     * \internal
     *
     * The document that this cursor is associated with.
     */
    Document* m_doc;

    /**
     * \internal
     *
     * Retains the behaviour of the cursor when an insert takes place at the cursor's position.
     */
    bool m_moveOnInsert : 1;
};

}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
