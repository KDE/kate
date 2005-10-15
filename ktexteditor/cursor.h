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
#include <kdebug.h>

namespace KTextEditor
{
class Document;
class Range;
class SmartRange;

/**
 * \short A Cursor represents a position in a Document.
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
  friend class Range;

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
     * Returns whether the current position of this cursor is a valid position
     * (line + column must both be >= 0).
     *
     * Smart cursors should override this to return whether the cursor is valid
     * within the linked document.
     */
    virtual bool isValid() const;

    /**
     * Returns an invalid cursor.
     * \todo should invalid cursors be always or never equal to each other?
     */
    static const Cursor& invalid();

    /**
     * Returns a cursor representing the start of any document - i.e., line 0, column 0.
     */
    static const Cursor& start();

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
     * Returns the range that this cursor belongs to, if any.
     */
    inline Range* range() const { return m_range; }

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

    inline friend kdbgstream& operator<< (kdbgstream& s, const Cursor& cursor) {
      if (&cursor)
        s << "(" << cursor.line() << ", " << cursor.column() << ")";
      else
        s << "(null cursor)";
      return s;
    }

    inline friend kndbgstream& operator<< (kndbgstream& s, const Cursor&) { return s; }

  protected:
    /**
     * Sets the range that this cursor belongs to.
     */
    void setRange(Range* range);

    /**
     * cursor line
     */
    int m_line;

    /**
     * cursor column
     */
    int m_column;

    /**
     * range which owns this cursor, if any
     */
    Range* m_range;
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
    SmartCursorWatcher();
    virtual ~SmartCursorWatcher();

    /**
     * Returns whether this watcher wants to be notified of changes that happen
     * directly to the cursor, e.g. by calls to SmartCursor::setPosition(), rather
     * than just when surrounding text changes.
     */
    bool wantsDirectChanges() const;

    /**
     * Set whether this watcher should be notified of changes that happen
     * directly to the cursor, e.g. by calls to SmartCursor::setPosition(), rather
     * than just when surrounding text changes.
     */
    void setWantsDirectChanges(bool wantsDirectChanges);

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
     * If both characters are simultaneously deleted, positionDeleted() is called instead.
     *
     * \param before true if the character immediately before was deleted, false if the
     *               character immediately after was deleted.
     */
    virtual void characterDeleted(SmartCursor* cursor, bool deletedBefore);

    /**
     * A character was inserted immediately before or after the cursor.
     *
     * Whether the char was inserted before or after this cursor depends on
     * moveOnInsert():
     * @li true -> the char was inserted before
     * @li false -> the char was inserted after
     */
    virtual void characterInserted(SmartCursor* cursor, bool insertedBefore);

  private:
    bool m_wantDirectChanges;
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
    SmartCursorNotifier();

    /**
     * Returns whether this notifier will notify of changes that happen
     * directly to the cursor, e.g. by calls to SmartCursor::setPosition(), rather
     * than just when surrounding text changes.
     */
    bool wantsDirectChanges() const;

    /**
     * Set whether this notifier should notify of changes that happen
     * directly to the cursor, e.g. by calls to SmartCursor::setPosition(), rather
     * than just when surrounding text changes.
     */
    void setWantsDirectChanges(bool wantsDirectChanges);

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
     * One character immediately surrounding the cursor was deleted.
     * If both characters are simultaneously deleted, positionDeleted() is called instead.
     *
     * \param before true if the character immediately before was deleted, false if the
     *               character immediately after was deleted.
     */
    void characterDeleted(KTextEditor::SmartCursor* cursor, bool deletedBefore);

    /**
     * A character was inserted immediately before or after the cursor.
     *
     * Whether the char was inserted before or after this cursor depends on
     * moveOnInsert():
     * @li true -> the char was inserted before
     * @li false -> the char was inserted after
     */
    void characterInserted(KTextEditor::SmartCursor* cursor, bool insertedBefore);

  private:
    bool m_wantDirectChanges;
};

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

    // Reimplementations
    virtual void setLine (int line);
    virtual void setColumn (int column);
    virtual void setPosition (const Cursor& pos);

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
