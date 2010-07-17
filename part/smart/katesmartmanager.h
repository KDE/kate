/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2005 Hamish Rodda <rodda@kde.org>
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

#ifndef KATESMARTMANAGER_H
#define KATESMARTMANAGER_H

#include <QtCore/QObject>
#include <QtCore/QSet>

#include <ktexteditor/smartrange.h>

class KateEditInfo;
class KateDocument;
class KateSmartCursor;
class KateSmartRange;
class KateSmartGroup;

/**
 * Manages SmartCursors and SmartRanges.
 *
 * \todo potential performance optimization: use separate sets for internal and non-internal cursors + ranges
 * \todo potential performance optimization: bypass unhooking routines when clearing
 */
class KateSmartManager : public QObject
{
  Q_OBJECT

  public:
    explicit KateSmartManager(KateDocument* parent);
    virtual ~KateSmartManager();

    KateDocument* doc() const;

    /**
     * The process of clearing works as follows:
     * - SmartCursors and SmartRanges emit the relevant signals as usual
     * - The smart manager takes care of deleting the ranges and the cursors
     *   which are not bound to ranges (those cursors get deleted by the ranges
     *   themselves)
     * - isClearing() is set to true while cursors only are being deleted (not cursors as part of a range)
     * - The smart manager takes care of cleaning its internal lists of cursors it deletes
     *   (deleted SmartCursors should not tell the smart manager that they have
     *   been deleted, ie when isClearing() is true)
     */
    inline bool isClearing() const { return m_clearing; }
    void clear(bool includingInternal);

    int currentRevision() const;
    void releaseRevision(int revision) const;
    void useRevision(int revision = -1);
    KTextEditor::Cursor translateFromRevision(const KTextEditor::Cursor& cursor, KTextEditor::SmartCursor::InsertBehavior insertBehavior = KTextEditor::SmartCursor::StayOnInsert);
    KTextEditor::Range translateFromRevision(const KTextEditor::Range& range, KTextEditor::SmartRange::InsertBehaviors insertBehavior = KTextEditor::SmartRange::ExpandLeft | KTextEditor::SmartRange::ExpandRight);

    KateSmartCursor* newSmartCursor(const KTextEditor::Cursor& position = KTextEditor::Cursor(), KTextEditor::SmartCursor::InsertBehavior insertBehavior = KTextEditor::SmartCursor::MoveOnInsert, bool internal = true);
    void deleteCursors(bool includingInternal);

    KateSmartRange* newSmartRange(const KTextEditor::Range& range = KTextEditor::Range(), KTextEditor::SmartRange* parent = 0L, KTextEditor::SmartRange::InsertBehaviors insertBehavior = KTextEditor::SmartRange::DoNotExpand, bool internal = true);
    KateSmartRange* newSmartRange(KateSmartCursor* start, KateSmartCursor* end, KTextEditor::SmartRange* parent = 0L, KTextEditor::SmartRange::InsertBehaviors insertBehavior = KTextEditor::SmartRange::DoNotExpand, bool internal = true);
    void unbindSmartRange(KTextEditor::SmartRange* range);
    void deleteRanges(bool includingInternal);

    void rangeGotParent(KateSmartRange* range);
    void rangeLostParent(KateSmartRange* range);
    /// This is called regardless of whether a range was deleted via clear(), or deleteRanges(), or otherwise.
    void rangeDeleted(KateSmartRange* range);

    KateSmartGroup* groupForLine(int line) const;

  Q_SIGNALS:
    void signalRangeDeleted(KateSmartRange* range);

  private Q_SLOTS:
    void slotTextChanged(KateEditInfo* edit);
    void verifyCorrect() const;

  private:
    KateSmartRange* feedbackRange(const KateEditInfo& edit, KateSmartRange* range);
    int usingRevision();

    void debugOutput() const;

    KateSmartGroup* m_firstGroup;
    QSet<KateSmartRange*> m_topRanges;
    KateSmartGroup* m_invalidGroup;
    bool m_clearing;
    
    struct KateTranslationDebugger;
    KateTranslationDebugger* m_currentKateTranslationDebugger;
};

#endif
