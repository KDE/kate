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

#ifndef KDELIBS_KTEXTEDITOR_RANGE_H
#define KDELIBS_KTEXTEDITOR_RANGE_H

#include <ktexteditor/cursor.h>

class KAction;

namespace KTextEditor
{
class Attribute;

/**
 * \short A Range represents a section of text, from one Cursor to another.
 *
 * A Range is a basic class which represents a range of text with two Cursors,
 * from a start() position to an end() position.
 *
 * For simplicity and convenience, ranges always maintain their start position to
 * be before or equal to their end position.  Attempting to set either the
 * start or end of the range beyond the respective end or start will result in
 * both values being set to the specified position.  In the constructor, the
 * start and end will be swapped if necessary.
 *
 * If you want additional functionality such as the ability to maintain positon
 * in a document, see SmartRange.
 *
 * \sa SmartRange
 */
class KTEXTEDITOR_EXPORT Range
{
  friend class Cursor;

  public:
    /**
     * The default constructor creates a range from position (0, 0) to
     * position (0, 0).
     */
    Range();

    /**
     * Constructor.
     * Creates a range from @e start to @e end.
     * If start is after end, they will be swapped.
     * @param start start position
     * @param end end position
     */
    Range(const Cursor& start, const Cursor& end);

    /**
     * Constructor
     * Creates a single-line range from \p start which extends \p width characters along the same line.
     */
    Range(const Cursor& start, int width);

    /**
     * Constructor
     * Creates a range from \p start to \p endLine, \p endCol.
     */
    Range(const Cursor& start, int endLine, int endCol);

    /**
     * Constructor.
     * Creates a range from @e startLine, @e startCol to @e endLine, @e endCol.
     * @param startLine start line
     * @param startCol start column
     * @param endLine end line
     * @param endCol end column
     */
    Range(int startLine, int startCol, int endLine, int endCol);

    /**
     * Copy constructor
     */
    Range(const Range& copy);

    /**
     * Virtual destructor
     */
    virtual ~Range();

    /**
     * Validity check.  In the base class, returns true unless the range starts before (0,0).
     */
    virtual bool isValid() const;

    /**
     * Returns an invalid range.
     */
    static const Range& invalid();

    /**
     * Get the start point of this range. This will always be <= end().
     *
     * This non-const function allows direct manipulation of start(), while still retaining
     * notification support.
     *
     * If start() is set to a position after end(), end() will be moved to the
     * same position as start(), as ranges are not allowed to have
     * start() > end().
     */
    inline Cursor& start() { return *m_start; }

    /**
     * Get the start point of this range. This will always be <= end().
     */
    inline const Cursor& start() const { return *m_start; }

    /**
     * Get the end point of this range. This will always be >= start().
     *
     * This non-const function allows direct manipulation of end(), while still retaining
     * notification support.
     *
     * If end() is set to a position before start(), start() will be moved to the
     * same position as end(), as ranges are not allowed to have
     * start() > end().
     */
    inline Cursor& end() { return *m_end; }

    /**
     * Get the end point of this range. This will always be >= start().
     */
    inline const Cursor& end() const { return *m_end; }

    /**
     * Convenience function.  Set the start and end lines to \p line.
     *
     * \param line the line number to assign to start() and end()
     */
    inline void setBothLines(int line) { setRange(Range(line, start().column(), line, end().column())); }

    /**
     * Set the start and end cursors to @e range.
     * @param range new range
     */
    virtual void setRange(const Range& range);

    /**
     * @overload void setRange(const Range& range)
     * If @e start is after @e end, they will be reversed.
     * @param start start cursor
     * @param end end cursor
     */
    virtual void setRange(const Cursor& start, const Cursor& end);

    /**
     * Expand this range if necessary to contain \p range.
     *
     * \param range range which this range should contain
     */
    virtual void expandToRange(const Range& range);

    /**
     * Confine this range if neccessary to fit within \p range.
     *
     * \param range range which should contain this range
     */
    virtual void confineToRange(const Range& range);

    // TODO: produce int versions with -1 before, 0 true, and +1 after if there is a need
    /**
     * Returns true if this range wholly encompasses \p line.
     */
    bool containsLine(int line) const;

    /**
     * Check whether the range includes at least part of @e line.
     * @param line line to check
     * @return @e true, if the range includes at least part of @e line, otherwise @e false
     */
    bool includesLine(int line) const;

    /**
     * Returns true if this range spans \p colmun.
     */
    bool spansColumn(int column) const;

    /**
     * Returns true if \p cursor is wholly contained within this range, ie >= start() and \< end().
     * \param cursor Cursor to test for containment
     */
    bool contains(const Cursor& cursor) const;

    /**
     * Check whether the range includes @e column.
     * @param column column to check
     * @return @e true, if the range includes @e column, otherwise @e false
     * \todo should be contains?
     */
    bool includesColumn(int column) const;
    /**
     * Check whether the range includes @e cursor. Returns
     * - -1 if @e cursor < @p start()
     * - 0 if @p start() <= @e cursor <= @p end()
     * - 1 if @e cursor > @p end()
     * @param line line to check
     * @return depending on the case either -1, 0 or 1
     * \todo should be contains?
     */
    int includes(const Cursor& cursor) const;

    /**
     * Check whether the this range contains @e range.
     * @param range range to check
     * @return @e true, if this range contains @e range, otherwise @e false
     */
    bool contains(const Range& range) const;
    /**
     * Check whether the this range overlaps with @e range.
     * @param range range to check against
     * @return @e true, if this range overlaps with @e range, otherwise @e false
     */
    bool overlaps(const Range& range) const;
    /**
     * Check whether @e cursor == @p start() or @e cursor == @p end().
     * @param cursor cursor to check
     * @return @e true, if the cursor is equal to @p start() or @p end(),
     *         otherwise @e false
     */
    bool boundaryAt(const Cursor& cursor) const;
    /**
     * Check whether @e line == @p start().line() or @e line == @p end().line().
     * @param line line to check
     * @return @e true, if the line is either the same with the start bound
     *         or the end bound, otherwise @e false
     */
    bool boundaryOnLine(int line) const;
    /**
     * Check whether @e column == @p start().column() or @e column == @p end().column().
     * @param column column to check
     * @return @e true, if the column is either the same with the start bound
     *         or the end bound, otherwise @e false
     */
    bool boundaryOnColumn(int column) const;

    /**
     * Check whether this range is wholly contained within one line, ie. if
     * start().line() == end().line().
     */
    inline bool onSingleLine() const { return start().line() == end().line(); }

    /**
     * Returns where \p cursor is positioned, relative to this range.
     * @return \e -1 if before, \e +1 if after, and \e 0 if \p cursor is contained within the range.
     */
    inline int relativePosition(const Cursor& cursor) const
      { return ((cursor < start()) ? -1 : ((cursor > end()) ? 1:0)); }

    /**
     * Returns the number of columns of the end() relative to the start().
     */
    inline int columnWidth() const { return end().column() - start().column(); }

    /**
     * Returns true if this range contains no characters, ie. the start() and end() positions are the same.
     */
    inline bool isEmpty() const { return start() == end(); }

    /**
     * = operator. Assignment.
     * @param rhs new range
     * @return *this
     */
    virtual Range& operator= (const Range& rhs);

    inline friend Range operator+(const Range& r1, const Range& r2) { return Range(r1.start() + r2.start(), r1.end() + r2.end()); }
    inline friend Range& operator+=(Range& r1, const Range& r2) { r1.setRange(r1.start() + r2.start(), r1.end() + r2.end()); return r1; }

    inline friend Range operator-(const Range& r1, const Range& r2) { return Range(r1.start() - r2.start(), r1.end() - r2.end()); }
    inline friend Range& operator-=(Range& r1, const Range& r2) { r1.setRange(r1.start() - r2.start(), r1.end() - r2.end()); return r1; }

    /**
     * == operator
     * @param r1 first range to compare
     * @param r2 second range to compare
     * @return @e true, if @e r1 and @e r2 equal, otherwise @e false
     */
    inline friend bool operator==(const Range& r1, const Range& r2)
      { return r1.start() == r2.start() && r1.end() == r2.end(); }

    /**
     * != operator
     * @param r1 first range to compare
     * @param r2 second range to compare
     * @return @e true, if @e r1 and @e r2 do @e not equal, otherwise @e false
     */
    inline friend bool operator!=(const Range& r1, const Range& r2)
      { return r1.start() != r2.start() || r1.end() != r2.end(); }

    /**
     * Greater than operator.
     * @param r1 first range to compare
     * @param r2 second range to compare
     * @return @e true, if @e r1 starts after where @e r2 ends, otherwise @e false
     */
    inline friend bool operator>(const Range& r1, const Range& r2)
      { return r1.start() > r2.end(); }

    /**
     * Less than operator.
     * @param r1 first range to compare
     * @param r2 second range to compare
     * @return @e true, if @e r1 ends before @e r2 begins, otherwise @e false
     */
    inline friend bool operator<(const Range& r1, const Range& r2)
      { return r1.end() < r2.start(); }

    inline friend kdbgstream& operator<< (kdbgstream& s, const Range& range) {
      if (&range)
        s << "[" << range.start() << " -> " << range.end() << "]";
      else
        s << "(null range)";
      return s;
    }

    inline friend kndbgstream& operator<< (kndbgstream& s, const Range&) { return s; }

    /**
     * \internal
     *
     * Notify this range that one or both of the cursors' position has changed directly.
     *
     * \param cursor the cursor that changed. If 0L, both cursors have changed.
     */
    virtual void cursorChanged(Cursor* cursor);

  protected:
    /**
     * Constructor for advanced cursor types.
     * Creates a range from @e start to @e end.
     * Takes ownership of @e start and @e end.
     */
    Range(Cursor* start, Cursor* end);

    Cursor* m_start;
    Cursor* m_end;
};

class SmartRange;

/**
 * \short A class which provides notifications of state changes to a SmartRange via QObject signals.
 *
 * This class provides notifications of changes to the position or contents of
 * a SmartRange via QObject signals.
 *
 * If you prefer to receive notifications via virtual inheritance, see SmartRangeWatcher.
 *
 * \sa SmartRange, SmartRangeWatcher
 */
class KTEXTEDITOR_EXPORT SmartRangeNotifier : public QObject
{
  Q_OBJECT
  friend class SmartRange;

  public:
    SmartRangeNotifier();

    /**
     * Returns whether this notifier will notify of changes that happen
     * directly to the range, e.g. by calls to SmartCursor::setRange(), or by
     * direct assignment to either of the start() or end() cursors, rather
     * than just when surrounding text changes.
     */
    bool wantsDirectChanges() const;

    /**
     * Set whether this notifier should notify of changes that happen
     * directly to the range, e.g. by calls to SmartCursor::setRange(), or by
     * direct assignment to either of the start() or end() cursors, rather
     * than just when surrounding text changes.
     */
    void setWantsDirectChanges(bool wantsDirectChanges);

  signals:
    /**
     * The range's position changed.
     */
    void positionChanged(KTextEditor::SmartRange* range);

    /**
     * The contents of the range changed.
     */
    void contentsChanged(KTextEditor::SmartRange* range);

    /**
     * The contents of the range changed.  This notification is special in that it is only emitted by
     * the top range of a heirachy, and also gives the furthest descendant child range which still
     * encompasses the whole change in contents.
     *
     * \param range the range which has changed
     * \param mostSpecificChild the child range which both contains the entire change and is 
     *                          the furthest descendant of this range.
     */
    void contentsChanged(KTextEditor::SmartRange* range, KTextEditor::SmartRange* mostSpecificChild);

    /**
     * NOTE: this signal does not appear to be useful on reflection.
     * if you want it, please email rodda@kde.org
     *
     * Either cursor's surrounding characters were both deleted.
     * \param start true if the start boundary was deleted, false if the end boundary was deleted.
     *
    void boundaryDeleted(KTextEditor::SmartRange* range, bool start);*/

    /**
     * The range now contains no characters (ie. the start and end cursors are the same).
     */
    void eliminated(KTextEditor::SmartRange* range);

    /**
     * NOTE: this signal does not appear to be useful on reflection.
     * if you want it, please email rodda@kde.org
     *
     * The first character of this range was deleted.
     *
    void firstCharacterDeleted(KTextEditor::SmartRange* range);

    **
     * NOTE: this signal does not appear to be useful on reflection.
     * if you want it, please email rodda@kde.org
     *
     * The last character of this range was deleted.
     *
    void lastCharacterDeleted(KTextEditor::SmartRange* range);*/

  private:
    bool m_wantDirectChanges;
};

/**
 * \short A class which provides notifications of state changes to a SmartRange via virtual inheritance.
 *
 * This class provides notifications of changes to the position or contents of
 * a SmartRange via virtual inheritance.
 *
 * If you prefer to receive notifications via QObject signals, see SmartRangeNotifier.
 *
 * \sa SmartRange, SmartRangeNotifier
 */
class KTEXTEDITOR_EXPORT SmartRangeWatcher
{
  public:
    SmartRangeWatcher();
    virtual ~SmartRangeWatcher();

    /**
     * Returns whether this watcher will be notified of changes that happen
     * directly to the range, e.g. by calls to SmartCursor::setRange(), or by
     * direct assignment to either of the start() or end() cursors, rather
     * than just when surrounding text changes.
     */
    bool wantsDirectChanges() const;

    /**
     * Set whether this watcher should be notified of changes that happen
     * directly to the range, e.g. by calls to SmartCursor::setRange(), or by
     * direct assignment to either of the start() or end() cursors, rather
     * than just when surrounding text changes.
     */
    void setWantsDirectChanges(bool wantsDirectChanges);

    /**
     * The range's position changed.
     */
    virtual void positionChanged(SmartRange* range);

    /**
     * The contents of the range changed.
     */
    virtual void contentsChanged(SmartRange* range);

    /**
     * The contents of the range changed.  This notification is special in that it is only emitted by
     * the top range of a heirachy, and also gives the furthest descendant child range which still
     * encompasses the whole change in contents.
     *
     * \param range the range which has changed
     * \param mostSpecificChild the child range which both contains the entire change and is 
     *                          the furthest descendant of this range.
     */
    virtual void contentsChanged(KTextEditor::SmartRange* range, KTextEditor::SmartRange* mostSpecificChild);

    /**
     * NOTE: this signal does not appear to be useful on reflection.
     * if you want it, please email rodda@kde.org
     *
     * Either cursor's surrounding characters were both deleted.
     * \param start true if the start boundary was deleted, false if the end boundary was deleted.
     *
    virtual void boundaryDeleted(SmartRange* range, bool start);*/

    /**
     * The range now contains no characters (ie. the start and end cursors are the same).
     */
    virtual void eliminated(SmartRange* range);

    /**
     * NOTE: this signal does not appear to be useful on reflection.
     * if you want it, please email rodda@kde.org
     *
     * The first character of this range was deleted.
     *
    virtual void firstCharacterDeleted(SmartRange* range);*/

    /**
     * NOTE: this signal does not appear to be useful on reflection.
     * if you want it, please email rodda@kde.org
     *
     * The last character of this range was deleted.
     *
    virtual void lastCharacterDeleted(SmartRange* range);*/

  private:
    bool m_wantDirectChanges;
};

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
