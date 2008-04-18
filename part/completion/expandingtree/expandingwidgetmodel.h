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
#include <QPersistentModelIndex>
#include <QPointer>

class KWidget;
class QTreeView;
class QTextEdit;

/**
 * Cares about expanding/un-expanding items in a tree-view together with ExpandingDelegate
 */
class ExpandingWidgetModel : public QAbstractTableModel {
    Q_OBJECT
    public:
        
    ExpandingWidgetModel( QWidget* parent );
    virtual ~ExpandingWidgetModel();
    
    enum ExpandingType {
      NotExpandable=0,
      Expandable,
      Expanded
    };

    ///The following three are convenience-functions for the current item that could be replaced by the later ones
    ///@return whether the current item can be expanded
    bool canExpandCurrentItem() const;
    ///@return whether the current item can be collapsed
    bool canCollapseCurrentItem() const;
    ///Expand/collapse the current item
    void setCurrentItemExpanded( bool );

    void clearMatchQualities();
    
    ///Unexpand all rows and clear all cached information about them(this includes deleting the expanding-widgets)
    void clearExpanding();
    
    ///@return whether the row given through index is expandable
    bool isExpandable(const QModelIndex& index) const;

    enum ExpansionType {
        NotExpanded = 0,
        ExpandDownwards, //The additional(expanded) information is shown UNDER the original information
        ExpandUpwards //The additional(expanded) information is shown ABOVE the original information
    };
    
    ///Returns whether the given index is currently partially expanded. Does not do any other checks like calling models for data.
    ExpansionType isPartiallyExpanded(const QModelIndex& index) const;

    ///@return whether row is currently expanded
    bool isExpanded(const QModelIndex & row) const;
    ///Change the expand-state of the row given through index. The display will be updated.
    void setExpanded(QModelIndex index, bool expanded);

    ///Returns the total height added through all open expanding-widgets
    int expandingWidgetsHeight() const;
    
    ///@return the expanding-widget for the given row, if available. Expanding-widgets are in best case available for all expanded rows.
    ///This does not return the partially-expand widget.
    QWidget* expandingWidget(const QModelIndex & row) const;

    ///Amount by which the height of a row increases when it is partially expanded
    int partiallyExpandWidgetHeight() const;
    /**
     * Notifies underlying models that the item was selected, collapses any previous partially expanded line,
     * checks whether this line should be partially expanded, and eventually does it.
     * Does nothing when nothing needs to be done.
     * Does NOT show the expanding-widget. That is done immediately when painting by ExpandingDelegate,
     * to reduce flickering. @see showPartialExpandWidget()
     * @param row The row
     * */
    ///
    virtual void rowSelected(const QModelIndex & row);

    ///Returns the rectangle for the partially expanded part of the given row
    QRect partialExpandRect(const QModelIndex & row) const;

    QString partialExpandText(const QModelIndex & row) const;
    
    ///Places and shows the expanding-widget for the given row, if it should be visible and is valid.
    ///Also shows the partial-expanding-widget when it should be visible.
    void placeExpandingWidget(const QModelIndex & row);
    
    virtual QTreeView* treeView() const = 0;
    
    ///Should return true if the given row should be painted like a contained item(as opposed to label-rows etc.)
    virtual bool indexIsItem(const QModelIndex& index) const = 0;

    ///Does not request data from index, this only returns local data like highlighting for expanded rows and similar
    virtual QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;

    ///Returns the first row that is currently partially expanded.
    QModelIndex partiallyExpandedRow() const;

    public slots:
    ///Place or hides all expanding-widgets to the correct positions. Should be called after the view was scrolled.
    void placeExpandingWidgets();

    protected:
    /**
     * @return the context-match quality from 0 to 10 if it could be determined, else -1
     * */
    virtual int contextMatchQuality(const QModelIndex & index) const = 0;

    ///Returns the match-color for the given index, or zero if match-quality could not be computed.
    uint matchColor(const QModelIndex& index) const;
    
    //Makes sure m_expandedIcon and m_collapsedIcon are loaded
    void cacheIcons() const;
    
    static QIcon m_expandedIcon;
    static QIcon m_collapsedIcon;
    
    //Does not update the view
    void partiallyUnExpand(const QModelIndex& index);
    //Finds out the basic height of the row represented by the given index. Basic means without respecting any expansion.
    int basicRowHeight( const QModelIndex& index ) const;
    
    private:
    QMap<QPersistentModelIndex, ExpansionType> m_partiallyExpanded;
    // Store expanding-widgets and cache whether items can be expanded
    mutable QMap<QPersistentModelIndex, ExpandingType> m_expandState;
    QMap< QPersistentModelIndex, QPointer<QWidget> > m_expandingWidgets; //Map rows to their expanding-widgets
    QMap< QPersistentModelIndex, int > m_contextMatchQualities; //Map rows to their context-match qualities(undefined if unknown, else 0 to 10). Not used yet, eventually remove.
};


/**
 * It is assumed that between each two strings, one space is inserted.
 * Helper-function to merge custom-highlighting variant-lists.
 *
 * @param stings A list of strings that should be merged
 * @param highlights One variant-list for highlighting, as described in the kde header ktextedtor/codecompletionmodel.h
 * @param gapBetweenStrings How many signs are inserted between 2 strings?
 * */
QList<QVariant> mergeCustomHighlighting( QStringList strings, QList<QVariantList> highlights, int gapBetweenStrings = 1 );
#endif
