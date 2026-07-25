/*
    SPDX-FileCopyrightText: 2026 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: MIT
*/

#include "acpclientpluginview.h"
#include "acpclient_debug.h"
#include "acpclientchatwidget.h"
#include "acpclientplugin.h"
#include "acpclientservermanager.h"
#include "acpsessionlistwidget.h"

#include <KActionCollection>
#include <KLocalizedString>
#include <KTextEditor/Document>
#include <KTextEditor/View>
#include <KXMLGUIFactory>

#include <QAction>
#include <QIcon>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QMessageBox>
#include <QTabWidget>

ACPClientPluginView::ACPClientPluginView(ACPClientPlugin *plugin,
                                         KTextEditor::MainWindow *mainWindow,
                                         const std::shared_ptr<ACPClientServerManager> &serverManager)
    : QObject(mainWindow)
    , m_plugin(plugin)
    , m_mainWindow(mainWindow)
    , m_serverManager(serverManager)
    , m_toolWidget(nullptr)
{
    qCDebug(ACPCLIENT) << "ACPClientPluginView created";

    setComponentName(QStringLiteral("kateacpclient"), i18n("Agent Client Protocol"));
    setXMLFile(QStringLiteral("ui.rc"));

    setupActions();
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

    // The tool view is managed by Kate, just clear the pointer
    m_chatToolView = nullptr;

    m_mainWindow->guiFactory()->removeClient(this);
}

void ACPClientPluginView::setupActions()
{
    // New session action
    m_newSessionAction = new QAction(i18n("New ACP Session"), this);
    m_newSessionAction->setIcon(QIcon::fromTheme(QStringLiteral("dialog-information")));
    m_newSessionAction->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_A);
    connect(m_newSessionAction, &QAction::triggered, this, &ACPClientPluginView::onNewSession);
    actionCollection()->addAction(QStringLiteral("acp_new_session"), m_newSessionAction);

    // Send prompt action
    m_sendPromptAction = new QAction(i18n("Send Prompt to ACP Agent"), this);
    m_sendPromptAction->setIcon(QIcon::fromTheme(QStringLiteral("mail-send")));
    m_sendPromptAction->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_S);
    connect(m_sendPromptAction, &QAction::triggered, this, &ACPClientPluginView::onSendPrompt);
    actionCollection()->addAction(QStringLiteral("acp_send_prompt"), m_sendPromptAction);

    // List sessions action
    m_listSessionsAction = new QAction(i18n("List ACP Sessions"), this);
    m_listSessionsAction->setIcon(QIcon::fromTheme(QStringLiteral("view-list")));
    connect(m_listSessionsAction, &QAction::triggered, this, &ACPClientPluginView::onListSessions);
    actionCollection()->addAction(QStringLiteral("acp_list_sessions"), m_listSessionsAction);

    // Manage servers action
    m_manageServersAction = new QAction(i18n("Manage ACP Servers"), this);
    m_manageServersAction->setIcon(QIcon::fromTheme(QStringLiteral("preferences-system")));
    connect(m_manageServersAction, &QAction::triggered, this, &ACPClientPluginView::onManageServers);
    actionCollection()->addAction(QStringLiteral("acp_manage_servers"), m_manageServersAction);

    // Show tools action
    m_showToolsAction = new QAction(i18n("Show ACP Tools"), this);
    m_showToolsAction->setIcon(QIcon::fromTheme(QStringLiteral("utilities-terminal")));
    connect(m_showToolsAction, &QAction::triggered, this, &ACPClientPluginView::onShowTools);
    actionCollection()->addAction(QStringLiteral("acp_show_tools"), m_showToolsAction);

    // Show chat action
    m_showChatAction = new QAction(i18n("Show ACP Chat"), this);
    m_showChatAction->setIcon(QIcon::fromTheme(QStringLiteral("internet-services")));
    connect(m_showChatAction, &QAction::triggered, this, &ACPClientPluginView::onShowChat);
    actionCollection()->addAction(QStringLiteral("acp_show_chat"), m_showChatAction);
}

void ACPClientPluginView::setupUI()
{
    showChatToolView();
}

void ACPClientPluginView::createSessionFromDocument()
{
    KTextEditor::View *view = m_mainWindow->activeView();
    if (!view) {
        Q_EMIT m_plugin->showMessage(KTextEditor::Message::Warning, i18n("No active document"));
        return;
    }

    KTextEditor::Document *doc = view->document();
    QString content = doc->text();

    if (content.isEmpty()) {
        Q_EMIT m_plugin->showMessage(KTextEditor::Message::Warning, i18n("Document is empty"));
        return;
    }

    Q_EMIT sessionRequested(content);
}

void ACPClientPluginView::sendSelectionAsPrompt()
{
    KTextEditor::View *view = m_mainWindow->activeView();
    if (!view) {
        Q_EMIT m_plugin->showMessage(KTextEditor::Message::Warning, i18n("No active document"));
        return;
    }

    if (!view->selection()) {
        Q_EMIT m_plugin->showMessage(KTextEditor::Message::Warning, i18n("No text selected"));
        return;
    }

    QString selectedText = view->selectionText();
    if (selectedText.isEmpty()) {
        Q_EMIT m_plugin->showMessage(KTextEditor::Message::Warning, i18n("Selection is empty"));
        return;
    }

    Q_EMIT sessionRequested(selectedText);
}

void ACPClientPluginView::showSessionManager()
{
    // TODO: Implement session manager dialog
    m_serverManager->listSessions();
}

void ACPClientPluginView::showServerConfig()
{
    // TODO: Implement server configuration dialog
}

void ACPClientPluginView::showToolPalette()
{
    // TODO: Implement tool palette
    m_serverManager->listTools();
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

    // Create a tab widget to hold chat and session list
    m_tabWidget = new QTabWidget(m_chatToolView);
    m_tabWidget->setObjectName(QStringLiteral("ACPClientTabWidget"));

    // Create the chat widget
    m_chatWidget = new ACPClientChatWidget(m_plugin, m_mainWindow, m_tabWidget);
    m_chatWidget->setObjectName(QStringLiteral("ACPChatWidget"));
    m_tabWidget->addTab(m_chatWidget, i18n("Chat"));

    // Create the session list widget
    m_sessionListWidget = new ACPSessionListWidget(m_serverManager.get(), m_tabWidget);
    m_sessionListWidget->setObjectName(QStringLiteral("ACPSessionListWidget"));
    m_tabWidget->addTab(m_sessionListWidget, i18n("Sessions"));

    // Connect session resumed signal to resume the session
    connect(m_sessionListWidget, &ACPSessionListWidget::sessionResumed, this, [this](const QString &sessionId) {
        m_chatWidget->setSessionId(sessionId);
        m_serverManager->resumeSession(sessionId);
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

void ACPClientPluginView::onNewSession()
{
    qCDebug(ACPCLIENT) << "New session requested";

    // Check if we have an active server
    ACPClientServer *server = m_serverManager->activeServer();
    if (!server) {
        QMessageBox::information(nullptr, i18n("No ACP Server"), i18n("No ACP agent server is connected. Please configure and connect to an agent first."));
        return;
    }

    // Create a new session
    QString sessionId = m_serverManager->createSession();
    if (sessionId.isEmpty()) {
        QMessageBox::warning(nullptr, i18n("Session Error"), i18n("Failed to create new session."));
        return;
    }

    Q_EMIT m_plugin->showMessage(KTextEditor::Message::Information, i18n("New ACP session created: %1", sessionId));
}

void ACPClientPluginView::onSendPrompt()
{
    qCDebug(ACPCLIENT) << "Send prompt requested";

    KTextEditor::View *view = m_mainWindow->activeView();
    if (!view) {
        QMessageBox::warning(nullptr, i18n("No Active Document"), i18n("No active document to send as prompt."));
        return;
    }

    ACPClientServer *server = m_serverManager->activeServer();
    if (!server) {
        QMessageBox::information(nullptr, i18n("No ACP Server"), i18n("No ACP agent server is connected. Please configure and connect to an agent first."));
        return;
    }

    // If there's a selection, use it; otherwise use the whole document
    QString prompt;
    if (view->selection()) {
        prompt = view->selectionText();
    } else {
        prompt = view->document()->text();
    }

    if (prompt.isEmpty()) {
        QMessageBox::warning(nullptr, i18n("Empty Prompt"), i18n("No text to send as prompt."));
        return;
    }

    // For now, create a session and send the prompt
    QString sessionId = m_serverManager->createSession();
    if (!sessionId.isEmpty()) {
        m_serverManager->sendPrompt(sessionId, prompt);
        Q_EMIT m_plugin->showMessage(KTextEditor::Message::Information, i18n("Prompt sent to ACP agent"));
    }
}

void ACPClientPluginView::onListSessions()
{
    qCDebug(ACPCLIENT) << "List sessions requested";
    showSessionManager();
}

void ACPClientPluginView::onManageServers()
{
    qCDebug(ACPCLIENT) << "Manage servers requested";
    showServerConfig();
}

void ACPClientPluginView::onShowTools()
{
    qCDebug(ACPCLIENT) << "Show tools requested";
    showToolPalette();
}

void ACPClientPluginView::onShowChat()
{
    qCDebug(ACPCLIENT) << "Show chat requested";
    showChatToolView();
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
    // Update action states
}

void ACPClientPluginView::onServerDisconnected()
{
    qCDebug(ACPCLIENT) << "Server disconnected";
    // Update action states
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
