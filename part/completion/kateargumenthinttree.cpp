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

#include <QHeaderView>

#include "kateargumenthinttree.h"
#include "kateargumenthintmodel.h"
#include "katecompletionwidget.h"
#include "expandingwidgetmodel.h"
#include "katecompletiondelegate.h"
#include <QModelIndex>


KateArgumentHintTree::KateArgumentHintTree( KateCompletionWidget* parent ) : KateExpandingTree(0), m_parent(parent) { //Do not use the completion-widget as widget-parent, because the argument-hint-tree will be rendered separately

  setFrameStyle( QFrame::Box | QFrame::Plain );
  setLineWidth( 1 );
  
  connect( parent, SIGNAL(destroyed(QObject*)), this, SLOT(deleteLater()) );
  setFrameStyle(QFrame::NoFrame);
  setFrameStyle( QFrame::Box | QFrame::Plain );
  setFocusPolicy(Qt::NoFocus);
  setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
  setUniformRowHeights(false);
  header()->hide();
  setRootIsDecorated(false);
  setIndentation(0);
  setAllColumnsShowFocus(true);
  setAlternatingRowColors(true);
  setItemDelegate(new KateCompletionDelegate(parent->argumentHintModel(), parent));
}

void KateArgumentHintTree::clearCompletion() {
  setCurrentIndex(QModelIndex());
}

KateArgumentHintModel* KateArgumentHintTree::model() const {
  return m_parent->argumentHintModel();
}

void KateArgumentHintTree::paintEvent ( QPaintEvent * event ) {
  QTreeView::paintEvent(event);
  updateGeometry(); ///@todo delay this. It is needed here, because visualRect(...) returns an invalid rect in updateGeometry before the content is painted
}

void KateArgumentHintTree::dataChanged ( const QModelIndex & topLeft, const QModelIndex & bottomRight ) {
  QTreeView::dataChanged(topLeft,bottomRight);
  //updateGeometry();
}

void KateArgumentHintTree::currentChanged ( const QModelIndex & current, const QModelIndex & previous ) {
/*  kDebug() << "currentChanged()" << endl;*/
  static_cast<ExpandingWidgetModel*>(model())->rowSelected(current);
  QTreeView::currentChanged(current, previous);
}

void KateArgumentHintTree::rowsInserted ( const QModelIndex & parent, int start, int end ) {
  QTreeView::rowsInserted(parent, start, end);
  updateGeometry();
}

void KateArgumentHintTree::updateGeometry(QRect geom) {
  //Avoid recursive calls of updateGeometry
  static bool updatingGeometry = false;
  if( updatingGeometry ) return;
  updatingGeometry = true;
  
  if( model()->rowCount(QModelIndex()) == 0 ) {
/*  kDebug() << "KateArgumentHintTree:: empty model" << endl;*/
    hide();
    setGeometry(geom);
    updatingGeometry = false;
    return;
  }

  show();
  resizeColumnToContents(0);
  int bottom = geom.bottom();
  QRect contentRect = visualRect(model()->index(model()->rowCount(QModelIndex())-1, 0));
  
  geom.setHeight(contentRect.bottom() + 5);
  geom.moveBottom(bottom);
/*  kDebug() << "KateArgumentHintTree::updateGeometry: updating geometry to " << geom << endl;*/
  setGeometry(geom);
  show();
  updatingGeometry = false;
}

void KateArgumentHintTree::updateGeometry() {
  updateGeometry( geometry() );
}

bool KateArgumentHintTree::nextCompletion()
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

  } while (!model()->indexIsCompletion(current));

  return true;
}

bool KateArgumentHintTree::previousCompletion()
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

  } while (!model()->indexIsCompletion(current));

  return true;
}

bool KateArgumentHintTree::pageDown( )
{
  QModelIndex old = currentIndex();
  QModelIndex current = moveCursor(MovePageDown, Qt::NoModifier);

  if (current.isValid()) {
    setCurrentIndex(current);
    if (!model()->indexIsCompletion(current))
      if (!nextCompletion())
        previousCompletion();
  }

  return current != old;
}

bool KateArgumentHintTree::pageUp( )
{
  QModelIndex old = currentIndex();
  QModelIndex current = moveCursor(MovePageUp, Qt::NoModifier);

  if (current.isValid()) {
    setCurrentIndex(current);
    if (!model()->indexIsCompletion(current))
      if (!previousCompletion())
        nextCompletion();
  }
  return current != old;
}

void KateArgumentHintTree::top( )
{
  QModelIndex current = moveCursor(MoveHome, Qt::NoModifier);
  setCurrentIndex(current);

  if (current.isValid()) {
    setCurrentIndex(current);
    if (!model()->indexIsCompletion(current))
      nextCompletion();
  }
}

void KateArgumentHintTree::bottom( )
{
  QModelIndex current = moveCursor(MoveEnd, Qt::NoModifier);
  setCurrentIndex(current);

  if (current.isValid()) {
    setCurrentIndex(current);
    if (!model()->indexIsCompletion(current))
      previousCompletion();
  }
}

#include "kateargumenthinttree.moc"
