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

#ifndef KDELIBS_KTEXTEDITOR_RANGEFEEDBACK_H
#define KDELIBS_KTEXTEDITOR_RANGEFEEDBACK_H

#include <kdelibs_export.h>

#include <QObject>

namespace KTextEditor
{
class SmartRange;
class View;

/**
 * \short A class which provides notifications of state changes to a SmartRange via QObject signals.
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

  signals:
    /**
     * The range's position changed.
     *
     * \param range pointer to the range which generated the notification.
     */
    void positionChanged(KTextEditor::SmartRange* range);

    /**
     * The contents of the range changed.
     *
     * \param range pointer to the range which generated the notification.
     */
    void contentsChanged(KTextEditor::SmartRange* range);

    /**
     * The contents of the range changed.  This notification is special in that it is only emitted by
     * the top range of a heirachy, and also gives the furthest descendant child range which still
     * encompasses the whole change (see \p contents).
     *
     * \param range pointer to the range which generated the notification.
     * \param mostSpecificChild the child range which both contains the entire change and is
     *                          the furthest descendant of this range.
     */
    void contentsChanged(KTextEditor::SmartRange* range, KTextEditor::SmartRange* mostSpecificChild);

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
    void mouseEntered(KTextEditor::SmartRange* range, KTextEditor::View* view);

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
    void mouseExited(KTextEditor::SmartRange* range, KTextEditor::View* view);

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
    void caretEntered(KTextEditor::SmartRange* range, KTextEditor::View* view);

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
    void caretExited(KTextEditor::SmartRange* range, KTextEditor::View* view);

    /**
     * The range now contains no characters (ie. the start and end cursors are the same).
     *
     * \param range pointer to the range which generated the notification.
     */
    void eliminated(KTextEditor::SmartRange* range);

    /**
     * The SmartRange instance specified by \p range is being deleted.
     *
     * \param range pointer to the range which is about to be deleted.  It is
     *              still safe to access information at this point.
     */
    void deleted(KTextEditor::SmartRange* range);

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
    virtual void positionChanged(SmartRange* range);

    /**
     * The contents of the range changed.
     *
     * \param range pointer to the range which generated the notification.
     */
    virtual void contentsChanged(SmartRange* range);

    /**
     * The contents of the range changed.  This notification is special in that it is only emitted by
     * the top range of a heirachy, and also gives the furthest descendant child range which still
     * encompasses the whole change (see \p contents).
     *
     * \param range the range which has changed
     * \param mostSpecificChild the child range which both contains the entire change and is 
     *                          the furthest descendant of this range.
     */
    virtual void contentsChanged(SmartRange* range, SmartRange* mostSpecificChild);

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
    virtual void mouseEntered(KTextEditor::SmartRange* range, KTextEditor::View* view);

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
    virtual void mouseExited(KTextEditor::SmartRange* range, KTextEditor::View* view);

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
    virtual void caretEntered(KTextEditor::SmartRange* range, KTextEditor::View* view);

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
    virtual void caretExited(KTextEditor::SmartRange* range, KTextEditor::View* view);

    /**
     * The range now contains no characters (ie. the start and end cursors are the same).
     *
     * \param range pointer to the range which generated the notification.
     */
    virtual void eliminated(SmartRange* range);

    /**
     * The SmartRange instance specified by \p range is being deleted.
     *
     * \param range pointer to the range which is about to be deleted.  It is
     *              still safe to access information at this point.
     */
    virtual void deleted(SmartRange* range);

  private:
    bool m_wantDirectChanges;
};

}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
