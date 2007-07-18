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

#include "kateview.h"
#include "katerenderer.h"
#include "kateconfig.h"

#include "katecompletionwidget.h"
#include "katecompletiondelegate.h"
#include "katecompletionmodel.h"

KateCompletionTree::KateCompletionTree(KateCompletionWidget* parent)
  : KateExpandingTree(parent)
{
  setUniformRowHeights(false);
  header()->hide();
  setRootIsDecorated(false);
  setIndentation(0);
  setFrameStyle(QFrame::NoFrame);
  setAllColumnsShowFocus(true);
  setAlternatingRowColors(true);

  // Provide custom highlighting to completion entries
  setItemDelegate(new KateCompletionDelegate(widget()->model(), widget()));

  // Prevent user from expanding / collapsing with the mouse
  setItemsExpandable(false);
}

void KateCompletionTree::currentChanged ( const QModelIndex & current, const QModelIndex & previous ) {
  widget()->model()->rowSelected(current.row());
  QTreeView::currentChanged(current, previous);
}

void KateCompletionTree::scrollContentsBy( int dx, int dy )
{
  QTreeView::scrollContentsBy(dx, dy);

  if (isVisible())
    resizeColumns();
}

KateCompletionWidget * KateCompletionTree::widget( ) const
{
  return static_cast<KateCompletionWidget*>(const_cast<QObject*>(parent()));
}

void KateCompletionTree::resizeColumns(bool fromResizeEvent, bool firstShow)
{
  static bool firstCall = false;
  if (firstCall)
    return;

  firstCall = true;

  setUpdatesEnabled(false);

  int indexOfName = header()->visualIndex(kateModel()->translateColumn(KTextEditor::CodeCompletionModel::Name));
  int oldIndentWidth = header()->sectionPosition(indexOfName);

  int numColumns = model()->columnCount();
  for (int i = 0; i < numColumns; ++i)
    resizeColumnToContents(i);

  int newIndentWidth = header()->sectionPosition(indexOfName);

  int minWidth = 50;
  int sectionSize = header()->sectionSize(indexOfName);
  //int scrollBarWidth = verticalScrollBar()->width();
  int newMinWidth = newIndentWidth + sectionSize;// + scrollBarWidth;
  minWidth = qMax(minWidth, newMinWidth);

  //kDebug() << "New min width: " << minWidth << " Old min: " << minimumWidth() << " width " << width() << endl;
  setMinimumWidth(minWidth);

  if (!fromResizeEvent && (firstShow || oldIndentWidth != newIndentWidth)) {
    int newWidth = qMax(widget()->width() - oldIndentWidth + newIndentWidth, minWidth);
    //kDebug() << k_funcinfo << "fromResize " << fromResizeEvent << " indexOfName " << indexOfName << " oldI " << oldIndentWidth << " newI " << newIndentWidth << " minw " << minWidth << " w " << widget()->width() << " newW " << newWidth << endl;
    widget()->resize(newWidth, widget()->height());
  }

  if (oldIndentWidth != newIndentWidth)
    widget()->updatePosition();

  setUpdatesEnabled(true);

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
