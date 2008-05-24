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

#include "expandingwidgetmodel.h"

#include <QTreeView>
#include <QModelIndex>
#include <QBrush>

#include <ktexteditor/codecompletionmodel.h>
#include <kiconloader.h>
#include <ktextedit.h>

#include "expandingdelegate.h"

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

uint ExpandingWidgetModel::matchColor(const QModelIndex& index) const {
  
  int matchQuality = contextMatchQuality( index.sibling(index.row(), 0) );
  
  if( matchQuality != -1 )
  {
    bool alternate = index.row() & 1;
    
    quint64 badMatchColor = 0xff7777ff; //Full blue
    quint64 goodMatchColor = 0xff77ff77; //Full green

    if( alternate ) {
      badMatchColor += 0x00080000;
      goodMatchColor += 0x00080000;
    }
    uint totalColor = (badMatchColor*(10-matchQuality) + goodMatchColor*matchQuality)/10;
    return totalColor;
  }else{
    return 0;
  }
}
    

QVariant ExpandingWidgetModel::data( const QModelIndex & index, int role ) const
{
  switch( role ) {
    case Qt::BackgroundRole:
    {
      if( index.column() == 0 ) {
        //Highlight by match-quality
        uint color = matchColor(index);
        if( color )
          return QBrush( color );
      }
      ///@todo Choose a color from the color scheme
      //Use a special background-color for expanded items
      if( isExpanded(index) ) {
        if( index.row() & 1 )
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
    QMap<QPersistentModelIndex,ExpandingWidgetModel::ExpandingType> oldExpandState = m_expandState;
    foreach( QPointer<QWidget> widget, m_expandingWidgets )
      delete widget;
    m_expandingWidgets.clear();
    m_expandState.clear();

    for( QMap<QPersistentModelIndex, ExpandingWidgetModel::ExpandingType>::const_iterator it = oldExpandState.begin(); it != oldExpandState.end(); ++it )
      if(it.value() == Expanded)
      	emit dataChanged(it.key(), it.key());
}

ExpandingWidgetModel::ExpansionType ExpandingWidgetModel::isPartiallyExpanded(const QModelIndex& index) const {
  if( m_partiallyExpanded.contains(firstColumn(index)) )
      return m_partiallyExpanded[firstColumn(index)];
  else
      return NotExpanded;
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
            
          //Either expand upwards or downwards, choose in a way that 
          //the visible fields of the new selected entry are not moved.
          if( oldIndex.isValid() && (oldIndex < idx || (!(oldIndex < idx) && oldIndex.parent() < idx.parent()) ) )
            m_partiallyExpanded.insert(idx, ExpandUpwards);
          else
            m_partiallyExpanded.insert(idx, ExpandDownwards);

          //Say that one row above until one row below has changed, so no items will need to be moved(the space that is taken from one item is given to the other)
          if( oldIndex.isValid() && oldIndex < idx ) {
            emit dataChanged(oldIndex, idx);

            if( treeView()->verticalScrollMode() == QAbstractItemView::ScrollPerItem )
            {
              //Qt fails to correctly scroll in ScrollPerItem mode, so the selected index is completely visible,
              //so we do the scrolling by hand.
              QRect selectedRect = treeView()->visualRect(idx);
              QRect frameRect = treeView()->frameRect();

              if( selectedRect.bottom() > frameRect.bottom() ) {
                int diff = selectedRect.bottom() - frameRect.bottom();
                //We need to scroll down
                QModelIndex newTopIndex = idx;
                
                QModelIndex nextTopIndex = idx;
                QRect nextRect = treeView()->visualRect(nextTopIndex);
                while( nextTopIndex.isValid() && nextRect.isValid() && nextRect.top() >= diff ) {
                  newTopIndex = nextTopIndex;
                  nextTopIndex = treeView()->indexAbove(nextTopIndex);
                  if( nextTopIndex.isValid() )
                    nextRect = treeView()->visualRect(nextTopIndex);
                }
                treeView()->scrollTo( newTopIndex, QAbstractItemView::PositionAtTop );
              }
            }

            //This is needed to keep the item we are expanding completely visible. Qt does not scroll the view to keep the item visible.
            //But we must make sure that it isn't too expensive.
            //We need to make sure that scrolling is efficient, and the whole content is not repainted.
            //Since we are scrolling anyway, we can keep the next line visible, which might be a cool feature.
            
            //Since this also doesn't work smoothly, leave it for now
            //treeView()->scrollTo( nextLine, QAbstractItemView::EnsureVisible ); 
          } else if( oldIndex.isValid() &&  idx < oldIndex ) {
            emit dataChanged(idx, oldIndex);
            
            //For consistency with the down-scrolling, we keep one additional line visible above the current visible.
            
            //Since this also doesn't work smoothly, leave it for now
/*            QModelIndex prevLine = idx.sibling(idx.row()-1, idx.column());
            if( prevLine.isValid() )
                treeView()->scrollTo( prevLine );*/
          } else
            emit dataChanged(idx, idx);
        } else if( oldIndex.isValid() ) {
          //We are not partially expanding a new row, but we previously had a partially expanded row. So signalize that it has been unexpanded.
          
            emit dataChanged(oldIndex, oldIndex);
        }
    }
  }else{
    kDebug( 13035 ) << "ExpandingWidgetModel::rowSelected: Row is already partially expanded";
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
  
  ExpansionType expansion = ExpandDownwards;
  
  if( m_partiallyExpanded.find(idx) != m_partiallyExpanded.end() )
      expansion = m_partiallyExpanded[idx];
  
    //Get the whole rectangle of the row:
    QModelIndex rightMostIndex = idx;
    QModelIndex tempIndex = idx;
    while( (tempIndex = rightMostIndex.sibling(rightMostIndex.row(), rightMostIndex.column()+1)).isValid() )
      rightMostIndex = tempIndex;

    QRect rect = treeView()->visualRect(idx);
    QRect rightMostRect = treeView()->visualRect(rightMostIndex);

    rect.setLeft( rect.left() + 20 );
    rect.setRight( rightMostRect.right() - 5 );

    //These offsets must match exactly those used in ExpandingDelegate::sizeHint()
    int top = rect.top() + 5;
    int bottom = rightMostRect.bottom() - 5 ;
    
    if( expansion == ExpandDownwards )
        top += basicRowHeight(idx);
    else
        bottom -= basicRowHeight(idx);
    
    rect.setTop( top );
    rect.setBottom( bottom );

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
    
  //kDebug( 13035 ) << "Setting expand-state of row " << idx.row() << " to " << expanded;
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
        KTextEdit* edit = new KTextEdit( v.value<QString>() );
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
    
    ExpandingDelegate* delegate = dynamic_cast<ExpandingDelegate*>( treeView()->itemDelegate(idx) );
    if( !delegate || !idx.isValid() ) {
    kDebug( 13035 ) << "ExpandingWidgetModel::basicRowHeight: Could not get delegate";
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
      
      QRect rect = treeView()->visualRect(idx);

      if( !rect.isValid() || rect.bottom() < 0 || rect.top() >= treeView()->height() ) {
          //The item is currently not visible
          w->hide();
          return;
      }

      QModelIndex rightMostIndex = idx;
      QModelIndex tempIndex = idx;
      while( (tempIndex = rightMostIndex.sibling(rightMostIndex.row(), rightMostIndex.column()+1)).isValid() )
        rightMostIndex = tempIndex;

      QRect rightMostRect = treeView()->visualRect(rightMostIndex);

      //Find out the basic height of the row
      rect.setLeft( rect.left() + 20 );
      rect.setRight( rightMostRect.right() - 5 );

      //These offsets must match exactly those used in KateCompletionDeleage::sizeHint()
      rect.setTop( rect.top() + basicRowHeight(idx) + 5 );
      rect.setHeight( w->height() );

      if( w->parent() != treeView()->viewport() || w->geometry() != rect || !w->isVisible() ) {
        w->setParent( treeView()->viewport() );

        w->setGeometry(rect);
        w->show();
      }
  }
}

void ExpandingWidgetModel::placeExpandingWidgets() {
  for( QMap<QPersistentModelIndex, QPointer<QWidget> >::const_iterator it = m_expandingWidgets.begin(); it != m_expandingWidgets.end(); ++it ) {
    placeExpandingWidget(it.key());
  }
}

int ExpandingWidgetModel::expandingWidgetsHeight() const
{
  int sum = 0;
  for( QMap<QPersistentModelIndex, QPointer<QWidget> >::const_iterator it = m_expandingWidgets.begin(); it != m_expandingWidgets.end(); ++it ) {
    if( isExpanded(it.key() ) && (*it) )
      sum += (*it)->height();
  }
  return sum;
}


QWidget* ExpandingWidgetModel::expandingWidget(const QModelIndex& idx_) const
{
  QModelIndex idx(firstColumn(idx_));

  if( m_expandingWidgets.contains(idx) )
    return m_expandingWidgets[idx];
  else
    return 0;
}

void ExpandingWidgetModel::cacheIcons() const {
    if( m_expandedIcon.isNull() )
      m_expandedIcon = KIconLoader::global()->loadIcon("go-down", KIconLoader::Small);
    
    if( m_collapsedIcon.isNull() )
      m_collapsedIcon = KIconLoader::global()->loadIcon("go-next", KIconLoader::Small);
}

QList<QVariant> mergeCustomHighlighting( int leftSize, const QList<QVariant>& left, int rightSize, const QList<QVariant>& right )
{
  QList<QVariant> ret = left;
  if( left.isEmpty() ) {
    ret << QVariant(0);
    ret << QVariant(leftSize);
    ret << QTextFormat(QTextFormat::CharFormat);
  }

  if( right.isEmpty() ) {
    ret << QVariant(leftSize);
    ret << QVariant(rightSize);
    ret << QTextFormat(QTextFormat::CharFormat);
  } else {
    QList<QVariant>::const_iterator it = right.begin();
    while( it != right.end() ) {
      {
        QList<QVariant>::const_iterator testIt = it;
        for(int a = 0; a < 2; a++) {
          ++testIt;
          if(testIt == right.end()) {
            kWarning() << "Length of input is not multiple of 3";
            break;
          }
        }
      }
        
      ret << QVariant( (*it).toInt() + leftSize );
      ++it;
      ret << QVariant( (*it).toInt() );
      ++it;
      ret << *it;
      if(!(*it).value<QTextFormat>().isValid())
        kDebug( 13035 ) << "Text-format is invalid";
      ++it;
    }
  }
  return ret;
}

//It is assumed that between each two strings, one space is inserted
QList<QVariant> mergeCustomHighlighting( QStringList strings, QList<QVariantList> highlights, int grapBetweenStrings )
{
    if(strings.isEmpty())   {
      kWarning() << "List of strings is empty";
      return QList<QVariant>();
    }
    
    if(highlights.isEmpty())   {
      kWarning() << "List of highlightings is empty";
      return QList<QVariant>();
    }

    if(strings.count() != highlights.count()) {
      kWarning() << "Length of string-list is " << strings.count() << " while count of highlightings is " << highlights.count() << ", should be same";
      return QList<QVariant>();
    }
    
    //Merge them together
    QString totalString = strings[0];
    QVariantList totalHighlighting = highlights[0];
    
    strings.pop_front();
    highlights.pop_front();

    while( !strings.isEmpty() ) {
      totalHighlighting = mergeCustomHighlighting( totalString.length(), totalHighlighting, strings[0].length(), highlights[0] );
      totalString += strings[0];

      for(int a = 0; a < grapBetweenStrings; a++)
        totalString += ' ';
      
      strings.pop_front();
      highlights.pop_front();
      
    }
    //Combine the custom-highlightings
    return totalHighlighting;
}
#include "expandingwidgetmodel.moc"

