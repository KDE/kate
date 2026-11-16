/*
    SPDX-FileCopyrightText: 2011-21 Kåre Särs <kare.sars@iki.fi>
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include <QTreeView>

class QPushButton;

class ResultsTreeView : public QTreeView
{
    Q_OBJECT
public:
    ResultsTreeView(QWidget *parent);

    void initViewItemOption(QStyleOptionViewItem *option) const override;

    QPushButton *expandResultsButton() const
    {
        return m_expandResultsButton;
    }
    QPushButton *filterButton() const
    {
        return m_filterButton;
    }
    void updateButtonsVisibility();

private:
    void updateFloatingButtonsGeometry();

    QColor m_fg;
    QPushButton *const m_expandResultsButton;
    QPushButton *const m_filterButton;
    QPushButton *const m_detachButton;

protected:
    void resizeEvent(QResizeEvent *) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *) override;

Q_SIGNALS:
    void geometryChanged();
    void detachClicked();
};
