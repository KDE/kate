#include "gotosymboldialog.h"
#include "lspclientserver.h"

#include <KTextEditor/MainWindow>
#include <KTextEditor/View>

#include <QStandardItemModel>

static constexpr int SymbolInfoRole = Qt::UserRole + 1;

GotoSymbolHUDDialog::GotoSymbolHUDDialog(KTextEditor::MainWindow *mainWindow, QSharedPointer<LSPClientServer> server)
    : QuickDialog(nullptr, mainWindow->window())
    , model(new QStandardItemModel(this))
    , mainWindow(mainWindow)
    , server(std::move(server))
{
    m_treeView.setModel(model);

    connect(&m_lineEdit, &QLineEdit::textChanged, this, &GotoSymbolHUDDialog::slotTextChanged);
}

void GotoSymbolHUDDialog::slotReturnPressed()
{
    auto symbol = m_treeView.currentIndex().data(SymbolInfoRole).value<LSPSymbolInformation>();
    if (!symbol.url.isValid() || symbol.url.isEmpty()) {
        return;
    }

    auto v = mainWindow->openUrl(symbol.url);
    if (v) {
        v->setCursorPosition(symbol.range.start());
    }
    close();
}

void GotoSymbolHUDDialog::openDialog()
{
    updateViewGeometry();
    setFocus();
    exec();
}

QIcon GotoSymbolHUDDialog::iconForSymbolKind(LSPSymbolKind kind) const
{
    switch (kind) {
    case LSPSymbolKind::File:
    case LSPSymbolKind::Module:
    case LSPSymbolKind::Namespace:
    case LSPSymbolKind::Package:
        return m_icon_pkg;
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
            item->setData(QVariant::fromValue(sym), SymbolInfoRole);
            model->appendRow(item);
        }
    };
    server->workspaceSymbol(text, this, hh);
}
