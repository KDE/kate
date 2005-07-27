/* This file is part of the KDE libraries
   Copyright (C) 2003,2004,2005 Hamish Rodda <rodda@kde.org>

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

#ifndef KATESUPERRANGE_H
#define KATESUPERRANGE_H

#include "katesupercursor.h"
#include <ktexteditor/range.h>

class KateRangeList;
class KateRangeType;

class KAction;

/**
 * Represents a range of text, from the start() to the end().
 *
 * Also tracks its position and emits useful signals.
 */
class KateSuperRange : public QObject, public KTextEditor::Range
{
  Q_OBJECT

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

    /**
     * Constructors.  Take posession of @p start and @p end.
     */
    KateSuperRange(KateSuperCursor* start, KateSuperCursor* end, KateSuperRange* parent = 0L);
    KateSuperRange(KateDocument* doc, const KateSuperRange& range, KateSuperRange* parent = 0L);
    KateSuperRange(KateDocument* doc, const KTextEditor::Cursor& start, const KTextEditor::Cursor& end, KateSuperRange* parent = 0L);
    KateSuperRange(KateDocument* doc, uint startLine = 0, uint startCol = 0, uint endLine = 0, uint endCol = 0, KateSuperRange* parent = 0L);

    virtual ~KateSuperRange();

    KateDocument* doc() const;
    virtual KateRangeList* owningList() const;

    KateSuperCursor& superStart();
    KateSuperCursor& superEnd();
    const KateSuperCursor& superStart() const;
    const KateSuperCursor& superEnd() const;

    KateSuperRange* parentRange() const;
    void setParentRange(KateSuperRange* r);
    int depth() const;

    KateSuperRange* firstChildRange() const;
    KateSuperRange* rangeBefore(KateSuperRange* range) const;
    KateSuperRange* rangeAfter(KateSuperRange* range) const;
    const QList<KateSuperRange*>& childRanges() const;
    bool insertChildRange(KateSuperRange* newChild, KateSuperRange* before);
    void clearAllChildRanges();
    void deleteAllChildRanges();

    /**
     * @returns the active KateAttribute for this range.
     */
    KateAttribute* attribute() const;

    /**
     * Sets the currently active attribute for this range.
     */
    void setAttribute(KateAttribute* attribute, bool ownsAttribute = true);

    void allowZeroLength(bool allow = true);

    enum AttachActions {
      NoActions   = 0x0,
      TagLines    = 0x1,
      Redraw      = 0x2
    };

    /**
     * Attach a range to a certain view.  Currently this can only tag and redraw
     * the view when it changes.  This is the default behaviour when attaching.
     *
     * The view can also be 0L, in which case the actions apply to all views.
     * The default for attachment is FIXME
     */
    void attachToView(KateView* view, int actions = TagLines | Redraw);

    /**
     * Returns true if the range totally encompasses @p line
     */
    bool includesWholeLine(uint lineNum) const;

    /**
     * Returns how this range reacts to characters inserted immediately outside the range.
     */
    int behaviour() const;

    /**
     * Determine how the range should react to characters inserted immediately outside the range.
     *
     * TODO does this need a custom function to enable determining of the behavior based on the
     * text that is inserted / deleted?
     *
     * @sa InsertBehaviour
     */
    void setBehaviour(int behaviour);

    /**
     * Start and end must be valid and start <= end.
     */
    virtual bool isValid() const;
    void setValid(bool valid);

    /**
     * Finds the most specific range in a heirachy for the given input range
     * (ie. the smallest range which wholly contains the input range)
     */
    KateSuperRange* findMostSpecificRange(const KTextEditor::Range& input);

    /**
     * Finds the first range which contains position \p pos.
     */
    KateSuperRange* firstRangeIncluding(const KTextEditor::Cursor& pos);

    /**
     * Finds the deepest range which contains position \p pos.
     */
    KateSuperRange* deepestRangeIncluding(const KTextEditor::Cursor& pos);

    /**
     * Classify this range as belonging to a particular group.
     *
     * This range automatically assumes styles, actions, etc from the group
     * definition.
     */
    KateRangeType* rangeType() const;

    /**
     * Attach an action to this range.  The action is enabled when the range is
     * entered by the cursor and disabled on exit.  The action is also added to
     * the context menu when the cursor is over this range.
     */
    void attachAction(KAction* action);
    void detachAction(KAction* action);

  signals:
    /**
     * More interesting signals that aren't worth implementing here:
     *  firstCharDeleted: start()::charDeleted()
     *  lastCharDeleted: end()::previousCharDeleted()
     */

    /**
     * The range's position changed.
     */
    void positionChanged();

    /**
     * The range's position was unchanged.
     */
    void positionUnChanged();

    /**
     * The contents of the range changed.
     */
    void contentsChanged();

    /**
     * Either cursor's surrounding characters were both deleted.
     */
    void boundaryDeleted();

    /**
     * The range now contains no characters (ie. the start and end cursors are the same).
     *
     * To eliminate this range under different conditions, connect the other signal directly
     * to this signal.
     */
    void eliminated();

    /**
     * Indicates the region needs re-drawing.
     */
    void tagRange(KateSuperRange* range);

  public slots:
    virtual void slotTagRange();

  protected:
    KateSuperRange(KateSuperCursor* start, KateSuperCursor* end, QObject* parent = 0L);

  private slots:
    void slotEvaluateChanged();
    void slotEvaluateUnChanged();
    void slotMousePositionChanged(const KTextEditor::Cursor& newPosition);
    void slotCaretPositionChanged(const KTextEditor::Cursor& newPosition);

  private:
    void init();
    void evaluateEliminated();
    void evaluatePositionChanged();

    KateAttribute* m_attribute;
    KateSuperRange* m_parentRange;
    QList<KateSuperRange*> m_childRanges;

    KateView* m_attachedView;
    int m_attachActions;
    QList<KAction*> m_associatedActions;
    bool  m_valid           :1,
          m_ownsAttribute   :1,
          m_evaluate        :1,
          m_startChanged    :1,
          m_endChanged      :1,
          m_deleteCursors   :1,
          m_allowZeroLength :1,
          m_mouseOver       :1,
          m_caretOver       :1;
};

class KateTopRange : public KateSuperRange
{
  Q_OBJECT

  public:
    KateTopRange(KateDocument* doc, KateRangeList* ownerList);

    virtual KateRangeList* owningList() const;

  private:
    KateRangeList* m_owningList;
};

#endif
