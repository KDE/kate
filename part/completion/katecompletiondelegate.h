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

#ifndef KATECOMPLETIONDELEGATE_H
#define KATECOMPLETIONDELEGATE_H

#include <QtGui/QItemDelegate>
#include <QtGui/QTextLine>
#include <QModelIndex>
#include <QPoint>

class KateRenderer;
class KateCompletionWidget;
class KateDocument;
class KateTextLine;
class ExpandingWidgetModel;

class KateCompletionDelegate : public QItemDelegate
{
  Q_OBJECT

  public:
    explicit KateCompletionDelegate(ExpandingWidgetModel* model, KateCompletionWidget* parent = 0L);

    KateRenderer* renderer() const;
    KateCompletionWidget* widget() const;
    KateDocument* document() const;

    // Overridden to create highlighting for current index
    virtual void paint ( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const;

    // Returns the basic size-hint as reported by QItemDelegate
    QSize basicSizeHint( const QModelIndex& index ) const;
    
  protected:
    //column may be -1 if unknown
    void changeBackground( int row, int column, QStyleOptionViewItem & option ) const;
    virtual void drawDisplay ( QPainter * painter, const QStyleOptionViewItem & option, const QRect & rect, const QString & text ) const;
    virtual QSize sizeHint ( const QStyleOptionViewItem & option, const QModelIndex & index ) const;
    virtual bool editorEvent ( QEvent * event, QAbstractItemModel * model, const QStyleOptionViewItem & option, const QModelIndex & index );
  private:

    //Returns true if this delegate is painting in the list of completions, opposed to the list of argument-hints
    bool isCompletionDelegate() const;

    KateTextLine* m_previousLine;

    ExpandingWidgetModel* model() const;

    mutable int m_cachedRow;
    mutable bool m_cachedRowSelected;
    mutable int m_cachedColumnStart;
    mutable QList<int> m_cachedColumnStarts;
    mutable QList<QTextLayout::FormatRange> m_cachedHighlights;
    ExpandingWidgetModel* m_model;
};

#endif
