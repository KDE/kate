//
// Description: Kate Plugin for GDB integration
//
//
// SPDX-FileCopyrightText: 2010 Ian Wakeling <ian.wakeling@ntlworld.com>
// SPDX-FileCopyrightText: 2010-2014 Kåre Särs <kare.sars@iki.fi>
//
//  SPDX-License-Identifier: LGPL-2.0-only

#include "plugin_kategdb.h"

#include <QBoxLayout>
#include <QFile>
#include <QFontDatabase>
#include <QKeyEvent>
#include <QScrollBar>
#include <QSplitter>
#include <QTabWidget>
#include <QTextEdit>
#include <QTreeWidget>

#include <KActionCollection>
#include <KConfigGroup>
#include <KXMLGUIFactory>
#include <QAction>
#include <QMenu>

#include <KColorScheme>
#include <KHistoryComboBox>
#include <KLocalizedString>
#include <KPluginFactory>

#include "debugconfigpage.h"
#include <KTextEditor/Document>
#include <QDir>
#include <ktexteditor/editor.h>
#include <ktexteditor/view.h>

K_PLUGIN_FACTORY_WITH_JSON(KatePluginGDBFactory, "kategdbplugin.json", registerPlugin<KatePluginGDB>();)

static const QString CONFIG_DEBUGPLUGIN{QStringLiteral("debugplugin")};
static const QString CONFIG_DAP_CONFIG{QStringLiteral("DAPConfiguration")};

using namespace Qt::Literals::StringLiterals;

KatePluginGDB::KatePluginGDB(QObject *parent, const VariantList &)
    : KTextEditor::Plugin(parent)
    , m_settingsPath(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + QStringLiteral("/debugger"))
    , m_defaultConfigPath(QUrl::fromLocalFile(m_settingsPath + QStringLiteral("/dap.json")))
{
    QDir().mkpath(m_settingsPath);
    readConfig();
}

int KatePluginGDB::configPages() const
{
    return 1;
}
KTextEditor::ConfigPage *KatePluginGDB::configPage(int number, QWidget *parent)
{
    if (number != 0) {
        return nullptr;
    }

    return new DebugConfigPage(parent, this);
}

KatePluginGDB::~KatePluginGDB()
{
}

void KatePluginGDB::readConfig()
{
    KConfigGroup config(KSharedConfig::openConfig(), CONFIG_DEBUGPLUGIN);
    m_configPath = config.readEntry(CONFIG_DAP_CONFIG, QUrl());

    Q_EMIT update();
}

void KatePluginGDB::writeConfig() const
{
    KConfigGroup config(KSharedConfig::openConfig(), CONFIG_DEBUGPLUGIN);
    config.writeEntry(CONFIG_DAP_CONFIG, m_configPath);

    Q_EMIT update();
}

QObject *KatePluginGDB::createView(KTextEditor::MainWindow *mainWindow)
{
    return new KatePluginGDBView(this, mainWindow);
}

KatePluginGDBView::KatePluginGDBView(KatePluginGDB *plugin, KTextEditor::MainWindow *mainWin)
    : QObject(mainWin)
    , m_plugin(plugin)
    , m_mainWin(mainWin)
{
    m_lastExecUrl = QUrl();
    m_lastExecLine = -1;
    m_lastExecFrame = 0;
    m_kateApplication = KTextEditor::Editor::instance()->application();
    m_focusOnInput = true;
    m_activeThread = -1;

    KXMLGUIClient::setComponentName(QStringLiteral("kategdb"), i18n("Kate Debug"));
    setXMLFile(QStringLiteral("ui.rc"));

    m_toolView.reset(
        m_mainWin->createToolView(plugin, i18n("Debug View"), KTextEditor::MainWindow::Bottom, QIcon::fromTheme(QStringLiteral("debug-run")), i18n("Debug")));

    m_localsStackToolView.reset(m_mainWin->createToolView(plugin,
                                                          i18n("Locals and Stack"),
                                                          KTextEditor::MainWindow::Right,
                                                          QIcon::fromTheme(QStringLiteral("debug-run")),
                                                          i18n("Locals and Stack")));

    m_tabWidget = new QTabWidget(m_toolView.get());

    // buttons widget, buttons are initialized at action connections
    m_buttonWidget = new QWidget(m_tabWidget);
    auto buttonsLayout = new QHBoxLayout(m_buttonWidget);
    buttonsLayout->setContentsMargins(0, 1, 0, 3);
    m_tabWidget->setCornerWidget(m_buttonWidget);

    // Output
    m_outputArea = new QTextEdit();
    m_outputArea->setAcceptRichText(false);
    m_outputArea->setReadOnly(true);
    m_outputArea->setUndoRedoEnabled(false);
    // fixed wide font, like konsole
    m_outputArea->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    // Use complementary color scheme for dark "terminal"
    KColorScheme schemeView(QPalette::Active, KColorScheme::Complementary);
    m_outputArea->setTextBackgroundColor(schemeView.background().color());
    m_outputArea->setTextColor(schemeView.foreground().color());
    QPalette p = m_outputArea->palette();
    p.setColor(QPalette::Base, schemeView.background().color());
    m_outputArea->setPalette(p);

    // input
    m_inputArea = new KHistoryComboBox(true);
    connect(m_inputArea, static_cast<void (KHistoryComboBox::*)(const QString &)>(&KHistoryComboBox::returnPressed), this, &KatePluginGDBView::slotSendCommand);
    auto *inputLayout = new QHBoxLayout();
    inputLayout->addWidget(m_inputArea, 10);
    inputLayout->setContentsMargins(0, 0, 0, 0);
    m_outputArea->setFocusProxy(m_inputArea); // take the focus from the outputArea

    m_gdbPage = new QWidget();
    auto *layout = new QVBoxLayout(m_gdbPage);
    layout->addWidget(m_outputArea);
    layout->addLayout(inputLayout);
    layout->setStretch(0, 10);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // stack page
    auto *stackContainer = new QWidget();
    auto *stackLayout = new QVBoxLayout(stackContainer);
    m_threadCombo = new QComboBox();
    m_stackTree = new QTreeWidget();
    stackLayout->addWidget(m_threadCombo);
    stackLayout->addWidget(m_stackTree);
    stackLayout->setStretch(0, 10);
    stackLayout->setContentsMargins(0, 0, 0, 0);
    stackLayout->setSpacing(0);
    QStringList headers;
    headers << QStringLiteral("  ") << i18nc("Column label (frame number)", "Nr") << i18nc("Column label", "Frame");
    m_stackTree->setHeaderLabels(headers);
    m_stackTree->setRootIsDecorated(false);
    m_stackTree->resizeColumnToContents(0);
    m_stackTree->resizeColumnToContents(1);
    m_stackTree->setAutoScroll(false);
    connect(m_stackTree, &QTreeWidget::itemActivated, this, &KatePluginGDBView::stackFrameSelected);

    connect(m_threadCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &KatePluginGDBView::threadSelected);

    auto *variableContainer = new QWidget();
    auto *variableLayout = new QVBoxLayout(variableContainer);
    m_scopeCombo = new QComboBox();
    connect(m_scopeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &KatePluginGDBView::scopeSelected);
    m_localsView = new LocalsView();
    variableLayout->addWidget(m_scopeCombo);
    variableLayout->addWidget(m_localsView);
    variableLayout->setStretch(0, 10);
    variableLayout->setContentsMargins(0, 0, 0, 0);
    variableLayout->setSpacing(0);

    auto *locStackSplitter = new QSplitter(m_localsStackToolView.get());
    locStackSplitter->addWidget(variableContainer);
    locStackSplitter->addWidget(stackContainer);
    locStackSplitter->setOrientation(Qt::Vertical);

    m_ioView = std::make_unique<IOView>();

    m_backend = new Backend(this);
    connect(m_backend, &BackendInterface::readyForInput, this, &KatePluginGDBView::enableDebugActions);
    connect(m_backend, &BackendInterface::debuggerCapabilitiesChanged, [this] {
        enableDebugActions(true);
    });

    connect(m_backend, &BackendInterface::outputText, this, &KatePluginGDBView::addOutputText);

    connect(m_backend, &BackendInterface::outputError, this, &KatePluginGDBView::addErrorText);

    connect(m_backend, &BackendInterface::debugLocationChanged, this, &KatePluginGDBView::slotGoTo);

    connect(m_backend, &BackendInterface::breakPointSet, this, &KatePluginGDBView::slotBreakpointSet);

    connect(m_backend, &BackendInterface::breakPointCleared, this, &KatePluginGDBView::slotBreakpointCleared);

    connect(m_backend, &BackendInterface::clearBreakpointMarks, this, &KatePluginGDBView::clearMarks);

    connect(m_backend, &BackendInterface::programEnded, this, &KatePluginGDBView::programEnded);

    connect(m_backend, &BackendInterface::gdbEnded, this, &KatePluginGDBView::programEnded);

    connect(m_backend, &BackendInterface::stackFrameInfo, this, &KatePluginGDBView::insertStackFrame);

    connect(m_backend, &BackendInterface::stackFrameChanged, this, &KatePluginGDBView::stackFrameChanged);

    connect(m_backend, &BackendInterface::scopesInfo, this, &KatePluginGDBView::insertScopes);

    connect(m_backend, &BackendInterface::variableScopeOpened, m_localsView, &LocalsView::openVariableScope);
    connect(m_backend, &BackendInterface::variableScopeClosed, m_localsView, &LocalsView::closeVariableScope);
    connect(m_backend, &BackendInterface::variableInfo, m_localsView, &LocalsView::addVariableLevel);

    connect(m_backend, &BackendInterface::threads, this, &KatePluginGDBView::onThreads);
    connect(m_backend, &BackendInterface::threadUpdated, this, &KatePluginGDBView::updateThread);

    connect(m_backend, &BackendInterface::debuggeeOutput, this, &KatePluginGDBView::addOutput);

    connect(m_backend, &BackendInterface::sourceFileNotFound, this, [this](const QString &fileName) {
        displayMessage(xi18nc("@info",
                              "<title>Could not open file:</title><nl/>%1<br/>Try adding a search path to Advanced Settings -> Source file search paths",
                              fileName),
                       KTextEditor::Message::Error);
    });

    connect(m_backend, &BackendInterface::backendError, this, [this](const QString &message, KTextEditor::Message::MessageType level) {
        displayMessage(message, level);
    });

    connect(m_localsView, &LocalsView::localsVisible, m_backend, &BackendInterface::slotQueryLocals);

    // Actions
    m_targetSelectAction = actionCollection()->add<KSelectAction>(QStringLiteral("targets"));
    m_targetSelectAction->setText(i18n("Targets"));
    m_targetSelectAction->setEnabled(true);
    connect(m_targetSelectAction->menu(), &QMenu::aboutToShow, this, &KatePluginGDBView::initDebugToolview);

    QAction *a = actionCollection()->addAction(QStringLiteral("debug"));
    a->setText(i18n("Start Debugging"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("debug-run")));
    actionCollection()->setDefaultShortcut(a, QKeySequence((Qt::CTRL | Qt::SHIFT | Qt::Key_F7)));
    connect(a, &QAction::triggered, this, &KatePluginGDBView::slotDebug);
    // This button is handled differently due to it having multiple actions
    m_continueButton = new QToolButton();
    m_continueButton->setAutoRaise(true);
    buttonsLayout->addWidget(m_continueButton);

    a = actionCollection()->addAction(QStringLiteral("kill"));
    a->setText(i18n("Kill / Stop Debugging"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-stop")));
    actionCollection()->setDefaultShortcut(a, QKeySequence((Qt::ALT | Qt::Key_F7)));
    connect(a, &QAction::triggered, m_backend, &BackendInterface::slotKill);
    buttonsLayout->addWidget(createDebugButton(a));

    a = actionCollection()->addAction(QStringLiteral("continue"));
    a->setText(i18n("Continue"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-start")));
    actionCollection()->setDefaultShortcut(a, QKeySequence((Qt::CTRL | Qt::Key_F7)));
    connect(a, &QAction::triggered, m_backend, &BackendInterface::slotContinue);

    a = actionCollection()->addAction(u"toggle_breakpoint"_s);
    a->setText(i18n("Toggle Breakpoint / Break"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-pause")));
    actionCollection()->setDefaultShortcut(a, QKeySequence((Qt::SHIFT | Qt::Key_F11)));
    connect(a, &QAction::triggered, this, &KatePluginGDBView::slotToggleBreakpoint);
    buttonsLayout->addWidget(createDebugButton(a));

    a = actionCollection()->addAction(u"clear_all_breakpoints"_s);
    a->setText(i18n("Clear All Breakpoints"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("edit-clear-all")));
    connect(a, &QAction::triggered, this, &KatePluginGDBView::clearMarks);

    a = actionCollection()->addAction(QStringLiteral("step_in"));
    a->setText(i18n("Step In"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("debug-step-into")));
    actionCollection()->setDefaultShortcut(a, QKeySequence((Qt::Key_F10)));
    connect(a, &QAction::triggered, m_backend, &BackendInterface::slotStepInto);
    buttonsLayout->addWidget(createDebugButton(a));

    a = actionCollection()->addAction(QStringLiteral("step_over"));
    a->setText(i18n("Step Over"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("debug-step-over")));
    actionCollection()->setDefaultShortcut(a, QKeySequence((Qt::Key_F9)));
    connect(a, &QAction::triggered, m_backend, &BackendInterface::slotStepOver);
    buttonsLayout->addWidget(createDebugButton(a));

    a = actionCollection()->addAction(QStringLiteral("step_out"));
    a->setText(i18n("Step Out"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("debug-step-out")));
    actionCollection()->setDefaultShortcut(a, QKeySequence((Qt::SHIFT | Qt::Key_F10)));
    connect(a, &QAction::triggered, m_backend, &BackendInterface::slotStepOut);
    buttonsLayout->addWidget(createDebugButton(a));

    a = actionCollection()->addAction(QStringLiteral("run_to_cursor"));
    a->setText(i18n("Run To Cursor"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("debug-run-cursor")));
    actionCollection()->setDefaultShortcut(a, QKeySequence((Qt::SHIFT | Qt::Key_F9)));
    connect(a, &QAction::triggered, this, &KatePluginGDBView::slotRunToCursor);
    buttonsLayout->addWidget(createDebugButton(a));

    a = actionCollection()->addAction(QStringLiteral("rerun"));
    a->setText(i18n("Restart Debugging"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh")));
    actionCollection()->setDefaultShortcut(a, QKeySequence((Qt::CTRL | Qt::SHIFT | Qt::Key_F7)));
    connect(a, &QAction::triggered, this, &KatePluginGDBView::slotRestart);
    buttonsLayout->addWidget(createDebugButton(a));

    a = actionCollection()->addAction(QStringLiteral("debug_hot_reload"));
    a->setText(i18n("Hot Reload"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("exception")));
    a->setVisible(false);
    connect(a, &QAction::triggered, m_backend, &Backend::slotHotReload);
    buttonsLayout->addWidget(createDebugButton(a));

    a = actionCollection()->addAction(QStringLiteral("debug_hot_restart"));
    a->setText(i18n("Hot Restart"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("edit-reset")));
    a->setVisible(false);
    connect(a, &QAction::triggered, m_backend, &Backend::slotHotRestart);
    buttonsLayout->addWidget(createDebugButton(a));

    buttonsLayout->addStretch();

    m_hotReloadOnSaveAction = a = actionCollection()->addAction(QStringLiteral("debug_hot_reload_on_save"));
    a->setText(i18n("Enable Hot Reload on Save"));
    a->setVisible(false);
    a->setCheckable(true);
    a->setChecked(true);
    connect(a, &QAction::triggered, this, [this](bool e) {
        enableHotReloadOnSave(e ? m_mainWin->activeView() : nullptr);
    });

    a = actionCollection()->addAction(QStringLiteral("move_pc"));
    a->setText(i18nc("Move Program Counter (next execution)", "Move PC"));
    connect(a, &QAction::triggered, this, &KatePluginGDBView::slotMovePC);
    actionCollection()->setDefaultShortcut(a, QKeySequence((Qt::ALT | Qt::Key_F9)));

    a = actionCollection()->addAction(QStringLiteral("print_value"));
    a->setText(i18n("Print Value"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("document-preview")));
    connect(a, &QAction::triggered, this, &KatePluginGDBView::slotValue);

    // popup context m_menu
    m_menu = new KActionMenu(i18n("Debug"), this);
    actionCollection()->addAction(QStringLiteral("popup_gdb"), m_menu);
    connect(m_menu->menu(), &QMenu::aboutToShow, this, &KatePluginGDBView::aboutToShowMenu);

    m_breakpoint = m_menu->menu()->addAction(u"popup_breakpoint"_s, this, &KatePluginGDBView::slotToggleBreakpoint);

    QAction *popupAction = m_menu->menu()->addAction(u"popup_run_to_cursor"_s, this, &KatePluginGDBView::slotRunToCursor);
    popupAction->setText(i18n("Run To Cursor"));
    popupAction = m_menu->menu()->addAction(QStringLiteral("move_pc"), this, &KatePluginGDBView::slotMovePC);
    popupAction->setText(i18nc("Move Program Counter (next execution)", "Move PC"));

    enableDebugActions(false);

    connect(m_mainWin, &KTextEditor::MainWindow::unhandledShortcutOverride, this, &KatePluginGDBView::handleEsc);

    const auto documents = KTextEditor::Editor::instance()->application()->documents();
    for (auto doc : documents) {
        enableBreakpointMarks(doc);
    }

    connect(KTextEditor::Editor::instance()->application(), &KTextEditor::Application::documentCreated, this, &KatePluginGDBView::enableBreakpointMarks);

    m_hotReloadTimer.setInterval(10);
    m_hotReloadTimer.setSingleShot(true);
    m_hotReloadTimer.callOnTimeout(m_backend, &Backend::slotHotReload);

    m_toolView->installEventFilter(this);

    m_mainWin->guiFactory()->addClient(this);
}

KatePluginGDBView::~KatePluginGDBView()
{
    m_mainWin->guiFactory()->removeClient(this);
}

void KatePluginGDBView::readSessionConfig(const KConfigGroup &config)
{
    m_sessionConfig = DebugPluginSessionConfig::read(config);
}

void KatePluginGDBView::writeSessionConfig(KConfigGroup &config)
{
    if (m_configView) {
        m_sessionConfig = {};
        m_configView->writeConfig(m_sessionConfig);
        DebugPluginSessionConfig::write(config, m_sessionConfig);
    } else {
        DebugPluginSessionConfig::write(config, m_sessionConfig);
    }
}

void KatePluginGDBView::slotDebug()
{
    initDebugToolview();
#ifndef Q_OS_WIN
    disconnect(m_ioView.get(), &IOView::stdOutText, nullptr, nullptr);
    disconnect(m_ioView.get(), &IOView::stdErrText, nullptr, nullptr);
    if (m_configView->showIOTab()) {
        connect(m_ioView.get(), &IOView::stdOutText, m_ioView.get(), &IOView::addStdOutText);
        connect(m_ioView.get(), &IOView::stdErrText, m_ioView.get(), &IOView::addStdErrText);
    } else {
        connect(m_ioView.get(), &IOView::stdOutText, this, &KatePluginGDBView::addOutputText);
        connect(m_ioView.get(), &IOView::stdErrText, this, &KatePluginGDBView::addErrorText);
    }
    QStringList ioFifos;
    ioFifos << m_ioView->stdinFifo();
    ioFifos << m_ioView->stdoutFifo();
    ioFifos << m_ioView->stderrFifo();
#endif

    m_outputArea->clear();

    enableDebugActions(true);
    m_mainWin->showToolView(m_toolView.get());
    m_tabWidget->setCurrentWidget(m_gdbPage);
    QScrollBar *sb = m_outputArea->verticalScrollBar();
    sb->setValue(sb->maximum());
    m_scopeCombo->clear();
    m_localsView->clear();

#ifndef Q_OS_WIN
    if (m_configView->debuggerIsGDB()) {
        m_backend->runDebugger(m_configView->currentGDBTarget(), ioFifos);
    } else {
        m_backend->runDebugger(m_configView->currentDAPTarget(true));
    }
#else
    m_backend->runDebugger(m_configView->currentDAPTarget(true));
#endif
}

void KatePluginGDBView::slotRestart()
{
    m_mainWin->showToolView(m_toolView.get());
    m_tabWidget->setCurrentWidget(m_gdbPage);
    QScrollBar *sb = m_outputArea->verticalScrollBar();
    sb->setValue(sb->maximum());
    m_scopeCombo->clear();
    m_localsView->clear();

    m_backend->slotReRun();
}

void KatePluginGDBView::aboutToShowMenu()
{
    if (!m_backend->debuggerRunning() || m_backend->debuggerBusy()) {
        m_breakpoint->setText(i18n("Insert breakpoint"));
        m_breakpoint->setDisabled(true);
        return;
    }

    m_breakpoint->setDisabled(false);

    KTextEditor::View *editView = m_mainWin->activeView();
    QUrl url = editView->document()->url();
    int line = editView->cursorPosition().line();

    line++; // GDB uses 1 based line numbers, kate uses 0 based...

    if (m_backend->hasBreakpoint(url, line)) {
        m_breakpoint->setText(i18n("Remove breakpoint"));
    } else {
        m_breakpoint->setText(i18n("Insert breakpoint"));
    }
}

void KatePluginGDBView::slotToggleBreakpoint()
{
    if (m_backend->debuggerRunning() && !m_backend->canContinue()) {
        m_backend->slotInterrupt();
    } else {
        KTextEditor::View *editView = m_mainWin->activeView();
        if (!editView) {
            return;
        }
        QUrl currURL = editView->document()->url();
        int line = editView->cursorPosition().line() + 1;
        bool added = true;
        m_backend->toggleBreakpoint(currURL, line, &added);

        // We don't support setting breakpoints for gdb when the debugger is not running'
        if (!m_backend->debuggerRunning() && m_configView && m_configView->debuggerIsGDB()) {
            return;
        }

        if (added) {
            slotBreakpointSet(currURL, line);
        } else {
            slotBreakpointCleared(currURL, line);
        }
    }
}

void KatePluginGDBView::slotBreakpointSet(const QUrl &file, int line)
{
    if (auto doc = m_kateApplication->findUrl(file)) {
        disconnect(doc, &KTextEditor::Document::markChanged, this, &KatePluginGDBView::updateBreakpoints);
        doc->addMark(line - 1, KTextEditor::Document::BreakpointActive);
        m_backend->saveBreakpoint(file, line);
        connect(doc, &KTextEditor::Document::markChanged, this, &KatePluginGDBView::updateBreakpoints);
    }
}

void KatePluginGDBView::slotBreakpointCleared(const QUrl &file, int line)
{
    if (auto doc = m_kateApplication->findUrl(file)) {
        disconnect(doc, &KTextEditor::Document::markChanged, this, &KatePluginGDBView::updateBreakpoints);
        doc->removeMark(line - 1, KTextEditor::Document::BreakpointActive);
        m_backend->removeSavedBreakpoint(file, line);
        connect(doc, &KTextEditor::Document::markChanged, this, &KatePluginGDBView::updateBreakpoints);
    }
}

void KatePluginGDBView::slotMovePC()
{
    KTextEditor::View *editView = m_mainWin->activeView();
    QUrl currURL = editView->document()->url();
    KTextEditor::Cursor cursor = editView->cursorPosition();

    m_backend->movePC(currURL, cursor.line() + 1);
}

void KatePluginGDBView::slotRunToCursor()
{
    KTextEditor::View *editView = m_mainWin->activeView();
    QUrl currURL = editView->document()->url();
    KTextEditor::Cursor cursor = editView->cursorPosition();

    // GDB starts lines from 1, kate returns lines starting from 0 (displaying 1)
    m_backend->runToCursor(currURL, cursor.line() + 1);
}

void KatePluginGDBView::slotGoTo(const QUrl &url, int lineNum)
{
    // Kate uses 0 start index, gdb/dap uses 1 start index
    lineNum--;
    // remove last location
    if ((url == m_lastExecUrl) && (lineNum == m_lastExecLine)) {
        return;
    } else {
        if (auto doc = m_kateApplication->findUrl(m_lastExecUrl)) {
            doc->removeMark(m_lastExecLine, KTextEditor::Document::Execution);
        }
    }

    // skip not existing files
    if (!QFile::exists(url.toLocalFile())) {
        m_lastExecLine = -1;
        return;
    }

    m_lastExecUrl = url;
    m_lastExecLine = lineNum;

    KTextEditor::View *editView = m_mainWin->openUrl(m_lastExecUrl);
    editView->setCursorPosition(KTextEditor::Cursor(m_lastExecLine, 0));
    m_mainWin->window()->raise();
    m_mainWin->window()->setFocus();
}

void KatePluginGDBView::enableDebugActions(bool enable)
{
    const bool canMove = m_backend->canMove();
    actionCollection()->action(QStringLiteral("step_in"))->setEnabled(enable && canMove);
    actionCollection()->action(QStringLiteral("step_over"))->setEnabled(enable && canMove);
    actionCollection()->action(QStringLiteral("step_out"))->setEnabled(enable && canMove);
    actionCollection()->action(QStringLiteral("move_pc"))->setEnabled(enable && m_backend->supportsMovePC());
    actionCollection()->action(QStringLiteral("run_to_cursor"))->setEnabled(enable && m_backend->supportsRunToCursor());
    actionCollection()->action(QStringLiteral("popup_gdb"))->setEnabled(enable);
    actionCollection()->action(QStringLiteral("continue"))->setEnabled(enable && m_backend->canContinue());
    actionCollection()->action(QStringLiteral("print_value"))->setEnabled(enable);

    auto a = actionCollection()->action(QStringLiteral("debug_hot_reload"));
    a->setVisible(enable && m_backend->canHotReload());
    a->setEnabled(enable && m_backend->canHotReload());

    m_hotReloadOnSaveAction->setVisible(enable && m_backend->canHotReload());
    m_hotReloadOnSaveAction->setEnabled(enable && m_backend->canHotReload());

    if (enable && m_backend->canHotReload()) {
        connect(m_mainWin, &KTextEditor::MainWindow::viewChanged, this, &KatePluginGDBView::enableHotReloadOnSave, Qt::UniqueConnection);
        enableHotReloadOnSave(m_mainWin->activeView());
    } else {
        m_hotReloadTimer.stop();
        enableHotReloadOnSave(nullptr);
        disconnect(m_mainWin, &KTextEditor::MainWindow::viewChanged, this, &KatePluginGDBView::enableHotReloadOnSave);
    }

    a = actionCollection()->action(QStringLiteral("debug_hot_restart"));
    a->setVisible(enable && m_backend->canHotRestart());
    a->setEnabled(enable && m_backend->canHotRestart());

    m_breakpoint->setEnabled(m_backend->debuggerRunning());

    const QIcon pauseToggle = m_backend->canContinue() ? QIcon::fromTheme(u"media-record"_s) : QIcon::fromTheme(u"media-playback-pause"_s);
    actionCollection()->action(u"toggle_breakpoint"_s)->setIcon(pauseToggle);
    actionCollection()->action(QStringLiteral("kill"))->setEnabled(m_backend->debuggerRunning());
    actionCollection()->action(QStringLiteral("rerun"))->setEnabled(m_backend->debuggerRunning());

    // Combine start and continue button to same button
    m_continueButton->removeAction(actionCollection()->action(!enable ? QStringLiteral("continue") : QStringLiteral("debug")));
    m_continueButton->setDefaultAction(actionCollection()->action(enable ? QStringLiteral("continue") : QStringLiteral("debug")));
    m_continueButton->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-start")));

    m_inputArea->setEnabled(enable && !m_backend->debuggerBusy());
    m_threadCombo->setEnabled(enable);
    m_stackTree->setEnabled(enable);
    m_scopeCombo->setEnabled(enable);
    m_localsView->setEnabled(enable);

    if (enable) {
        m_inputArea->setFocusPolicy(Qt::WheelFocus);

        if (m_focusOnInput || m_configView->takeFocusAlways()) {
            m_inputArea->setFocus();
            m_focusOnInput = false;
        } else {
            if (m_mainWin->activeView()) {
                m_mainWin->activeView()->setFocus();
            }
        }
    } else {
        m_inputArea->setFocusPolicy(Qt::NoFocus);
        if (m_mainWin->activeView()) {
            m_mainWin->activeView()->setFocus();
        }
    }

    m_ioView->enableInput(!enable && m_backend->debuggerRunning());

    if ((m_lastExecLine > -1)) {
        if (auto doc = m_kateApplication->findUrl(m_lastExecUrl)) {
            if (enable) {
                doc->setMarkDescription(KTextEditor::Document::Execution, i18n("Execution point"));
                doc->setMarkIcon(KTextEditor::Document::Execution, QIcon::fromTheme(QStringLiteral("arrow-right")));
                doc->addMark(m_lastExecLine, KTextEditor::Document::Execution);
            } else {
                doc->removeMark(m_lastExecLine, KTextEditor::Document::Execution);
            }
        }
    }
}

void KatePluginGDBView::programEnded()
{
    if (auto doc = m_kateApplication->findUrl(m_lastExecUrl); doc && m_lastExecLine >= 0) {
        doc->removeMark(m_lastExecLine, KTextEditor::Document::Execution);
    }

    // don't set the execution mark on exit
    m_lastExecLine = -1;
    m_stackTree->clear();
    m_scopeCombo->clear();
    m_localsView->clear();
    m_threadCombo->clear();

    // Indicate the state change by showing the debug outputArea
    m_mainWin->showToolView(m_toolView.get());
    m_tabWidget->setCurrentWidget(m_configView);

    m_localsView->clear();
    m_ioView->clearOutput();
}

void KatePluginGDBView::clearMarks()
{
    const auto documents = m_kateApplication->documents();
    for (KTextEditor::Document *doc : documents) {
        const QHash<int, KTextEditor::Mark *> marks = doc->marks();
        QHashIterator<int, KTextEditor::Mark *> i(marks);
        while (i.hasNext()) {
            i.next();
            if ((i.value()->type == KTextEditor::Document::Execution) || (i.value()->type == KTextEditor::Document::BreakpointActive)) {
                m_backend->removeSavedBreakpoint(doc->url(), i.value()->line);
                doc->removeMark(i.value()->line, i.value()->type);
            }
        }
    }
}

void KatePluginGDBView::slotSendCommand()
{
    QString cmd = m_inputArea->currentText();

    if (cmd.isEmpty()) {
        cmd = m_lastCommand;
    }

    m_inputArea->addToHistory(cmd);
    m_inputArea->setCurrentItem(QString());
    m_focusOnInput = true;
    m_lastCommand = cmd;
    m_backend->issueCommand(cmd);

    QScrollBar *sb = m_outputArea->verticalScrollBar();
    sb->setValue(sb->maximum());
}

void KatePluginGDBView::insertStackFrame(int level, const QString &info)
{
    if (level < 0) {
        m_stackTree->resizeColumnToContents(2);
        return;
    }

    if (level == 0) {
        m_stackTree->clear();
    }
    QStringList columns;
    columns << QStringLiteral("  "); // icon place holder
    columns << QString::number(level);
    int lastSpace = info.lastIndexOf(QLatin1Char(' '));
    QString shortInfo = info.mid(lastSpace);
    columns << shortInfo;

    auto *item = new QTreeWidgetItem(columns);
    item->setToolTip(2, QStringLiteral("<qt>%1<qt>").arg(info));
    m_stackTree->insertTopLevelItem(level, item);
}

void KatePluginGDBView::stackFrameSelected()
{
    m_backend->changeStackFrame(m_stackTree->currentIndex().row());
}

void KatePluginGDBView::stackFrameChanged(int level)
{
    QTreeWidgetItem *current = m_stackTree->topLevelItem(m_lastExecFrame);
    QTreeWidgetItem *next = m_stackTree->topLevelItem(level);

    if (current) {
        current->setIcon(0, QIcon());
    }
    if (next) {
        next->setIcon(0, QIcon::fromTheme(QStringLiteral("arrow-right")));
    }
    m_lastExecFrame = level;
}

void KatePluginGDBView::insertScopes(const QList<dap::Scope> &scopes, std::optional<int> activeId)
{
    m_scopeCombo->clear();

    int selected = -1;
    int index = 0;
    for (const auto &scope : scopes) {
        QString name = scope.expensive.value_or(false) ? QStringLiteral("%1!").arg(scope.name) : scope.name;
        if (activeId && (activeId == scope.variablesReference)) {
            selected = index;
        }
        m_scopeCombo->addItem(QIcon::fromTheme(QStringLiteral("")).pixmap(10, 10), scope.name, scope.variablesReference);
        ++index;
    }
    if (selected >= 0) {
        m_scopeCombo->setCurrentIndex(selected);
    }
}

void KatePluginGDBView::scopeSelected(int scope)
{
    if (scope < 0)
        return;
    m_backend->changeScope(m_scopeCombo->itemData(scope).toInt());
}

void KatePluginGDBView::onThreads(const QList<dap::Thread> &threads)
{
    m_threadCombo->clear();
    int activeThread = m_activeThread;
    m_activeThread = -1;
    bool oldActiveThreadFound = false;

    const auto pix = QIcon::fromTheme(QLatin1String("")).pixmap(10, 10);
    for (const auto &thread : threads) {
        QString name = i18n("Thread %1", thread.id);
        if (!thread.name.isEmpty()) {
            name += QStringLiteral(": %1").arg(thread.name);
        }
        QPixmap icon = pix;
        if (thread.id == activeThread) {
            icon = QIcon::fromTheme(QStringLiteral("arrow-right")).pixmap(10, 10);
            oldActiveThreadFound = true;
        }
        m_threadCombo->addItem(icon, name, thread.id);
    }

    // try to set an active thread
    if (m_threadCombo->count() > 0) {
        int idx = 0; // use first thread
        if (oldActiveThreadFound) {
            // if old active thread was found, use that instead
            idx = m_threadCombo->findData(activeThread);
            m_activeThread = activeThread;
        } else {
            m_activeThread = m_threadCombo->itemData(idx).toInt();
        }
        m_threadCombo->setCurrentIndex(idx);
    }
}

void KatePluginGDBView::updateThread(const dap::Thread &thread, Backend::ThreadState state, bool isActive)
{
    int idx = m_threadCombo->findData(thread.id);
    if (idx == -1 && state != Backend::ThreadState::Exited) {
        // thread wasn't found, add it
        QString name = i18n("Thread %1", thread.id);
        const QPixmap pix = QIcon::fromTheme(QLatin1String("")).pixmap(10, 10);
        m_threadCombo->addItem(pix, name, thread.id);
    } else if (idx != -1 && state == Backend::ThreadState::Exited) {
        // remove exited thread
        m_threadCombo->removeItem(idx);
    }

    // Try to set an active thread
    if (isActive) {
        if (m_activeThread != thread.id && m_activeThread != -1) {
            int oldActiveIdx = m_threadCombo->findData(m_activeThread);
            const QPixmap pix = QIcon::fromTheme(QLatin1String("")).pixmap(10, 10);
            m_threadCombo->setItemIcon(oldActiveIdx, QIcon::fromTheme(QStringLiteral("arrow-right")).pixmap(10, 10));
        }

        m_activeThread = thread.id;
        m_threadCombo->setItemIcon(idx, QIcon::fromTheme(QStringLiteral("arrow-right")).pixmap(10, 10));
        m_threadCombo->setCurrentIndex(idx);
    }

    if (m_activeThread == -1 && m_threadCombo->count() > 0) {
        // activate first thread if nothing active
        m_activeThread = m_threadCombo->itemData(0).toInt();
        m_threadCombo->setCurrentIndex(0);
    }
}

void KatePluginGDBView::threadSelected(int thread)
{
    if (thread < 0)
        return;
    m_backend->changeThread(m_threadCombo->itemData(thread).toInt());
}

QString KatePluginGDBView::currentWord()
{
    KTextEditor::View *kv = m_mainWin->activeView();
    if (!kv) {
        qDebug() << "no KTextEditor::View";
        return QString();
    }

    if (!kv->cursorPosition().isValid()) {
        qDebug() << "cursor not valid!";
        return QString();
    }

    int line = kv->cursorPosition().line();
    int col = kv->cursorPosition().column();

    QString linestr = kv->document()->line(line);

    int startPos = qMax(qMin(col, linestr.length() - 1), 0);
    int lindex = linestr.length() - 1;
    int endPos = startPos;
    while (startPos >= 0
           && (linestr[startPos].isLetterOrNumber() || linestr[startPos] == QLatin1Char('_') || linestr[startPos] == QLatin1Char('~')
               || ((startPos > 1) && (linestr[startPos] == QLatin1Char('.')) && !linestr[startPos - 1].isSpace())
               || ((startPos > 2) && (linestr[startPos] == QLatin1Char('>')) && (linestr[startPos - 1] == QLatin1Char('-'))
                   && !linestr[startPos - 2].isSpace()))) {
        if (linestr[startPos] == QLatin1Char('>')) {
            startPos--;
        }
        startPos--;
    }
    while (
        endPos < linestr.length()
        && (linestr[endPos].isLetterOrNumber() || linestr[endPos] == QLatin1Char('_')
            || ((endPos < lindex - 1) && (linestr[endPos] == QLatin1Char('.')) && !linestr[endPos + 1].isSpace())
            || ((endPos < lindex - 2) && (linestr[endPos] == QLatin1Char('-')) && (linestr[endPos + 1] == QLatin1Char('>')) && !linestr[endPos + 2].isSpace())
            || ((endPos > 1) && (linestr[endPos - 1] == QLatin1Char('-')) && (linestr[endPos] == QLatin1Char('>'))))) {
        if (linestr[endPos] == QLatin1Char('-')) {
            endPos++;
        }
        endPos++;
    }
    if (startPos == endPos) {
        qDebug() << "no word found!";
        return QString();
    }

    // qDebug() << linestr.mid(startPos+1, endPos-startPos-1);
    return linestr.mid(startPos + 1, endPos - startPos - 1);
}

void KatePluginGDBView::initDebugToolview()
{
    if (m_configView) {
        return;
    }
    // config page
    m_configView = new ConfigView(nullptr, m_mainWin, m_plugin, m_targetSelectAction);

    connect(m_configView, &ConfigView::showIO, this, &KatePluginGDBView::showIO);

    m_tabWidget->addTab(m_gdbPage, i18nc("Tab label", "Debug Output"));
    m_tabWidget->addTab(m_configView, i18nc("Tab label", "Settings"));
    m_tabWidget->setCurrentWidget(m_configView); // initially show config

    connect(m_configView, &ConfigView::configChanged, this, [this]() {
        if (!m_configView->debuggerIsGDB())
            return;

        GDBTargetConf config = m_configView->currentGDBTarget();
        if (m_backend->targetName() == config.targetName) {
            m_backend->setFileSearchPaths(config.srcPaths);
        }
    });

    m_configView->readConfig(m_sessionConfig);
}

void KatePluginGDBView::slotValue()
{
    QString variable;
    KTextEditor::View *editView = m_mainWin->activeView();
    if (editView && editView->selection() && editView->selectionRange().onSingleLine()) {
        variable = editView->selectionText();
    }

    if (variable.isEmpty()) {
        variable = currentWord();
    }

    if (variable.isEmpty()) {
        return;
    }

    m_inputArea->addToHistory(m_backend->slotPrintVariable(variable));
    m_inputArea->setCurrentItem(QString());

    m_mainWin->showToolView(m_toolView.get());
    m_tabWidget->setCurrentWidget(m_gdbPage);

    QScrollBar *sb = m_outputArea->verticalScrollBar();
    sb->setValue(sb->maximum());
}

void KatePluginGDBView::showIO(bool show)
{
    if (show) {
        m_tabWidget->addTab(m_ioView.get(), i18n("IO"));
    } else {
        m_tabWidget->removeTab(m_tabWidget->indexOf(m_ioView.get()));
    }
}

void KatePluginGDBView::addOutput(const dap::Output &output)
{
    if (output.isSpecialOutput()) {
        addErrorText(output.output);
        return;
    }
    if (m_configView->showIOTab()) {
        if (output.category == dap::Output::Category::Stdout) {
            m_ioView->addStdOutText(output.output);
        } else {
            m_ioView->addStdErrText(output.output);
        }
    } else {
        if (output.category == dap::Output::Category::Stdout) {
            addOutputText(output.output);
        } else {
            addErrorText(output.output);
        }
    }
}

void KatePluginGDBView::addOutputText(QString const &text)
{
    QScrollBar *scrollb = m_outputArea->verticalScrollBar();
    if (!scrollb) {
        return;
    }
    bool atEnd = (scrollb->value() == scrollb->maximum());

    QTextCursor cursor = m_outputArea->textCursor();
    if (!cursor.atEnd()) {
        cursor.movePosition(QTextCursor::End);
    }
    cursor.insertText(text);

    if (atEnd) {
        scrollb->setValue(scrollb->maximum());
    }
}

void KatePluginGDBView::addErrorText(QString const &text)
{
    m_outputArea->setFontItalic(true);
    addOutputText(text);
    m_outputArea->setFontItalic(false);
}

bool KatePluginGDBView::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        auto *ke = static_cast<QKeyEvent *>(event);
        if ((obj == m_toolView.get()) && (ke->key() == Qt::Key_Escape)) {
            m_mainWin->hideToolView(m_toolView.get());
            event->accept();
            return true;
        }
    } else if (event->type() == QEvent::Show) {
        initDebugToolview();
    }
    return QObject::eventFilter(obj, event);
}

void KatePluginGDBView::handleEsc(QEvent *e)
{
    if (!m_mainWin || !m_toolView) {
        return;
    }

    auto *k = static_cast<QKeyEvent *>(e);
    if (k->key() == Qt::Key_Escape && k->modifiers() == Qt::NoModifier) {
        if (m_toolView->isVisible()) {
            m_mainWin->hideToolView(m_toolView.get());
        }
    }
}

void KatePluginGDBView::enableBreakpointMarks(KTextEditor::Document *document)
{
    if (document) {
        document->setEditableMarks(document->editableMarks() | KTextEditor::Document::BreakpointActive);
        document->setMarkDescription(KTextEditor::Document::BreakpointActive, i18n("Breakpoint"));
        document->setMarkIcon(KTextEditor::Document::BreakpointActive, QIcon::fromTheme(QStringLiteral("media-record")));

        connect(document, &KTextEditor::Document::viewCreated, this, &KatePluginGDBView::prepareDocumentBreakpoints);
    }
}

void KatePluginGDBView::enableHotReloadOnSave(KTextEditor::View *view)
{
    QObject::disconnect(m_hotReloadOnSaveConnection);
    if (m_hotReloadOnSaveAction->isEnabled() && m_hotReloadOnSaveAction->isChecked() && view && view->document()) {
        auto doc = view->document();
        m_hotReloadOnSaveConnection = connect(doc, &KTextEditor::Document::documentSavedOrUploaded, &m_hotReloadTimer, qOverload<>(&QTimer::start));
    }
}

void KatePluginGDBView::prepareDocumentBreakpoints(KTextEditor::Document *document)
{
    // If debugger is running and we open the file, check if there is a break point set in every line
    // and add markers accordingly
    if (m_backend->debuggerRunning()) {
        for (auto i = 0; i < document->lines(); i++) {
            if (m_backend->hasBreakpoint(document->url(), i)) {
                document->setMark(i - 1, KTextEditor::Document::MarkTypes::BreakpointActive);
            }
        }
    }
    // Update breakpoints when they're added or removed to the debugger
    connect(document, &KTextEditor::Document::markChanged, this, &KatePluginGDBView::updateBreakpoints);
}

void KatePluginGDBView::updateBreakpoints(const KTextEditor::Document *document, const KTextEditor::Mark mark)
{
    if (mark.type == KTextEditor::Document::MarkTypes::BreakpointActive) {
        if (m_backend->debuggerRunning() && !m_backend->canContinue()) {
            m_backend->slotInterrupt();
        }
        bool added = false;
        m_backend->toggleBreakpoint(document->url(), mark.line + 1, &added);
    }
}

void KatePluginGDBView::displayMessage(const QString &msg, KTextEditor::Message::MessageType level)
{
    KTextEditor::View *kv = m_mainWin->activeView();
    if (!kv) {
        return;
    }

    delete m_infoMessage;
    m_infoMessage = new KTextEditor::Message(msg, level);
    m_infoMessage->setWordWrap(true);
    m_infoMessage->setPosition(KTextEditor::Message::BottomInView);
    m_infoMessage->setAutoHide(8000);
    m_infoMessage->setAutoHideMode(KTextEditor::Message::Immediate);
    m_infoMessage->setView(kv);
    kv->document()->postMessage(m_infoMessage);
}

QToolButton *KatePluginGDBView::createDebugButton(QAction *action)
{
    auto *button = new QToolButton(m_buttonWidget);
    button->setDefaultAction(action);
    button->setAutoRaise(true);
    connect(action, &QAction::visibleChanged, button, [button, action] {
        button->setVisible(action->isVisible());
    });
    button->setVisible(action->isVisible());
    return button;
}

#include "moc_plugin_kategdb.cpp"
#include "plugin_kategdb.moc"
