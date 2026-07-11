/*
    SPDX-FileCopyrightText: 2026

    SPDX-License-Identifier: MIT
*/

#include "acpclientplugin.h"
#include "acpclient_debug.h"
#include "acpclientconfigpage.h"
#include "acpclientpluginview.h"
#include "acpclientservermanager.h"

#include <KConfigGroup>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KSharedConfig>

#include <QApplication>
#include <QDir>
#include <QJsonObject>
#include <QMessageBox>
#include <QStandardPaths>
#include <QTimer>
#include <memory>

QString acpClientConfigGroup()
{
    return QStringLiteral("acpclient");
}

static constexpr char CONFIG_AUTO_START[] = "AutoStartSession";
static constexpr char CONFIG_SHOW_NOTIFICATIONS[] = "ShowNotifications";
static constexpr char CONFIG_SHOW_TOOL_CALLS[] = "ShowToolCalls";
static constexpr char CONFIG_SHOW_PROGRESS[] = "ShowProgress";
static constexpr char CONFIG_DEBUG_MODE[] = "DebugMode";
static constexpr char CONFIG_SERVER_CONFIG[] = "ServerConfiguration";
static constexpr char CONFIG_ALLOWED_COMMANDS[] = "AllowedServerCommandLines";
static constexpr char CONFIG_BLOCKED_COMMANDS[] = "BlockedServerCommandLines";

K_PLUGIN_FACTORY_WITH_JSON(ACPClientPluginFactory, "acpclientplugin.json", registerPlugin<ACPClientPlugin>();)

static const bool debug = (qEnvironmentVariableIntValue("ACPCLIENT_DEBUG") == 1);
static QLoggingCategory::CategoryFilter oldCategoryFilter = nullptr;
static void myCategoryFilter(QLoggingCategory *category)
{
    // Deactivate info and debug if not debug mode
    if (qstrcmp(category->categoryName(), "kateacpclientplugin") == 0) {
        category->setEnabled(QtInfoMsg, debug);
        category->setEnabled(QtDebugMsg, debug);
    } else if (oldCategoryFilter) {
        oldCategoryFilter(category);
    }
}

ACPClientPlugin::ACPClientPlugin(QObject *parent)
    : KTextEditor::Plugin(parent)
    , m_settingsPath(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + QStringLiteral("/acpclient"))
    , m_defaultConfigPath(QUrl::fromLocalFile(m_settingsPath + QStringLiteral("/settings.json")))
    , m_debugMode(debug)
{
    qCDebug(ACPCLIENT) << "ACPClientPlugin created";

    // Ensure settings path exists
    QDir().mkpath(m_settingsPath);

    // Ensure we don't spam the user with debug messages per default
    if (!oldCategoryFilter) {
        oldCategoryFilter = QLoggingCategory::installFilter(myCategoryFilter);
    }

    // Apply our config
    readConfig();
}

ACPClientPlugin::~ACPClientPlugin() = default;

QObject *ACPClientPlugin::createView(KTextEditor::MainWindow *mainWindow)
{
    qCDebug(ACPCLIENT) << "Creating view for main window";

    if (!m_serverManager) {
        m_serverManager = std::shared_ptr<ACPClientServerManager>(ACPClientServerManager::new_(this, this));
    }

    auto view = new ACPClientPluginView(this, mainWindow, m_serverManager);
    m_views.append(view);

    // Connect signals
    connect(view, &ACPClientPluginView::sessionRequested, this, [this](const QString &prompt) {
        // Create a new session and send the prompt
        QString sessionId = m_serverManager->createSession();
        if (!sessionId.isEmpty()) {
            m_serverManager->sendPrompt(sessionId, prompt);
        }
    });

    connect(view, &ACPClientPluginView::toolCallRequested, this, [this](const QString &toolId, const QJsonObject &arguments) {
        m_serverManager->callTool(toolId, arguments);
    });

    connect(this, &ACPClientPlugin::showMessage, mainWindow, [](KTextEditor::Message::MessageType level, const QString &msg) {
        KTextEditor::Message *message = new KTextEditor::Message(msg, level);
        message->setPosition(KTextEditor::Message::BottomInView);
        message->setWordWrap(true);
        message->setAutoHide(5000);
        // message->show(); FIXME
    });

    return view;
}

int ACPClientPlugin::configPages() const
{
    return 1; // We have one config page
}

KTextEditor::ConfigPage *ACPClientPlugin::configPage(int number, QWidget *parent)
{
    if (number != 0) {
        return nullptr;
    }
    return new ACPClientConfigPage(this, parent);
}

void ACPClientPlugin::writeConfig() const
{
    qCDebug(ACPCLIENT) << "Writing config";

    KConfigGroup config(KSharedConfig::openConfig(), acpClientConfigGroup());

    config.writeEntry(CONFIG_AUTO_START, m_autoStartSession);
    config.writeEntry(CONFIG_SHOW_NOTIFICATIONS, m_showNotifications);
    config.writeEntry(CONFIG_SHOW_TOOL_CALLS, m_showToolCalls);
    config.writeEntry(CONFIG_SHOW_PROGRESS, m_showProgress);
    config.writeEntry(CONFIG_DEBUG_MODE, m_debugMode);

    // Write server command line permissions
    for (const auto &[cmdline, allowed] : m_serverCommandLineToAllowedState) {
        const auto key = allowed ? CONFIG_ALLOWED_COMMANDS : CONFIG_BLOCKED_COMMANDS;
        // Store as a string list
        QStringList cmdlines = config.readEntry(key, QStringList());
        if (allowed) {
            if (!cmdlines.contains(cmdline)) {
                cmdlines.append(cmdline);
            }
        } else {
            cmdlines.removeAll(cmdline);
        }
        config.writeEntry(key, cmdlines);
    }

    config.sync();
}

void ACPClientPlugin::readConfig()
{
    qCDebug(ACPCLIENT) << "Reading config";

    KConfigGroup config(KSharedConfig::openConfig(), acpClientConfigGroup());

    m_autoStartSession = config.readEntry(CONFIG_AUTO_START, false);
    m_showNotifications = config.readEntry(CONFIG_SHOW_NOTIFICATIONS, true);
    m_showToolCalls = config.readEntry(CONFIG_SHOW_TOOL_CALLS, true);
    m_showProgress = config.readEntry(CONFIG_SHOW_PROGRESS, true);
    m_debugMode = config.readEntry(CONFIG_DEBUG_MODE, debug);

    // Read allowed and blocked command lines
    QStringList allowedCmdlines = config.readEntry(CONFIG_ALLOWED_COMMANDS, QStringList());
    QStringList blockedCmdlines = config.readEntry(CONFIG_BLOCKED_COMMANDS, QStringList());

    for (const QString &cmdline : allowedCmdlines) {
        m_serverCommandLineToAllowedState[cmdline] = true;
    }
    for (const QString &cmdline : blockedCmdlines) {
        m_serverCommandLineToAllowedState[cmdline] = false;
    }
}

bool ACPClientPlugin::isCommandLineAllowed(const QStringList &cmdline)
{
    QString fullCommandLine = cmdline.join(QLatin1Char(' '));

    auto it = m_serverCommandLineToAllowedState.find(fullCommandLine);
    if (it != m_serverCommandLineToAllowedState.end()) {
        return it->second;
    }

    // If we don't have a stored decision, ask the user
    if (m_currentActiveCommandLineDialogs.find(fullCommandLine) == m_currentActiveCommandLineDialogs.end()) {
        m_currentActiveCommandLineDialogs.insert(fullCommandLine);
        QMetaObject::invokeMethod(this, "askForCommandLinePermission", Qt::QueuedConnection, Q_ARG(QString, fullCommandLine));
    }

    // For now, return true and let the async dialog handle the blocking
    return true;
}

void ACPClientPlugin::askForCommandLinePermission(const QString &fullCommandLineString)
{
    m_currentActiveCommandLineDialogs.erase(fullCommandLineString);

    // Ask user for permission
    QMessageBox::StandardButton result =
        QMessageBox::question(nullptr,
                              i18n("Allow ACP Server Execution?"),
                              i18n("The ACP client plugin wants to execute:\n\n%1\n\nAllow this command to be executed?", fullCommandLineString),
                              QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                              QMessageBox::Yes);

    bool allowed = (result == QMessageBox::Yes);
    m_serverCommandLineToAllowedState[fullCommandLineString] = allowed;

    // Write config to persist the decision
    writeConfig();

    // Restart servers if needed
    if (m_serverManager) {
        m_serverManager->startAutoStartServers();
    }
}

#include "acpclientplugin.moc"
