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

#ifndef KATEARGUMENTHINTTREE_H
#define KATEARGUMENTHINTTREE_H

#include "kateexpandingtree.h"

class KateCompletionWidget;
class KateArgumentHintModel;
class QRect;

class KateArgumentHintTree : public KateExpandingTree {
  Q_OBJECT
  public:
    KateArgumentHintTree( KateCompletionWidget* parent );

    // Navigation
    bool nextCompletion();
    bool previousCompletion();
    bool pageDown();
    bool pageUp();
    void top();
    void bottom();

    void  clearCompletion();
  public slots:
    void updateGeometry();
    void updateGeometry(QRect rect);
  protected:
    virtual void paintEvent ( QPaintEvent * event );
    virtual void rowsInserted ( const QModelIndex & parent, int start, int end );
    virtual void dataChanged ( const QModelIndex & topLeft, const QModelIndex & bottomRight );
    virtual void currentChanged ( const QModelIndex & current, const QModelIndex & previous );
  private:
    KateArgumentHintModel* model() const;
    
    KateCompletionWidget* m_parent;
};

#endif
