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

#include "katecompletiondelegate.h"

#include <QTextLayout>
#include <QPainter>

#include "katerenderer.h"
#include "katetextline.h"
#include "katedocument.h"
#include "kateview.h"
#include "katehighlight.h"
#include "katerenderrange.h"
#include "katesmartrange.h"

#include "katecompletionwidget.h"
#include "katecompletionmodel.h"

KateCompletionDelegate::KateCompletionDelegate(KateCompletionWidget* parent)
  : QItemDelegate(parent)
  , m_previousLine(0L)
  , m_cachedRow(-1)
  , m_cachedRowSelected(false)
{
}

void KateCompletionDelegate::paint( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
  //kDebug() << "Painting row " << index.row() << ", column " << index.column() << ", internal " << index.internalPointer() << ", drawselected " << option.showDecorationSelected << ", selected " << (option.state & QStyle::State_Selected) << endl;
  if (index.row() != m_cachedRow || ( option.state & QStyle::State_Selected  ) != m_cachedRowSelected) {
    m_cachedColumnStarts.clear();
    m_cachedHighlights.clear();

    const KateCompletionModel* model = static_cast<const KateCompletionModel*>(index.model());
    if (!model->indexIsCompletion(index)) {
      m_cachedRow = -1;
      return QItemDelegate::paint(painter, option, index);
    }

    // Which highlighting to use?
#ifdef __GNUC__
#warning with KTextEditor::CodeCompletionModel::HighlightMethod it didnt compile with gcc 3.3.6, so I changed it to KTextEditor::CodeCompletionModel::HighlightingMethod, Alex
#endif
    QVariant highlight = model->data(model->index(index.row(), KTextEditor::CodeCompletionModel::Name, index.parent()), KTextEditor::CodeCompletionModel::HighlightingMethod);

    // TODO: config enable specifying no highlight as default
    int highlightMethod = KTextEditor::CodeCompletionModel::InternalHighlighting;
    if (highlight.canConvert(QVariant::Int))
      highlightMethod = highlight.toInt();

    KTextEditor::Cursor completionStart = widget()->completionRange()->start();

    QString startText = document()->text(KTextEditor::Range(completionStart.line(), 0, completionStart.line(), completionStart.column()));

    KateTextLine::Ptr thisLine(new KateTextLine());
    thisLine->insertText(0, startText);

    int len = completionStart.column();
    for (int i = 0; i < KTextEditor::CodeCompletionModel::ColumnCount; ++i) {
      m_cachedColumnStarts.append(len);
      QString text = model->data(model->index(index.row(), i, index.parent()), Qt::DisplayRole).toString();
      thisLine->insertText(thisLine->length(), text);
      len += text.length();
    }

    //kDebug() << k_funcinfo << "About to highlight with mode " << highlightMethod << " text [" << thisLine->string() << "]" << endl;

    if (highlightMethod & KTextEditor::CodeCompletionModel::InternalHighlighting) {
      KateTextLine::Ptr previousLine;
      if (completionStart.line())
        previousLine = document()->kateTextLine(completionStart.line() - 1);
      else
        previousLine = new KateTextLine();

      document()->highlight()->doHighlight(previousLine.data(), thisLine.data(), 0L, 0L);
    }

    NormalRenderRange rr;
    if (highlightMethod & KTextEditor::CodeCompletionModel::CustomHighlighting) {
      QList<QVariant> customHighlights = model->data(model->index(index.row(), KTextEditor::CodeCompletionModel::Name, index.parent()), KTextEditor::CodeCompletionModel::CustomHighlight).toList();

      for (int i = 0; i + 2 < customHighlights.count(); i += 3) {
        if (!customHighlights[i].canConvert(QVariant::Int) || !customHighlights[i+1].canConvert(QVariant::Int) || !customHighlights[i+2].canConvert<void*>()) {
          kWarning() << k_funcinfo << "Unable to convert triple to custom formatting." << endl;
          continue;
        }

        rr.addRange(new KTextEditor::Range(completionStart.start() + KTextEditor::Cursor(0, customHighlights[i].toInt()), completionStart.start() + KTextEditor::Cursor(0, customHighlights[i+1].toInt())), static_cast<KTextEditor::Attribute*>(customHighlights[i+2].value<void*>()));
      }
    }

    m_cachedHighlights = renderer()->decorationsForLine(thisLine, 0, false, &rr, option.state & QStyle::State_Selected);

    /*kDebug() << k_funcinfo << "Highlights for line [" << thisLine->string() << "]:" << endl;
    foreach (const QTextLayout::FormatRange& fr, m_cachedHighlights)
      kDebug() << k_funcinfo << fr.start << " len " << fr.length << " format " << endl;*/

    m_cachedRow = index.row();
    m_cachedRowSelected = option.state & QStyle::State_Selected;
  }

  m_cachedColumnStart = m_cachedColumnStarts[index.column()];

  QItemDelegate::paint(painter, option, index);
}

void KateCompletionDelegate::drawDisplay( QPainter * painter, const QStyleOptionViewItem & option, const QRect & rect, const QString & text ) const
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
    additionalFormats.append(format);
  }

  /*kDebug() << k_funcinfo << "Highlights for text [" << text << "] col start " << m_cachedColumnStart << ":" << endl;
  foreach (const QTextLayout::FormatRange& fr, m_cachedHighlights)
    kDebug() << k_funcinfo << fr.start << " len " << fr.length << " format " << fr.format.fontWeight() << endl;*/

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

KateRenderer * KateCompletionDelegate::renderer( ) const
{
  return widget()->view()->renderer();
}

KateCompletionWidget * KateCompletionDelegate::widget( ) const
{
  return static_cast<KateCompletionWidget*>(const_cast<QObject*>(parent()));
}

KateDocument * KateCompletionDelegate::document( ) const
{
  return widget()->view()->doc();
}

#include "katecompletiondelegate.moc"
