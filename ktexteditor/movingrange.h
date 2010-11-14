/* This file is part of the KDE project
 *
 *  Copyright (C) 2010 Christoph Cullmann <cullmann@kde.org>
 *
 *  Based on code of the SmartCursor/Range by:
 *  Copyright (C) 2003-2005 Hamish Rodda <rodda@kde.org>
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

#ifndef KDELIBS_KTEXTEDITOR_MOVINGRANGE_H
#define KDELIBS_KTEXTEDITOR_MOVINGRANGE_H

#include <ktexteditor/ktexteditor_export.h>
#include <ktexteditor/attribute.h>
#include <ktexteditor/range.h>
#include <ktexteditor/movingcursor.h>

namespace KTextEditor
{

class Document;
class View;
class MovingRangeFeedback;

/**
 * \short A range which is bound to a specific Document, and maintains its position.
 *
 * \ingroup kte_group_moving_classes
 *
 * A MovingRange is an extension of the basic Range class. It maintains its
 * position in the document. As a result of this, MovingRange%s may not be copied, as they need
 * to maintain a connection to the associated Document.
 *
 * Create a new MovingRange like this:
 * \code
 * // Retrieve the MovingInterface
 * KTextEditor::MovingInterface* moving =
 *     qobject_cast<KTextEditor::MovingInterface*>( yourDocument );
 *
 * if ( moving ) {
 *     KTextEditor::MovingRange* range = moving->newMovingRange();
 * }
 * \endcode
 *
 * When finished with a MovingRange, simply delete it.
 * If the document the cursor belong to is deleted, it will get deleted automatically.
 *
 * \sa Cursor, MovingCursor, Range and MovingInterface.
 *
 * \author Christoph Cullmann \<cullmann@kde.org\>
 *
 * \since 4.5
 */
class KTEXTEDITOR_EXPORT MovingRange
{
  //
  // sub types
  //
  public:
     /// Determine how the range reacts to characters inserted immediately outside the range.
    enum InsertBehavior {
      /// Don't expand to encapsulate new characters in either direction. This is the default.
      DoNotExpand = 0x0,
      /// Expand to encapsulate new characters to the left of the range.
      ExpandLeft = 0x1,
      /// Expand to encapsulate new characters to the right of the range.
      ExpandRight = 0x2
    };
    Q_DECLARE_FLAGS(InsertBehaviors, InsertBehavior)

    /**
     * Behavior of range if it becomes empty.
     */
    enum EmptyBehavior {
      AllowEmpty        = 0x0, ///< allow range to be empty
      InvalidateIfEmpty = 0x1  ///< invalidate range, if it becomes empty
    };

  //
  // stuff that needs to be implemented by editor part cusors
  //
  public:
    /**
     * Set insert behaviors.
     * @param insertBehaviors new insert behaviors
     */
    virtual void setInsertBehaviors (InsertBehaviors insertBehaviors) = 0;

    /**
     * Get current insert behaviors.
     * @return current insert behaviors
     */
    virtual InsertBehaviors insertBehaviors () const = 0;

    /**
     * Set if this range will invalidate itself if it becomes empty.
     * @param emptyBehavior behavior on becoming empty
     */
    virtual void setEmptyBehavior (EmptyBehavior emptyBehavior) = 0;

    /**
     * Will this range invalidate itself if it becomes empty?
     * @return behavior on becoming empty
     */
    virtual EmptyBehavior emptyBehavior () const = 0;

    /**
     * Gets the document to which this range is bound.
     * \return a pointer to the document
     */
    virtual Document *document () const = 0;

    /**
     * Set the range of this range.
     * A TextRange is not allowed to be empty, as soon as start == end position, it will become
     * automatically invalid!
     * @param range new range for this clever range
     */
    virtual void setRange (const KTextEditor::Range &range) = 0;

    /**
     * Retrieve start cursor of this range, read-only.
     * @return start cursor
     */
    virtual const MovingCursor &start () const = 0;

    /**
     * Retrieve end cursor of this range, read-only.
     * @return end cursor
     */
    virtual const MovingCursor &end () const = 0;

    /**
     * Gets the active view for this range. Might be already invalid, internally only used for pointer comparisons.
     *
     * \return a pointer to the active view
     */
    virtual View *view () const = 0;

    /**
     * Sets the currently active view for this range.
     * This will trigger update of the relevant view parts, if the view changed.
     * Set view before the attribute, that will avoid not needed redraws.
     *
     * \param attribute View to assign to this range. If null, simply
     *                  removes the previous view.
     */
    virtual void setView (View *view) = 0;

    /**
     * Gets the active Attribute for this range.
     *
     * \return a pointer to the active attribute
     */
    virtual Attribute::Ptr attribute () const = 0;

    /**
     * Sets the currently active attribute for this range.
     * This will trigger update of the relevant view parts, if the attribute changed.
     *
     * \param attribute Attribute to assign to this range. If null, simply
     *                  removes the previous Attribute.
     */
    virtual void setAttribute (Attribute::Ptr attribute) = 0;

    /**
     * Is this range's attribute only visible in views, not for example prints?
     * Default is false.
     * @return range visible only for views
     */
    virtual bool attributeOnlyForViews () const = 0;

    /**
     * Set if this range's attribute is only visible in views, not for example prints.
     * @param onlyForViews attribute only valid for views
     */
    virtual void setAttributeOnlyForViews (bool onlyForViews) = 0;

    /**
     * Gets the active MovingRangeFeedback for this range.
     *
     * \return a pointer to the active MovingRangeFeedback
     */
    virtual MovingRangeFeedback *feedback () const = 0;

    /**
     * Sets the currently active MovingRangeFeedback for this range.
     * This will trigger evaluation if feedback must be send again (for example if mouse is already inside range).
     *
     * \param attribute MovingRangeFeedback to assign to this range. If null, simply
     *                  removes the previous MovingRangeFeedback.
     */
    virtual void setFeedback (MovingRangeFeedback *feedback) = 0;
    
    /**
     * Gets the current Z-depth of this range.
     * Ranges with smaller Z-depth than others will win during rendering.
     * Default is 0.0.
     * 
     * Defined depths for common kind of ranges use in editor components implenting this interface,
     * smaller depths are more more in the foreground and will win during rendering:
     * - Selection == -100000.0
     * - Search == -10000.0
     * - Bracket Highlighting == -1000.0
     * - Folding Hover == -100.0
     * 
     * \return current Z-depth of this range
     */
    virtual qreal zDepth () const = 0;
    
    /**
     * Set the current Z-depth of this range.
     * Ranges with smaller Z-depth than others will win during rendering.
     * This will trigger update of the relevant view parts, if the depth changed.
     * Set depth before the attribute, that will avoid not needed redraws.
     * Default is 0.0.
     * 
     * \param zDepth new Z-depth of this range
     */
    virtual void setZDepth (qreal zDepth) = 0;

    /**
     * Destruct the moving range.
     */
    virtual ~MovingRange ();

  //
  // forbidden stuff
  //
  protected:
    /**
     * For inherited class only.
     */
    MovingRange ();

  private:
    /**
     * no copy constructor, don't allow this to be copied.
     */
    MovingRange (const MovingRange &);

    /**
     * no assignment operator, no copying around clever ranges.
     */
    MovingRange &operator= (const MovingRange &);

  //
  // convenience API
  //
  public:
    /**
     * \overload
     * Set the range of this range
     * A TextRange is not allowed to be empty, as soon as start == end position, it will become
     * automatically invalid!
     * @param start new start for this clever range
     * @param end new end for this clever range
     */
    void setRange (const Cursor &start, const Cursor &end);

    /**
     * Convert this clever range into a dumb one.
     * @return normal range
     */
    const Range toRange () const { return Range (start().toCursor(), end().toCursor()); }

    /**
     * Convert this clever range into a dumb one. Equal to toRange, allowing to use implicit conversion.
     * @return normal range
     */
    operator const Range () const { return Range (start().toCursor(), end().toCursor()); }

    /**
     * kDebug() stream operator. Writes this range to the debug output in a nicely formatted way.
     * @param s debug stream
     * @param cursor range to print
     * @return debug stream
     */
    inline friend QDebug operator<< (QDebug s, const MovingRange *range) {
      if (range)
        s << "[" << range->start() << " -> " << range->end() << "]";
      else
        s << "(null range)";
      return s.space();
    }

    /**
     * kDebug() stream operator. Writes this range to the debug output in a nicely formatted way.
     * @param s debug stream
     * @param range range to print
     * @return debug stream
     */
    inline friend QDebug operator<< (QDebug s, const MovingRange &range) {
      return s << &range;
    }

    /**
     * Returns true if this range contains no characters, ie. the start() and
     * end() positions are the same.
     *
     * \returns \e true if the range contains no characters, otherwise \e false
     */
    inline bool isEmpty() const {
      return start() == end();
    }

    //BEGIN comparison functions
    /**
     * \name Comparison
     *
     * The following functions perform checks against this range in comparison
     * to other lines, columns, cursors, and ranges.
     */
    /**
     * Check whether the this range wholly encompasses \e range.
     *
     * \param range range to check
     *
     * \return \e true, if this range contains \e range, otherwise \e false
     */
    inline bool contains(const Range& range) const {
      return range.start() >= start() && range.end() <= end();
    }

    /**
     * Check to see if \p cursor is contained within this range, ie >= start() and \< end().
     *
     * \param cursor the position to test for containment
     *
     * \return \e true if the cursor is contained within this range, otherwise \e false.
     */
    inline bool contains(const Cursor& cursor) const {
      return cursor >= start() && cursor < end();
    }

    /**
     * Returns true if this range wholly encompasses \p line.
     *
     * \param line line to check
     *
     * \return \e true if the line is wholly encompassed by this range, otherwise \e false.
     */
    inline bool containsLine(int line) const {
      return (line > start().line() || (line == start().line() && !start().column())) && line < end().line();
    }

    /**
     * Check whether the range contains \e column.
     *
     * \param column column to check
     *
     * \return \e true if the range contains \e column, otherwise \e false
     */
    inline bool containsColumn(int column) const {
      return column >= start().column() && column < end().column();
    }

    /**
     * Check whether the this range overlaps with \e range.
     *
     * \param range range to check against
     *
     * \return \e true, if this range overlaps with \e range, otherwise \e false
     */
    bool overlaps(const Range& range) const;

    /**
     * Check whether the range overlaps at least part of \e line.
     *
     * \param line line to check
     *
     * \return \e true, if the range overlaps at least part of \e line, otherwise \e false
     */
    inline bool overlapsLine(int line) const {
      return line >= start().line() && line <= end().line();
    }

    /**
     * Check to see if this range overlaps \p column; that is, if \p column is
     * between start().column() and end().column().  This function is most likely
     * to be useful in relation to block text editing.
     *
     * \param column the column to test
     *
     * \return \e true if the column is between the range's starting and ending
     *         columns, otherwise \e false.
     */
    inline bool overlapsColumn(int column) const {
      return start().column() <= column && end().column() > column;
    }
    
    /**
     * Check whether the start() and end() cursors of this range
     * are on the same line.
     *
     * \return \e true if both the start and end positions are on the same
     *         line, otherwise \e false
     */
    inline bool onSingleLine() const {
      return start().line() == end().line();
    }

    //END comparison functions
};

Q_DECLARE_OPERATORS_FOR_FLAGS(MovingRange::InsertBehaviors)

}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
