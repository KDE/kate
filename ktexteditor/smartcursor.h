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
 * \short A SmartCursor is a Cursor which is bound to a specific Document, and maintains its position.
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
 * \sa Cursor, SmartCursorNotifier, and SmartCursorWatcher.
 */
class KTEXTEDITOR_EXPORT SmartCursor : public Cursor
{
  friend class SmartRange;

  public:
    virtual ~SmartCursor();

    // BEGIN Functionality present from having this cursor associated with a Document
    /**
     * Returns the document to which this cursor is attached.
     */
    inline Document* document() const { return m_doc; }

    /**
     * Returns whether the specified position is a valid position within the
     * associated document.
     * \sa Document::cursorInText
     */
    virtual bool isValid(const Cursor& position) const = 0;

    /**
     * Return the character in the document at this position.
     */
    QChar character() const;

    /**
     * Insert @p text.
     * @param text text to insert
     * @param block insert this text as a visual block of text rather than a linear sequence
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
     */
    SmartRange* smartRange() const;

    /**
     * @returns true if the cursor is situated at the end of the line, false if it isn't.
     */
    virtual bool atEndOfLine() const = 0;
    bool atEndOfDocument() const;

    // BEGIN Behaviour methods
    /**
     * Returns how this cursor behaves when text is inserted at the cursor.
     * Defaults to moving on insert.
     */
    inline bool moveOnInsert() const { return m_moveOnInsert; }

    /**
     * Change the behavior of the cursor when text is inserted at the cursor.
     *
     * If @p moveOnInsert is true, the cursor will end up at the end of the insert.
     */
    inline void setMoveOnInsert(bool moveOnInsert) { m_moveOnInsert = moveOnInsert; }
    // END

    // BEGIN Notification methods
    /**
     * Connect to the notifier to receive signals indicating change of state of this cursor.
     * The notifier is created at the time it is first requested.  If you have finished with
     * notifications for a reasonable period of time you can save memory by calling deleteNotifier().
     */
    virtual SmartCursorNotifier* notifier() = 0;

    /**
     * When finished with a notifier(), call this method to save memory by
     * having the SmartCursorNotifier deleted.
     */
    virtual void deleteNotifier() = 0;

    /**
     * Provide a SmartCursorWatcher to receive calls indicating change of state of this cursor.
     * To finish receiving notifications, call this function with \p watcher set to 0L.
     * \param watcher the class which will receive notifications about changes to this cursor.
     */
    virtual void setWatcher(SmartCursorWatcher* watcher = 0L) = 0;
    // END

    inline SmartCursor& operator=(const SmartCursor& c) { setPosition(c); return *this; }

  protected:
    SmartCursor(const Cursor& position, Document* doc, bool moveOnInsert);

    /**
     * Called whenever the feedback requirements may have changed.
     *
     * Note: this is not called on construction, although it would make sense -
     *       unfortunately the subclass is not yet initialized.
     */
    virtual void checkFeedback() = 0;

    /**
     * Sets the range that this cursor belongs to.
     */
    void setRange(SmartRange* range);

  private:
    SmartCursor(const SmartCursor &);

    Document* m_doc;
    bool m_moveOnInsert : 1;
};

}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
