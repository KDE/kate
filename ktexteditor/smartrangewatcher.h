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

#ifndef KDELIBS_KTEXTEDITOR_SMARTRANGEWATCHER_H
#define KDELIBS_KTEXTEDITOR_SMARTRANGEWATCHER_H

#include <ktexteditor/ktexteditor_export.h>
#include <QtCore/QObject>

#include <ktexteditor/attribute.h>

// TODO: Should SmartRangeWatcher become a base class for SmartRangeNotifier?

namespace KTextEditor
{
class SmartRange;
class View;

/**
 * \short A class which provides notifications of state changes to a SmartRange via virtual inheritance.
 *
 * \ingroup kte_group_smart_classes
 *
 * This class provides notifications of changes to the position or contents of
 * a SmartRange via virtual inheritance.
 *
 * If you prefer to receive notifications via QObject signals, see SmartRangeNotifier.
 *
 * Before destruction, you must unregister the watcher from any ranges it is watching.
 *
 * \sa SmartRange, SmartRangeNotifier
 *
 * \author Hamish Rodda \<rodda@kde.org\>
 */
class KTEXTEDITOR_EXPORT SmartRangeWatcher
{
  public:
    /**
     * Default constructor
     */
    SmartRangeWatcher();

    /**
     * Virtual destructor
     */
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
     *
     * \param wantsDirectChanges whether this watcher should receive notifications for direct changes.
     */
    void setWantsDirectChanges(bool wantsDirectChanges);

    /**
     * The range's position changed.
     *
     * \param range pointer to the range which generated the notification.
     */
    virtual void rangePositionChanged(SmartRange* range);

    /**
     * The contents of the range changed.
     *
     * \param range pointer to the range which generated the notification.
     */
    virtual void rangeContentsChanged(SmartRange* range);

    /**
     * The contents of the range changed.  This notification is special in that it is only emitted by
     * the top range of a heirachy, and also gives the furthest descendant child range which still
     * encompasses the whole change (see \p contents).
     *
     * \param range the range which has changed
     * \param mostSpecificChild the child range which both contains the entire change and is 
     *                          the furthest descendant of this range.
     */
    virtual void rangeContentsChanged(SmartRange* range, SmartRange* mostSpecificChild);

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
    virtual void mouseEnteredRange(SmartRange* range, View* view);

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
    virtual void mouseExitedRange(SmartRange* range, View* view);

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
    virtual void caretEnteredRange(SmartRange* range, View* view);

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
    virtual void caretExitedRange(SmartRange* range, View* view);

    /**
     * The range now contains no characters (ie. the start and end cursors are the same).
     *
     * \param range pointer to the range which generated the notification.
     */
    virtual void rangeEliminated(SmartRange* range);

    /**
     * The SmartRange instance specified by \p range is being deleted.
     *
     * \param range pointer to the range which is about to be deleted.  It is
     *              still safe to access information at this point.
     */
    virtual void rangeDeleted(SmartRange* range);

    /**
     * The range's parent was changed.
     *
     * \param range pointer to the range which generated the notification.
     * \param newParent pointer to the range which was is now the parent range.
     * \param oldParent pointer to the range which used to be the parent range.
     */
    virtual void parentRangeChanged(SmartRange* range, SmartRange* newParent, SmartRange* oldParent);

    /**
     * The range \a child was inserted as a child range into the current \a range.
     *
     * \param range pointer to the range which generated the notification.
     * \param child pointer to the range which was inserted as a child range.
     */
    virtual void childRangeInserted(SmartRange* range, SmartRange* child);

    /**
     * The child range \a child was removed from the current \a range.
     *
     * \param range pointer to the range which generated the notification.
     * \param child pointer to the child range which was removed.
     */
    virtual void childRangeRemoved(SmartRange* range, SmartRange* child);

    /**
     * The highlighting attribute of \a range was changed from \a previousAttribute to
     * \a currentAttribute.
     *
     * \param range pointer to the range which generated the notification.
     * \param currentAttribute the attribute newly assigned to this range
     * \param previousAttribute the attribute previously assigned to this range
     */
    virtual void rangeAttributeChanged(SmartRange* range, Attribute::Ptr currentAttribute, Attribute::Ptr previousAttribute);

  private:
    bool m_wantDirectChanges;
};

}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
