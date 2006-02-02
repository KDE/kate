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

#include <QHeaderView>
#include <QScrollBar>

#include <ktexteditor/codecompletion2.h>

#include "kateview.h"
#include "katerenderer.h"
#include "kateconfig.h"
#include "katefont.h"

#include "katecompletionwidget.h"
#include "katecompletiondelegate.h"
#include "katecompletionmodel.h"

KateCompletionTree::KateCompletionTree(KateCompletionWidget* parent)
  : QTreeView(parent)
{
  setUniformRowHeights(true);
  header()->hide();
  setRootIsDecorated(false);
  setIndentation(0);
  setFrameStyle(QFrame::NoFrame);

  // Provide custom highlighting to completion entries
  setItemDelegate(new KateCompletionDelegate(widget()));

  // Prevent user from expanding / collapsing with the mouse
  setItemsExpandable(false);
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

void KateCompletionTree::expandAll( const QModelIndex & index )
{
  if (model()->hasChildren(index)) {
    expand(index);

    int rowCount = model()->rowCount(index);
    for (int i = 0; i < rowCount; ++i)
      expandAll(model()->index(i, 0, index));
  }
}

void KateCompletionTree::resizeColumns(bool fromResizeEvent)
{
  setUpdatesEnabled(false);

  int visualIndexOfName = header()->visualIndex(KTextEditor::CodeCompletionModel::Name);
  int oldIndentWidth = header()->sectionPosition(visualIndexOfName);

  for (int i = 0; i < KTextEditor::CodeCompletionModel::ColumnCount; ++i)
    resizeColumnToContents(i);

  int newIndentWidth = header()->sectionPosition(visualIndexOfName);

  int minWidth = 50;
  int newMinWidth = newIndentWidth + header()->sectionSize(visualIndexOfName) + verticalScrollBar()->width();
  minWidth = QMAX(minWidth, newMinWidth);

  if (!fromResizeEvent && oldIndentWidth != newIndentWidth) {
    int newWidth = widget()->width() - oldIndentWidth + newIndentWidth;
    //kDebug() << k_funcinfo << fromResizeEvent << " oldI " << oldIndentWidth << " newI " << newIndentWidth << " minw " << minWidth << " w " << widget()->width() << " newW " << newWidth << endl;
    widget()->resize(newWidth, widget()->height());
  }

  setMinimumWidth(minWidth);

  if (oldIndentWidth != newIndentWidth)
    widget()->updatePosition();

  setUpdatesEnabled(true);
}

QStyleOptionViewItem KateCompletionTree::viewOptions( ) const
{
  QStyleOptionViewItem opt = QTreeView::viewOptions();

  opt.font = widget()->view()->renderer()->config()->fontStruct()->font(false, false);

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

void KateCompletionTree::pageDown( )
{
  QModelIndex current = moveCursor(MovePageDown, Qt::NoModifier);

  if (current.isValid()) {
    setCurrentIndex(current);
    if (!kateModel()->indexIsCompletion(current))
      if (!nextCompletion())
        previousCompletion();
  }
}

void KateCompletionTree::pageUp( )
{
  QModelIndex current = moveCursor(MovePageUp, Qt::NoModifier);

  if (current.isValid()) {
    setCurrentIndex(current);
    if (!kateModel()->indexIsCompletion(current))
      if (!previousCompletion())
        nextCompletion();
  }
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
