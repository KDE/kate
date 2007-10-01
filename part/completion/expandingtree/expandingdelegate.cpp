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
#include "expandingdelegate.h"

#include <QtGui/QTextLine>
#include <QtGui/QPainter>
#include <QtGui/QBrush>
#include <QKeyEvent>

#include "expandingwidgetmodel.h"

ExpandingDelegate::ExpandingDelegate(ExpandingWidgetModel* model, QObject* parent)
  : QItemDelegate(parent)
  , m_cachedRow(-1)
  , m_cachedRowSelected(false)
  , m_model(model)
{
}

void ExpandingDelegate::paint( QPainter * painter, const QStyleOptionViewItem & optionOld, const QModelIndex & index ) const
{
  QStyleOptionViewItem option(optionOld);

  changeBackground(index.row(), index.column(), option);
    
  if( index.column() == 0 )
    model()->placeExpandingWidget(index);

  //Make sure the decorations are painted at the top, because the center of expanded items will be filled with the embedded widget.
  option.decorationAlignment = Qt::AlignTop | option.decorationAlignment;
  
  //kDebug() << "Painting row " << index.row() << ", column " << index.column() << ", internal " << index.internalPointer() << ", drawselected " << option.showDecorationSelected << ", selected " << (option.state & QStyle::State_Selected);

  if ((index.row() != m_cachedRow) || ( ( option.state & QStyle::State_Selected  ) == QStyle::State_Selected ) != m_cachedRowSelected) {
    m_cachedColumnStarts.clear();
    m_cachedHighlights.clear();

    if (!model()->indexIsItem(index) ) {
      m_cachedRow = -1;
      return QItemDelegate::paint(painter, option, index);
    }

    m_cachedHighlights = createHighlighting(index, option);

    /*kDebug() << "Highlights for line [" << thisLine->string() << "]:";
    foreach (const QTextLayout::FormatRange& fr, m_cachedHighlights)
      kDebug() << fr.start << " len " << fr.length << " format ";*/

    m_cachedRow = index.row();
    m_cachedRowSelected = option.state & QStyle::State_Selected;
  }

  m_cachedColumnStart = m_cachedColumnStarts[index.column()];

  QItemDelegate::paint(painter, option, index);
}

QList<QTextLayout::FormatRange> ExpandingDelegate::createHighlighting(const QModelIndex& index, QStyleOptionViewItem& option) const {
  return QList<QTextLayout::FormatRange>();
}

QSize ExpandingDelegate::basicSizeHint( const QModelIndex& index ) const {
  return QItemDelegate::sizeHint( QStyleOptionViewItem(), index );
}

QSize ExpandingDelegate::sizeHint ( const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
  QSize s = QItemDelegate::sizeHint( option, index );
  if( model()->isExpanded(index) && model()->expandingWidget( index ) )
  {
    QWidget* widget = model()->expandingWidget( index );
    QSize widgetSize = widget->size();

    s.setHeight( widgetSize.height() + s.height() + 10 ); //10 is the sum that must match exactly the offsets used in ExpandingWidgetModel::placeExpandingWidgets
  } else if( model()->isPartiallyExpanded( index ) ) {
    s.setHeight( s.height() + 30 + 10 );
  }
  return s;
}

void ExpandingDelegate::changeBackground( int row, int column, QStyleOptionViewItem & option ) const {
    //Highlight selected items specially
    QColor highlight = option.palette.color( option.palette.currentColorGroup(), QPalette::Highlight );

    highlight.setRgb(0x45aed0);

    for(int a = 0; a <=2; a++ )
      option.palette.setColor( (QPalette::ColorGroup)a, QPalette::Highlight, highlight );
}

void ExpandingDelegate::drawDisplay( QPainter * painter, const QStyleOptionViewItem & option, const QRect & rect, const QString & text ) const
{
  if (m_cachedRow == -1)
    return QItemDelegate::drawDisplay(painter, option, rect, text);

  QTextLayout layout(text, option.font, painter->device());

  QRect textRect = rect.adjusted(1, 0, -1, 0); // remove width padding

  QList<QTextLayout::FormatRange> additionalFormats;

  for (int i = 0; i < m_cachedHighlights.count(); ++i) {
    if (m_cachedHighlights[i].start + m_cachedHighlights[i].length <= m_cachedColumnStart)
      continue;

    if (!additionalFormats.count())
      if (i != 0 && m_cachedHighlights[i - 1].start + m_cachedHighlights[i - 1].length > m_cachedColumnStart) {
        QTextLayout::FormatRange before;
        before.start = 0;
        before.length = m_cachedHighlights[i - 1].start + m_cachedHighlights[i - 1].length - m_cachedColumnStart;
        before.format = m_cachedHighlights[i - 1].format;
        additionalFormats.append(before);
      }

    QTextLayout::FormatRange format;
    format.start = m_cachedHighlights[i].start - m_cachedColumnStart;
    format.length = m_cachedHighlights[i].length;
    format.format = m_cachedHighlights[i].format;
    format.format.setBackground(QBrush(Qt::NoBrush)); //Make sure that our own background-color is preserved Maybe remove this
    additionalFormats.append(format);
  }

  /*kDebug() << "Highlights for text [" << text << "] col start " << m_cachedColumnStart << ":";
  foreach (const QTextLayout::FormatRange& fr, m_cachedHighlights)
    kDebug() << fr.start << " len " << fr.length << " format " << fr.format.fontWeight();*/

  layout.setAdditionalFormats(additionalFormats);

  QTextOption to;
  to.setAlignment(option.displayAlignment);
  to.setWrapMode(QTextOption::WrapAnywhere);
  layout.setTextOption(to);

  layout.beginLayout();
  QTextLine line = layout.createLine();
  line.setLineWidth(rect.width());
  layout.endLayout();

  layout.draw(painter, rect.topLeft());

  return;

  //if (painter->fontMetrics().width(text) > textRect.width() && !text.contains(QLatin1Char('\n')))
      //str = elidedText(option.fontMetrics, textRect.width(), option.textElideMode, text);
  //qt_format_text(option.font, textRect, option.displayAlignment, str, 0, 0, 0, 0, painter);
}

ExpandingWidgetModel* ExpandingDelegate::model() const {
  return m_model;
}

void ExpandingDelegate::heightChanged() const {
}

bool ExpandingDelegate::editorEvent ( QEvent * event, QAbstractItemModel * /*model*/, const QStyleOptionViewItem & /*option*/, const QModelIndex & index )
{
  QKeyEvent* keyEvent = 0;
  if( event->type() == QEvent::KeyPress )
    keyEvent = reinterpret_cast<QKeyEvent*>(event);
  
  if( event->type() == QEvent::MouseButtonRelease )
  {
    event->accept();
    model()->setExpanded(index, !model()->isExpanded( index ));
    heightChanged();

    return true;
  } else {
    event->ignore();
  }
  
  return false;
}

#include "expandingdelegate.moc"
