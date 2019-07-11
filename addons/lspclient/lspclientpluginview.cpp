/***************************************************************************
 *   Copyright (C) 2015 by Eike Hein <hein@kde.org>                        *
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

#include "lspclientpluginview.h"
#include "lspclientsymbolview.h"
#include "lspclientplugin.h"
#include "lspclientservermanager.h"
#include "lspclientcompletion.h"

#include "lspclient_debug.h"

#include <KActionCollection>
#include <KActionMenu>
#include <KLocalizedString>
#include <KStandardAction>
#include <KXMLGUIFactory>
#include <KAcceleratorManager>

#include <KTextEditor/CodeCompletionInterface>
#include <KTextEditor/Document>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>
#include <KTextEditor/Message>
#include <KTextEditor/MovingInterface>
#include <KXMLGUIClient>

#include <ktexteditor/markinterface.h>
#include <ktexteditor/movinginterface.h>
#include <ktexteditor/movingrange.h>
#include <ktexteditor/configinterface.h>

#include <QKeyEvent>
#include <QHBoxLayout>
#include <QAction>
#include <QTreeWidget>
#include <QHeaderView>
#include <QTimer>
#include <QSet>
#include <QTextCodec>
#include <QApplication>
#include <QFileInfo>

namespace RangeData
{

enum {
    FileUrlRole = Qt::UserRole,
    StartLineRole,
    StartColumnRole,
    EndLineRole,
    EndColumnRole,
    KindRole
};

class KindEnum
{
public:
    enum _kind {
        Text = (int) LSPDocumentHighlightKind::Text,
        Read = (int) LSPDocumentHighlightKind::Read,
        Write = (int) LSPDocumentHighlightKind::Write,
        Error = 10 + (int) LSPDiagnosticSeverity::Error,
        Warning = 10 + (int) LSPDiagnosticSeverity::Warning,
        Information = 10 + (int) LSPDiagnosticSeverity::Information,
        Hint = 10 + (int) LSPDiagnosticSeverity::Hint,
        Related
    };

    KindEnum(int v)
    { m_value = (_kind) v; }

    KindEnum(LSPDocumentHighlightKind hl)
        : KindEnum((_kind) (hl))
    {}

    KindEnum(LSPDiagnosticSeverity sev)
        : KindEnum(_kind(10 + (int) sev))
    {}

    operator _kind()
    { return m_value; }

private:
    _kind m_value;
};

static constexpr KTextEditor::MarkInterface::MarkTypes markType = KTextEditor::MarkInterface::markType31;
static constexpr KTextEditor::MarkInterface::MarkTypes markTypeDiagError = KTextEditor::MarkInterface::Error;
static constexpr KTextEditor::MarkInterface::MarkTypes markTypeDiagWarning = KTextEditor::MarkInterface::Warning;
static constexpr KTextEditor::MarkInterface::MarkTypes markTypeDiagOther = KTextEditor::MarkInterface::markType30;
static constexpr KTextEditor::MarkInterface::MarkTypes markTypeDiagAll =
        KTextEditor::MarkInterface::MarkTypes (markTypeDiagError | markTypeDiagWarning | markTypeDiagOther);

}

static QIcon
diagnosticsIcon(LSPDiagnosticSeverity severity)
{
#define RETURN_CACHED_ICON(name) \
{ \
static QIcon icon(QIcon::fromTheme(QStringLiteral(name))); \
return icon; \
}
    switch (severity)
    {
    case LSPDiagnosticSeverity::Error:
        RETURN_CACHED_ICON("dialog-error")
    case LSPDiagnosticSeverity::Warning:
        RETURN_CACHED_ICON("dialog-warning")
    case LSPDiagnosticSeverity::Information:
    case LSPDiagnosticSeverity::Hint:
        RETURN_CACHED_ICON("dialog-information")
    default:
        break;
    }
    return QIcon();
}

// helper to read lines from unopened documents
// lightweight and does not require additional symbols
class FileLineReader
{
    QFile file;
    int linesRead = 0;

public:
    FileLineReader(const QUrl & url)
        : file(url.path())
    {
        file.open(QIODevice::ReadOnly);
    }

    // called with increasing lineno
    QString line(int lineno)
    {
        while (file.isOpen() && !file.atEnd()) {
            auto line = file.readLine();
            if (linesRead++ == lineno) {
                QTextCodec::ConverterState state;
                QTextCodec *codec = QTextCodec::codecForName("UTF-8");
                QString text = codec->toUnicode(line.constData(), line.size(), &state);
                if (state.invalidChars > 0) {
                    text = QString::fromLatin1(line);
                }
                while (text.size() && text.at(text.size() -1).isSpace())
                    text.chop(1);
                return text;
            }
        }
        return QString();
    }
};


class LSPClientPluginViewImpl : public QObject, public KXMLGUIClient
{
    Q_OBJECT

    typedef LSPClientPluginViewImpl self_type;

    LSPClientPlugin *m_plugin;
    KTextEditor::MainWindow *m_mainWindow;
    QSharedPointer<LSPClientServerManager> m_serverManager;
    QScopedPointer<LSPClientCompletion> m_completion;
    QScopedPointer<QObject> m_symbolView;

    QPointer<QAction> m_findDef;
    QPointer<QAction> m_findDecl;
    QPointer<QAction> m_findRef;
    QPointer<QAction> m_highlight;
    QPointer<QAction> m_hover;
    QPointer<QAction> m_complDocOn;
    QPointer<QAction> m_refDeclaration;
    QPointer<QAction> m_restartServer;
    QPointer<QAction> m_restartAll;

    // toolview
    QScopedPointer<QWidget> m_toolView;
    QPointer<QTabWidget> m_tabWidget;
    // applied search ranges
    typedef QMultiHash<KTextEditor::Document*, KTextEditor::MovingRange*> RangeCollection;
    RangeCollection m_ranges;
    // tree is either added to tabwidget or owned here
    QScopedPointer<QTreeWidget> m_ownedTree;
    // in either case, the tree that directs applying marks/ranges
    QPointer<QTreeWidget> m_markTree;

    // diagnostics tab
    QPointer<QTreeWidget> m_diagnosticsTree;
    // diagnostics ranges
    RangeCollection m_diagnosticsRanges;

    // views on which completions have been registered
    QSet<KTextEditor::View *> m_completionViews;
    // outstanding request
    LSPClientServer::RequestHandle m_handle;
    // timeout on request
    bool m_req_timeout = false;

public:
    LSPClientPluginViewImpl(LSPClientPlugin *plugin, KTextEditor::MainWindow *mainWin)
        : QObject(mainWin), m_plugin(plugin), m_mainWindow(mainWin),
          m_serverManager(LSPClientServerManager::new_(plugin, mainWin)),
          m_completion(LSPClientCompletion::new_(m_serverManager)),
          m_symbolView(LSPClientSymbolView::new_(plugin, mainWin, m_serverManager))
    {
        KXMLGUIClient::setComponentName(QStringLiteral("lspclient"), i18n("LSP Client"));
        setXMLFile(QStringLiteral("ui.rc"));

        connect(m_mainWindow, &KTextEditor::MainWindow::viewChanged, this, &self_type::updateState);
        connect(m_mainWindow, &KTextEditor::MainWindow::unhandledShortcutOverride, this, &self_type::handleEsc);
        connect(m_serverManager.get(), &LSPClientServerManager::serverChanged, this, &self_type::updateState);

        m_findDef = actionCollection()->addAction(QStringLiteral("lspclient_find_definition"), this, &self_type::goToDefinition);
        m_findDef->setText(i18n("Go to Definition"));
        m_findDecl = actionCollection()->addAction(QStringLiteral("lspclient_find_declaration"), this, &self_type::goToDeclaration);
        m_findDecl->setText(i18n("Go to Declaration"));
        m_findRef = actionCollection()->addAction(QStringLiteral("lspclient_find_references"), this, &self_type::findReferences);
        m_findRef->setText(i18n("Find References"));
        m_highlight = actionCollection()->addAction(QStringLiteral("lspclient_highlight"), this, &self_type::highlight);
        m_highlight->setText(i18n("Highlight"));
        // perhaps hover suggests to do so on mouse-over,
        // but let's just use a (convenient) action/shortcut for it
        m_hover = actionCollection()->addAction(QStringLiteral("lspclient_hover"), this, &self_type::hover);
        m_hover->setText(i18n("Hover"));

        // general options
        m_complDocOn = actionCollection()->addAction(QStringLiteral("lspclient_completion_doc"), this, &self_type::displayOptionChanged);
        m_complDocOn->setText(i18n("Show selected completion documentation"));
        m_complDocOn->setCheckable(true);
        m_refDeclaration = actionCollection()->addAction(QStringLiteral("lspclient_references_declaration"), this, &self_type::displayOptionChanged);
        m_refDeclaration->setText(i18n("Include declaration in references"));
        m_refDeclaration->setCheckable(true);

        // server control
        m_restartServer = actionCollection()->addAction(QStringLiteral("lspclient_restart_server"), this, &self_type::restartCurrent);
        m_restartServer->setText(i18n("Restart LSP Server"));
        m_restartAll = actionCollection()->addAction(QStringLiteral("lspclient_restart_all"), this, &self_type::restartAll);
        m_restartAll->setText(i18n("Restart All LSP Servers"));

        // popup menu
        auto menu = new KActionMenu(i18n("LSP Client"), this);
        actionCollection()->addAction(QStringLiteral("popup_lspclient"), menu);
        menu->addAction(m_findDef);
        menu->addAction(m_findDecl);
        menu->addAction(m_findRef);
        menu->addAction(m_highlight);
        menu->addAction(m_hover);
        menu->addSeparator();
        menu->addAction(m_complDocOn);
        menu->addAction(m_refDeclaration);
        menu->addSeparator();
        menu->addAction(m_restartServer);
        menu->addAction(m_restartAll);

        // sync with plugin settings if updated
        connect(m_plugin, &LSPClientPlugin::update, this, &self_type::configUpdated);

        // toolview
        m_toolView.reset(mainWin->createToolView(plugin, QStringLiteral("kate_lspclient"),
                                                 KTextEditor::MainWindow::Bottom,
                                                 QIcon::fromTheme(QStringLiteral("application-x-ms-dos-executable")),
                                                 i18n("LSP Client")));
        m_tabWidget = new QTabWidget(m_toolView.get());
        m_tabWidget->setFocusPolicy(Qt::NoFocus);
        m_tabWidget->setTabsClosable(true);
        KAcceleratorManager::setNoAccel(m_tabWidget);
        connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, &self_type::tabCloseRequested);

        // diagnostics tab
        m_diagnosticsTree = new QTreeWidget();
        m_diagnosticsTree->setHeaderHidden(true);
        m_diagnosticsTree->setFocusPolicy(Qt::NoFocus);
        m_diagnosticsTree->setLayoutDirection(Qt::LeftToRight);
        m_diagnosticsTree->setColumnCount(2);
        m_diagnosticsTree->setSortingEnabled(false);
        m_diagnosticsTree->setAlternatingRowColors(true);
        m_tabWidget->addTab(m_diagnosticsTree, i18n("Diagnostics"));
        connect(m_diagnosticsTree, &QTreeWidget::itemClicked, this, &self_type::goToItemLocation);

        configUpdated();
        updateState();

        m_mainWindow->guiFactory()->addClient(this);
    }

    ~LSPClientPluginViewImpl()
    {
        clearAllLocationMarks();
        clearAllDiagnosticsMarks();
        m_mainWindow->guiFactory()->removeClient(this);
    }

    void displayOptionChanged()
    {
        if (m_complDocOn)
            m_completion->setSelectedDocumentation(m_complDocOn->isChecked());
    }

    void configUpdated()
    {
        if (m_complDocOn)
            m_complDocOn->setChecked(m_plugin->m_complDoc);
        if (m_refDeclaration)
            m_refDeclaration->setChecked(m_plugin->m_refDeclaration);
        displayOptionChanged();
    }

    void restartCurrent()
    {
        KTextEditor::View *activeView = m_mainWindow->activeView();
        auto server = m_serverManager->findServer(activeView);
        if (server)
            m_serverManager->restart(server.get());
    }

    void restartAll()
    {
        m_serverManager->restart(nullptr);
    }

    static
    void clearMarks(KTextEditor::Document *doc, RangeCollection & ranges, uint markType)
    {
        KTextEditor::MarkInterface* iface = qobject_cast<KTextEditor::MarkInterface*>(doc);
        if (iface) {
            const QHash<int, KTextEditor::Mark*> marks = iface->marks();
            QHashIterator<int, KTextEditor::Mark*> i(marks);
            while (i.hasNext()) {
                i.next();
                if (i.value()->type & markType) {
                    iface->removeMark(i.value()->line, markType);
                }
            }
        }

        for (auto it = ranges.find(doc); it != ranges.end() && it.key() == doc;) {
            delete it.value();
            it = ranges.erase(it);
        }
    }

    static
    void clearMarks(RangeCollection & ranges, uint markType)
    {
        while (!ranges.empty()) {
            clearMarks(ranges.begin().key(), ranges, markType);
        }
    }

    Q_SLOT void clearAllMarks(KTextEditor::Document *doc)
    {
        clearMarks(doc, m_ranges, RangeData::markType);
        clearMarks(doc, m_diagnosticsRanges, RangeData::markTypeDiagAll);
    }

    void clearAllLocationMarks()
    {
        clearMarks(m_ranges, RangeData::markType);
        // no longer add any again
        m_ownedTree.reset();
        m_markTree.clear();
    }

    void clearAllDiagnosticsMarks()
    {
        clearMarks(m_diagnosticsRanges, RangeData::markTypeDiagAll);
    }

    void addMarks(KTextEditor::Document *doc, QTreeWidgetItem *item, RangeCollection & ranges)
    {
        KTextEditor::MovingInterface* miface = qobject_cast<KTextEditor::MovingInterface*>(doc);
        KTextEditor::MarkInterface* iface = qobject_cast<KTextEditor::MarkInterface*>(doc);
        KTextEditor::View* activeView = m_mainWindow->activeView();
        KTextEditor::ConfigInterface* ciface = qobject_cast<KTextEditor::ConfigInterface*>(activeView);

        if (!miface || !iface)
            return;

        auto url = item->data(0, RangeData::FileUrlRole).toUrl();
        if (url != doc->url())
            return;

        int line = item->data(0, RangeData::StartLineRole).toInt();
        int column = item->data(0, RangeData::StartColumnRole).toInt();
        int endLine = item->data(0, RangeData::EndLineRole).toInt();
        int endColumn = item->data(0, RangeData::EndColumnRole).toInt();
        RangeData::KindEnum kind = (RangeData::KindEnum) item->data(0, RangeData::KindRole).toInt();

        KTextEditor::Range range(line, column, endLine, endColumn);
        KTextEditor::Attribute::Ptr attr(new KTextEditor::Attribute());

        KTextEditor::MarkInterface::MarkTypes markType = RangeData::markType;
        switch (kind) {
        case RangeData::KindEnum::Text:
        {
            // well, it's a bit like searching for something, so re-use that color
            QColor rangeColor = Qt::yellow;
            if (ciface) {
                rangeColor = ciface->configValue(QStringLiteral("search-highlight-color")).value<QColor>();
            }
            attr->setBackground(rangeColor);
            break;
        }
        // FIXME are there any symbolic/configurable ways to pick these colors?
        case RangeData::KindEnum::Read:
            attr->setBackground(Qt::green);
            break;
        case RangeData::KindEnum::Write:
            attr->setBackground(Qt::red);
            break;
        // use underlining for diagnostics to avoid lots of fancy flickering
        case RangeData::KindEnum::Error:
            markType = RangeData::markTypeDiagError;
            attr->setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
            attr->setUnderlineColor(Qt::red);
            break;
        case RangeData::KindEnum::Warning:
            markType = RangeData::markTypeDiagWarning;
            attr->setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
            attr->setUnderlineColor(QColor(255, 128, 0));
            break;
        case RangeData::KindEnum::Information:
        case RangeData::KindEnum::Hint:
        case RangeData::KindEnum::Related:
            markType = RangeData::markTypeDiagOther;
            attr->setUnderlineStyle(QTextCharFormat::DashUnderline);
            attr->setUnderlineColor(Qt::blue);
            break;
        }
        if (activeView) {
            attr->setForeground(activeView->defaultStyleAttribute(KTextEditor::dsNormal)->foreground().color());
        }

        // highlight the range
        KTextEditor::MovingRange* mr = miface->newMovingRange(range);
        mr->setAttribute(attr);
        mr->setZDepth(-90000.0); // Set the z-depth to slightly worse than the selection
        mr->setAttributeOnlyForViews(true);
        ranges.insert(doc, mr);

        // add match mark for range
        const int ps = 32;
        bool handleClick = true;
        switch (markType) {
        case RangeData::markType:
            iface->setMarkDescription(markType, i18n("RangeHighLight"));
            iface->setMarkPixmap(markType, QIcon().pixmap(0, 0));
            handleClick = false;
            break;
        case RangeData::markTypeDiagError:
            iface->setMarkDescription(markType, i18n("Error"));
            iface->setMarkPixmap(markType, diagnosticsIcon(LSPDiagnosticSeverity::Error).pixmap(ps, ps));
            break;
        case RangeData::markTypeDiagWarning:
            iface->setMarkDescription(markType, i18n("Warning"));
            iface->setMarkPixmap(markType, diagnosticsIcon(LSPDiagnosticSeverity::Warning).pixmap(ps, ps));
            break;
        case RangeData::markTypeDiagOther:
            iface->setMarkDescription(markType, i18n("Information"));
            iface->setMarkPixmap(markType, diagnosticsIcon(LSPDiagnosticSeverity::Information).pixmap(ps, ps));
            break;
        default:
            Q_ASSERT(false);
            break;
        }
        iface->addMark(line, markType);

        // ensure runtime match
        auto conn = connect(doc, SIGNAL(aboutToInvalidateMovingInterfaceContent(KTextEditor::Document*)),
            this, SLOT(clearAllMarks(KTextEditor::Document*)), Qt::UniqueConnection);
        Q_ASSERT(conn);
        conn = connect(doc, SIGNAL(aboutToDeleteMovingInterfaceContent(KTextEditor::Document*)),
            this, SLOT(clearAllMarks(KTextEditor::Document*)), Qt::UniqueConnection);
        Q_ASSERT(conn);

        if (handleClick) {
            conn = connect(doc, SIGNAL(markClicked(KTextEditor::Document*, KTextEditor::Mark, bool&)),
                this, SLOT(onMarkClicked(KTextEditor::Document*,KTextEditor::Mark, bool&)), Qt::UniqueConnection);
            Q_ASSERT(conn);
        }
    }

    void addMarks(KTextEditor::Document *doc, QTreeWidget *tree, RangeCollection & ranges)
    {
        // check if already added
        if (ranges.contains(doc))
            return;

        QTreeWidgetItemIterator it(tree, QTreeWidgetItemIterator::All);
        while (*it) {
            addMarks(doc, *it, ranges);
             ++it;
         }
    }

    void goToDocumentLocation(const QUrl & uri, int line, int column)
    {
        KTextEditor::View *activeView = m_mainWindow->activeView();
        if (!activeView || uri.isEmpty() || line < 0 || column < 0)
            return;

        KTextEditor::Document *document = activeView->document();
        KTextEditor::Cursor cdef(line, column);

        if (document && uri == document->url()) {
            activeView->setCursorPosition(cdef);
        } else {
            KTextEditor::View *view = m_mainWindow->openUrl(uri);
            if (view) {
                view->setCursorPosition(cdef);
            }
        }
    }

    void goToItemLocation(QTreeWidgetItem *it)
    {
        if (it) {
            auto url = it->data(0, RangeData::FileUrlRole).toUrl();
            auto line = it->data(0, RangeData::StartLineRole).toInt();
            auto column = it->data(0, RangeData::StartColumnRole).toInt();
            goToDocumentLocation(url, line, column);
        }
    }

    void tabCloseRequested(int index)
    {
        auto widget = m_tabWidget->widget(index);
        if (widget != m_diagnosticsTree) {
            if (widget == m_markTree) {
                clearAllLocationMarks();
            }
            delete widget;
        }
    }

    // local helper to overcome some differences in LSP types
    struct RangeItem
    {
        QUrl uri;
        LSPRange range;
        LSPDocumentHighlightKind kind;
    };

    static
    bool compareRangeItem(const RangeItem & a, const RangeItem & b)
    { return (a.uri < b.uri) || ((a.uri == b.uri) && a.range < b.range); }

    KTextEditor::Document*
    findDocument(const QUrl & url)
    {
        auto views = m_mainWindow->views();
        for (const auto v: views) {
            auto doc = v->document();
            if (doc && doc->url() == url)
                return doc;
        }
        return nullptr;
    }

    void onExpanded(const QModelIndex & index, QTreeWidget * treeWidget)
    {
        auto rootIndex = index.data(RangeData::EndColumnRole).toInt();
        auto rootItem = treeWidget->topLevelItem(rootIndex);

        if (!rootItem || rootItem->data(0, RangeData::KindRole).toBool())
            return;

        KTextEditor::Document *doc = nullptr;
        QScopedPointer<FileLineReader> fr;
        for (int i = 0; i < rootItem->childCount(); i++) {
            auto child = rootItem->child(i);
            if (i == 0) {
                auto url = child->data(0, RangeData::FileUrlRole).toUrl();
                doc = findDocument(url);
                if (!doc) {
                    fr.reset(new FileLineReader(url));
                }
            }
            auto text = child->text(0);
            auto lineno = child->data(0, RangeData::StartLineRole).toInt();
            auto line = doc ? doc->line(lineno) : fr->line(lineno);
            text += line;
            child->setText(0, text);
        }

        // mark as processed
        rootItem->setData(0, RangeData::KindRole, true);
    }

    void fillItemRoles(QTreeWidgetItem * item, const QUrl & url, const LSPRange & range, RangeData::KindEnum kind)
    {
        item->setData(0, RangeData::FileUrlRole, QVariant(url));
        item->setData(0, RangeData::StartLineRole, range.start().line());
        item->setData(0, RangeData::StartColumnRole, range.start().column());
        item->setData(0, RangeData::EndLineRole, range.end().line());
        item->setData(0, RangeData::EndColumnRole, range.end().column());
        item->setData(0, RangeData::KindRole, (int) kind);
    }

    void makeTree(const QVector<RangeItem> & locations)
    {
        // group by url, assuming input is suitably sorted that way
        auto treeWidget = new QTreeWidget();
        treeWidget->setHeaderHidden(true);
        treeWidget->setFocusPolicy(Qt::NoFocus);
        treeWidget->setLayoutDirection(Qt::LeftToRight);
        treeWidget->setColumnCount(1);
        treeWidget->setSortingEnabled(false);

        QUrl lastUrl;
        QTreeWidgetItem *parent = nullptr;
        for (const auto & loc: locations) {
            if (loc.uri != lastUrl) {
                if (parent) {
                    parent->setText(0, QStringLiteral("%1: %2").arg(lastUrl.path()).arg(parent->childCount()));
                }
                lastUrl = loc.uri;
                parent = new QTreeWidgetItem(treeWidget);
                parent->setData(0, RangeData::EndColumnRole, treeWidget->topLevelItemCount() - 1);
            }
            auto item = new QTreeWidgetItem(parent);
            item->setText(0, i18n("Line: %1: ", loc.range.start().line() + 1));
            fillItemRoles(item, loc.uri, loc.range, loc.kind);
        }
        if (parent)
            parent->setText(0, QStringLiteral("%1: %2").arg(lastUrl.path()).arg(parent->childCount()));

        // add line data if file (root) item gets expanded
        auto h = [this, treeWidget] (const QModelIndex & index) { onExpanded(index, treeWidget); };
        connect(treeWidget, &QTreeWidget::expanded, this, h);

        // plain heuristic; auto-expand all when safe and/or useful to do so
        if (treeWidget->topLevelItemCount() <= 2 || locations.size() <= 20) {
            treeWidget->expandAll();
        }


        m_ownedTree.reset(treeWidget);
        m_markTree = treeWidget;
    }

    void showTree(const QString & title)
    {
        // transfer widget from owned to tabwidget
        auto treeWidget = m_ownedTree.take();
        int index = m_tabWidget->addTab(treeWidget, title);
        connect(treeWidget, &QTreeWidget::itemClicked, this, &self_type::goToItemLocation);

        // activate the resulting tab
        m_tabWidget->setCurrentIndex(index);
        m_mainWindow->showToolView(m_toolView.get());
    }

    void showMessage(const QString & text, KTextEditor::Message::MessageType level)
    {
        KTextEditor::View *view = m_mainWindow->activeView();
        if (!view || !view->document()) return;

        auto kmsg = new KTextEditor::Message(text, level);
        kmsg->setPosition(KTextEditor::Message::BottomInView);
        kmsg->setAutoHide(500);
        kmsg->setView(view);
        view->document()->postMessage(kmsg);
    }

    void handleEsc(QEvent *e)
    {
        if (!m_mainWindow) return;

        QKeyEvent *k = static_cast<QKeyEvent *>(e);
        if (k->key() == Qt::Key_Escape && k->modifiers() == Qt::NoModifier) {
            if (!m_ranges.empty()) {
                clearAllLocationMarks();
            } else if (m_toolView->isVisible()) {
                m_mainWindow->hideToolView(m_toolView.get());
            }
        }
    }

    template<typename Handler>
    using LocationRequest = std::function<LSPClientServer::RequestHandle(LSPClientServer &,
        const QUrl & document, const LSPPosition & pos,
        const QObject *context, const Handler & h)>;

    template<typename Handler>
    void positionRequest(const LocationRequest<Handler> & req, const Handler & h)
    {
        KTextEditor::View *activeView = m_mainWindow->activeView();
        auto server = m_serverManager->findServer(activeView);
        if (!server)
            return;

        KTextEditor::Cursor cursor = activeView->cursorPosition();

        clearAllLocationMarks();
        m_req_timeout = false;
        QTimer::singleShot(1000, this, [this] { m_req_timeout = true; });
        m_handle.cancel() = req(*server, activeView->document()->url(),
            {cursor.line(), cursor.column()}, this, h);
    }

    QString currentWord()
    {
        KTextEditor::View *activeView = m_mainWindow->activeView();
        if (activeView) {
            KTextEditor::Cursor cursor = activeView->cursorPosition();
            return activeView->document()->wordAt(cursor);
        } else {
            return QString();
        }
    }

    // some template and function type trickery here, but at least that buck stops here then ...
    template<typename ReplyEntryType, bool doshow = true, typename HandlerType = ReplyHandler<QList<ReplyEntryType>>>
    void processLocations(const QString & title,
        const typename utils::identity<LocationRequest<HandlerType>>::type & req, bool onlyshow,
        const std::function<RangeItem(const ReplyEntryType &)> & itemConverter)
    {
        auto h = [this, title, onlyshow, itemConverter] (const QList<ReplyEntryType> & defs)
        {
            if (defs.count() == 0) {
                showMessage(i18n("No results"), KTextEditor::Message::Information);
            } else {
                // convert to helper type
                QVector<RangeItem> ranges;
                ranges.reserve(defs.size());
                for (const auto & def: defs) {
                    ranges.push_back(itemConverter(def));
                }
                // ... so we can sort it also
                std::stable_sort(ranges.begin(), ranges.end(), compareRangeItem);
                makeTree(ranges);

                if (defs.count() > 1 || onlyshow) {
                    showTree(title);
                }
                // it's not nice to jump to some location if we are too late
                if (!m_req_timeout && !onlyshow) {
                    // assuming here that the first location is the best one
                    const auto &item = itemConverter(defs.at(0));
                    const auto &pos = item.range.start();
                    goToDocumentLocation(item.uri, pos.line(), pos.column());
                }
                // update marks
                updateState();
            }
        };

        positionRequest<HandlerType>(req, h);
    }

    static RangeItem
    locationToRangeItem(const LSPLocation & loc)
    { return {loc.uri, loc.range, LSPDocumentHighlightKind::Text}; }

    void goToDefinition()
    {
        auto title = i18nc("@title:tab", "Definition: %1", currentWord());
        processLocations<LSPLocation>(title, &LSPClientServer::documentDefinition, false, &self_type::locationToRangeItem);
    }

    void goToDeclaration()
    {
        auto title = i18nc("@title:tab", "Declaration: %1", currentWord());
        processLocations<LSPLocation>(title, &LSPClientServer::documentDeclaration, false, &self_type::locationToRangeItem);
    }

    void findReferences()
    {
        auto title = i18nc("@title:tab", "References: %1", currentWord());
        bool decl = m_refDeclaration->isChecked();
        auto req = [decl] (LSPClientServer & server, const QUrl & document, const LSPPosition & pos,
                    const QObject *context, const DocumentDefinitionReplyHandler & h)
        { return server.documentReferences(document, pos, decl, context, h); };

        processLocations<LSPLocation>(title, req, true, &self_type::locationToRangeItem);
    }

    void highlight()
    {
        // determine current url to capture and use later on
        QUrl url;
        const KTextEditor::View* viewForRequest(m_mainWindow->activeView());
        if (viewForRequest && viewForRequest->document()) {
            url = viewForRequest->document()->url();
        }

        auto title = i18nc("@title:tab", "Highlight: %1", currentWord());
        auto converter = [url] (const LSPDocumentHighlight & hl)
        { return RangeItem {url, hl.range, hl.kind}; };

        processLocations<LSPDocumentHighlight, false>(title, &LSPClientServer::documentHighlight, true, converter);
    }

    void hover()
    {
        auto h = [this] (const LSPHover & info)
        {
            // TODO ?? also indicate range in some way ??
            auto text = info.contents.value.length() ? info.contents.value : i18n("No Hover Info");
            showMessage(text, KTextEditor::Message::Information);
        };

        positionRequest<DocumentHoverReplyHandler>(&LSPClientServer::documentHover, h);
    }

    static QTreeWidgetItem*
    getItem(const QTreeWidget *treeWidget, const QUrl & url)
    {
        QTreeWidgetItem *topItem = nullptr;
        for (int i = 0; i < treeWidget->topLevelItemCount(); ++i) {
            auto item = treeWidget->topLevelItem(i);
            if (item->text(0) == url.path()) {
                topItem = item;
                break;
            }
        }
        return topItem;
    }

    Q_SLOT void onMarkClicked(KTextEditor::Document *document, KTextEditor::Mark mark, bool &handled)
    {
        QTreeWidgetItem *topItem = getItem(m_diagnosticsTree, document->url());
        for (int i = 0; i < topItem->childCount(); ++i) {
            auto item = topItem->child(i);
            int line = item->data(0, RangeData::StartLineRole).toInt();
            if (line == mark.line && m_diagnosticsTree) {
                m_diagnosticsTree->scrollToItem(item);
                m_tabWidget->setCurrentWidget(m_diagnosticsTree);
                m_mainWindow->showToolView(m_toolView.get());
                handled = true;
                break;
            }
        }
    }

    void onDiagnostics(const LSPPublishDiagnosticsParams & diagnostics)
    {
        QTreeWidgetItem *topItem = getItem(m_diagnosticsTree, diagnostics.uri);

        if (!topItem) {
            topItem = new QTreeWidgetItem(m_diagnosticsTree);
            topItem->setText(0, diagnostics.uri.path());
        } else {
            qDeleteAll(topItem->takeChildren());
        }

        for (const auto & diag : diagnostics.diagnostics) {
            auto item = new QTreeWidgetItem(topItem);
            QString source;
            if (diag.source.length()) {
                source = QStringLiteral("[%1] ").arg(diag.source);
            }
            item->setIcon(0, diagnosticsIcon(diag.severity));
            item->setText(0, source + diag.message);
            fillItemRoles(item, diagnostics.uri, diag.range, diag.severity);
            const auto &related = diag.relatedInformation;
            if (!related.location.uri.isEmpty()) {
                auto relatedItem = new QTreeWidgetItem(item);
                relatedItem->setText(0, related.message);
                auto basename = QFileInfo(related.location.uri.path()).fileName();
                relatedItem->setText(1, QStringLiteral("%1:%2").arg(basename).arg(related.location.range.start().line()));
                fillItemRoles(relatedItem, related.location.uri, related.location.range, RangeData::KindEnum::Related);
                item->setExpanded(true);
            }
        }

        // TODO perhaps add some custom delegate that only shows 1 line
        // and only the whole text when item selected ??
        topItem->setExpanded(true);
        topItem->setHidden(topItem->childCount() == 0);

        m_diagnosticsTree->resizeColumnToContents(1);
        m_diagnosticsTree->resizeColumnToContents(0);
        m_diagnosticsTree->scrollToItem(topItem);

        updateState();
    }

    void updateState()
    {
        KTextEditor::View *activeView = m_mainWindow->activeView();
        auto server = m_serverManager->findServer(activeView);
        bool defEnabled = false, declEnabled = false, refEnabled = false, hoverEnabled = false, highlightEnabled = false;

        if (server) {
            const auto& caps = server->capabilities();
            defEnabled = caps.definitionProvider;
            // FIXME no real official protocol way to detect, so enable anyway
            declEnabled = caps.declarationProvider || true;
            refEnabled = caps.referencesProvider;
            hoverEnabled = caps.hoverProvider;
            highlightEnabled = caps.documentHighlightProvider;

            connect(server.get(), &LSPClientServer::publishDiagnostics,
                this, &self_type::onDiagnostics, Qt::UniqueConnection);
        }

        if (m_findDef)
            m_findDef->setEnabled(defEnabled);
        if (m_findDecl)
            m_findDecl->setEnabled(declEnabled);
        if (m_findRef)
            m_findRef->setEnabled(refEnabled);
        if (m_highlight)
            m_highlight->setEnabled(highlightEnabled);
        if (m_hover)
            m_hover->setEnabled(hoverEnabled);
        if (m_complDocOn)
            m_complDocOn->setEnabled(server);
        if (m_restartServer)
            m_restartServer->setEnabled(server);

        // update completion with relevant server
        m_completion->setServer(server);

        displayOptionChanged();
        updateCompletion(activeView, server.get());

        // update marks if applicable
        if (m_markTree && activeView)
            addMarks(activeView->document(), m_markTree, m_ranges);
        if (m_diagnosticsTree && activeView) {
            clearMarks(activeView->document(), m_diagnosticsRanges, RangeData::markTypeDiagAll);
            addMarks(activeView->document(), m_diagnosticsTree, m_diagnosticsRanges);
        }
    }

    void viewDestroyed(QObject *view)
    {
        m_completionViews.remove(static_cast<KTextEditor::View *>(view));
    }

    void updateCompletion(KTextEditor::View *view, LSPClientServer * server)
    {
        bool registered = m_completionViews.contains(view);

        KTextEditor::CodeCompletionInterface *cci = qobject_cast<KTextEditor::CodeCompletionInterface *>(view);

        if (!cci) {
            return;
        }

        if (!registered && server && server->capabilities().completionProvider.provider) {
            qCInfo(LSPCLIENT) << "registering cci";
            cci->registerCompletionModel(m_completion.get());
            m_completionViews.insert(view);

            connect(view, &KTextEditor::View::destroyed, this,
                &self_type::viewDestroyed,
                Qt::UniqueConnection);
        }

        if (registered && !server) {
            qCInfo(LSPCLIENT) << "unregistering cci";
            cci->unregisterCompletionModel(m_completion.get());
            m_completionViews.remove(view);
        }
    }
};

QObject*
LSPClientPluginView::new_(LSPClientPlugin *plugin, KTextEditor::MainWindow *mainWin)
{
    return new LSPClientPluginViewImpl(plugin, mainWin);
}

#include "lspclientpluginview.moc"
