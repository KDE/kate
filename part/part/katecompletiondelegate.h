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

#include <QItemDelegate>
#include <QTextLayout>

class KateRenderer;
class KateCompletionWidget;
class KateDocument;
class KateTextLine;

class KateCompletionDelegate : public QItemDelegate
{
  Q_OBJECT

  public:
    KateCompletionDelegate(KateCompletionWidget* parent = 0L);

    KateRenderer* renderer() const;
    KateCompletionWidget* widget() const;
    KateDocument* document() const;

    // Overridden to create highlighting for current index
    virtual void paint ( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const;

  protected:
    virtual void drawDisplay ( QPainter * painter, const QStyleOptionViewItem & option, const QRect & rect, const QString & text ) const;

  private:
    KateTextLine* m_previousLine;

    mutable int m_cachedRow;
    mutable bool m_cachedRowSelected;
    mutable int m_cachedColumnStart;
    mutable QList<int> m_cachedColumnStarts;
    mutable QList<QTextLayout::FormatRange> m_cachedHighlights;
};

#endif
