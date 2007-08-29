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

#ifndef KATECOMPLETIONMODEL_H
#define KATECOMPLETIONMODEL_H

#include <QtGui/QAbstractProxyModel>
#include <QtCore/QPair>
#include <QtCore/QList>

#include <ktexteditor/codecompletionmodel.h>

#include "expandingwidgetmodel.h"

class KateCompletionWidget;
class KateArgumentHintModel;
class KateView;
class QWidget;
class QTextEdit;

/**
 * This class has the responsibility for filtering, sorting, and manipulating
 * code completion data provided by a CodeCompletionModel.
 *
 * @author Hamish Rodda <rodda@kde.org>
 */
class KateCompletionModel : public ExpandingWidgetModel
{
  Q_OBJECT

  public:
    KateCompletionModel(KateCompletionWidget* parent = 0L);
    ~KateCompletionModel();

    QList<KTextEditor::CodeCompletionModel*> completionModels() const;
    void clearCompletionModels(bool skipReset = false);
    void addCompletionModel(KTextEditor::CodeCompletionModel* model);
    void setCompletionModel(KTextEditor::CodeCompletionModel* model);
    void setCompletionModels(const QList<KTextEditor::CodeCompletionModel*>& models);
    void removeCompletionModel(KTextEditor::CodeCompletionModel* model);

    KateView* view() const;
    KateCompletionWidget* widget() const;

    const QString& currentCompletion() const;
    void setCurrentCompletion(const QString& completion);

    Qt::CaseSensitivity matchCaseSensitivity() const;
    void setMatchCaseSensitivity( Qt::CaseSensitivity cs );

    static QString columnName(int column);
    int translateColumn(int sourceColumn) const;

    static QString propertyName(KTextEditor::CodeCompletionModel::CompletionProperty property);

    virtual void rowSelected(const QModelIndex& row);

    virtual bool indexIsCompletion(const QModelIndex& index) const;

    virtual int columnCount ( const QModelIndex & parent = QModelIndex() ) const;
    virtual QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
    virtual Qt::ItemFlags flags ( const QModelIndex & index ) const;
    virtual bool hasChildren ( const QModelIndex & parent = QModelIndex() ) const;
    virtual bool hasIndex ( int row, int column, const QModelIndex & parent = QModelIndex() ) const;
    virtual QModelIndex index ( int row, int column, const QModelIndex & parent = QModelIndex() ) const;

    // Disabled in case of bugs, reenable once fully debugged.
    //virtual QMap<int, QVariant> itemData ( const QModelIndex & index ) const;
    virtual QModelIndex parent ( const QModelIndex & index ) const;
    virtual int rowCount ( const QModelIndex & parent = QModelIndex() ) const;

    // Disabled in case of bugs, reenable once fully debugged.
    //virtual QModelIndex sibling ( int row, int column, const QModelIndex & index ) const;
    virtual void sort ( int column, Qt::SortOrder order = Qt::AscendingOrder );

    virtual QModelIndex mapToSource(const QModelIndex &proxyIndex) const;
    virtual QModelIndex mapFromSource(const QModelIndex &sourceIndex) const;

    // Sorting
    bool isSortingEnabled() const;
    bool isSortingAlphabetical() const;
    bool isSortingByInheritanceDepth() const;
    void setSortingByInheritanceDepth(bool byIneritance);
    void setSortingAlphabetical(bool alphabetical);

    Qt::CaseSensitivity sortingCaseSensitivity() const;
    void setSortingCaseSensitivity(Qt::CaseSensitivity cs);

    bool isSortingReverse() const;
    void setSortingReverse(bool reverse);

    // Filtering
    bool isFilteringEnabled() const;

    bool filterContextMatchesOnly() const;
    void setFilterContextMatchesOnly(bool filter);

    bool filterByAttribute() const;
    void setFilterByAttribute(bool filter);

    KTextEditor::CodeCompletionModel::CompletionProperties filterAttributes() const;
    void setFilterAttributes(KTextEditor::CodeCompletionModel::CompletionProperties attributes);

    // A maximum depth of <= 0 equals don't filter by inheritance depth (i.e. infinity) and is default
    int maximumInheritanceDepth() const;
    void setMaximumInheritanceDepth(int maxDepth);

    // Grouping
    bool isGroupingEnabled() const;

    enum gm {
      ScopeType     = 0x1,
      Scope         = 0x2,
      AccessType    = 0x4,
      ItemType      = 0x8
    };

    enum { //An own property that will be used to mark the best-matches group internally
      BestMatchesProperty = 2*KTextEditor::CodeCompletionModel::LastProperty
    };

    Q_DECLARE_FLAGS(GroupingMethods, gm)

    static const int ScopeTypeMask = 0x380000;
    static const int AccessTypeMask = 0x7;
    static const int ItemTypeMask = 0xfe0;

    GroupingMethods groupingMethod() const;
    void setGroupingMethod(GroupingMethods m);

    bool accessIncludeConst() const;
    void setAccessIncludeConst(bool include);
    bool accessIncludeStatic() const;
    void setAccessIncludeStatic(bool include);
    bool accessIncludeSignalSlot() const;
    void setAccessIncludeSignalSlot(bool include);

    // Column merging
    bool isColumnMergingEnabled() const;

    const QList< QList<int> >& columnMerges() const;
    void setColumnMerges(const QList< QList<int> >& columnMerges);

    void debugStats();

  protected:
    virtual int contextMatchQuality(const QModelIndex& row) const;

  Q_SIGNALS:
    void expandIndex(const QModelIndex& index);

  public Q_SLOTS:
    void setSortingEnabled(bool enable);
    void setFilteringEnabled(bool enable);
    void setGroupingEnabled(bool enable);
    void setColumnMergingEnabled(bool enable);

  private Q_SLOTS:
    void slotRowsInserted( const QModelIndex & parent, int start, int end );
    void slotRowsRemoved( const QModelIndex & parent, int start, int end );
    void slotModelReset();

  private:
    QTreeView* treeView() const;

    friend class KateArgumentHintModel;
    typedef QPair<KTextEditor::CodeCompletionModel*, int> ModelRow;
    ModelRow modelRowPair(const QModelIndex& index) const;

    // Represents a source row; provides sorting method
    class Item {
      public:
        Item(KateCompletionModel* model, ModelRow sourceRow);

        bool isValid() const;
        // Returns true if the item is not filtered and matches the current completion string
        bool isVisible() const;
        // Returns whether the item is filtered or not
        bool isFiltered() const;
        // Returns whether the item matches the current completion string
        bool isMatching() const;

        bool filter();
        bool match(const QString& newCompletion = QString());

        ModelRow sourceRow() const;

        // Sorting operator
        bool operator<(const Item& rhs) const;

      private:
        KateCompletionModel* model;
        ModelRow m_sourceRow;

        mutable QString m_completionSortingName;
        bool m_haveCompletionName;
        
        int inheritanceDepth;

        // True when currently matching completion string
        bool matchCompletion;
        // True when passes all active filters
        bool matchFilters;

        QString completionSortingName() const;
    };

    // Grouping and sorting of rows
    class Group {
      public:
        explicit Group(KateCompletionModel* model);

        void addItem(Item i);
        void resort();
        void refilter();
        void clear();
        //Returns whether this group should be ordered before other
        bool orderBefore(Group* other) const;
        //Returns a number that can be used for ordering
        int orderNumber() const;

        KateCompletionModel* model;
        int attribute;
        QString title, scope;
        QList<ModelRow> rows;
        QList<Item> prefilter;
        bool isEmpty;
    };

    void createGroups();
    void clearGroups();
    void hideOrShowGroup(Group* g);
    Group* fetchGroup(int attribute, const QString& scope = QString());
    //If this returns nonzero on an index, the index is the header of the returned group
    Group* groupForIndex(const QModelIndex& index) const;
    inline Group* groupOfParent(const QModelIndex& child) const { return static_cast<Group*>(child.internalPointer()); }
    QModelIndex indexForRow(Group* g, int row) const;
    QModelIndex indexForGroup(Group* g) const;

    enum changeTypes {
      Broaden,
      Narrow,
      Change
    };


    void changeCompletions(Group* g, const QString& newCompletion, changeTypes changeType);

    void deleteRows(Group* g, QMutableListIterator<ModelRow>& filtered, int countBackwards, int startRow);
    void addRows(Group* g, QMutableListIterator<ModelRow>& filtered, int startRow, const QList<ModelRow>& newItems);

    bool hasGroups() const;
    bool hasCompletionModel() const;

    /// Removes attributes not used in grouping from the input \a attribute
    int groupingAttributes(int attribute) const;
    int countBits(int value) const;

    void resort();
    void refilter();
    void rematch();

    //Updates the best-matches group
    void updateBestMatches();

    //Places all expanding-widgets to the correct positions in the viewport
    void placeExpandingWidgets();

    // ### Runtime state
    // General
    QList<KTextEditor::CodeCompletionModel*> m_completionModels;
    QString m_currentMatch;
    Qt::CaseSensitivity m_matchCaseSensitivity;

    // Column merging
    QList< QList<int> > m_columnMerges;

    Group* m_ungrouped;
    Group* m_argumentHints; //The argument-hints will be passed on to another model, to be shown in another widget
    Group* m_bestMatches; //A temporary group used for holding the best matches of all visible items

    // Storing the sorted order
    QList<Group*> m_rowTable;
    QList<Group*> m_emptyGroups;
    // Quick access to each specific group (if it exists)
    QMultiHash<int, Group*> m_groupHash;

    // ### Configurable state
    // Sorting
    bool m_sortingEnabled;
    bool m_sortingAlphabetical;
    bool m_isSortingByInheritance;
    Qt::CaseSensitivity m_sortingCaseSensitivity;
    bool m_sortingReverse;
    QHash< int, QList<int> > m_sortingGroupingOrder;

    // Filtering
    bool m_filteringEnabled;
    bool m_filterContextMatchesOnly;
    bool m_filterByAttribute;
    KTextEditor::CodeCompletionModel::CompletionProperties m_filterAttributes;
    int m_maximumInheritanceDepth;

    // Grouping
    bool m_groupingEnabled;
    GroupingMethods m_groupingMethod;
    bool m_accessConst, m_accessStatic, m_accesSignalSlot;

    // Column merging
    bool m_columnMergingEnabled;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(KateCompletionModel::GroupingMethods)

#endif
