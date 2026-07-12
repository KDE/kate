/*
    SPDX-FileCopyrightText: 2026

    SPDX-License-Identifier: MIT
*/

#include "acpclientconfigpage.h"
#include "acpclient_debug.h"
#include "acpclientplugin.h"
#include "acpclientserver.h"
#include "acpclientservermanager.h"
#include "acpserverdialog.h"

#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>

#include <QDir>
#include <QFile>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QVBoxLayout>

// UI includes
#include "ui_acpconfigwidget.h"

ACPClientConfigPage::ACPClientConfigPage(ACPClientPlugin *plugin, QWidget *parent)
    : KTextEditor::ConfigPage(parent)
    , m_plugin(plugin)
    , m_ui(new Ui::ACPConfigWidget)
{
    qCDebug(ACPCLIENT) << "ACPClientConfigPage created";

    QWidget *widget = new QWidget(this);
    m_ui->setupUi(widget);

    // Load configuration
    loadConfig();

    // Connect signals
    connect(m_ui->addServerButton, &QPushButton::clicked, this, &ACPClientConfigPage::addServer);
    connect(m_ui->editServerButton, &QPushButton::clicked, this, &ACPClientConfigPage::editServer);
    connect(m_ui->removeServerButton, &QPushButton::clicked, this, &ACPClientConfigPage::removeServer);
    connect(m_ui->serverListWidget, &QListWidget::itemSelectionChanged, this, &ACPClientConfigPage::serverSelected);

    // Update server list display
    updateServerList();

    // Set the widget
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);
}

ACPClientConfigPage::~ACPClientConfigPage()
{
    delete m_ui;
}

QString ACPClientConfigPage::name() const
{
    return i18n("ACP Client");
}

QString ACPClientConfigPage::fullName() const
{
    return i18n("Agent Client Protocol Client Configuration");
}

QIcon ACPClientConfigPage::icon() const
{
    return QIcon::fromTheme(QStringLiteral("utilities-terminal"));
}

void ACPClientConfigPage::apply()
{
    qCDebug(ACPCLIENT) << "Applying ACP client configuration";
    saveConfig();
}

void ACPClientConfigPage::defaults()
{
    qCDebug(ACPCLIENT) << "Resetting ACP client configuration to defaults";

    // Reset UI to defaults
    m_ui->autoStartCheckBox->setChecked(false);
    m_ui->showNotificationsCheckBox->setChecked(true);
    m_ui->showToolCallsCheckBox->setChecked(true);
    m_ui->showProgressCheckBox->setChecked(true);
    m_ui->debugModeCheckBox->setChecked(false);

    // Reset servers to default
    m_servers.clear();

    // Load default configuration
    ACPClientServer::ServerInfo vibeServer;
    vibeServer.name = QStringLiteral("Mistral Vibe (vibe-acp)");
    vibeServer.version = QStringLiteral("1.0");
    vibeServer.command = QStringLiteral("vibe-acp");
    vibeServer.autoStart = true;
    m_servers.append(vibeServer);

    updateServerList();
}

void ACPClientConfigPage::reset()
{
    qCDebug(ACPCLIENT) << "Resetting ACP client configuration";
    loadConfig();
}

void ACPClientConfigPage::loadConfig()
{
    qCDebug(ACPCLIENT) << "Loading ACP client configuration";

    // Load plugin options
    m_ui->autoStartCheckBox->setChecked(m_plugin->m_autoStartSession);
    m_ui->showNotificationsCheckBox->setChecked(m_plugin->m_showNotifications);
    m_ui->showToolCallsCheckBox->setChecked(m_plugin->m_showToolCalls);
    m_ui->showProgressCheckBox->setChecked(m_plugin->m_showProgress);
    m_ui->debugModeCheckBox->setChecked(m_plugin->m_debugMode);

    // Load servers from server manager if available
    if (m_plugin->m_serverManager) {
        for (ACPClientServer *server : m_plugin->m_serverManager->servers()) {
            m_servers.append(server->info());
        }
    }

    // If no servers loaded, try to load from default config
    if (m_servers.isEmpty()) {
        loadDefaultServers();
    }

    updateServerList();
}

void ACPClientConfigPage::saveConfig()
{
    qCDebug(ACPCLIENT) << "Saving ACP client configuration";

    // Save plugin options
    m_plugin->m_autoStartSession = m_ui->autoStartCheckBox->isChecked();
    m_plugin->m_showNotifications = m_ui->showNotificationsCheckBox->isChecked();
    m_plugin->m_showToolCalls = m_ui->showToolCallsCheckBox->isChecked();
    m_plugin->m_showProgress = m_ui->showProgressCheckBox->isChecked();
    m_plugin->m_debugMode = m_ui->debugModeCheckBox->isChecked();

    // Save to plugin config
    m_plugin->writeConfig();

    // Save servers configuration
    saveServersConfig();

    // Apply changes to server manager
    if (m_plugin->m_serverManager) {
        // Clear existing servers
        for (ACPClientServer *server : m_plugin->m_serverManager->servers()) {
            m_plugin->m_serverManager->removeServer(server);
        }

        // Add configured servers
        for (const ACPClientServer::ServerInfo &info : m_servers) {
            m_plugin->m_serverManager->createServer(info);
        }
    }
}

void ACPClientConfigPage::loadDefaultServers()
{
    qCDebug(ACPCLIENT) << "Loading default ACP servers";

    // Load from shipped settings.json
    QString settingsPath = QStringLiteral(":/kateacpclient/settings.json");
    QFile settingsFile(settingsPath);

    if (settingsFile.exists() && settingsFile.open(QIODevice::ReadOnly)) {
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(settingsFile.readAll(), &parseError);

        if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
            QJsonObject obj = doc.object();
            if (obj.contains(u"servers") && obj[u"servers"].isArray()) {
                QJsonArray servers = obj[u"servers"].toArray();
                for (const QJsonValue &v : servers) {
                    if (v.isObject()) {
                        QJsonObject serverObj = v.toObject();
                        ACPClientServer::ServerInfo info;

                        info.name = serverObj[u"name"].toString();
                        info.version = serverObj[u"version"].toString();
                        info.command = serverObj[u"command"].toString();

                        if (serverObj.contains(u"arguments") && serverObj[u"arguments"].isArray()) {
                            QJsonArray args = serverObj[u"arguments"].toArray();
                            for (const QJsonValue &arg : args) {
                                info.arguments.append(arg.toString());
                            }
                        }

                        info.autoStart = serverObj[u"auto_start"].toBool();

                        if (serverObj.contains(u"metadata") && serverObj[u"metadata"].isObject()) {
                            info.metadata = serverObj[u"metadata"].toObject();
                        }

                        m_servers.append(info);
                    }
                }
            }
        }
        settingsFile.close();
    }

    // If still no servers, add vibe-acp as default
    if (m_servers.isEmpty()) {
        ACPClientServer::ServerInfo vibeServer;
        vibeServer.name = QStringLiteral("Mistral Vibe (vibe-acp)");
        vibeServer.version = QStringLiteral("1.0");
        vibeServer.command = QStringLiteral("vibe-acp");
        vibeServer.autoStart = true;
        m_servers.append(vibeServer);
    }
}

void ACPClientConfigPage::saveServersConfig()
{
    qCDebug(ACPCLIENT) << "Saving servers configuration";

    // Save to plugin's settings path
    QString settingsPath = m_plugin->m_settingsPath + QStringLiteral("/settings.json");
    QDir().mkpath(m_plugin->m_settingsPath);

    QJsonObject settingsObj;
    settingsObj[u"version"] = QStringLiteral("1.0");
    settingsObj[u"acp_protocol_version"] = QStringLiteral("2.0");

    // Save servers
    QJsonArray serversArray;
    for (const ACPClientServer::ServerInfo &info : m_servers) {
        QJsonObject serverObj;
        serverObj[u"name"] = info.name;
        serverObj[u"version"] = info.version;
        serverObj[u"command"] = info.command;
        serverObj[u"arguments"] = QJsonArray::fromStringList(info.arguments);
        serverObj[u"auto_start"] = info.autoStart;
        serverObj[u"metadata"] = info.metadata;

        serversArray.append(serverObj);
    }
    settingsObj[u"servers"] = serversArray;

    // Save options
    QJsonObject optionsObj;
    optionsObj[u"auto_start_session"] = m_ui->autoStartCheckBox->isChecked();
    optionsObj[u"show_notifications"] = m_ui->showNotificationsCheckBox->isChecked();
    optionsObj[u"show_tool_calls"] = m_ui->showToolCallsCheckBox->isChecked();
    optionsObj[u"show_progress"] = m_ui->showProgressCheckBox->isChecked();
    optionsObj[u"debug_mode"] = m_ui->debugModeCheckBox->isChecked();
    settingsObj[u"options"] = optionsObj;

    // Write to file
    QFile outFile(settingsPath);
    if (outFile.open(QIODevice::WriteOnly)) {
        outFile.write(QJsonDocument(settingsObj).toJson());
        outFile.close();
    }
}

void ACPClientConfigPage::addServer()
{
    qCDebug(ACPCLIENT) << "Adding new server";

    ACPClientServer::ServerInfo info;
    info.name = i18n("New Server");
    info.command = QStringLiteral("acp-agent");
    info.autoStart = false;

    ACPServerDialog dialog(this, info);
    if (dialog.exec() == QDialog::Accepted) {
        ACPClientServer::ServerInfo newInfo = dialog.serverInfo();
        m_servers.append(newInfo);
        updateServerList();

        // Select the new server
        m_ui->serverListWidget->setCurrentRow(m_ui->serverListWidget->count() - 1);
    }
}

void ACPClientConfigPage::removeServer()
{
    qCDebug(ACPCLIENT) << "Removing server";

    int row = m_ui->serverListWidget->currentRow();
    if (row >= 0 && row < m_servers.size()) {
        if (QMessageBox::question(this, i18n("Remove Server"), i18n("Are you sure you want to remove server '%1'?", m_servers[row].name)) == QMessageBox::Yes) {
            m_servers.removeAt(row);
            updateServerList();
        }
    }
}

void ACPClientConfigPage::editServer()
{
    qCDebug(ACPCLIENT) << "Editing server";

    int row = m_ui->serverListWidget->currentRow();
    if (row >= 0 && row < m_servers.size()) {
        ACPClientServer::ServerInfo info = m_servers[row];
        ACPServerDialog dialog(this, info);
        if (dialog.exec() == QDialog::Accepted) {
            m_servers[row] = dialog.serverInfo();
            updateServerList();
            m_ui->serverListWidget->setCurrentRow(row);
        }
    }
}

void ACPClientConfigPage::serverSelected()
{
    qCDebug(ACPCLIENT) << "Server selected";

    int row = m_ui->serverListWidget->currentRow();
    m_ui->editServerButton->setEnabled(row >= 0 && row < m_servers.size());
    m_ui->removeServerButton->setEnabled(row >= 0 && row < m_servers.size());

    if (row >= 0 && row < m_servers.size()) {
        const ACPClientServer::ServerInfo &info = m_servers[row];
        m_ui->serverNameLabel->setText(info.name);
        m_ui->serverCommandLabel->setText(info.command);

        m_ui->serverTypeLabel->setText(i18n("Standard I/O"));
        m_ui->serverAutoStartLabel->setText(info.autoStart ? i18n("Yes") : i18n("No"));
    } else {
        m_ui->serverNameLabel->setText(i18n("(none selected)"));
        m_ui->serverCommandLabel->setText(QStringLiteral("-"));
        m_ui->serverTypeLabel->setText(QStringLiteral("-"));
        m_ui->serverAutoStartLabel->setText(QStringLiteral("-"));
    }
}

void ACPClientConfigPage::updateServerList()
{
    qCDebug(ACPCLIENT) << "Updating server list";

    m_ui->serverListWidget->clear();

    for (const ACPClientServer::ServerInfo &info : m_servers) {
        QStringList parts;
        parts.append(info.name);
        parts.append(info.command);

        if (info.autoStart) {
            parts.append(i18n("(auto)"));
        }

        m_ui->serverListWidget->addItem(parts.join(QStringLiteral(" - ")));
    }

    // Update buttons
    serverSelected();
}
