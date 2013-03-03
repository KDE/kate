/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2003 Anders Lund <anders.lund@lund.tdcadsl.dk>
 *  Copyright (C) 2010 Christoph Cullmann <cullmann@kde.org>
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

#ifndef _KateWordCompletion_h_
#define _KateWordCompletion_h_

#include <ktexteditor/plugin.h>
#include <ktexteditor/view.h>
#include <ktexteditor/codecompletioninterface.h>
#include <ktexteditor/codecompletionmodel.h>
#include <ktexteditor/configpage.h>
#include "codecompletionmodelcontrollerinterfacev4.h"
#include <kxmlguiclient.h>

#include <QtCore/QEvent>
#include <QtCore/QObject>
#include <QtCore/QList>

#include <kdebug.h>


class KateWordCompletionModel : public KTextEditor::CodeCompletionModel2, public KTextEditor::CodeCompletionModelControllerInterface4
{
  Q_OBJECT
  Q_INTERFACES(KTextEditor::CodeCompletionModelControllerInterface4)
  public:
    KateWordCompletionModel( QObject *parent );
    ~KateWordCompletionModel();

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

    virtual bool shouldHideItemsWithEqualNames() const;

    const QStringList allMatches( KTextEditor::View *view, const KTextEditor::Range &range ) const;

    virtual void executeCompletionItem2(KTextEditor::Document* document, const KTextEditor::Range& word, const QModelIndex& index) const;

  private:
    QStringList m_matches;
    bool m_automatic;
};

class KateWordCompletionView : public QObject
{
  Q_OBJECT

  public:
    KateWordCompletionView(KTextEditor::View *view, KActionCollection* ac );
    ~KateWordCompletionView();

  private Q_SLOTS:
    void completeBackwards();
    void completeForwards();
    void slotCursorMoved();

    void shellComplete();

    void popupCompletionList();

  private:
    void complete( bool fw=true );

    const QString word() const;
    const KTextEditor::Range range() const;

    QString findLongestUnique( const QStringList &matches, int lead ) const;

    KTextEditor::View *m_view;
    KateWordCompletionModel *m_dWCompletionModel;
    struct KateWordCompletionViewPrivate *d;
};

#endif // _DocWordCompletionPlugin_h_

// kate: space-indent on; indent-width 2; replace-tabs on; mixed-indent off;
