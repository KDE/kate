/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "gotosymboldialog.h"
#include "lspclientserver.h"

#include <KLocalizedString>
#include <KSyntaxHighlighting/Theme>
#include <KTextEditor/Editor>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>

#include <QFileInfo>
#include <QPainter>
#include <QStandardItemModel>
#include <QStyledItemDelegate>

#include <drawing_utils.h>
#include <ktexteditor_utils.h>

static constexpr int SymbolInfoRole = Qt::UserRole + 1;

struct GotoSymbolItem {
    QUrl fileUrl;
    KTextEditor::Cursor pos;
    LSPSymbolKind kind;
};
Q_DECLARE_METATYPE(GotoSymbolItem)
Q_DECLARE_TYPEINFO(GotoSymbolItem, Q_MOVABLE_TYPE);

class GotoSymbolHUDStyleDelegate : public QStyledItemDelegate
{
public:
    GotoSymbolHUDStyleDelegate(QObject *parent)
        : QStyledItemDelegate(parent)
    {
    }

    void setColors()
    {
        using KSyntaxHighlighting::Theme;
        auto theme = KTextEditor::Editor::instance()->theme();
        normalColor = theme.textColor(Theme::Normal);
        typeColor = theme.textColor(Theme::DataType);
        keywordColor = theme.textColor(Theme::Keyword);
        funcColor = theme.textColor(Theme::Function);
    }

    void setFont(const QFont &font)
    {
        monoFont = font;
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QStyleOptionViewItem options = option;
        initStyleOption(&options, index);
        options.icon = Utils::colorIcon(options.icon, normalColor);

        auto style = options.widget->style();

        painter->save();

        QString text = options.text;
        options.text = QString();
        style->drawControl(QStyle::CE_ItemViewItem, &options, painter, options.widget);

        const auto textRect = options.widget->style()->subElementRect(QStyle::SE_ItemViewItemText, &options, options.widget);

        auto symbol = index.data(SymbolInfoRole).value<GotoSymbolItem>();
        auto kind = symbol.kind;

        QList<QTextLayout::FormatRange> fmts;
        int colons = text.indexOf(QStringLiteral("::"));
        int i = 0;
        // container name
        if (colons != -1) {
            QTextCharFormat fmt;
            fmt.setForeground(keywordColor);
            fmt.setFont(monoFont);
            fmts.append({.start = 0, .length = colons, .format = fmt});
            i = colons + 2;
        }
        // symbol name
        {
            QTextCharFormat f;
            f.setForeground(colorForSymbolKind(kind));
            f.setFont(monoFont);
            fmts.append({.start = i, .length = int(text.length() - i), .format = f});
        }

        // add file name to the text we are going to display
        auto file = QFileInfo(symbol.fileUrl.toLocalFile()).fileName();
        int textLength = text.length();
        text += QStringLiteral(" ") + file;

        // file name
        {
            QTextCharFormat f;
            f.setForeground(Qt::gray);
            fmts.append({.start = textLength, .length = int(text.length() - textLength), .format = f});
        }

        options.rect = textRect;
        Utils::paintItemViewText(painter, text, options, fmts);

        painter->restore();
    }

private:
    QColor colorForSymbolKind(LSPSymbolKind kind) const
    {
        switch (kind) {
        case LSPSymbolKind::File:
        case LSPSymbolKind::Module:
        case LSPSymbolKind::Namespace:
        case LSPSymbolKind::Package:
            return keywordColor;
        case LSPSymbolKind::Struct:
        case LSPSymbolKind::Class:
        case LSPSymbolKind::Interface:
            return typeColor;
        case LSPSymbolKind::Enum:
            return typeColor;
        case LSPSymbolKind::Method:
        case LSPSymbolKind::Function:
        case LSPSymbolKind::Constructor:
            return funcColor;
        // all others considered/assumed Variable
        case LSPSymbolKind::Variable:
        case LSPSymbolKind::Constant:
        case LSPSymbolKind::String:
        case LSPSymbolKind::Number:
        case LSPSymbolKind::Property:
        case LSPSymbolKind::Field:
        default:
            return normalColor;
        }
    }

    QColor funcColor;
    QColor keywordColor;
    QColor typeColor;
    QColor normalColor;

    QFont monoFont;
};

GotoSymbolHUDDialog::GotoSymbolHUDDialog(KTextEditor::MainWindow *mainWindow, std::shared_ptr<LSPClientServer> server)
    : HUDDialog(mainWindow->window())
    , model(new QStandardItemModel(this))
    , mainWindow(mainWindow)
    , server(std::move(server))
{
    m_lineEdit.setPlaceholderText(i18n("Filter..."));

    m_treeView.setModel(model);
    auto delegate = new GotoSymbolHUDStyleDelegate(this);
    m_treeView.setItemDelegate(delegate);
    setPaletteToEditorColors();

    connect(&m_lineEdit, &QLineEdit::textChanged, this, &GotoSymbolHUDDialog::slotTextChanged);
    connect(KTextEditor::Editor::instance(), &KTextEditor::Editor::configChanged, this, &GotoSymbolHUDDialog::setPaletteToEditorColors);
}

void GotoSymbolHUDDialog::setPaletteToEditorColors()
{
    auto pal = m_treeView.palette();
    auto e = KTextEditor::Editor::instance();
    auto bg = QColor::fromRgba(e->theme().editorColor(KSyntaxHighlighting::Theme::BackgroundColor));
    auto fg = QColor::fromRgba(e->theme().textColor(KSyntaxHighlighting::Theme::Normal));
    auto hl = QColor::fromRgba(e->theme().editorColor(KSyntaxHighlighting::Theme::TextSelection));
    pal.setColor(QPalette::Base, bg);
    pal.setColor(QPalette::Text, fg);
    pal.setColor(QPalette::Highlight, hl);
    m_treeView.setPalette(pal);

    auto *delegate = static_cast<GotoSymbolHUDStyleDelegate *>(m_treeView.itemDelegate());
    delegate->setColors();
    delegate->setFont(Utils::editorFont());
}

void GotoSymbolHUDDialog::slotReturnPressed(const QModelIndex &index)
{
    auto symbol = index.data(SymbolInfoRole).value<GotoSymbolItem>();
    if (!symbol.fileUrl.isValid() || symbol.fileUrl.isEmpty()) {
        return;
    }

    auto v = mainWindow->openUrl(symbol.fileUrl);
    if (v) {
        v->setCursorPosition(symbol.pos);
    }
    deleteLater();
    hide();
}

QIcon GotoSymbolHUDDialog::iconForSymbolKind(LSPSymbolKind kind) const
{
    switch (kind) {
    case LSPSymbolKind::File:
    case LSPSymbolKind::Module:
    case LSPSymbolKind::Namespace:
    case LSPSymbolKind::Package:
        return m_icon_pkg;
    case LSPSymbolKind::Struct:
    case LSPSymbolKind::Class:
    case LSPSymbolKind::Interface:
        return m_icon_class;
    case LSPSymbolKind::Enum:
        return m_icon_typedef;
    case LSPSymbolKind::Method:
    case LSPSymbolKind::Function:
    case LSPSymbolKind::Constructor:
        return m_icon_function;
    // all others considered/assumed Variable
    case LSPSymbolKind::Variable:
    case LSPSymbolKind::Constant:
    case LSPSymbolKind::String:
    case LSPSymbolKind::Number:
    case LSPSymbolKind::Property:
    case LSPSymbolKind::Field:
    default:
        return m_icon_var;
    }
}

void GotoSymbolHUDDialog::slotTextChanged(const QString &text)
{
    /**
     * empty text can lead to return *all* symbols of the workspace, which may choke the dialog
     * so we ignore it
     *
     * Also, at least 2 characters must be there to start getting symbols from the server
     */
    if (!server || text.isEmpty() || text.length() < 2) {
        return;
    }

    auto hh = [this](const std::vector<LSPSymbolInformation> &symbols) {
        model->clear();
        for (const auto &sym : symbols) {
            auto item = new QStandardItem(iconForSymbolKind(sym.kind), sym.name);
            item->setData(QVariant::fromValue(GotoSymbolItem{.fileUrl = sym.url, .pos = sym.range.start(), .kind = sym.kind}), SymbolInfoRole);
            model->appendRow(item);
        }
        m_treeView.setCurrentIndex(model->index(0, 0));
    };
    server->workspaceSymbol(text, this, hh);
}
