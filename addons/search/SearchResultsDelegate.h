/*
    SPDX-FileCopyrightText: 2011 Kåre Särs <kare.sars@iki.fi>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef HTML_DELEGATE_H
#define HTML_DELEGATE_H

#include <QFont>
#include <QStyledItemDelegate>

class SearchResultsDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit SearchResultsDelegate(QObject *parent);

    void paint(QPainter *, const QStyleOptionViewItem &, const QModelIndex &) const override;
    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const override;

private:
    void paintMatchItem(QPainter *, const QStyleOptionViewItem &, const QModelIndex &index) const;

    QFont m_font;
    QColor m_textColor;
    QColor m_textColorLight;

    QColor m_altBackground;
    QColor m_numBackground;

    QColor m_borderColor;

    QColor m_searchColor;
    QColor m_replaceColor;
};

#endif
