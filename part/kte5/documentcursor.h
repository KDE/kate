/* This file is part of the KDE project
 * 
 *  Copyright (C) 2010 Christoph Cullmann <cullmann@kde.org>
 *  Copyright (C) 2012 Dominik Haumann <dhaumann@kde.org>
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

#ifndef KTEXTEDITOR_DOCUMENT_CURSOR_H
#define KTEXTEDITOR_DOCUMENT_CURSOR_H

#include <ktexteditor/cursor.h>
#include <ktexteditor/document.h>

#include "katepartprivate_export.h"

namespace KTextEditor {

/**
 * \short A Cursor which is bound to a specific Document.
 *
 * \section documentcursor_intro Introduction
 * A DocumentCursor is an extension of the basic Cursor class.
 * The DocumentCursor is bound to a specific Document instance.
 * This way, the cursor povides additional functions like gotoNextLine(),
 * gotoPreviousLine() and move() according to the WrapBehavior.
 *
 * The only difference to a MovingCursor is that the DocumentCursor's
 * position does not automatically move on text manipulation.
 *
 * \section documentcursor_position Validity
 *
 * When constructing a DocumentCursor, a valid document pointer is required
 * in the constructor. A null pointer will assert in debug builds.
 * Further, a DocumentCursor should only be used as long as the Document
 * exists, otherwise the DocumentCursor contains a dangling pointer to the
 * previously assigned Document.
 *
 * \section documentcursor_example Example
 *
 * A DocumentCursor is created and used like this:
 * \code
 * KTextEditor::DocumentCursor docCursor(document);
 * docCursor.setPosition(0, 0);
 * docCursor.gotoNextLine();
 * docCursor.move(5); // move 5 characters to the right
 * \endcode
 *
 * \see KTextEditor::Cursor, KTextEditor::MovingCursor
 *
 * \author Dominik Haumann \<dhaumann@kde.org\>
 *
 * \todo KDE5: move to ktexteditor interface, use it in
 *       MovingCursor::move() to avoid code duplication
 */
class KATEPART_TESTS_EXPORT DocumentCursor
{
  //
  // sub types
  //
  public:
    /**
     * Wrap behavior for end of line treatement used in move().
     */
    enum WrapBehavior {
      Wrap = 0x0,  ///< wrap at end of line
      NoWrap = 0x1 ///< do not wrap at end of line
    };

  //
  // Constructor
  //
  public:
    /**
     * Constructor that creates a DocumentCursor at the \e invalid position
     * (-1, -1).
     * \see isValid()
     */
    DocumentCursor(KTextEditor::Document* document);

    /**
     * Constructor that creates a DocumentCursor located at \p position.
     */
    DocumentCursor(KTextEditor::Document* document, const KTextEditor::Cursor& position);

    /**
     * Constructor that creates a DocumentCursor located at \p line and \p column.
     */
    DocumentCursor(KTextEditor::Document* document, int line, int column);

    /**
     * Copy constructor. Make sure the Document of the DocumentCursor is
     * valid.
     */
    DocumentCursor (const DocumentCursor &other);

  //
  // stuff that needs to be implemented by editor part cusors
  //
  public:

    /**
     * Gets the document to which this cursor is bound.
     * \return a pointer to the document
     */
    Document *document () const;

    /**
     * Set the current cursor position to \e position.
     * If \e position is not valid, meaning that either its line < 0 or its
     * column < 0, then the document cursor is set to invalid(-1, -1).
     *
     * \param position new cursor position
     */
    void setPosition (const KTextEditor::Cursor& position);

    /**
     * Retrieve the line on which this cursor is situated.
     * \return line number, where 0 is the first line.
     */
    int line() const;

    /**
     * Retrieve the column on which this cursor is situated.
     * \return column number, where 0 is the first column.
     */
    int column() const;

    /**
     * Destruct the moving cursor.
     */
    ~DocumentCursor ();

  //
  // forbidden stuff
  //
  private:
    /**
     * no default constructor, as we need a document.
     */
    DocumentCursor ();

  //
  // convenience API
  //
  public:

    /**
     * Returns whether the current position of this cursor is a valid position,
     * i.e. whether line() >= 0 and column() >= 0.
     * \return \e true , if the cursor position is valid, otherwise \e false
     */
    inline bool isValid() const {
      return line() >= 0 && column() >= 0;
    }

    /**
     * Check whether the current position of this cursor is a valid text
     * position.
     * \return \e true , if the cursor is a valid text position, otherwise \e false
     */
    // TODO KDE5: use KTE::Document::isValidTextPosition()
    inline bool isValidTextPosition() const {
      return isValid() && line() < document()->lines() && column() <= document()->lineLength(line());
    }

    /**
     * Make sure the cursor position is at a valid text position according to
     * the following rules.
     * - If the cursor is invalid(), i.e. either line < 0 or column < 0, it is
     *   set to (0, 0).
     * - If the cursor's line is past the number of lines in the document, the
     *   cursor is set to Document::documentEnd().
     * - If the cursor's column is past the line length, the cursor column is
     *   set to the line length.
     * - If the cursor is inside a Unicode surrogate, the cursor is moved to the
     *   beginning of the Unicode surrogate.
     *
     * After calling makeValid(), the cursor is guaranteed to be located at
     * a valid text position.
     * \see isValidTextPosition(), isValid()
     */
    void makeValid();

    /**
     * \overload
     *
     * Set the cursor position to \e line and \e column.
     *
     * \param line new cursor line
     * \param column new cursor column
     */
    void setPosition (int line, int column);

    /**
     * Set the cursor line to \e line.
     * \param line new cursor line
     */
    void setLine(int line);

    /**
     * Set the cursor column to \e column.
     * \param column new cursor column
     */
    void setColumn(int column);

    /**
     * Determine if this cursor is located at column 0 of a valid text line.
     *
     * \return \e true if cursor is a valid text position and column()=0, otherwise \e false.
     */
    bool atStartOfLine() const;

    /**
     * Determine if this cursor is located at the end of the current line.
     *
     * \return \e true if the cursor is situated at the end of the line, otherwise \e false.
     */
    bool atEndOfLine() const;

    /**
     * Determine if this cursor is located at line 0 and column 0.
     *
     * \return \e true if the cursor is at start of the document, otherwise \e false.
     */
    bool atStartOfDocument() const;

    /**
     * Determine if this cursor is located at the end of the last line in the
     * document.
     *
     * \return \e true if the cursor is at the end of the document, otherwise \e false.
     */
    bool atEndOfDocument() const;

    /**
     * Moves the cursor to the next line and sets the column to 0. If the cursor
     * position is already in the last line of the document, the cursor position
     * remains unchanged and the return value is \e false.
     *
     * \return \e true on success, otherwise \e false
     */
    bool gotoNextLine();

    /**
     * Moves the cursor to the previous line and sets the column to 0. If the
     * cursor position is already in line 0, the cursor position remains
     * unchanged and the return value is \e false.
     *
     * \return \e true on success, otherwise \e false
     */
    bool gotoPreviousLine();

    /**
     * Moves the cursor \p chars character forward or backwards. If \e wrapBehavior
     * equals WrapBehavior::Wrap, the cursor is automatically wrapped to the
     * next line at the end of a line.
     *
     * When moving backwards, the WrapBehavior does not have any effect.
     * \note If the cursor could not be moved the amount of chars requested,
     *       the cursor is not moved at all!
     *
     * \return \e true on success, otherwise \e false
     */
    bool move(int chars, WrapBehavior wrapBehavior = Wrap);

    /**
     * Convert this clever cursor into a dumb one.
     * @return normal cursor
     */
    const Cursor& toCursor () const;

    /**
     * Convert this clever cursor into a dumb one. Equal to toCursor, allowing to use implicit conversion.
     * @return normal cursor
     */
    operator const Cursor& () const;

  //
  // operators for: DocumentCursor <-> DocumentCursor
  //
    /**
     * Assignment operator. Same as the copy constructor. Make sure that
     * the assigned Document is a valid document pointer.
     */
    DocumentCursor &operator= (const DocumentCursor &other);

    /**
     * Equality operator.
     *
     * \note comparison between two invalid cursors is undefined.
     *       comparison between an invalid and a valid cursor will always be \e false.
     *
     * \param c1 first cursor to compare
     * \param c2 second cursor to compare
     * \return \e true, if c1's and c2's assigned document, line and column are \e equal.
     */
    inline friend bool operator==(const DocumentCursor& c1, const DocumentCursor& c2)
      { return c1.document() == c2.document() && c1.line() == c2.line() && c1.column() == c2.column(); }

    /**
     * Inequality operator.
     * \param c1 first cursor to compare
     * \param c2 second cursor to compare
     * \return \e true, if c1's and c2's assigned document, line and column are \e not equal.
     */
    inline friend bool operator!=(const DocumentCursor& c1, const DocumentCursor& c2)
      { return !(c1 == c2); }

    /**
     * Greater than operator.
     * \param c1 first cursor to compare
     * \param c2 second cursor to compare
     * \return \e true, if c1's position is greater than c2's position,
     *         otherwise \e false.
     */
    inline friend bool operator>(const DocumentCursor& c1, const DocumentCursor& c2)
      { return c1.line() > c2.line() || (c1.line() == c2.line() && c1.column() > c2.column()); }

    /**
     * Greater than or equal to operator.
     * \param c1 first cursor to compare
     * \param c2 second cursor to compare
     * \return \e true, if c1's position is greater than or equal to c2's
     *         position, otherwise \e false.
     */
    inline friend bool operator>=(const DocumentCursor& c1, const DocumentCursor& c2)
      { return c1.line() > c2.line() || (c1.line() == c2.line() && c1.column() >= c2.column()); }

    /**
     * Less than operator.
     * \param c1 first cursor to compare
     * \param c2 second cursor to compare
     * \return \e true, if c1's position is greater than or equal to c2's
     *         position, otherwise \e false.
     */
    inline friend bool operator<(const DocumentCursor& c1, const DocumentCursor& c2)
      { return !(c1 >= c2); }

    /**
     * Less than or equal to operator.
     * \param c1 first cursor to compare
     * \param c2 second cursor to compare
     * \return \e true, if c1's position is lesser than or equal to c2's
     *         position, otherwise \e false.
     */
    inline friend bool operator<=(const DocumentCursor& c1, const DocumentCursor& c2)
      { return !(c1 > c2); }

    /**
     * kDebug() stream operator. Writes this cursor to the debug output in a nicely formatted way.
     * @param s debug stream
     * @param cursor cursor to print
     * @return debug stream
     */
    inline friend QDebug operator<< (QDebug s, const DocumentCursor *cursor) {
      if (cursor)
        s.nospace() << "(" << cursor->document() << ": " << cursor->line() << ", " << cursor->column() << ")";
      else
        s.nospace() << "(null document cursor)";
      return s.space();
    }

    /**
     * kDebug() stream operator. Writes this cursor to the debug output in a nicely formatted way.
     * @param s debug stream
     * @param cursor cursor to print
     * @return debug stream
     */
    inline friend QDebug operator<< (QDebug s, const DocumentCursor &cursor) {
      return s << &cursor;
    }

  private:
    KTextEditor::Document* m_document;
    KTextEditor::Cursor m_cursor;
};

}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
