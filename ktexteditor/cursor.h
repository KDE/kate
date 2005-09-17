/* This file is part of the KDE project
   Copyright (C) 2003-2005 Hamish Rodda <rodda@kde.org>
   Copyright (C) 2001-2005 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002 Christian Couder <christian@kdevelop.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>

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

#ifndef KDELIBS_KTEXTEDITOR_CURSOR_H
#define KDELIBS_KTEXTEDITOR_CURSOR_H

#include <kdelibs_export.h>
#include <QObject>

namespace KTextEditor
{
class Document;

/**
 * \short A class which represents a position in a document.
 *
 * A Cursor is a basic class which contains the line() and column() a position
 * in a Document. It is very lightweight and maintains no affiliation with a
 * particular Document.
 *
 * \note The Cursor class is designed to be passed via value, while SmartCursor
 * and derivatives must be passed via pointer as they maintain a connection
 * with their document internally.
 * \note Lines and columns start at 0.
 *
 * If you want additional functionality such as the ability to maintain positon
 * in a document, see SmartCursor.
 *
 * \sa SmartCursor
 */
class KTEXTEDITOR_EXPORT Cursor
{
  public:
    /**
     * The default constructor creates a cursor at position (0,0).
     */
    Cursor();

    /**
     * This constructor creates a cursor initialized with @e line
     * and @e column.
     * @param line line for cursor
     * @param column column for cursor
     */
    Cursor(int line, int column);

    /// Copy constructor.
    Cursor(const Cursor& copy);

    // Virtual destructor. Do not remove! Needed for inheritance.
    virtual ~Cursor ();

    /**
     * Returns whether this cursor is a SmartCursor.
     */
    virtual bool isSmart() const;

    /**
     * Get both the line and column of the cursor position.
     * @param _line will be filled with current cursor line
     * @param _column will be filled with current cursor column
     */
    void position (int &_line, int &_column) const;

    /**
     * Retrieve the line on which this cursor is situated.
     * @return line number, where 0 is the first line.
     */
    virtual int line() const;

    /**
     * Set the cursor line to @e line.
     * @note This function is @c virtual to allow reimplementations to do
     *       checks here or to emit signals.
     * @param line new cursor line
     */
    virtual void setLine (int line);

    /**
     * Retrieve the column on which this cursor is situated.
     * @return column number, where 0 is the first column.
     */
    inline int column() const { return m_column; }

    /**
     * Set the cursor column to @e column.
     * @note This function is @c virtual to allow reimplementations to do
     *       checks here or to emit signals.
     * @param column new cursor column
     */
    virtual void setColumn (int column);

    /**
     * Set the current cursor position to @e pos.
     * @note This function is @c virtual to allow reimplementations to do
     *       checks here or to emit signals.
     * @param pos new cursor position
     */
    virtual void setPosition (const Cursor& pos);

    /**
     * Set the cursor position to @e line and @e column.
     * @param line new cursor line
     * @param column new cursor column
     */
    inline void setPosition(int line, int column) { setPosition(Cursor(line, column)); }

    /**
     * @returns true if the cursor is situated at the start of the line, false if it isn't.
     */
    inline bool atStartOfLine() const { return m_column == 0; }
    inline bool atStartOfDocument() const { return m_line == 0 && m_column == 0; }

    inline Cursor& operator=(const Cursor& c) { setPosition(c); return *this; }
    inline friend Cursor operator+(const Cursor& c1, const Cursor& c2) { return Cursor(c1.line() + c2.line(), c1.column() + c2.column()); }
    inline friend Cursor& operator+=(Cursor& c1, const Cursor& c2) { c1.setPosition(c1.line() + c2.line(), c1.column() + c2.column()); return c1; }

    inline friend Cursor operator-(const Cursor& c1, const Cursor& c2) { return Cursor(c1.line() - c2.line(), c1.column() - c2.column()); }
    inline friend Cursor& operator-=(Cursor& c1, const Cursor& c2) { c1.setPosition(c1.line() - c2.line(), c1.column() - c2.column()); return c1; }

    /**
     * == operator
     * @param c1 first cursor to compare
     * @param c2 second cursor to compare
     * @return @e true, if c1's and c2's line and column are @e equal.
     */
    inline friend bool operator==(const Cursor& c1, const Cursor& c2)
      { return c1.line() == c2.line() && c1.column() == c2.column(); }

    /**
     * != operator
     * @param c1 first cursor to compare
     * @param c2 second cursor to compare
     * @return @e true, if c1's and c2's line and column are @e not equal.
     */
    inline friend bool operator!=(const Cursor& c1, const Cursor& c2)
      { return !(c1 == c2); }

    /**
     * Greater than operator.
     * @param c1 first cursor to compare
     * @param c2 second cursor to compare
     * @return @e true, if c1's position is greater than c2's position,
     *         otherwise @e false.
     */
    inline friend bool operator>(const Cursor& c1, const Cursor& c2)
      { return c1.line() > c2.line() || (c1.line() == c2.line() && c1.m_column > c2.m_column); }

    /**
     * >= operator
     * @param c1 first cursor to compare
     * @param c2 second cursor to compare
     * @return @e true, if c1's position is greater than or equal to c2's
     *         position, otherwise @e false.
     */
    inline friend bool operator>=(const Cursor& c1, const Cursor& c2)
      { return c1.line() > c2.line() || (c1.line() == c2.line() && c1.m_column >= c2.m_column); }

    /**
     * Less than operator
     * @param c1 first cursor to compare
     * @param c2 second cursor to compare
     * @return @e true, if c1's position is greater than or equal to c2's
     *         position, otherwise @e false.
     */
    inline friend bool operator<(const Cursor& c1, const Cursor& c2)
      { return !(c1 >= c2); }

    /**
     * <= operator
     * @param c1 first cursor to compare
     * @param c2 second cursor to compare
     * @return @e true, if c1's position is lesser than or equal to c2's
     *         position, otherwise @e false.
     */
    inline friend bool operator<=(const Cursor& c1, const Cursor& c2)
      { return !(c1 > c2); }

  protected:
    /**
     * cursor line
     */
    int m_line;

    /**
     * cursor column
     */
    int m_column;
};

class SmartCursor;

/**
 * \short A class which provides notifications of state changes to a SmartCursor via virtual inheritance.
 *
 * This class provides notifications of changes to a SmartCursor such as the
 * position in the document, and deletion or insertion of text immediately
 * before or after the cursor.
 *
 * If you prefer to receive notifications via QObject signals, see SmartCursorNotifier.
 *
 * \sa SmartCursor, SmartCursorNotifier
 */
class KTEXTEDITOR_EXPORT SmartCursorWatcher
{
  public:
    virtual ~SmartCursorWatcher();

    /**
     * The cursor's position was changed.
     */
    virtual void positionChanged(SmartCursor* cursor);

    /**
     * The cursor's surrounding characters were both deleted simultaneously.
     * The cursor is automatically placed at the start of the deleted region.
     */
    virtual void positionDeleted(SmartCursor* cursor);

    /**
     * The character immediately surrounding the cursor was deleted.
     * \param before true if the character immediately before was deleted, false if the
     *               character immediately after was deleted.
     */
    virtual void characterDeleted(SmartCursor* cursor, bool before);

    /**
     * A character was inserted immediately before or after the cursor.
     *
     * Whether the char was inserted before or after this cursor depends on
     * moveOnInsert():
     * @li true -> the char was inserted before
     * @li false -> the char was inserted after
     */
    virtual void characterInserted(SmartCursor* cursor, bool insertedBefore);
};

/**
 * \short A class which provides notifications of state changes to a SmartCursor via QObject signals.
 *
 * This class provides notifications of changes to a SmartCursor such as the
 * position in the document, and deletion or insertion of text immediately
 * before or after the cursor.
 *
 * If you prefer to receive notifications via virtual inheritance, see SmartCursorWatcher.
 *
 * \sa SmartCursor, SmartCursorNotifier
 */
class KTEXTEDITOR_EXPORT SmartCursorNotifier : public QObject
{
  Q_OBJECT

  public:
  signals:
    /**
     * The cursor's position was changed.
     */
    void positionChanged(KTextEditor::SmartCursor* cursor);

    /**
     * The cursor's surrounding characters were both deleted simultaneously.
     * The cursor is automatically placed at the start of the deleted region.
     */
    void positionDeleted(KTextEditor::SmartCursor* cursor);

    /**
     * The character immediately surrounding the cursor was deleted.
     * \param before true if the character immediately before was deleted, false if the
     *               character immediately after was deleted.
     */
    void characterDeleted(KTextEditor::SmartCursor* cursor, bool before);

    /**
     * A character was inserted immediately before or after the cursor.
     *
     * Whether the char was inserted before or after this cursor depends on
     * moveOnInsert():
     * @li true -> the char was inserted before
     * @li false -> the char was inserted after
     */
    void characterInserted(KTextEditor::SmartCursor* cursor, bool insertedBefore);
};

class SmartRange;

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
 * \sa Cursor, SmartCursorNotifier, SmartCursorWatcher.
 */
class KTEXTEDITOR_EXPORT SmartCursor : public Cursor
{
  public:
    virtual ~SmartCursor();

    /**
     * Returns the document to which this cursor is attached.
     */
    inline Document* document() const { return m_doc; }

    /**
     * Returns whether the current position of this cursor is a valid position
     * within the document to which it is bound.
     */
    virtual bool isValid() const = 0;

    /**
     * Returns whether the specified position is a valid position within the
     * associated document.
     * \sa Document::cursorInText
     */
    virtual bool isValid(const Cursor& position) const = 0;

    /**
     * Returns whether this cursor is a SmartCursor.
     */
    virtual bool isSmart() const;

    inline SmartRange* belongsToRange() const { return m_range; }
    inline void setBelongsToRange(SmartRange* range) { m_range = range; checkFeedback(); }

    // Text editing commands here

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

    virtual void checkFeedback() = 0;

  private:
    SmartCursor(const SmartCursor &);

    Document* m_doc;
    bool m_moveOnInsert : 1;
    SmartRange* m_range;
};

}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
