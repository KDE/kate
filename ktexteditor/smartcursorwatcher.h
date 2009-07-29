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

#ifndef KDELIBS_KTEXTEDITOR_SMARTCURSORWATCHER_H
#define KDELIBS_KTEXTEDITOR_SMARTCURSORWATCHER_H

#include <ktexteditor/ktexteditor_export.h>
#include <QtCore/QObject>
#include <kdebug.h>

namespace KTextEditor
{
class SmartCursor;

/**
 * \short A class which provides notifications of state changes to a SmartCursor via virtual inheritance.
 *
 * \ingroup kte_group_smart_classes
 *
 * This class provides notifications of changes to a SmartCursor such as the
 * position in the document, and deletion or insertion of text immediately
 * before or after the cursor.
 *
 * If you prefer to receive notifications via QObject signals, see SmartCursorNotifier.
 *
 * \sa SmartCursor, SmartCursorNotifier
 *
 * \author Hamish Rodda \<rodda@kde.org\>
 */
class KTEXTEDITOR_EXPORT SmartCursorWatcher
{
  public:
    /**
     * Default constructor.
     */
    SmartCursorWatcher();

    /**
     * Virtual destructor.
     */
    virtual ~SmartCursorWatcher();

    /**
     * Returns whether this watcher wants to be notified of changes that happen
     * directly to the cursor, e.g. by calls to SmartCursor::setPosition(), rather
     * than just when surrounding text changes.
     */
    bool wantsDirectChanges() const;

    /**
     * Set whether this watcher should be notified of changes that happen
     * directly to the cursor, e.g. by calls to SmartCursor::setPosition(), rather
     * than just when surrounding text changes.
     *
     * \param wantsDirectChanges whether this watcher should receive notifications for direct changes.
     */
    void setWantsDirectChanges(bool wantsDirectChanges);

    /**
     * The cursor's position was changed.
     *
     * \param cursor pointer to the cursor which generated the notification.
     */
    virtual void positionChanged(SmartCursor* cursor);

    /**
     * The cursor's surrounding characters were both deleted simultaneously.
     * The cursor is automatically placed at the start of the deleted region.
     *
     * \param cursor pointer to the cursor which generated the notification.
     */
    virtual void positionDeleted(SmartCursor* cursor);

    /**
     * The character immediately surrounding the cursor was deleted.
     * If both characters are simultaneously deleted, positionDeleted() is called instead.
     *
     * \param cursor pointer to the cursor which generated the notification.
     * \param deletedBefore \c true if the character immediately before was deleted,
     *                      \c false if the character immediately after was deleted.
     */
    virtual void characterDeleted(SmartCursor* cursor, bool deletedBefore);

    /**
     * A character was inserted immediately before or after the cursor, as given
     * by \p insertedBefore.
     *
     * \param cursor pointer to the cursor which generated the notification.
     * \param insertedBefore \e true if a character was inserted before \p cursor,
     *                       \e false if a character was inserted after
     */
    virtual void characterInserted(SmartCursor* cursor, bool insertedBefore);

    /**
     * The SmartCursor instance specified by \p cursor is being deleted.
     *
     * \param cursor pointer to the cursor which is about to be deleted.  It is
     *               still safe to access information at this point.
     */
    virtual void deleted(SmartCursor* cursor);

  private:
    bool m_wantDirectChanges;
};

}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
