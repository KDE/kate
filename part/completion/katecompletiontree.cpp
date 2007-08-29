/* This file is part of the KDE libraries
   Copyright (C) 2006 Hamish Rodda <rodda@kde.org>

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

#include "katecompletiontree.h"

#include <QtGui/QHeaderView>
#include <QtGui/QScrollBar>
#include <QVector>
#include <QTimer>
#include <QApplication>
#include <QDesktopWidget>

#include "kateview.h"
#include "katerenderer.h"
#include "kateconfig.h"

#include "katecompletionwidget.h"
#include "katecompletiondelegate.h"
#include "katecompletionmodel.h"

KateCompletionTree::KateCompletionTree(KateCompletionWidget* parent)
  : KateExpandingTree(parent), m_needResize(false)
{
  setUniformRowHeights(false);
  header()->hide();
  setRootIsDecorated(false);
  setIndentation(0);
  setFrameStyle(QFrame::NoFrame);
  setAllColumnsShowFocus(true);
  setAlternatingRowColors(true);

  m_resizeTimer = new QTimer(this);
  m_resizeTimer->setSingleShot(true);

  connect(m_resizeTimer, SIGNAL(timeout()), this, SLOT(resizeColumnsSlot()));
  
  // Provide custom highlighting to completion entries
  setItemDelegate(new KateCompletionDelegate(widget()->model(), widget()));

  // Prevent user from expanding / collapsing with the mouse
  setItemsExpandable(false);
}

void KateCompletionTree::currentChanged ( const QModelIndex & current, const QModelIndex & previous ) {
  widget()->model()->rowSelected(current);
  QTreeView::currentChanged(current, previous);
}

void KateCompletionTree::scrollContentsBy( int dx, int dy )
{
  QTreeView::scrollContentsBy(dx, dy);

  if (isVisible())
    m_resizeTimer->start(300);
}

KateCompletionWidget * KateCompletionTree::widget( ) const
{
  return static_cast<KateCompletionWidget*>(const_cast<QObject*>(parent()));
}

void KateCompletionTree::resizeColumnsSlot()
{
  resizeColumns();
}

void KateCompletionTree::resizeColumns(bool fromResizeEvent, bool firstShow)
{
  static bool firstCall = false;
  if (firstCall)
    return;

  if( firstShow ) { ///@todo This might make some flickering, but is needed because visualRect(..) for group child-indices returns invalid rects before the widget is shown
    m_resizeTimer->start(100);
    m_needResize = true;
  } else if( m_needResize ) {
    m_needResize = false;
    firstShow = true;
  }

  firstCall = true;

  widget()->setUpdatesEnabled(false);

  int modelIndexOfName = kateModel()->translateColumn(KTextEditor::CodeCompletionModel::Name);
  int oldIndentWidth = columnViewportPosition(modelIndexOfName);

  ///Step 1: Compute the needed column-sizes for the visible content
  QRect visibleViewportRect(0, 0, width(), height());

  int numColumns = model()->columnCount();
  
  QVector<int> columnSize(numColumns, 5);
  columnSize[0] = 50;

  QModelIndex current = indexAt(QPoint(1,1));

  if( current.child(0,0).isValid() ) //If the index has children, it is a group-label. Then we should start with it's first child.
    current = current.child(0,0);

  int num = 0;
  bool changed = false;
  while( current.isValid() && visualRect(current).isValid() && visualRect(current).intersects(visibleViewportRect) )
  {
    changed = true;
    num++;
    for( int a = 0; a < numColumns; a++ )
    {
      QSize s = sizeHintForIndex (current.sibling(current.row(), a));
      if( s.width() > columnSize[a] && s.width() < 700 )
        columnSize[a] = s.width();
      else if( s.width() > 700 )
        kDebug() << "got invalid size-hint of width " << s.width();
    }

    QModelIndex oldCurrent = current;
    current = current.sibling(current.row()+1, 0);
    
    //Are we at the end of a group? If yes, move on into the next group
    if( !current.isValid() && oldCurrent.parent().isValid() ) {
      current = oldCurrent.parent().sibling( oldCurrent.parent().row()+1, 0 );
      if( current.isValid() && current.child(0,0).isValid() )
        current = current.child(0,0);
    }
  }

  int totalColumnsWidth = 0;

  ///Step 2: Update column-sizes
  if( changed ) {

    int minimumResize = 0;
    int maximumResize = 0;
    for( int n = 0; n < numColumns; n++ ) {
      totalColumnsWidth += columnSize[n];
      
      int diff = columnSize[n] - columnWidth(n);
      if( diff < minimumResize )
        minimumResize = diff;
      if( diff > maximumResize )
        maximumResize = diff;
    }

    if( minimumResize > -40 && maximumResize == 0 ) {
      //No column needs to be exanded, and no column needs to be reduced by more then 40 pixels.
      //To prevent flashing, do not resize at all.
      totalColumnsWidth = 0;
      for( int n = 0; n < numColumns; n++ ) {
        columnSize[n] = columnWidth(n);
        totalColumnsWidth += columnSize[n];
      }
    } else {
      //It may happen that while initial showing, no visual rectangles can be retrieved.
      for( int n = 0; n < numColumns; n++ )
        setColumnWidth(n, columnSize[n]);
    }
  }

  ///Step 3: Update widget-size and -position
  
  int newIndentWidth = columnViewportPosition(modelIndexOfName);

  int scrollBarWidth = verticalScrollBar()->width();
  int newMinWidth = totalColumnsWidth /*+ scrollBarWidth*/;

  int minWidth = qMax(500, newMinWidth);

  //kDebug() << "New min width: " << minWidth << " Old min: " << minimumWidth() << " width " << width();
  setMinimumWidth(minWidth);

  if (!fromResizeEvent && (firstShow || oldIndentWidth != newIndentWidth))
  {
    //Never allow a completion-widget to be wider than 2/3 of the screen
    int maxWidth = (QApplication::desktop()->screenGeometry(widget()->view()).width()*3) / 4;
    int newWidth = qMin(maxWidth, minWidth); 
    //kDebug() << "fromResize " << fromResizeEvent << " indexOfName " << modelIndexOfName << " oldI " << oldIndentWidth << " newI " << newIndentWidth << " minw " << minWidth << " w " << widget()->width() << " newW " << newWidth;
    widget()->resize(newWidth + scrollBarWidth, widget()->height());
  }

  if( totalColumnsWidth ) //Set the size of the last column to fill the whole rest of the widget
    setColumnWidth(numColumns-1, width() - totalColumnsWidth + columnSize[numColumns-1]);
  
  if (oldIndentWidth != newIndentWidth)
    widget()->updatePosition();

  widget()->setUpdatesEnabled(true);

  firstCall = false;
}

QStyleOptionViewItem KateCompletionTree::viewOptions( ) const
{
  QStyleOptionViewItem opt = QTreeView::viewOptions();

  opt.font = widget()->view()->renderer()->config()->font();

  return opt;
}

KateCompletionModel * KateCompletionTree::kateModel( ) const
{
  return static_cast<KateCompletionModel*>(model());
}

bool KateCompletionTree::nextCompletion()
{
  QModelIndex current;
  QModelIndex firstCurrent = currentIndex();

  do {
    QModelIndex oldCurrent = currentIndex();

    current = moveCursor(MoveDown, Qt::NoModifier);

    if (current != oldCurrent && current.isValid()) {
      setCurrentIndex(current);

    } else {
      if (firstCurrent.isValid())
        setCurrentIndex(firstCurrent);
      return false;
    }

  } while (!kateModel()->indexIsCompletion(current));

  return true;
}

bool KateCompletionTree::previousCompletion()
{
  QModelIndex current;
  QModelIndex firstCurrent = currentIndex();

  do {
    QModelIndex oldCurrent = currentIndex();

    current = moveCursor(MoveUp, Qt::NoModifier);

    if (current != oldCurrent && current.isValid()) {
      setCurrentIndex(current);

    } else {
      if (firstCurrent.isValid())
        setCurrentIndex(firstCurrent);
      return false;
    }

  } while (!kateModel()->indexIsCompletion(current));

  return true;
}

bool KateCompletionTree::pageDown( )
{
  QModelIndex old = currentIndex();
  QModelIndex current = moveCursor(MovePageDown, Qt::NoModifier);

  if (current.isValid()) {
    setCurrentIndex(current);
    if (!kateModel()->indexIsCompletion(current))
      if (!nextCompletion())
        previousCompletion();
  }

  return current != old;
}

bool KateCompletionTree::pageUp( )
{
  QModelIndex old = currentIndex();
  QModelIndex current = moveCursor(MovePageUp, Qt::NoModifier);

  if (current.isValid()) {
    setCurrentIndex(current);
    if (!kateModel()->indexIsCompletion(current))
      if (!previousCompletion())
        nextCompletion();
  }
  return current != old;
}

void KateCompletionTree::top( )
{
  QModelIndex current = moveCursor(MoveHome, Qt::NoModifier);
  setCurrentIndex(current);

  if (current.isValid()) {
    setCurrentIndex(current);
    if (!kateModel()->indexIsCompletion(current))
      nextCompletion();
  }
}

void KateCompletionTree::bottom( )
{
  QModelIndex current = moveCursor(MoveEnd, Qt::NoModifier);
  setCurrentIndex(current);

  if (current.isValid()) {
    setCurrentIndex(current);
    if (!kateModel()->indexIsCompletion(current))
      previousCompletion();
  }
}

#include "katecompletiontree.moc"
