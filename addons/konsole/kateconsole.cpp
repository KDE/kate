/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2001 Christoph Cullmann <cullmann@kde.org>
   SPDX-FileCopyrightText: 2002 Joseph Wenninger <jowenn@kde.org>
   SPDX-FileCopyrightText: 2002 Anders Lund <anders.lund@lund.tdcadsl.dk>
   SPDX-FileCopyrightText: 2007 Anders Lund <anders@alweb.dk>
   SPDX-FileCopyrightText: 2017 Ederag <edera@gmx.fr>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#include "kateconsole.h"

#include <KLocalizedString>
#include <KTextEditor/Document>
#include <ktexteditor/message.h>
#include <ktexteditor/view.h>

#include <KActionCollection>
#include <KShell>
#include <kde_terminal_interface.h>
#include <kparts/part.h>

#include <KMessageBox>

#include <QAction>
#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QDir>
#include <QFileInfo>
#include <QGroupBox>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPointer>
#include <QPushButton>
#include <QRadioButton>
#include <QShowEvent>
#include <QStyle>
#include <QTabWidget>
#include <QVBoxLayout>

#include <KAuthorized>
#include <KColorScheme>
#include <KConfigGroup>
#include <KPluginFactory>
#include <KSharedConfig>
#include <KXMLGUIFactory>

K_PLUGIN_FACTORY_WITH_JSON(KateKonsolePluginFactory, "katekonsoleplugin.json", registerPlugin<KateKonsolePlugin>();)

QStringList defaultEscapeExceptions()
{
    static const QStringList escapeExceptions{QStringLiteral("vi"), QStringLiteral("vim"), QStringLiteral("nvim"), QStringLiteral("git")};
    return escapeExceptions;
}

// directory to use for given view
static QString directoryForView(const KTextEditor::View *view)
{
    if (const QUrl u = view ? view->document()->url() : QUrl(); u.isValid() && u.isLocalFile()) {
        QFileInfo fi(u.toLocalFile());
        return fi.absolutePath();
    }
    return {};
}

KateKonsolePlugin::KateKonsolePlugin(QObject *parent)
    : KTextEditor::Plugin(parent)
    , m_previousEditorEnv(qgetenv("EDITOR"))
{
}

static void setEditorEnv(const QByteArray &value)
{
    if (value.isNull()) {
        qunsetenv("EDITOR");
    } else {
        qputenv("EDITOR", value.data());
    }
}

KateKonsolePlugin::~KateKonsolePlugin()
{
    setEditorEnv(m_previousEditorEnv);
}

QObject *KateKonsolePlugin::createView(KTextEditor::MainWindow *mainWindow)
{
    if (!KAuthorized::authorize(QStringLiteral("shell_access"))) {
        return nullptr;
    }

    auto *view = new KateKonsolePluginView(this, mainWindow);
    return view;
}

KTextEditor::ConfigPage *KateKonsolePlugin::configPage(int number, QWidget *parent)
{
    if (number != 0) {
        return nullptr;
    }
    return new KateKonsoleConfigPage(parent, this);
}

void KateKonsolePlugin::readConfig()
{
    for (KateKonsolePluginView *view : std::as_const(mViews)) {
        view->readConfig();
    }
}

KateKonsolePluginView::KateKonsolePluginView(KateKonsolePlugin *plugin, KTextEditor::MainWindow *mainWindow)
    : QObject(mainWindow)
    , m_plugin(plugin)
{
    // init console
    QWidget *toolview = mainWindow->createToolView(plugin,
                                                   QStringLiteral("kate_private_plugin_katekonsoleplugin"),
                                                   KTextEditor::MainWindow::Bottom,
                                                   QIcon::fromTheme(QStringLiteral("dialog-scripts")),
                                                   i18n("Terminal"));
    m_console = new KateConsole(m_plugin, mainWindow, toolview);

    connect(toolview, SIGNAL(toolVisibleChanged(bool)), m_console, SLOT(slotChangeVisiblityActionText(bool)));

    // register this view
    m_plugin->mViews.append(this);
}

KateKonsolePluginView::~KateKonsolePluginView()
{
    // unregister this view
    m_plugin->mViews.removeAll(this);

    // cleanup, kill toolview + console
    auto toolview = m_console->parent();
    delete m_console;
    delete toolview;
}

void KateKonsolePluginView::readConfig()
{
    m_console->readConfig();
}

KateConsole::KateConsole(KateKonsolePlugin *plugin, KTextEditor::MainWindow *mw, QWidget *parent)
    : QWidget(parent)
    , m_part(nullptr)
    , m_mw(mw)
    , m_toolView(parent)
    , m_plugin(plugin)
{
    KXMLGUIClient::setComponentName(QStringLiteral("katekonsole"), i18n("Terminal"));
    setXMLFile(QStringLiteral("ui.rc"));

    // make sure we have a vertical layout
    new QVBoxLayout(this);
    layout()->setContentsMargins(0, 1, 0, 0);

    QAction *a = actionCollection()->addAction(QStringLiteral("katekonsole_tools_pipe_to_terminal"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("dialog-scripts")));
    a->setText(i18nc("@action", "&Pipe to Terminal"));
    connect(a, &QAction::triggered, this, &KateConsole::slotPipeToConsole);

    a = actionCollection()->addAction(QStringLiteral("katekonsole_tools_sync"));
    a->setText(i18nc("@action", "S&ynchronize Terminal with Current Document"));
    connect(a, &QAction::triggered, this, &KateConsole::slotManualSync);

    a = actionCollection()->addAction(QStringLiteral("katekonsole_tools_run"));
    a->setText(i18nc("@action", "Run Current Document"));
    connect(a, &QAction::triggered, this, &KateConsole::slotRun);

    a = actionCollection()->addAction(QStringLiteral("katekonsole_tools_toggle_visibility"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("dialog-scripts")));
    a->setText(i18nc("@action", "S&how Terminal Panel"));
    KActionCollection::setDefaultShortcut(a, QKeySequence(Qt::Key_F4));
    connect(a, &QAction::triggered, this, &KateConsole::slotToggleVisibility);

    a = actionCollection()->addAction(QStringLiteral("katekonsole_tools_toggle_focus"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("swap-panels")));
    a->setText(i18nc("@action", "&Focus Terminal Panel"));
    KActionCollection::setDefaultShortcut(a, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_F4));
    connect(a, &QAction::triggered, this, &KateConsole::slotToggleFocus);

    a = actionCollection()->addAction(QStringLiteral("katekonsole_tools_split_view_vertical"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("view-split-left-right")));
    a->setText(i18nc("@action", "&Split Terminal Vertically"));
    connect(a, &QAction::triggered, this, &KateConsole::slotSplitVertical);

    a = actionCollection()->addAction(QStringLiteral("katekonsole_tools_split_view_horizontal"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("view-split-top-bottom")));
    a->setText(i18nc("@action", "&Split Terminal Horizontally"));
    connect(a, &QAction::triggered, this, &KateConsole::slotSplitHorizontal);

    a = actionCollection()->addAction(QStringLiteral("katekonsole_tools_new_tab"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("tab-new")));
    a->setText(i18nc("@action", "&New Terminal Tab"));
    connect(a, &QAction::triggered, this, &KateConsole::slotNewTab);

    connect(m_mw, &KTextEditor::MainWindow::unhandledShortcutOverride, this, &KateConsole::handleEsc);

    m_mw->guiFactory()->addClient(this);

    readConfig();

    connect(qApp, &QApplication::focusChanged, this, &KateConsole::focusChanged);
}

KateConsole::~KateConsole()
{
    // we unplug our actions below, avoid that we trigger their usage
    disconnect(qApp, &QApplication::focusChanged, this, &KateConsole::focusChanged);

    m_mw->guiFactory()->removeClient(this);
    if (m_part) {
        disconnect(m_part, &KParts::ReadOnlyPart::destroyed, this, &KateConsole::slotDestroyed);
    }
}

void KateConsole::loadConsoleIfNeeded(QString directory, bool force)
{
    if (!force) {
        if (!window() || !parent()) {
            return;
        }
        if (!window() || !isVisibleTo(window())) {
            return;
        }
    }

    const bool firstShell = !m_part;
    if (!m_part) {
        m_part = hasKonsole() ? pluginFactory()->create<KParts::ReadOnlyPart>(this) : nullptr;
        if (!m_part) {
            return;
        }

        if (auto konsoleTabWidget = qobject_cast<QTabWidget *>(m_part->widget())) {
            konsoleTabWidget->setTabBarAutoHide(true);
            konsoleTabWidget->installEventFilter(this);
        }
        layout()->addWidget(m_part->widget());

        setFocusProxy(m_part->widget());

        connect(m_part, &KParts::ReadOnlyPart::destroyed, this, &KateConsole::slotDestroyed);
        // clang-format off
        connect(m_part, SIGNAL(overrideShortcut(QKeyEvent*,bool&)), this, SLOT(overrideShortcut(QKeyEvent*,bool&)));
        // clang-format on
    }

    // if we want one terminal per directory, we will try to locate a tab that has the
    // right one or start a new one
    if (auto konsoleTabWidget = qobject_cast<QTabWidget *>(m_part->widget()); konsoleTabWidget && (m_syncMode == SyncCreateTabPerDir)) {
        // if no dir is set, explicitly use the current working dir to be able to reuse even that konsole
        if (directory.isEmpty()) {
            directory = QDir::currentPath();
        }

        // we attach a property to the tabs to find the right one
        QWidget *tabWithRightDirectory = nullptr;

        // use always the first one
        if (firstShell) {
            // mark it and change dir
            tabWithRightDirectory = konsoleTabWidget->currentWidget();
            tabWithRightDirectory->setProperty("kate_shell_directory", directory);
            qobject_cast<TerminalInterface *>(m_part)->showShellInDir(directory);
        }

        // shortcut for common case: on the right tab already
        else if (konsoleTabWidget->currentWidget()->property("kate_shell_directory").toString() == directory) {
            tabWithRightDirectory = konsoleTabWidget->currentWidget();
        }

        // search
        else {
            for (int i = 0; i < konsoleTabWidget->count(); ++i) {
                if (konsoleTabWidget->widget(i)->property("kate_shell_directory").toString() == directory) {
                    tabWithRightDirectory = konsoleTabWidget->widget(i);
                    break;
                }
            }
        }

        // found the right one? make it active
        if (tabWithRightDirectory) {
            konsoleTabWidget->setCurrentWidget(tabWithRightDirectory);
        } else {
            // trigger new shell tab and mark it
            QMetaObject::invokeMethod(m_part, "createSession", Q_ARG(QString, QString()), Q_ARG(QString, directory));
            konsoleTabWidget->currentWidget()->setProperty("kate_shell_directory", directory);
        }
    } else if (firstShell) {
        // start the terminal on the first run
        qobject_cast<TerminalInterface *>(m_part)->showShellInDir((m_syncMode == SyncNothing) ? QString() : directory);
    }
}

static bool isCtrlShiftT(QKeyEvent *ke)
{
    return ke->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier) && ke->key() == Qt::Key_T;
}

bool KateConsole::eventFilter(QObject *w, QEvent *e)
{
    if (!m_part) {
        return QWidget::eventFilter(w, e);
    }

    if (e->type() == QEvent::KeyPress || e->type() == QEvent::ShortcutOverride) {
        auto *keyEvent = static_cast<QKeyEvent *>(e);
        if (isCtrlShiftT(keyEvent)) {
            e->accept();
            QMetaObject::invokeMethod(m_part, "newTab");
            return true;
        }
    }

    return QWidget::eventFilter(w, e);
}

void KateConsole::slotDestroyed()
{
    m_part = nullptr;
    m_currentPath.clear();
    setFocusProxy(nullptr);

    // hide the dockwidget
    if (parent()) {
        m_mw->hideToolView(m_toolView);
    }
}

void KateConsole::overrideShortcut(QKeyEvent *, bool &override)
{
    /**
     * let konsole handle all shortcuts
     */
    override = true;
}

void KateConsole::showEvent(QShowEvent *)
{
    loadConsoleIfNeeded(directoryForView(m_mw->activeView()));
}

void KateConsole::paintEvent(QPaintEvent *e)
{
    if (hasKonsole()) {
        QWidget::paintEvent(e);
        return;
    }
    QPainter p(this);
    p.setPen(QPen(KColorScheme().foreground(KColorScheme::NegativeText), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRect(rect().adjusted(1, 1, -1, -1));
    auto font = p.font();
    font.setPixelSize(20);
    p.setFont(font);
    const QString text = i18n("Konsole not installed. Please install konsole to be able to use the terminal.");
    p.drawText(rect(), Qt::AlignCenter | Qt::TextWordWrap, text);
}

static constexpr QLatin1String eolChar()
{
    // On windows, if the shell is powershell
    // '\r' needs to be sent to trigger enter
#ifdef Q_OS_WIN
    return QLatin1String("\r\n");
#else
    return QLatin1String("\n");
#endif
}

void KateConsole::cd(const QString &path)
{
    if (m_currentPath == path) {
        return;
    }

    if (!m_part) {
        return;
    }

    m_currentPath = path;
    QString command = QLatin1String(" cd ") + KShell::quoteArg(m_currentPath) + eolChar();

    // special handling for some interpreters
    TerminalInterface *t = qobject_cast<TerminalInterface *>(m_part);
    if (t) {
        // ghci doesn't allow \space dir names, does allow spaces in dir names
        // irb can take spaces or \space but doesn't allow " 'path' "
        if (t->foregroundProcessName() == QLatin1String("irb")) {
            command = QLatin1String("Dir.chdir(\"") + path + QLatin1String("\") \n");
        } else if (t->foregroundProcessName() == QLatin1String("ghc")) {
            command = QLatin1String(":cd ") + path + u'\n';
        } else if (!t->foregroundProcessName().isEmpty()) {
            // If something is running, dont try to cd anywhere
            return;
        }
    }

#ifndef Q_OS_WIN // Doesn't work with PS or cmd.exe on windows
    // Send prior Ctrl-E, Ctrl-U to ensure the line is empty
    sendInput(QStringLiteral("\x05\x15"));
#endif
    sendInput(command);
}

void KateConsole::sendInput(const QString &text)
{
    loadConsoleIfNeeded();

    if (!m_part) {
        return;
    }

    if (TerminalInterface *t = qobject_cast<TerminalInterface *>(m_part)) {
        t->sendInput(text);
    }
}

KPluginFactory *KateConsole::pluginFactory()
{
    if (s_pluginFactory) {
        return s_pluginFactory;
    }
    const QString konsolePart = QStringLiteral("kf6/parts/konsolepart");
    return s_pluginFactory = KPluginFactory::loadFactory(konsolePart).plugin;
}

void KateConsole::slotPipeToConsole()
{
    if (KMessageBox::warningContinueCancel(
            m_mw->window(),
            i18n("Do you really want to pipe the text to the console? This will execute any contained commands with your user rights."),
            i18n("Pipe to Terminal?"),
            KGuiItem(i18n("Pipe to Terminal")),
            KStandardGuiItem::cancel(),
            QStringLiteral("Pipe To Terminal Warning"))
        != KMessageBox::Continue) {
        return;
    }

    KTextEditor::View *v = m_mw->activeView();

    if (!v) {
        return;
    }

    if (!m_part) {
        loadConsoleIfNeeded(directoryForView(v), /*force=*/true);
    }

    if (v->selection()) {
        sendInput(v->selectionText());
    } else {
        sendInput(v->document()->text());
    }
}

void KateConsole::slotSync()
{
    const auto newDir = directoryForView(m_mw->activeView());
    loadConsoleIfNeeded(newDir);
    if ((m_syncMode == SyncCurrentTab) && !newDir.isEmpty()) {
        cd(newDir);
    }
}

void KateConsole::slotViewOrUrlChanged(KTextEditor::View *view)
{
    disconnect(m_urlChangedConnection);
    if (view) {
        KTextEditor::Document *doc = view->document();
        m_urlChangedConnection = connect(doc, &KParts::ReadOnlyPart::urlChanged, this, &KateConsole::slotSync);
    }

    slotSync();
}

void KateConsole::slotManualSync()
{
    if (!m_part || !m_part->widget()->isVisible()) {
        m_mw->showToolView(qobject_cast<QWidget *>(parent()));
    }

    m_currentPath.clear();
    const auto newDir = directoryForView(m_mw->activeView());
    loadConsoleIfNeeded(newDir);
    if (!newDir.isEmpty()) {
        cd(newDir);
    }
}

void KateConsole::slotRun()
{
    KTextEditor::View *view = m_mw->activeView();
    if (!view) {
        return;
    }

    KTextEditor::Document *document = view->document();
    const QUrl url = document->url();
    if (!url.isLocalFile()) {
        QPointer<KTextEditor::Message> message = new KTextEditor::Message(i18n("Not a local file: '%1'", url.toDisplayString()), KTextEditor::Message::Error);
        // auto hide is enabled and set to a sane default value of several seconds.
        message->setAutoHide(2000);
        message->setAutoHideMode(KTextEditor::Message::Immediate);
        document->postMessage(message);
        return;
    }
    // ensure that file is saved
    if (document->isModified()) {
        document->save();
    }

    KConfigGroup cg(KSharedConfig::openConfig(), QStringLiteral("Konsole"));
    // The string that should be output to terminal, upon acceptance
    QString output_str;
    // Set prefix first
    QString first_line = document->line(0);
    const QLatin1String shebang("#!");
    if (first_line.startsWith(shebang)) {
        // If there's a shebang, respect it
        output_str += first_line.remove(0, shebang.size()).append(u' ');
    } else {
        output_str += cg.readEntry("RunPrefix", "");
    }

    // then filename
    QFileInfo fileInfo(url.toLocalFile());
    const bool removeExt = cg.readEntry("RemoveExtension", false);
    // append filename without extension (i.e. keep only the basename)
    const QString path = fileInfo.absolutePath() + u'/' + (removeExt ? fileInfo.baseName() : fileInfo.fileName());
    output_str += KShell::quoteArg(path);

    const QString msg = i18n(
        "Do you really want to Run the document ?\n"
        "This will execute the following command,\n"
        "with your user rights, in the terminal:\n"
        "'%1'",
        output_str);
    const auto result = KMessageBox::warningContinueCancel(m_mw->window(),
                                                           msg,
                                                           i18n("Run in Terminal?"),
                                                           KGuiItem(i18n("Run")),
                                                           KStandardGuiItem::cancel(),
                                                           QStringLiteral("Konsole: Run in Terminal Warning"));
    if (result != KMessageBox::Continue) {
        return;
    }

    if (!m_part) {
        loadConsoleIfNeeded(directoryForView(view), /*force=*/true);
    }

    // echo to terminal
    sendInput(output_str + eolChar());
}

void KateConsole::slotToggleVisibility()
{
    if (!m_part || !m_part->widget()->isVisible()) {
        m_mw->showToolView(qobject_cast<QWidget *>(parent()));
    } else {
        m_mw->hideToolView(m_toolView);
    }
    slotChangeVisiblityActionText({});
}

void KateConsole::slotChangeVisiblityActionText(bool)
{
    QAction *action = actionCollection()->action(QStringLiteral("katekonsole_tools_toggle_visibility"));
    if (!m_toolView || m_toolView->isHidden()) {
        action->setText(i18nc("@action", "S&how Terminal Panel"));
    } else {
        action->setText(i18nc("@action", "&Hide Terminal Panel"));
    }
}

void KateConsole::focusChanged(QWidget *, QWidget *now)
{
    QAction *action = actionCollection()->action(QStringLiteral("katekonsole_tools_toggle_focus"));
    if (m_part && m_part->widget()->isAncestorOf(now)) {
        action->setText(i18n("Defocus Terminal Panel"));
    } else if (action->text() != i18n("Focus Terminal Panel")) {
        action->setText(i18n("Focus Terminal Panel"));
    }
}

void KateConsole::slotSplitVertical()
{
    if (!m_part) {
        return;
    }
    auto splitAction = m_part->action(QStringLiteral("split-view-left-right"));
    if (splitAction) {
        splitAction->setEnabled(true);
        splitAction->activate(QAction::Trigger);
    }
}

void KateConsole::slotSplitHorizontal()
{
    if (!m_part) {
        return;
    }
    auto splitAction = m_part->action(QStringLiteral("split-view-top-bottom"));
    if (splitAction) {
        splitAction->setEnabled(true);
        splitAction->activate(QAction::Trigger);
    }
}

void KateConsole::slotNewTab()
{
    if (!m_part) {
        return;
    }
    QMetaObject::invokeMethod(m_part, "newTab");
}

void KateConsole::slotToggleFocus()
{
    if (!m_part) {
        m_mw->showToolView(qobject_cast<QWidget *>(parent()));
        return; // this shows and focuses the konsole
    }

    if (m_part->widget()->hasFocus()) {
        if (m_mw->activeView()) {
            m_mw->activeView()->setFocus();
        }
    } else {
        // show the view if it is hidden
        if (auto p = qobject_cast<QWidget *>(parent()); p->isHidden()) {
            m_mw->showToolView(p);
        } else { // should focus the widget too!
            m_part->widget()->setFocus(Qt::OtherFocusReason);
        }
    }
}

// default is the least intrusive: no sync, people can chose
static constexpr int defaultAutoSyncronizeMode = KateConsole::SyncNothing;

void KateConsole::readConfig()
{
    m_syncMode = static_cast<KateConsole::SyncMode>(
        KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("Konsole")).readEntry("AutoSyncronizeMode", defaultAutoSyncronizeMode));

    disconnect(m_mw, &KTextEditor::MainWindow::viewChanged, this, &KateConsole::slotViewOrUrlChanged);
    disconnect(m_urlChangedConnection);
    if (m_syncMode != SyncNothing) {
        connect(m_mw, &KTextEditor::MainWindow::viewChanged, this, &KateConsole::slotViewOrUrlChanged);
    }

    if (KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("Konsole")).readEntry("SetEditor", false)) {
        qputenv("EDITOR", "kate -b");
    } else {
        setEditorEnv(m_plugin->previousEditorEnv());
    }
}

void KateConsole::handleEsc(QEvent *e)
{
    if (!KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("Konsole")).readEntry("KonsoleEscKeyBehaviour", true)) {
        return;
    }

    QStringList exceptList =
        KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("Konsole")).readEntry("KonsoleEscKeyExceptions", defaultEscapeExceptions());

    if (!m_mw || !m_toolView || !e) {
        return;
    }

    auto *k = static_cast<QKeyEvent *>(e);
    if (k->key() == Qt::Key_Escape && k->modifiers() == Qt::NoModifier) {
        if (m_part) {
            const auto app = qobject_cast<TerminalInterface *>(m_part)->foregroundProcessName();
            if (m_toolView && m_toolView->isVisible() && !exceptList.contains(app)) {
                m_mw->hideToolView(m_toolView);
            }
        } else if (m_toolView && m_toolView->isVisible()) {
            m_mw->hideToolView(m_toolView);
        }
    }
}

KateKonsoleConfigPage::KateKonsoleConfigPage(QWidget *parent, KateKonsolePlugin *plugin)
    : KTextEditor::ConfigPage(parent)
    , mPlugin(plugin)
{
    auto *lo = new QVBoxLayout(this);
    lo->setSpacing(QApplication::style()->pixelMetric(QStyle::PM_LayoutHorizontalSpacing));

    auto *vboxSync = new QVBoxLayout;
    auto groupSync = new QGroupBox(i18n("Terminal Synchronization Mode"), this);
    m_syncMode = new QButtonGroup(this);
    m_syncMode->setExclusive(true);
    auto sync = new QRadioButton(i18n("&Don't synchronize terminal tab with current document"), groupSync);
    vboxSync->addWidget(sync);
    m_syncMode->addButton(sync, KateConsole::SyncNothing);
    sync = new QRadioButton(i18n("&Automatically synchronize the current terminal tab with the current document when possible"), groupSync);
    vboxSync->addWidget(sync);
    m_syncMode->addButton(sync, KateConsole::SyncCurrentTab);
    sync = new QRadioButton(i18n("&Automatically create a terminal tab for the directory of the current document and switch to it"), groupSync);
    vboxSync->addWidget(sync);
    m_syncMode->addButton(sync, KateConsole::SyncCreateTabPerDir);
    groupSync->setLayout(vboxSync);
    lo->addWidget(groupSync);

    auto *vboxRun = new QVBoxLayout;
    auto *groupRun = new QGroupBox(i18n("Run in terminal"), this);
    // Remove extension
    cbRemoveExtension = new QCheckBox(i18n("&Remove extension"), this);
    vboxRun->addWidget(cbRemoveExtension);
    // Prefix
    auto *framePrefix = new QFrame(this);
    auto *hboxPrefix = new QHBoxLayout(framePrefix);
    auto *label = new QLabel(i18n("Prefix:"), framePrefix);
    hboxPrefix->addWidget(label);
    lePrefix = new QLineEdit(framePrefix);
    hboxPrefix->addWidget(lePrefix);
    vboxRun->addWidget(framePrefix);
    // show warning next time
    auto *frameWarn = new QFrame(this);
    auto *hboxWarn = new QHBoxLayout(frameWarn);
    auto *buttonWarn = new QPushButton(i18n("&Show warning next time"), frameWarn);
    buttonWarn->setWhatsThis(
        i18n("The next time '%1' is executed, "
             "make sure a warning window will pop up, "
             "displaying the command to be sent to terminal, "
             "for review.",
             i18n("Run in terminal")));
    connect(buttonWarn, &QPushButton::pressed, this, &KateKonsoleConfigPage::slotEnableRunWarning);
    hboxWarn->addWidget(buttonWarn);
    vboxRun->addWidget(frameWarn);
    groupRun->setLayout(vboxRun);
    lo->addWidget(groupRun);

    cbSetEditor = new QCheckBox(i18n("Set &EDITOR environment variable to 'kate -b'"), this);
    lo->addWidget(cbSetEditor);
    auto *tmp = new QLabel(this);
    tmp->setText(i18n("Important: The document has to be closed to make the console application continue"));
    lo->addWidget(tmp);

    cbSetEscHideKonsole = new QCheckBox(i18n("Hide Konsole on pressing 'Esc'"));
    lo->addWidget(cbSetEscHideKonsole);
    auto *hideKonsoleLabel =
        new QLabel(i18n("This may cause issues with terminal apps that use Esc key, for e.g., vim. Add these apps in the input below (Comma separated list)"),
                   this);
    hideKonsoleLabel->setWordWrap(true);
    lo->addWidget(hideKonsoleLabel);

    leEscExceptions = new QLineEdit(this);
    lo->addWidget(leEscExceptions);

    reset();
    lo->addStretch();

    connect(m_syncMode, &QButtonGroup::buttonToggled, this, &KateKonsoleConfigPage::changed);

#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
    connect(cbRemoveExtension, &QCheckBox::stateChanged, this, &KTextEditor::ConfigPage::changed);
#else
    connect(cbRemoveExtension, &QCheckBox::checkStateChanged, this, &KTextEditor::ConfigPage::changed);
#endif
    connect(lePrefix, &QLineEdit::textChanged, this, &KateKonsoleConfigPage::changed);
#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
    connect(cbSetEditor, &QCheckBox::stateChanged, this, &KateKonsoleConfigPage::changed);
#else
    connect(cbSetEditor, &QCheckBox::checkStateChanged, this, &KateKonsoleConfigPage::changed);
#endif
#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
    connect(cbSetEscHideKonsole, &QCheckBox::stateChanged, this, &KateKonsoleConfigPage::changed);
#else
    connect(cbSetEscHideKonsole, &QCheckBox::checkStateChanged, this, &KateKonsoleConfigPage::changed);
#endif
    connect(leEscExceptions, &QLineEdit::textChanged, this, &KateKonsoleConfigPage::changed);
}

void KateKonsoleConfigPage::slotEnableRunWarning()
{
    KMessageBox::enableMessage(QStringLiteral("Konsole: Run in Terminal Warning"));
}

QString KateKonsoleConfigPage::name() const
{
    return i18n("Terminal");
}

QString KateKonsoleConfigPage::fullName() const
{
    return i18n("Terminal Settings");
}

QIcon KateKonsoleConfigPage::icon() const
{
    return QIcon::fromTheme(QStringLiteral("dialog-scripts"));
}

void KateKonsoleConfigPage::apply()
{
    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("Konsole"));
    config.writeEntry("AutoSyncronizeMode", m_syncMode->checkedId());
    config.writeEntry("RemoveExtension", cbRemoveExtension->isChecked());
    config.writeEntry("RunPrefix", lePrefix->text());
    config.writeEntry("SetEditor", cbSetEditor->isChecked());
    config.writeEntry("KonsoleEscKeyBehaviour", cbSetEscHideKonsole->isChecked());
    config.writeEntry("KonsoleEscKeyExceptions", leEscExceptions->text().split(u','));
    config.sync();
    mPlugin->readConfig();
}

void KateKonsoleConfigPage::reset()
{
    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("Konsole"));
    m_syncMode->button(config.readEntry("AutoSyncronizeMode", defaultAutoSyncronizeMode))->setChecked(true);
    cbRemoveExtension->setChecked(config.readEntry("RemoveExtension", false));
    lePrefix->setText(config.readEntry("RunPrefix", ""));
    cbSetEditor->setChecked(config.readEntry("SetEditor", false));
    cbSetEscHideKonsole->setChecked(config.readEntry("KonsoleEscKeyBehaviour", true));
    leEscExceptions->setText(config.readEntry("KonsoleEscKeyExceptions", defaultEscapeExceptions()).join(u','));
}

#include "kateconsole.moc"
#include "moc_kateconsole.cpp"

// kate: space-indent on; indent-width 4; replace-tabs on;
