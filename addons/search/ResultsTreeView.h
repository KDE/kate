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

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    QStyleOptionViewItem viewOptions() const override;
#else
    void initViewItemOption(QStyleOptionViewItem *option) const override;
#endif

private:
    QColor m_fg;
    class QPushButton *const m_detachButton;

protected:
    void resizeEvent(QResizeEvent *) override;
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    void enterEvent(QEvent *event) override;
#else
    void enterEvent(QEnterEvent *event) override;
#endif
    void leaveEvent(QEvent *) override;

Q_SIGNALS:
    void geometryChanged();
    void detachClicked();
};

#endif
