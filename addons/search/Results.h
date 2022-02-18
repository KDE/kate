/*
    SPDX-FileCopyrightText: 2011-21 Kåre Särs <kare.sars@iki.fi>
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef KATE_SEARCH_RESULTS_H
#define KATE_SEARCH_RESULTS_H

#include "MatchModel.h"
#include "ui_results.h"

#include <QWidget>

class Results final : public QWidget, public Ui::Results
{
    Q_OBJECT
public:
    Results(QWidget *parent = nullptr);
    int matches = 0;
    QRegularExpression regExp;
    bool useRegExp = false;
    bool matchCase = false;
    QString searchStr;
    QString replaceStr;
    int searchPlaceIndex = 0;
    QString treeRootText;
    MatchModel matchModel;
    bool displayFolderOptions = false;

    bool isEmpty() const;
    void setFilterLineVisible(bool visible);
    void expandRoot();
    bool isMatch(const QModelIndex &index) const;
    class MatchProxyModel *model() const;
    QModelIndex firstFileMatch(KTextEditor::Document *doc) const;
    QModelIndex closestMatchAfter(KTextEditor::Document *doc, const KTextEditor::Cursor &cursor) const;
    QModelIndex firstMatch() const;
    QModelIndex nextMatch(const QModelIndex &itemIndex) const;
    QModelIndex prevMatch(const QModelIndex &itemIndex) const;
    QModelIndex closestMatchBefore(KTextEditor::Document *doc, const KTextEditor::Cursor &cursor) const;
    QModelIndex lastMatch() const;
    KTextEditor::Range matchRange(const QModelIndex &matchIndex) const;
    bool replaceSingleMatch(KTextEditor::Document *doc, const QModelIndex &matchIndex, const QRegularExpression &regExp, const QString &replaceString);
};

#endif
