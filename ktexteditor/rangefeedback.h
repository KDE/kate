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

}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
