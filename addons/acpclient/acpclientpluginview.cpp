/*
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>
    SPDX-FileCopyrightText: 2026 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: MIT
*/

#include "acpclientpluginview.h"
#include "acpclient_debug.h"
#include "acpclientchatwidget.h"
#include "acpclientplugin.h"
#include "acpclientservermanager.h"
#include "acpserverlistwidget.h"
#include "acpsessionlistwidget.h"

#include <KLocalizedString>
#include <KXMLGUIFactory>

#include <QIcon>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTabWidget>

ACPClientPluginView::ACPClientPluginView(ACPClientPlugin *plugin,
                                         KTextEditor::MainWindow *mainWindow,
                                         const std::shared_ptr<ACPClientServerManager> &serverManager)
    : QObject(mainWindow)
    , m_plugin(plugin)
    , m_mainWindow(mainWindow)
    , m_serverManager(serverManager)
{
    qCDebug(ACPCLIENT) << "ACPClientPluginView created";

    setComponentName(QStringLiteral("kateacpclient"), i18n("Agent Client Protocol"));
    setXMLFile(QStringLiteral("ui.rc"));

    setupUI();

    // Connect to server manager signals
    connect(m_serverManager.get(), &ACPClientServerManager::messageReceived, this, &ACPClientPluginView::onMessageReceived);
    connect(m_serverManager.get(), &ACPClientServerManager::errorOccurred, this, [this](const QString &error) {
        Q_EMIT m_plugin->showMessage(KTextEditor::Message::Error, error);
    });

    m_mainWindow->guiFactory()->addClient(this);
}

ACPClientPluginView::~ACPClientPluginView()
{
    qCDebug(ACPCLIENT) << "ACPClientPluginView destroyed";

    // Clean up chat tool view
    delete m_chatWidget;
    m_chatWidget = nullptr;
    delete m_sessionListWidget;
    m_sessionListWidget = nullptr;
    delete m_serverListWidget;
    m_serverListWidget = nullptr;

    // The tool view is managed by Kate, just clear the pointer
    m_chatToolView = nullptr;

    m_mainWindow->guiFactory()->removeClient(this);
}

void ACPClientPluginView::setupUI()
{
    showChatToolView();

    // Query session list once UI is set up
    if (m_serverManager->activeServer()) {
        m_serverManager->listSessions();
    }
}

void ACPClientPluginView::showChatToolView()
{
    qCDebug(ACPCLIENT) << "Show chat tool view requested";

    // Create the tool view using Kate's createToolView
    m_chatToolView = m_mainWindow->createToolView(m_plugin,
                                                  QStringLiteral("kate_private_plugin_acpclient_chat"),
                                                  KTextEditor::MainWindow::Right,
                                                  QIcon::fromTheme(QStringLiteral("internet-services")),
                                                  i18n("ACP Chat"));

    // Create a tab widget to hold servers, sessions and chat
    m_tabWidget = new QTabWidget(m_chatToolView);
    m_tabWidget->setObjectName(QStringLiteral("ACPClientTabWidget"));

    // Create the server list widget
    m_serverListWidget = new ACPServerListWidget(m_serverManager.get(), m_tabWidget);
    m_serverListWidget->setObjectName(QStringLiteral("ACPServerListWidget"));
    m_serversTabIndex = m_tabWidget->addTab(m_serverListWidget, i18n("Servers"));

    // Create the session list widget
    m_sessionListWidget = new ACPSessionListWidget(m_serverManager.get(), m_tabWidget);
    m_sessionListWidget->setObjectName(QStringLiteral("ACPSessionListWidget"));
    m_sessionsTabIndex = m_tabWidget->addTab(m_sessionListWidget, i18n("Sessions"));

    // Create the chat widget
    m_chatWidget = new ACPClientChatWidget(m_plugin, m_mainWindow, m_tabWidget);
    m_chatWidget->setObjectName(QStringLiteral("ACPChatWidget"));
    m_chatTabIndex = m_tabWidget->addTab(m_chatWidget, i18n("Chat"));

    // Connect server activated signal to set active server
    connect(m_serverListWidget, &ACPServerListWidget::serverActivated, this, [this](const QString &serverName) {
        m_serverManager->setActiveServer(serverName);
    });

    // Disable Sessions and Chat tabs until a server is available
    m_tabWidget->setTabEnabled(m_sessionsTabIndex, false);
    m_tabWidget->setTabEnabled(m_chatTabIndex, false);

    // Update tab enabled state when active server changes
    updateTabEnabledState();
    connect(m_serverManager.get(), &ACPClientServerManager::activeServerChanged, this, &ACPClientPluginView::updateTabEnabledState);

    // Connect session activated signal to load/resume the session
    connect(m_sessionListWidget, &ACPSessionListWidget::sessionResumed, this, [this](const QString &sessionId) {
        if (sessionId.isEmpty()) {
            qCWarning(ACPCLIENT) << "Cannot load session: empty session ID";
            return;
        }

        qCDebug(ACPCLIENT) << "Loading session from double-click:" << sessionId;

        // Initialize the chat widget with the session (sets up server connections)
        if (m_chatWidget) {
            m_chatWidget->initializeWithSession(sessionId);
        }

        // Check agent capabilities to determine which method to use
        if (m_serverManager->supportsLoadSession()) {
            qCDebug(ACPCLIENT) << "Using session/load (agent supports loadSession)";
            m_serverManager->loadSession(sessionId);
        } else if (m_serverManager->supportsResumeSession()) {
            qCDebug(ACPCLIENT) << "Using session/resume (agent supports resume)";
            m_serverManager->resumeSession(sessionId);
        } else {
            qCWarning(ACPCLIENT) << "Agent does not support session/load or session/resume";
        }

        // Switch to the chat tab
        if (m_tabWidget && m_chatToolView) {
            m_tabWidget->setCurrentIndex(m_chatTabIndex);
        }
    });

    // Connect to server manager for session list updates
    connect(m_serverManager.get(), &ACPClientServerManager::sessionListReceived, this, &ACPClientPluginView::onSessionListReceived);

    // Connect to active server changed to auto-query sessions when server becomes available
    connect(m_serverManager.get(), &ACPClientServerManager::activeServerChanged, this, [this](ACPClientServer *server) {
        if (server) {
            // Query session list when an active server is set
            m_serverManager->listSessions();
        }
    });
}

void ACPClientPluginView::onSessionListReceived(const QJsonArray &sessions)
{
    qCDebug(ACPCLIENT) << "Session list received with" << sessions.size() << "sessions";

    if (m_sessionListWidget) {
        m_sessionListWidget->updateSessionList(sessions);
    }
}

void ACPClientPluginView::onServerConnected()
{
    qCDebug(ACPCLIENT) << "Server connected";
}

void ACPClientPluginView::onServerDisconnected()
{
    qCDebug(ACPCLIENT) << "Server disconnected";
    updateTabEnabledState();
}

void ACPClientPluginView::updateTabEnabledState()
{
    // Enable Sessions and Chat tabs only if there's an active server
    bool hasActiveServer = m_serverManager && m_serverManager->activeServer();

    if (m_tabWidget) {
        if (m_sessionsTabIndex >= 0) {
            m_tabWidget->setTabEnabled(m_sessionsTabIndex, hasActiveServer);
        }
        if (m_chatTabIndex >= 0) {
            m_tabWidget->setTabEnabled(m_chatTabIndex, hasActiveServer);
        }
    }
}

void ACPClientPluginView::onMessageReceived(const QJsonDocument &message)
{
    // Parse and display messages as needed
    if (message.isObject()) {
        QJsonObject obj = message.object();
        if (obj.contains(u"method")) {
            QString method = obj[u"method"].toString();

            if (method == ACP::NOTIFICATION_SESSION_UPDATE) {
                // Handle session update
                if (obj.contains(u"params")) {
                    QJsonObject params = obj[u"params"].toObject();
                    if (params.contains(u"message")) {
                        QString msg = params[u"message"].toString();
                        if (!msg.isEmpty()) {
                            Q_EMIT m_plugin->showMessage(KTextEditor::Message::Information, i18n("ACP: %1", msg));
                        }
                    }
                }
            }
        }
    }
}
