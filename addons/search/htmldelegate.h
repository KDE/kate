/*
    SPDX-FileCopyrightText: 2011 Kåre Särs <kare.sars@iki.fi>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef HTML_DELEGATE_H
#define HTML_DELEGATE_H

#include <QFont>
#include <QStyledItemDelegate>

class SPHtmlDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit SPHtmlDelegate(QObject *parent);

    void paint(QPainter *, const QStyleOptionViewItem &, const QModelIndex &) const override;
    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const override;

private:
    void paintMatchItem(QPainter *, const QStyleOptionViewItem &, const QModelIndex &index) const;

    QFont m_font;
    QBrush m_curLineHighlightColor;
    QBrush m_iconBorderColor;
    QBrush m_borderColor;
    QBrush m_lineNumColor;
    QBrush m_curLineNumColor;
    QBrush m_textColor;
    QBrush m_searchColor;
    QBrush m_replaceColor;
};

#endif
