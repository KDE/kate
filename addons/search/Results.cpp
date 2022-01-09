#include "Results.h"

#include "MatchProxyModel.h"
#include "htmldelegate.h"

#include <KSyntaxHighlighting/Theme>
#include <KTextEditor/Editor>

Results::Results(QWidget *parent)
    : QWidget(parent)
{
    setupUi(this);

    treeView->setItemDelegate(new SPHtmlDelegate(treeView));

    MatchProxyModel *proxy = new MatchProxyModel(this);
    proxy->setSourceModel(&matchModel);
    proxy->setRecursiveFilteringEnabled(true);
    treeView->setModel(proxy);

    filterLineEdit->setVisible(false);
    filterLineEdit->setPlaceholderText(i18n("Type to filter through results..."));

    connect(filterLineEdit, &QLineEdit::textChanged, this, [this, proxy](const QString &text) {
        proxy->setFilterText(text);
        QTimer::singleShot(10, treeView, &QTreeView::expandAll);
    });

    auto updateColors = [this](KTextEditor::Editor *e) {
        if (!e) {
            return;
        }

        const auto theme = e->theme();
        auto bg = QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::BackgroundColor));
        auto hl = QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::TextSelection));
        auto search = QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::SearchHighlight));
        auto replace = QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::ReplaceHighlight));
        auto fg = QColor::fromRgba(theme.textColor(KSyntaxHighlighting::Theme::Normal));

        auto pal = treeView->palette();
        pal.setColor(QPalette::Base, bg);
        pal.setColor(QPalette::Highlight, hl);
        pal.setColor(QPalette::Text, fg);
        matchModel.setMatchColors(fg.name(QColor::HexArgb), search.name(QColor::HexArgb), replace.name(QColor::HexArgb));
        treeView->setPalette(pal);

        Q_EMIT colorsChanged();
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

void Results::expandRoot()
{
    treeView->expand(treeView->model()->index(0, 0));
}

MatchProxyModel *Results::model() const
{
    return static_cast<MatchProxyModel *>(treeView->model());
}

bool Results::isMatch(const QModelIndex &index) const
{
    Q_ASSERT(index.model() == model());
    return matchModel.isMatch(model()->mapToSource(index));
}

QModelIndex Results::firstFileMatch(const QUrl &url) const
{
    return model()->mapFromSource(matchModel.firstFileMatch(url));
}

QModelIndex Results::closestMatchAfter(const QUrl &url, const KTextEditor::Cursor &cursor) const
{
    return model()->mapFromSource(matchModel.closestMatchAfter(url, cursor));
}

QModelIndex Results::firstMatch() const
{
    return model()->mapFromSource(matchModel.firstMatch());
}

QModelIndex Results::nextMatch(const QModelIndex &itemIndex) const
{
    Q_ASSERT(itemIndex.model() == model());
    return model()->mapFromSource(matchModel.nextMatch(model()->mapToSource(itemIndex)));
}

QModelIndex Results::prevMatch(const QModelIndex &itemIndex) const
{
    Q_ASSERT(itemIndex.model() == model());
    return model()->mapFromSource(matchModel.prevMatch(model()->mapToSource(itemIndex)));
}

QModelIndex Results::closestMatchBefore(const QUrl &url, const KTextEditor::Cursor &cursor) const
{
    return model()->mapFromSource(matchModel.closestMatchBefore(url, cursor));
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

