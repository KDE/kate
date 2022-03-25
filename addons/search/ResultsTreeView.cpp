/*
    SPDX-FileCopyrightText: 2011-21 Kåre Särs <kare.sars@iki.fi>
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "ResultsTreeView.h"

#include <KSyntaxHighlighting/Theme>
#include <KTextEditor/Editor>

ResultsTreeView::ResultsTreeView(QWidget *parent)
    : QTreeView(parent)
{
    auto updateColors = [this](KTextEditor::Editor *e) {
        if (!e) {
            return;
        }

        const auto theme = e->theme();
        auto base = QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::BackgroundColor));
        auto highlight = QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::TextSelection));
        m_fg = QColor::fromRgba(theme.textColor(KSyntaxHighlighting::Theme::Normal));

        auto pal = palette();
        pal.setColor(QPalette::Base, base);
        pal.setColor(QPalette::Text, m_fg);
        pal.setColor(QPalette::Highlight, highlight);
        setPalette(pal);
    };

    auto *e = KTextEditor::Editor::instance();
    connect(e, &KTextEditor::Editor::configChanged, this, updateColors);
    updateColors(e);
}

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
QStyleOptionViewItem ResultsTreeView::viewOptions() const
{
    auto options = QTreeView::viewOptions();
    // We set this here so that the "expand triangle" in the treeview
    // is always colored in a visible color. This is important for
    // styles like fusion, where it can be dark on dark
    options.palette.setColor(QPalette::WindowText, m_fg);
    return options;
}
#else
void ResultsTreeView::initViewItemOption(QStyleOptionViewItem *option) const
{
    QTreeView::initViewItemOption(option);
    // We set this here so that the "expand triangle" in the treeview
    // is always colored in a visible color. This is important for
    // styles like fusion, where it can be dark on dark
    option->palette.setColor(QPalette::WindowText, m_fg);
}
#endif
