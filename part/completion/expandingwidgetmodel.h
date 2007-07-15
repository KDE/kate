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
#ifndef EXPANDING_WIDGET_MODEL_H
#define EXPANDING_WIDGET_MODEL_H

#include <QAbstractTableModel>
#include <QtCore/QHash>
#include <QIcon>

class KWidget;
class QTreeView;
class QTextEdit;

/**
 * Cares about expanding/un-expanding items in a tree-view together with KateCompletionDelegate
 */
class ExpandingWidgetModel : public QAbstractTableModel {
    Q_OBJECT;
    public:
        
    ExpandingWidgetModel( QWidget* parent );
    virtual ~ExpandingWidgetModel();
    
    enum ExpandingType {
      NotExpandable=0,
      Expandable,
      Expanded
    };
    
    void clearExpanding();
    
    // Expanding
    bool isExpandable(const QModelIndex& index) const;

    bool canExpandCurrentItem() const;
    bool canCollapseCurrentItem() const;
    void setCurrentItemExpanded( bool );

    ///Returns whether the given row is currently partially expanded. Does not do any other checks like calling models for data.
    bool isPartiallyExpanded(int row) const;
    
    ///@return whether row is currently expanded
    bool isExpanded(int row) const;
    ///@return change the expand-state of the row given through index. The display will be updated.
    void setExpanded(QModelIndex index, bool expanded);

    ///@return the expanding-widget for the given row, if available. Expanding-widgets are in best case available for all expanded rows.
    ///This does not return the partially-expand widget.
    QWidget* expandingWidget(int row) const;

    int partiallyExpandWidgetHeight() const;
    /**
     * Notifies underlying models that the item was selected, collapses any previous partially expanded line,
     * checks whether this line should be partially expanded, and eventually does it.
     * Does nothing when nothing needs to be done.
     * Does NOT show the expanding-widget. That must be done immediately when painting by KateCompletionDelegate,
     * to reduce flickering. @see showPartialExpandWidget()
     * @param row The row
     * */
    ///
    void rowSelected(int row);

    ///Places and shows the expanding-widget for the given row, if it should be visible and is valid.
    ///Also shows the partial-expanding-widget when it should be visible.
    void placeExpandingWidget(int row);
    
    ///Place all expanding-widgets to the correct positions
    void placeExpandingWidgets();
    
    virtual QTreeView* treeView() const = 0;
    
    ///Should return true if the given row should be painted like a completion-item
    virtual bool indexIsCompletion(const QModelIndex& index) const = 0;
    
    protected:
    //Makes sure m_expandedIcon and m_collapsedIcon are loaded
    void cacheIcons() const;
    
    static QIcon m_expandedIcon;
    static QIcon m_collapsedIcon;
    
    //Does not update the view
    void partiallyUnExpand(int row);
    //Finds out the basic height of the row represented by the given index. Basic means without respecting any expansion.
    int basicRowHeight( const QModelIndex& index ) const;
    
    private:
    // Store expanding-widgets and cache whether items can be expanded
    mutable QHash<int, ExpandingType> m_expandState;
    QHash< int, QWidget* > m_expandingWidgets; //Map row-numbers to their expanding-widgets
    int m_partiallyExpandedRow;
    QTextEdit* m_partiallyExpandWidget; ///@todo instead of embedding this widget, use QTextDocument to paint the content in KateCompletionDelegate
};

#endif
