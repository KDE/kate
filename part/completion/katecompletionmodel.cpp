/* This file is part of the KDE libraries
   Copyright (C) 2005-2007 Hamish Rodda <rodda@kde.org>

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

using namespace KTextEditor;

KateCompletionModel::KateCompletionModel(KateCompletionWidget* parent)
  : QAbstractItemModel(parent)
  , m_matchCaseSensitivity(Qt::CaseInsensitive)
  , m_ungrouped(new Group(this))
  , m_sortingEnabled(false)
  , m_sortingAlphabetical(false)
  , m_sortingCaseSensitivity(Qt::CaseInsensitive)
  , m_sortingReverse(false)
  , m_filteringEnabled(false)
  , m_filterContextMatchesOnly(false)
  , m_filterByAttribute(false)
  , m_filterAttributes(KTextEditor::CodeCompletionModel::NoProperty)
  , m_maximumInheritanceDepth(0)
  , m_groupingEnabled(false)
  , m_accessConst(false)
  , m_accessStatic(false)
  , m_accesSignalSlot(false)
  , m_columnMergingEnabled(false)
{
  m_ungrouped->attribute = 0;
  m_ungrouped->title = i18n("Other");
  m_emptyGroups.append(m_ungrouped);
  m_groupHash.insert(0, m_ungrouped);
}

QVariant KateCompletionModel::data( const QModelIndex & index, int role ) const
{
  if (!hasCompletionModel() || !index.isValid())
    return QVariant();

  if (!hasGroups() || groupOfParent(index)) {
    switch (role) {
      case Qt::TextAlignmentRole:
        if (isColumnMergingEnabled() && m_columnMerges.count()) {
          int c = 0;
          foreach (const QList<int>& list, m_columnMerges) {
            foreach (int column, list) {
              if (c++ == index.column()) {
                if (column == CodeCompletionModel::Scope)
                  if (list.count() == 1)
                    return Qt::AlignRight;

                goto dontalign;
              }
            }
          }

        } else if ((!isColumnMergingEnabled() || m_columnMerges.isEmpty()) && index.column() == CodeCompletionModel::Scope) {
          return Qt::AlignRight;
        }

        dontalign:
        break;
    }

    // Merge text for column merging
    if (role == Qt::DisplayRole && m_columnMerges.count()) {
      QString text;
      foreach (int column, m_columnMerges[index.column()]) {
        QModelIndex sourceIndex = mapToSource(createIndex(index.row(), column, index.internalPointer()));
        text.append(sourceIndex.data(role).toString());
      }

      return text;
    }

    return mapToSource(index).data(role);
  }

  Group* g = groupForIndex(index);

  if (g) {
    switch (role) {
      case Qt::DisplayRole:
        if (!index.column())
          return g->title;
        break;

      case Qt::FontRole:
        if (!index.column()) {
          QFont f = view()->renderer()->config()->font();
          f.setBold(true);
          return f;
        }
        break;

      case Qt::ForegroundRole:
        return QColor(Qt::white);

      case Qt::BackgroundRole:
        return QColor(Qt::black);
    }
  }

  return QVariant();
}


Qt::ItemFlags KateCompletionModel::flags( const QModelIndex & index ) const
{
  if (!hasCompletionModel() || !index.isValid())
    return 0;

  if (!hasGroups() || groupOfParent(index))
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;

  return Qt::ItemIsEnabled;
}

KateView * KateCompletionModel::view( ) const
{
  return static_cast<const KateCompletionWidget*>(QObject::parent())->view();
}

void KateCompletionModel::setMatchCaseSensitivity( Qt::CaseSensitivity cs )
{
  m_matchCaseSensitivity = cs;
}

int KateCompletionModel::columnCount( const QModelIndex& ) const
{
  return isColumnMergingEnabled() && !m_columnMerges.isEmpty() ? m_columnMerges.count() : KTextEditor::CodeCompletionModel::ColumnCount;
}

KateCompletionModel::ModelRow KateCompletionModel::modelRowPair(const QModelIndex& index) const
{
  return qMakePair(static_cast<CodeCompletionModel*>(const_cast<QAbstractItemModel*>(index.model())), index.row());
}

bool KateCompletionModel::hasChildren( const QModelIndex & parent ) const
{
  if (!hasCompletionModel())
    return false;

  if (!parent.isValid()) {
    if (hasGroups())
      return true;

    return !m_ungrouped->rows.isEmpty();
  }

  if (parent.column() != 0)
    return false;

  if (!hasGroups())
    return false;

  if (Group* g = groupForIndex(parent))
    return !g->rows.isEmpty();

  return false;
}

QModelIndex KateCompletionModel::index( int row, int column, const QModelIndex & parent ) const
{
  if (row < 0 || column < 0 || column >= columnCount(QModelIndex()))
    return QModelIndex();

  if (parent.isValid() || !hasGroups()) {
    if (parent.isValid() && parent.column() != 0)
      return QModelIndex();

    Group* g = groupForIndex(parent);

    if (!g)
      return QModelIndex();

    if (row >= g->rows.count()) {
      //kWarning() << k_funcinfo << "Invalid index requested: row " << row << " beyond indivdual range in group " << g << endl;
      return QModelIndex();
    }

    //kDebug() << k_funcinfo << "Returning index for child " << row << " of group " << g << endl;
    return createIndex(row, column, g);
  }

  if (row >= m_rowTable.count()) {
    //kWarning() << k_funcinfo << "Invalid index requested: row " << row << " beyond group range." << endl;
    return QModelIndex();
  }

  //kDebug() << k_funcinfo << "Returning index for group " << m_rowTable[row] << endl;
  return createIndex(row, column, 0);
}

/*QModelIndex KateCompletionModel::sibling( int row, int column, const QModelIndex & index ) const
{
  if (row < 0 || column < 0 || column >= columnCount(QModelIndex()))
    return QModelIndex();

  if (!index.isValid()) {
  }

  if (Group* g = groupOfParent(index)) {
    if (row >= g->rows.count())
      return QModelIndex();

    return createIndex(row, column, g);
  }

  if (hasGroups())
    return QModelIndex();

  if (row >= m_ungrouped->rows.count())
    return QModelIndex();

  return createIndex(row, column, m_ungrouped);
}*/

bool KateCompletionModel::hasIndex( int row, int column, const QModelIndex & parent ) const
{
  if (row < 0 || column < 0 || column >= columnCount(QModelIndex()))
    return false;

  if (parent.isValid() || !hasGroups()) {
    if (parent.isValid() && parent.column() != 0)
      return false;

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
  if (!hasGroups())
    return QModelIndex();

  int row = m_rowTable.indexOf(g);

  if (row == -1)
    return QModelIndex();

  return createIndex(row, 0, 0);
}

void KateCompletionModel::clearGroups( )
{
  m_ungrouped->clear();
  // Don't bother trying to work out where it is
  m_rowTable.removeAll(m_ungrouped);
  m_emptyGroups.removeAll(m_ungrouped);

  qDeleteAll(m_rowTable);
  qDeleteAll(m_emptyGroups);
  m_rowTable.clear();
  m_emptyGroups.clear();
  m_groupHash.clear();

  m_emptyGroups.append(m_ungrouped);
  m_groupHash.insert(0, m_ungrouped);
}

void KateCompletionModel::createGroups()
{
  clearGroups();

  foreach (CodeCompletionModel* sourceModel, m_completionModels)
    for (int i = 0; i < sourceModel->rowCount(); ++i) {
      QModelIndex sourceIndex = sourceModel->index(i, CodeCompletionModel::Name, QModelIndex());

      int completionFlags = sourceIndex.data(CodeCompletionModel::CompletionRole).toInt();
      QString scopeIfNeeded = (groupingMethod() & Scope) ? sourceModel->index(i, CodeCompletionModel::Scope, QModelIndex()).data(Qt::DisplayRole).toString() : QString();

      Group* g = fetchGroup(completionFlags, scopeIfNeeded);

      g->addItem(Item(this, ModelRow(sourceModel, i)));
    }

  resort();
  reset();
}

KateCompletionModel::Group* KateCompletionModel::fetchGroup( int attribute, const QString& scope )
{
  if (!groupingMethod())
    return m_ungrouped;

  int groupingAttribute = groupingAttributes(attribute);
  //kDebug() << attribute << " " << groupingAttribute << endl;

  if (m_groupHash.contains(groupingAttribute))
    if (groupingMethod() & Scope) {
      for (QHash<int, Group*>::ConstIterator it = m_groupHash.find(groupingAttribute); it != m_groupHash.constEnd() && it.key() == groupingAttribute; ++it)
        if (it.value()->scope == scope)
          return it.value();
    } else {
      return m_groupHash.value(groupingAttribute);
    }

  Group* ret = new Group(this);

  ret->attribute = attribute;
  ret->scope = scope;

  QString st, at, it;

  if (groupingMethod() & ScopeType) {
    if (attribute & KTextEditor::CodeCompletionModel::GlobalScope)
      st = "Global Scope";
    else if (attribute & KTextEditor::CodeCompletionModel::NamespaceScope)
      st = "Namespace Scope";
    else if (attribute & KTextEditor::CodeCompletionModel::LocalScope)
      st = "Local Scope";
    else
      st = i18n("Unspecified Scope");

    ret->title = st;
  }

  if (groupingMethod() & Scope) {
    if (!ret->title.isEmpty())
      ret->title.append(" ");

    ret->title.append(scope);
  }

  if (groupingMethod() & AccessType) {
    if (attribute & KTextEditor::CodeCompletionModel::Public)
      at = "Public";
    else if (attribute & KTextEditor::CodeCompletionModel::Protected)
      at = "Protected";
    else if (attribute & KTextEditor::CodeCompletionModel::Private)
      at = "Private";
    else
      at = i18n("Unspecified Access");

    if (accessIncludeStatic() && attribute & KTextEditor::CodeCompletionModel::Static)
      at.append(i18n(" Static"));

    if (accessIncludeConst() && attribute & KTextEditor::CodeCompletionModel::Const)
      at.append(" Const");

    if (!ret->title.isEmpty())
      ret->title.append(", ");

    ret->title.append(at);
  }

  if (groupingMethod() & ItemType) {
    if (attribute & CodeCompletionModel::Namespace)
      it = i18n("Namespaces");
    else if (attribute & CodeCompletionModel::Class)
      it = i18n("Classes");
    else if (attribute & CodeCompletionModel::Struct)
      it = i18n("Structs");
    else if (attribute & CodeCompletionModel::Union)
      it = i18n("Unions");
    else if (attribute & CodeCompletionModel::Function)
      it = i18n("Functions");
    else if (attribute & CodeCompletionModel::Variable)
      it = i18n("Variables");
    else if (attribute & CodeCompletionModel::Enum)
      it = i18n("Enumerations");
    else
      it = i18n("Unspecified Item Type");

    if (!ret->title.isEmpty())
      ret->title.append(" ");

    ret->title.append(it);
  }

  m_emptyGroups.append(ret);
  m_groupHash.insert(groupingAttribute, ret);

  return ret;
}

bool KateCompletionModel::hasGroups( ) const
{
  return m_groupingEnabled;
}

KateCompletionModel::Group* KateCompletionModel::groupForIndex( const QModelIndex & index ) const
{
  if (!index.isValid())
    if (!hasGroups())
      return m_ungrouped;
    else
      return 0L;

  if (groupOfParent(index))
    return 0L;

  if (index.row() < 0 || index.row() >= m_rowTable.count())
    return m_ungrouped;

  return m_rowTable[index.row()];
}

/*QMap< int, QVariant > KateCompletionModel::itemData( const QModelIndex & index ) const
{
  if (!hasGroups() || groupOfParent(index)) {
    QModelIndex index = mapToSource(index);
    if (index.isValid())
      return index.model()->itemData(index);
  }

  return QAbstractItemModel::itemData(index);
}*/

QModelIndex KateCompletionModel::parent( const QModelIndex & index ) const
{
  if (!index.isValid())
    return QModelIndex();

  if (Group* g = groupOfParent(index)) {
    if (!hasGroups()) {
      Q_ASSERT(g == m_ungrouped);
      return QModelIndex();
    }

    int row = m_rowTable.indexOf(g);
    Q_ASSERT(row != -1);
    return createIndex(row, 0, 0);
  }

  return QModelIndex();
}

int KateCompletionModel::rowCount( const QModelIndex & parent ) const
{
  if (!parent.isValid())
    if (hasGroups()) {
      //kDebug() << k_funcinfo << "Returning row count for toplevel " << m_rowTable.count() << endl;
      return m_rowTable.count();
    } else {
      //kDebug() << k_funcinfo << "Returning ungrouped row count for toplevel " << m_ungrouped->rows.count() << endl;
      return m_ungrouped->rows.count();
    }

  Group* g = groupForIndex(parent);

  // This is not an error, seems you don't have to check hasChildren()
  if (!g)
    return 0;

  //kDebug() << k_funcinfo << "Returning row count for group " << g << " as " << g->rows.count() << endl;
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

  if (Group* g = groupOfParent(proxyIndex)) {
    ModelRow source = g->rows[proxyIndex.row()];
    return source.first->index(source.second, proxyIndex.column());
  }

  return QModelIndex();
}

QModelIndex KateCompletionModel::mapFromSource( const QModelIndex & sourceIndex ) const
{
  if (!sourceIndex.isValid())
    return QModelIndex();

  if (!hasGroups())
    return index(m_ungrouped->rows.indexOf(modelRowPair(sourceIndex)), sourceIndex.column(), QModelIndex());

  foreach (Group* g, m_rowTable) {
    int row = g->rows.indexOf(modelRowPair(sourceIndex));
    if (row != -1)
      return index(row, sourceIndex.column(), QModelIndex());
  }

  // Copied from above
  foreach (Group* g, m_emptyGroups) {
    int row = g->rows.indexOf(modelRowPair(sourceIndex));
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

  if (m_currentMatch.length() > completion.length() && m_currentMatch.startsWith(completion, m_matchCaseSensitivity)) {
    // Filter has been broadened
    changeType = Broaden;

  } else if (m_currentMatch.length() < completion.length() && completion.startsWith(m_currentMatch, m_matchCaseSensitivity)) {
    // Filter has been narrowed
    changeType = Narrow;
  }

  kDebug() << k_funcinfo << "Old match: " << m_currentMatch << ", new: " << completion << ", type: " << changeType << endl;

  if (!hasGroups())
    changeCompletions(m_ungrouped, completion, changeType);
  else {
    foreach (Group* g, m_rowTable)
      changeCompletions(g, completion, changeType);
    foreach (Group* g, m_emptyGroups)
      changeCompletions(g, completion, changeType);
  }

  m_currentMatch = completion;
}

#define COMPLETE_DELETE \
  if (rowDeleteStart != -1) { \
    if (!g->isEmpty) \
      deleteRows(g, filtered, index - rowDeleteStart, rowDeleteStart); \
    filterIndex -= index - rowDeleteStart + 1; \
    index -= index - rowDeleteStart; \
    rowDeleteStart = -1; \
  }

#define COMPLETE_ADD \
  if (rowAddStart != -1) { \
    addRows(g, filtered, rowAddStart, rowAdd); \
    filterIndex += rowAdd.count(); \
    index += rowAdd.count(); \
    rowAddStart = -1; \
    rowAdd.clear(); \
  }

void KateCompletionModel::changeCompletions( Group * g, const QString & newCompletion, changeTypes changeType )
{
  QMutableListIterator<ModelRow> filtered = g->rows;
  QMutableListIterator<Item> prefilter = g->prefilter;

  int rowDeleteStart = -1;
  int rowAddStart = -1;
  QList<ModelRow> rowAdd;

  int index = 0;
  int filterIndex = 0;
  while (prefilter.hasNext()) {
    if (filtered.hasNext()) {
      if (filtered.peekNext() == prefilter.peekNext().sourceRow()) {
        // Currently being displayed
        if (changeType != Broaden) {
          if (prefilter.peekNext().match(newCompletion)) {
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
        ++filterIndex;
        filtered.next();

      } else {
        // Currently hidden
        if (changeType != Narrow) {
          if (prefilter.peekNext().match(newCompletion)) {
            // needs to be made visible
            COMPLETE_DELETE

            if (rowAddStart == -1)
              rowAddStart = filterIndex;

            rowAdd.append(prefilter.peekNext().sourceRow());

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
        if (prefilter.peekNext().match(newCompletion)) {
          // needs to be made visible
          COMPLETE_DELETE

          if (rowAddStart == -1)
            rowAddStart = filterIndex;

          rowAdd.append(prefilter.peekNext().sourceRow());

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

    ++index;
    prefilter.next();
  }

  COMPLETE_DELETE
  COMPLETE_ADD

  hideOrShowGroup(g);
}

void KateCompletionModel::hideOrShowGroup(Group* g)
{
  if (!g->isEmpty) {
    if (g->rows.isEmpty()) {
      // Move to empty group list
      g->isEmpty = true;
      int row = m_rowTable.indexOf(g);
      if (row != -1) {
        if (hasGroups())
          beginRemoveRows(QModelIndex(), row, row);
        m_rowTable.removeAt(row);
        if (hasGroups())
          endRemoveRows();
        m_emptyGroups.append(g);
      } else {
        kWarning() << k_funcinfo << "Group " << g << " not found in row table!!" << endl;
      }
    }

  } else {
    if (!g->rows.isEmpty()) {
      // Move off empty group list
      // FIXME insert into correctly sorted location
      g->isEmpty = false;
      int row = m_rowTable.count();
      if (hasGroups())
        beginInsertRows(QModelIndex(), row, row);
      m_rowTable.append(g);
      if (hasGroups())
        endInsertRows();
      m_emptyGroups.removeAll(g);

      // Cheating a bit here... we already display these rows
      //beginInsertRows(indexForGroup(g), 0, g->rows.count() - 1);
      //endInsertRows();
    }
  }
}

void KateCompletionModel::deleteRows( Group* g, QMutableListIterator<ModelRow> & filtered, int countBackwards, int startRow )
{
  QModelIndex groupIndex = indexForGroup(g);
  Q_ASSERT(!hasGroups() || groupIndex.isValid());

  beginRemoveRows(groupIndex, startRow, startRow + countBackwards - 1);

  for (int i = 0; i < countBackwards; ++i) {
    filtered.remove();

    if (i == countBackwards - 1)
      break;

    if (!filtered.hasPrevious()) {
      kWarning() << k_funcinfo << "Tried to move back too far!" << endl;
      break;
    }

    filtered.previous();
  }

  endRemoveRows();
}


void KateCompletionModel::addRows( Group * g, QMutableListIterator<ModelRow> & filtered, int startRow, const QList<ModelRow> & newItems )
{
  bool notify = true;

  QModelIndex groupIndex = indexForGroup(g);
  if (hasGroups() && !groupIndex.isValid())
    // Group is currently hidden... don't emit begin/endInsertRows.
    notify = false;

  if (notify)
    beginInsertRows(groupIndex, startRow, startRow + newItems.count() - 1);

  for (int i = 0; i < newItems.count(); ++i)
    filtered.insert(newItems[i]);

  if (notify)
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

void KateCompletionModel::slotRowsInserted( const QModelIndex & /*parent*/, int /*start*/, int /*end*/ )
{
}

void KateCompletionModel::slotRowsRemoved( const QModelIndex & /*parent*/, int /*start*/, int /*end*/ )
{
}

bool KateCompletionModel::hasCompletionModel( ) const
{
  return !m_completionModels.isEmpty();
}

void KateCompletionModel::setFilteringEnabled( bool enable )
{
  if (m_filteringEnabled != enable)
    m_filteringEnabled = enable;
}

void KateCompletionModel::setSortingEnabled( bool enable )
{
  if (m_sortingEnabled != enable) {
    m_sortingEnabled = enable;
    resort();
  }
}

void KateCompletionModel::setGroupingEnabled(bool enable)
{
  if (m_groupingEnabled != enable)
    m_groupingEnabled = enable;
}

void KateCompletionModel::setColumnMergingEnabled(bool enable)
{
  if (m_columnMergingEnabled != enable)
    m_columnMergingEnabled = enable;
}

bool KateCompletionModel::isColumnMergingEnabled( ) const
{
  return m_columnMergingEnabled;
}

bool KateCompletionModel::isGroupingEnabled( ) const
{
  return m_groupingEnabled;
}

bool KateCompletionModel::isFilteringEnabled( ) const
{
  return m_filteringEnabled;
}

bool KateCompletionModel::isSortingEnabled( ) const
{
  return m_sortingEnabled;
}

QString KateCompletionModel::columnName( int column )
{
  switch (column) {
    case KTextEditor::CodeCompletionModel::Prefix:
      return i18n("Prefix");
    case KTextEditor::CodeCompletionModel::Icon:
      return i18n("Icon");
    case KTextEditor::CodeCompletionModel::Scope:
      return i18n("Scope");
    case KTextEditor::CodeCompletionModel::Name:
      return i18n("Name");
    case KTextEditor::CodeCompletionModel::Arguments:
      return i18n("Arguments");
    case KTextEditor::CodeCompletionModel::Postfix:
      return i18n("Postfix");
  }

  return QString();
}

const QList< QList < int > > & KateCompletionModel::columnMerges( ) const
{
  return m_columnMerges;
}

void KateCompletionModel::setColumnMerges( const QList< QList < int > > & columnMerges )
{
  m_columnMerges = columnMerges;
  reset();
}

int KateCompletionModel::translateColumn( int sourceColumn ) const
{
  if (m_columnMerges.isEmpty())
    return sourceColumn;

  int c = 0;
  foreach (const QList<int>& list, m_columnMerges) {
    foreach (int column, list) {
      if (column == sourceColumn)
        return c;
      c++;
    }
  }
  return -1;
}

int KateCompletionModel::groupingAttributes( int attribute ) const
{
  int ret = 0;

  if (m_groupingMethod & ScopeType) {
    if (countBits(attribute & ScopeTypeMask) > 1)
      kWarning() << k_funcinfo << "Invalid completion model metadata: more than one scope type modifier provided." << endl;

    if (attribute & KTextEditor::CodeCompletionModel::GlobalScope)
      ret |= KTextEditor::CodeCompletionModel::GlobalScope;
    else if (attribute & KTextEditor::CodeCompletionModel::NamespaceScope)
      ret |= KTextEditor::CodeCompletionModel::NamespaceScope;
    else if (attribute & KTextEditor::CodeCompletionModel::LocalScope)
      ret |= KTextEditor::CodeCompletionModel::LocalScope;
  }

  if (m_groupingMethod & AccessType) {
    if (countBits(attribute & AccessTypeMask) > 1)
      kWarning() << k_funcinfo << "Invalid completion model metadata: more than one access type modifier provided." << endl;

    if (attribute & KTextEditor::CodeCompletionModel::Public)
      ret |= KTextEditor::CodeCompletionModel::Public;
    else if (attribute & KTextEditor::CodeCompletionModel::Protected)
      ret |= KTextEditor::CodeCompletionModel::Protected;
    else if (attribute & KTextEditor::CodeCompletionModel::Private)
      ret |= KTextEditor::CodeCompletionModel::Private;

    if (accessIncludeStatic() && attribute & KTextEditor::CodeCompletionModel::Static)
      ret |= KTextEditor::CodeCompletionModel::Static;

    if (accessIncludeConst() && attribute & KTextEditor::CodeCompletionModel::Const)
      ret |= KTextEditor::CodeCompletionModel::Const;
  }

  if (m_groupingMethod & ItemType) {
    if (countBits(attribute & ItemTypeMask) > 1)
      kWarning() << k_funcinfo << "Invalid completion model metadata: more than one item type modifier provided." << endl;

    if (attribute & KTextEditor::CodeCompletionModel::Namespace)
      ret |= KTextEditor::CodeCompletionModel::Namespace;
    else if (attribute & KTextEditor::CodeCompletionModel::Class)
      ret |= KTextEditor::CodeCompletionModel::Class;
    else if (attribute & KTextEditor::CodeCompletionModel::Struct)
      ret |= KTextEditor::CodeCompletionModel::Struct;
    else if (attribute & KTextEditor::CodeCompletionModel::Union)
      ret |= KTextEditor::CodeCompletionModel::Union;
    else if (attribute & KTextEditor::CodeCompletionModel::Function)
      ret |= KTextEditor::CodeCompletionModel::Function;
    else if (attribute & KTextEditor::CodeCompletionModel::Variable)
      ret |= KTextEditor::CodeCompletionModel::Variable;
    else if (attribute & KTextEditor::CodeCompletionModel::Enum)
      ret |= KTextEditor::CodeCompletionModel::Enum;

    /*
    if (itemIncludeTemplate() && attribute & KTextEditor::CodeCompletionModel::Template)
      ret |= KTextEditor::CodeCompletionModel::Template;*/
  }

  return ret;
}

void KateCompletionModel::setGroupingMethod( GroupingMethods m )
{
  m_groupingMethod = m;

  createGroups();
}

bool KateCompletionModel::accessIncludeConst( ) const
{
  return m_accessConst;
}

void KateCompletionModel::setAccessIncludeConst( bool include )
{
  if (m_accessConst != include) {
    m_accessConst = include;

    if (groupingMethod() & AccessType)
      createGroups();
  }
}

bool KateCompletionModel::accessIncludeStatic( ) const
{
  return m_accessStatic;
}

void KateCompletionModel::setAccessIncludeStatic( bool include )
{
  if (m_accessStatic != include) {
    m_accessStatic = include;

    if (groupingMethod() & AccessType)
      createGroups();
  }
}

bool KateCompletionModel::accessIncludeSignalSlot( ) const
{
  return m_accesSignalSlot;
}

void KateCompletionModel::setAccessIncludeSignalSlot( bool include )
{
  if (m_accesSignalSlot != include) {
    m_accesSignalSlot = include;

    if (groupingMethod() & AccessType)
      createGroups();
  }
}

int KateCompletionModel::countBits( int value ) const
{
  int count = 0;
  for (int i = 1; i; i <<= 1)
    if (i & value)
      count++;

  return count;
}

KateCompletionModel::GroupingMethods KateCompletionModel::groupingMethod( ) const
{
  return m_groupingMethod;
}

bool KateCompletionModel::isSortingAlphabetical( ) const
{
  return m_sortingAlphabetical;
}

bool KateCompletionModel::isSortingReverse( ) const
{
  return m_sortingReverse;
}

Qt::CaseSensitivity KateCompletionModel::sortingCaseSensitivity( ) const
{
  return m_sortingCaseSensitivity;
}

KateCompletionModel::Item::Item( KateCompletionModel* m, ModelRow sr )
  : model(m)
  , m_sourceRow(sr)
  , matchCompletion(true)
  , matchFilters(true)
{
  filter();
  match();
}

bool KateCompletionModel::Item::operator <( const Item & rhs ) const
{
  int ret = 0;
  if (model->isSortingAlphabetical()) {
    QString c1 = completionName();
    QString c2 = rhs.completionName();
    if (model->sortingCaseSensitivity() == Qt::CaseSensitive) {
      c1 = c1.toLower();
      c2 = c2.toLower();
      ret = QString::localeAwareCompare(c1, c2);

    } else {
      ret = QString::localeAwareCompare(c1, c2);
    }

    //kDebug() << k_funcinfo << c1 << " c/w " << c2 << " -> " << (model->isSortingReverse() ? ret > 0 : ret < 0) << " (" << ret << ")" << endl;

  } else {
    // FIXME need to define a better default ordering for multiple model display
    ret = m_sourceRow.second - rhs.m_sourceRow.second;
  }

  return model->isSortingReverse() ? ret > 0 : ret < 0;
}

QString KateCompletionModel::Item::completionName( ) const
{
  return m_sourceRow.first->index(m_sourceRow.second, CodeCompletionModel::Name, QModelIndex()).data(Qt::DisplayRole).toString();
}

void KateCompletionModel::Group::addItem( Item i )
{
  if (model->isSortingEnabled()) {
    QList<Item>::Iterator it = model->isSortingReverse() ? qUpperBound(--prefilter.end(), prefilter.end(), i) : qUpperBound(prefilter.begin(), prefilter.end(), i);
    if (it != prefilter.end()) {
      prefilter.insert(it, i);
      if (i.isVisible()) {
        int index = rows.indexOf(it->sourceRow());
        if (index != -1)
          rows.append(i.sourceRow());
        else
          rows.insert(index, i.sourceRow());
      }
    } else {
      prefilter.append(i);
      if (i.isVisible())
        rows.append(i.sourceRow());
    }

  } else {
    if (i.isVisible())
      prefilter.append(i);
  }
}

KateCompletionModel::Group::Group( KateCompletionModel * m )
  : model(m)
  , isEmpty(true)
{
  Q_ASSERT(model);
}

void KateCompletionModel::setSortingAlphabetical( bool alphabetical )
{
  if (m_sortingAlphabetical != alphabetical) {
    m_sortingAlphabetical = alphabetical;
    resort();
  }
}

void KateCompletionModel::Group::resort( )
{
  qStableSort(prefilter.begin(), prefilter.end());
  //int oldRowCount = rows.count();
  rows.clear();
  foreach (const Item& i, prefilter)
    if (i.isVisible())
      rows.append(i.sourceRow());

  model->hideOrShowGroup(this);

  //Q_ASSERT(rows.count() == oldRowCount);
}

void KateCompletionModel::setSortingCaseSensitivity( Qt::CaseSensitivity cs )
{
  if (m_sortingCaseSensitivity != cs) {
    m_sortingCaseSensitivity = cs;
    resort();
  }
}

void KateCompletionModel::setSortingReverse( bool reverse )
{
  if (m_sortingReverse != reverse) {
    m_sortingReverse = reverse;
    resort();
  }
}

void KateCompletionModel::resort( )
{
  foreach (Group* g, m_rowTable)
    g->resort();

  foreach (Group* g, m_emptyGroups)
    g->resort();
}

bool KateCompletionModel::Item::isValid( ) const
{
  return model && m_sourceRow.first && m_sourceRow.second >= 0;
}

void KateCompletionModel::Group::clear( )
{
  prefilter.clear();
  rows.clear();
  isEmpty = true;
}

bool KateCompletionModel::filterContextMatchesOnly( ) const
{
  return m_filterContextMatchesOnly;
}

void KateCompletionModel::setFilterContextMatchesOnly( bool filter )
{
  if (m_filterContextMatchesOnly != filter) {
    m_filterContextMatchesOnly = filter;
    refilter();
  }
}

bool KateCompletionModel::filterByAttribute( ) const
{
  return m_filterByAttribute;
}

void KateCompletionModel::setFilterByAttribute( bool filter )
{
  if (m_filterByAttribute == filter) {
    m_filterByAttribute = filter;
    refilter();
  }
}

KTextEditor::CodeCompletionModel::CompletionProperties KateCompletionModel::filterAttributes( ) const
{
  return m_filterAttributes;
}

void KateCompletionModel::setFilterAttributes( KTextEditor::CodeCompletionModel::CompletionProperties attributes )
{
  if (m_filterAttributes == attributes) {
    m_filterAttributes = attributes;
    refilter();
  }
}

int KateCompletionModel::maximumInheritanceDepth( ) const
{
  return m_maximumInheritanceDepth;
}

void KateCompletionModel::setMaximumInheritanceDepth( int maxDepth )
{
  if (m_maximumInheritanceDepth != maxDepth) {
    m_maximumInheritanceDepth = maxDepth;
    refilter();
  }
}

void KateCompletionModel::refilter( )
{
  m_ungrouped->refilter();

  foreach (Group* g, m_rowTable)
    g->refilter();

  foreach (Group* g, m_emptyGroups)
    g->refilter();
}

void KateCompletionModel::Group::refilter( )
{
  rows.clear();
  foreach (const Item& i, prefilter)
    if (!i.isFiltered())
      rows.append(i.sourceRow());
}

bool KateCompletionModel::Item::filter( )
{
  matchFilters = false;

  if (model->isFilteringEnabled()) {
    QModelIndex sourceIndex = m_sourceRow.first->index(m_sourceRow.second, CodeCompletionModel::Name, QModelIndex());

    if (model->filterContextMatchesOnly()) {
      QVariant contextMatch = sourceIndex.data(CodeCompletionModel::MatchType);
      if (contextMatch.canConvert(QVariant::Bool) && !contextMatch.toBool())
        goto filter;
    }

    if (model->filterByAttribute()) {
      int completionFlags = sourceIndex.data(CodeCompletionModel::CompletionRole).toInt();
      if (model->filterAttributes() & completionFlags)
        goto filter;
    }

    if (model->maximumInheritanceDepth() > 0) {
      int inheritanceDepth = sourceIndex.data(CodeCompletionModel::InheritanceDepth).toInt();
      if (inheritanceDepth > model->maximumInheritanceDepth())
        goto filter;
    }
  }

  matchFilters = true;

  filter:
  return matchFilters;
}

bool KateCompletionModel::Item::match(const QString& newCompletion)
{
  // Check to see if the item is matched by the current completion string
  QModelIndex sourceIndex = m_sourceRow.first->index(m_sourceRow.second, CodeCompletionModel::Name, QModelIndex());

  QString match = newCompletion;
  if (match.isEmpty())
    match = model->currentCompletion();

  matchCompletion = sourceIndex.data(Qt::DisplayRole).toString().startsWith(match, model->matchCaseSensitivity());
  return matchCompletion;
}

QString KateCompletionModel::propertyName( KTextEditor::CodeCompletionModel::CompletionProperty property )
{
  switch (property) {
    case CodeCompletionModel::Public:
      return i18n("Public");

    case CodeCompletionModel::Protected:
      return i18n("Protected");

    case CodeCompletionModel::Private:
      return i18n("Private");

    case CodeCompletionModel::Static:
      return i18n("Static");

    case CodeCompletionModel::Const:
      return i18n("Constant");

    case CodeCompletionModel::Namespace:
      return i18n("Namespace");

    case CodeCompletionModel::Class:
      return i18n("Class");

    case CodeCompletionModel::Struct:
      return i18n("Struct");

    case CodeCompletionModel::Union:
      return i18n("Union");

    case CodeCompletionModel::Function:
      return i18n("Function");

    case CodeCompletionModel::Variable:
      return i18n("Variable");

    case CodeCompletionModel::Enum:
      return i18n("Enumeration");

    case CodeCompletionModel::Template:
      return i18n("Template");

    case CodeCompletionModel::Virtual:
      return i18n("Virtual");

    case CodeCompletionModel::Override:
      return i18n("Override");

    case CodeCompletionModel::Inline:
      return i18n("Inline");

    case CodeCompletionModel::Friend:
      return i18n("Friend");

    case CodeCompletionModel::Signal:
      return i18n("Signal");

    case CodeCompletionModel::Slot:
      return i18n("Slot");

    case CodeCompletionModel::LocalScope:
      return i18n("Local Scope");

    case CodeCompletionModel::NamespaceScope:
      return i18n("Namespace Scope");

    case CodeCompletionModel::GlobalScope:
      return i18n("Global Scope");

    default:
      return i18n("Unknown Property");
  }
}

bool KateCompletionModel::Item::isVisible( ) const
{
  return matchCompletion && matchFilters;
}

bool KateCompletionModel::Item::isFiltered( ) const
{
  return !matchFilters;
}

bool KateCompletionModel::Item::isMatching( ) const
{
  return matchFilters;
}

KateCompletionModel::ModelRow KateCompletionModel::Item::sourceRow( ) const
{
  return m_sourceRow;
}

const QString & KateCompletionModel::currentCompletion( ) const
{
  return m_currentMatch;
}

Qt::CaseSensitivity KateCompletionModel::matchCaseSensitivity( ) const
{
  return m_matchCaseSensitivity;
}

void KateCompletionModel::addCompletionModel(KTextEditor::CodeCompletionModel * model)
{
  if (m_completionModels.contains(model))
    return;

  m_completionModels.append(model);

  connect(model, SIGNAL(rowsInserted(const QModelIndex&, int, int)), SLOT(slotRowsInserted(const QModelIndex&, int, int)));
  connect(model, SIGNAL(rowsRemoved(const QModelIndex&, int, int)), SLOT(slotRowsRemoved(const QModelIndex&, int, int)));

  // This performs the reset
  createGroups();
}

void KateCompletionModel::setCompletionModel(KTextEditor::CodeCompletionModel* model)
{
  clearCompletionModels(true);
  addCompletionModel(model);
}

void KateCompletionModel::setCompletionModels(const QList<KTextEditor::CodeCompletionModel*>& models)
{
  //if (m_completionModels == models)
    //return;

  clearCompletionModels(true);

  m_completionModels = models;

  foreach (KTextEditor::CodeCompletionModel* model, models) {
    connect(model, SIGNAL(rowsInserted(const QModelIndex&, int, int)), SLOT(slotRowsInserted(const QModelIndex&, int, int)));
    connect(model, SIGNAL(rowsRemoved(const QModelIndex&, int, int)), SLOT(slotRowsRemoved(const QModelIndex&, int, int)));
  }

  // This performs the reset
  createGroups();
}

QList< KTextEditor::CodeCompletionModel * > KateCompletionModel::completionModels() const
{
  return m_completionModels;
}

void KateCompletionModel::removeCompletionModel(CodeCompletionModel * model)
{
  if (!model || m_completionModels.contains(model))
    return;

  clearGroups();

  model->disconnect(this);

  m_completionModels.removeAll(model);

  if (!m_completionModels.isEmpty())
    createGroups();

  reset();
}

void KateCompletionModel::clearCompletionModels(bool skipReset)
{
  foreach (CodeCompletionModel * model, m_completionModels)
    model->disconnect(this);

  m_completionModels.clear();

  clearGroups();

  if (!skipReset)
    reset();
}

#include "katecompletionmodel.moc"
