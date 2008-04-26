
/* This file is part of the KDE libraries
   Copyright (C) 2006 Hamish Rodda <rodda@kde.org>

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

#ifndef ExpandingDelegate_H
#define ExpandingDelegate_H

#include <QtGui/QItemDelegate>
#include <QtGui/QTextLine>
#include <QModelIndex>
#include <QPoint>

class KateRenderer;
class KateCompletionWidget;
class KateDocument;
class KateTextLine;
class ExpandingWidgetModel;
class QVariant;

/**
 * This is a delegate that cares, together with ExpandingWidgetModel, about embedded widgets in tree-view.
 * */

class ExpandingDelegate : public QItemDelegate
{
  Q_OBJECT

  public:
    explicit ExpandingDelegate(ExpandingWidgetModel* model, QObject* parent = 0L);


    // Overridden to create highlighting for current index
    virtual void paint ( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const;

    // Returns the basic size-hint as reported by QItemDelegate
    QSize basicSizeHint( const QModelIndex& index ) const;
    
    ExpandingWidgetModel* model() const;
  protected:
    //Called right before paint to allow last-minute changes to the style
    virtual void adjustStyle( const QModelIndex& index, QStyleOptionViewItem & option ) const;
    virtual void drawDisplay ( QPainter * painter, const QStyleOptionViewItem & option, const QRect & rect, const QString & text ) const;
    virtual QSize sizeHint ( const QStyleOptionViewItem & option, const QModelIndex & index ) const;
    virtual bool editorEvent ( QEvent * event, QAbstractItemModel * model, const QStyleOptionViewItem & option, const QModelIndex & index );

    //option can be changed
    virtual QList<QTextLayout::FormatRange> createHighlighting(const QModelIndex& index, QStyleOptionViewItem& option) const;

    /**
     * Creates a list of FormatRanges as should be returned by createHighlighting from a list of QVariants as described in the kde header ktexteditor/codecompletionmodel.h
     * */
    QList<QTextLayout::FormatRange> highlightingFromVariantList(const QList<QVariant>& customHighlights) const;
    
    //Called when an item was expanded/unexpanded and the height changed
    virtual void heightChanged() const;
  
    mutable int m_currentColumnStart; //Text-offset for custom highlighting, will be applied to m_cachedHighlights(Only highlights starting after this will be used). Shoult be zero of the highlighting is not taken from kate.
    mutable QList<int> m_currentColumnStarts;
    mutable QList<QTextLayout::FormatRange> m_cachedHighlights;
  
    mutable Qt::Alignment m_cachedAlignment;
  private:

    ExpandingWidgetModel* m_model;
};

#endif
