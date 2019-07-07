/***************************************************************************
 *   Copyright (C) 2014,2018 by Kåre Särs <kare.sars@iki.fi>               *
 *   Copyright (C) 2019 by Mark Nauwelaerts <mark.nauwelaerts@gmail.com>   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#include "lspclientsymbolview.h"

#include <KLocalizedString>

#include <KTextEditor/Document>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>

#include <QHBoxLayout>
#include <QTreeWidget>
#include <QMenu>
#include <QHeaderView>
#include <QTimer>
#include <QPointer>

/*
 * Instantiates and manages the symbol outline toolview.
 */
class LSPClientSymbolViewImpl : public QObject, public LSPClientSymbolView
{
    Q_OBJECT

    typedef LSPClientSymbolViewImpl self_type;

    LSPClientPlugin *m_plugin;
    KTextEditor::MainWindow *m_mainWindow;
    QSharedPointer<LSPClientServerManager> m_serverManager;
    QScopedPointer<QWidget> m_toolview;
    // parent ownership
    QPointer<QTreeWidget> m_symbols;
    QScopedPointer<QMenu> m_popup;
    // icons used in tree representation
    QIcon m_icon_pkg;
    QIcon m_icon_class;
    QIcon m_icon_typedef;
    QIcon m_icon_function;
    QIcon m_icon_var;
    // initialized/updated from plugin settings
    // managed by context menu later on
    // parent ownership
    QAction *m_detailsOn;
    QAction *m_expandOn;
    QAction *m_treeOn;
    QAction *m_sortOn;
    // timers to delay some todo's
    QTimer m_refreshTimer;
    QTimer m_currentItemTimer;
    int m_oldCursorLine;
    // outstanding request
    LSPClientServer::RequestHandle m_handle;

public:
    LSPClientSymbolViewImpl(LSPClientPlugin *plugin, KTextEditor::MainWindow *mainWin,
        QSharedPointer<LSPClientServerManager> manager)
        : m_plugin(plugin), m_mainWindow(mainWin), m_serverManager(manager)
    {
        m_icon_pkg = QIcon::fromTheme(QStringLiteral("code-block"));
        m_icon_class = QIcon::fromTheme(QStringLiteral("code-class"));
        m_icon_typedef = QIcon::fromTheme(QStringLiteral("code-typedef"));
        m_icon_function = QIcon::fromTheme(QStringLiteral("code-function"));
        m_icon_var = QIcon::fromTheme(QStringLiteral("code-variable"));

        m_toolview.reset(m_mainWindow->createToolView(plugin, QStringLiteral("lspclient_symbol_outline"),
                                                      KTextEditor::MainWindow::Right,
                                                      QIcon::fromTheme(QStringLiteral("code-context")),
                                                      i18n("LSP Client Symbol Outline")));

        QWidget *container = new QWidget(m_toolview.get());
        QHBoxLayout *layout = new QHBoxLayout(container);

        m_symbols = new QTreeWidget();
        m_symbols->setFocusPolicy(Qt::NoFocus);
        m_symbols->setLayoutDirection( Qt::LeftToRight );
        layout->addWidget(m_symbols, 10);
        layout->setContentsMargins(0,0,0,0);

        QStringList titles;
        titles << i18nc("@title:column", "Symbols") << i18nc("@title:column", "Position");
        m_symbols->setColumnCount(3);
        m_symbols->setHeaderLabels(titles);
        m_symbols->setColumnHidden(1, true);
        m_symbols->setColumnHidden(2, true);
        m_symbols->setContextMenuPolicy(Qt::CustomContextMenu);
        m_symbols->setIndentation(10);

        connect(m_symbols, &QTreeWidget::itemClicked, this, &self_type::goToSymbol);
        connect(m_symbols, &QTreeWidget::customContextMenuRequested, this, &self_type::showContextMenu);
        connect(m_symbols, &QTreeWidget::itemExpanded, this, &self_type::updateCurrentTreeItem);
        connect(m_symbols, &QTreeWidget::itemCollapsed, this, &self_type::updateCurrentTreeItem);

        // context menu
        m_popup.reset(new QMenu(m_symbols));
        m_treeOn = m_popup->addAction(i18n("Tree Mode"), this, &self_type::displayOptionChanged);
        m_treeOn->setCheckable(true);
        m_expandOn = m_popup->addAction(i18n("Automatically Expand Tree"), this, &self_type::displayOptionChanged);
        m_expandOn->setCheckable(true);
        m_sortOn = m_popup->addAction(i18n("Sort Alphabetically"), this, &self_type::displayOptionChanged);
        m_sortOn->setCheckable(true);
        m_detailsOn = m_popup->addAction(i18n("Show Details"), this, &self_type::displayOptionChanged);
        m_detailsOn->setCheckable(true);
        m_popup->addSeparator();
        m_popup->addAction(i18n("Expand All"), this, &self_type::expandAll);
        m_popup->addAction(i18n("Collapse All"), this, &self_type::collapseAll);

        // sync with plugin settings if updated
        connect(m_plugin, &LSPClientPlugin::update, this, &self_type::configUpdated);

        // get updated
        m_refreshTimer.setSingleShot(true);
        connect(&m_refreshTimer, &QTimer::timeout, this, &self_type::refresh);
        m_currentItemTimer.setSingleShot(true);
        connect(&m_currentItemTimer, &QTimer::timeout, this, &self_type::updateCurrentTreeItem);

        connect(m_mainWindow, &KTextEditor::MainWindow::viewChanged, this, &self_type::viewChanged);
        connect(m_serverManager.get(), &LSPClientServerManager::serverChanged, this, &self_type::refresh);

        // initial trigger
        configUpdated();
    }

    void displayOptionChanged()
    {
        m_expandOn->setEnabled(m_treeOn->isChecked());
        refresh();
    }

    void configUpdated()
    {
        m_treeOn->setChecked(m_plugin->m_symbolTree);
        m_detailsOn->setChecked(m_plugin->m_symbolDetails);
        m_expandOn->setChecked(m_plugin->m_symbolExpand);
        m_sortOn->setChecked(m_plugin->m_symbolSort);
        displayOptionChanged();
    }

    void showContextMenu(const QPoint&)
    {
        m_popup->popup(QCursor::pos(), m_treeOn);
    }

    void viewChanged(KTextEditor::View *view)
    {
        refresh();

        if (view) {
          connect(view, &KTextEditor::View::cursorPositionChanged, this, &self_type::cursorPositionChanged, Qt::UniqueConnection);
          if (view->document()) {
            connect(view->document(), &KTextEditor::Document::textChanged, this, &self_type::textChanged, Qt::UniqueConnection);
          }
        }
    }

    void textChanged()
    {
        // refresh also updates current position
        m_currentItemTimer.stop();
        m_refreshTimer.start(500);
    }

    void cursorPositionChanged(KTextEditor::View *view,
                               const KTextEditor::Cursor &newPosition)
    {
        if (m_refreshTimer.isActive()) {
            // update will come upon refresh
            return;
        }

        if (view && newPosition.line() != m_oldCursorLine) {
            m_oldCursorLine = newPosition.line();
            m_currentItemTimer.start(100);
        }
    }

    void makeNodes(const QList<LSPSymbolInformation> & symbols, bool tree,
        bool show_detail, QTreeWidget * widget, QTreeWidgetItem * parent,
        int * details)
    {
        QIcon *icon = nullptr;

        for (const auto& symbol: symbols) {
            switch (symbol.kind) {
            case LSPSymbolKind::File:
            case LSPSymbolKind::Module:
            case LSPSymbolKind::Namespace:
            case LSPSymbolKind::Package:
                if (symbol.children.count() == 0)
                    continue;
                icon = &m_icon_pkg;
                break;
            case LSPSymbolKind::Class:
            case LSPSymbolKind::Interface:
                icon = &m_icon_class;
                break;
            case LSPSymbolKind::Enum:
                icon = &m_icon_typedef;
                break;
            case LSPSymbolKind::Method:
            case LSPSymbolKind::Function:
            case LSPSymbolKind::Constructor:
                icon = &m_icon_function;
                break;
            // all others considered/assumed Variable
            case LSPSymbolKind::Variable:
            case LSPSymbolKind::Constant:
            case LSPSymbolKind::String:
            case LSPSymbolKind::Number:
            case LSPSymbolKind::Property:
            case LSPSymbolKind::Field:
            default:
                // skip local variable
                // property, field, etc unlikely in such case anyway
                if (parent && parent->icon(0).cacheKey() == m_icon_function.cacheKey())
                    continue;
                icon = &m_icon_var;
            }
            auto node = parent && tree ?
                    new QTreeWidgetItem(parent) : new QTreeWidgetItem(widget);
            if (!symbol.detail.isEmpty() && details)
                ++details;
            auto detail = show_detail ? symbol.detail : QStringLiteral("");
            node->setText(0, symbol.name + detail);
            node->setIcon(0, *icon);
            node->setText(1, QString::number(symbol.range.start().line(), 10));
            node->setText(2, QString::number(symbol.range.end().line(), 10));
            // recurse children
            makeNodes(symbol.children, tree, show_detail, widget, node, details);
        }
    }

    void onDocumentSymbols(const QList<LSPSymbolInformation> & outline)
    {
        if (!m_symbols)
            return;

        // populate with sort disabled
        Qt::SortOrder sortOrder = m_symbols->header()->sortIndicatorOrder();
        m_symbols->clear();
        m_symbols->setSortingEnabled(false);
        int details = 0;

        makeNodes(outline, m_treeOn->isChecked(), m_detailsOn->isChecked(),
            m_symbols, nullptr, &details);
        if (m_symbols->topLevelItemCount() == 0) {
            QTreeWidgetItem *node = new QTreeWidgetItem(m_symbols);
            node->setText(0, i18n("No outline items"));
        }
        if (m_expandOn->isChecked())
            expandAll();
        // disable detail setting if no such info available
        // (as an indication there is nothing to show anyway)
        if (!details)
            m_detailsOn->setEnabled(false);
        if (m_sortOn->isChecked()) {
            m_symbols->setSortingEnabled(true);
            m_symbols->sortItems(0, sortOrder);
        }
        // current item tracking
        updateCurrentTreeItem();
        m_oldCursorLine = -1;
    }

    void refresh()
    {
        m_handle.cancel();
        auto view = m_mainWindow->activeView();
        auto server = m_serverManager->findServer(view);
        if (server) {
            server->documentSymbols(view->document()->url(), this,
                mem_fun(&self_type::onDocumentSymbols, this));
        } else if (m_symbols) {
            m_symbols->clear();
            QTreeWidgetItem *node = new QTreeWidgetItem(m_symbols);
            node->setText(0, i18n("No server available"));
        }
    }

    QTreeWidgetItem* getCurrentItem(QTreeWidgetItem * item, int line)
    {
        for (int i = 0; i < item->childCount(); i++) {
            auto citem = getCurrentItem(item->child(i), line);
            if (citem)
                return citem;
        }

        int lstart = item->data(1, Qt::DisplayRole).toInt();
        int lend = item->data(2, Qt::DisplayRole).toInt();
        if (lstart <= line && line <= lend)
            return item;

        return nullptr;
    }

    void updateCurrentTreeItem()
    {
        KTextEditor::View* editView = m_mainWindow->activeView();
        if (!editView || !m_symbols) {
            return;
        }

        int currLine = editView->cursorPositionVirtual().line();
        auto item = getCurrentItem(m_symbols->invisibleRootItem(), currLine);

        // go up until a non-expanded item is found
        // (the others were collapsed for some reason ...)

        while (item) {
            auto parent = item->parent();
            if (parent && !parent->isExpanded()) {
                item = parent;
            } else {
                break;
            }
        }

        m_symbols->blockSignals(true);
        m_symbols->setCurrentItem(item);
        m_symbols->blockSignals(false);
    }

    void expandAll()
    {
        if (!m_symbols)
            return;

        QTreeWidgetItemIterator it(m_symbols, QTreeWidgetItemIterator::HasChildren);
        while (*it) {
            m_symbols->expandItem(*it);
             ++it;
         }
    }

    void collapseAll()
    {
        if (!m_symbols)
            return;

        QTreeWidgetItemIterator it(m_symbols, QTreeWidgetItemIterator::HasChildren);
        while (*it) {
            m_symbols->collapseItem(*it);
             ++it;
         }
    }

    void goToSymbol(QTreeWidgetItem *it)
    {
      KTextEditor::View *kv = m_mainWindow->activeView();
      if (kv && it && !it->text(1).isEmpty()) {
        kv->setCursorPosition(KTextEditor::Cursor(it->text(1).toInt(nullptr, 10), 0));
      }
    }
};


QObject*
LSPClientSymbolView::new_(LSPClientPlugin *plugin, KTextEditor::MainWindow *mainWin,
    QSharedPointer<LSPClientServerManager> manager)
{
    return new LSPClientSymbolViewImpl(plugin, mainWin, manager);
}

#include "lspclientsymbolview.moc"
