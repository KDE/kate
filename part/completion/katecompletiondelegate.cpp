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
    ExpandingDelegate(model, parent)
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

    NormalRenderRange rr;
    if (highlightMethod & KTextEditor::CodeCompletionModel::CustomHighlighting) {
      QList<QVariant> customHighlights = model()->data(model()->index(index.row(), KTextEditor::CodeCompletionModel::Name, index.parent()), KTextEditor::CodeCompletionModel::CustomHighlight).toList();

      for (int i = 0; i + 2 < customHighlights.count(); i += 3) {
        if (!customHighlights[i].canConvert(QVariant::Int) || !customHighlights[i+1].canConvert(QVariant::Int) || !customHighlights[i+2].canConvert<void*>()) {
          kWarning() << "Unable to convert triple to custom formatting.";
          continue;
        }

        rr.addRange(new KTextEditor::Range(completionStart.start() + KTextEditor::Cursor(0, customHighlights[i].toInt()), completionStart.start() + KTextEditor::Cursor(0, customHighlights[i+1].toInt())), KTextEditor::Attribute::Ptr(static_cast<KTextEditor::Attribute*>(customHighlights[i+2].value<void*>())));
      }
    }
  
  return renderer()->decorationsForLine(thisLine, 0, false, &rr, option.state & QStyle::State_Selected);
}
