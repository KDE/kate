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

#ifndef KDELIBS_KTEXTEDITOR_CURSORFEEDBACK_H
#define KDELIBS_KTEXTEDITOR_CURSORFEEDBACK_H

#include <kdelibs_export.h>
#include <QObject>
#include <kdebug.h>

namespace KTextEditor
{
class SmartCursor;

/**
 * \short A class which provides notifications of state changes to a SmartCursor via virtual inheritance.
 *
 * This class provides notifications of changes to a SmartCursor such as the
 * position in the document, and deletion or insertion of text immediately
 * before or after the cursor.
 *
 * If you prefer to receive notifications via QObject signals, see SmartCursorNotifier.
 *
 * \sa SmartCursor, SmartCursorNotifier
 */
class KTEXTEDITOR_EXPORT SmartCursorWatcher
{
  public:
    SmartCursorWatcher();
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
     */
    void setWantsDirectChanges(bool wantsDirectChanges);

    /**
     * The cursor's position was changed.
     */
    virtual void positionChanged(SmartCursor* cursor);

    /**
     * The cursor's surrounding characters were both deleted simultaneously.
     * The cursor is automatically placed at the start of the deleted region.
     */
    virtual void positionDeleted(SmartCursor* cursor);

    /**
     * The character immediately surrounding the cursor was deleted.
     * If both characters are simultaneously deleted, positionDeleted() is called instead.
     *
     * \param deletedBefore @c true if the character immediately before was deleted,
     *               @c false if the character immediately after was deleted.
     */
    virtual void characterDeleted(SmartCursor* cursor, bool deletedBefore);

    /**
     * A character was inserted immediately before or after the cursor.
     *
     * Whether the char was inserted before or after this cursor depends on
     * moveOnInsert():
     * @li true -> the char was inserted before
     * @li false -> the char was inserted after
     */
    virtual void characterInserted(SmartCursor* cursor, bool insertedBefore);

  private:
    bool m_wantDirectChanges;
};

/**
 * \short A class which provides notifications of state changes to a SmartCursor via QObject signals.
 *
 * This class provides notifications of changes to a SmartCursor such as the
 * position in the document, and deletion or insertion of text immediately
 * before or after the cursor.
 *
 * If you prefer to receive notifications via virtual inheritance, see SmartCursorWatcher.
 *
 * \sa SmartCursor, SmartCursorNotifier
 */
class KTEXTEDITOR_EXPORT SmartCursorNotifier : public QObject
{
  Q_OBJECT

  public:
    SmartCursorNotifier();

    /**
     * Returns whether this notifier will notify of changes that happen
     * directly to the cursor, e.g. by calls to SmartCursor::setPosition(), rather
     * than just when surrounding text changes.
     */
    bool wantsDirectChanges() const;

    /**
     * Set whether this notifier should notify of changes that happen
     * directly to the cursor, e.g. by calls to SmartCursor::setPosition(), rather
     * than just when surrounding text changes.
     */
    void setWantsDirectChanges(bool wantsDirectChanges);

  signals:
    /**
     * The cursor's position was changed.
     */
    void positionChanged(KTextEditor::SmartCursor* cursor);

    /**
     * The cursor's surrounding characters were both deleted simultaneously.
     * The cursor is automatically placed at the start of the deleted region.
     */
    void positionDeleted(KTextEditor::SmartCursor* cursor);

    /**
     * One character immediately surrounding the cursor was deleted.
     * If both characters are simultaneously deleted, positionDeleted() is called instead.
     *
     * \param deletedBefore @c true if the character immediately before was deleted,
     *               @c false if the character immediately after was deleted.
     */
    void characterDeleted(KTextEditor::SmartCursor* cursor, bool deletedBefore);

    /**
     * A character was inserted immediately before or after the cursor.
     *
     * Whether the char was inserted before or after this cursor depends on
     * moveOnInsert():
     * @li true -> the char was inserted before
     * @li false -> the char was inserted after
     */
    void characterInserted(KTextEditor::SmartCursor* cursor, bool insertedBefore);

  private:
    bool m_wantDirectChanges;
};

}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
