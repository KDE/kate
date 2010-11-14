/*  This file is part of the Kate project.
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

#ifndef KATE_TEXTRANGE_H
#define KATE_TEXTRANGE_H

#include <ktexteditor/view.h>
#include <ktexteditor/movingrange.h>
#include <ktexteditor/movingrangefeedback.h>

#include "katepartprivate_export.h"
#include "katetextcursor.h"

namespace Kate {

class TextBuffer;

/**
 * Class representing a 'clever' text range.
 * It will automagically move if the text inside the buffer it belongs to is modified.
 * By intention no subclass of KTextEditor::Range, must be converted manually.
 * A TextRange is allowed to be empty. If you call setInvalidateIfEmpty(true),
 * a TextRange will become automatically invalid as soon as start() == end()
 * position holds.
 */
class KATEPART_TESTS_EXPORT TextRange : public KTextEditor::MovingRange {
  // this is a friend, block changes might invalidate ranges...
  friend class TextBlock;

  public:
    /**
     * Construct a text range.
     * A TextRange is not allowed to be empty, as soon as start == end position, it will become
     * automatically invalid!
     * @param buffer parent text buffer
     * @param range The initial text range assumed by the new range.
     * @param insertBehavior Define whether the range should expand when text is inserted adjacent to the range.
     * @param emptyBehavior Define whether the range should invalidate itself on becoming empty.
     */
    TextRange (TextBuffer &buffer, const KTextEditor::Range &range, InsertBehaviors insertBehavior, EmptyBehavior emptyBehavior = AllowEmpty);

    /**
     * Destruct the text block
     */
    ~TextRange ();

    /**
     * Set insert behaviors.
     * @param insertBehaviors new insert behaviors
     */
    void setInsertBehaviors (InsertBehaviors insertBehaviors);

    /**
     * Get current insert behaviors.
     * @return current insert behaviors
     */
    InsertBehaviors insertBehaviors () const;

    /**
     * Set if this range will invalidate itself if it becomes empty.
     * @param emptyBehavior behavior on becoming empty
     */
    void setEmptyBehavior (EmptyBehavior emptyBehavior);

    /**
     * Will this range invalidate itself if it becomes empty?
     * @return behavior on becoming empty
     */
    EmptyBehavior emptyBehavior () const { return m_invalidateIfEmpty ? InvalidateIfEmpty : AllowEmpty; }

    /**
     * Gets the document to which this range is bound.
     * \return a pointer to the document
     */
    KTextEditor::Document *document () const;

    /**
     * Set the range of this range.
     * A TextRange is not allowed to be empty, as soon as start == end position, it will become
     * automatically invalid!
     * @param range new range for this clever range
     */
    void setRange (const KTextEditor::Range &range);

    /**
     * \overload
     * Set the range of this range
     * A TextRange is not allowed to be empty, as soon as start == end position, it will become
     * automatically invalid!
     * @param start new start for this clever range
     * @param end new end for this clever range
     */
    void setRange (const KTextEditor::Cursor &start, const KTextEditor::Cursor &end) { KTextEditor::MovingRange::setRange (start, end); }

    /**
     * Retrieve start cursor of this range, read-only.
     * @return start cursor
     */
    const KTextEditor::MovingCursor &start () const { return m_start; }

	/**
	 * Non-virtual version of start(), which is faster.
     * @return start cursor
	 */
    const TextCursor &startInternal() const { return m_start; }

    /**
     * Retrieve end cursor of this range, read-only.
     * @return end cursor
     */
    const KTextEditor::MovingCursor &end () const { return m_end; }

    /**
     * Nonvirtual version of end(), which is faster.
     * @return end cursor
     */
    const TextCursor &endInternal () const { return m_end; }

    /**
     * Convert this clever range into a dumb one.
     * @return normal range
     */
    const KTextEditor::Range toRange () const { return KTextEditor::Range (start().toCursor(), end().toCursor()); }

    /**
     * Convert this clever range into a dumb one. Equal to toRange, allowing to use implicit conversion.
     * @return normal range
     */
    operator const KTextEditor::Range () const { return KTextEditor::Range (start().toCursor(), end().toCursor()); }

    /**
     * Gets the active view for this range. Might be already invalid, internally only used for pointer comparisons.
     *
     * \return a pointer to the active view
     */
    KTextEditor::View *view () const { return m_view; }

    /**
     * Sets the currently active view for this range.
     * This will trigger update of the relevant view parts, if the view changed.
     * Set view before the attribute, that will avoid not needed redraws.
     *
     * \param attribute View to assign to this range. If null, simply
     *                  removes the previous view.
     */
    void setView (KTextEditor::View *view);

    /**
     * Gets the active Attribute for this range.
     *
     * \return a pointer to the active attribute
     */
    KTextEditor::Attribute::Ptr attribute () const { return m_attribute; }

    /**
	 * \return whether a nonzero attribute is set. This is faster than checking attribute(),
	 *             because the reference-counting is omitted.
	 */
    bool hasAttribute() const { return !m_attribute.isNull(); }

    /**
     * Sets the currently active attribute for this range.
     * This will trigger update of the relevant view parts.
     *
     * \param attribute Attribute to assign to this range. If null, simply
     *                  removes the previous Attribute.
     */
    void setAttribute (KTextEditor::Attribute::Ptr attribute);

    /**
     * Gets the active MovingRangeFeedback for this range.
     *
     * \return a pointer to the active MovingRangeFeedback
     */
    KTextEditor::MovingRangeFeedback *feedback () const { return m_feedback; }

    /**
     * Sets the currently active MovingRangeFeedback for this range.
     * This will trigger evaluation if feedback must be send again (for example if mouse is already inside range).
     *
     * \param attribute MovingRangeFeedback to assign to this range. If null, simply
     *                  removes the previous MovingRangeFeedback.
     */
    void setFeedback (KTextEditor::MovingRangeFeedback *feedback);

    /**
     * Is this range's attribute only visible in views, not for example prints?
     * Default is false.
     * @return range visible only for views
     */
    bool attributeOnlyForViews () const { return m_attributeOnlyForViews; }

    /**
     * Set if this range's attribute is only visible in views, not for example prints.
     * @param onlyForViews attribute only valid for views
     */
    void setAttributeOnlyForViews (bool onlyForViews);
    
    /**
     * Gets the current Z-depth of this range.
     * Ranges with smaller Z-depth than others will win during rendering.
     * Default is 0.0.
     * 
     * \return current Z-depth of this range
     */
    qreal zDepth () const { return m_zDepth; }
    
    /**
     * Set the current Z-depth of this range.
     * Ranges with smaller Z-depth than others will win during rendering.
     * This will trigger update of the relevant view parts, if the depth changed.
     * Set depth before the attribute, that will avoid not needed redraws.
     * Default is 0.0.
     * 
     * \param zDepth new Z-depth of this range
     */
    void setZDepth (qreal zDepth);

  private:
    /**
     * Check if range is valid, used by constructor and setRange.
     * If at least one cursor is invalid, both will set to invalid.
     * Same if range itself is invalid (start >= end).
     * @param oldStartLine old start line of this range before changing of cursors, needed to add/remove range from m_ranges in blocks
     * @param oldEndLine old end line of this range
     * @param notifyAboutChange should feedback be emited or not?
     */
    void checkValidity (int oldStartLine = -1, int oldEndLine = -1, bool notifyAboutChange = true);

    /**
     * Add/Remove range from the lookup m_ranges hash of each block
     * @param oldStartLine old start line of this range before changing of cursors, needed to add/remove range from m_ranges in blocks
     * @param oldEndLine old end line of this range
     * @param startLine start line to start looking for the range to remove
     * @param endLine end line of this range
     */
    void fixLookup (int oldStartLine, int oldEndLine, int startLine, int endLine);

  private:
    /**
     * parent text buffer
     * is a reference, and no pointer, as this must always exist and can't change
     */
    TextBuffer &m_buffer;

    /**
     * Start cursor for this range, is a clever cursor
     */
    TextCursor m_start;

    /**
     * End cursor for this range, is a clever cursor
     */
    TextCursor m_end;

    /**
     * The view for which the attribute is valid, 0 means any view
     */
    KTextEditor::View *m_view;

    /**
     * This range's current attribute.
     */
    KTextEditor::Attribute::Ptr m_attribute;

    /**
     * pointer to the active MovingRangeFeedback
     */
    KTextEditor::MovingRangeFeedback *m_feedback;
    
    /**
     * Z-depth of this range for rendering
     */
    qreal m_zDepth;

    /**
     * Is this range's attribute only visible in views, not for example prints?
     */
    bool m_attributeOnlyForViews;

    /**
     * Will this range invalidate itself if it becomes empty?
     */
    bool m_invalidateIfEmpty;
};

}

#endif
