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

#ifndef KDELIBS_KTEXTEDITOR_SMARTRANGE_H
#define KDELIBS_KTEXTEDITOR_SMARTRANGE_H

#include <ktexteditor/range.h>
#include <ktexteditor/smartcursor.h>

#include <QList>

class KAction;

namespace KTextEditor
{
class Attribute;
class SmartRangeNotifier;
class SmartRangeWatcher;

/**
 * \short A Range which is bound to a specific Document, and maintains its position.
 *
 * A SmartRange is an extension of the basic Range class. It maintains its
 * position in the document and provides a number of convenience methods,
 * including those for accessing and manipulating the content of the associated
 * Document.  As a result of this, SmartRanges may not be copied, as they need
 * to maintain a connection to the assicated Document.
 *
 * For simplicity of code, ranges always maintain their start position to
 * be before or equal to their end position.  Attempting to set either the
 * start or end of the range beyond the respective end or start will result in
 * both values being set to the specified position.
 *
 * \sa Range, SmartRangeNotifier, SmartRangeWatcher
 */
class KTEXTEDITOR_EXPORT SmartRange : public Range
{
  friend class SmartCursor;

  public:
    /// Determine how the range reacts to characters inserted immediately outside the range.
    enum InsertBehaviour {
      /// Don't expand to encapsulate new characters in either direction. This is the default.
      DoNotExpand = 0,
      /// Expand to encapsulate new characters to the left of the range.
      ExpandLeft = 0x1,
      /// Expand to encapsulate new characters to the right of the range.
      ExpandRight = 0x2
    };
    Q_DECLARE_FLAGS(InsertBehaviours, InsertBehaviour);

    virtual ~SmartRange();

    // Overload
    virtual void setRange(const Range& range);

    // BEGIN Functionality present from having this range associated with a Document
    /**
     * Retrieve the document associated with this SmartRange.
     */
    Document* document() const;

    SmartCursor& smartStart() { return *static_cast<SmartCursor*>(m_start); }
    const SmartCursor& smartStart() const { return *static_cast<const SmartCursor*>(m_start); }

    SmartCursor& smartEnd() { return *static_cast<SmartCursor*>(m_end); }
    const SmartCursor& smartEnd() const { return *static_cast<const SmartCursor*>(m_end); }

    /**
     * Retrieve the text which is contained within this range.
     *
     * \param block specify whether the text should be returned from the range's visual block, rather
     *              than all the text within the range.
     */
    virtual QStringList text(bool block = false) const;

    /**
     * Replace text in this range with \p text
     *
     * @param text text to use as a replacement
     * @param block insert this text as a visual block of text rather than a linear sequence
     * @return @e true on success, otherwise @e false
     */
    virtual bool replaceText(const QStringList &text, bool block = false);

    /**
     * Remove text contained within this range.
     * The range itself will not be deleted.
     *
     * \param block specify whether the text should be deleted from the range's visual block, rather
     *              than all the text within the range.
     */
    virtual bool removeText(bool block = false);
    // END

    // BEGIN Behaviour
    /**
     * Returns how this range reacts to characters inserted immediately outside the range.
     */
    InsertBehaviours insertBehaviour() const;

    /**
     * Determine how the range should react to characters inserted immediately outside the range.
     *
     * TODO does this need a custom function to enable determining of the behavior based on the
     * text that is inserted / deleted?
     *
     * @sa InsertBehaviour
     */
    void setInsertBehaviour(InsertBehaviours behaviour);
    // END

    // BEGIN Relationships to other ranges
    /**
     * Returns this range's parent range, if one exists.
     *
     * At all times, this range will be contained within parentRange().
     */
    inline SmartRange* parentRange() const { return m_parentRange; }

    /**
     * Set this range's parent range.
     *
     * At all times, this range will be contained within parentRange().  So, if it is outside of the
     * new parent, it will be constrained automatically.
     */
    virtual void setParentRange(SmartRange* r);

    /// Overloaded to confine child ranges as well.
    virtual void confineToRange(const Range& range);
    /// Overloaded to expand parent ranges.
    virtual void expandToRange(const Range& range);

    inline int depth() const { return m_parentRange ? m_parentRange->depth() + 1 : 0; }

    const QList<SmartRange*>& childRanges() const;
    void clearChildRanges();
    void deleteChildRanges();
    SmartRange* childBefore( const SmartRange * range ) const;
    SmartRange* childAfter( const SmartRange * range ) const;

    /**
     * Finds the most specific range in a heirachy for the given input range
     * (ie. the smallest range which wholly contains the input range)
     */
    SmartRange* findMostSpecificRange(const Range& input) const;

    /**
     * Finds the first child range which contains position \p pos.
     */
    SmartRange* firstRangeIncluding(const Cursor& pos) const;

    /**
     * Finds the deepest child range which contains position \p pos.
     */
    SmartRange* deepestRangeContaining(const Cursor& pos) const;
    // END

    // BEGIN Arbitrary highlighting
    /**
     * Gets the active Attribute for this range.  If one was set directly, it will be returned.
     * If not, when there is an attributeGroup() defined for this range, and followActiveGroup() is true,
     * the currently active Attribute from the attributeGroup() will be returned.
     */
    Attribute* attribute() const;

    /**
     * Sets the currently active attribute for this range.
     *
     * \param attribute Attribute to assign to this range.  If this is null, the AttributeGroup system will take over.
     * \param ownsAttribute Set to true when this object should take ownership of \p attribute.
     *                      If true, \p attribute will be deleted when this cursor is deleted.
     */
    virtual void setAttribute(Attribute* attribute, bool ownsAttribute = false);
    // END

    // BEGIN Action binding
    /**
     * Attach an action to this range.  This will enable the attached action(s) when the caret
     * enters the range, and disable them on exit.  The action is also added to the context menu when
     * the caret is within the range.
     */
    void attachAction(KAction* action);

    /**
     * Remove an action from this range.
     */
    void detachAction(KAction* action);

    /**
     * Returns a list of currently associated KActions.
     */
    const QList<KAction*>& associatedActions() const { return m_associatedActions; }
    // END

    // BEGIN Notification methods
    /**
     * Connect to the notifier to receive signals indicating change of state of this range.
     * The notifier is created at the time it is first requested.  If you have finished with
     * notifications for a reasonable period of time you can save memory by calling deleteNotifier().
     */
    virtual bool hasNotifier() const = 0;

    /**
     * Connect to the notifier to receive signals indicating change of state of this range.
     * The notifier is created at the time it is first requested.  If you have finished with
     * notifications for a reasonable period of time you can save memory by calling deleteNotifier().
     */
    virtual SmartRangeNotifier* notifier() = 0;

    /**
     * When finished with a notifier(), call this method to save memory by
     * having the SmartRangeNotifier deleted.
     */
    virtual void deleteNotifier() = 0;

    /**
     * Provide a SmartRangeWatcher to receive calls indicating change of state of this range.
     * To finish receiving notifications, call this function with \p watcher set to 0L.
     * \param watcher the class which will receive notifications about changes to this range.
     */
    virtual SmartRangeWatcher* watcher() const = 0;

    /**
     * Provide a SmartRangeWatcher to receive calls indicating change of state of this range.
     * To finish receiving notifications, call this function with \p watcher set to 0L.
     * \param watcher the class which will receive notifications about changes to this range.
     */
    virtual void setWatcher(SmartRangeWatcher* watcher) = 0;
    // END

    /**
     * \internal
     *
     * Notify this range that one or both of the cursors' position has changed directly.
     *
     * \param cursor the cursor that changed. If 0L, both cursors have changed.
     */
    virtual void cursorChanged(Cursor* cursor);

  protected:
    SmartRange(SmartCursor* start, SmartCursor* end, SmartRange* parent = 0L, InsertBehaviours insertBehaviour = DoNotExpand);

    /**
     * Implementation detail.
     * This routine is called when the range changes how much feedback it may need, eg. if it adds an action.
     */
    virtual void checkFeedback() = 0;

  private:
    Q_DISABLE_COPY(SmartRange)

    void insertChildRange(SmartRange* newChild);
    void removeChildRange(SmartRange* newChild);

    Attribute* m_attribute;
    SmartRange* m_parentRange;
    QList<SmartRange*> m_childRanges;
    QList<KAction*> m_associatedActions;

    bool              m_ownsAttribute     :1;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(SmartRange::InsertBehaviours);

}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
