/*
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>
    SPDX-FileCopyrightText: 2026 Christoph Cullmann <cullmann@kde.org>

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

QString acpClientConfigGroup()
{
    return QStringLiteral("acpclient");
}

static constexpr char CONFIG_SERVER_CONFIG[] = "ServerConfiguration";
static constexpr char CONFIG_TOOL_CALL_PERMISSION[] = "ToolCallPermission";
static constexpr char CONFIG_EXPAND_THOUGHTS[] = "ExpandThoughts";
static constexpr char CONFIG_LOG_MESSAGES[] = "LogMessages";
static constexpr char CONFIG_PRETTY_PRINT_LOG[] = "PrettyPrintLog";

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
    , m_serverManager(this, this)
{
    qCDebug(ACPCLIENT) << "ACPClientPlugin created";

    // Ensure settings path exists
    QDir().mkpath(m_settingsPath);

    // Ensure we don't spam the user with debug messages per default
    if (!oldCategoryFilter) {
        oldCategoryFilter = QLoggingCategory::installFilter(myCategoryFilter);
    }

    // handle errors from the servers
    connect(&m_serverManager, &ACPClientServerManager::errorOccurred, this, [this](const QString &error) {
        showMessage(Error, error);
    });

    // Apply our config, will trigger server manager settings loading, too
    readConfig();
}

ACPClientPlugin::~ACPClientPlugin() = default;

QObject *ACPClientPlugin::createView(KTextEditor::MainWindow *mainWindow)
{
    return new ACPClientPluginView(this, mainWindow, &m_serverManager);
}

void ACPClientPlugin::showMessage(MessageType level, const QString &msg)
{
    const auto icon = QIcon::fromTheme(QStringLiteral("internet-services"));
    Utils::showMessage(msg, icon, i18n("Keyboard Macros"), level);
}

int ACPClientPlugin::configPages() const
{
    return 1; // We have one config page
}

KTextEditor::ConfigPage *ACPClientPlugin::configPage(int number, QWidget *parent)
{
    if (number == 0) {
        return new ACPClientConfigPage(this, parent);
    }

    return nullptr;
}

void ACPClientPlugin::writeConfig() const
{
    KConfigGroup config(KSharedConfig::openConfig(), acpClientConfigGroup());
    config.writeEntry(CONFIG_SERVER_CONFIG, m_configPath);
    config.writeEntry(CONFIG_TOOL_CALL_PERMISSION, static_cast<int>(m_toolCallPermission));
    config.writeEntry(CONFIG_EXPAND_THOUGHTS, m_expandThoughts);
    config.writeEntry(CONFIG_LOG_MESSAGES, m_logMessages);
    config.writeEntry(CONFIG_PRETTY_PRINT_LOG, m_prettyPrintLog);

    // trigger that we react on config changes in the server manager and Co.
    Q_EMIT update();
}

void ACPClientPlugin::readConfig()
{
    KConfigGroup config(KSharedConfig::openConfig(), acpClientConfigGroup());
    m_configPath = config.readEntry(CONFIG_SERVER_CONFIG, QUrl());
    m_toolCallPermission = static_cast<ToolCallPermission>(config.readEntry(CONFIG_TOOL_CALL_PERMISSION, static_cast<int>(AskEachTime)));
    m_expandThoughts = config.readEntry(CONFIG_EXPAND_THOUGHTS, false);
    m_logMessages = config.readEntry(CONFIG_LOG_MESSAGES, false);
    m_prettyPrintLog = config.readEntry(CONFIG_PRETTY_PRINT_LOG, false);

    // trigger that we react on config changes in the server manager and Co.
    Q_EMIT update();
}

void ACPClientPlugin::setExpandThoughts(bool expand)
{
    if (m_expandThoughts == expand) {
        return;
    }

    m_expandThoughts = expand;

    // Stores the option and notifies the views through update()
    writeConfig();
}

#include "acpclientplugin.moc"
