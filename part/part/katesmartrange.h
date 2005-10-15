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

#include "katesmartcursor.h"
#include <ktexteditor/smartrange.h>
#include <ktexteditor/rangefeedback.h>
#include "kateedit.h"

class KateSmartRange;

/**
 * Internal Implementation of KTextEditor::SmartRangeNotifier.
 */
class KateSmartRangeNotifier : public KTextEditor::SmartRangeNotifier
{
  Q_OBJECT
  friend class KateSmartRange;

  public:
    KateSmartRangeNotifier(KateSmartRange* owner);

    /**
     * Implementation detail. Returns whether the positionChanged() signal
     * needs to be emitted, as it is a relatively expensive signal to emit.
     */
    bool needsPositionChanges() const;

  protected:
    virtual void connectNotify(const char* signal);
    virtual void disconnectNotify(const char* signal);

  private:
    KateSmartRange* m_owner;
};

/**
 * Internal implementation of KTextEditor::SmartRange.
 * Represents a range of text, from the start() to the end().
 *
 * Also tracks its position and emits useful signals.
 */
class KateSmartRange : public KTextEditor::SmartRange
{
  friend class KateSmartRangeNotifier;

  public:
    /**
     * Constructors.  Take posession of @p start and @p end.
     */
    KateSmartRange(const KTextEditor::Range& range, KateDocument* doc, KTextEditor::SmartRange* parent = 0L, KTextEditor::SmartRange::InsertBehaviours insertBehaviour = DoNotExpand);
    /// overload
    KateSmartRange(KateDocument* doc, KTextEditor::SmartRange* parent = 0L);

    KateSmartRange(KateSmartCursor* start, KateSmartCursor* end, KTextEditor::SmartRange* parent = 0L, KTextEditor::SmartRange::InsertBehaviours insertBehaviour = DoNotExpand);
    virtual ~KateSmartRange();

    KateDocument* kateDocument() const;
    KateSmartCursor& kStart() { return *static_cast<KateSmartCursor*>(m_start); }
    KateSmartCursor& kEnd() { return *static_cast<KateSmartCursor*>(m_end); }

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
     * Start and end must be valid.
     */
    virtual bool isValid() const;

    //virtual void tagRange();

    virtual bool hasNotifier() const;
    virtual KTextEditor::SmartRangeNotifier* notifier();
    virtual void deleteNotifier();
    virtual KTextEditor::SmartRangeWatcher* watcher() const;
    virtual void setWatcher(KTextEditor::SmartRangeWatcher* watcher);

    virtual void setParentRange(SmartRange* r);

    /**
     * Implementation detail. Defines the level of feedback required for any connected
     * watcher / notifier.
     *
    enum FeedbackLevel {
      /// Don't provide any feedback.
      NoFeedback,
      /// Only provide feedback when the range in question is the most specific, wholly encompassing range to have been changed.
      MostSpecificContentChanged,
      /// Provide feedback whenever the contents of a range change.
      ContentChanged,
      /// Provide feedback whenever the position of a range changes.
      PositionChanged
    };
    Q_DECLARE_FLAGS(FeedbackLevels, FeedbackLevel);*/

    bool feedbackEnabled() const { return m_notifier || m_watcher; }
    // request is internal!! Only KateSmartGroup gets to set it to false.
    /*void setFeedbackLevel(int feedbackLevel, bool request = true);*/

    /// One or both of the cursors has been changed.
    void translated(const KateEditInfo& edit);
    void feedbackMostSpecific(KateSmartRange* mostSpecific);
    /// The range has been shifted only
    void shifted();

    inline KateSmartRange& operator=(const KTextEditor::Range& r) { setRange(r); return *this; }

  protected:
    virtual void checkFeedback();

  private:
    void slotEvaluateChanged();
    void slotEvaluateUnChanged();
    void slotMousePositionChanged(const KTextEditor::Cursor& newPosition);
    void slotCaretPositionChanged(const KTextEditor::Cursor& newPosition);

  private:
    void init();
    void evaluateEliminated();
    void evaluatePositionChanged();

    KateSmartRangeNotifier* m_notifier;
    KTextEditor::SmartRangeWatcher* m_watcher;
    KateView* m_attachedView;
    int m_attachActions;
    //FeedbackLevels m_feedbackLevel;
    bool  m_mouseOver             :1,
          m_caretOver             :1;
};

//Q_DECLARE_OPERATORS_FOR_FLAGS(KateSmartRange::FeedbackLevels);

#endif
