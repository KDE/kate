/*
    SPDX-FileCopyrightText: 2011-21 Kåre Särs <kare.sars@iki.fi>
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "Results.h"

#include "MatchProxyModel.h"
#include "SearchResultsDelegate.h"

#include <KSyntaxHighlighting/Theme>
#include <KTextEditor/Editor>

Results::Results(QWidget *parent)
    : QWidget(parent)
{
    setupUi(this);

    treeView->setItemDelegate(new SearchResultsDelegate(treeView));
    treeView->setProperty("_breeze_borders_sides", QVariant::fromValue(QFlags{Qt::TopEdge}));
    connect(treeView, &ResultsTreeView::detachClicked, this, [this] {
        Q_EMIT requestDetachToMainWindow(this);
    });

    auto *proxy = new MatchProxyModel(this);
    proxy->setSourceModel(&matchModel);
    proxy->setRecursiveFilteringEnabled(true);
    treeView->setModel(proxy);

    filterLineEdit->setVisible(false);
    filterLineEdit->setPlaceholderText(i18n("Filter…"));

    connect(filterLineEdit, &QLineEdit::textChanged, this, [this, proxy](const QString &text) {
        proxy->setFilterText(text);
        QTimer::singleShot(10, treeView, &QTreeView::expandAll);
    });

    auto updateColors = [this](KTextEditor::Editor *e) {
        if (!e) {
            return;
        }
        const auto theme = e->theme();
        auto search = QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::SearchHighlight));
        auto replace = QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::ReplaceHighlight));
        auto fg = QColor::fromRgba(theme.textColor(KSyntaxHighlighting::Theme::Normal));

        matchModel.setMatchColors(fg.name(QColor::HexArgb), search.name(QColor::HexArgb), replace.name(QColor::HexArgb));
    };

    auto e = KTextEditor::Editor::instance();
    connect(e, &KTextEditor::Editor::configChanged, this, updateColors);
    updateColors(e);
}

void Results::setFilterLineVisible(bool visible)
{
    filterLineEdit->setVisible(visible);
    if (!visible) {
        filterLineEdit->clear();
    } else {
        filterLineEdit->setFocus();
    }
}

void Results::setTrimWhiteSpace(bool set)
{
    auto *delegate = static_cast<SearchResultsDelegate *>(treeView->itemDelegate());
    delegate->setTrimWhiteSpace(set);
}

void Results::expandRoot()
{
    treeView->expand(treeView->model()->index(0, 0));
}

MatchProxyModel *Results::model() const
{
    return static_cast<MatchProxyModel *>(treeView->model());
}

bool Results::isEmpty() const
{
    return matchModel.isEmpty();
}

bool Results::isMatch(const QModelIndex &index) const
{
    Q_ASSERT(!index.isValid() || index.model() == model());
    return MatchModel::isMatch(model()->mapToSource(index));
}

QModelIndex Results::firstFileMatch(KTextEditor::Document *doc) const
{
    return model()->mapFromSource(matchModel.firstFileMatch(doc));
}

QModelIndex Results::closestMatchAfter(KTextEditor::Document *doc, const KTextEditor::Cursor &cursor) const
{
    return model()->mapFromSource(matchModel.closestMatchAfter(doc, cursor));
}

QModelIndex Results::firstMatch() const
{
    return model()->mapFromSource(matchModel.firstMatch());
}

QModelIndex Results::nextMatch(const QModelIndex &itemIndex) const
{
    Q_ASSERT(!itemIndex.isValid() || itemIndex.model() == model());
    return model()->mapFromSource(matchModel.nextMatch(model()->mapToSource(itemIndex)));
}

QModelIndex Results::prevMatch(const QModelIndex &itemIndex) const
{
    Q_ASSERT(!itemIndex.isValid() || itemIndex.model() == model());
    return model()->mapFromSource(matchModel.prevMatch(model()->mapToSource(itemIndex)));
}

QModelIndex Results::closestMatchBefore(KTextEditor::Document *doc, const KTextEditor::Cursor &cursor) const
{
    return model()->mapFromSource(matchModel.closestMatchBefore(doc, cursor));
}

QModelIndex Results::lastMatch() const
{
    return model()->mapFromSource(matchModel.lastMatch());
}

KTextEditor::Range Results::matchRange(const QModelIndex &matchIndex) const
{
    Q_ASSERT(matchIndex.model() == model());
    return matchModel.matchRange(model()->mapToSource(matchIndex));
}

bool Results::replaceSingleMatch(KTextEditor::Document *doc, const QModelIndex &matchIndex, const QRegularExpression &regExp, const QString &replaceString)
{
    Q_ASSERT(matchIndex.model() == model());
    const auto sourceIndex = model()->mapToSource(matchIndex);
    return matchModel.replaceSingleMatch(doc, sourceIndex, regExp, replaceString);
}

#include "moc_Results.cpp"
