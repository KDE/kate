/*   Kate search plugin
 *
 * Copyright (C) 2011 by Kåre Särs <kare.sars@iki.fi>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program in a file called COPYING; if not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#ifndef _REPLACE_MATCHES_H_
#define _REPLACE_MATCHES_H_

#include <QElapsedTimer>
#include <QObject>
#include <QRegularExpression>
#include <QTreeWidget>
#include <ktexteditor/application.h>
#include <ktexteditor/document.h>
#include <ktexteditor/movinginterface.h>
#include <ktexteditor/movingrange.h>

class ReplaceMatches : public QObject
{
    Q_OBJECT

public:
    enum MatchData {
        FileUrlRole = Qt::UserRole,
        FileNameRole,
        StartLineRole,
        StartColumnRole,
        EndLineRole,
        EndColumnRole,
        MatchLenRole,
        PreMatchRole,
        MatchRole,
        PostMatchRole,
        ReplacedRole,
        ReplacedTextRole,
    };

    ReplaceMatches(QObject *parent = nullptr);
    void setDocumentManager(KTextEditor::Application *manager);

    bool replaceMatch(KTextEditor::Document *doc, QTreeWidgetItem *item, const KTextEditor::Range &range, const QRegularExpression &regExp, const QString &replaceTxt);
    bool replaceSingleMatch(KTextEditor::Document *doc, QTreeWidgetItem *item, const QRegularExpression &regExp, const QString &replaceTxt);
    void replaceChecked(QTreeWidget *tree, const QRegularExpression &regexp, const QString &replace);

    KTextEditor::Document *findNamed(const QString &name);

public Q_SLOTS:
    void cancelReplace();

private Q_SLOTS:
    void doReplaceNextMatch();

Q_SIGNALS:
    void replaceStatus(const QUrl &url, int replacedInFile, int matchesInFile);
    void replaceDone();

private:
    void updateTreeViewItems(QTreeWidgetItem *fileItem);

    KTextEditor::Application *m_manager = nullptr;
    QTreeWidget *m_tree = nullptr;
    int m_rootIndex = -1;
    int m_childStartIndex = -1;
    QVector<KTextEditor::MovingRange *> m_currentMatches;
    QVector<bool> m_currentReplaced;

    QRegularExpression m_regExp;
    QString m_replaceText;
    bool m_cancelReplace;
    QElapsedTimer m_progressTime;
};

#endif
