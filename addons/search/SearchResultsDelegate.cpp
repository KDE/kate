/*
    SPDX-FileCopyrightText: 2011 Kåre Särs <kare.sars@iki.fi>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "SearchResultsDelegate.h"
#include "MatchModel.h"

#include <KLocalizedString>
#include <KSyntaxHighlighting/Theme>
#include <KTextEditor/Editor>
#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QModelIndex>
#include <QPainter>
#include <QTextCharFormat>
#include <QTextDocument>

#include <drawing_utils.h>
#include <ktexteditor_utils.h>

// make list spacing resemble the default list spacing
// (which would not be the case with default QTextDocument margin)
static const int s_ItemMargin = 1;

static void lTrim(QString &in)
{
    int i = 0;
    for (auto c : in) {
        if (c != u' ') {
            break;
        }
        i++;
    }
    if (i < in.size()) {
        in.remove(0, i);
    }
}

SearchResultsDelegate::SearchResultsDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
    const auto e = KTextEditor::Editor::instance();
    const auto theme = e->theme();

    const auto updateColors = [this] {
        m_font = Utils::editorFont();
        const auto theme = KTextEditor::Editor::instance()->theme();

        m_textColorLight = m_textColor = QColor::fromRgba(theme.textColor(KSyntaxHighlighting::Theme::Normal));
        m_textColorLight.setAlpha(150);

        m_numBackground = m_altBackground = QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::IconBorder));
        m_numBackground.setAlpha(150);

        m_borderColor = QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::Separator));

        m_searchColor = QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::SearchHighlight));
        m_searchColor.setAlpha(200);

        m_replaceColor = QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::ReplaceHighlight));
        m_replaceColor.setAlpha(200);
    };
    connect(e, &KTextEditor::Editor::configChanged, this, updateColors);
    updateColors();
    m_font = Utils::editorFont();
}

static int lineNumAreaWidth(const QModelIndex &index, const QFontMetrics &fm)
{
    const auto lastRangeForFile = index.parent().data(MatchModel::LastMatchedRangeInFileRole).value<KTextEditor::Range>();
    const QString lineCol = QStringLiteral("%1:%2").arg(lastRangeForFile.start().line() + 1).arg(lastRangeForFile.start().column() + 1);
    return fm.horizontalAdvance(lineCol);
}

void SearchResultsDelegate::paintMatchItem(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &index) const
{
    const auto match = index.data(MatchModel::MatchItemRole).value<KateSearchMatch>();
    const int line = match.range.start().line() + 1;
    const int col = match.range.start().column() + 1;
    const QString lineCol = QStringLiteral("%1:%2").arg(line).arg(col);

    QStyle *style = opt.widget->style() ? opt.widget->style() : qApp->style();
    const QFontMetrics fm(m_font);

    static constexpr int hMargins = 2;

    const QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &opt, opt.widget);

    QRectF iconBorderRect = textRect;

    p->save();

    p->setFont(m_font);

    const bool rtl = opt.direction == Qt::RightToLeft;

    // line num area
    const bool selected = opt.state & QStyle::State_Selected;

    const int lineColWidth = lineNumAreaWidth(index, fm) + (hMargins * 2);
    if (rtl) {
        iconBorderRect.setX(textRect.width() - lineColWidth);
    }
    iconBorderRect.setWidth(lineColWidth);

    // line number area background
    p->fillRect(iconBorderRect, m_numBackground);

    // line numbers
    const QBrush lineNumCol = selected ? m_textColor : m_textColorLight;
    p->setPen(QPen(lineNumCol, 1));
    p->drawText(iconBorderRect.adjusted(2., 0., -2., 0.), Qt::AlignVCenter, lineCol);

    // draw the line number area separator line
    p->setPen(QPen(m_borderColor, 1));
    const QPointF p1 = rtl ? iconBorderRect.topLeft() : iconBorderRect.topRight();
    const QPointF p2 = rtl ? iconBorderRect.bottomLeft() : iconBorderRect.bottomRight();
    p->drawLine(p1, p2);

    // match
    p->setPen(QPen(m_textColor, 1));
    QString text;
    bool replacing = !match.replaceText.isEmpty();

    QString preMatchStr = match.preMatchStr;
    if (m_trimWhiteSpace) {
        lTrim(preMatchStr);
    }

    if (replacing) {
        text = preMatchStr + match.matchStr + match.replaceText + match.postMatchStr;
    } else {
        text = preMatchStr + match.matchStr + match.postMatchStr;
    }

    QList<QTextLayout::FormatRange> formats;

    QTextLayout::FormatRange fontFmt;
    fontFmt.start = 0;
    fontFmt.length = text.size();
    fontFmt.format.setFont(m_font);
    formats << fontFmt;

    QTextLayout::FormatRange matchFmt;
    matchFmt.start = preMatchStr.size();
    matchFmt.length = match.matchStr.size();
    matchFmt.format.setBackground(m_searchColor);
    matchFmt.format.setFontStrikeOut(replacing);
    formats << matchFmt;

    if (replacing) {
        QTextLayout::FormatRange repFmt;
        repFmt.start = preMatchStr.size() + match.matchStr.size();
        repFmt.length = match.replaceText.size();
        repFmt.format.setBackground(m_replaceColor);
        formats << repFmt;
    }

    // paint the match text
    auto opts = opt;
    opts.rect =
        rtl ? textRect.adjusted(0, 0, -(iconBorderRect.width() + (hMargins * 2)), 0) : textRect.adjusted(iconBorderRect.width() + (hMargins * 2), 0, 0, 0);
    Utils::paintItemViewText(p, text, opts, formats);

    p->restore();
}

static bool isMatchItem(const QModelIndex &index)
{
    return index.parent().isValid() && index.parent().parent().isValid();
}

void SearchResultsDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (!index.isValid()) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    QStyleOptionViewItem options = option;
    initStyleOption(&options, index);

    // draw item without text
    options.text = QString();
    if (!isMatchItem(index)) {
        options.backgroundBrush = m_altBackground;
    }
    options.widget->style()->drawControl(QStyle::CE_ItemViewItem, &options, painter, options.widget);

    if (isMatchItem(index)) {
        paintMatchItem(painter, options, index);
    } else {
        QTextDocument doc;
        doc.setDefaultFont(m_font);
        doc.setDocumentMargin(s_ItemMargin);
        doc.setHtml(index.data().toString());

        painter->save();

        // draw area
        QRect clip = options.widget->style()->subElementRect(QStyle::SE_ItemViewItemText, &options);
        if (index.flags() == Qt::NoItemFlags) {
            painter->setBrush(QBrush(QWidget().palette().color(QPalette::Base)));
            painter->setPen(QWidget().palette().color(QPalette::Base));
            painter->drawRect(QRect(clip.topLeft() - QPoint(20, 0), clip.bottomRight()));
            painter->translate(clip.topLeft() - QPoint(20, 0));
        } else {
            painter->translate(clip.topLeft() - QPoint(0, 0));
        }
        QAbstractTextDocumentLayout::PaintContext pcontext;
        pcontext.palette.setColor(QPalette::Text, options.palette.text().color());
        doc.documentLayout()->draw(painter, pcontext);

        painter->restore();
    }
}

QSize SearchResultsDelegate::sizeHint(const QStyleOptionViewItem &opt, const QModelIndex &index) const
{
    QSize s = QStyledItemDelegate::sizeHint(opt, index);
    QFontMetrics fm(m_font);
    s.setHeight(fm.lineSpacing());
    s = s.grownBy({0, 2, 0, 2});
    return s;
}

void SearchResultsDelegate::setTrimWhiteSpace(bool trim)
{
    m_trimWhiteSpace = trim;
}
