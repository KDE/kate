/*
    SPDX-FileCopyrightText: 2011 Kåre Särs <kare.sars@iki.fi>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef HTML_DELEGATE_H
#define HTML_DELEGATE_H

#include <QFont>
#include <QStyledItemDelegate>

class KateSearchMatch;

class SPHtmlDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit SPHtmlDelegate(QObject *parent);

    void paint(QPainter *, const QStyleOptionViewItem &, const QModelIndex &) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    void setMaxLineCol(int line, int col);

private:
    void paintMatchItem(QPainter *, const QStyleOptionViewItem &, const KateSearchMatch &) const;

    QFont m_font;
    int m_lineNumAreaWidth = 0;
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
