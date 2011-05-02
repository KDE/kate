/* This file is part of the KDE project
   Copyright (C) 2010 Thomas Fjellstrom <thomas@fjellstrom.ca>

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

#include "katefiletreeproxymodel.h"
#include "katefiletreemodel.h"
#include "katefiletreedebug.h"

#include <kstringhandler.h>
#include <ktexteditor/document.h>

KateFileTreeProxyModel::KateFileTreeProxyModel(QObject *parent)
  : QSortFilterProxyModel(parent)
{
  kDebug(debugArea()) << "BEGIN!";
}

bool KateFileTreeProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
  //kDebug(debugArea()) << ": BEGIN!";
  KateFileTreeModel *model = static_cast<KateFileTreeModel*>(sourceModel());
  
  bool left_isdir = model->isDir(left);
  bool right_isdir = model->isDir(right);

  // in tree mode, there will be parent nodes, we want to put those first
  if(left_isdir != right_isdir) {
      return ((left_isdir - right_isdir)) > 0;
  }
  
  switch(sortRole()) {
    case Qt::DisplayRole: {
      QString left_name = model->data(left).toString();
      QString right_name = model->data(right).toString();
      return KStringHandler::naturalCompare(left_name, right_name, Qt::CaseInsensitive) < 0;
    }

    case KateFileTreeModel::PathRole: {
      QString left_name = model->data(left, KateFileTreeModel::PathRole).toString();
      QString right_name = model->data(right, KateFileTreeModel::PathRole).toString();
      return KStringHandler::naturalCompare(left_name, right_name, Qt::CaseInsensitive) < 0;
    }

    case KateFileTreeModel::OpeningOrderRole:
      return (left.row() - right.row()) < 0;
  }

  kDebug(debugArea()) << "this shouldn't happen!";
  return false;
}

QModelIndex KateFileTreeProxyModel::docIndex(KTextEditor::Document *doc)
{
  kDebug(debugArea()) << "!";
  return mapFromSource(static_cast<KateFileTreeModel*>(sourceModel())->docIndex(doc));
}

bool KateFileTreeProxyModel::isDir(const QModelIndex &index)
{
  return static_cast<KateFileTreeModel*>(sourceModel())->isDir(mapToSource(index));
}

// kate: space-indent on; indent-width 2; replace-tabs on;
