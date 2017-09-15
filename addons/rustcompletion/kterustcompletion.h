/***************************************************************************
 *   Copyright (C) 2015 by Eike Hein <hein@kde.org>                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#ifndef KTERUSTCOMPLETION_H
#define KTERUSTCOMPLETION_H

#include <QIcon>
#include <QUrl>

#include <KTextEditor/CodeCompletionModel>
#include <KTextEditor/CodeCompletionModelControllerInterface>

class KTERustCompletionPlugin;

namespace KTextEditor {
    class Document;
    class View;
}

struct CompletionMatch {
    CompletionMatch() : type(KTextEditor::CodeCompletionModel::NoProperty), depth(0), line(-1), col(-1) {}
    QString text;
    QIcon icon;
    KTextEditor::CodeCompletionModel::CompletionProperty type;
    int depth;
    QUrl url;
    int line;
    int col;
};

class KTERustCompletion : public KTextEditor::CodeCompletionModel, public KTextEditor::CodeCompletionModelControllerInterface
{
    Q_OBJECT

    Q_INTERFACES(KTextEditor::CodeCompletionModelControllerInterface)

    public:
        KTERustCompletion(KTERustCompletionPlugin *plugin);
        ~KTERustCompletion() override;

        enum MatchAction {
            Complete = 0,
            FindDefinition
        };

        bool shouldStartCompletion(KTextEditor::View *view, const QString &insertedText, bool userInsertion, const KTextEditor::Cursor &position) override;

        void completionInvoked(KTextEditor::View *view, const KTextEditor::Range &range, InvocationType invocationType) override;

        void aborted(KTextEditor::View *view) override;

        QVariant data(const QModelIndex &index, int role) const override;

        QList<CompletionMatch> getMatches(const KTextEditor::Document *document, MatchAction action, const KTextEditor::Cursor &position);

    private:
        static void addType(CompletionMatch &match, const QString &type);

        QList<CompletionMatch> m_matches;

        KTERustCompletionPlugin *m_plugin;
};

#endif

