/* This file is part of the KDE libraries
   Copyright (C) 2003,2004,2005 Hamish Rodda <rodda@kde.org>

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

#ifndef KATERANGELIST_H
#define KATERANGELIST_H

#include "katesmartrange.h"

/**
 * A class which contains a list of KTextEditor::Ranges. Mostly a convenience class,
 * as storage of the relationships is done within the ranges themselves.
 */
class KateRangeList : public QObject
{
  Q_OBJECT

  public:
    KateRangeList(KateDocument* doc, QObject* parent = 0L, const char* name = 0L);
    virtual ~KateRangeList();

    operator KateDocument*() const;
    
    /// Verification method. This should always return true. For debugging purposes.
    bool isValid() const;

    /**
     * Override to emit rangeEliminated() signals.
     */
    virtual void clear();

    /**
     * Finds the most specific range in a heirachy for the given input range
     * (ie. the smallest range which wholly contains the input range)
     */
    KateSmartRange* findMostSpecificRange(const KTextEditor::Range& input) const;

    /**
     * Finds the first range which includes position \p pos.
     */
    KateSmartRange* firstRangeIncluding(const KTextEditor::Cursor& pos) const;
    
    /**
     * Finds the deepest range which includes position \p pos.
     */
    KateSmartRange* deepestRangeIncluding(const KTextEditor::Cursor& pos) const;

    /**
     * @retval true if one of the ranges other than the topmost in the list includes @p cursor
     * @retval false otherwise
     */
    bool rangesInclude(const KTextEditor::Cursor& cursor);

    /**
     * @returns the top and all encompasing range
     */
    KateSmartRange* topRange() const;

    /**
     * @returns the KateRangeType for ranges in this list
     */
    KateRangeType* rangeType() const;

    /**
     * Set a default range type that will be applied to ranges in this
     * list if they don't already have a different type applied.
     * FIXME implement
     */
    void setRangeType(KateRangeType* t);

    /**
     * An action to enable / disable the functionality (highlighting,
     * input filtering etc) of this list.
     */
    KAction* enableDisableAction() const;
    void setEnableDisableAction(KAction* action);

    void tagAll() const;

  signals:
    /**
     * Connected to all ranges if connect()ed.
     */
    void tagRange(KateSmartRange* range);

  private:
    KateDocument* m_doc;
    KateSmartRange* m_topRange;
    KateRangeType* m_rangeType;
};

#endif
