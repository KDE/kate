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
#include <ktexteditor/codecompletionmodel.h>
#include <kiconloader.h>
#include "katecompletiondelegate.h"
#include "expandingwidgetmodel.h"

QIcon ExpandingWidgetModel::m_expandedIcon;
QIcon ExpandingWidgetModel::m_collapsedIcon;

using namespace KTextEditor;

ExpandingWidgetModel::ExpandingWidgetModel( QWidget* parent ) : 
        QAbstractTableModel(parent)
        , m_partiallyExpandedRow(-1)
        , m_partiallyExpandWidget(new QTextEdit())
{
  m_partiallyExpandWidget->setReadOnly(true);
  m_partiallyExpandWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

ExpandingWidgetModel::~ExpandingWidgetModel() {
    clearExpanding();
    delete m_partiallyExpandWidget;
}

void ExpandingWidgetModel::clearExpanding() {
    m_partiallyExpandedRow = -1;
    m_partiallyExpandWidget->hide();
    m_expandState.clear();
    foreach( QWidget* widget, m_expandingWidgets )
      delete widget;
    m_expandingWidgets.clear();
}

bool ExpandingWidgetModel::isPartiallyExpanded(int row) const {
  return row == m_partiallyExpandedRow;
}

void ExpandingWidgetModel::partiallyUnExpand(int row)
{
  if( m_partiallyExpandedRow == row )
  {
      m_partiallyExpandedRow = -1;
      m_partiallyExpandWidget->hide();
  }
}

int ExpandingWidgetModel::partiallyExpandWidgetHeight() const {
  return m_partiallyExpandWidget->height();
}

void ExpandingWidgetModel::rowSelected(int row)
{
   //kDebug() << "row selected: " << row << endl;
  if( m_partiallyExpandedRow != row )
  {
      QModelIndex oldIndex;
      //Unexpand the previous partially expanded row
      if( m_partiallyExpandedRow != -1)
      {
        oldIndex = index(m_partiallyExpandedRow, 0 );
        partiallyUnExpand(m_partiallyExpandedRow);
      }
    
      //Notify the underlying models that the item was selected, and eventually get back the text for the expanding widget.
      QModelIndex idx( index(row, 0 ) );
      if( !idx.isValid() ) {
        //All items have been unselected
        m_partiallyExpandedRow = -1;
        if( oldIndex.isValid() )
          emit dataChanged(oldIndex, oldIndex);
        return;
      }
    
      QVariant variant = data(idx, CodeCompletionModel::ItemSelected);

    if( !isExpanded(row) && variant.type() == QVariant::String) {
       //kDebug() << "expanding: " << row << endl;
      m_partiallyExpandedRow = row;

      //Say that one row above until one row below has changed, so no items will need to be moved(the space that is taken from one item is given to the other)
      if( oldIndex.isValid() && oldIndex.row() < idx.row() )
        emit dataChanged(oldIndex, idx);
      else if( oldIndex.isValid() && oldIndex.row() > idx.row() )
        emit dataChanged(idx, oldIndex);
      else
        emit dataChanged(idx, idx);
      //Only partially expand unexpanded rows

      m_partiallyExpandWidget->setHtml(variant.toString());

      QTextEdit* w = m_partiallyExpandWidget;

      w->hide(); //Hide the widget until painting takes place
      
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

      if( w->parent() != treeView()->viewport() || w->geometry() != rect ) {
        w->setParent( treeView()->viewport() );

        w->setGeometry(rect);
      
        // To reduce flickering, the widget is not shown here, but instead when the underlying row is painted
      }
    } else if( oldIndex.isValid() ) {
      //We are not partially expanding a new row, but we previously had a partially expanded row. So signalize that it has been unexpanded.
        emit dataChanged(oldIndex, oldIndex);
    }
    if( isExpanded(row) )
      kDebug() << "ExpandingWidgetModel::rowSelected: row is already expanded" << endl;
    if( variant.type() != QVariant::String )
      kDebug() << "ExpandingWidgetModel::rowSelected: no string returned " << endl;
  }else{
    kDebug() << "ExpandingWidgetModel::rowSelected: Row is already partially expanded" << endl;
  }
}

bool ExpandingWidgetModel::isExpandable(const QModelIndex& idx) const
{
  if( !m_expandState.contains(idx.row()) )
  {
    m_expandState.insert(idx.row(), NotExpandable);
    QVariant v = data(idx, CodeCompletionModel::IsExpandable);
    if( v.canConvert<bool>() && v.value<bool>() )
        m_expandState[idx.row()] = Expandable;
  }

  return m_expandState[idx.row()] != NotExpandable;
}

bool ExpandingWidgetModel::isExpanded(int row) const {
  return m_expandState.contains(row) && m_expandState[row] == Expanded;
}

void ExpandingWidgetModel::setExpanded(QModelIndex idx, bool expanded)
{
  if( !idx.isValid() )
    return;
  
  if( isExpandable(idx) ) {
    if( !expanded && m_expandingWidgets.contains(idx.row()) && m_expandingWidgets[idx.row()] ) {
      m_expandingWidgets[idx.row()]->hide();
    }
      
    m_expandState[idx.row()] = expanded ? Expanded : Expandable;

    if( expanded )
      partiallyUnExpand(idx.row());
    
    if( expanded && !m_expandingWidgets.contains(idx.row()) )
    {
      QVariant v = data(idx, CodeCompletionModel::ExpandingWidget);
      
      if( v.canConvert<QWidget*>() ) {
        m_expandingWidgets[idx.row()] = v.value<QWidget*>();
      } else if( v.canConvert<QString>() ) {
        //Create a html widget that shows the given string
        QTextEdit* edit = new QTextEdit( v.value<QString>() );
        edit->setReadOnly(true);
        edit->resize(200, 50); //Make the widget small so it embeds nicely.
        m_expandingWidgets[idx.row()] = edit;
      } else {
        m_expandingWidgets[idx.row()] = 0;
      }
    }

    //Eventually partially expand the row
    if( !expanded && treeView()->currentIndex().row() == idx.row() && m_partiallyExpandedRow != idx.row() )
      rowSelected(idx.row()); //Partially expand the row.
    
    emit dataChanged(idx, idx);
    
    treeView()->scrollTo(idx);
  }
}

int ExpandingWidgetModel::basicRowHeight( const QModelIndex& idx ) const {
      KateCompletionDelegate* delegate = dynamic_cast<KateCompletionDelegate*>( treeView()->itemDelegate(idx) );
      if( !delegate || !idx.isValid() ) {
        kDebug() << "ExpandingWidgetModel::basicRowHeight: Could not get delegate" << endl;
        return 15;
      }
      return delegate->basicSizeHint( idx ).height();
}


void ExpandingWidgetModel::placeExpandingWidget(int row) {
  QWidget* w = 0;
  if( m_expandingWidgets.contains(row) )
    w = m_expandingWidgets[row];
  
  if( w && isExpanded(row) ) {
      QModelIndex idx( index(row, 0 ) );
      if( !idx.isValid() )
        return;

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
  } else if( m_partiallyExpandedRow != -1 && m_partiallyExpandedRow == row ) {
    ///@todo don't embed this, instead paint the content
    m_partiallyExpandWidget->show();
  }
}

void ExpandingWidgetModel::placeExpandingWidgets() {
  for( QHash<int, QWidget*>::const_iterator it = m_expandingWidgets.begin(); it != m_expandingWidgets.end(); ++it ) {
    placeExpandingWidget(it.key());
  }
}

QWidget* ExpandingWidgetModel::expandingWidget(int row) const {
  if( m_expandingWidgets.contains(row) )
    return m_expandingWidgets[row];
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

