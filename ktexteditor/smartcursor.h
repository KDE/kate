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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KDELIBS_KTEXTEDITOR_SMARTCURSOR_H
#define KDELIBS_KTEXTEDITOR_SMARTCURSOR_H

#include <ktexteditor/ktexteditor_export.h>
#include <ktexteditor/cursor.h>

namespace KTextEditor
{
class SmartRange;

class SmartCursorWatcher;
class SmartCursorNotifier;

/**
 * \short A Cursor which is bound to a specific Document, and maintains its position.
 *
 * \ingroup kte_group_smart_classes
 *
 * A SmartCursor is an extension of the basic Cursor class. It maintains its
 * position in the document and provides a number of convenience methods,
 * including those for accessing and manipulating the content of the associated
 * Document.  As a result of this, SmartCursor%s may not be copied, as they need
 * to maintain a connection to the associated Document.
 *
 * To receive notifications when the position of the cursor changes, or other
 * similar notifications, see either SmartCursorNotifier for QObject signal
 * notification via notifier(), or SmartCursorWatcher for virtual inheritance
 * notification via setWatcher().
 *
 * Create a new SmartCursor like this:
 * \code
 * // Retrieve the SmartInterface
 * KTextEditor::SmartInterface* smart =
 *     qobject_cast<KTextEditor::SmartInterface*>( yourDocument );
 *
 * if ( smart ) {
 *     KTextEditor::SmartCursor* cursor = smart->newSmartCursor();
 * }
 * \endcode
 *
 * When finished with a SmartCursor, simply delete it.
 *
 * \sa Cursor, SmartCursorNotifier, SmartCursorWatcher, and SmartInterface.
 *
 * \author Hamish Rodda \<rodda@kde.org\>
 */
class KTEXTEDITOR_EXPORT SmartCursor : public Cursor
{
  friend class SmartRange;

  public:
    /**
     * Virtual destructor.
     *
     * Subclasses should call SmartCursorNotifier::deleted() and
     * SmartCursorWatcher::deleted() methods \e before cleaning themselves up.
     */
    virtual ~SmartCursor();

    /**
     * Returns that this cursor is a SmartCursor.
     */
    virtual bool isSmartCursor() const;

    /**
     * Returns this cursor as a SmartCursor
     */
    virtual SmartCursor* toSmartCursor() const;

    /**
     * Returns the range that this cursor belongs to, if any.
     *
     * \sa Cursor::range()
     */
    SmartRange* smartRange() const;

    //BEGIN Functionality present from having this cursor associated with a Document
    /**
     * \name Document-related functions
     *
     * The following functions are provided for convenient access to the
     * associated Document.
     * \{
     */
    /**
     * Returns the document to which this cursor is attached.
     */
    Document* document() const;

    /**
     * Determine if this cursor is located at the end of the current line.
     *
     * \return \e true if the cursor is situated at the end of the line, otherwise \e false.
     */
    virtual bool atEndOfLine() const;

    /**
     * Determine if this cursor is located at the end of the document.
     *
     * \return \e true if the cursor is situated at the end of the document, otherwise \e false.
     */
    virtual bool atEndOfDocument() const;

    /**
     * \reimp
     * \sa Document::cursorInText()
     */
    virtual bool isValid() const;

    /**
     * Returns the character in the document immediately after this position,
     * ie. from this position to this position plus Cursor(0,1).
     */
    QChar character() const;

    /**
     * Insert \p text into the associated Document.
     *
     * \param text text to insert
     * \param block insert this text as a visual block of text rather than a linear sequence
     *
     * \return \e true on success, otherwise \e false
     */
    virtual bool insertText(const QStringList &text, bool block = false);

    /**
     * Defines the ways in which the cursor can be advanced.
     * Important for languages where multiple characters are required to
     * form one letter.
     */
    enum AdvanceMode {
      /// Movement is calculated on the basis of absolute numbers of characters
      ByCharacter,
      /// Movement takes into account valid cursor positions only (as defined by bidirectional processing)
      ByCursorPosition
    };

    /**
     * Move cursor by specified \a distance along the document buffer.
     *
     * E.g.:
     * \code
     * cursor.advance(1);
     * \endcode
     * will move the cursor forward by one character, or, if the cursor is already
     * on the end of the line, will move it to the start of the next line.
     *
     * \note Negative numbers should be accepted, and move backwards.
     * \note Not all \a mode%s are required to be supported.
     *
     * \param distance distance to advance (or go back if \a distance is negative)
     * \param mode whether to move by character, or by number of valid cursor positions
     *
     * \return true if the position could be reached within the document, otherwise false
     *         (the cursor should not move if \a distance is beyond the end of the document).
     */
    virtual bool advance(int distance, AdvanceMode mode = ByCharacter);
    //END

    //BEGIN Behavior methods
    /**
     * \}
     *
     * \name Behavior
     *
     * The following functions relate to the behavior of this SmartCursor.
     * \{
     */
    enum InsertBehavior {
      StayOnInsert = 0,
      MoveOnInsert
    };
    /**
     * Returns how this cursor behaves when text is inserted at the cursor.
     * Defaults to moving on insert.
     */
    InsertBehavior insertBehavior() const;

    /**
     * Change the behavior of the cursor when text is inserted at the cursor.
     *
     * If \p moveOnInsert is true, the cursor will end up at the end of the insert.
     */
    void setInsertBehavior(InsertBehavior insertBehavior);
    //END

    //BEGIN Notification methods
    /**
     * \}
     *
     * \name Notification
     *
     * The following functions allow for changes related to this cursor to be
     * notified to 3rd party programs.
     * \{
     */
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
    //!\}
    //END

    /**
     * Assignment operator. Assigns the current position of the provided cursor, \p c, only;
     * does not assign watchers, notifiers, behavior, etc.
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
    inline SmartCursor& operator=(const SmartCursor& cursor)
      { setPosition(cursor); return *this; }

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
     * \param insertBehavior the behavior of this cursor when on the position of an insert.
     */
    SmartCursor(const Cursor& position, Document* doc, InsertBehavior insertBehavior);

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
     * Retains the behavior of the cursor when an insert takes place at the cursor's position.
     */
    bool m_moveOnInsert : 1;
};

}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
