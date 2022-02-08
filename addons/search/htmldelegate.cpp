/*
    SPDX-FileCopyrightText: 2011 Kåre Särs <kare.sars@iki.fi>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "htmldelegate.h"
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

#include <kfts_fuzzy_match.h>
#include <ktexteditor_utils.h>

// make list spacing resemble the default list spacing
// (which would not be the case with default QTextDocument margin)
static const int s_ItemMargin = 1;

SPHtmlDelegate::SPHtmlDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
    const auto e = KTextEditor::Editor::instance();
    const auto theme = e->theme();

    const auto updateColors = [this] {
        m_font = Utils::editorFont();
        const auto theme = KTextEditor::Editor::instance()->theme();
        m_lineNumColor = QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::LineNumbers));
        m_borderColor = QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::Separator));
        m_curLineNumColor = QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::CurrentLineNumber));
        m_textColor = QColor::fromRgba(theme.textColor(KSyntaxHighlighting::Theme::Normal));
        m_iconBorderColor = QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::IconBorder));
        m_curLineHighlightColor = QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::CurrentLine));
        m_searchColor = QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::SearchHighlight));
        m_replaceColor = QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::ReplaceHighlight));
    };
    connect(e, &KTextEditor::Editor::configChanged, this, updateColors);
    updateColors();
    m_font = Utils::editorFont();
}

static int lineNumAreaWidth(const QModelIndex &index, const QFontMetrics &fm)
{
    const auto lastRangeForFile = index.parent().data(MatchModel::LastMatchedRangeInFile).value<KTextEditor::Range>();
    const QString lineCol = QStringLiteral("%1:%2").arg(lastRangeForFile.start().line() + 1).arg(lastRangeForFile.start().column() + 1);
    return fm.horizontalAdvance(lineCol);
}

void SPHtmlDelegate::paintMatchItem(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &index) const
{
    const KateSearchMatch match = index.data(MatchModel::MatchItem).value<KateSearchMatch>();
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
    const QBrush iconBorderRectColor = selected ? m_curLineHighlightColor : m_iconBorderColor;

    const int lineColWidth = lineNumAreaWidth(index, fm) + (hMargins * 2);
    if (rtl) {
        iconBorderRect.setX(textRect.width() - lineColWidth);
    }
    iconBorderRect.setWidth(lineColWidth);

    // line number area background
    p->fillRect(iconBorderRect, iconBorderRectColor);

    // line numbers
    const QBrush lineNumCol = selected ? m_curLineNumColor : m_lineNumColor;
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
    if (replacing) {
        text = match.preMatchStr + match.matchStr + match.replaceText + match.postMatchStr;
    } else {
        text = match.preMatchStr + match.matchStr + match.postMatchStr;
    }

    QVector<QTextLayout::FormatRange> formats;

    QTextLayout::FormatRange fontFmt;
    fontFmt.start = 0;
    fontFmt.length = text.size();
    fontFmt.format.setFont(m_font);
    formats << fontFmt;

    QTextLayout::FormatRange matchFmt;
    matchFmt.start = match.preMatchStr.size();
    matchFmt.length = match.matchStr.size();
    matchFmt.format.setBackground(m_searchColor);
    matchFmt.format.setFontItalic(replacing);
    matchFmt.format.setFontStrikeOut(replacing);
    formats << matchFmt;

    if (replacing) {
        QTextLayout::FormatRange repFmt;
        repFmt.start = match.preMatchStr.size() + match.matchStr.size();
        repFmt.length = match.replaceText.size();
        repFmt.format.setBackground(m_replaceColor);
        formats << repFmt;
    }

    // paint the match text
    auto opts = opt;
    opts.rect = rtl ? textRect.adjusted(0, 0, -(iconBorderRect.width() + hMargins * 2), 0) : textRect.adjusted(iconBorderRect.width() + hMargins * 2, 0, 0, 0);
    kfts::paintItemViewText(p, text, opts, formats);

    p->restore();
}

static bool isMatchItem(const QModelIndex &index)
{
    return index.parent().isValid() && index.parent().parent().isValid();
}

void SPHtmlDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (!index.isValid()) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    QStyleOptionViewItem options = option;
    initStyleOption(&options, index);

    // draw item without text
    options.text = QString();
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
