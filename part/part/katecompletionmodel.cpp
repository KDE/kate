/* This file is part of the KDE libraries
   Copyright (C) 2005-2006 Hamish Rodda <rodda@kde.org>

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

#include "katecompletionmodel.h"

#include <klocale.h>

#include "katecompletionwidget.h"
#include "kateview.h"
#include "katerenderer.h"
#include "kateconfig.h"
#include "katefont.h"

using namespace KTextEditor;

KateCompletionModel::KateCompletionModel(KateCompletionWidget* parent)
  : QAbstractProxyModel(parent)
  , m_caseSensitive(Qt::CaseInsensitive)
  , m_hasCompletionModel(false)
  , m_columnCount(KTextEditor::CodeCompletionModel::ColumnCount)
  , m_ungrouped(new Group())
  , m_ungroupedDisplayed(false)
{
  m_ungrouped->attribute = 0;
  m_ungrouped->title = i18n("Other");
}

QVariant KateCompletionModel::data( const QModelIndex & index, int role ) const
{
  if (!hasCompletionModel())
    return QVariant();

  if (!hasGroups() || groupOfParent(index)) {
    switch (role) {
      case Qt::TextAlignmentRole:
        if (index.column() == CodeCompletionModel::Scope)
          return Qt::AlignRight;
        break;
    }

    return sourceModel()->data(mapToSource(index), role);
  }

  Group* g = groupForIndex(index);

  if (g) {
    switch (role) {
      case Qt::DisplayRole:
        if (!index.column())
          return g->title;
        break;

      case Qt::FontRole:
        if (!index.column())
          return view()->renderer()->config()->fontStruct()->font(true, false);
        break;

      case Qt::TextColorRole:
        return QColor(Qt::white);

      case Qt::BackgroundColorRole:
        return QColor(Qt::black);
    }
  }

  return QVariant();
}


Qt::ItemFlags KateCompletionModel::flags( const QModelIndex & index ) const
{
  if (!hasCompletionModel())
    return 0;

  if (!hasGroups() || groupOfParent(index))
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;

  return Qt::ItemIsEnabled;
}

CodeCompletionModel * KateCompletionModel::completionModel( ) const
{
  return static_cast<CodeCompletionModel*>(sourceModel());
}

KateView * KateCompletionModel::view( ) const
{
  return static_cast<const KateCompletionWidget*>(QObject::parent())->view();
}

void KateCompletionModel::clearGroups( )
{
  if (m_ungroupedDisplayed) {
    m_rowTable.removeAll(m_ungrouped);
    m_ungroupedDisplayed = false;
  }

  qDeleteAll(m_rowTable);
  m_rowTable.clear();
}

void KateCompletionModel::setSourceModel( QAbstractItemModel* sm )
{
  if (sourceModel() == sm)
    return;

  clearGroups();

  if (hasCompletionModel())
    sourceModel()->disconnect(this);

  QAbstractProxyModel::setSourceModel(sm);

  m_hasCompletionModel = sm;

  //m_columnCount = 0;

  if (hasCompletionModel()) {
    connect(sourceModel(), SIGNAL(rowsInserted(const QModelIndex&, int, int)), SLOT(slotRowsInserted(const QModelIndex&, int, int)));
    connect(sourceModel(), SIGNAL(rowsRemoved(const QModelIndex&, int, int)), SLOT(slotRowsRemoved(const QModelIndex&, int, int)));

    //if (sourceModel()->rowCount())
      //m_columnCount = sourceModel()->columnCount(sourceModel()->index(0, 0, QModelIndex()));

    createGroups();
  }

  reset();
}

void KateCompletionModel::setCaseSensitivity( Qt::CaseSensitivity cs )
{
  m_caseSensitive = cs;
}

int KateCompletionModel::columnCount( const QModelIndex & parent ) const
{
  return m_columnCount;
}

bool KateCompletionModel::hasChildren( const QModelIndex & parent ) const
{
  if (!hasCompletionModel())
    return false;

  if (!parent.isValid()) {
    if (hasGroups())
      return true;

    return m_ungrouped->rows.count();
  }

  if (!hasGroups())
    return false;

  if (Group* g = groupForIndex(parent))
    return !g->rows.isEmpty();

  return false;
}

QModelIndex KateCompletionModel::index( int row, int column, const QModelIndex & parent ) const
{
  if (row < 0 || column < 0 || column >= m_columnCount)
    return QModelIndex();

  if (parent.isValid()) {
    Group* g = groupForIndex(parent);

    if (row >= g->rows.count()) {
      //kdWarning() << k_funcinfo << "Invalid index requested: row " << row << " beyond indivdual range in group " << g << endl;
      return QModelIndex();
    }

    //kdDebug() << k_funcinfo << "Returning index for child " << row << " of group " << g << endl;
    return createIndex(row, column, g);
  }

  if (row >= m_rowTable.count()) {
    //kdWarning() << k_funcinfo << "Invalid index requested: row " << row << " beyond group range." << endl;
    return QModelIndex();
  }

  //kdDebug() << k_funcinfo << "Returning index for group " << m_rowTable[row] << endl;
  return createIndex(row, column, 0);
}

QModelIndex KateCompletionModel::sibling( int row, int column, const QModelIndex & index ) const
{
  if (!index.isValid() || row < 0 || column < 0 || column >= m_columnCount)
    return QModelIndex();

  if (Group* g = groupOfParent(index)) {
    if (row >= g->rows.count())
      return QModelIndex();

    return createIndex(row, column, g);
  }

  if (hasGroups())
    return QModelIndex();

  if (row >= m_ungrouped->rows.count())
    return QModelIndex();

  return createIndex(row, column, 0);
}

bool KateCompletionModel::hasIndex( int row, int column, const QModelIndex & parent ) const
{
  if (row < 0 || column < 0 || column >= m_columnCount)
    return false;

  if (parent.isValid()) {
    Group* g = groupForIndex(parent);

    if (row >= g->rows.count())
      return false;

    return true;
  }

  if (row >= m_rowTable.count())
    return false;

  return true;
}

QModelIndex KateCompletionModel::indexForRow( Group * g, int row ) const
{
  if (row < 0 || row >= g->rows.count())
    return QModelIndex();

  return createIndex(row, 0, g);
}

QModelIndex KateCompletionModel::indexForGroup( Group * g ) const
{
  if (g == m_ungrouped)
    return QModelIndex();

  int row = m_rowTable.indexOf(g);

  if (row == -1)
    return QModelIndex();

  return createIndex(row, 0, 0);
}

void KateCompletionModel::createGroups( )
{
  clearGroups();

  for (int i = 0; i < sourceModel()->rowCount(); ++i) {
    QModelIndex sourceIndex = sourceModel()->index(i, CodeCompletionModel::Name, QModelIndex());
    int completionFlags = sourceModel()->data(sourceIndex, CodeCompletionModel::CompletionRole).toInt();

    Group* g = 0L;

    for (int j = CodeCompletionModel::Namespace; j <= CodeCompletionModel::Enum; j *= 2) {
      if (completionFlags & j) {
        g = fetchGroup(j);
        break;
      }
    }

    if (!g)
      g = ungrouped();

    if (sourceModel()->data(sourceIndex, Qt::DisplayRole).toString().startsWith(m_currentMatch, m_caseSensitive))
      g->rows.append(i);

    g->prefilter.append(i);
  }
}

KateCompletionModel::Group* KateCompletionModel::fetchGroup( int attribute )
{
  foreach (Group* g, m_rowTable)
    if (g->attribute == attribute)
      return g;

  Group* ret = new Group();

  ret->attribute = attribute;

  switch (attribute) {
    case CodeCompletionModel::Namespace:
      ret->title = i18n("Namespaces");
      break;

    case CodeCompletionModel::Class:
      ret->title = i18n("Classes");
      break;

    case CodeCompletionModel::Struct:
      ret->title = i18n("Structs");
      break;

    case CodeCompletionModel::Union:
      ret->title = i18n("Unions");
      break;

    case CodeCompletionModel::Function:
      ret->title = i18n("Functions");
      break;

    case CodeCompletionModel::Variable:
      ret->title = i18n("Variables");
      break;

    case CodeCompletionModel::Enum:
      ret->title = i18n("Enumerations");
      break;
  }

  m_rowTable.append(ret);

  return ret;
}

bool KateCompletionModel::hasGroups( ) const
{
  return m_rowTable.count();
}

KateCompletionModel::Group* KateCompletionModel::groupForIndex( const QModelIndex & index ) const
{
  if (!index.isValid())
    return 0L;

  if (!hasGroups())
    return m_ungrouped;

  if (groupOfParent(index))
    return 0L;

  if (index.row() < 0 || index.row() >= m_rowTable.count())
    return m_ungrouped;

  return m_rowTable[index.row()];
}

QMap< int, QVariant > KateCompletionModel::itemData( const QModelIndex & index ) const
{
  if (!hasGroups() || groupOfParent(index))
    return sourceModel()->itemData(mapToSource(index));

  return QAbstractProxyModel::itemData(index);
}

QModelIndex KateCompletionModel::parent( const QModelIndex & index ) const
{
  if (!index.isValid())
    return QModelIndex();

  if (Group* g = groupOfParent(index)) {
    int row = m_rowTable.indexOf(g);
    return createIndex(row, 0, 0);
  }

  return QModelIndex();
}

int KateCompletionModel::rowCount( const QModelIndex & parent ) const
{
  if (!parent.isValid())
    if (hasGroups()) {
      //kdDebug() << k_funcinfo << "Returning row count for toplevel " << m_rowTable.count() << endl;
      return m_rowTable.count();
    } else {
      //kdDebug() << k_funcinfo << "Returning ungrouped row count for toplevel " << m_ungrouped->rows.count() << endl;
      return m_ungrouped->rows.count();
    }

  Group* g = groupForIndex(parent);
  //kdDebug() << k_funcinfo << "Returning row count for group " << g << " as " << g->rows.count() << endl;
  return g->rows.count();
}

void KateCompletionModel::sort( int column, Qt::SortOrder order )
{
  Q_UNUSED(column)
  Q_UNUSED(order)
}

QModelIndex KateCompletionModel::mapToSource( const QModelIndex & proxyIndex ) const
{
  if (!proxyIndex.isValid())
    return QModelIndex();

  if (Group* g = groupOfParent(proxyIndex))
    return sourceModel()->index(g->rows[proxyIndex.row()], proxyIndex.column());

  return QModelIndex();
}

QModelIndex KateCompletionModel::mapFromSource( const QModelIndex & sourceIndex ) const
{
  if (!sourceIndex.isValid())
    return QModelIndex();

  if (!hasGroups())
    return index(m_ungrouped->rows.indexOf(sourceIndex.row()), sourceIndex.column(), QModelIndex());

  foreach (Group* g, m_rowTable) {
    int row = g->rows.indexOf(sourceIndex.row());
    if (row != -1)
      return index(row, sourceIndex.column(), QModelIndex());
  }

  return QModelIndex();
}

void KateCompletionModel::setCurrentCompletion( const QString & completion )
{
  if (m_currentMatch == completion)
    return;

  if (!hasCompletionModel()) {
    m_currentMatch = completion;
    return;
  }

  changeTypes changeType = Change;

  if (m_currentMatch.length() > completion.length() && m_currentMatch.startsWith(completion, m_caseSensitive)) {
    // Filter has been broadened
    changeType = Broaden;

  } else if (m_currentMatch.length() < completion.length() && completion.startsWith(m_currentMatch, m_caseSensitive)) {
    // Filter has been narrowed
    changeType = Narrow;
  }

  //kdDebug() << k_funcinfo << "Old match: " << m_currentMatch << ", new: " << completion << ", type: " << changeType << endl;

  if (!hasGroups())
    changeCompletions(m_ungrouped, completion, changeType);
  else
    foreach (Group* g, m_rowTable)
      changeCompletions(g, completion, changeType);

  m_currentMatch = completion;
}

#define COMPLETE_DELETE \
  if (rowDeleteStart != -1) { \
    deleteRows(g, filtered, index - rowDeleteStart + 1, rowDeleteStart); \
    rowDeleteStart = -1; \
  }

#define COMPLETE_ADD \
  if (rowAddStart != -1) { \
    addRows(g, filtered, rowAddStart, rowAdd); \
    rowAddStart = -1; \
    rowAdd.clear(); \
  }

#define DO_COMPARISON \
  QModelIndex sourceIndex = sourceModel()->index(prefilter.peekNext(), CodeCompletionModel::Name, QModelIndex()); \
  QString comp = sourceModel()->data(sourceIndex, Qt::DisplayRole).toString(); \
  bool toDisplay = comp.startsWith(newCompletion, m_caseSensitive);

void KateCompletionModel::changeCompletions( Group * g, const QString & newCompletion, changeTypes changeType )
{
  QMutableListIterator<int> filtered = g->rows;
  QListIterator<int> prefilter = g->prefilter;

  int rowDeleteStart = -1;
  int rowAddStart = -1;
  QList<int> rowAdd;

  int index = 0;
  while (prefilter.hasNext()) {
    if (filtered.hasNext()) {
      if (filtered.peekNext() == prefilter.peekNext()) {
        // Currently being displayed
        if (changeType != Broaden) {
          DO_COMPARISON

          if (toDisplay) {
            // no change required to this item
            COMPLETE_DELETE
            COMPLETE_ADD

          } else {
            // Needs to be hidden
            COMPLETE_ADD

            if (rowDeleteStart == -1)
              rowDeleteStart = index;
          }

        } else {
          COMPLETE_DELETE
          COMPLETE_ADD
        }

        // Advance iterator - item matched
        filtered.next();

      } else {
        // Currently hidden
        if (changeType != Narrow) {
          DO_COMPARISON

          if (toDisplay) {
            // needs to be made visible
            COMPLETE_DELETE

            if (rowAddStart == -1)
              rowAddStart = index;

            rowAdd.append(prefilter.peekNext());

          } else {
            // no change required to this item
            COMPLETE_DELETE
            COMPLETE_ADD
          }

        } else {
          COMPLETE_DELETE
          COMPLETE_ADD
        }
      }

    } else {
      // Currently hidden
      if (changeType != Narrow) {
        DO_COMPARISON

        if (toDisplay) {
          // needs to be made visible
          COMPLETE_DELETE

          if (rowAddStart == -1)
            rowAddStart = index;

          rowAdd.append(prefilter.peekNext());

        } else {
          // no change required to this item
          COMPLETE_DELETE
          COMPLETE_ADD
        }

      } else {
        COMPLETE_DELETE
        COMPLETE_ADD
      }
    }

    COMPLETE_DELETE
    COMPLETE_ADD

    ++index;
    prefilter.next();
  }
}

void KateCompletionModel::deleteRows( Group* g, QMutableListIterator<int> & filtered, int countBackwards, int startRow )
{
  beginRemoveRows(indexForGroup(g), startRow, startRow + countBackwards - 1);

  for (int i = 0; i < countBackwards; ++i) {
    filtered.previous();
    filtered.remove();
  }

  endRemoveRows();
}


void KateCompletionModel::addRows( Group * g, QMutableListIterator<int> & filtered, int startRow, const QList< int > & newItems )
{
  beginInsertRows(indexForGroup(g), startRow, startRow + newItems.count() - 1);

  for (int i = 0; i < newItems.count(); ++i)
    filtered.insert(newItems[i]);

  endInsertRows();
}

bool KateCompletionModel::indexIsCompletion( const QModelIndex & index ) const
{
  if (!hasGroups())
    return true;

  if (groupOfParent(index))
    return true;

  return false;
}

void KateCompletionModel::slotRowsInserted( const QModelIndex & parent, int start, int end )
{
}

void KateCompletionModel::slotRowsRemoved( const QModelIndex & parent, int start, int end )
{
}

bool KateCompletionModel::hasCompletionModel( ) const
{
  return m_hasCompletionModel;
}

KateCompletionModel::Group * KateCompletionModel::ungrouped( )
{
  if (!m_ungroupedDisplayed) {
    m_rowTable.append(m_ungrouped);
    m_ungroupedDisplayed = true;
  }

  return m_ungrouped;
}

#include "katecompletionmodel.moc"
