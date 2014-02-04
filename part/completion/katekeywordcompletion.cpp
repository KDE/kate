/* This file is part of the KDE libraries
   Copyright (C) 2014 Sven Brauch <svenbrauch@gmail.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2, or any later version,
   as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "katekeywordcompletion.h"

#include "katehighlight.h"
#include "katehighlighthelpers.h"
#include "katedocument.h"
#include "katetextline.h"

#include <KLocalizedString>

KateKeywordCompletionModel::KateKeywordCompletionModel(QObject* parent)
    : CodeCompletionModel2(parent)
{
    setHasGroups(false);
}

void KateKeywordCompletionModel::completionInvoked(KTextEditor::View* view, const KTextEditor::Range& range,
                                                   KTextEditor::CodeCompletionModel::InvocationType /*invocationType*/)
{
    KateDocument* doc = static_cast<KateDocument*>(view->document());
    if ( ! doc->highlight() || doc->highlight()->noHighlighting() ) {
        // no highlighting -- nothing to do
        return;
    }

    Kate::TextLine line = doc->kateTextLine(range.end().line());
    Kate::TextLine previousLine = doc->kateTextLine(range.end().line() - 1);
    Kate::TextLine nextLine = doc->kateTextLine(range.end().line() + 1);
    bool contextChanged;
    QVector<KateHighlighting::ContextChange> contextChanges;
    // Ask the highlighting engine to re-calcualte the highlighting for the line
    // where completion was invoked, and store all the context changes in the process.
    doc->highlight()->doHighlight(previousLine.data(), line.data(), nextLine.data(),
                                  contextChanged, 0, &contextChanges);

    // From the list of context changes, find the highlighting context which is
    // active at the position where completion was invoked.
    KateHlContext* context = 0;
    foreach ( const KateHighlighting::ContextChange& change, contextChanges ) {
        if ( change.pos == 0 || change.pos <= range.end().column() ) {
            context = change.toContext;
        }
        if ( change.pos > range.end().column() ) {
            break;
        }
    }

    // Find all keyword items which exist for that context,
    // and suggest them as completion items.
    QSet<QString> items;
    if ( ! context ) {
        // default context
        context = doc->highlight()->contextNum(0);
    }
    foreach ( const KateHlItem* item, context->items ) {
        if ( const KateHlKeyword* kw = dynamic_cast<const KateHlKeyword*>(item) ) {
            items.unite(kw->allKeywords());
        }
    }
    m_items = items.toList();
    qSort(m_items);
}

QModelIndex KateKeywordCompletionModel::parent(const QModelIndex& index) const
{
    if ( index.internalId() )
        return createIndex(0, 0, 0);
    else
        return QModelIndex();
}

QModelIndex KateKeywordCompletionModel::index(int row, int column, const QModelIndex& parent) const
{
    if ( !parent.isValid() ) {
        if ( row == 0 )
            return createIndex(row, column, 0);
        else
            return QModelIndex();
    } else if ( parent.parent().isValid() ) {
        return QModelIndex();
    }

    if ( row < 0 || row >= m_items.count() || column < 0 || column >= ColumnCount ) {
        return QModelIndex();
    }

    return createIndex(row, column, 1);
}

int KateKeywordCompletionModel::rowCount(const QModelIndex& parent) const
{
    if( !parent.isValid() && !m_items.isEmpty() )
        return 1; //One root node to define the custom group
    else if(parent.parent().isValid())
        return 0; //Completion-items have no children
    else
        return m_items.count();
}

static bool isInWord(const KTextEditor::View* view, const KTextEditor::Cursor& position, QChar c)
{
    KateDocument* document = static_cast<KateDocument*>(view->document());
    KateHighlighting* highlight = document->highlight();
    Kate::TextLine line = document->kateTextLine(position.line());
    return highlight->isInWord(c, line->attribute(position.column()-1));
};

KTextEditor::Range KateKeywordCompletionModel::completionRange(KTextEditor::View* view,
                                                               const KTextEditor::Cursor& position)
{
    const QString& text = view->document()->text(KTextEditor::Range(position, KTextEditor::Cursor(position.line(), 0)));
    int pos;
    for ( pos = text.size() - 1; pos >= 0; pos-- ) {
        if ( isInWord(view, position, text.at(pos)) ) {
            // This needs to be aware of what characters are word-characters in the
            // active language, so that languages which prefix commands with e.g. @
            // or \ have properly working completion.
            continue;
        }
        break;
    }
    return KTextEditor::Range(KTextEditor::Cursor(position.line(), pos + 1), position);
}

bool KateKeywordCompletionModel::shouldAbortCompletion(KTextEditor::View* view, const KTextEditor::Range& range,
                                                       const QString& currentCompletion)
{
    if ( view->cursorPosition() < range.start() || view->cursorPosition() > range.end() )
      return true; // Always abort when the completion-range has been left
    // Do not abort completions when the text has been empty already before and a newline has been entered

    foreach ( QChar c, currentCompletion ) {
        if ( ! isInWord(view, range.start(), c) ) {
            return true;
        }
    }
    return false;
}

bool KateKeywordCompletionModel::shouldStartCompletion(KTextEditor::View* /*view*/, const QString& insertedText,
                                                       bool userInsertion, const KTextEditor::Cursor& /*position*/)
{
    if ( userInsertion && insertedText.size() > 3 && ! insertedText.contains(' ')
         && insertedText.at(insertedText.size()-1).isLetter() ) {
        return true;
    }
    return false;
}

bool KateKeywordCompletionModel::shouldHideItemsWithEqualNames() const
{
    return true;
}

QVariant KateKeywordCompletionModel::data(const QModelIndex& index, int role) const
{
    if ( role == UnimportantItemRole )
        return QVariant(true);
    if ( role == InheritanceDepth )
        return 9000;

    if ( !index.parent().isValid() ) {
        // group header
        switch ( role ) {
            case Qt::DisplayRole:
                return i18n("Language keywords");
            case GroupRole:
                return Qt::DisplayRole;
        }
    }

    if ( index.column() == KTextEditor::CodeCompletionModel::Name && role == Qt::DisplayRole )
        return m_items.at(index.row());

    if ( index.column() == KTextEditor::CodeCompletionModel::Icon && role == Qt::DecorationRole ) {
        static const QIcon icon(KIcon("code-variable").pixmap(QSize(16, 16)));
        return icon;
    }

  return QVariant();
}

KTextEditor::CodeCompletionModelControllerInterface3::MatchReaction KateKeywordCompletionModel::matchingItem(
    const QModelIndex& /*matched*/)
{
    return KTextEditor::CodeCompletionModelControllerInterface3::None;
}

#include "katekeywordcompletion.moc"

// kate: indent-width 4; replace-tabs on
