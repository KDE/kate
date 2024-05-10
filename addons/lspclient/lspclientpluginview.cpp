/*
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>

    SPDX-License-Identifier: MIT
*/

#include "lspclientpluginview.h"
#include "diagnostics/diagnosticview.h"
#include "gotosymboldialog.h"
#include "inlayhints.h"
#include "lspclientcompletion.h"
#include "lspclienthover.h"
#include "lspclientplugin.h"
#include "lspclientservermanager.h"
#include "lspclientsymbolview.h"
#include "lspclientutils.h"
#include "texthint/KateTextHintManager.h"

#include "lspclient_debug.h"

#include <KAcceleratorManager>
#include <KActionCollection>
#include <KActionMenu>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KStandardAction>
#include <KXMLGUIFactory>

#include <KTextEditor/Document>
#include <KTextEditor/MainWindow>
#include <KTextEditor/Message>
#include <KTextEditor/SessionConfigInterface>
#include <KTextEditor/View>
#include <KXMLGUIClient>

#include <ktexteditor/editor.h>
#include <ktexteditor/movingrange.h>
#include <ktexteditor_version.h>

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QJsonObject>
#include <QKeyEvent>
#include <QKeySequence>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScopeGuard>
#include <QSet>
#include <QStandardItem>
#include <QStringDecoder>
#include <QStyledItemDelegate>
#include <QTimer>
#include <QTreeView>
#include <unordered_map>
#include <utility>

#include <drawing_utils.h>
#include <ktexteditor_utils.h>

namespace RangeData
{
enum {
    // preserve UserRole for generic use where needed
    FileUrlRole = Qt::UserRole + 1,
    RangeRole,
    KindRole,
};

class KindEnum
{
public:
    enum _kind {
        Text = static_cast<int>(LSPDocumentHighlightKind::Text),
        Read = static_cast<int>(LSPDocumentHighlightKind::Read),
        Write = static_cast<int>(LSPDocumentHighlightKind::Write),
    };

    KindEnum(int v)
    {
        m_value = _kind(v);
    }

    KindEnum(LSPDocumentHighlightKind hl)
        : KindEnum(static_cast<_kind>(hl))
    {
    }

    KindEnum(LSPDiagnosticSeverity sev)
        : KindEnum(_kind(10 + static_cast<int>(sev)))
    {
    }

    operator _kind()
    {
        return m_value;
    }

private:
    _kind m_value;
};

static constexpr KTextEditor::Document::MarkTypes markType = KTextEditor::Document::markType31;
}

KTextEditor::Document *findDocument(KTextEditor::MainWindow *mainWindow, const QUrl &url)
{
    auto views = mainWindow->views();
    for (const auto v : views) {
        auto doc = v->document();
        if (doc && doc->url() == url) {
            return doc;
        }
    }
    return nullptr;
}

// helper to read lines from unopened documents
// lightweight and does not require additional symbols
class FileLineReader
{
    QFile file;
    int lastLineNo = -1;
    QString lastLine;

public:
    FileLineReader(const QUrl &url)
        : file(url.toLocalFile())
    {
        file.open(QIODevice::ReadOnly);
    }

    // called with non-descending lineno
    QString line(int lineno)
    {
        if (lineno == lastLineNo) {
            return lastLine;
        }
        while (file.isOpen() && !file.atEnd()) {
            auto line = file.readLine();
            if (++lastLineNo == lineno) {
                auto toUtf16 = QStringDecoder(QStringDecoder::Utf8);
                QString text = toUtf16(line);
                if (toUtf16.hasError()) {
                    text = QString::fromLatin1(line);
                }

                text = text.trimmed();
                lastLine = text;
                return text;
            }
        }
        return QString();
    }
};

class CloseAllowedMessageBox : public QMessageBox
{
public:
    using QMessageBox::QMessageBox;

    void closeEvent(QCloseEvent *) override
    {
    }
};

// tab widget that closes tabs on middle click
class ClosableTabWidget : public QTabWidget
{
public:
    ClosableTabWidget(QWidget *parent = nullptr)
        : QTabWidget(parent){};

    void mousePressEvent(QMouseEvent *event) override
    {
        QTabWidget::mousePressEvent(event);

        if (event->button() == Qt::MiddleButton) {
            int id = tabBar()->tabAt(event->pos());
            if (id >= 0) {
                Q_EMIT tabCloseRequested(id);
            }
        }
    }
};

class LocationTreeDelegate : public QStyledItemDelegate
{
public:
    LocationTreeDelegate(QObject *parent, const QFont &font)
        : QStyledItemDelegate(parent)
        , m_monoFont(font)
    {
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        auto options = option;
        initStyleOption(&options, index);

        painter->save();

        QString text = index.data().toString();

        options.text = QString(); // clear old text
        options.widget->style()->drawControl(QStyle::CE_ItemViewItem, &options, painter, options.widget);

        QList<QTextLayout::FormatRange> formats;
        if (!index.parent().isValid()) {
            int lastSlash = text.lastIndexOf(QLatin1Char('/'));
            if (lastSlash != -1) {
                QTextCharFormat fmt;
                fmt.setFontWeight(QFont::Bold);
                formats.append({.start = lastSlash + 1, .length = int(text.length() - (lastSlash + 1)), .format = fmt});
            }
        } else {
            // mind translation; let's hope/assume the colon survived
            int nextColon = text.indexOf(QLatin1Char(':'), 0);
            if (nextColon != 1 && nextColon < text.size()) {
                nextColon = text.indexOf(QLatin1Char(':'), nextColon + 1);
            }
            if (nextColon != -1) {
                QTextCharFormat fmt;
                fmt.setFont(m_monoFont);
                int codeStart = nextColon + 1;
                formats.append({.start = codeStart, .length = int(text.length() - codeStart), .format = fmt});
            }
        }

        /** There might be an icon? Make sure to not draw over it **/
        auto textRectX = options.widget->style()->subElementRect(QStyle::SE_ItemViewItemText, &options, options.widget).x();
        auto width = textRectX - options.rect.x();
        painter->translate(width, 0);

        Utils::paintItemViewText(painter, text, options, formats);

        painter->restore();
    }

private:
    QFont m_monoFont;
};

/**
 * @brief This is just a helper class that provides "underline" on Ctrl + click
 */
class CtrlHoverFeedback : public QObject
{
    Q_OBJECT
public:
    void highlight(KTextEditor::View *activeView)
    {
        // sanity checksQString
        if (!activeView) {
            return;
        }

        auto doc = activeView->document();
        if (!doc) {
            return;
        }

        if (!w) {
            return;
        }

        w->setCursor(Qt::PointingHandCursor);

        // underline the hovered word
        auto &mr = docs[doc];
        if (mr) {
            mr->setRange(range);
        } else {
            mr.reset(doc->newMovingRange(range));
            connect(doc, &KTextEditor::Document::aboutToInvalidateMovingInterfaceContent, this, &CtrlHoverFeedback::clearMovingRange, Qt::UniqueConnection);
            connect(doc, &KTextEditor::Document::aboutToDeleteMovingInterfaceContent, this, &CtrlHoverFeedback::clearMovingRange, Qt::UniqueConnection);
        }

        static KTextEditor::Attribute::Ptr attr;
        if (!attr) {
            attr = new KTextEditor::Attribute;
            attr->setUnderlineStyle(QTextCharFormat::SingleUnderline);
        }
        mr->setAttribute(attr);
    }

    void clear(KTextEditor::View *activeView)
    {
        if (activeView) {
            auto doc = activeView->document();
            auto it = docs.find(doc);
            if (it != docs.end()) {
                auto &mr = it->second;
                if (mr) {
                    mr->setRange(KTextEditor::Range::invalid());
                }
            }
        }
        if (w && w->cursor() != Qt::IBeamCursor) {
            w->setCursor(Qt::IBeamCursor);
        }
        w.clear();
    }

    void setRangeAndWidget(const KTextEditor::Range &r, QWidget *wid)
    {
        range = r;
        w = wid;
    }

    bool isValid() const
    {
        return !w.isNull();
    }

private:
    Q_SLOT void clearMovingRange(KTextEditor::Document *doc)
    {
        if (doc) {
            auto it = docs.find(doc);
            if (it != docs.end()) {
                docs.erase(it);
            }
        }
    }

private:
    QPointer<QWidget> w;
    std::unordered_map<KTextEditor::Document *, std::unique_ptr<KTextEditor::MovingRange>> docs;
    KTextEditor::Range range;
};

class LSPClientPluginViewImpl : public QObject, public KXMLGUIClient
{
    Q_OBJECT

    typedef LSPClientPluginViewImpl self_type;

    LSPClientPlugin *m_plugin;
    KTextEditor::MainWindow *m_mainWindow;
    std::shared_ptr<LSPClientServerManager> m_serverManager;
    std::unique_ptr<LSPClientCompletion> m_completion;
    KateTextHintProvider m_textHintprovider;
    std::unique_ptr<LSPClientHover> m_hover;
    std::unique_ptr<LSPClientSymbolView> m_symbolView;

    QPointer<QAction> m_findDef;
    QPointer<QAction> m_findDecl;
    QPointer<QAction> m_findTypeDef;
    QPointer<QAction> m_findRef;
    QPointer<QAction> m_findImpl;
    QPointer<QAction> m_triggerHighlight;
    QPointer<QAction> m_triggerSymbolInfo;
    QPointer<QAction> m_triggerGotoSymbol;
    QPointer<QAction> m_triggerFormat;
    QPointer<QAction> m_triggerRename;
    QPointer<QAction> m_expandSelection;
    QPointer<QAction> m_shrinkSelection;
    QPointer<QAction> m_complDocOn;
    QPointer<QAction> m_signatureHelp;
    QPointer<QAction> m_refDeclaration;
    QPointer<QAction> m_complParens;
    QPointer<QAction> m_autoHover;
    QPointer<QAction> m_onTypeFormatting;
    QPointer<QAction> m_incrementalSync;
    QPointer<QAction> m_highlightGoto;
    QPointer<QAction> m_diagnostics;
    QPointer<QAction> m_messages;
    QPointer<QAction> m_closeDynamic;
    QPointer<QAction> m_restartServer;
    QPointer<QAction> m_restartAll;
    QPointer<QAction> m_switchSourceHeader;
    QPointer<QAction> m_expandMacro;
    QPointer<QAction> m_memoryUsage;
    QPointer<QAction> m_inlayHints;
    QPointer<KActionMenu> m_requestCodeAction;

    // toolview
    std::unique_ptr<QWidget> m_toolView;
    QPointer<ClosableTabWidget> m_tabWidget;
    // applied search ranges
    typedef QMultiHash<KTextEditor::Document *, KTextEditor::MovingRange *> RangeCollection;
    RangeCollection m_ranges;
    // applied marks
    typedef QSet<KTextEditor::Document *> DocumentCollection;
    DocumentCollection m_marks;
    // modelis either owned by tree added to tabwidget or owned here
    std::unique_ptr<QStandardItemModel> m_ownedModel;
    // in either case, the model that directs applying marks/ranges
    QPointer<QStandardItemModel> m_markModel;
    // goto definition and declaration jump list is more a menu than a
    // search result, so let's not keep adding new tabs for those
    // previous tree for definition result
    QPointer<QTreeView> m_defTree;
    // ... and for declaration
    QPointer<QTreeView> m_declTree;
    // ... and for type definition
    QPointer<QTreeView> m_typeDefTree;

    // views on which completions have been registered
    QList<KTextEditor::View *> m_completionViews;

    // outstanding request
    LSPClientServer::RequestHandle m_handle;
    // timeout on request
    bool m_req_timeout = false;

    // accept incoming applyEdit
    bool m_accept_edit = false;
    // characters to trigger format request
    QList<QChar> m_onTypeFormattingTriggers;

    // ongoing workDoneProgress
    // list of (enhanced server token, progress begin)
    QList<std::pair<QString, LSPWorkDoneProgressParams>> m_workDoneProgress;

    CtrlHoverFeedback m_ctrlHoverFeedback = {};

    SemanticHighlighter m_semHighlightingManager;
    InlayHintsManager m_inlayHintsHandler;

    class LSPDiagnosticProvider : public DiagnosticsProvider
    {
    public:
        LSPDiagnosticProvider(KTextEditor::MainWindow *mainWin, LSPClientPluginViewImpl *lsp)
            : DiagnosticsProvider(mainWin, lsp)
            , m_lsp(lsp)
        {
            name = i18n("LSP");
        }

        QJsonObject suppressions(KTextEditor::Document *doc) const override
        {
            auto config = m_lsp->m_serverManager->findServerConfig(doc);
            if (config.isObject()) {
                auto config_o = config.toObject();
                return config_o.value(QStringLiteral("suppressions")).toObject();
            }
            return {};
        }

    private:
        LSPClientPluginViewImpl *m_lsp;
    };

    LSPDiagnosticProvider m_diagnosticProvider;

public:
    LSPClientPluginViewImpl(LSPClientPlugin *plugin, KTextEditor::MainWindow *mainWin, std::shared_ptr<LSPClientServerManager> serverManager)
        : QObject(mainWin)
        , m_plugin(plugin)
        , m_mainWindow(mainWin)
        , m_serverManager(std::move(serverManager))
        , m_completion(LSPClientCompletion::new_(m_serverManager))
        , m_textHintprovider(m_mainWindow, this)
        , m_hover(LSPClientHover::new_(m_serverManager, &m_textHintprovider))
        , m_symbolView(LSPClientSymbolView::new_(plugin, mainWin, m_serverManager))
        , m_semHighlightingManager(m_serverManager)
        , m_inlayHintsHandler(m_serverManager, this)
        , m_diagnosticProvider(mainWin, this)
    {
        KXMLGUIClient::setComponentName(QStringLiteral("lspclient"), i18n("LSP Client"));
        setXMLFile(QStringLiteral("ui.rc"));

        connect(m_mainWindow, &KTextEditor::MainWindow::viewChanged, this, &self_type::updateState);
        connect(m_mainWindow, &KTextEditor::MainWindow::unhandledShortcutOverride, this, &self_type::handleEsc);
        connect(m_serverManager.get(), &LSPClientServerManager::serverChanged, this, &self_type::onServerChanged);
        connect(m_plugin, &LSPClientPlugin::showMessage, this, &self_type::onShowMessage);
        connect(m_serverManager.get(), &LSPClientServerManager::serverShowMessage, this, &self_type::onMessage);
        connect(m_serverManager.get(), &LSPClientServerManager::serverLogMessage, this, [this](LSPClientServer *server, LSPLogMessageParams params) {
            switch (params.type) {
            case LSPMessageType::Error:
                params.message.prepend(QStringLiteral("[Error] "));
                break;
            case LSPMessageType::Warning:
                params.message.prepend(QStringLiteral("[Warn] "));
                break;
            case LSPMessageType::Info:
                params.message.prepend(QStringLiteral("[Info] "));
                break;
            case LSPMessageType::Log:
                break;
            }
            params.type = LSPMessageType::Log;
            onMessage(server, params);
        });
        connect(m_serverManager.get(), &LSPClientServerManager::serverWorkDoneProgress, this, &self_type::onWorkDoneProgress);
        connect(m_serverManager.get(), &LSPClientServerManager::showMessageRequest, this, &self_type::showMessageRequest);

        m_findDef = actionCollection()->addAction(QStringLiteral("lspclient_find_definition"), this, &self_type::goToDefinition);
        m_findDef->setText(i18n("Go to Definition"));
        m_findDecl = actionCollection()->addAction(QStringLiteral("lspclient_find_declaration"), this, &self_type::goToDeclaration);
        m_findDecl->setText(i18n("Go to Declaration"));
        m_findTypeDef = actionCollection()->addAction(QStringLiteral("lspclient_find_type_definition"), this, &self_type::goToTypeDefinition);
        m_findTypeDef->setText(i18n("Go to Type Definition"));
        m_findRef = actionCollection()->addAction(QStringLiteral("lspclient_find_references"), this, &self_type::findReferences);
        m_findRef->setText(i18n("Find References"));
        actionCollection()->setDefaultShortcut(m_findRef, QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_L));
        m_findImpl = actionCollection()->addAction(QStringLiteral("lspclient_find_implementations"), this, &self_type::findImplementation);
        m_findImpl->setText(i18n("Find Implementations"));
        m_triggerHighlight = actionCollection()->addAction(QStringLiteral("lspclient_highlight"), this, &self_type::highlight);
        m_triggerHighlight->setText(i18n("Highlight"));
        m_triggerSymbolInfo = actionCollection()->addAction(QStringLiteral("lspclient_symbol_info"), this, &self_type::symbolInfo);
        m_triggerSymbolInfo->setText(i18n("Symbol Info"));
        m_triggerGotoSymbol = actionCollection()->addAction(QStringLiteral("lspclient_goto_workspace_symbol"), this, &self_type::gotoWorkSpaceSymbol);
        m_triggerGotoSymbol->setText(i18n("Search and Go to Symbol"));
        actionCollection()->setDefaultShortcut(m_triggerGotoSymbol, QKeySequence(Qt::ALT | Qt::CTRL | Qt::Key_P));
        m_triggerFormat = actionCollection()->addAction(QStringLiteral("lspclient_format"), this, [this]() {
            format();
        });
        m_triggerFormat->setText(i18n("Format"));
        actionCollection()->setDefaultShortcut(m_triggerFormat, QKeySequence(QStringLiteral("Ctrl+T, F"), QKeySequence::PortableText));
        m_triggerRename = actionCollection()->addAction(QStringLiteral("lspclient_rename"), this, &self_type::rename);
        m_triggerRename->setText(i18n("Rename"));
        actionCollection()->setDefaultShortcut(m_triggerRename, QKeySequence(Qt::Key_F2));
        m_expandSelection = actionCollection()->addAction(QStringLiteral("lspclient_expand_selection"), this, &self_type::expandSelection);
        m_expandSelection->setText(i18n("Expand Selection"));
        actionCollection()->setDefaultShortcut(m_expandSelection, QKeySequence(Qt::CTRL | Qt::Key_2));
        m_shrinkSelection = actionCollection()->addAction(QStringLiteral("lspclient_shrink_selection"), this, &self_type::shrinkSelection);
        m_shrinkSelection->setText(i18n("Shrink Selection"));
        actionCollection()->setDefaultShortcut(m_shrinkSelection, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_2));
        m_switchSourceHeader = actionCollection()->addAction(QStringLiteral("lspclient_clangd_switchheader"), this, &self_type::clangdSwitchSourceHeader);
        m_switchSourceHeader->setText(i18n("Switch Source Header"));
        actionCollection()->setDefaultShortcut(m_switchSourceHeader, Qt::Key_F12);
        m_expandMacro = actionCollection()->addAction(QStringLiteral("lspclient_rust_analyzer_expand_macro"), this, &self_type::rustAnalyzerExpandMacro);
        m_expandMacro->setText(i18n("Expand Macro"));
        m_requestCodeAction = actionCollection()->add<KActionMenu>(QStringLiteral("lspclient_code_action"));
        m_requestCodeAction->setText(i18n("Code Action"));
        QList<QKeySequence> scuts;
        scuts << QKeySequence(Qt::ALT | Qt::Key_Return) << QKeySequence(Qt::ALT | Qt::Key_Enter);
        actionCollection()->setDefaultShortcuts(m_requestCodeAction, scuts);
        connect(m_requestCodeAction, &QWidgetAction::triggered, this, [this] {
            auto view = m_mainWindow->activeView();
            if (m_requestCodeAction && view) {
                const QPoint p = view->cursorPositionCoordinates();
                m_requestCodeAction->menu()->exec(view->mapToGlobal(p));
            }
        });
        connect(m_requestCodeAction->menu(), &QMenu::aboutToShow, this, &self_type::requestCodeAction);

        // general options
        m_complDocOn = actionCollection()->addAction(QStringLiteral("lspclient_completion_doc"), this, &self_type::displayOptionChanged);
        m_complDocOn->setText(i18n("Show selected completion documentation"));
        m_complDocOn->setCheckable(true);
        m_signatureHelp = actionCollection()->addAction(QStringLiteral("lspclient_signature_help"), this, &self_type::displayOptionChanged);
        m_signatureHelp->setText(i18n("Enable signature help with auto completion"));
        m_signatureHelp->setCheckable(true);
        m_refDeclaration = actionCollection()->addAction(QStringLiteral("lspclient_references_declaration"), this, &self_type::displayOptionChanged);
        m_refDeclaration->setText(i18n("Include declaration in references"));
        m_refDeclaration->setCheckable(true);
        m_complParens = actionCollection()->addAction(QStringLiteral("lspclient_completion_parens"), this, &self_type::displayOptionChanged);
        m_complParens->setText(i18n("Add parentheses upon function completion"));
        m_complParens->setCheckable(true);
        m_autoHover = actionCollection()->addAction(QStringLiteral("lspclient_auto_hover"), this, &self_type::displayOptionChanged);
        m_autoHover->setText(i18n("Show hover information"));
        m_autoHover->setCheckable(true);
        m_onTypeFormatting = actionCollection()->addAction(QStringLiteral("lspclient_type_formatting"), this, &self_type::displayOptionChanged);
        m_onTypeFormatting->setText(i18n("Format on typing"));
        m_onTypeFormatting->setCheckable(true);
        m_incrementalSync = actionCollection()->addAction(QStringLiteral("lspclient_incremental_sync"), this, &self_type::displayOptionChanged);
        m_incrementalSync->setText(i18n("Incremental document synchronization"));
        m_incrementalSync->setCheckable(true);
        m_highlightGoto = actionCollection()->addAction(QStringLiteral("lspclient_highlight_goto"), this, &self_type::displayOptionChanged);
        m_highlightGoto->setText(i18n("Highlight goto location"));
        m_highlightGoto->setCheckable(true);
        m_inlayHints = actionCollection()->addAction(QStringLiteral("lspclient_inlay_hint"), this, [this](bool checked) {
            if (!checked) {
                m_inlayHintsHandler.disable();
            }
            displayOptionChanged();
        });
        m_inlayHints->setCheckable(true);
        m_inlayHints->setText(i18n("Show Inlay Hints"));

        // diagnostics
        m_diagnostics = actionCollection()->addAction(QStringLiteral("lspclient_diagnostics"), this, &self_type::displayOptionChanged);
        m_diagnostics->setText(i18n("Show Diagnostics Notifications"));
        m_diagnostics->setCheckable(true);

        // messages
        m_messages = actionCollection()->addAction(QStringLiteral("lspclient_messages"), this, &self_type::displayOptionChanged);
        m_messages->setText(i18n("Show Messages"));
        m_messages->setCheckable(true);

        // extra
        m_memoryUsage = actionCollection()->addAction(QStringLiteral("lspclient_clangd_memoryusage"), this, &self_type::clangdMemoryUsage);
        m_memoryUsage->setText(i18n("Server Memory Usage"));

        // server control and misc actions
        m_closeDynamic = actionCollection()->addAction(QStringLiteral("lspclient_close_dynamic"), this, &self_type::closeDynamic);
        m_closeDynamic->setText(i18n("Close All Dynamic Reference Tabs"));
        m_restartServer = actionCollection()->addAction(QStringLiteral("lspclient_restart_server"), this, &self_type::restartCurrent);
        m_restartServer->setText(i18n("Restart LSP Server"));
        m_restartAll = actionCollection()->addAction(QStringLiteral("lspclient_restart_all"), this, &self_type::restartAll);
        m_restartAll->setText(i18n("Restart All LSP Servers"));

        QAction *goToAction = new QAction(i18n("Go To"));
        actionCollection()->addAction(QStringLiteral("lspclient_goto_menu"), goToAction);
        QMenu *goTo = new QMenu();
        goToAction->setMenu(goTo);
        goTo->addActions({m_findDecl, m_findDef, m_findTypeDef, m_switchSourceHeader});

        QAction *lspOtherAction = new QAction(i18n("LSP Client"));
        actionCollection()->addAction(QStringLiteral("lspclient_other_menu"), lspOtherAction);
        QMenu *lspOther = new QMenu();
        lspOtherAction->setMenu(lspOther);
        lspOther->addAction(m_findImpl);
        lspOther->addAction(m_triggerHighlight);
        lspOther->addAction(m_triggerGotoSymbol);
        lspOther->addAction(m_expandMacro);
        lspOther->addAction(m_triggerFormat);
        lspOther->addAction(m_triggerSymbolInfo);
        lspOther->addSeparator();
        lspOther->addAction(m_closeDynamic);
        lspOther->addSeparator();
        lspOther->addAction(m_restartServer);
        lspOther->addAction(m_restartAll);

        // more options
        auto moreOptions = new KActionMenu(i18n("More options"), this);
        lspOther->addSeparator();
        lspOther->addAction(moreOptions);
        moreOptions->addAction(m_complDocOn);
        moreOptions->addAction(m_signatureHelp);
        moreOptions->addAction(m_refDeclaration);
        moreOptions->addAction(m_complParens);
        moreOptions->addAction(m_autoHover);
        moreOptions->addAction(m_onTypeFormatting);
        moreOptions->addAction(m_incrementalSync);
        moreOptions->addAction(m_highlightGoto);
        moreOptions->addAction(m_inlayHints);
        moreOptions->addSeparator();
        moreOptions->addAction(m_diagnostics);
        moreOptions->addAction(m_messages);
        moreOptions->addSeparator();
        moreOptions->addAction(m_memoryUsage);

        // sync with plugin settings if updated
        connect(m_plugin, &LSPClientPlugin::update, this, &self_type::configUpdated);

        m_diagnosticProvider.setObjectName(QStringLiteral("LSPDiagnosticProvider"));
        connect(&m_diagnosticProvider, &DiagnosticsProvider::requestFixes, this, &self_type::fixDiagnostic);
        connect(&m_textHintprovider, &KateTextHintProvider::textHintRequested, this, &self_type::onTextHint);

        connect(m_mainWindow, &KTextEditor::MainWindow::viewCreated, this, &self_type::onViewCreated);

        connect(this, &self_type::ctrlClickDefRecieved, this, &self_type::onCtrlMouseMove);

        configUpdated();
        updateState();

        m_mainWindow->guiFactory()->addClient(this);
    }

    void initToolView()
    {
        if (m_tabWidget || m_toolView) {
            return;
        }
        // toolview
        m_toolView.reset(m_mainWindow->createToolView(m_plugin,
                                                      QStringLiteral("kate_lspclient"),
                                                      KTextEditor::MainWindow::Bottom,
                                                      QIcon::fromTheme(QStringLiteral("format-text-code")),
                                                      i18n("LSP")));
        m_tabWidget = new ClosableTabWidget(m_toolView.get());
        m_toolView->layout()->addWidget(m_tabWidget);
        m_tabWidget->setFocusPolicy(Qt::NoFocus);
        m_tabWidget->setTabsClosable(true);
        KAcceleratorManager::setNoAccel(m_tabWidget);
        connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, &self_type::tabCloseRequested);
        connect(m_tabWidget, &QTabWidget::currentChanged, this, &self_type::tabChanged);
    }

    void onViewCreated(KTextEditor::View *view)
    {
        if (view && view->focusProxy()) {
            view->focusProxy()->installEventFilter(this);
        }
    }

    KTextEditor::View *viewFromWidget(QWidget *widget)
    {
        if (widget) {
            return qobject_cast<KTextEditor::View *>(widget->parent());
        }
        return nullptr;
    }

    /**
     * @brief normal word range queried from doc->wordRangeAt() will not include
     * full header from a #include line. For example in line "#include <abc/some>"
     * we get either abc or some. But for Ctrl + click we need to highlight it as
     * one thing, so this function expands the range from wordRangeAt() to do that.
     */
    static void expandToFullHeaderRange(KTextEditor::Range &range, QStringView lineText)
    {
        auto expandRangeTo = [lineText, &range](QChar c, int startPos) {
            int end = lineText.indexOf(c, startPos);
            if (end > -1) {
                auto startC = range.start();
                startC.setColumn(startPos);
                auto endC = range.end();
                endC.setColumn(end);
                range.setStart(startC);
                range.setEnd(endC);
            }
        };

        int angleBracketPos = lineText.indexOf(QLatin1Char('<'), 7);
        if (angleBracketPos > -1) {
            expandRangeTo(QLatin1Char('>'), angleBracketPos + 1);
        } else {
            int startPos = lineText.indexOf(QLatin1Char('"'), 7);
            if (startPos > -1) {
                expandRangeTo(QLatin1Char('"'), startPos + 1);
            }
        }
    }

    bool eventFilter(QObject *obj, QEvent *event) override
    {
        if (!obj->isWidgetType()) {
            return QObject::eventFilter(obj, event);
        }

        // common stuff that we need for both events
        auto viewInternal = static_cast<QWidget *>(obj);
        KTextEditor::View *v = viewFromWidget(viewInternal);
        if (!v) {
            return false;
        }

        if (event->type() == QEvent::Leave) {
            if (m_ctrlHoverFeedback.isValid()) {
                m_ctrlHoverFeedback.clear(v);
            }
            return QObject::eventFilter(obj, event);
        }

        if (event->type() != QEvent::MouseButtonPress && event->type() != QEvent::MouseMove) {
            // we are only concerned with mouse events for now :)
            return QObject::eventFilter(obj, event);
        }

        auto mouseEvent = static_cast<QMouseEvent *>(event);
        const auto coords = viewInternal->mapTo(v, mouseEvent->pos());
        const auto cur = v->coordinatesToCursor(coords);
        // there isn't much we can do now, just bail out
        // if we have selection, and the click is on the selection ignore
        // the event as we might block actions like ctrl-drag of text
        if (!cur.isValid() || v->selectionRange().contains(cur)) {
            return false;
        }

        // The user pressed Ctrl + Click
        if (event->type() == QEvent::MouseButtonPress) {
            if (mouseEvent->button() == Qt::LeftButton && mouseEvent->modifiers() == Qt::ControlModifier) {
                // must set cursor else we will be jumping somewhere else!!
                v->setCursorPosition(cur);
                m_ctrlHoverFeedback.clear(v);
                goToDefinition();
            }
        }
        // The user is hovering with Ctrl pressed
        else if (event->type() == QEvent::MouseMove) {
            if (mouseEvent->modifiers() == Qt::ControlModifier) {
                auto doc = v->document();
                auto range = doc->wordRangeAt(cur);
                if (!range.isEmpty()) {
                    // check if we are in #include
                    // and expand the word range
                    auto lineText = doc->line(range.start().line());
                    if (lineText.startsWith(QLatin1String("#include")) && range.start().column() > 7) {
                        expandToFullHeaderRange(range, lineText);
                    }

                    m_ctrlHoverFeedback.setRangeAndWidget(range, viewInternal);
                    // this will not go anywhere actually, but just signal whether we have a definition
                    // Also, please rethink very hard if you are going to reuse this method. It's made
                    // only for Ctrl+Hover
                    processCtrlMouseHover(cur);
                } else {
                    // if there is no word, unset the cursor and remove the highlight
                    m_ctrlHoverFeedback.clear(v);
                }
            } else {
                // simple mouse move, make sure to unset the cursor
                // and remove the highlight
                m_ctrlHoverFeedback.clear(v);
            }
        }

        return false;
    }

    ~LSPClientPluginViewImpl() override
    {
        m_mainWindow->guiFactory()->removeClient(this);

        // unregister all code-completion providers, else we might crash
        for (auto view : std::as_const(m_completionViews)) {
            view->unregisterCompletionModel(m_completion.get());
        }

        clearAllLocationMarks();
    }

    void configureTreeView(QTreeView *treeView)
    {
        treeView->setHeaderHidden(true);
        treeView->setFocusPolicy(Qt::NoFocus);
        treeView->setLayoutDirection(Qt::LeftToRight);
        treeView->setSortingEnabled(false);
        treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);

        // styling
        treeView->setItemDelegate(new LocationTreeDelegate(treeView, Utils::editorFont()));

        // context menu
        treeView->setContextMenuPolicy(Qt::CustomContextMenu);
        auto menu = new QMenu(treeView);
        menu->addAction(i18n("Expand All"), treeView, &QTreeView::expandAll);
        menu->addAction(i18n("Collapse All"), treeView, &QTreeView::collapseAll);
        auto h = [treeView, menu](const QPoint &p) {
            menu->popup(treeView->viewport()->mapToGlobal(p));
        };
        connect(treeView, &QTreeView::customContextMenuRequested, h);
    }

    void displayOptionChanged()
    {
        m_serverManager->setIncrementalSync(m_incrementalSync->isChecked());
        // use snippets if and only if parentheses are requested
        auto &clientCaps = m_serverManager->clientCapabilities();
        auto snippetSupport = m_complParens->isChecked();
        if (clientCaps.snippetSupport != snippetSupport) {
            clientCaps.snippetSupport = snippetSupport;
            // restart servers to make it apply
            // (not likely frequently toggled)
            restartAll();
        }
        updateState();
    }

    void configUpdated()
    {
        if (m_complDocOn) {
            m_complDocOn->setChecked(m_plugin->m_complDoc);
        }
        if (m_signatureHelp) {
            m_signatureHelp->setChecked(m_plugin->m_signatureHelp);
        }
        if (m_refDeclaration) {
            m_refDeclaration->setChecked(m_plugin->m_refDeclaration);
        }
        if (m_complParens) {
            m_complParens->setChecked(m_plugin->m_complParens);
        }
        if (m_autoHover) {
            m_autoHover->setChecked(m_plugin->m_autoHover);
        }
        if (m_onTypeFormatting) {
            m_onTypeFormatting->setChecked(m_plugin->m_onTypeFormatting);
        }
        if (m_incrementalSync) {
            m_incrementalSync->setChecked(m_plugin->m_incrementalSync);
        }
        if (m_highlightGoto) {
            m_highlightGoto->setChecked(m_plugin->m_highlightGoto);
        }
        if (m_diagnostics) {
            m_diagnostics->setChecked(m_plugin->m_diagnostics);
        }
        if (m_messages) {
            m_messages->setChecked(m_plugin->m_messages);
        }
        if (m_completion) {
            m_completion->setAutoImport(m_plugin->m_autoImport);
        }
        if (m_inlayHints) {
            m_inlayHints->setChecked(m_plugin->m_inlayHints);
        }
        displayOptionChanged();
    }

    void restartCurrent()
    {
        KTextEditor::View *activeView = m_mainWindow->activeView();
        auto server = m_serverManager->findServer(activeView);
        if (server) {
            m_serverManager->restart(server.get());
        }
    }

    void restartAll()
    {
        m_serverManager->restart(nullptr);
    }

    static void clearMarks(KTextEditor::Document *doc, RangeCollection &ranges, DocumentCollection &docs, uint markType)
    {
        if (docs.contains(doc)) {
            const QHash<int, KTextEditor::Mark *> marks = doc->marks();
            QHashIterator<int, KTextEditor::Mark *> i(marks);
            while (i.hasNext()) {
                i.next();
                if (i.value()->type & markType) {
                    doc->removeMark(i.value()->line, markType);
                }
            }
            docs.remove(doc);
        }

        for (auto it = ranges.find(doc); it != ranges.end() && it.key() == doc;) {
            delete it.value();
            it = ranges.erase(it);
        }
    }

    static void clearMarks(RangeCollection &ranges, DocumentCollection &docs, uint markType)
    {
        while (!ranges.empty()) {
            clearMarks(ranges.begin().key(), ranges, docs, markType);
        }
    }

    Q_SLOT void clearAllMarks(KTextEditor::Document *doc)
    {
        clearMarks(doc, m_ranges, m_marks, RangeData::markType);
    }

    void clearAllLocationMarks()
    {
        clearMarks(m_ranges, m_marks, RangeData::markType);
        // no longer add any again
        m_ownedModel.reset();
        m_markModel.clear();
    }

    void addMarks(KTextEditor::Document *doc, QStandardItem *item, RangeCollection *ranges, DocumentCollection *docs)
    {
        Q_ASSERT(item);

        // only consider enabled items
        if (!(item->flags() & Qt::ItemIsEnabled)) {
            return;
        }

        auto url = item->data(RangeData::FileUrlRole).toUrl();
        // document url could end up empty while in intermediate reload state
        // (and then it might match a parent item with no RangeData at all)
        if (url != doc->url() || url.isEmpty()) {
            return;
        }

        KTextEditor::Range range = item->data(RangeData::RangeRole).value<LSPRange>();
        if (!range.isValid() || range.isEmpty()) {
            return;
        }
        auto line = range.start().line();
        RangeData::KindEnum kind = RangeData::KindEnum(item->data(RangeData::KindRole).toInt());

        KTextEditor::Attribute::Ptr attr;

        bool enabled = false;
        switch (kind) {
        case RangeData::KindEnum::Text: {
            // well, it's a bit like searching for something, so re-use that color
            static KTextEditor::Attribute::Ptr searchAttr;
            if (!searchAttr) {
                searchAttr = new KTextEditor::Attribute();
                const auto theme = KTextEditor::Editor::instance()->theme();
                QColor rangeColor = theme.editorColor(KSyntaxHighlighting::Theme::SearchHighlight);
                searchAttr->setBackground(rangeColor);
                searchAttr->setForeground(QBrush(theme.textColor(KSyntaxHighlighting::Theme::Normal)));
            }
            attr = searchAttr;
            enabled = true;
            break;
        }
        // FIXME are there any symbolic/configurable ways to pick these colors?
        case RangeData::KindEnum::Read: {
            static KTextEditor::Attribute::Ptr greenAttr;
            if (!greenAttr) {
                const auto theme = KTextEditor::Editor::instance()->theme();
                greenAttr = new KTextEditor::Attribute();
                greenAttr->setBackground(Qt::green);
                greenAttr->setForeground(QBrush(theme.textColor(KSyntaxHighlighting::Theme::Normal)));
            }
            attr = greenAttr;
            enabled = true;
            break;
        }
        case RangeData::KindEnum::Write: {
            static KTextEditor::Attribute::Ptr redAttr;
            if (!redAttr) {
                const auto theme = KTextEditor::Editor::instance()->theme();
                redAttr = new KTextEditor::Attribute();
                redAttr->setBackground(Qt::red);
                redAttr->setForeground(QBrush(theme.textColor(KSyntaxHighlighting::Theme::Normal)));
            }
            attr = redAttr;
            enabled = true;
            break;
        }
        }

        if (!attr) {
            qWarning() << "Unexpected null attr";
        }

        // highlight the range
        if (enabled && ranges && attr) {
            KTextEditor::MovingRange *mr = doc->newMovingRange(range);
            mr->setZDepth(-90000.0); // Set the z-depth to slightly worse than the selection
            mr->setAttribute(attr);
            mr->setAttributeOnlyForViews(true);
            ranges->insert(doc, mr);
        }

        auto *iface = doc;
        KTextEditor::Document::MarkTypes markType = RangeData::markType;
        // add match mark for range
        switch (markType) {
        case RangeData::markType:
            iface->setMarkDescription(markType, i18n("RangeHighLight"));
            iface->setMarkIcon(markType, QIcon());
            enabled = true;
            break;
        default:
            Q_ASSERT(false);
            break;
        }
        if (enabled && docs) {
            iface->addMark(line, markType);
            docs->insert(doc);
        }

        connect(doc, &KTextEditor::Document::aboutToInvalidateMovingInterfaceContent, this, &self_type::clearAllMarks, Qt::UniqueConnection);
        connect(doc, &KTextEditor::Document::aboutToDeleteMovingInterfaceContent, this, &self_type::clearAllMarks, Qt::UniqueConnection);

        // reload might save/restore marks before/after above signals, so let's clear before that
        connect(doc, &KTextEditor::Document::aboutToReload, this, &self_type::clearAllMarks, Qt::UniqueConnection);
    }

    void addMarksRec(KTextEditor::Document *doc, QStandardItem *item, RangeCollection *ranges, DocumentCollection *docs)
    {
        Q_ASSERT(item);
        addMarks(doc, item, ranges, docs);
        for (int i = 0; i < item->rowCount(); ++i) {
            addMarksRec(doc, item->child(i), ranges, docs);
        }
    }

    void addMarks(KTextEditor::Document *doc, QStandardItemModel *treeModel, RangeCollection &ranges, DocumentCollection &docs)
    {
        // check if already added
        auto oranges = ranges.contains(doc) ? nullptr : &ranges;
        auto odocs = docs.contains(doc) ? nullptr : &docs;

        if (!oranges && !odocs) {
            return;
        }

        Q_ASSERT(treeModel);
        addMarksRec(doc, treeModel->invisibleRootItem(), oranges, odocs);
    }

    void goToDocumentLocation(const QUrl &uri, const KTextEditor::Range &location)
    {
        int line = location.start().line();
        int column = location.start().column();
        KTextEditor::View *activeView = m_mainWindow->activeView();
        if (!activeView || uri.isEmpty() || line < 0 || column < 0) {
            return;
        }

        KTextEditor::Document *document = activeView->document();
        KTextEditor::Cursor cdef(line, column);

        KTextEditor::View *targetView = nullptr;
        if (document && uri == document->url()) {
            targetView = activeView;
        } else {
            targetView = m_mainWindow->openUrl(uri);
        }
        if (targetView) {
            // save current position for location history
            Utils::addPositionToHistory(activeView->document()->url(), activeView->cursorPosition(), m_mainWindow);
            // save the position to which we are jumping in location history
            Utils::addPositionToHistory(targetView->document()->url(), cdef, m_mainWindow);
            targetView->setCursorPosition(cdef);
            highlightLandingLocation(targetView, location);
        }
    }

    /**
     * @brief give a short 1sec temporary highlight where you land
     */
    void highlightLandingLocation(KTextEditor::View *view, const KTextEditor::Range &location)
    {
        if (!m_highlightGoto || !m_highlightGoto->isChecked()) {
            return;
        }
        auto doc = view->document();
        if (!doc) {
            return;
        }
        auto mr = doc->newMovingRange(location);
        KTextEditor::Attribute::Ptr attr(new KTextEditor::Attribute);
        attr->setUnderlineStyle(QTextCharFormat::SingleUnderline);
        mr->setView(view);
        mr->setAttribute(attr);
        QTimer::singleShot(1000, doc, [mr] {
            mr->setRange(KTextEditor::Range::invalid());
            delete mr;
        });
    }

    static QModelIndex getPrimaryModelIndex(QModelIndex index)
    {
        // in case of a multiline diagnostics item, a split secondary line has no data set
        // so we need to go up to the primary parent item
        if (!index.data(RangeData::RangeRole).isValid() && index.parent().data(RangeData::RangeRole).isValid()) {
            return index.parent();
        }
        return index;
    }

    void goToItemLocation(const QModelIndex &_index)
    {
        auto index = getPrimaryModelIndex(_index);
        auto url = index.data(RangeData::FileUrlRole).toUrl();
        auto start = index.data(RangeData::RangeRole).value<LSPRange>();
        goToDocumentLocation(url, start);
    }

    void fixDiagnostic(const QUrl url, const Diagnostic &diagnostic, const QVariant &data)
    {
        KTextEditor::View *activeView = m_mainWindow->activeView();
        QPointer<KTextEditor::Document> document = activeView->document();
        auto server = m_serverManager->findServer(activeView);
        if (!server || !document) {
            return;
        }

        auto executeCodeAction = [this, server](LSPCodeAction action, std::shared_ptr<LSPClientRevisionSnapshot> snapshot) {
            // apply edit before command
            applyWorkspaceEdit(action.edit, snapshot.get());
            executeServerCommand(server, action.command);
        };

        // only engage action if active document matches diagnostic document
        if (url != document->url()) {
            return;
        }

        // store some things to find item safely later on
        // QPersistentModelIndex pindex(index);
        std::shared_ptr<LSPClientRevisionSnapshot> snapshot(m_serverManager->snapshot(server.get()));
        auto h = [url, snapshot, executeCodeAction, this, data](const QList<LSPCodeAction> &actions) {
            // add actions below diagnostic item
            QList<DiagnosticFix> fixes;
            for (const auto &action : actions) {
                DiagnosticFix fix;
                fix.fixTitle = action.title;
                fix.fixCallback = [executeCodeAction, action, snapshot] {
                    executeCodeAction(action, snapshot);
                };
                fixes << fix;
            }
            Q_EMIT m_diagnosticProvider.fixesAvailable(fixes, data);
        };

        auto range = activeView->selectionRange();
        if (!range.isValid()) {
            range = {activeView->cursorPosition(), activeView->cursorPosition()};
        }
        server->documentCodeAction(url, range, {}, {diagnostic}, this, h);
    }

    bool tabCloseRequested(int index)
    {
        auto widget = m_tabWidget->widget(index);
        if (m_markModel && widget == m_markModel->parent()) {
            clearAllLocationMarks();
        }
        delete widget;

        if (m_tabWidget->count() == 0) {
            m_toolView.release()->deleteLater();
        }

        return true;
    }

    void tabChanged(int index)
    {
        // reset to regular foreground
        m_tabWidget->tabBar()->setTabTextColor(index, QColor());
    }

    void closeDynamic()
    {
        if (m_tabWidget) {
            for (int i = 0; i < m_tabWidget->count();) {
                // if so deemed suitable, tab will be spared and not closed
                if (!tabCloseRequested(i)) {
                    ++i;
                }
            }
        }
    }

    // local helper to overcome some differences in LSP types
    struct RangeItem {
        QUrl uri;
        LSPRange range;
        LSPDocumentHighlightKind kind;

        bool isValid() const
        {
            return uri.isValid() && range.isValid();
        }
    };

    static bool compareRangeItem(const RangeItem &a, const RangeItem &b)
    {
        return (a.uri < b.uri) || ((a.uri == b.uri) && a.range < b.range);
    }

    // provide Qt::DisplayRole (text) line lazily;
    // only find line's text content when so requested
    // This may then involve opening reading some file, at which time
    // all items for that file will be resolved in one go.
    struct LineItem : public QStandardItem {
        KTextEditor::MainWindow *m_mainWindow;

        LineItem(KTextEditor::MainWindow *mainWindow)
            : m_mainWindow(mainWindow)
        {
        }

        QVariant data(int role = Qt::UserRole + 1) const override
        {
            auto rootItem = this->parent();
            if (role != Qt::DisplayRole || !rootItem) {
                return QStandardItem::data(role);
            }

            auto line = data(Qt::UserRole);
            // either of these mean we tried to obtain line already
            if (line.isValid() || rootItem->data(RangeData::KindRole).toBool()) {
                return QStandardItem::data(role).toString().append(line.toString());
            }

            KTextEditor::Document *doc = nullptr;
            std::unique_ptr<FileLineReader> fr;
            for (int i = 0; i < rootItem->rowCount(); i++) {
                auto child = rootItem->child(i);
                if (i == 0) {
                    auto url = child->data(RangeData::FileUrlRole).toUrl();
                    doc = findDocument(m_mainWindow, url);
                    if (!doc) {
                        fr.reset(new FileLineReader(url));
                    }
                }
                auto lineno = child->data(RangeData::RangeRole).value<LSPRange>().start().line();
                auto line = doc ? doc->line(lineno) : fr->line(lineno);
                child->setData(line, Qt::UserRole);
            }

            // mark as processed
            rootItem->setData(RangeData::KindRole, true);

            // should work ok
            return data(role);
        }
    };

    static void
    fillItemRoles(QStandardItem *item, const QUrl &url, const LSPRange _range, RangeData::KindEnum kind, const LSPClientRevisionSnapshot *snapshot = nullptr)
    {
        auto range = snapshot ? transformRange(url, *snapshot, _range) : _range;
        item->setData(QVariant(url), RangeData::FileUrlRole);
        QVariant vrange;
        vrange.setValue(range);
        item->setData(vrange, RangeData::RangeRole);
        item->setData(static_cast<int>(kind), RangeData::KindRole);
    }

    QString getProjectBaseDir()
    {
        QObject *project = m_mainWindow->pluginView(QStringLiteral("kateprojectplugin"));
        if (project) {
            auto baseDir = project->property("projectBaseDir").toString();
            if (!baseDir.endsWith(QLatin1Char('/'))) {
                return baseDir + QLatin1Char('/');
            }

            return baseDir;
        }

        return {};
    }

    QString shortenPath(QString projectBaseDir, QString url)
    {
        if (!projectBaseDir.isEmpty() && url.startsWith(projectBaseDir)) {
            return url.mid(projectBaseDir.length());
        }

        return url;
    }

    void makeTree(const QList<RangeItem> &locations, const LSPClientRevisionSnapshot *snapshot)
    {
        // group by url, assuming input is suitably sorted that way
        auto treeModel = new QStandardItemModel();
        treeModel->setColumnCount(1);

        QString baseDir = getProjectBaseDir();
        QUrl lastUrl;
        QStandardItem *parent = nullptr;
        for (const auto &loc : locations) {
            // ensure we create a parent, if not already there (bug 427270) or we have a different url
            if (!parent || loc.uri != lastUrl) {
                if (parent) {
                    parent->setText(QStringLiteral("%1: %2").arg(shortenPath(baseDir, lastUrl.toLocalFile())).arg(parent->rowCount()));
                }
                lastUrl = loc.uri;
                parent = new QStandardItem();
                treeModel->appendRow(parent);
            }
            auto item = new LineItem(m_mainWindow);
            parent->appendRow(item);
            // add partial display data; line will be added by item later on
            item->setText(i18n("Line: %1: ", loc.range.start().line() + 1));
            fillItemRoles(item, loc.uri, loc.range, loc.kind, snapshot);
        }
        if (parent) {
            parent->setText(QStringLiteral("%1: %2").arg(shortenPath(baseDir, lastUrl.toLocalFile())).arg(parent->rowCount()));
        }

        // plain heuristic; mark for auto-expand all when safe and/or useful to do so
        if (treeModel->rowCount() <= 2 || locations.size() <= 20) {
            treeModel->invisibleRootItem()->setData(true, RangeData::KindRole);
        }

        m_ownedModel.reset(treeModel);
        m_markModel = treeModel;
    }

    void showTree(const QString &title, QPointer<QTreeView> *targetTree)
    {
        // create toolview if not present
        if (!m_tabWidget) {
            initToolView();
        }

        // clean up previous target if any
        if (targetTree && *targetTree) {
            int index = m_tabWidget->indexOf(*targetTree);
            if (index >= 0) {
                tabCloseRequested(index);
            }
        }

        // setup view
        auto treeView = new QTreeView();
        configureTreeView(treeView);

        // transfer model from owned to tree and that in turn to tabwidget
        auto treeModel = m_ownedModel.release();
        treeView->setModel(treeModel);
        treeModel->setParent(treeView);
        int index = m_tabWidget->addTab(treeView, title);
        connect(treeView, &QTreeView::clicked, this, &self_type::goToItemLocation);

        if (treeModel->invisibleRootItem()->data(RangeData::KindRole).toBool()) {
            treeView->expandAll();
        }

        // track for later cleanup
        if (targetTree) {
            *targetTree = treeView;
        }

        // activate the resulting tab
        m_tabWidget->setCurrentIndex(index);
        m_mainWindow->showToolView(m_toolView.get());
    }

    void showMessage(const QString &text, KTextEditor::Message::MessageType level)
    {
        KTextEditor::View *view = m_mainWindow->activeView();
        if (!view || !view->document()) {
            return;
        }

        auto kmsg = new KTextEditor::Message(text, level);
        kmsg->setPosition(KTextEditor::Message::BottomInView);
        kmsg->setAutoHide(500);
        kmsg->setView(view);
        view->document()->postMessage(kmsg);
    }

    void handleEsc(QEvent *e)
    {
        if (!m_mainWindow) {
            return;
        }

        QKeyEvent *k = static_cast<QKeyEvent *>(e);
        if (k->key() == Qt::Key_Escape && k->modifiers() == Qt::NoModifier) {
            if (!m_ranges.empty()) {
                clearAllLocationMarks();
            } else if (m_toolView && m_toolView->isVisible()) {
                m_mainWindow->hideToolView(m_toolView.get());
            }
        }
    }

    template<typename Handler>
    using LocationRequest = std::function<
        LSPClientServer::RequestHandle(LSPClientServer &, const QUrl &document, const LSPPosition &pos, const QObject *context, const Handler &h)>;

    template<typename Handler>
    void positionRequest(const LocationRequest<Handler> &req,
                         const Handler &h,
                         std::unique_ptr<LSPClientRevisionSnapshot> *snapshot = nullptr,
                         KTextEditor::Cursor cur = KTextEditor::Cursor::invalid())
    {
        KTextEditor::View *activeView = m_mainWindow->activeView();
        auto server = m_serverManager->findServer(activeView);
        if (!server) {
            return;
        }

        // track revision if requested
        if (snapshot) {
            snapshot->reset(m_serverManager->snapshot(server.get()));
        }

        KTextEditor::Cursor cursor = cur.isValid() ? cur : activeView->cursorPosition();

        clearAllLocationMarks();
        m_req_timeout = false;
        QTimer::singleShot(1000, this, [this] {
            m_req_timeout = true;
        });
        m_handle.cancel() = req(*server, activeView->document()->url(), {cursor.line(), cursor.column()}, this, h);
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

    Q_SIGNAL void ctrlClickDefRecieved(const RangeItem &range);

    Q_SLOT void onCtrlMouseMove(const RangeItem &range)
    {
        if (range.isValid()) {
            if (m_ctrlHoverFeedback.isValid()) {
                m_ctrlHoverFeedback.highlight(m_mainWindow->activeView());
            }
        }
    }

    // some template and function type trickery here, but at least that buck stops here then ...
    template<typename ReplyEntryType, bool doshow = true, typename HandlerType = ReplyHandler<QList<ReplyEntryType>>>
    void processLocations(const QString &title,
                          const typename utils::identity<LocationRequest<HandlerType>>::type &req,
                          bool onlyshow,
                          const std::function<RangeItem(const ReplyEntryType &)> &itemConverter,
                          QPointer<QTreeView> *targetTree = nullptr)
    {
        // no capture for move only using initializers available (yet), so shared outer type
        // the additional level of indirection is so it can be 'filled-in' after lambda creation
        std::shared_ptr<std::unique_ptr<LSPClientRevisionSnapshot>> s(new std::unique_ptr<LSPClientRevisionSnapshot>);
        auto h = [this, title, onlyshow, itemConverter, targetTree, s](const QList<ReplyEntryType> &defs) {
            if (defs.count() == 0) {
                showMessage(i18n("No results"), KTextEditor::Message::Information);
            } else {
                // convert to helper type
                QList<RangeItem> ranges;
                ranges.reserve(defs.size());
                for (const auto &def : defs) {
                    ranges.push_back(itemConverter(def));
                }
                // ... so we can sort it also
                std::stable_sort(ranges.begin(), ranges.end(), compareRangeItem);
                makeTree(ranges, s.get()->get());

                // assuming that reply ranges refer to revision when submitted
                // (not specified anyway in protocol/reply)
                if (defs.count() > 1 || onlyshow) {
                    showTree(title, targetTree);
                }
                // it's not nice to jump to some location if we are too late
                if (!m_req_timeout && !onlyshow) {
                    // assuming here that the first location is the best one
                    const auto &item = itemConverter(defs.at(0));
                    goToDocumentLocation(item.uri, item.range);
                    // forego mark and such if only a single destination
                    if (defs.count() == 1) {
                        clearAllLocationMarks();
                    }
                }
                // update marks
                updateMarks();
            }
        };

        positionRequest<HandlerType>(req, h, s.get());
    }

    void processCtrlMouseHover(const KTextEditor::Cursor &cursor)
    {
        auto h = [this](const QList<LSPLocation> &defs) {
            if (defs.isEmpty()) {
                return;
            } else {
                const auto item = locationToRangeItem(defs.at(0));
                Q_EMIT this->ctrlClickDefRecieved(item);
                return;
            }
        };

        using Handler = std::function<void(const QList<LSPLocation> &)>;
        auto request = &LSPClientServer::documentDefinition;
        positionRequest<Handler>(request, h, nullptr, cursor);
    }

    static RangeItem locationToRangeItem(const LSPLocation &loc)
    {
        return {.uri = loc.uri, .range = loc.range, .kind = LSPDocumentHighlightKind::Text};
    }

    void goToDefinition()
    {
        auto title = i18nc("@title:tab", "Definition: %1", currentWord());
        processLocations<LSPLocation>(title, &LSPClientServer::documentDefinition, false, &self_type::locationToRangeItem, &m_defTree);
    }

    void goToDeclaration()
    {
        auto title = i18nc("@title:tab", "Declaration: %1", currentWord());
        processLocations<LSPLocation>(title, &LSPClientServer::documentDeclaration, false, &self_type::locationToRangeItem, &m_declTree);
    }

    void goToTypeDefinition()
    {
        auto title = i18nc("@title:tab", "Type Definition: %1", currentWord());
        processLocations<LSPLocation>(title, &LSPClientServer::documentTypeDefinition, false, &self_type::locationToRangeItem, &m_typeDefTree);
    }

    void findReferences()
    {
        auto title = i18nc("@title:tab", "References: %1", currentWord());
        bool decl = m_refDeclaration->isChecked();
        // clang-format off
        auto req = [decl](LSPClientServer &server, const QUrl &document, const LSPPosition &pos, const QObject *context, const DocumentDefinitionReplyHandler &h)
        { return server.documentReferences(document, pos, decl, context, h); };
        // clang-format on

        processLocations<LSPLocation>(title, req, true, &self_type::locationToRangeItem);
    }

    void findImplementation()
    {
        auto title = i18nc("@title:tab", "Implementation: %1", currentWord());
        processLocations<LSPLocation>(title, &LSPClientServer::documentImplementation, true, &self_type::locationToRangeItem);
    }

    void highlight()
    {
        // determine current url to capture and use later on
        QUrl url;
        const KTextEditor::View *viewForRequest(m_mainWindow->activeView());
        if (viewForRequest && viewForRequest->document()) {
            url = viewForRequest->document()->url();
        }

        auto title = i18nc("@title:tab", "Highlight: %1", currentWord());
        auto converter = [url](const LSPDocumentHighlight &hl) {
            return RangeItem{.uri = url, .range = hl.range, .kind = hl.kind};
        };

        processLocations<LSPDocumentHighlight, false>(title, &LSPClientServer::documentHighlight, true, converter);
    }

    void symbolInfo()
    {
        // trigger manually the normally automagic hover
        if (auto activeView = m_mainWindow->activeView()) {
            m_hover->showTextHint(activeView, activeView->cursorPosition(), true);
        }
    }

    void requestCodeAction()
    {
        if (!m_requestCodeAction)
            return;
        m_requestCodeAction->menu()->clear();

        KTextEditor::View *activeView = m_mainWindow->activeView();
        if (!activeView) {
            m_requestCodeAction->menu()->addAction(i18n("No Actions"))->setEnabled(false);
            return;
        }

        KTextEditor::Document *document = activeView->document();
        auto server = m_serverManager->findServer(activeView);
        auto range = activeView->selectionRange();
        if (!range.isValid()) {
            range = activeView->document()->wordRangeAt(activeView->cursorPosition());
        }
        if (!server || !document || !range.isValid()) {
            m_requestCodeAction->menu()->addAction(i18n("No Actions"))->setEnabled(false);
            return;
        }

        QPointer<QAction> loadingAction = m_requestCodeAction->menu()->addAction(i18n("Loading..."));
        loadingAction->setEnabled(false);

        // store some things to find item safely later on
        std::shared_ptr<LSPClientRevisionSnapshot> snapshot(m_serverManager->snapshot(server.get()));
        auto h = [this, snapshot, server, loadingAction](const QList<LSPCodeAction> &actions) {
            auto menu = m_requestCodeAction->menu();
            // clearing menu also hides it, and so added actions end up not shown
            if (actions.isEmpty()) {
                menu->addAction(i18n("No Actions"))->setEnabled(false);
            }
            for (const auto &action : actions) {
                auto text = action.kind.size() ? QStringLiteral("[%1] %2").arg(action.kind).arg(action.title) : action.title;
                menu->addAction(text, this, [this, action, snapshot, server]() {
                    applyWorkspaceEdit(action.edit, snapshot.get());
                    executeServerCommand(server, action.command);
                });
            }
            if (loadingAction) {
                menu->removeAction(loadingAction);
            }
        };
        server->documentCodeAction(document->url(), range, {}, {}, this, h);
    }

    void executeServerCommand(std::shared_ptr<LSPClientServer> server, const LSPCommand &command)
    {
        if (!command.command.isEmpty()) {
            // accept edit requests that may be sent to execute command
            m_accept_edit = true;
            // but only for a short time
            QTimer::singleShot(2000, this, [this] {
                m_accept_edit = false;
            });
            server->executeCommand(command);
        }
    }

    static void applyEdits(KTextEditor::Document *doc, const LSPClientRevisionSnapshot *snapshot, const QList<LSPTextEdit> &edits)
    {
        ::applyEdits(doc, snapshot, edits);
    }

    void applyEdits(const QUrl &url, const LSPClientRevisionSnapshot *snapshot, const QList<LSPTextEdit> &edits)
    {
        auto document = findDocument(m_mainWindow, url);
        if (!document) {
            KTextEditor::View *view = m_mainWindow->openUrl(url);
            if (view) {
                document = view->document();
            }
        }
        applyEdits(document, snapshot, edits);
    }

    void applyWorkspaceEdit(const LSPWorkspaceEdit &edit, const LSPClientRevisionSnapshot *snapshot)
    {
        auto currentView = m_mainWindow->activeView();
        // edits may be in changes or documentChanges
        // the latter is handled in a sneaky way, but not announced in capabilities
        for (auto it = edit.changes.begin(); it != edit.changes.end(); ++it) {
            applyEdits(it.key(), snapshot, it.value());
        }
        // ... as/though the document version is not (yet) taken into account
        for (auto &change : edit.documentChanges) {
            applyEdits(change.textDocument.uri, snapshot, change.edits);
        }
        if (currentView) {
            m_mainWindow->activateView(currentView->document());
        }
    }

    void onApplyEdit(const LSPApplyWorkspaceEditParams &edit, const ApplyEditReplyHandler &h, bool &handled)
    {
        if (handled) {
            return;
        }
        handled = true;

        if (m_accept_edit) {
            qCInfo(LSPCLIENT) << "applying edit" << edit.label;
            applyWorkspaceEdit(edit.edit, nullptr);
        } else {
            qCInfo(LSPCLIENT) << "ignoring edit";
        }
        h({.applied = m_accept_edit, .failureReason = QString()});
    }

    template<typename Collection>
    void checkEditResult(const Collection &c)
    {
        if (c.empty()) {
            showMessage(i18n("No edits"), KTextEditor::Message::Information);
        }
    }

    void delayCancelRequest(LSPClientServer::RequestHandle &&h, int timeout_ms = 4000)
    {
        QTimer::singleShot(timeout_ms, this, [h]() mutable {
            h.cancel();
        });
    }

    void format(QChar lastChar = QChar(), bool save = false)
    {
        KTextEditor::View *activeView = m_mainWindow->activeView();
        QPointer<KTextEditor::Document> document = activeView ? activeView->document() : nullptr;
        auto server = m_serverManager->findServer(activeView);
        if (!server || !document) {
            return;
        }

        int tabSize = 4;
        bool insertSpaces = true;
        tabSize = document->configValue(QStringLiteral("tab-width")).toInt();
        insertSpaces = document->configValue(QStringLiteral("replace-tabs")).toBool();

        // sigh, no move initialization capture ...
        // (again) assuming reply ranges wrt revisions submitted at this time
        std::shared_ptr<LSPClientRevisionSnapshot> snapshot(m_serverManager->snapshot(server.get()));
        auto h = [this, document, snapshot, lastChar, save](const QList<LSPTextEdit> &edits) {
            if (lastChar.isNull()) {
                checkEditResult(edits);
            }
            if (document) {
                // Must clear formatting triggers here otherwise on applying edits we
                // might end up triggering formatting again ending up in an infinite loop
                auto savedTriggers = m_onTypeFormattingTriggers;
                m_onTypeFormattingTriggers.clear();
                applyEdits(document, snapshot.get(), edits);
                m_onTypeFormattingTriggers = savedTriggers;
                if (save) {
                    disconnect(document, &KTextEditor::Document::documentSavedOrUploaded, this, &self_type::formatOnSave);
                    document->documentSave();
                    connect(document, &KTextEditor::Document::documentSavedOrUploaded, this, &self_type::formatOnSave);
                }
            }
        };

        auto options = LSPFormattingOptions{.tabSize = tabSize, .insertSpaces = insertSpaces, .extra = QJsonObject()};
        auto handle = !lastChar.isNull()
            ? server->documentOnTypeFormatting(document->url(), activeView->cursorPosition(), lastChar, options, this, h)
            : (activeView->selection() ? server->documentRangeFormatting(document->url(), activeView->selectionRange(), options, this, h)
                                       : server->documentFormatting(document->url(), options, this, h));
        delayCancelRequest(std::move(handle));
    }

    void rename()
    {
        KTextEditor::View *activeView = m_mainWindow->activeView();
        QPointer<KTextEditor::Document> document = activeView ? activeView->document() : nullptr;
        auto server = m_serverManager->findServer(activeView);
        if (!server || !document) {
            return;
        }

        QString wordUnderCursor = document->wordAt(activeView->cursorPosition());
        if (wordUnderCursor.isEmpty()) {
            return;
        }

        bool ok = false;
        // results are typically (too) limited
        // due to server implementation or limited view/scope
        // so let's add a disclaimer that it's not our fault
        QString newName = QInputDialog::getText(activeView,
                                                i18nc("@title:window", "Rename"),
                                                i18nc("@label:textbox", "New name (caution: not all references may be replaced)"),
                                                QLineEdit::Normal,
                                                wordUnderCursor,
                                                &ok);
        if (!ok) {
            return;
        }

        std::shared_ptr<LSPClientRevisionSnapshot> snapshot(m_serverManager->snapshot(server.get()));
        auto h = [this, snapshot](const LSPWorkspaceEdit &edit) {
            if (edit.documentChanges.empty()) {
                checkEditResult(edit.changes);
            }
            applyWorkspaceEdit(edit, snapshot.get());
        };
        auto handle = server->documentRename(document->url(), activeView->cursorPosition(), newName, this, h);
        delayCancelRequest(std::move(handle));
    }

    void expandSelection()
    {
        changeSelection(true);
    }

    void shrinkSelection()
    {
        changeSelection(false);
    }

    void changeSelection(bool expand)
    {
        KTextEditor::View *activeView = m_mainWindow->activeView();
        QPointer<KTextEditor::Document> document = activeView ? activeView->document() : nullptr;
        auto server = m_serverManager->findServer(activeView);
        if (!server || !document) {
            return;
        }

        auto h = [this, activeView, expand](const QList<std::shared_ptr<LSPSelectionRange>> &reply) {
            if (reply.isEmpty()) {
                showMessage(i18n("No results"), KTextEditor::Message::Information);
            }

            auto cursors = activeView->cursorPositions();

            if (cursors.size() != reply.size()) {
                showMessage(i18n("Not enough results"), KTextEditor::Message::Information);
            }

            auto selections = activeView->selectionRanges();
            QList<KTextEditor::Range> ret;

            for (int i = 0; i < cursors.size(); i++) {
                const auto &lspSelectionRange = reply.at(i);

                if (lspSelectionRange) {
                    LSPRange currentRange = selections.isEmpty() || !selections.at(i).isValid() ? LSPRange(cursors.at(i), cursors.at(i)) : selections.at(i);

                    auto resultRange = findNextSelection(lspSelectionRange, currentRange, expand);
                    ret.append(resultRange);
                } else {
                    ret.append(KTextEditor::Range::invalid());
                }
            }

            activeView->setSelections(ret);
        };

        auto handle = server->selectionRange(document->url(), activeView->cursorPositions(), this, h);
        delayCancelRequest(std::move(handle));
    }

    static LSPRange findNextSelection(std::shared_ptr<LSPSelectionRange> selectionRange, const LSPRange &current, bool expand)
    {
        if (expand) {
            while (selectionRange && !selectionRange->range.contains(current)) {
                selectionRange = selectionRange->parent;
            }

            if (selectionRange) {
                if (selectionRange->range != current) {
                    return selectionRange->range;
                } else if (selectionRange->parent) {
                    return selectionRange->parent->range;
                }
            }
        } else {
            std::shared_ptr<LSPSelectionRange> previous = nullptr;

            while (selectionRange && current.contains(selectionRange->range) && current != selectionRange->range) {
                previous = selectionRange;
                selectionRange = selectionRange->parent;
            }

            if (previous) {
                return previous->range;
            }
        }

        return LSPRange::invalid();
    }

    void clangdSwitchSourceHeader()
    {
        KTextEditor::View *activeView = m_mainWindow->activeView();
        KTextEditor::Document *document = activeView->document();
        auto server = m_serverManager->findServer(activeView);
        if (!server || !document) {
            return;
        }

        auto h = [this](const QString &reply) {
            if (!reply.isEmpty()) {
                m_mainWindow->openUrl(QUrl(reply));
            } else {
                showMessage(i18n("Corresponding Header/Source not found"), KTextEditor::Message::Information);
            }
        };
        server->clangdSwitchSourceHeader(document->url(), this, h);
    }

    void clangdMemoryUsage()
    {
        KTextEditor::View *activeView = m_mainWindow->activeView();
        auto server = m_serverManager->findServer(activeView);
        if (!server)
            return;

        auto h = [this](const QString &reply) {
            auto view = m_mainWindow->openUrl(QUrl());
            if (view) {
                auto doc = view->document();
                doc->setText(reply);
                // position at top
                view->setCursorPosition({0, 0});
                // adjust mode
                const QString mode = QStringLiteral("JSON");
                doc->setHighlightingMode(mode);
                doc->setMode(mode);
                // no save file dialog when closing
                doc->setModified(false);
            }
        };
        server->clangdMemoryUsage(this, h);
    }

    void rustAnalyzerExpandMacro()
    {
        KTextEditor::View *activeView = m_mainWindow->activeView();
        auto server = m_serverManager->findServer(activeView);
        if (!server)
            return;

        auto position = activeView->cursorPosition();
        QPointer<KTextEditor::View> v(activeView);
        auto h = [this, v, position](const LSPExpandedMacro &reply) {
            if (v && !reply.expansion.isEmpty()) {
                m_textHintprovider.showTextHint(reply.expansion, TextHintMarkupKind::PlainText, position);
            } else {
                showMessage(i18n("No results"), KTextEditor::Message::Information);
            }
        };
        server->rustAnalyzerExpandMacro(this, activeView->document()->url(), position, h);
    }

    void gotoWorkSpaceSymbol()
    {
        KTextEditor::View *activeView = m_mainWindow->activeView();
        auto server = m_serverManager->findServer(activeView);
        if (!server) {
            return;
        }
        GotoSymbolHUDDialog dialog(m_mainWindow, server);
        dialog.openDialog();
    }

    static QStandardItem *getItem(const QStandardItemModel &model, const QUrl &url)
    {
        // local file in custom role, Qt::DisplayRole might have additional elements
        auto l = model.match(model.index(0, 0, QModelIndex()), Qt::UserRole, url.toLocalFile(), 1, Qt::MatchExactly);
        if (l.length()) {
            return model.itemFromIndex(l.at(0));
        }
        return nullptr;
    }

    static QStandardItem *getItem(const QStandardItem *topItem, KTextEditor::Cursor pos, bool onlyLine)
    {
        QStandardItem *targetItem = nullptr;
        if (topItem) {
            int count = topItem->rowCount();
            int first = 0, last = count;
            // let's not run wild on a linear search in a flood of diagnostics
            if (count > 50) {
                // instead, let's *assume* sorted and use binary search to get closer
                // it probably is sorted, so it should work out
                // if not, at least we tried (without spending/wasting more on sorting)
                auto getLine = [topItem, count](int index) {
                    Q_ASSERT(index >= 0);
                    Q_ASSERT(index < count);
                    auto range = topItem->child(index)->data(RangeData::RangeRole).value<LSPRange>();
                    return range.start().line();
                };
                int first = 0, last = count - 1;
                int target = pos.line();
                while (first + 1 < last) {
                    int middle = first + (last - first) / 2;
                    Q_ASSERT(first != middle);
                    Q_ASSERT(middle != last);
                    if (getLine(middle) < target) {
                        first = middle;
                    } else {
                        last = middle;
                    }
                }
            }
            for (int i = first; i < last; ++i) {
                auto item = topItem->child(i);
                if (!(item->flags() & Qt::ItemIsEnabled)) {
                    continue;
                }
                auto range = item->data(RangeData::RangeRole).value<LSPRange>();
                if ((onlyLine && pos.line() == range.start().line()) || (range.contains(pos))) {
                    targetItem = item;
                    break;
                }
            }
        }
        return targetItem;
    }

    void onDiagnostics(const LSPPublishDiagnosticsParams &diagnostics)
    {
        if (m_diagnostics->isChecked()) {
            Q_EMIT m_diagnosticProvider.diagnosticsAdded(diagnostics);
        }
    }

    void onServerChanged()
    {
        m_diagnosticProvider.requestClearSuppressions(&m_diagnosticProvider);
        updateState();
    }

    void onTextHint(KTextEditor::View *view, const KTextEditor::Cursor &position)
    {
        // only trigger generic hover if no diagnostic to show;
        // avoids interference by generic hover info
        // (which is likely not so useful in this case/position anyway)
        if (m_autoHover && m_autoHover->isChecked()) {
            m_hover->showTextHint(view, position, false);
        }
    }

    KTextEditor::View *viewForUrl(const QUrl &url) const
    {
        const auto views = m_mainWindow->views();
        for (auto *view : views) {
            if (view->document()->url() == url) {
                return view;
            }
        }
        return nullptr;
    }

    void addMessage(LSPMessageType level, const QString &category, const QString &msg, const QString &token = {})
    {
        // skip messaging if not enabled
        if (!m_messages->isChecked()) {
            return;
        }

        // use generic output view
        QVariantMap genericMessage;
        genericMessage.insert(QStringLiteral("category"), category);
        genericMessage.insert(QStringLiteral("text"), msg);

        // translate level to the type keys
        QString type;
        switch (level) {
        case LSPMessageType::Error:
            type = QStringLiteral("Error");
            break;
        case LSPMessageType::Warning:
            type = QStringLiteral("Warning");
            break;
        case LSPMessageType::Info:
            type = QStringLiteral("Info");
            break;
        case LSPMessageType::Log:
            type = QStringLiteral("Log");
            break;
        }
        genericMessage.insert(QStringLiteral("type"), type);

        if (!token.isEmpty()) {
            genericMessage.insert(QStringLiteral("token"), token);
        }

        // host application will handle these message for us, including auto-show settings
        Utils::showMessage(genericMessage, m_mainWindow);
    }

    // params type is same for show or log and is treated the same way
    void onMessage(LSPClientServer *server, const LSPLogMessageParams &params)
    {
        // determine server description
        auto message = params.message;
        if (server) {
            message = QStringLiteral("%1\n%2").arg(LSPClientServerManager::serverDescription(server), message);
        }
        addMessage(params.type, i18nc("@info", "LSP Server"), message);
    }

    void onWorkDoneProgress(LSPClientServer *server, const LSPWorkDoneProgressParams &params)
    {
        // provided token is/should be unique (from server perspective)
        // but we are dealing with multiple servers, so let's make a combined token
        const auto token = QStringLiteral("%1:%2").arg((quintptr)server).arg(params.token.toString());
        // find title in matching begin entry (if any)
        QString title;
        // plain search
        // could have used a map, but now we can discard the oldest one if needed
        int index = -1;
        for (int i = 0; i < m_workDoneProgress.size(); ++i) {
            auto &e = m_workDoneProgress.at(i);
            if (e.first == token) {
                index = i;
                title = e.second.value.title;
                break;
            }
        }
        if (index < 0) {
            if (m_workDoneProgress.size() > 10) {
                m_workDoneProgress.pop_front();
            }
            m_workDoneProgress.push_back({token, params});
        } else if (params.value.kind == LSPWorkDoneProgressKind::End) {
            m_workDoneProgress.remove(index);
        }
        // title (likely) only in initial message
        if (title.isEmpty()) {
            title = params.value.title;
        }

        const auto percent = params.value.percentage;
        QString msg = title;
        if (!percent.has_value()) {
            msg.append(QLatin1String(" ")).append(params.value.message);
        } else {
            msg.append(QStringLiteral(" [%1%] ").arg(percent.value(), 3));
            msg.append(params.value.message);
        }

        addMessage(LSPMessageType::Info, i18nc("@info", "LSP Server"), msg, token);
    }

    void onShowMessage(KTextEditor::Message::MessageType level, const QString &msg)
    {
        // translate level
        LSPMessageType lvl = LSPMessageType::Log;
        using KMessage = KTextEditor::Message;
        switch (level) {
        case KMessage::Error:
            lvl = LSPMessageType::Error;
            break;
        case KMessage::Warning:
            lvl = LSPMessageType::Warning;
            break;
        case KMessage::Information:
            lvl = LSPMessageType::Info;
            break;
        case KMessage::Positive:
            lvl = LSPMessageType::Log;
            break;
        }

        addMessage(lvl, i18nc("@info", "LSP Client"), msg);
    }

    void onTextChanged(KTextEditor::Document *doc)
    {
        KTextEditor::View *activeView = m_mainWindow->activeView();
        if (!activeView || activeView->document() != doc) {
            return;
        }

        if (m_plugin->m_semanticHighlighting) {
            m_semHighlightingManager.doSemanticHighlighting(activeView, true);
        }

        if (m_onTypeFormattingTriggers.empty()) {
            return;
        }

        // NOTE the intendation mode should probably be set to None,
        // so as not to experience unpleasant interference
        auto cursor = activeView->cursorPosition();
        QChar lastChar = cursor.column() == 0 ? QChar::fromLatin1('\n') : doc->characterAt({cursor.line(), cursor.column() - 1});
        if (m_onTypeFormattingTriggers.contains(lastChar)) {
            format(lastChar);
        }
    }

    void formatOnSave(KTextEditor::Document *doc, bool)
    {
        // only trigger for active doc
        auto activeView = m_mainWindow->activeView();
        if (activeView && activeView->document() == doc) {
            format({}, true);
        }
    }

    void updateState()
    {
        KTextEditor::View *activeView = m_mainWindow->activeView();

        auto doc = activeView ? activeView->document() : nullptr;
        auto server = m_serverManager->findServer(activeView);
        bool defEnabled = false, declEnabled = false, typeDefEnabled = false, refEnabled = false, implEnabled = false;
        bool hoverEnabled = false, highlightEnabled = false, codeActionEnabled = false;
        bool formatEnabled = false;
        bool renameEnabled = false;
        bool selectionRangeEnabled = false;
        bool isClangd = false;
        bool isRustAnalyzer = false;
        bool formatOnSave = false;

        if (server) {
            const auto &caps = server->capabilities();
            defEnabled = caps.definitionProvider;
            declEnabled = caps.declarationProvider;
            typeDefEnabled = caps.typeDefinitionProvider;
            refEnabled = caps.referencesProvider;
            implEnabled = caps.implementationProvider;
            hoverEnabled = caps.hoverProvider;
            highlightEnabled = caps.documentHighlightProvider;
            formatEnabled = caps.documentFormattingProvider || caps.documentRangeFormattingProvider;
            renameEnabled = caps.renameProvider;
            codeActionEnabled = caps.codeActionProvider;
            selectionRangeEnabled = caps.selectionRangeProvider;
            formatOnSave = formatEnabled && m_plugin->m_fmtOnSave;

            connect(server.get(), &LSPClientServer::publishDiagnostics, this, &self_type::onDiagnostics, Qt::UniqueConnection);
            connect(server.get(), &LSPClientServer::applyEdit, this, &self_type::onApplyEdit, Qt::UniqueConnection);

            // update format trigger characters
            const auto &fmt = caps.documentOnTypeFormattingProvider;
            if (fmt.provider && m_onTypeFormatting->isChecked()) {
                m_onTypeFormattingTriggers = fmt.triggerCharacters;
            } else {
                m_onTypeFormattingTriggers.clear();
            }
            // and monitor for such
            if (doc) {
                connect(doc, &KTextEditor::Document::textChanged, this, &self_type::onTextChanged, Qt::UniqueConnection);
                connect(doc, &KTextEditor::Document::reloaded, this, &self_type::updateState, Qt::UniqueConnection);
            }
            // only consider basename (full path may have been custom specified)
            auto lspServer = QFileInfo(server->cmdline().front()).fileName();
            isClangd = lspServer == QStringLiteral("clangd");
            isRustAnalyzer = lspServer == QStringLiteral("rust-analyzer");

            const bool semHighlightingEnabled = m_plugin->m_semanticHighlighting;
            if (semHighlightingEnabled) {
                m_semHighlightingManager.doSemanticHighlighting(activeView, false);
            }

            if (caps.inlayHintProvider && m_inlayHints->isChecked()) {
                m_inlayHintsHandler.setActiveView(activeView);
            }
        }

        // show context menu actions only if server is active
        actionCollection()->action(QStringLiteral("lspclient_code_action"))->setVisible(server != nullptr);
        actionCollection()->action(QStringLiteral("lspclient_goto_menu"))->setVisible(server != nullptr);
        actionCollection()->action(QStringLiteral("lspclient_find_references"))->setVisible(server != nullptr);
        actionCollection()->action(QStringLiteral("lspclient_rename"))->setVisible(server != nullptr);
        actionCollection()->action(QStringLiteral("lspclient_other_menu"))->setVisible(server != nullptr);

        if (m_findDef) {
            m_findDef->setEnabled(defEnabled);
        }
        if (m_findDecl) {
            m_findDecl->setEnabled(declEnabled);
        }
        if (m_findTypeDef) {
            m_findTypeDef->setEnabled(typeDefEnabled);
        }
        if (m_findRef) {
            m_findRef->setEnabled(refEnabled);
        }
        if (m_findImpl) {
            m_findImpl->setEnabled(implEnabled);
        }
        if (m_triggerHighlight) {
            m_triggerHighlight->setEnabled(highlightEnabled);
        }
        if (m_triggerSymbolInfo) {
            m_triggerSymbolInfo->setEnabled(hoverEnabled);
        }
        if (m_triggerFormat) {
            m_triggerFormat->setEnabled(formatEnabled);
        }
        if (m_triggerRename) {
            m_triggerRename->setEnabled(renameEnabled);
        }
        if (m_complDocOn) {
            m_complDocOn->setEnabled(server != nullptr);
        }
        if (m_restartServer) {
            m_restartServer->setEnabled(server != nullptr);
        }
        if (m_requestCodeAction) {
            m_requestCodeAction->setEnabled(codeActionEnabled);
        }
        if (m_expandSelection) {
            m_expandSelection->setEnabled(selectionRangeEnabled);
        }
        if (m_shrinkSelection) {
            m_shrinkSelection->setEnabled(selectionRangeEnabled);
        }
        m_switchSourceHeader->setEnabled(isClangd);
        m_switchSourceHeader->setVisible(isClangd);
        m_memoryUsage->setEnabled(isClangd);
        m_memoryUsage->setVisible(isClangd);
        m_expandMacro->setEnabled(isRustAnalyzer);
        m_expandMacro->setVisible(isRustAnalyzer);

        // update completion with relevant server
        m_completion->setServer(server);
        if (m_complDocOn) {
            m_completion->setSelectedDocumentation(m_complDocOn->isChecked());
        }
        if (m_signatureHelp) {
            m_completion->setSignatureHelp(m_signatureHelp->isChecked());
        }
        if (m_complParens) {
            m_completion->setCompleteParens(m_complParens->isChecked());
        }
        updateCompletion(activeView, server.get());

        // update hover with relevant server
        m_hover->setServer(server && server->capabilities().hoverProvider ? server : nullptr);

        updateMarks(doc);

        if (formatOnSave) {
            auto t = Qt::ConnectionType(Qt::UniqueConnection | Qt::QueuedConnection);
            connect(activeView->document(), &KTextEditor::Document::documentSavedOrUploaded, this, &self_type::formatOnSave, t);
        }

        // connect for cleanup stuff
        if (activeView) {
            connect(activeView, &KTextEditor::View::destroyed, this, &self_type::viewDestroyed, Qt::UniqueConnection);
        }
    }

    void updateMarks(KTextEditor::Document *doc = nullptr)
    {
        if (!doc) {
            KTextEditor::View *activeView = m_mainWindow->activeView();
            doc = activeView ? activeView->document() : nullptr;
        }

        // update marks if applicable
        if (m_markModel && doc) {
            addMarks(doc, m_markModel, m_ranges, m_marks);
        }
    }

    void viewDestroyed(QObject *view)
    {
        m_completionViews.removeAll(view);
    }

    void updateCompletion(KTextEditor::View *view, LSPClientServer *server)
    {
        if (!view) {
            return;
        }

        bool registered = m_completionViews.contains(view);

        if (!registered && server && server->capabilities().completionProvider.provider) {
            qCInfo(LSPCLIENT) << "registering cci";
            view->registerCompletionModel(m_completion.get());
            m_completionViews.append(view);
        }

        if (registered && !server) {
            qCInfo(LSPCLIENT) << "unregistering cci";
            view->unregisterCompletionModel(m_completion.get());
            m_completionViews.removeAll(view);
        }
    }

    Q_INVOKABLE QAbstractItemModel *documentSymbolsModel()
    {
        return m_symbolView->documentSymbolsModel();
    }

    void showMessageRequest(const LSPShowMessageParams &message,
                            const QList<LSPMessageRequestAction> &actions,
                            const std::function<void()> chooseNothing,
                            bool &handled)
    {
        if (handled) {
            return;
        }

        handled = true;

        CloseAllowedMessageBox box(m_mainWindow->window());
        box.setWindowTitle(i18n("Question from LSP server"));
        box.setText(message.message);
        switch (message.type) {
        case (LSPMessageType::Error):
            box.setIcon(QMessageBox::Critical);
            break;
        case (LSPMessageType::Warning):
            box.setIcon(QMessageBox::Warning);
            break;
        case (LSPMessageType::Info):
            box.setIcon(QMessageBox::Question);
            break;
        case (LSPMessageType::Log):
            box.setIcon(QMessageBox::Question);
            break;
        }

        std::map<QAbstractButton *, std::function<void()>> onClick;
        for (auto &action : actions) {
            QString escaped = action.title;
            escaped.replace(QLatin1Char('&'), QLatin1String("&&"));
            QAbstractButton *button = box.addButton(escaped, QMessageBox::AcceptRole);
            onClick[button] = action.choose;
        }
        box.exec();
        if (actions.empty() || box.clickedButton() == nullptr) {
            chooseNothing();
        } else {
            onClick[box.clickedButton()]();
        }
    }
};

QObject *LSPClientPluginView::new_(LSPClientPlugin *plugin, KTextEditor::MainWindow *mainWin, std::shared_ptr<LSPClientServerManager> manager)
{
    return new LSPClientPluginViewImpl(plugin, mainWin, manager);
}

#include "lspclientpluginview.moc"
