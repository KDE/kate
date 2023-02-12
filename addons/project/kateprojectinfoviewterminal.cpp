/*  This file is part of the Kate project.
 *
 *  SPDX-FileCopyrightText: 2012 Christoph Cullmann <cullmann@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "kateprojectinfoviewterminal.h"
#include "kateprojectpluginview.h"

#include "KateTerminalWidget.h"

#include <KActionCollection>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KSharedConfig>
#include <KShell>
#include <KTextEditor/MainWindow>
#include <KXMLGUIClient>
#include <KXMLGUIFactory>
#include <kde_terminal_interface.h>
#include <ktexteditor_utils.h>

#include <QTabWidget>

KPluginFactory *KateProjectInfoViewTerminal::s_pluginFactory = nullptr;

static QAction *actionFromPlugin(KTextEditor::MainWindow *mainWindow, const QString &pluginName, const QString &actionName)
{
    auto f = mainWindow->guiFactory();
    if (f) {
        const auto clients = f->clients();
        for (auto *c : clients) {
            if (c && c->componentName() == pluginName) {
                return c->actionCollection()->action(actionName);
            }
        }
    }
    return nullptr;
}

KateProjectInfoViewTerminal::KateProjectInfoViewTerminal(KateProjectPluginView *pluginView, const QString &directory)
    : m_pluginView(pluginView)
    , m_directory(directory)
    , m_konsolePart(nullptr)
{
    /**
     * layout widget
     */
    m_layout = new QVBoxLayout(this);
    m_layout->setSpacing(0);
    m_layout->setContentsMargins(0, 0, 0, 0);

    m_showProjectInfoViewAction = Utils::toolviewShowAction(m_pluginView->mainWindow(), QStringLiteral("kateprojectinfo"));

    m_searchInFilesAction = actionFromPlugin(pluginView->mainWindow(), QStringLiteral("katesearch"), QStringLiteral("search_in_files"));
}

KateProjectInfoViewTerminal::~KateProjectInfoViewTerminal()
{
    /**
     * avoid endless loop
     */
    if (m_konsolePart) {
        disconnect(m_konsolePart, &KParts::ReadOnlyPart::destroyed, this, &KateProjectInfoViewTerminal::loadTerminal);
    }

    if (m_termWidget) {
        disconnect(m_termWidget, &QObject::destroyed, this, &KateProjectInfoViewTerminal::loadTerminal);
    }
}

KPluginFactory *KateProjectInfoViewTerminal::pluginFactory()
{
    if (s_pluginFactory) {
        return s_pluginFactory;
    }
    return s_pluginFactory = KPluginFactory::loadFactory(QStringLiteral("konsolepart")).plugin;
}

void KateProjectInfoViewTerminal::showEvent(QShowEvent *)
{
    /**
     * we delay the terminal construction until we have some part to have a usable WINDOWID, see bug 411965
     */
    if (!m_konsolePart && !m_termWidget) {
        loadTerminal();
    }
    if (m_searchInFilesAction && hasFocus()) {
        m_searchInFilesAction->setEnabled(false);
    }
}

void KateProjectInfoViewTerminal::loadTerminal()
{
    if (hasKonsole() && !forceOwnTerm()) {
        /**
         * null in any case, if loadTerminal fails below and we are in the destroyed event
         */
        m_konsolePart = nullptr;
        setFocusProxy(nullptr);

        /**
         * we shall not arrive here without a factory, if it is not there, no terminal toolview shall be created
         */
        Q_ASSERT(pluginFactory());

        /**
         * create part
         */
        m_konsolePart = pluginFactory()->create<KParts::ReadOnlyPart>(this);
        if (!m_konsolePart) {
            return;
        }

        /**
         * init locale translation stuff
         */
        // FIXME KF5 KGlobal::locale()->insertCatalog("konsole");

        /**
         * switch to right directory
         */
        qobject_cast<TerminalInterface *>(m_konsolePart)->showShellInDir(m_directory);

        /**
         * add to widget
         */
        if (auto konsoleTabWidget = qobject_cast<QTabWidget *>(m_konsolePart->widget())) {
            konsoleTabWidget->setTabBarAutoHide(true);
            konsoleTabWidget->installEventFilter(this);
        }
        m_layout->addWidget(m_konsolePart->widget());

        setFocusProxy(m_konsolePart->widget());

        /**
         * guard destruction, create new terminal!
         */
        connect(m_konsolePart, &KParts::ReadOnlyPart::destroyed, this, &KateProjectInfoViewTerminal::loadTerminal);
        // clang-format off
        connect(m_konsolePart, SIGNAL(overrideShortcut(QKeyEvent*,bool&)), this, SLOT(overrideShortcut(QKeyEvent*,bool&)));
        // clang-format on
    } else {
        m_termWidget = new KateTerminalWidget(this);
        m_layout->addWidget(m_termWidget);
        setFocusProxy(m_termWidget);
        m_termWidget->showShellInDir(m_directory);
        m_termWidget->installEventFilter(this);

        connect(m_termWidget, &QObject::destroyed, this, &KateProjectInfoViewTerminal::loadTerminal);
    }
}

void KateProjectInfoViewTerminal::overrideShortcut(QKeyEvent *keyEvent, bool &override)
{
    if (m_showProjectInfoViewAction && !m_showProjectInfoViewAction->shortcut().isEmpty()) {
        int modifiers = keyEvent->modifiers();
        int key = keyEvent->key();
        QKeySequence k(modifiers | key);
        if (m_showProjectInfoViewAction->shortcut().matches(k)) {
            override = false;
            return;
        }
    }

    /**
     * let konsole handle all shortcuts
     */
    override = true;
}

// share with konsole plugin
static const QStringList s_escapeExceptions{QStringLiteral("vi"), QStringLiteral("vim"), QStringLiteral("nvim")};

bool KateProjectInfoViewTerminal::ignoreEsc() const
{
    if (hasKonsole() && !forceOwnTerm()) {
        if (!m_konsolePart || !KConfigGroup(KSharedConfig::openConfig(), "Konsole").readEntry("KonsoleEscKeyBehaviour", true)) {
            return false;
        }

        const QStringList exceptList = KConfigGroup(KSharedConfig::openConfig(), "Konsole").readEntry("KonsoleEscKeyExceptions", s_escapeExceptions);
        const auto app = qobject_cast<TerminalInterface *>(m_konsolePart)->foregroundProcessName();
        return exceptList.contains(app);
    }
    // KateTerminalWidget doesn't have foregroundProcessName
    return false;
}

bool KateProjectInfoViewTerminal::isLoadable()
{
    return KateTerminalWidget::isAvailable() || (pluginFactory() != nullptr);
}

void KateProjectInfoViewTerminal::respawn(const QString &directory)
{
    if (!isLoadable()) {
        return;
    }

    if (hasKonsole() && !forceOwnTerm()) {
        m_directory = directory;
        disconnect(m_konsolePart, &KParts::ReadOnlyPart::destroyed, this, &KateProjectInfoViewTerminal::loadTerminal);

        if (m_konsolePart != nullptr) {
            delete m_konsolePart;
        }
    } else {
        if (m_termWidget) {
            disconnect(m_termWidget, &QObject::destroyed, this, &KateProjectInfoViewTerminal::loadTerminal);
        }
        delete m_termWidget;
        m_termWidget = nullptr;
    }

    loadTerminal();
}

static bool isCtrlShiftT(QKeyEvent *ke)
{
    return ke->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier) && ke->key() == Qt::Key_T;
}

bool KateProjectInfoViewTerminal::eventFilter(QObject *w, QEvent *e)
{
    if (!m_konsolePart && !m_termWidget) {
        return QWidget::eventFilter(w, e);
    }

    // Disable search in files action as it clashes with konsole's search shortcut
    // with default shortcuts
    if (m_searchInFilesAction) {
        if (e->type() == QEvent::Enter) {
            m_searchInFilesAction->setEnabled(false);
        } else if (e->type() == QEvent::Leave) {
            m_searchInFilesAction->setEnabled(true);
        }
    }

    if (e->type() == QEvent::KeyPress || e->type() == QEvent::ShortcutOverride) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(e);
        if (isCtrlShiftT(keyEvent)) {
            e->accept();
            if (m_konsolePart) {
                auto tiface = qobject_cast<TerminalInterface *>(m_konsolePart);
                const auto profile = QString{};
                const auto workingDir = tiface->currentWorkingDirectory();
                QMetaObject::invokeMethod(m_konsolePart, "createSession", Q_ARG(QString, profile), Q_ARG(QString, workingDir));
                return true;
            } else if (m_termWidget) {
                const auto profile = QString{};
                const auto workingDir = m_termWidget->currentWorkingDirectory();
                m_termWidget->createSession({}, workingDir);
            }
        }
    }

    return QWidget::eventFilter(w, e);
}

void KateProjectInfoViewTerminal::runCommand(const QString &workingDir, const QString &cmd)
{
    if (hasKonsole() && !forceOwnTerm()) {
        auto terminal = qobject_cast<TerminalInterface *>(m_konsolePart);
        if (!terminal) {
            loadTerminal();
        }
        terminal->sendInput(QStringLiteral("\x05\x15"));
        const QString changeDirCmd = QStringLiteral("cd ") + KShell::quoteArg(workingDir) + QStringLiteral("\n");
        terminal->sendInput(changeDirCmd);
        terminal->sendInput(cmd.trimmed() + QStringLiteral("\n"));
    } else if (isLoadable()) {
        if (!m_termWidget) {
            loadTerminal();
        }
        m_termWidget->sendInput(QStringLiteral("\x05\x15"));
        const QString changeDirCmd = QStringLiteral("cd ") + KShell::quoteArg(workingDir) + QStringLiteral("\n");
        m_termWidget->sendInput(changeDirCmd);
        m_termWidget->sendInput(cmd.trimmed() + QStringLiteral("\n"));
    }
}
