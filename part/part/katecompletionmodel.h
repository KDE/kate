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

#include <QAbstractProxyModel>
#include <QPair>
#include <QMutableListIterator>

class KateCompletionWidget;
class KateView;

namespace KTextEditor { class CodeCompletionModel; }

/**
 * This class has the responsibility for filtering, sorting, and manipulating
 * code completion data provided by a CodeCompletionModel.
 *
 * @author Hamish Rodda <rodda@kde.org>
 */
class KateCompletionModel : public QAbstractProxyModel
{
  Q_OBJECT

  public:
    KateCompletionModel(KateCompletionWidget* parent = 0L);

    KTextEditor::CodeCompletionModel* completionModel() const;

    KateView* view() const;

    virtual void setSourceModel( QAbstractItemModel* sourceModel );

    void setCurrentCompletion(const QString& completion);
    void setCaseSensitivity(Qt::CaseSensitivity cs);

    bool indexIsCompletion(const QModelIndex& index) const;

    virtual int columnCount ( const QModelIndex & parent = QModelIndex() ) const;
    virtual QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
    virtual Qt::ItemFlags flags ( const QModelIndex & index ) const;
    virtual bool hasChildren ( const QModelIndex & parent = QModelIndex() ) const;
    virtual bool hasIndex ( int row, int column, const QModelIndex & parent = QModelIndex() ) const;
    virtual QModelIndex index ( int row, int column, const QModelIndex & parent = QModelIndex() ) const;

    virtual QMap<int, QVariant> itemData ( const QModelIndex & index ) const;
    virtual QModelIndex parent ( const QModelIndex & index ) const;
    virtual int rowCount ( const QModelIndex & parent = QModelIndex() ) const;
    virtual QModelIndex sibling ( int row, int column, const QModelIndex & index ) const;
    virtual void sort ( int column, Qt::SortOrder order = Qt::AscendingOrder );

    virtual QModelIndex mapToSource(const QModelIndex &proxyIndex) const;
    virtual QModelIndex mapFromSource(const QModelIndex &sourceIndex) const;

  private slots:
    void slotRowsInserted( const QModelIndex & parent, int start, int end );
    void slotRowsRemoved( const QModelIndex & parent, int start, int end );

  private:
    // Grouping and sorting of rows
    struct Group {
      int attribute;
      QString title;
      QList<int> rows;
      QList<int> prefilter;
    };

    void createGroups();
    void clearGroups();
    Group* ungrouped();
    Group* fetchGroup(int attribute);
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

    void deleteRows(Group* g, QMutableListIterator<int>& filtered, int countBackwards, int startRow);
    void addRows(Group* g, QMutableListIterator<int>& filtered, int startRow, const QList<int>& newItems);

    bool hasGroups() const;
    bool hasCompletionModel() const;

    QString m_currentMatch;
    Qt::CaseSensitivity m_caseSensitive;

    bool m_hasCompletionModel;

    // Column merging
    int m_columnCount;
    QMap<int, QPair<int,int> > m_columnMerges;

    Group* m_ungrouped;
    bool m_ungroupedDisplayed;
    QList<Group*> m_rowTable;
};

#endif
