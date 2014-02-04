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

#ifndef KATEKEYWORDCOMPLETIONMODEL_H
#define KATEKEYWORDCOMPLETIONMODEL_H

#include "ktexteditor/codecompletionmodel.h"
#include "codecompletionmodelcontrollerinterfacev4.h"

/**
 * @brief Highlighting-file based keyword completion for the editor.
 *
 * This model offers completion of language-specific keywords based on information
 * taken from the kate syntax files. It queries the highlighting engine to get the
 * correct context for a given cursor position, then suggests all keyword items
 * from the XML file for the active language.
 */
class KateKeywordCompletionModel : public KTextEditor::CodeCompletionModel2
                                 , public KTextEditor::CodeCompletionModelControllerInterface4
{
    Q_OBJECT
    Q_INTERFACES(KTextEditor::CodeCompletionModelControllerInterface4)

public:
    KateKeywordCompletionModel(QObject* parent);
    virtual QVariant data(const QModelIndex& index, int role) const;
    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex& index) const;
    virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
    virtual void completionInvoked(KTextEditor::View* view, const KTextEditor::Range& range,
                                   InvocationType invocationType);
    virtual KTextEditor::Range completionRange(KTextEditor::View* view, const KTextEditor::Cursor& position);
    virtual bool shouldAbortCompletion(KTextEditor::View* view, const KTextEditor::Range& range,
                                       const QString& currentCompletion);
    virtual bool shouldStartCompletion(KTextEditor::View* view, const QString& insertedText, bool userInsertion,
                                       const KTextEditor::Cursor& position);
    virtual MatchReaction matchingItem(const QModelIndex& matched);
    virtual bool shouldHideItemsWithEqualNames() const;

private:
    QList<QString> m_items;
};

#endif // KATEKEYWORDCOMPLETIONMODEL_H

// kate: indent-width 4; replace-tabs on
