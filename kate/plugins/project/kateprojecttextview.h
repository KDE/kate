/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2010 Christoph Cullmann <cullmann@kde.org>
 *  Copyright (C) 2003 Anders Lund <anders.lund@lund.tdcadsl.dk>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#ifndef KATE_PROJECT_TEXT_VIEW_H
#define KATE_PROJECT_TEXT_VIEW_H

#include "kateprojectpluginview.h"

#include <ktexteditor/view.h>
#include <ktexteditor/codecompletionmodel.h>
#include <ktexteditor/codecompletionmodelcontrollerinterface.h>

/**
 * Addon class for KTextEditor::View.
 * Will be created for each view in the application and provide additional
 * stuff like code completion and co. based on project info.
 */
class KateProjectTextView : public KTextEditor::CodeCompletionModel, public KTextEditor::CodeCompletionModelControllerInterface3
{
  Q_OBJECT
  
  Q_INTERFACES(KTextEditor::CodeCompletionModelControllerInterface3)

  public:
    /**
     * Construct addon view.
     * @param pluginView our plugin view
     * @param view the view we are bundled to, will be our parent for auto-deletion.
     */
    KateProjectTextView (KateProjectPluginView *pluginView, KTextEditor::View *view);

    /**
     * Deconstruct addon view.
     */
    ~KateProjectTextView ();

    /**
     * This function is responsible to generating / updating the list of current
     * completions. The default implementation does nothing.
     *
     * When implementing this function, remember to call setRowCount() (or implement
     * rowCount()), and to generate the appropriate change notifications (for instance
     * by calling QAbstractItemModel::reset()).
     * @param view The view to generate completions for
     * @param range The range of text to generate completions for
     * */
    void completionInvoked(KTextEditor::View* view, const KTextEditor::Range& range, InvocationType invocationType);

    
    bool shouldStartCompletion(KTextEditor::View* view, const QString &insertedText, bool userInsertion, const KTextEditor::Cursor &position);
    bool shouldAbortCompletion(KTextEditor::View* view, const KTextEditor::Range &range, const QString &currentCompletion);

    
    
    void saveMatches( KTextEditor::View* view,
                            const KTextEditor::Range& range);

    int rowCount ( const QModelIndex & parent ) const;

    QVariant data(const QModelIndex& index, int role) const;
    virtual QModelIndex index(int row, int column, const QModelIndex& parent=QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex& index) const;
    virtual MatchReaction matchingItem(const QModelIndex& matched);

    virtual KTextEditor::Range completionRange(KTextEditor::View* view, const KTextEditor::Cursor &position);

    const QStringList allMatches( KTextEditor::View *view, const KTextEditor::Range &range ) const;

  private:
    /**
     * our plugin view
     */
    KateProjectPluginView *m_pluginView;

    /**
     * our view
     */
    KTextEditor::View *m_view;
    
    /**
     * list of matching words
     */
    QStringList m_matches;
    
    /**
     * automatic invocation?
     */
    bool m_automatic;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
