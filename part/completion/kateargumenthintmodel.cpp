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

#include <QTextFormat>
#include <QGridLayout>

#include <ktexteditor/codecompletionmodel.h>
#include "kateargumenthintmodel.h"
#include "katecompletionwidget.h"
#include "kateargumenthinttree.h"

using namespace KTextEditor;

QList<QVariant> mergeCustomHighlighting( int leftSize, const QList<QVariant>& left, int rightSize, const QList<QVariant>& right )
{
  QList<QVariant> ret = left;
  if( left.isEmpty() ) {
    ret << QVariant(0);
    ret << QVariant(leftSize);
    ret << QTextFormat();
  }

  if( right.isEmpty() ) {
    ret << QVariant(leftSize);
    ret << QVariant(leftSize+rightSize);
    ret << QTextFormat();
  } else {
    QList<QVariant>::const_iterator it = right.begin();
    while( it != right.end() ) {
      ret << QVariant( (*it).toInt() + leftSize );
      ++it;
      ret << QVariant( (*it).toInt() + leftSize );
      ++it;
      ret << *it;
      ++it;
    }
  }
  return ret;
}

//It is assumed that between each two strings, one space is inserted
QList<QVariant> mergeCustomHighlighting( QStringList strings, QList<QVariantList> highlights )
{
    //Merge them together
    QString totalString = strings[0];
    QVariantList totalHighlighting = highlights[0];

    strings.erase(0);
    highlights.erase(0);

    while( !strings.isEmpty() ) {

      totalHighlighting = mergeCustomHighlighting( totalString.length(), totalHighlighting, strings[0].length(), highlights[0] );
      totalString += strings[0] + " ";
      strings.erase(0);
      highlights.erase(0);
    }
    //Combine the custom-highlightings
    return totalHighlighting;
}

void KateArgumentHintModel::clear() {
  m_rows.clear();
  clearExpanding();
}

void KateArgumentHintModel::parentModelReset() {
  clear();
  buildRows();
}

void KateArgumentHintModel::buildRows() {
  m_rows.clear();
  QMap<int, QList<int> > m_depths; //Map each hint-depth to a list of functions of that depth
  for( int a = 0; a < group()->rows.count(); a++ ) {
    KateCompletionModel::ModelRow source = group()->rows[a];
    QModelIndex  sourceIndex = source.first->index(source.second, 0);
    QVariant v = sourceIndex.data(CodeCompletionModel::ArgumentHintDepth);
    if( v.type() == QVariant::Int ) {
      QList<int>& lst( m_depths[v.toInt()] );
      lst << a;
    }
  }

  for( QMap<int, QList<int> >::const_iterator it = m_depths.begin(); it != m_depths.end(); ++it ) {
    foreach( int row, *it )
      m_rows.push_front(row);//Insert rows in reversed order
    m_rows.push_front( -it.key() );
  }
}

KateArgumentHintModel::KateArgumentHintModel( KateCompletionWidget* parent ) : ExpandingWidgetModel(parent), m_parent(parent) {
  connect(parent->model(), SIGNAL("modelReset()"), this, SLOT(parentModelReset()));
}

QVariant KateArgumentHintModel::data ( const QModelIndex & index, int role ) const {
  if( index.row() <  0 || index.row() >= m_rows.count() ) {
    //kDebug() << "KateArgumentHintModel::data: index out of bound: " << index.row() << " total rows: " << m_rows.count() << endl;
    return QVariant();
  }

  if( m_rows[index.row()] < 0 ) {
    //Show labels
    if( role == Qt::DisplayRole && index.column() == 0 ) {
      return QString("Depth %1").arg(-m_rows[index.row()]);
    } else if( role == Qt::BackgroundRole ) {
      return QColor(Qt::black);
    }else if( role == Qt::ForegroundRole ) {
      return QColor(Qt::white);
    }else{
      return QVariant();
    }
  }

  if( m_rows[index.row()] <  0 || m_rows[index.row()] >= group()->rows.count() ) {
    kDebug() << "KateArgumentHintModel::data: index out of bound: " << m_rows[index.row()] << " total rows: " << group()->rows.count() << endl;
    return QVariant();
  }
  
  KateCompletionModel::ModelRow source = group()->rows[m_rows[index.row()]];
  if( !source.first ) {
    kDebug() << "KateArgumentHintModel::data: Row does not exist in source" << endl;
    return QVariant();
  }

  if( index.column() == 0 ) {
    switch( role ) {
      case Qt::DecorationRole:
      {
        //Show the expand-handle
        model()->cacheIcons();

        if( !isExpanded(index.row() ) )
          return QVariant( model()->m_collapsedIcon );
        else
          return QVariant( model()->m_expandedIcon );
      }
      case Qt::DisplayRole:
        //Ignore text in the first column(we create our own compound text in the second)
        return QVariant();
    }
  }

   QModelIndex  sourceIndex = source.first->index(source.second, index.column());
 
  if( !sourceIndex.isValid() ) {
    kDebug() << "KateArgumentHintModel::data: Source-index is not valid" << endl;
    return QVariant();
  }

  switch( role ) {
    case Qt::DisplayRole:
    {
      //Construct the text
      QString totalText;
      for( int a = CodeCompletionModel::Prefix; a <= CodeCompletionModel::Postfix; a++ )
        if( a != CodeCompletionModel::Scope ) //Skip the scope
          totalText += source.first->index(source.second, a).data(Qt::DisplayRole).toString() + " ";
    
      
      return QVariant(totalText);
    }
    case CodeCompletionModel::HighlightingMethod:
    {
      //Return that we are doing custom-highlighting of one of the sub-strings does it
      for( int a = CodeCompletionModel::Prefix; a <= CodeCompletionModel::Postfix; a++ )
          if( source.first->index(source.second, a).data(Qt::DisplayRole).type() == QVariant::Int )
            return QVariant(1);
    
      return QVariant();
    }
    case CodeCompletionModel::CustomHighlight:
    {
      QStringList strings;
      
      //Collect strings
      for( int a = CodeCompletionModel::Prefix; a <= CodeCompletionModel::Postfix; a++ )
          strings << source.first->index(source.second, a).data(Qt::DisplayRole).toString();

      QList<QVariantList> highlights;

      //Collect custom-highlightings
      for( int a = CodeCompletionModel::Prefix; a <= CodeCompletionModel::Postfix; a++ )
          highlights << source.first->index(source.second, a).data(CodeCompletionModel::CustomHighlight).toList();

      return mergeCustomHighlighting( strings, highlights );
    }
    case Qt::DecorationRole:
    {
      //Redirect the decoration to the decoration of the item-column
      return source.first->index(source.second, CodeCompletionModel::Icon).data(role);
    }
  }
  
  QVariant v = ExpandingWidgetModel::data( index, role );
  if( v.isValid() )
    return v;
  else
    return sourceIndex.data( role );
}

int KateArgumentHintModel::rowCount ( const QModelIndex & parent ) const {
  if( !parent.isValid() )
    return m_rows.count();
  else
    return 0;
}

int KateArgumentHintModel::columnCount ( const QModelIndex & /*parent*/ ) const {
  return 2; //2 Columns, one for the expand-handle, one for the signature
}

KateCompletionModel::Group* KateArgumentHintModel::group() const {
  return model()->m_argumentHints;
}

KateCompletionModel* KateArgumentHintModel::model() const {
  return m_parent->model();
}

QTreeView* KateArgumentHintModel::treeView() const {
  return m_parent->argumentHintTree();
}

void KateArgumentHintModel::emitDataChanged( const QModelIndex& start, const QModelIndex& end ) {
  emit dataChanged(start, end);
}

bool KateArgumentHintModel::indexIsCompletion(const QModelIndex& index) const {
  return index.row() >= 0 && index.row() < m_rows.count() && m_rows[index.row()] >= 0;
}

int KateArgumentHintModel::contextMatchQuality(int row) const {
  if( row <  0 || row >= m_rows.count() )
    return -1;

  if( m_rows[row] <  0 || m_rows[row] >= group()->rows.count() )
    return -1; //Probably a label
  
  KateCompletionModel::ModelRow source = group()->rows[m_rows[row]];
  if( !source.first )
    return -1;
  
   QModelIndex  sourceIndex = source.first->index(source.second, 0);
 
  if( !sourceIndex.isValid() )
    return -1;

  int depth = sourceIndex.data(CodeCompletionModel::ArgumentHintDepth).toInt();

  switch(depth) {
    case 1:
    {
      //This argument-hint is on the lowest level, match it with the currently selected item in the completion-widget
      int row = m_parent->model()->partiallyExpandedRow();
      if( row == -1 )
        return -1;

      QModelIndex selectedIndex = m_parent->model()->mapToSource( m_parent->model()->index(row,0) );
      if( !selectedIndex.isValid() )
        return -1;

      if( selectedIndex.model() != sourceIndex.model() )
        return -1; //We can only match items from the same source-model

      sourceIndex.data( CodeCompletionModel::SetMatchContext );

      QVariant v = selectedIndex.data( CodeCompletionModel::MatchQuality );
      if( v.type() == QVariant::Int ) 
        return v.toInt();
    }
    break;
    default:
      //Do some other nice matching here in future
      break;
  }

  return -1;
}

#include "kateargumenthintmodel.moc"
