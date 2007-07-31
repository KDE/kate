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
#include <QTreeView>
#include <QTextEdit>
#include <QModelIndex>
#include <QBrush>
#include <ktexteditor/codecompletionmodel.h>
#include <kiconloader.h>
#include "katecompletiondelegate.h"
#include "expandingwidgetmodel.h"

QIcon ExpandingWidgetModel::m_expandedIcon;
QIcon ExpandingWidgetModel::m_collapsedIcon;

using namespace KTextEditor;

inline QModelIndex firstColumn( const QModelIndex& index ) {
    return index.sibling(index.row(), 0);
}

ExpandingWidgetModel::ExpandingWidgetModel( QWidget* parent ) : 
        QAbstractTableModel(parent)
{
}

ExpandingWidgetModel::~ExpandingWidgetModel() {
    clearExpanding();
}

QVariant ExpandingWidgetModel::data( const QModelIndex & index, int role ) const {
  switch( role ) {
    case Qt::BackgroundRole:
    {
      if( index.column() == 0 ) {
        //Highlight by match-quality
        int matchQuality = contextMatchQuality(index);
        if( matchQuality != -1 )
        {
          uint badMatchColor = 0xffff0000; //Full red
          uint goodMatchColor = 0xff00ff00; //Full green
          uint totalColor = (badMatchColor*(10-matchQuality) + goodMatchColor*matchQuality)/10;
          return QBrush(totalColor);
        }
      }
      
      //Use a special background-color for expanded items
      if( isExpanded(index) ) {
        bool alternate = index.row() & 1;
        if( alternate )
          return QBrush(0xffd8ca6c);
        else
          return QBrush(0xffeddc6a);
      }
    }
  }
  return QVariant();
}

void ExpandingWidgetModel::clearMatchQualities() {
    m_contextMatchQualities.clear();
}

QModelIndex ExpandingWidgetModel::partiallyExpandedRow() const {
    if( m_partiallyExpanded.isEmpty() )
        return QModelIndex();
    else
        return m_partiallyExpanded.begin().key();
}

void ExpandingWidgetModel::clearExpanding() {
    clearMatchQualities();
    m_expandState.clear();
    foreach( QWidget* widget, m_expandingWidgets )
      delete widget;
    m_expandingWidgets.clear();
}

bool ExpandingWidgetModel::isPartiallyExpanded(const QModelIndex& index) const {
  return m_partiallyExpanded.contains(firstColumn(index));
}

void ExpandingWidgetModel::partiallyUnExpand(const QModelIndex& idx_)
{
  QModelIndex index( firstColumn(idx_) );
  m_partiallyExpanded.remove(index);
  m_partiallyExpanded.remove(idx_);
}

int ExpandingWidgetModel::partiallyExpandWidgetHeight() const {
  return 60; ///@todo use font-metrics text-height*2 for 2 lines
}

void ExpandingWidgetModel::rowSelected(const QModelIndex& idx_)
{
  QModelIndex idx( firstColumn(idx_) );
  if( !m_partiallyExpanded.contains( idx ) )
  {
      QModelIndex oldIndex = partiallyExpandedRow();
      //Unexpand the previous partially expanded row
      if( !m_partiallyExpanded.isEmpty() )
      { ///@todo allow multiple partially expanded rows
        while( !m_partiallyExpanded.isEmpty() )
            m_partiallyExpanded.erase(m_partiallyExpanded.begin());
            //partiallyUnExpand( m_partiallyExpanded.begin().key() );
      }
    
      //Notify the underlying models that the item was selected, and eventually get back the text for the expanding widget.
      if( !idx.isValid() ) {
        //All items have been unselected
        if( oldIndex.isValid() )
          emit dataChanged(oldIndex, oldIndex);
      } else {
        QVariant variant = data(idx, CodeCompletionModel::ItemSelected);

        if( !isExpanded(idx) && variant.type() == QVariant::String) {
           //kDebug() << "expanding: " << idx << endl;
          m_partiallyExpanded.insert(idx, true);

          //Say that one row above until one row below has changed, so no items will need to be moved(the space that is taken from one item is given to the other)
          if( oldIndex.isValid() && oldIndex < idx )
            emit dataChanged(oldIndex, idx);
          else if( oldIndex.isValid() &&  idx < oldIndex )
            emit dataChanged(idx, oldIndex);
          else
            emit dataChanged(idx, idx);
        } else if( oldIndex.isValid() ) {
          //We are not partially expanding a new row, but we previously had a partially expanded row. So signalize that it has been unexpanded.
            emit dataChanged(oldIndex, oldIndex);
        }
        if( isExpanded(idx) )
          kDebug() << "ExpandingWidgetModel::rowSelected: row is already expanded" << endl;
        if( variant.type() != QVariant::String )
          kDebug() << "ExpandingWidgetModel::rowSelected: no string returned " << endl;
    }
  }else{
    kDebug() << "ExpandingWidgetModel::rowSelected: Row is already partially expanded" << endl;
  }
}

QString ExpandingWidgetModel::partialExpandText(const QModelIndex& idx) const {
  if( !idx.isValid() )
    return QString();

  return data(firstColumn(idx), CodeCompletionModel::ItemSelected).toString();
}

QRect ExpandingWidgetModel::partialExpandRect(const QModelIndex& idx_) const 
{
  QModelIndex idx(firstColumn(idx_));
  
  if( !idx.isValid() )
    return QRect();
  
    //Get the whole rectangle of the row:
    QModelIndex rightMostIndex = idx;
    QModelIndex tempIndex = idx;
    while( (tempIndex = rightMostIndex.sibling(rightMostIndex.row(), rightMostIndex.column()+1)).isValid() )
      rightMostIndex = tempIndex;

    QRect rect = treeView()->visualRect(idx);
    QRect rightMostRect = treeView()->visualRect(rightMostIndex);

    rect.setLeft( rect.left() + 20 );
    rect.setRight( rightMostRect.right() - 5 );

    //These offsets must match exactly those used in KateCompletionDelegate::sizeHint()
    rect.setTop( rect.top() + basicRowHeight(idx) + 5 );
    rect.setBottom( rightMostRect.bottom() - 5 );

    return rect;
}

bool ExpandingWidgetModel::isExpandable(const QModelIndex& idx_) const
{
  QModelIndex idx(firstColumn(idx_));
  
  if( !m_expandState.contains(idx) )
  {
    m_expandState.insert(idx, NotExpandable);
    QVariant v = data(idx, CodeCompletionModel::IsExpandable);
    if( v.canConvert<bool>() && v.value<bool>() )
        m_expandState[idx] = Expandable;
  }

  return m_expandState[idx] != NotExpandable;
}

bool ExpandingWidgetModel::isExpanded(const QModelIndex& idx_) const 
{
    QModelIndex idx(firstColumn(idx_));
    return m_expandState.contains(idx) && m_expandState[idx] == Expanded;
}

void ExpandingWidgetModel::setExpanded(QModelIndex idx_, bool expanded)
{
  QModelIndex idx(firstColumn(idx_));
    
  //kDebug() << "Setting expand-state of row " << idx.row() << " to " << expanded << endl;
  if( !idx.isValid() )
    return;
  
  if( isExpandable(idx) ) {
    if( !expanded && m_expandingWidgets.contains(idx) && m_expandingWidgets[idx] ) {
      m_expandingWidgets[idx]->hide();
    }
      
    m_expandState[idx] = expanded ? Expanded : Expandable;

    if( expanded )
      partiallyUnExpand(idx);
    
    if( expanded && !m_expandingWidgets.contains(idx) )
    {
      QVariant v = data(idx, CodeCompletionModel::ExpandingWidget);
      
      if( v.canConvert<QWidget*>() ) {
        m_expandingWidgets[idx] = v.value<QWidget*>();
      } else if( v.canConvert<QString>() ) {
        //Create a html widget that shows the given string
        QTextEdit* edit = new QTextEdit( v.value<QString>() );
        edit->setReadOnly(true);
        edit->resize(200, 50); //Make the widget small so it embeds nicely.
        m_expandingWidgets[idx] = edit;
      } else {
        m_expandingWidgets[idx] = 0;
      }
    }

    //Eventually partially expand the row
    if( !expanded && treeView()->currentIndex() == idx && !isPartiallyExpanded(idx) )
      rowSelected(idx); //Partially expand the row.
    
    emit dataChanged(idx, idx);
    
    treeView()->scrollTo(idx);
  }
}

int ExpandingWidgetModel::basicRowHeight( const QModelIndex& idx_ ) const 
{
  QModelIndex idx(firstColumn(idx_));
    
    KateCompletionDelegate* delegate = dynamic_cast<KateCompletionDelegate*>( treeView()->itemDelegate(idx) );
    if( !delegate || !idx.isValid() ) {
    kDebug() << "ExpandingWidgetModel::basicRowHeight: Could not get delegate" << endl;
    return 15;
    }
    return delegate->basicSizeHint( idx ).height();
}


void ExpandingWidgetModel::placeExpandingWidget(const QModelIndex& idx_) 
{
  QModelIndex idx(firstColumn(idx_));

  QWidget* w = 0;
  if( m_expandingWidgets.contains(idx) )
    w = m_expandingWidgets[idx];
  
  if( w && isExpanded(idx) ) {
      if( !idx.isValid() )
        return;
      
      if( idx.parent().isValid() && !treeView()->isExpanded(idx.parent()) ) {
          //The item is in a group that is currently not expanded in tree-like way
          w->hide();
          return;
      }

      QModelIndex rightMostIndex = idx;
      QModelIndex tempIndex = idx;
      while( (tempIndex = rightMostIndex.sibling(rightMostIndex.row(), rightMostIndex.column()+1)).isValid() )
        rightMostIndex = tempIndex;

      QRect rect = treeView()->visualRect(idx);
      QRect rightMostRect = treeView()->visualRect(rightMostIndex);

      //Find out the basic height of the row
      rect.setLeft( rect.left() + 20 );
      rect.setRight( rightMostRect.right() - 5 );

      //These offsets must match exactly those used in KateCompletionDeleage::sizeHint()
      rect.setTop( rect.top() + basicRowHeight(idx) + 5 );
      rect.setBottom( rightMostRect.bottom() - 5 );

      if( w->parent() != treeView()->viewport() || w->geometry() != rect || !w->isVisible() ) {
        w->setParent( treeView()->viewport() );

        w->setGeometry(rect);
        w->show();
      }
  }
}

void ExpandingWidgetModel::placeExpandingWidgets() {
  for( QMap<QPersistentModelIndex, QWidget*>::const_iterator it = m_expandingWidgets.begin(); it != m_expandingWidgets.end(); ++it ) {
    placeExpandingWidget(it.key());
  }
}

QWidget* ExpandingWidgetModel::expandingWidget(const QModelIndex& idx_) const {
  QModelIndex idx(firstColumn(idx_));

  if( m_expandingWidgets.contains(idx) )
    return m_expandingWidgets[idx];
  else
    return 0;
}

void ExpandingWidgetModel::cacheIcons() const {
    if( m_expandedIcon.isNull() )
      m_expandedIcon = KIconLoader::global()->loadIcon("go-down", K3Icon::Small);
    
    if( m_collapsedIcon.isNull() )
      m_collapsedIcon = KIconLoader::global()->loadIcon("go-next", K3Icon::Small);
}

#include "expandingwidgetmodel.moc"

