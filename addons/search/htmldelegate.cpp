/***************************************************************************
 *   This file is part of Kate search plugin                               *
 *   Copyright 2011 Kåre Särs <kare.sars@iki.fi>                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "htmldelegate.h"

#include <QtGui/QPainter>
#include <QtCore/QModelIndex>
#include <QtGui/QTextDocument>
#include <QtGui/QAbstractTextDocumentLayout>
#include <QtGui/QTextCharFormat>
#include <KLocalizedString>

SPHtmlDelegate::SPHtmlDelegate( QObject* parent )
: QStyledItemDelegate(parent)
{}

SPHtmlDelegate::~SPHtmlDelegate() {}

void SPHtmlDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{ 
    QStyleOptionViewItem options = option;
    initStyleOption(&options, index);

    QTextDocument doc;
    //doc.setDocumentMargin(0);
    doc.setHtml(index.data().toString());

    painter->save();
    options.text = QString();  // clear old text
    options.widget->style()->drawControl(QStyle::CE_ItemViewItem, &options, painter, options.widget);

    // draw area
    QRect clip = options.widget->style()->subElementRect(QStyle::SE_ItemViewItemText, &options);
    QFontMetrics metrics(options.font);
    if (index.flags() == Qt::NoItemFlags) {
        painter->setBrush(QBrush(QWidget().palette().color(QPalette::Base)));
        painter->setPen(QWidget().palette().color(QPalette::Base));
        painter->drawRect(QRect(clip.topLeft() - QPoint(20, metrics.descent()), clip.bottomRight()));
        painter->translate(clip.topLeft() - QPoint(20, metrics.descent()));
    }
    else {
        painter->translate(clip.topLeft() - QPoint(0, metrics.descent()));
    }
    QAbstractTextDocumentLayout::PaintContext pcontext;
    doc.documentLayout()->draw(painter, pcontext);

    painter->restore();
}

QSize SPHtmlDelegate::sizeHint(const QStyleOptionViewItem& /*option*/, const QModelIndex& index) const
{
    QTextDocument doc;
    //doc.setDocumentMargin(0);
    doc.setHtml(index.data().toString());
    //qDebug() << doc.toPlainText() << doc.size().toSize();
    return doc.size().toSize() + QSize(30, 0); // add margin for the check-box
}
