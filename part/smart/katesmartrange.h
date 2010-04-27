/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2003,2004,2005 Hamish Rodda <rodda@kde.org>
 *  Copyright (C) 2010 Christoph Cullmann <cullmann@kde.org>
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

#ifndef KATESMARTRANGE_H
#define KATESMARTRANGE_H

#include "katesmartcursor.h"
#include <ktexteditor/smartrange.h>
#include <ktexteditor/smartrangenotifier.h>

class KateSmartRange;
class KateEditInfo;

/**
 * Internal Implementation of KTextEditor::SmartRangeNotifier.
 */
class KateSmartRangeNotifier : public KTextEditor::SmartRangeNotifier
{
  Q_OBJECT
  friend class KateSmartRange;

  public:
    explicit KateSmartRangeNotifier(KateSmartRange* owner);

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
    KateSmartRange(const KTextEditor::Range& range, KateDocument* doc, KTextEditor::SmartRange* parent = 0L, KTextEditor::SmartRange::InsertBehaviors insertBehavior = DoNotExpand);
    /// overload
    explicit KateSmartRange(KateDocument* doc, KTextEditor::SmartRange* parent = 0L);

    KateSmartRange(KateSmartCursor* start, KateSmartCursor* end, KTextEditor::SmartRange* parent = 0L, KTextEditor::SmartRange::InsertBehaviors insertBehavior = DoNotExpand);
    virtual ~KateSmartRange();

    /// Accessor for the document this range belongs to (set in constructor)
    KateDocument* kateDocument() const;
    /// Accessor for the start of the range (set in constructor)
    inline KateSmartCursor& kStart() { return *static_cast<KateSmartCursor*>(m_start); }
    inline const KateSmartCursor& kStart() const { return *static_cast<const KateSmartCursor*>(m_start); }
    /// Accessor for the end of the range (set in constructor)
    inline KateSmartCursor& kEnd() { return *static_cast<KateSmartCursor*>(m_end); }
    inline const KateSmartCursor& kEnd() const { return *static_cast<const KateSmartCursor*>(m_end); }

    bool isInternal() const { return m_isInternal; }
    void setInternal();

    void unbindAndDelete();

    enum AttachActions {
      NoActions   = 0x0,
      TagLines    = 0x1,
      Redraw      = 0x2
    };

    virtual void setParentRange(SmartRange* r);

    using SmartRange::rebuildChildStructure;

    bool feedbackEnabled() const { return notifiers().count() || watchers().count(); }

    /// One or both of the cursors has been changed.
    void translated(const KateEditInfo& edit);
    void feedbackRangeContentsChanged(KateSmartRange* mostSpecific);
    /// The range has been shifted only
    void shifted();
    /// Mouse / caret in or out
    void feedbackMouseEnteredRange(KTextEditor::View* view);
    void feedbackMouseExitedRange(KTextEditor::View* view);
    void feedbackCaretEnteredRange(KTextEditor::View* view);
    void feedbackCaretExitedRange(KTextEditor::View* view);

    inline KateSmartRange& operator=(const KTextEditor::Range& r) { setRange(r); return *this; }

  protected:
    friend class KateSmartCursor; //Has to call rangeChanged directly
    virtual KTextEditor::SmartRangeNotifier* createNotifier();
    virtual void checkFeedback();

  private:
    void init();

    bool m_isInternal;
};

#endif
