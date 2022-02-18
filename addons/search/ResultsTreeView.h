/*
    SPDX-FileCopyrightText: 2011-21 Kåre Särs <kare.sars@iki.fi>
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef KATE_SEARCH_RESULTS_TREE_VIEW_H
#define KATE_SEARCH_RESULTS_TREE_VIEW_H

#include <QTreeView>

class ResultsTreeView : public QTreeView
{
    Q_OBJECT
public:
    ResultsTreeView(QWidget *parent);
    QStyleOptionViewItem viewOptions() const override;

private:
    QColor m_fg;
};

#endif
