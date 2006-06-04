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

using namespace KTextEditor;

KateCompletionModel::KateCompletionModel(KateCompletionWidget* parent)
  : QAbstractProxyModel(parent)
  , m_matchCaseSensitivity(Qt::CaseInsensitive)
  , m_hasCompletionModel(false)
  , m_ungrouped(new Group())
  , m_ungroupedDisplayed(false)
  , m_sortingEnabled(false)
  , m_filteringEnabled(false)
  , m_groupingEnabled(false)
  , m_columnMergingEnabled(false)
{
  m_ungrouped->attribute = 0;
  m_ungrouped->title = i18n("Other");
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
      foreach (int column, m_columnMerges[index.column()])
        text.append(sourceModel()->data(mapToSource(createIndex(index.row(), column, index.internalPointer())), role).toString());

      return text;
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
        if (!index.column()) {
          QFont f = view()->renderer()->config()->font();
          f.setBold(true);
          return f;
        }
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
  m_groupHash.clear();
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

  if (hasCompletionModel()) {
    connect(sourceModel(), SIGNAL(rowsInserted(const QModelIndex&, int, int)), SLOT(slotRowsInserted(const QModelIndex&, int, int)));
    connect(sourceModel(), SIGNAL(rowsRemoved(const QModelIndex&, int, int)), SLOT(slotRowsRemoved(const QModelIndex&, int, int)));

    createGroups();
  }

  reset();
}

void KateCompletionModel::setMatchCaseSensitivity( Qt::CaseSensitivity cs )
{
  m_matchCaseSensitivity = cs;
}

int KateCompletionModel::columnCount( const QModelIndex& ) const
{
  return isColumnMergingEnabled() && !m_columnMerges.isEmpty() ? m_columnMerges.count() : KTextEditor::CodeCompletionModel::ColumnCount;
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
  if (row < 0 || column < 0 || column >= columnCount(QModelIndex()))
    return QModelIndex();

  if (parent.isValid()) {
    Group* g = groupForIndex(parent);

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

QModelIndex KateCompletionModel::sibling( int row, int column, const QModelIndex & index ) const
{
  if (!index.isValid() || row < 0 || column < 0 || column >= columnCount(QModelIndex()))
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
  if (row < 0 || column < 0 || column >= columnCount(QModelIndex()))
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
    QString scopeIfNeeded = (groupingMethod() & Scope) ? sourceModel()->data(sourceModel()->index(i, CodeCompletionModel::Scope, QModelIndex()), Qt::DisplayRole).toString() : QString();

    Group* g = fetchGroup(completionFlags, scopeIfNeeded);

    if (sourceModel()->data(sourceIndex, Qt::DisplayRole).toString().startsWith(m_currentMatch, m_matchCaseSensitivity))
      g->rows.append(i);

    g->prefilter.append(i);
  }
}

KateCompletionModel::Group* KateCompletionModel::fetchGroup( int attribute, const QString& scope )
{
  if (!groupingMethod())
    return ungrouped();

  int groupingAttribute = groupingAttributes(attribute);
  kDebug() << attribute << " " << groupingAttribute << endl;

  if (m_groupHash.contains(groupingAttribute))
    if (groupingMethod() & Scope) {
      for (QHash<int, Group*>::ConstIterator it = m_groupHash.find(groupingAttribute); it != m_groupHash.constEnd() && it.key() == groupingAttribute; ++it)
        if (it.value()->scope == scope)
          return it.value();
    } else {
      return m_groupHash.value(groupingAttribute);
    }

  Group* ret = new Group();

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

  m_rowTable.append(ret);
  m_groupHash.insert(groupingAttribute, ret);

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
      //kDebug() << k_funcinfo << "Returning row count for toplevel " << m_rowTable.count() << endl;
      return m_rowTable.count();
    } else {
      //kDebug() << k_funcinfo << "Returning ungrouped row count for toplevel " << m_ungrouped->rows.count() << endl;
      return m_ungrouped->rows.count();
    }

  Group* g = groupForIndex(parent);
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

  if (m_currentMatch.length() > completion.length() && m_currentMatch.startsWith(completion, m_matchCaseSensitivity)) {
    // Filter has been broadened
    changeType = Broaden;

  } else if (m_currentMatch.length() < completion.length() && completion.startsWith(m_currentMatch, m_matchCaseSensitivity)) {
    // Filter has been narrowed
    changeType = Narrow;
  }

  //kDebug() << k_funcinfo << "Old match: " << m_currentMatch << ", new: " << completion << ", type: " << changeType << endl;

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
  bool toDisplay = comp.startsWith(newCompletion, m_matchCaseSensitivity);

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

void KateCompletionModel::slotRowsInserted( const QModelIndex & /*parent*/, int /*start*/, int /*end*/ )
{
}

void KateCompletionModel::slotRowsRemoved( const QModelIndex & /*parent*/, int /*start*/, int /*end*/ )
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

void KateCompletionModel::setFilteringEnabled( bool enable )
{
  if (m_filteringEnabled != enable)
    m_filteringEnabled = enable;
}

void KateCompletionModel::setSortingEnabled( bool enable )
{
  if (m_sortingEnabled != enable)
    m_sortingEnabled = enable;
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
  int count;
  for (int i = 0; i; i <<= 1)
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

#include "katecompletionmodel.moc"
