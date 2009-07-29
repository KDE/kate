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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KDELIBS_KTEXTEDITOR_SMARTRANGENOTIFIER_H
#define KDELIBS_KTEXTEDITOR_SMARTRANGENOTIFIER_H

#include <ktexteditor/ktexteditor_export.h>
#include <QtCore/QObject>

#include <ktexteditor/attribute.h>

// TODO: Should SmartRangeWatcher become a base class for SmartRangeNotifier?

namespace KTextEditor
{
class SmartRange;
class View;

/**
 * \short A class which provides notifications of state changes to a SmartRange via QObject signals.
 *
 * \ingroup kte_group_smart_classes
 *
 * This class provides notifications of changes to the position or contents of
 * a SmartRange via QObject signals.
 *
 * If you prefer to receive notifications via virtual inheritance, see SmartRangeWatcher.
 *
 * \sa SmartRange, SmartRangeWatcher
 *
 * \author Hamish Rodda \<rodda@kde.org\>
 */
class KTEXTEDITOR_EXPORT SmartRangeNotifier : public QObject
{
  Q_OBJECT
  friend class SmartRange;

  public:
    /**
     * Default constructor.
     */
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
     *
     * \param wantsDirectChanges whether this watcher should provide notifications for direct changes.
     */
    void setWantsDirectChanges(bool wantsDirectChanges);

  Q_SIGNALS:
    /**
     * The range's position changed.
     *
     * \param range pointer to the range which generated the notification.
     */
    void rangePositionChanged(KTextEditor::SmartRange* range);

    /**
     * The contents of the range changed.
     *
     * \param range pointer to the range which generated the notification.
     */
    void rangeContentsChanged(KTextEditor::SmartRange* range);

    /**
     * The contents of the range changed.
     *
     * \warning This notification is special in that it is only emitted by
     * the top range of a heirachy, and also gives the furthest descendant child range which still
     * encompasses the whole change (see \p contents).
     *
     * \param range pointer to the range which generated the notification.
     * \param mostSpecificChild the child range which both contains the entire change and is
     *                          the furthest descendant of this range.
     */
    void rangeContentsChanged(KTextEditor::SmartRange* range, KTextEditor::SmartRange* mostSpecificChild);

    /**
     * The mouse cursor on \a view entered \p range.
     *
     * \todo For now, to receive this notification, the range heirachy must be registered with
     * the SmartInterface as for arbitrary highlighting with dynamic highlighting.
     * Need to add another (and probably simplify existing) method.
     *
     * \param range pointer to the range which generated the notification.
     * \param view view over which the mouse moved to generate the notification
     */
    void mouseEnteredRange(KTextEditor::SmartRange* range, KTextEditor::View* view);

    /**
     * The mouse cursor on \a view exited \p range.
     *
     * \todo For now, to receive this notification, the range heirachy must be registered with
     * the SmartInterface as for arbitrary highlighting with dynamic highlighting.
     * Need to add another (and probably simplify existing) method.
     *
     * \param range pointer to the range which generated the notification.
     * \param view view over which the mouse moved to generate the notification
     */
    void mouseExitedRange(KTextEditor::SmartRange* range, KTextEditor::View* view);

    /**
     * The caret on \a view entered \p range.
     *
     * \todo For now, to receive this notification, the range heirachy must be registered with
     * the SmartInterface as for arbitrary highlighting with dynamic highlighting.
     * Need to add another (and probably simplify existing) method.
     *
     * \param range pointer to the range which generated the notification.
     * \param view view over which the mouse moved to generate the notification
     */
    void caretEnteredRange(KTextEditor::SmartRange* range, KTextEditor::View* view);

    /**
     * The caret on \a view exited \p range.
     *
     * \todo For now, to receive this notification, the range heirachy must be registered with
     * the SmartInterface as for arbitrary highlighting with dynamic highlighting.
     * Need to add another (and probably simplify existing) method.
     *
     * \param range pointer to the range which generated the notification.
     * \param view view over which the mouse moved to generate the notification
     */
    void caretExitedRange(KTextEditor::SmartRange* range, KTextEditor::View* view);

    /**
     * The range now contains no characters (ie. the start and end cursors are the same).
     *
     * \param range pointer to the range which generated the notification.
     */
    void rangeEliminated(KTextEditor::SmartRange* range);

    /**
     * The SmartRange instance specified by \p range is being deleted.
     *
     * \param range pointer to the range which is about to be deleted.  It is
     *              still safe to access information at this point.
     */
    void rangeDeleted(KTextEditor::SmartRange* range);

    /**
     * The range's parent was changed.
     *
     * \param range pointer to the range which generated the notification.
     * \param newParent pointer to the range which was is now the parent range.
     * \param oldParent pointer to the range which used to be the parent range.
     */
    void parentRangeChanged(KTextEditor::SmartRange* range, KTextEditor::SmartRange* newParent, KTextEditor::SmartRange* oldParent);

    /**
     * The range \a child was inserted as a child range into the current \a range.
     *
     * \param range pointer to the range which generated the notification.
     * \param child pointer to the range which was inserted as a child range.
     */
    void childRangeInserted(KTextEditor::SmartRange* range, KTextEditor::SmartRange* child);

    /**
     * The child range \a child was removed from the current \a range.
     *
     * \param range pointer to the range which generated the notification.
     * \param child pointer to the child range which was removed.
     */
    void childRangeRemoved(KTextEditor::SmartRange* range, KTextEditor::SmartRange* child);

    /**
     * The highlighting attribute of \a range was changed from \a previousAttribute to
     * \a currentAttribute.
     *
     * \param range pointer to the range which generated the notification.
     * \param currentAttribute the attribute newly assigned to this range
     * \param previousAttribute the attribute previously assigned to this range
     */
    void rangeAttributeChanged(KTextEditor::SmartRange* range, KTextEditor::Attribute::Ptr currentAttribute, KTextEditor::Attribute::Ptr previousAttribute);

  private:
    bool m_wantDirectChanges;
};

}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
