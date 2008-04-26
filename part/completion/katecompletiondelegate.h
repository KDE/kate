/* This file is part of the KDE libraries
   Copyright (C) 2007 David Nolden <david.nolden.kdevelop@art-master.de>

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

#ifndef KATECOMPLETIONDELEGATE_H
#define KATECOMPLETIONDELEGATE_H

#include "expandingtree/expandingdelegate.h"

class KateCompletionDelegate : public ExpandingDelegate {
  public:
    explicit KateCompletionDelegate(ExpandingWidgetModel* model, KateCompletionWidget* parent);
    
    KateRenderer* renderer() const;
    KateCompletionWidget* widget() const;
    KateDocument* document() const;
  protected:
    virtual void adjustStyle( const QModelIndex& index, QStyleOptionViewItem & option ) const;
    mutable int m_cachedRow;
    mutable QList<int> m_cachedColumnStarts;
    virtual void heightChanged() const;
    QList<QTextLayout::FormatRange> createHighlighting(const QModelIndex& index, QStyleOptionViewItem& option) const;
    
};

#endif
