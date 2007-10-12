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

#include "katecompletiondelegate.h"

#include <ktexteditor/codecompletionmodel.h>

#include "katerenderer.h"
#include "katetextline.h"
#include "katedocument.h"
#include "kateview.h"
#include "katehighlight.h"
#include "katerenderrange.h"
#include "katesmartrange.h"

#include "katecompletionwidget.h"
#include "katecompletionmodel.h"
#include "katecompletiontree.h"

KateCompletionDelegate::KateCompletionDelegate(ExpandingWidgetModel* model, KateCompletionWidget* parent) :
    ExpandingDelegate(model, parent), m_cachedRow(-1)
{
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

void KateCompletionDelegate::heightChanged() const {
  if(parent())
    widget()->updateHeight();
}

QList<QTextLayout::FormatRange> KateCompletionDelegate::createHighlighting(const QModelIndex& index, QStyleOptionViewItem& option) const {
    // Which highlighting to use?
  //model()->index(index.row(), KTextEditor::CodeCompletionModel::Name, index.parent())
    if( index.row() == m_cachedRow ) {
        if( index.column() < m_cachedColumnStarts.size() ) {
            m_currentColumnStart = m_cachedColumnStarts[index.column()];
        } else {
            kWarning() << "Column-count does not match";
        }
        return m_cachedHighlights;
    }
    
    ///@todo reset the cache when the model changed
    m_cachedRow = index.row();
    
    QVariant highlight = model()->data(index, KTextEditor::CodeCompletionModel::HighlightingMethod);

    // TODO: config enable specifying no highlight as default
    int highlightMethod = KTextEditor::CodeCompletionModel::InternalHighlighting;
    if (highlight.canConvert(QVariant::Int))
      highlightMethod = highlight.toInt();
    

    KTextEditor::Cursor completionStart = widget()->completionRange()->start();

    QString startText = document()->text(KTextEditor::Range(completionStart.line(), 0, completionStart.line(), completionStart.column()));

    KateTextLine::Ptr thisLine(new KateTextLine());
    thisLine->insertText(0, startText);

    int len = completionStart.column();
    m_cachedColumnStarts.clear();
    for (int i = 0; i < KTextEditor::CodeCompletionModel::ColumnCount; ++i) {
      m_cachedColumnStarts.append(len);
      QString text = model()->data(model()->index(index.row(), i, index.parent()), Qt::DisplayRole).toString();
      thisLine->insertText(thisLine->length(), text);
      len += text.length();
    }

    //kDebug() << "About to highlight with mode " << highlightMethod << " text [" << thisLine->string() << "]";

    if (highlightMethod & KTextEditor::CodeCompletionModel::InternalHighlighting) {
      KateTextLine::Ptr previousLine;
      if (completionStart.line())
        previousLine = document()->kateTextLine(completionStart.line() - 1);
      else
        previousLine = new KateTextLine();

      QVector<int> foldingList;
      bool ctxChanged = false;
      document()->highlight()->doHighlight(previousLine.data(), thisLine.data(), foldingList, ctxChanged);
    }

  if (highlightMethod & KTextEditor::CodeCompletionModel::CustomHighlighting)
    return highlightingFromVariantList(model()->data(index, KTextEditor::CodeCompletionModel::CustomHighlight).toList());

  m_currentColumnStart = m_cachedColumnStarts[index.column()];
  
  NormalRenderRange rr;
  QList<QTextLayout::FormatRange> ret = renderer()->decorationsForLine(thisLine, 0, false, &rr, option.state & QStyle::State_Selected);

  //Remove background-colors
  for( QList<QTextLayout::FormatRange>::iterator it = ret.begin(); it != ret.end(); ++it )
    (*it).format.clearBackground();
  
  return ret;
}


