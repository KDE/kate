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

#ifndef KATEARGUMENTHINTMODEL_H
#define KATEARGUMENTHINTMODEL_H

#include <QAbstractListModel>
#include "katecompletionmodel.h"
#include "expandingtree/expandingwidgetmodel.h"

class KateCompletionWidget;

class KateArgumentHintModel : public ExpandingWidgetModel {
  Q_OBJECT
  public:
    KateArgumentHintModel( KateCompletionWidget* parent );

    virtual QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
    
    virtual int rowCount ( const QModelIndex & parent ) const;

    virtual int columnCount ( const QModelIndex & /*parent*/ ) const;

    virtual QTreeView* treeView() const;

    virtual bool indexIsItem(const QModelIndex& index) const;

    void emitDataChanged( const QModelIndex& start, const QModelIndex& end );
    
    void buildRows();
    void clear();
  protected:
    virtual int contextMatchQuality(const QModelIndex& row) const;
  public slots:
    void parentModelReset();
  private:
    KateCompletionModel::Group* group() const;
    KateCompletionModel* model() const;

    QList<int> m_rows; //Maps rows to either a positive row-number in the source group, or to a negative number which indicates a label
    
    KateCompletionWidget* m_parent;
};

#endif
