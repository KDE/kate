/* This file is part of the KDE project
   Copyright (C) 2005 Hamish Rodda <rodda@kde.org>

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

#ifndef KDELIBS_KTEXTEDITOR_SMARTINTERFACE_H
#define KDELIBS_KTEXTEDITOR_SMARTINTERFACE_H

#include <ktexteditor/range.h>

namespace KTextEditor
{

/**
 * \short An interface for creating SmartCursors and SmartRanges.
 *
 * Use this interface to create new SmartCursors and SmartRanges.
 *
 * You then own these objects; upon deletion they de-register themselves from
 * the Document with which they were associated.  Alternatively, they are all
 * deleted with the deletion of the owning Document.
 */
class KTEXTEDITOR_EXPORT SmartInterface
{
  public:
    virtual ~SmartInterface();

    // New cursor methods
    /**
     * Creates a new SmartCursor.
     * \param position The initial cursor position assumed by the new cursor.
     * \param moveOnInsert Define whether the cursor should move when text is inserted at the cursor position.
     */
    virtual SmartCursor* newSmartCursor(const Cursor& position, bool moveOnInsert = true) = 0;
    /// \overload
    inline SmartCursor* newSmartCursor(bool moveOnInsert = true)
      { return newSmartCursor(Cursor(), moveOnInsert); }
    /// \overload
    inline SmartCursor* newSmartCursor(int line, int column, bool moveOnInsert = true)
      { return newSmartCursor(Cursor(line, column), moveOnInsert); }

    /**
     * Creates a new SmartRange.
     * \param position The initial text range assumed by the new range.
     * \param parent The parent SmartRange, if this is to be the child of an existing range.
     * \param insertBehaviour Define whether the range should expand when text is inserted at ends of the range.
     */
    virtual SmartRange* newSmartRange(const Range& range, SmartRange* parent = 0L, int insertBehaviour = SmartRange::DoNotExpand) = 0;
    /// \overload
    inline SmartRange* newSmartRange(SmartRange* parent = 0L, int insertBehaviour = SmartRange::DoNotExpand)
      { return newSmartRange(Range(), parent, insertBehaviour); }
    /// \overload
    inline SmartRange* newSmartRange(const Cursor& startPosition, const Cursor& endPosition, SmartRange* parent = 0L, int insertBehaviour = SmartRange::DoNotExpand)
      { return newSmartRange(Range(startPosition, endPosition), parent, insertBehaviour); }
    /// \overload
    inline SmartRange* newSmartRange(int startLine, int startColumn, int endLine, int endColumn, SmartRange* parent = 0L, int insertBehaviour = SmartRange::DoNotExpand)
      { return newSmartRange(Range(startLine, startColumn, endLine, endColumn), parent, insertBehaviour); }
};

KTEXTEDITOR_EXPORT SmartInterface *smartInterface(class Document *doc);

}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
