/* This file is part of the KDE project
   Copyright (C) 2003-2005 Hamish Rodda <rodda@kde.org>
   Copyright (C) 2008 David Nolden <david.nolden.kdevelop@art-master.de>

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

#ifndef KDELIBS_KTEXTEDITOR_SMARTRANGE_H
#define KDELIBS_KTEXTEDITOR_SMARTRANGE_H

#include <ktexteditor/ktexteditor_export.h>
#include <ktexteditor/range.h>
#include <ktexteditor/smartcursor.h>
#include <ktexteditor/attribute.h>

#include <QtCore/QList>

#ifndef DOXYGEN_SHOULD_SKIP_THIS
template <class T> class QStack;
#endif

class KAction;

namespace KTextEditor
{
class SmartRangeNotifier;
class SmartRangeWatcher;

/**
 * \short A Range which is bound to a specific Document, and maintains its position.
 *
 * \ingroup kte_group_smart_classes
 *
 * A SmartRange is an extension of the basic Range class. It maintains its
 * position in the document and provides extra functionality,
 * including:
 * \li convenience functions for accessing and manipulating the content
 * of the associated document,
 * \li adjusting behavior in response to text edits,
 * \li forming a tree structure out of multiple SmartRange%s,
 * \li providing attribute information for the arbitrary highlighting extension,
 * \li allowing KAction%s to be bound to the range (note: not currently implemented), and
 * \li providing notification of changes to 3rd party software.
 *
 * As a result of a smart range's close association with a document, and the processing
 * that occurrs as a result, smart ranges may not be copied.
 *
 * For simplicity of code, ranges always maintain their start position to
 * be before or equal to their end position.  Attempting to set either the
 * start or end of the range beyond the respective end or start will result in
 * both values being set to the specified position.
 *
 * Hierarchical range-trees maintain specific relationships:
 * - When a child-range is changed, all parent-ranges are resized so the child-range fits in
 * - When a parent-range is changed, all child-ranges are resized so they fit in
 *
 * This means that parent-ranges always completely contain all their child-ranges. However
 * it may lead to unexpected range-changes from the perspective of your application, so keep
 * this in mind.
 *
 * The child-ranges of one smart-range are allowed to overlap each other. However overlaps
 * should be omitted where possible, for performance-reasons, and because each range can
 * be overlapped by max. 63 sibling-ranges while still rendering correctly.
 *
 * Create a new SmartRange like this:
 * \code
 * // Retrieve the SmartInterface
 * KTextEditor::SmartInterface* smart =
 *     qobject_cast<KTextEditor::SmartInterface*>( yourDocument );
 *
 * if ( smart ) {
 *     KTextEditor::SmartRange* range = smart->newSmartRange();
 * }
 * \endcode
 *
 * When finished with a SmartRange, simply delete it.
 *
 * \sa Range, SmartRangeNotifier, SmartRangeWatcher, and SmartInterface
 *
 * \author Hamish Rodda \<rodda@kde.org\>
 */
class KTEXTEDITOR_EXPORT SmartRange : public Range
{
  friend class SmartCursor;

  public:
    /// Determine how the range reacts to characters inserted immediately outside the range.
    enum InsertBehavior {
      /// Don't expand to encapsulate new characters in either direction. This is the default.
      DoNotExpand = 0,
      /// Expand to encapsulate new characters to the left of the range.
      ExpandLeft = 0x1,
      /// Expand to encapsulate new characters to the right of the range.
      ExpandRight = 0x2
    };
    Q_DECLARE_FLAGS(InsertBehaviors, InsertBehavior)

    virtual ~SmartRange();

    /**
     * Returns that this range is a SmartRange.
     */
    virtual bool isSmartRange() const;

    /**
     * Returns this range as a SmartRange, if it is one.
     */
    virtual SmartRange* toSmartRange() const;

    /**
     * \name Position
     *
     * The following functions provide access and manipulation of the range's position.
     * \{
     */
    /**
     * \copydoc Range::setRange(const Range&)
     *
     * This function also provides any required adjustment of parent and child ranges,
     * and notification of the change if required.
     */
    virtual void setRange(const Range& range);

    /**
     * Get the start point of this range. This version returns a casted
     * version of start(), as SmartRange%s always use SmartCursor%s as
     * the start() and end().
     *
     * \returns a reference to the start of this range.
     */
    inline SmartCursor& smartStart()
      { return *static_cast<SmartCursor*>(m_start); }

    /**
     * Get the start point of this range. This version returns a casted
     * version of start(), as SmartRange%s always use SmartCursor%s as
     * the start() and end().
     *
     * \returns a const reference to the start of this range.
     */
    inline const SmartCursor& smartStart() const
      { return *static_cast<const SmartCursor*>(m_start); }

    /**
     * Get the end point of this range. This version returns a casted
     * version of end(), as SmartRange%s always use SmartCursor%s as
     * the start() and end().
     *
     * \returns a reference to the end of this range.
     */
    inline SmartCursor& smartEnd()
      { return *static_cast<SmartCursor*>(m_end); }

    /**
     * Get the end point of this range. This version returns a casted
     * version of end(), as SmartRange%s always use SmartCursor%s as
     * the start() and end().
     *
     * \returns a const reference to the end of this range.
     */
    inline const SmartCursor& smartEnd() const
      { return *static_cast<const SmartCursor*>(m_end); }

    /**
     * \overload confineToRange(const Range&)
     * Overloaded version which confines child ranges as well.
     */
    virtual bool confineToRange(const Range& range);

    /**
     * \overload expandToRange(const Range&)
     * Overloaded version which expands child ranges as well.
     */
    virtual bool expandToRange(const Range& range);

    //BEGIN Functionality present from having this range associated with a Document
    /**
     * \}
     *
     * \name Document-related functions
     *
     * The following functions are provided for convenient access to the
     * associated Document.
     * \{
     */
    /**
     * Retrieve the document associated with this SmartRange.
     *
     * \return a pointer to the associated document
     */
    Document* document() const;

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
     * \param text text to use as a replacement
     * \param block insert this text as a visual block of text rather than a linear sequence
     * \return \e true on success, otherwise \e false
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
    //END

    //BEGIN Behavior
    /**
     * \}
     *
     * \name Behavior
     *
     * The following functions relate to the behavior of this SmartRange.
     * \{
     */
    /**
     * Returns how this range reacts to characters inserted immediately outside the range.
     *
     * \return the current insert behavior.
     */
    InsertBehaviors insertBehavior() const;

    /**
     * Determine how the range should react to characters inserted immediately outside the range.
     *
     * \todo does this need a custom function to enable determining of the behavior based on the
     * text that is inserted / deleted?
     *
     * \param behavior the insertion behavior to use for future edits
     *
     * \sa InsertBehavior
     */
    void setInsertBehavior(InsertBehaviors behavior);
    //END

    //BEGIN Relationships to other ranges
    /**
     * \}
     *
     * \name Tree structure
     *
     * The following functions relate to the tree structure functionality.
     * \{
     */
    /**
     * Returns this range's parent range, if one exists.
     *
     * At all times, this range will be contained within parentRange().
     *
     * \return a pointer to the current parent range
     */
    inline SmartRange* parentRange() const
      { return m_parentRange; }

    /**
     * Set this range's parent range.
     *
     * At all times, this range will be contained within parentRange().  So, if it is outside of the
     * new parent to begin with, it will be expanded automatically.
     *
     * When being inserted into the parent range, the parent range will be fit in between any other
     * pre-existing child ranges, and may resize them so as not to overlap.  However, once insertion
     * has occurred, changing this range directly will only resize the others, it will \e not change
     * the order of the ranges.  To change the order, unset the parent range, change the range, and
     * re-set the parent range.
     *
     * \param r range to become the new parent of this range
     */
    virtual void setParentRange(SmartRange* r);

    /**
     * Determine whether \a parent is a parent of this range.
     *
     * \param parent range to check to see if it is a parent of this range.
     *
     * \return \e true if \a parent is in the parent heirachy, otherwise \e false.
     */
    bool hasParent(SmartRange* parent) const;

    /**
     * Calculate the current depth of this range.
     *
     * \return the depth of this range, where 0 is no parent, 1 is one parent, etc.
     */
    inline int depth() const
      { return m_parentRange ? m_parentRange->depth() + 1 : 0; }

    /**
     * Returns the range's top parent range, or this range if there are no parents.
     *
     * \return a pointer to the top parent range
     */
    inline SmartRange* topParentRange() const
      { return parentRange() ? parentRange()->topParentRange() : const_cast<SmartRange*>(this); }

    /**
     * Get the ordered list of child ranges.
     *
     * To insert a child range, simply set its parent to this range using setParentRange().
     *
     * \returns a list of child ranges.
     */
    const QList<SmartRange*>& childRanges() const;

    /**
     * Clears child ranges - i.e., removes the text that is covered by the ranges.
     * The ranges themselves are not deleted.
     *
     * \sa removeText()
     */
    void clearChildRanges();

    /**
     * Deletes child ranges - i.e., deletes the SmartRange objects only.
     * The underlying text is not affected.
     */
    void deleteChildRanges();

    /**
     * Clears child ranges - i.e., clears the text that is covered by the ranges,
     * and deletes the SmartRange objects.
     */
    void clearAndDeleteChildRanges();

    /**
     * Find the child before \p range, if any.
     * The order is determined by the range end-cursors.
     *
     * \param range to seach backwards from
     *
     * \return the range before \p range if one exists, otherwise null.
     */
    SmartRange* childBefore( const SmartRange * range ) const;

    /**
     * Find the child after \p range, if any.
     * The order is determined by the range end-cursors.
     *
     * \param range to seach forwards from
     *
     * \return the range after \p range if one exists, otherwise null.
     */
    SmartRange* childAfter( const SmartRange * range ) const;

    /**
     * Finds the most specific range in a heirachy for the given input range
     * (ie. the smallest range which wholly contains the input range)
     *
     * In case of overlaps, the smallest containing range is chosen,
     * if there are multiple of the same size, then the first one.
     *
     * \param input the range to use in searching
     *
     * \return the deepest range which contains \p input
     */
    SmartRange* mostSpecificRange(const Range& input) const;

    /**
     * Finds the first child range which contains position \p pos.
     *
     * \param pos the cursor position to use in searching
     *
     * \return the most shallow range (from and including this range) which
     *         contains \p pos
     */
    SmartRange* firstRangeContaining(const Cursor& pos) const;

    /**
     * Finds the deepest range in the heirachy which contains position \p pos.
     * Allows the caller to determine which ranges were entered and exited
     * by providing pointers to QStack<SmartRange*>.
     *
     * If child-ranges overlap in the given position,
     * the first smallest one is returned.
     * 
     * \param pos the cursor position to use in searching
     * \param rangesEntered provide a QStack<SmartRange*> here to find out
     *                      which ranges were entered during the traversal.
     *                      The top item was the first descended.
     * \param rangesExited provide a QStack<SmartRange*> here to find out
     *                     which ranges were exited during the traversal.
     *                     The top item was the first exited.
     *
     * \return the deepest range (from and including this range) which
     *         contains \p pos, or null if no ranges contain this position.
     */
    SmartRange* deepestRangeContaining(const Cursor& pos,
                                       QStack<SmartRange*>* rangesEntered = 0L,
                                       QStack<SmartRange*>* rangesExited = 0L) const;

    QList<SmartRange*> deepestRangesContaining(const Cursor& pos) const;
    
    /**
     * Returns the count of ranges within the parent-range
     * that end behind this range, and that overlap this range.
     */
    int overlapCount() const;
    //END

    //BEGIN Arbitrary highlighting
    /**
     * \}
     *
     * \name Arbitrary highlighting
     *
     * The following functions relate to arbitrary highlighting capabilities.
     * \{
     */
    /**
     * Gets the active Attribute for this range.
     *
     * \return a pointer to the active attribute
     */
    Attribute::Ptr attribute() const;

    /**
     * Sets the currently active attribute for this range.
     *
     * \param attribute Attribute to assign to this range. If null, simply
     *                  removes the previous Attribute.
     * 
     * \note \ref SmartInterface::addHighlightToDocument must be called with the top-range before
     *       the highlighting can work.
     */
    void setAttribute(Attribute::Ptr attribute);
    //END

    //BEGIN Action binding
    /**
     * \}
     *
     * \name Action binding
     *
     * The following functions relate to action binding capabilities.
     *
     * \note This feature is currently not implemented (ETA KDE 4.1).
     * \{
     */
    /**
     * Associate an action with this range.  The associated action(s) will be
     * enabled when the caret enters the range, and disabled them on exit.
     * The action is also added to the context menu when the mouse/caret is within
     * an associated range.
     *
     * \param action KAction to associate with this range
     */
    void associateAction(KAction* action);

    /**
     * Remove the association with an action from this range; it will no
     * longer be managed.
     *
     * \param action KAction to dissociate from this range
     */
    void dissociateAction(KAction* action);

    /**
     * Access the list of currently associated KAction%s.
     *
     * \return the list of associated actions
     */
    const QList<KAction*>& associatedActions() const
      { return m_associatedActions; }

    /**
     * Clears all associations between KAction%s and this range.
     */
    void clearAssociatedActions();
    //END

    //BEGIN Notification methods
    /**
     * \}
     *
     * \name Notification
     *
     * The following functions allow for changes related to this range to be
     * notified to 3rd party programs.
     * \{
     */
    /**
     * Connect to a notifier to receive signals indicating change of state of this range.
     * This function creates a notifier if none is already bound to this range; if one has
     * already been assigned this will return the first notifier.
     *
     * If you have finished with notifications for a reasonable period of time you can
     * save memory by calling deleteNotifier().
     */
    SmartRangeNotifier* primaryNotifier();

    /**
     * Returns a list of notifiers which are receiving signals indicating change of state
     * of this range.  These notifiers may be receiving signals from other ranges as well.
     */
    const QList<SmartRangeNotifier*> notifiers() const;

    /**
     * Register a notifier to receive signals indicating change of state of this range.
     *
     * \param notifier notifier to register. Ownership is not transferred.
     */
    void addNotifier(SmartRangeNotifier* notifier);

    /**
     * Deregister a notifier and no longer deliver signals indicating change of state of this range.
     *
     * \param notifier notifier to deregister.
     */
    void removeNotifier(SmartRangeNotifier* notifier);

    /**
     * When finished with the primaryNotifier(), call this method to save memory by
     * having the SmartRangeNotifier deleted.
     *
     * \note If a notifier was first registered via addNotifier() rather than created inside
     *       primaryNotifier(), this method will delete that notifier.  Text editor implementations
     *       should not use notifiers for internal purposes, instead use watchers (faster and
     *       has documentation to this effect)
     */
    void deletePrimaryNotifier();

    /**
     * Returns a list of registered SmartRangeWatchers.
     *
     * \note this function may return watchers internal to the text editor's implementation,
     *       eg. in the case of arbitrary highlighting and kate part.  Removing these watchers
     *       with removeWatcher() will result in malfunction.
     */
    const QList<SmartRangeWatcher*>& watchers() const;

    /**
     * Register a SmartRangeWatcher to receive calls indicating change of state
     * of this range. To finish receiving notifications, call removeWatcher().
     *
     * \param watcher the instance of a class which is to receive
     *                notifications about changes to this range.
     */
    void addWatcher(SmartRangeWatcher* watcher);

    /**
     * Stop delivery of notifications to a SmartRangeWatcher.
     *
     * \param watcher the watcher that no longer wants notifications.
     */
    void removeWatcher(SmartRangeWatcher* watcher);
    //!\}
    //END

    /**
     * Assignment operator. Assigns the current position of the provided range, \p rhs, only;
     * does not assign watchers, notifiers, behavior etc.
     *
     * \note The assignment will be performed even if the provided range belongs to
     *       another Document.
     *
     * \param rhs range to assign to this range.
     *
     * \return a reference to this range, after assignment has occurred.
     *
     * \see setRange()
     */
    inline SmartRange& operator=(const SmartRange& rhs)
      { setRange(rhs); return *this; }

    /**
     * Assignment operator. Assigns the current position of the provided range, \p rhs.
     *
     * \param rhs range to assign to this range.
     *
     * \return a reference to this range, after assignment has occurred.
     *
     * \see setRange()
     */
    inline SmartRange& operator=(const Range& rhs)
      { setRange(rhs); return *this; }

  protected:
    /**
     * Constructor for subclasses to utilise.  Protected to prevent direct
     * instantiation.
     *
     * \note 3rd party developers: you do not (and should not) need to subclass
     *       the Smart* classes; instead, use the SmartInterface to create instances.
     *
     * \internal
     *
     * \param start the start cursor to use - ownership is taken
     * \param end the end cursor to use - ownership is taken
     * \param parent the parent range if this is a subrange of another range
     * \param insertBehavior the behavior of this range when an insert happens
     *                        immediately outside the range.
     */
    SmartRange(SmartCursor* start, SmartCursor* end, SmartRange* parent = 0L, InsertBehaviors insertBehavior = DoNotExpand);

    /**
     * \internal
     *
     * Notify this range that one or both of the cursors' position has changed directly.
     *
     * \param cursor the cursor that changed. If null, both cursors have changed.
     * \param from the previous position of this range
     */
    virtual void rangeChanged(Cursor* cursor, const Range& from);

    /**
     * \internal
     *
     * This routine is called when the range changes how much feedback it may need, eg. if it adds an action.
     */
    virtual void checkFeedback();

    /**
     * \internal
     *
     * Called to request creation of a new SmartRangeNotifier for this object.
     */
    virtual SmartRangeNotifier* createNotifier() = 0;

    /**
     * Is called after child-ranges have changed internally without the rangeChanged() notification, for example
     * after translations. It rebuilds the child-structure, so it is consistent again.
     */
    void rebuildChildStructure();
    
  private:
    /**
     * \internal
     * Copy constructor: Disable copying of this class.
     */
    SmartRange(const SmartRange&);

    /**
     * \internal
     * Implementation of deepestRangeContaining().
     */
    SmartRange* deepestRangeContainingInternal(const Cursor& pos,
                                               QStack<SmartRange*>* rangesEntered,
                                               QStack<SmartRange*>* rangesExited,
                                               bool first = false) const;

    /**
     * \internal
     *
     * New child classes call this to register themselves.
     */
    void insertChildRange(SmartRange* newChild);

    /**
     * \internal
     *
     * Disassociating child classes call this to de-register themselves.
     */
    void removeChildRange(SmartRange* newChild);

    /**
     * \internal
     *
     * This range's current attribute.
     */
    Attribute::Ptr m_attribute;

    SmartRange* m_parentRange;

    /**
     * \internal
     *
     * The list of this range's child ranges, sorted by end-cursor.
     */
    QList<SmartRange*> m_childRanges;

    /**
     * \internal
     *
     * The list of this range's associated KAction%s.
     */
    QList<KAction*> m_associatedActions;

    /**
     * \internal
     *
     * The list of registered SmartRangeNotifiers.
     */
    QList<SmartRangeNotifier*> m_notifiers;

    /**
     * \internal
     *
     * The list of registered SmartRangeWatchers.
     */
    QList<SmartRangeWatcher*> m_watchers;

    /**
     * \internal
     *
     * Whether this range owns the currently assigned attribute or not.
     */
    bool m_ownsAttribute :1;
    /**
     * \internal
     *
     * How many ranges that end behind this one at least partially overlap it.
     * Currently max. 64 overlaps are allowed.
     */
    uchar m_overlapCount:6;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(SmartRange::InsertBehaviors)

}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
