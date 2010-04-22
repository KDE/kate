/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2005-2006 Hamish Rodda <rodda@kde.org>
 *  Copyright (C) 2007-2008 David Nolden <david.nolden.kdevelop@art-master.de>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "katecompletionmodel.h"

#include <QTextEdit>
#include <QMultiMap>
#include <QTimer>

#include <klocale.h>
#include <kiconloader.h>
#include <kapplication.h>

#include "katecompletionwidget.h"
#include "katecompletiontree.h"
#include "katecompletiondelegate.h"
#include "kateargumenthintmodel.h"
#include "kateview.h"
#include "katerenderer.h"
#include "kateconfig.h"
#include <ktexteditor/codecompletionmodelcontrollerinterface.h>

using namespace KTextEditor;

///A helper-class for handling completion-models with hierarchical grouping/optimization
class HierarchicalModelHandler {
public:
  HierarchicalModelHandler(CodeCompletionModel* model);
  void addValue(CodeCompletionModel::ExtraItemDataRoles role, const QVariant& value);
  //Walks the index upwards and collects all defined completion-roles on the way
  void collectRoles(const QModelIndex& index);
  void takeRole(const QModelIndex& index);

  CodeCompletionModel* model() const;

  //Assumes that index is a sub-index of the indices where role-values were taken
  QVariant getData(CodeCompletionModel::ExtraItemDataRoles role, const QModelIndex& index) const;
  
  bool hasHierarchicalRoles() const;

  int inheritanceDepth(const QModelIndex& i) const;
  
  QString customGroup() const {
    return m_customGroup;
  }
  
  int customGroupingKey() const {
    return m_groupSortingKey;
  }
private:
  typedef QMap<CodeCompletionModel::ExtraItemDataRoles, QVariant> RoleMap;
  RoleMap m_roleValues;
  QString m_customGroup;
  int m_groupSortingKey;
  CodeCompletionModel* m_model;
};

CodeCompletionModel* HierarchicalModelHandler::model() const {
  return m_model;
}

bool HierarchicalModelHandler::hasHierarchicalRoles() const {
  return !m_roleValues.isEmpty();
}

void HierarchicalModelHandler::collectRoles(const QModelIndex& index) {
  if( index.parent().isValid() )
    collectRoles(index.parent());
  if(m_model->rowCount(index) != 0)
    takeRole(index);
}

int HierarchicalModelHandler::inheritanceDepth(const QModelIndex& i) const {
  return getData(CodeCompletionModel::InheritanceDepth, i).toInt();
}

void HierarchicalModelHandler::takeRole(const QModelIndex& index) {
  QVariant v = index.data(CodeCompletionModel::GroupRole);
  if( v.isValid() && v.canConvert(QVariant::Int) ) {
    QVariant value = index.data(v.toInt());
    if(v.toInt() == Qt::DisplayRole) {
      m_customGroup = index.data(Qt::DisplayRole).toString();
      QVariant sortingKey = index.data(CodeCompletionModel::InheritanceDepth);
      if(sortingKey.canConvert(QVariant::Int))
        m_groupSortingKey = sortingKey.toInt();
    }else{
      m_roleValues[(CodeCompletionModel::ExtraItemDataRoles)v.toInt()] = value;
    }
  }else{
    kDebug( 13035 ) << "Did not return valid GroupRole in hierarchical completion-model";
  }
}

QVariant HierarchicalModelHandler::getData(CodeCompletionModel::ExtraItemDataRoles role, const QModelIndex& index) const {
  RoleMap::const_iterator it = m_roleValues.find(role);
  if( it != m_roleValues.end() )
    return *it;
  else
    return index.data(role);
}

HierarchicalModelHandler::HierarchicalModelHandler(CodeCompletionModel* model) : m_groupSortingKey(-1),  m_model(model)  {
}

void HierarchicalModelHandler::addValue(CodeCompletionModel::ExtraItemDataRoles role, const QVariant& value) {
  m_roleValues[role] = value;
}

KateCompletionModel::KateCompletionModel(KateCompletionWidget* parent)
  : ExpandingWidgetModel(parent)
  , m_hasGroups(false)
  , m_matchCaseSensitivity(Qt::CaseInsensitive)
  , m_ungrouped(new Group(this))
  , m_argumentHints(new Group(this))
  , m_bestMatches(new Group(this))
  , m_sortingEnabled(false)
  , m_sortingAlphabetical(false)
  , m_isSortingByInheritance(false)
  , m_sortingCaseSensitivity(Qt::CaseInsensitive)
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
//   , m_haveExactMatch(false)
{

  m_ungrouped->attribute = 0;
  m_argumentHints->attribute = -1;
  m_bestMatches->attribute = BestMatchesProperty;

  m_argumentHints->title = i18n("Argument-hints");
  m_bestMatches->title = i18n("Best matches");

  m_emptyGroups.append(m_ungrouped);
  m_emptyGroups.append(m_argumentHints);
  m_emptyGroups.append(m_bestMatches);

  m_updateBestMatchesTimer = new QTimer(this);
  m_updateBestMatchesTimer->setSingleShot(true);
  connect(m_updateBestMatchesTimer, SIGNAL(timeout()), this, SLOT(updateBestMatches()));

  m_groupHash.insert(0, m_ungrouped);
  m_groupHash.insert(-1, m_argumentHints);
  m_groupHash.insert(BestMatchesProperty, m_argumentHints);
}

KateCompletionModel::~KateCompletionModel() {
  clearCompletionModels();
  delete m_argumentHints;
  delete m_ungrouped;
  delete m_bestMatches;
}

QTreeView* KateCompletionModel::treeView() const {
  return view()->completionWidget()->treeView();
}

QVariant KateCompletionModel::data( const QModelIndex & index, int role ) const
{
  if (!hasCompletionModel() || !index.isValid())
    return QVariant();

  if( role == Qt::DecorationRole && index.column() == KTextEditor::CodeCompletionModel::Prefix && isExpandable(index) )
  {
    cacheIcons();

    if( !isExpanded(index ) )
      return QVariant( m_collapsedIcon );
    else
      return QVariant( m_expandedIcon );
  }

  //groupOfParent returns a group when the index is a member of that group, but not the group head/label.
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
    if (role == Qt::DisplayRole && m_columnMerges.count() && isColumnMergingEnabled()) {
      QString text;
      foreach (int column, m_columnMerges[index.column()]) {
        QModelIndex sourceIndex = mapToSource(createIndex(index.row(), column, index.internalPointer()));
        text.append(sourceIndex.data(role).toString());
      }

      return text;
    }

    if(role == CodeCompletionModel::HighlightingMethod)
    {
      //Return that we are doing custom-highlighting of one of the sub-strings does it. Unfortunately internal highlighting does not work for the other substrings.
      foreach (int column, m_columnMerges[index.column()]) {
	QModelIndex sourceIndex = mapToSource(createIndex(index.row(), column, index.internalPointer()));
	QVariant method = sourceIndex.data(CodeCompletionModel::HighlightingMethod);
	if( method.type() == QVariant::Int && method.toInt() ==  CodeCompletionModel::CustomHighlighting)
	  return QVariant(CodeCompletionModel::CustomHighlighting);
      }
      return QVariant();
    }
    if(role == CodeCompletionModel::CustomHighlight)
    {
      //Merge custom highlighting if multiple columns were merged
      QStringList strings;

      //Collect strings
      foreach (int column, m_columnMerges[index.column()])
          strings << mapToSource(createIndex(index.row(), column, index.internalPointer())).data(Qt::DisplayRole).toString();

      QList<QVariantList> highlights;

      //Collect custom-highlightings
      foreach (int column, m_columnMerges[index.column()])
          highlights << mapToSource(createIndex(index.row(), column, index.internalPointer())).data(CodeCompletionModel::CustomHighlight).toList();

      return mergeCustomHighlighting( strings, highlights, 0 );
    }

    QVariant v = mapToSource(index).data(role);
    if( v.isValid() )
      return v;
    else
      return ExpandingWidgetModel::data(index, role);
  }

  //Returns a nonzero group if this index is the head of a group(A Label in the list)
  Group* g = groupForIndex(index);
  
  if (g && (!g->isEmpty)) {
    switch (role) {
      case Qt::DisplayRole:
          //We return the group-header for all columns, ExpandingDelegate will paint them properly over the whole space
          return QString(' ' + g->title);
        break;

      case Qt::FontRole:
        if (!index.column()) {
          QFont f = view()->renderer()->config()->font();
          f.setBold(true);
          return f;
        }
        break;

      case Qt::ForegroundRole:
        return KApplication::kApplication()->palette().toolTipText().color();
      case Qt::BackgroundRole:
        return KApplication::kApplication()->palette().toolTipBase().color();
    }
  }

  return QVariant();
}

int KateCompletionModel::contextMatchQuality(const QModelIndex& index) const {
  if(!index.isValid())
    return 0;
  Group* g = groupOfParent(index);
  if(!g || g->filtered.size() < index.row())
    return 0;
  
  return contextMatchQuality(g->filtered[index.row()].sourceRow());
}

int KateCompletionModel::contextMatchQuality(const ModelRow& source) const {
  QModelIndex realIndex = source.second;

  int bestMatch = -1;
  //Iterate through all argument-hints and find the best match-quality
  foreach( const Item& item, m_argumentHints->filtered )
  {
    const ModelRow& row(item.sourceRow());
    if( realIndex.model() != row.first )
      continue; //We can only match within the same source-model

    QModelIndex hintIndex = row.second;

    QVariant depth = hintIndex.data(CodeCompletionModel::ArgumentHintDepth);
    if( !depth.isValid() || depth.type() != QVariant::Int || depth.toInt() != 1 )
      continue; //Only match completion-items to argument-hints of depth 1(the ones the item will be given to as argument)

    hintIndex.data(CodeCompletionModel::SetMatchContext);

    QVariant matchQuality = realIndex.data(CodeCompletionModel::MatchQuality);
    if( matchQuality.isValid() && matchQuality.type() == QVariant::Int ) {
      int m = matchQuality.toInt();
      if( m > bestMatch )
        bestMatch = m;
    }
  }
  
  if(m_argumentHints->filtered.isEmpty()) {
    QVariant matchQuality = realIndex.data(CodeCompletionModel::MatchQuality);
    if( matchQuality.isValid() && matchQuality.type() == QVariant::Int ) {
      int m = matchQuality.toInt();
      if( m > bestMatch )
        bestMatch = m;
    }
  }

  return bestMatch;
}

Qt::ItemFlags KateCompletionModel::flags( const QModelIndex & index ) const
{
  if (!hasCompletionModel() || !index.isValid())
    return 0;

  if (!hasGroups() || groupOfParent(index))
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;

  return Qt::ItemIsEnabled;
}

KateCompletionWidget* KateCompletionModel::widget() const {
  return static_cast<KateCompletionWidget*>(QObject::parent());
}

KateView * KateCompletionModel::view( ) const
{
  return widget()->view();
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
  return qMakePair(static_cast<CodeCompletionModel*>(const_cast<QAbstractItemModel*>(index.model())), index);
}

bool KateCompletionModel::hasChildren( const QModelIndex & parent ) const
{
  if (!hasCompletionModel())
    return false;

  if (!parent.isValid()) {
    if (hasGroups())
      return true;

    return !m_ungrouped->filtered.isEmpty();
  }

  if (parent.column() != 0)
    return false;

  if (!hasGroups())
    return false;

  if (Group* g = groupForIndex(parent))
    return !g->filtered.isEmpty();

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

    if (row >= g->filtered.count()) {
      //kWarning() << "Invalid index requested: row " << row << " beyond indivdual range in group " << g;
      return QModelIndex();
    }

    //kDebug( 13035 ) << "Returning index for child " << row << " of group " << g;
    return createIndex(row, column, g);
  }

  if (row >= m_rowTable.count()) {
    //kWarning() << "Invalid index requested: row " << row << " beyond group range.";
    return QModelIndex();
  }

  //kDebug( 13035 ) << "Returning index for group " << m_rowTable[row];
  return createIndex(row, column, 0);
}

/*QModelIndex KateCompletionModel::sibling( int row, int column, const QModelIndex & index ) const
{
  if (row < 0 || column < 0 || column >= columnCount(QModelIndex()))
    return QModelIndex();

  if (!index.isValid()) {
  }

  if (Group* g = groupOfParent(index)) {
    if (row >= g->filtered.count())
      return QModelIndex();

    return createIndex(row, column, g);
  }

  if (hasGroups())
    return QModelIndex();

  if (row >= m_ungrouped->filtered.count())
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

    if (row >= g->filtered.count())
      return false;

    return true;
  }

  if (row >= m_rowTable.count())
    return false;

  return true;
}

QModelIndex KateCompletionModel::indexForRow( Group * g, int row ) const
{
  if (row < 0 || row >= g->filtered.count())
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

void KateCompletionModel::clearGroups( bool shouldReset )
{
  clearExpanding();
  m_ungrouped->clear();
  m_argumentHints->clear();
  m_bestMatches->clear();

  // Don't bother trying to work out where it is
  m_rowTable.removeAll(m_ungrouped);
  m_emptyGroups.removeAll(m_ungrouped);

  m_rowTable.removeAll(m_argumentHints);
  m_emptyGroups.removeAll(m_argumentHints);

  m_rowTable.removeAll(m_bestMatches);
  m_emptyGroups.removeAll(m_bestMatches);

  qDeleteAll(m_rowTable);
  qDeleteAll(m_emptyGroups);
  m_rowTable.clear();
  m_emptyGroups.clear();
  m_groupHash.clear();
  m_customGroupHash.clear();

  m_emptyGroups.append(m_ungrouped);
  m_groupHash.insert(0, m_ungrouped);

  m_emptyGroups.append(m_argumentHints);
  m_groupHash.insert(-1, m_argumentHints);

  m_emptyGroups.append(m_bestMatches);
  m_groupHash.insert(BestMatchesProperty, m_bestMatches);

  if(shouldReset)
    reset();
}

QSet<KateCompletionModel::Group*> KateCompletionModel::createItems(const HierarchicalModelHandler& _handler, const QModelIndex& i, bool notifyModel) {
  HierarchicalModelHandler handler(_handler);
  QSet<Group*> ret;

  if( handler.model()->rowCount(i) == 0 ) {
    //Leaf node, create an item
    ret.insert( createItem(handler, i, notifyModel) );
  } else {
    //Non-leaf node, take the role from the node, and recurse to the sub-nodes
    handler.takeRole(i);
    for(int a = 0; a < handler.model()->rowCount(i); a++)
      ret += createItems(handler, i.child(a, 0), notifyModel);
  }

  return ret;
}

QSet<KateCompletionModel::Group*> KateCompletionModel::deleteItems(const QModelIndex& i) {
  QSet<Group*> ret;

  if( i.model()->rowCount(i) == 0 ) {
    //Leaf node, delete the item
    Group* g = groupForIndex(mapFromSource(i));
    ret.insert(g);
    g->removeItem(ModelRow(const_cast<CodeCompletionModel*>(static_cast<const CodeCompletionModel*>(i.model())), i));
  } else {
    //Non-leaf node
    for(int a = 0; a < i.model()->rowCount(i); a++)
      ret += deleteItems(i.child(a, 0));
  }

  return ret;
}

void KateCompletionModel::createGroups()
{
  //After clearing the model, it has to be reset, else we will be in an invalid state while inserting
  //new groups.
  clearGroups(true);

  bool has_groups=false;
  foreach (CodeCompletionModel* sourceModel, m_completionModels) {
    has_groups|=sourceModel->hasGroups();
    for (int i = 0; i < sourceModel->rowCount(); ++i)
      createItems(HierarchicalModelHandler(sourceModel), sourceModel->index(i, 0));
  }
  m_hasGroups=has_groups;

  //debugStats();

  foreach (Group* g, m_rowTable)
    hideOrShowGroup(g);

  foreach (Group* g, m_emptyGroups)
    hideOrShowGroup(g);

  updateBestMatches();
  
  reset();

  emit contentGeometryChanged();
}

KateCompletionModel::Group* KateCompletionModel::createItem(const HierarchicalModelHandler& handler, const QModelIndex& sourceIndex, bool notifyModel)
{
  //QModelIndex sourceIndex = sourceModel->index(row, CodeCompletionModel::Name, QModelIndex());

  int completionFlags = handler.getData(CodeCompletionModel::CompletionRole, sourceIndex).toInt();

  //Scope is expensive, should not be used with big models
  QString scopeIfNeeded = (groupingMethod() & Scope) ? sourceIndex.sibling(sourceIndex.row(), CodeCompletionModel::Scope).data(Qt::DisplayRole).toString() : QString();

  int argumentHintDepth = handler.getData(CodeCompletionModel::ArgumentHintDepth, sourceIndex).toInt();

  Group* g;
  if( argumentHintDepth ) {
    g = m_argumentHints;
  } else{
    QString customGroup = handler.customGroup();
    if(!customGroup.isNull() && m_hasGroups) {
      if(m_customGroupHash.contains(customGroup)) {
        g = m_customGroupHash[customGroup];
      }else{
        g = new Group(this);
        g->title = customGroup;
        g->customSortingKey = handler.customGroupingKey();
        m_emptyGroups.append(g);
        m_customGroupHash.insert(customGroup, g);
      }
    }else{
      g = fetchGroup(completionFlags, scopeIfNeeded, handler.hasHierarchicalRoles());
    }
  }

  Item item = Item(g != m_argumentHints, this, handler, ModelRow(handler.model(), sourceIndex));

  if(g != m_argumentHints)
    item.match();

  g->addItem(item, notifyModel);

  return g;
}

void KateCompletionModel::slotRowsInserted( const QModelIndex & parent, int start, int end )
{
  QSet<Group*> affectedGroups;

  HierarchicalModelHandler handler(static_cast<CodeCompletionModel*>(sender()));
  if(parent.isValid())
    handler.collectRoles(parent);


  for (int i = start; i <= end; ++i)
    affectedGroups += createItems(handler, parent.isValid() ? parent.child(i, 0) :  handler.model()->index(i, 0), true);

  foreach (Group* g, affectedGroups)
      hideOrShowGroup(g);

    emit contentGeometryChanged();
}

void KateCompletionModel::slotRowsRemoved( const QModelIndex & parent, int start, int end )
{
  CodeCompletionModel* source = static_cast<CodeCompletionModel*>(sender());

  QSet<Group*> affectedGroups;

  for (int i = start; i <= end; ++i) {
    QModelIndex index = parent.isValid() ? parent.child(i, 0) :  source->index(i, 0);

    affectedGroups += deleteItems(index);
  }

  foreach (Group* g, affectedGroups)
    hideOrShowGroup(g);

  emit contentGeometryChanged();
}

KateCompletionModel::Group* KateCompletionModel::fetchGroup( int attribute, const QString& scope, bool forceGrouping )
{
  Q_UNUSED(forceGrouping);

  ///@todo use forceGrouping
  if (!hasGroups())
    return m_ungrouped;

  int groupingAttribute = groupingAttributes(attribute);
  //kDebug( 13035 ) << attribute << " " << groupingAttribute;

  if (m_groupHash.contains(groupingAttribute)) {
    if (groupingMethod() & Scope) {
      for (QHash<int, Group*>::ConstIterator it = m_groupHash.constFind(groupingAttribute); it != m_groupHash.constEnd() && it.key() == groupingAttribute; ++it)
        if (it.value()->scope == scope)
          return it.value();
    } else {
      return m_groupHash.value(groupingAttribute);
    }
  }
  Group* ret = new Group(this);

  ret->attribute = attribute;
  ret->scope = scope;

  QString st, at, it;

  if (groupingMethod() & ScopeType) {
    if (attribute & KTextEditor::CodeCompletionModel::GlobalScope)
      st = "Global";
    else if (attribute & KTextEditor::CodeCompletionModel::NamespaceScope)
      st = "Namespace";
    else if (attribute & KTextEditor::CodeCompletionModel::LocalScope)
      st = "Local";

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

    if (accessIncludeStatic() && attribute & KTextEditor::CodeCompletionModel::Static)
      at.append(" Static");

    if (accessIncludeConst() && attribute & KTextEditor::CodeCompletionModel::Const)
      at.append(" Const");

    if( !at.isEmpty() ) {
      if (!ret->title.isEmpty())
        ret->title.append(", ");

      ret->title.append(at);
    }
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

    if( !it.isEmpty() ) {
      if (!ret->title.isEmpty())
        ret->title.append(" ");

      ret->title.append(it);
    }
  }

  m_emptyGroups.append(ret);
  m_groupHash.insert(groupingAttribute, ret);

  return ret;
}

bool KateCompletionModel::hasGroups( ) const
{
  //kDebug( 13035 ) << "m_groupHash.size()"<<m_groupHash.size();
  //kDebug( 13035 ) << "m_rowTable.count()"<<m_rowTable.count();
  // We cannot decide whether there is groups easily. The problem: The code-model can
  // be populated with a delay from within a background-thread.
  // Proper solution: Ask all attached code-models(Through a new interface) whether they want to use grouping,
  // and if at least one wants to, return true, else return false.
  return m_groupingEnabled && m_hasGroups;
}

KateCompletionModel::Group* KateCompletionModel::groupForIndex( const QModelIndex & index ) const
{
  if (!index.isValid()) {
    if (!hasGroups())
      return m_ungrouped;
    else
      return 0L;
  }

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

    if (row == -1) {
      kWarning() << "Couldn't find parent for index" << index;
      return QModelIndex();
    }

    return createIndex(row, 0, 0);
  }

  return QModelIndex();
}

int KateCompletionModel::rowCount( const QModelIndex & parent ) const
{
  if (!parent.isValid()) {
    if (hasGroups()) {
      //kDebug( 13035 ) << "Returning row count for toplevel " << m_rowTable.count();
      return m_rowTable.count();
    } else {
      //kDebug( 13035 ) << "Returning ungrouped row count for toplevel " << m_ungrouped->filtered.count();
      return m_ungrouped->filtered.count();
    }
  }

  Group* g = groupForIndex(parent);

  // This is not an error, seems you don't have to check hasChildren()
  if (!g)
    return 0;

  //kDebug( 13035 ) << "Returning row count for group " << g << " as " << g->filtered.count();
  return g->filtered.count();
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
    if( proxyIndex.row() >= 0 && proxyIndex.row() < g->filtered.count() ) {
      ModelRow source = g->filtered[proxyIndex.row()].sourceRow();
      return source.second.sibling(source.second.row(), proxyIndex.column());
    }else{
      kDebug( 13035 ) << "Invalid proxy-index";
    }
  }

  return QModelIndex();
}

QModelIndex KateCompletionModel::mapFromSource( const QModelIndex & sourceIndex ) const
{
  if (!sourceIndex.isValid())
    return QModelIndex();

  if (!hasGroups())
    return index(m_ungrouped->rowOf(modelRowPair(sourceIndex)), sourceIndex.column(), QModelIndex());

  foreach (Group* g, m_rowTable) {
    int row = g->rowOf(modelRowPair(sourceIndex));
    if (row != -1)
      return index(row, sourceIndex.column(), indexForGroup(g));
  }

  // Copied from above
  foreach (Group* g, m_emptyGroups) {
    int row = g->rowOf(modelRowPair(sourceIndex));
    if (row != -1)
      return index(row, sourceIndex.column(), indexForGroup(g));
  }

  return QModelIndex();
}

void KateCompletionModel::setCurrentCompletion( KTextEditor::CodeCompletionModel* model, const QString & completion )
{
  if (m_currentMatch[model] == completion)
    return;

  if (!hasCompletionModel()) {
    m_currentMatch[model] = completion;
    return;
  }
  
  changeTypes changeType = Change;

  if (m_currentMatch[model].length() > completion.length() && m_currentMatch[model].startsWith(completion, m_matchCaseSensitivity)) {
    // Filter has been broadened
    changeType = Broaden;

  } else if (m_currentMatch[model].length() < completion.length() && completion.startsWith(m_currentMatch[model], m_matchCaseSensitivity)) {
    // Filter has been narrowed
    changeType = Narrow;
  }

  //kDebug( 13035 ) << model << "Old match: " << m_currentMatch[model] << ", new: " << completion << ", type: " << changeType;

  m_currentMatch[model] = completion;

  bool needsReset = false;
  
  if (!hasGroups()) {
    needsReset |= changeCompletions(m_ungrouped, changeType);
  } else {
    foreach (Group* g, m_rowTable) {
      if(g != m_argumentHints)
        needsReset |= changeCompletions(g, changeType);
    }
    foreach (Group* g, m_emptyGroups) {
      if(g != m_argumentHints)
        needsReset |= changeCompletions(g, changeType);
    }

    updateBestMatches();
  }

  kDebug()<<"needsReset"<<needsReset;
  if(needsReset)
    reset();

  clearExpanding(); //We need to do this, or be aware of expanding-widgets while filtering.
  emit contentGeometryChanged();
  kDebug();
}

QString KateCompletionModel::commonPrefixInternal(const QString &forcePrefix) const
{
  QString commonPrefix;   // isNull() = true
  
  QList< Group* > groups = m_rowTable; 
  groups += m_ungrouped;
  
  foreach (Group* g, groups) {
    foreach(const Item& item, g->filtered)
    {
      uint startPos = m_currentMatch[item.sourceRow().first].length();
      const QString candidate = item.name().mid(startPos);
      
      if(!candidate.startsWith(forcePrefix))
        continue;
      
      if(commonPrefix.isNull()) {
        commonPrefix = candidate;
        
        //Replace QString::null prefix with QString(""), so we won't initialize it again
        if(commonPrefix.isNull())
          commonPrefix = QString("");  // isEmpty() = true, isNull() = false
      }else{
        commonPrefix = commonPrefix.left(candidate.length());
        
        for(int a = 0; a < commonPrefix.length(); ++a) {
          if(commonPrefix[a] != candidate[a]) {
            commonPrefix = commonPrefix.left(a);
            break;
          }
        }
      }
    }
  }
  
  return commonPrefix;
}

QString KateCompletionModel::commonPrefix(QModelIndex selectedIndex) const
{
  QString commonPrefix = commonPrefixInternal(QString());
  
  if(commonPrefix.isEmpty() && selectedIndex.isValid()) {
    Group* g = m_ungrouped;
    if(hasGroups())
      g = groupOfParent(selectedIndex);
    
    if(g && selectedIndex.row() < g->filtered.size())
    {
      //Follow the path of the selected item, finding the next non-empty common prefix
      Item item = g->filtered[selectedIndex.row()];
      int matchLength = m_currentMatch[item.sourceRow().first].length();
      commonPrefix = commonPrefixInternal(item.name().mid(matchLength).left(1));
    }
  }
  
  return commonPrefix;
}

bool KateCompletionModel::changeCompletions( Group * g, changeTypes changeType )
{
  bool notifyModel = true;
  if(changeType != Narrow) {
    notifyModel = false;
    g->filtered = g->prefilter;
    //In the "Broaden" or "Change" case, just re-filter everything,
    //and don't notify the model. The model is notified afterwards through a reset().
  }
  //This code determines what of the filtered items still fit, and computes the ranges that were removed, giving
  //them to beginRemoveRows(..) in batches
  
  QList <KateCompletionModel::Item > newFiltered;
  int deleteUntil = -1; //In each state, the range [currentRow+1, deleteUntil] needs to be deleted
  for(int currentRow = g->filtered.count()-1; currentRow >= 0; --currentRow) {
    if(g->filtered[currentRow].match()) {
      //This row does not need to be deleted, which means that currentRow+1 to deleteUntil need to be deleted now
      if(deleteUntil != -1 && notifyModel) {
        beginRemoveRows(indexForGroup(g), currentRow+1, deleteUntil);
        endRemoveRows();
      }
      deleteUntil = -1;
      
      newFiltered.prepend(g->filtered[currentRow]);
    }else{
      if(deleteUntil == -1)
        deleteUntil = currentRow; //Mark that this row needs to be deleted
    }
  }
  
  if(deleteUntil != -1) {
    beginRemoveRows(indexForGroup(g), 0, deleteUntil);
    endRemoveRows();
  }
  
  g->filtered = newFiltered;
  hideOrShowGroup(g, notifyModel);
  return !notifyModel;
}

int KateCompletionModel::Group::orderNumber() const {
    if( this == model->m_ungrouped )
      return 700;

    if(customSortingKey != -1)
      return customSortingKey;
    
    if( attribute & BestMatchesProperty )
      return 1;
    
    if (attribute & KTextEditor::CodeCompletionModel::LocalScope)
      return 100;
    else if (attribute & KTextEditor::CodeCompletionModel::Public)
      return 200;
    else if (attribute & KTextEditor::CodeCompletionModel::Protected)
      return 300;
    else if (attribute & KTextEditor::CodeCompletionModel::Private)
      return 400;
    else if (attribute & KTextEditor::CodeCompletionModel::NamespaceScope)
      return 500;
    else if (attribute & KTextEditor::CodeCompletionModel::GlobalScope)
      return 600;


    return 700;
}

bool KateCompletionModel::Group::orderBefore(Group* other) const {
    return orderNumber() < other->orderNumber();
}

void KateCompletionModel::hideOrShowGroup(Group* g, bool notifyModel)
{
  if( g == m_argumentHints ) {
    emit argumentHintsChanged();
    m_updateBestMatchesTimer->start(200); //We have new argument-hints, so we have new best matches
    return; //Never show argument-hints in the normal completion-list
  }

  if (!g->isEmpty) {
    if (g->filtered.isEmpty()) {
      // Move to empty group list
      g->isEmpty = true;
      int row = m_rowTable.indexOf(g);
      if (row != -1) {
        if (hasGroups() && notifyModel)
          beginRemoveRows(QModelIndex(), row, row);
        m_rowTable.removeAt(row);
        if (hasGroups() && notifyModel)
          endRemoveRows();
        m_emptyGroups.append(g);
      } else {
        kWarning() << "Group " << g << " not found in row table!!";
      }
    }

  } else {

    if (!g->filtered.isEmpty()) {
      // Move off empty group list
      g->isEmpty = false;

      int row = 0; //Find row where to insert
      for( int a = 0; a < m_rowTable.count(); a++ ) {
        if( g->orderBefore(m_rowTable[a]) ) {
        row = a;
        break;
        }
        row = a+1;
      }

      if(notifyModel) {
        if (hasGroups())
          beginInsertRows(QModelIndex(), row, row);
        else
          beginInsertRows(QModelIndex(), 0, g->filtered.count());
      }
      m_rowTable.insert(row, g);
      if(notifyModel)
        endInsertRows();
      m_emptyGroups.removeAll(g);
    }
  }
}

bool KateCompletionModel::indexIsItem( const QModelIndex & index ) const
{
  if (!hasGroups())
    return true;

  if (groupOfParent(index))
    return true;

  return false;
}

void KateCompletionModel::slotModelReset()
{
  createGroups();

  //debugStats();
}

void KateCompletionModel::debugStats()
{
  if (!hasGroups())
    kDebug( 13035 ) << "Model groupless, " << m_ungrouped->filtered.count() << " items.";
  else {
    kDebug( 13035 ) << "Model grouped (" << m_rowTable.count() << " groups):";
    foreach (Group* g, m_rowTable)
      kDebug( 13035 ) << "Group" << g << "count" << g->filtered.count();
  }
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

  /* Debugging - dump column merge list

  QString columnMerge;
  foreach (const QList<int>& list, m_columnMerges) {
    columnMerge += '[';
    foreach (int column, list) {
      columnMerge += QString::number(column) + " ";
    }
    columnMerge += "] ";
  }

  kDebug( 13035 ) << k_funcinfo << columnMerge;*/

  int c = 0;
  foreach (const QList<int>& list, m_columnMerges) {
    foreach (int column, list) {
      if (column == sourceColumn)
        return c;
    }
    c++;
  }
  return -1;
}

int KateCompletionModel::groupingAttributes( int attribute ) const
{
  int ret = 0;

  if (m_groupingMethod & ScopeType) {
    if (countBits(attribute & ScopeTypeMask) > 1)
      kWarning() << "Invalid completion model metadata: more than one scope type modifier provided.";

    if (attribute & KTextEditor::CodeCompletionModel::GlobalScope)
      ret |= KTextEditor::CodeCompletionModel::GlobalScope;
    else if (attribute & KTextEditor::CodeCompletionModel::NamespaceScope)
      ret |= KTextEditor::CodeCompletionModel::NamespaceScope;
    else if (attribute & KTextEditor::CodeCompletionModel::LocalScope)
      ret |= KTextEditor::CodeCompletionModel::LocalScope;
  }

  if (m_groupingMethod & AccessType) {
    if (countBits(attribute & AccessTypeMask) > 1)
      kWarning() << "Invalid completion model metadata: more than one access type modifier provided.";

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
      kWarning() << "Invalid completion model metadata: more than one item type modifier provided.";

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

bool KateCompletionModel::isSortingByInheritanceDepth() const {
  return m_isSortingByInheritance;
}
void KateCompletionModel::setSortingByInheritanceDepth(bool byInheritance) {
  m_isSortingByInheritance = byInheritance;
}

bool KateCompletionModel::isSortingAlphabetical( ) const
{
  return m_sortingAlphabetical;
}

Qt::CaseSensitivity KateCompletionModel::sortingCaseSensitivity( ) const
{
  return m_sortingCaseSensitivity;
}

KateCompletionModel::Item::Item( bool doInitialMatch, KateCompletionModel* m, const HierarchicalModelHandler& handler, ModelRow sr )
  : model(m)
  , m_sourceRow(sr)
  , matchCompletion(StartsWithMatch)
  , matchFilters(true)
  , m_haveExactMatch(false)
{
  
  inheritanceDepth = handler.getData(CodeCompletionModel::InheritanceDepth, m_sourceRow.second).toInt();

  QModelIndex nameSibling = sr.second.sibling(sr.second.row(), CodeCompletionModel::Name);
  m_nameColumn = nameSibling.data(Qt::DisplayRole).toString();

  if(doInitialMatch) {
    filter();
    match();
  }
}

bool KateCompletionModel::Item::operator <( const Item & rhs ) const
{
  int ret = 0;

  //kDebug( 13035 ) << c1 << " c/w " << c2 << " -> " << (model->isSortingReverse() ? ret > 0 : ret < 0) << " (" << ret << ")";

  if( model->isSortingByInheritanceDepth() )
    ret = inheritanceDepth - rhs.inheritanceDepth;

  if (ret == 0 && model->isSortingAlphabetical()) {
    if(!m_completionSortingName.isEmpty() && !rhs.m_completionSortingName.isEmpty())
      //Shortcut, plays a role in this tight loop
      ret = QString::compare(m_completionSortingName, rhs.m_completionSortingName);
    else
      ret = QString::compare(completionSortingName(), rhs.completionSortingName()); //Do not use localeAwareCompare, because it is simply too slow for a list of about 1000 items
  }

  if( ret == 0 ) {
    // FIXME need to define a better default ordering for multiple model display
    ret = m_sourceRow.second.row() - rhs.m_sourceRow.second.row();
  }

  return ret < 0;
}

QString KateCompletionModel::Item::completionSortingName( ) const
{
  if(m_completionSortingName.isEmpty()) {
    m_completionSortingName = m_nameColumn;
    if (model->sortingCaseSensitivity() == Qt::CaseInsensitive)
      m_completionSortingName = m_completionSortingName.toLower();
  }

  return m_completionSortingName;
}

void KateCompletionModel::Group::addItem( Item i, bool notifyModel )
{
  if (isEmpty)
    notifyModel = false;

  QModelIndex groupIndex;
  if (notifyModel)
    groupIndex = model->indexForGroup(this);

  if (model->isSortingEnabled()) {
    
    prefilter.insert(qUpperBound(prefilter.begin(), prefilter.end(), i), i);
    if(i.isVisible()) {
      QList<Item>::iterator it = qUpperBound(filtered.begin(), filtered.end(), i);
      uint rowNumber = it - filtered.begin();
      
      if(notifyModel)
        model->beginInsertRows(groupIndex, rowNumber, rowNumber);
      
      filtered.insert(it, i);
    }
  } else {
    if(notifyModel)
      model->beginInsertRows(groupIndex, prefilter.size(), prefilter.size());
    if (i.isVisible())
      prefilter.append(i);
  }
  
  if(notifyModel)
    model->endInsertRows();
}

bool KateCompletionModel::Group::removeItem(const ModelRow& row)
{
  for (int pi = 0; pi < prefilter.count(); ++pi)
    if (prefilter[pi].sourceRow() == row) {
      int index = rowOf(row);
      if (index != -1)
        model->beginRemoveRows(model->indexForGroup(this), index, index);

      filtered.removeAt(index);
      prefilter.removeAt(pi);

      if (index != -1)
        model->endRemoveRows();

      return index != -1;
    }

  Q_ASSERT(false);
  return false;
}

KateCompletionModel::Group::Group( KateCompletionModel * m )
  : model(m)
  , isEmpty(true)
  , customSortingKey(-1)
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
  //int oldRowCount = filtered.count();
  filtered.clear();
  foreach (const Item& i, prefilter)
    if (i.isVisible())
      filtered.append(i);

  model->hideOrShowGroup(this);
  //Q_ASSERT(filtered.count() == oldRowCount);
}

void KateCompletionModel::setSortingCaseSensitivity( Qt::CaseSensitivity cs )
{
  if (m_sortingCaseSensitivity != cs) {
    m_sortingCaseSensitivity = cs;
    resort();
  }
}

void KateCompletionModel::resort( )
{
  foreach (Group* g, m_rowTable)
    g->resort();

  foreach (Group* g, m_emptyGroups)
    g->resort();

  emit contentGeometryChanged();
}

bool KateCompletionModel::Item::isValid( ) const
{
  return model && m_sourceRow.first && m_sourceRow.second.row() >= 0;
}

void KateCompletionModel::Group::clear( )
{
  prefilter.clear();
  filtered.clear();
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
    if(g != m_argumentHints)
      g->refilter();

  foreach (Group* g, m_emptyGroups)
    if(g != m_argumentHints)
      g->refilter();

  updateBestMatches();

  clearExpanding(); //We need to do this, or be aware of expanding-widgets while filtering.
}

void KateCompletionModel::Group::refilter( )
{
  filtered.clear();
  foreach (const Item& i, prefilter)
    if (!i.isFiltered())
      filtered.append(i);
}

bool KateCompletionModel::Item::filter( )
{
  matchFilters = false;

  if (model->isFilteringEnabled()) {
    QModelIndex sourceIndex = m_sourceRow.second.sibling(m_sourceRow.second.row(), CodeCompletionModel::Name);

    if (model->filterContextMatchesOnly()) {
      QVariant contextMatch = sourceIndex.data(CodeCompletionModel::MatchQuality);
      if (contextMatch.canConvert(QVariant::Int) && !contextMatch.toInt())
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

uint KateCompletionModel::filteredItemCount() const
{
  uint ret = 0;
  foreach(Group* group, m_rowTable)
    ret += group->filtered.size();
  
  return ret;
}

bool KateCompletionModel::shouldMatchHideCompletionList() const {
//   return m_haveExactMatch;
  ///@todo Make this faster
  foreach(Group* group, m_rowTable)
    foreach(const Item& item, group->filtered)
      if(item.haveExactMatch()) {
        KTextEditor::CodeCompletionModelControllerInterface2* iface2 = dynamic_cast<KTextEditor::CodeCompletionModelControllerInterface2*>(item.sourceRow().first);
        KTextEditor::CodeCompletionModelControllerInterface3* iface3 = dynamic_cast<KTextEditor::CodeCompletionModelControllerInterface3*>(item.sourceRow().first);
        if (! (iface2 || iface3) ) return true;
        if(iface2 && iface2->matchingItem(item.sourceRow().second) == KTextEditor::CodeCompletionModelControllerInterface2::HideListIfAutomaticInvocation)
          return true;
        if(iface3 && iface3->matchingItem(item.sourceRow().second) == KTextEditor::CodeCompletionModelControllerInterface3::HideListIfAutomaticInvocation)
          return true;
      }
  return false;
}

KateCompletionModel::Item::MatchType KateCompletionModel::Item::match()
{
  // Check to see if the item is matched by the current completion string
  QModelIndex sourceIndex = m_sourceRow.second.sibling(m_sourceRow.second.row(), CodeCompletionModel::Name);

  QString match = model->currentCompletion(m_sourceRow.first);

  m_haveExactMatch = false;
  
   // Hehe, everything matches nothing! (ie. everything matches a blank string)
   if (match.isEmpty())
     return PerfectMatch;
  
  matchCompletion = (m_nameColumn.startsWith(match, model->matchCaseSensitivity()) ? StartsWithMatch : NoMatch);

  if(matchCompletion && match.length() == m_nameColumn.length()) {
    matchCompletion = PerfectMatch;
    m_haveExactMatch = true;
  }
  
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

const KateCompletionModel::ModelRow& KateCompletionModel::Item::sourceRow( ) const
{
  return m_sourceRow;
}

QString KateCompletionModel::currentCompletion( KTextEditor::CodeCompletionModel* model ) const
{
  return m_currentMatch.value(model);
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
  connect(model, SIGNAL(modelReset()), SLOT(slotModelReset()));

  // This performs the reset
  createGroups();
}

void KateCompletionModel::setCompletionModel(KTextEditor::CodeCompletionModel* model)
{
  clearCompletionModels();
  addCompletionModel(model);
}

void KateCompletionModel::setCompletionModels(const QList<KTextEditor::CodeCompletionModel*>& models)
{
  //if (m_completionModels == models)
    //return;

  clearCompletionModels();

  m_completionModels = models;

  foreach (KTextEditor::CodeCompletionModel* model, models) {
    connect(model, SIGNAL(rowsInserted(const QModelIndex&, int, int)), SLOT(slotRowsInserted(const QModelIndex&, int, int)));
    connect(model, SIGNAL(rowsRemoved(const QModelIndex&, int, int)), SLOT(slotRowsRemoved(const QModelIndex&, int, int)));
    connect(model, SIGNAL(modelReset()), SLOT(slotModelReset()));
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
  if (!model || !m_completionModels.contains(model))
    return;

  m_currentMatch.remove(model);

  clearGroups(false);

  model->disconnect(this);

  m_completionModels.removeAll(model);

  if (!m_completionModels.isEmpty()) {
    // This performs the reset
    createGroups();
  }else{
    emit contentGeometryChanged();
    reset();
  }
}

//Updates the best-matches group
void KateCompletionModel::updateBestMatches() {
  int maxMatches = 300; //We cannot do too many operations here, because they are all executed whenever a character is added. Would be nice if we could split the operations up somewhat using a timer.

  m_updateBestMatchesTimer->stop();
  //Maps match-qualities to ModelRows paired together with the BestMatchesCount returned by the items.
  typedef QMultiMap<int, QPair<int, ModelRow> > BestMatchMap;
  BestMatchMap matches;
  
  if(!hasGroups()) {
    //If there is no grouping, just change the order of the items, moving the best matching ones to the front
    QMultiMap<int, int> rowsForQuality;
    
    int row = 0;
    foreach(const Item& item, m_ungrouped->filtered) {
      ModelRow source = item.sourceRow();
      
      QVariant v = source.second.data(CodeCompletionModel::BestMatchesCount);

      if( v.type() == QVariant::Int && v.toInt() > 0 ) {
        int quality = contextMatchQuality(source);
        if(quality > 0)
          rowsForQuality.insert(quality, row);
      }
      
      ++row;
      --maxMatches;
      if(maxMatches < 0)
        break;
    }
    
    if(!rowsForQuality.isEmpty()) {
      //Rewrite m_ungrouped->filtered in a new order
      QSet<int> movedToFront;
      QList<Item> newFiltered;
      for(QMultiMap<int, int>::const_iterator it = rowsForQuality.constBegin(); it != rowsForQuality.constEnd(); ++it) {
        newFiltered.prepend(m_ungrouped->filtered[it.value()]);
        movedToFront.insert(it.value());
      }
      
      {
        uint size = m_ungrouped->filtered.size();
        for(uint a = 0; a < size; ++a)
          if(!movedToFront.contains(a))
            newFiltered.append(m_ungrouped->filtered[a]);
      }
      m_ungrouped->filtered = newFiltered;
    }
    return;
  }
  
  ///@todo Cache the CodeCompletionModel::BestMatchesCount
  foreach (Group* g, m_rowTable) {
    if( g == m_bestMatches )
      continue;
    for( int a = 0; a < g->filtered.size(); a++ )
    {
      ModelRow source = g->filtered[a].sourceRow();

      QVariant v = source.second.data(CodeCompletionModel::BestMatchesCount);

      if( v.type() == QVariant::Int && v.toInt() > 0 ) {
        //Return the best match with any of the argument-hints

        int quality = contextMatchQuality(source);
        if( quality > 0 )
          matches.insert(quality, qMakePair(v.toInt(), g->filtered[a].sourceRow()));
        --maxMatches;
      }

      if( maxMatches < 0 )
        break;
    }
    if( maxMatches < 0 )
      break;
  }

  //Now choose how many of the matches will be taken. This is done with the rule:
  //The count of shown best-matches should equal the average count of their BestMatchesCounts
  int cnt = 0;
  int matchesSum = 0;
  BestMatchMap::const_iterator it = matches.constEnd();
  while( it != matches.constBegin() )
  {
    --it;
    ++cnt;
    matchesSum += (*it).first;
    if( cnt > matchesSum / cnt )
      break;
  }

  m_bestMatches->filtered.clear();
  
  it = matches.constEnd();

  while( it != matches.constBegin() && cnt > 0 )
  {
    --it;
    --cnt;

    m_bestMatches->filtered.append( Item( true, this, HierarchicalModelHandler((*it).second.first), (*it).second) );
  }

  hideOrShowGroup(m_bestMatches);
}

void KateCompletionModel::rowSelected(const QModelIndex& row) {
  ExpandingWidgetModel::rowSelected(row);
  ///@todo delay this
  int rc = widget()->argumentHintModel()->rowCount(QModelIndex());
  if( rc == 0 ) return;

  //For now, simply update the whole column 0
  QModelIndex start = widget()->argumentHintModel()->index(0,0);
  QModelIndex end = widget()->argumentHintModel()->index(rc-1,0);

  widget()->argumentHintModel()->emitDataChanged(start, end);
}

void KateCompletionModel::clearCompletionModels()
{
  foreach (CodeCompletionModel * model, m_completionModels)
    model->disconnect(this);

  m_completionModels.clear();

  m_currentMatch.clear();

  clearGroups();
}

#include "katecompletionmodel.moc"
// kate: space-indent on; indent-width 2; replace-tabs on;
