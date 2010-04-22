/* This file is part of the KDE project
 *
 *  Copyright (C) 2010 Christoph Cullmann <cullmann@kde.org>
 *
 *  Based on code of the SmartCursor/Range by:
 *  Copyright (C) 2003-2005 Hamish Rodda <rodda@kde.org>
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

#ifndef KDELIBS_KTEXTEDITOR_MOVINGINTERFACE_H
#define KDELIBS_KTEXTEDITOR_MOVINGINTERFACE_H

#include <ktexteditor/ktexteditor_export.h>
#include <ktexteditor/movingcursor.h>
#include <ktexteditor/movingrange.h>
#include <ktexteditor/movingrangefeedback.h>

namespace KTextEditor
{

/**
 * \short A class which provides the interface to create MovingCursors and MovingRanges.
 *
 * \ingroup kte_group_moving_classes
 *
 * This class provides the interface for KTextEditor::Documents to create MovingCursors/Ranges.
 *
 * \author Christoph Cullmann \<cullmann@kde.org\>
 *
 * \since 4.5
 */
class KTEXTEDITOR_EXPORT MovingInterface
{
  public:
    /**
     * Default constructor
     */
    MovingInterface ();

    /**
     * Virtual destructor
     */
    virtual ~MovingInterface ();

    /**
     * Create a new moving cursor for this document.
     * @param position position of the moving cursor to create
     * @param insertBehavior insertion behavior
     * @return new moving cursor for the document
     */
    virtual MovingCursor *newMovingCursor (const Cursor &position, MovingCursor::InsertBehavior insertBehavior = MovingCursor::MoveOnInsert) = 0;

    /**
     * Create a new moving range for this document.
     * @param range range of the moving range to create
     * @param insertBehaviors insertion behaviors
     * @return new moving range for the document
     */
    virtual MovingRange *newMovingRange (const Range &range, MovingRange::InsertBehaviors insertBehaviors = MovingRange::DoNotExpand) = 0;

  private:
    /**
     * private d-pointer
     */
    class MovingInterfacePrivate * const d;
};

}

Q_DECLARE_INTERFACE(KTextEditor::MovingInterface, "org.kde.KTextEditor.MovingInterface")

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
