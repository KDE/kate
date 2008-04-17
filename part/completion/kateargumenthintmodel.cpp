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

#include "kateargumenthintmodel.h"

#include <QTextFormat>
#include <QGridLayout>
#include <kapplication.h>

#include <ktexteditor/codecompletionmodel.h>
#include "katecompletionwidget.h"
#include "kateargumenthinttree.h"
#include "katecompletiontree.h"

using namespace KTextEditor;

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
    QModelIndex  sourceIndex = source.second.sibling(source.second.row(), 0);
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
  
  reset();
  emit contentStateChanged(!m_rows.isEmpty());
}

KateArgumentHintModel::KateArgumentHintModel( KateCompletionWidget* parent ) : ExpandingWidgetModel(parent), m_parent(parent) {
  connect(parent->model(), SIGNAL(modelReset()), this, SLOT(parentModelReset()));
  connect(parent->model(), SIGNAL(argumentHintsChanged()), this, SLOT(parentModelReset()));
}

QVariant KateArgumentHintModel::data ( const QModelIndex & index, int role ) const {
  if( index.row() <  0 || index.row() >= m_rows.count() ) {
    //kDebug() << "KateArgumentHintModel::data: index out of bound: " << index.row() << " total rows: " << m_rows.count();
    return QVariant();
  }

  if( m_rows[index.row()] < 0 ) {
    //Show labels
    if( role == Qt::DisplayRole && index.column() == 0 ) {
      return QString("Depth %1").arg(-m_rows[index.row()]);
    } else if( role == Qt::BackgroundRole ) {
      return KApplication::kApplication()->palette().toolTipBase().color();
    }else if( role == Qt::ForegroundRole ) {
      return KApplication::kApplication()->palette().toolTipText().color();
    }else{
      return QVariant();
    }
  }

  if( m_rows[index.row()] <  0 || m_rows[index.row()] >= group()->rows.count() ) {
    kDebug() << "KateArgumentHintModel::data: index out of bound: " << m_rows[index.row()] << " total rows: " << group()->rows.count();
    return QVariant();
  }
  
  KateCompletionModel::ModelRow source = group()->rows[m_rows[index.row()]];
  if( !source.first ) {
    kDebug() << "KateArgumentHintModel::data: Row does not exist in source";
    return QVariant();
  }

  if( index.column() == 0 ) {
    switch( role ) {
      case Qt::DecorationRole:
      {
        //Show the expand-handle
        model()->cacheIcons();

        if( !isExpanded(index ) )
          return QVariant( model()->m_collapsedIcon );
        else
          return QVariant( model()->m_expandedIcon );
      }
      case Qt::DisplayRole:
        //Ignore text in the first column(we create our own compound text in the second)
        return QVariant();
    }
  }

  QModelIndex  sourceIndex = source.second.sibling(source.second.row(), index.column());
 
  if( !sourceIndex.isValid() ) {
    kDebug() << "KateArgumentHintModel::data: Source-index is not valid";
    return QVariant();
  }

  switch( role ) {
    case Qt::DisplayRole:
    {
      //Construct the text
      QString totalText;
      for( int a = CodeCompletionModel::Prefix; a <= CodeCompletionModel::Postfix; a++ )
        if( a != CodeCompletionModel::Scope ) //Skip the scope
          totalText += source.second.sibling(source.second.row(), a).data(Qt::DisplayRole).toString() + ' ';
    
      
      return QVariant(totalText);
    }
    case CodeCompletionModel::HighlightingMethod:
    {
      //Return that we are doing custom-highlighting of one of the sub-strings does it
      for( int a = CodeCompletionModel::Prefix; a <= CodeCompletionModel::Postfix; a++ )
          if( source.second.sibling(source.second.row(), a).data(CodeCompletionModel::HighlightingMethod).type() == QVariant::Int ) {
            return QVariant(CodeCompletionModel::CustomHighlighting);
          }
    
      return QVariant();
    }
    case CodeCompletionModel::CustomHighlight:
    {
      QStringList strings;
      
      //Collect strings
      for( int a = CodeCompletionModel::Prefix; a <= CodeCompletionModel::Postfix; a++ )
          strings << source.second.sibling(source.second.row(), a).data(Qt::DisplayRole).toString();

      QList<QVariantList> highlights;

      //Collect custom-highlightings
      for( int a = CodeCompletionModel::Prefix; a <= CodeCompletionModel::Postfix; a++ )
          highlights << source.second.sibling(source.second.row(), a).data(CodeCompletionModel::CustomHighlight).toList();

      //Replace invalid QTextFormats with match-quality color or yellow.
      for( QList<QVariantList>::iterator it = highlights.begin(); it != highlights.end(); ++it )
      {
        QVariantList& list( *it );
        
        for( int a = 2; a < list.count(); a += 3 )
        {
          if( list[a].canConvert<QTextFormat>() )
          {
            QTextFormat f = list[a].value<QTextFormat>();
            
            if(!f.isValid())
            {
              f = QTextFormat( QTextFormat::CharFormat );
              uint color = matchColor( index );

              if( color )
                f.setBackground( QBrush(color) );
              else
                f.setBackground( Qt::yellow );
              
              list[a] = QVariant( f );
            }
          }
        }
      }
      
      
      return mergeCustomHighlighting( strings, highlights );
    }
    case Qt::DecorationRole:
    {
      //Redirect the decoration to the decoration of the item-column
      return source.second.sibling(source.second.row(), CodeCompletionModel::Icon).data(role);
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

bool KateArgumentHintModel::indexIsItem(const QModelIndex& index) const {
  return index.row() >= 0 && index.row() < m_rows.count() && m_rows[index.row()] >= 0;
}

int KateArgumentHintModel::contextMatchQuality(const QModelIndex& index) const {
  int row=index.row();
  if( row <  0 || row >= m_rows.count() )
    return -1;

  if( m_rows[row] <  0 || m_rows[row] >= group()->rows.count() )
    return -1; //Probably a label
  
  KateCompletionModel::ModelRow source = group()->rows[m_rows[row]];
  if( !source.first )
    return -1;
  
   QModelIndex  sourceIndex = source.second.sibling(source.second.row(), 0);
 
  if( !sourceIndex.isValid() )
    return -1;

  int depth = sourceIndex.data(CodeCompletionModel::ArgumentHintDepth).toInt();

  switch(depth) {
    case 1:
    {
      //This argument-hint is on the lowest level, match it with the currently selected item in the completion-widget
      QModelIndex row = m_parent->treeView()->currentIndex();
      if( !row.isValid() )
        return -1;

      QModelIndex selectedIndex = m_parent->model()->mapToSource( row );
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
