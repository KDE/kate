/* This file is part of the KDE libraries
   Copyright (C) 2006 Hamish Rodda <rodda@kde.org>
   Copyright (C) 2007-2008 David Nolden <david.nolden.kdevelop@art-master.de>

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

#ifndef KATECOMPLETIONTREE_H
#define KATECOMPLETIONTREE_H

#include "expandingtree/expandingtree.h"

class KateCompletionWidget;
class KateCompletionModel;

class QTimer;

class KateCompletionTree : public ExpandingTree
{
  Q_OBJECT

  public:
    explicit KateCompletionTree(KateCompletionWidget* parent);

    KateCompletionWidget* widget() const;
    KateCompletionModel* kateModel() const;

    void resizeColumns(bool fromResizeEvent = false, bool firstShow = false, bool forceResize = false);

    // Navigation
    bool nextCompletion();
    bool previousCompletion();
    bool pageDown();
    bool pageUp();
    void top();
    void bottom();
    
    void scheduleUpdate();

  private slots:
    void resizeColumnsSlot();

  protected:
    virtual void currentChanged ( const QModelIndex & current, const QModelIndex & previous ); ///Not available as a signal in this way
    virtual void scrollContentsBy(int dx, int dy);
    virtual QStyleOptionViewItem viewOptions() const;
  private:
    QTimer* m_resizeTimer;
};

#endif
