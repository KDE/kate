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

#include "expandingtree.h"

#include <QTextLayout>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <QPainter>
#include <kdebug.h>
#include "expandingwidgetmodel.h"

ExpandingTree::ExpandingTree(QWidget* parent) : QTreeView(parent) {
  m_drawText.documentLayout()->setPaintDevice(this);
  setUniformRowHeights(false);
}

void ExpandingTree::drawRow ( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const {
  QTreeView::drawRow( painter, option, index );

  const ExpandingWidgetModel* eModel = qobject_cast<const ExpandingWidgetModel*>(model());
  if( eModel && eModel->isPartiallyExpanded( index ) )
  {
    QRect rect = eModel->partialExpandRect( index );
    if( rect.isValid() )
    {
      painter->fillRect(rect,QBrush(0xffffffff));

      QAbstractTextDocumentLayout::PaintContext ctx;
      ctx.clip = QRectF(0,0,rect.width(),rect.height());;
      painter->setViewTransformEnabled(true);
      painter->translate(rect.left(), rect.top());
      
      m_drawText.setHtml( eModel->partialExpandText( index ) );
      m_drawText.setPageSize(QSizeF(rect.width(), rect.height()));
      m_drawText.documentLayout()->draw( painter, ctx );
    
      painter->translate(-rect.left(), -rect.top());
    }
  }
}

