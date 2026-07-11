/*
    SPDX-FileCopyrightText: 2026

    SPDX-License-Identifier: MIT
*/

#include "acpclientpluginview.h"
#include "acpclient_debug.h"
#include "acpclientplugin.h"
#include "acpclientservermanager.h"

#include <KActionCollection>
#include <KLocalizedString>
#include <KTextEditor/Document>
#include <KTextEditor/View>
#include <KXMLGUIFactory>

#include <QAction>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QMessageBox>

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
}

void ACPClientPluginView::setupUI()
{
    // For now, actions are added to the menu system via XML
    // We'll add them to a menu in the future
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

    Q_EMIT m_plugin->showMessage(KTextEditor::Message::Info, i18n("New ACP session created: %1", sessionId));
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
        Q_EMIT m_plugin->showMessage(KTextEditor::Message::Info, i18n("Prompt sent to ACP agent"));
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
    qCDebug(ACPCLIENT) << "Message received in view:" << message.toJson();

    // Parse and display messages as needed
    if (message.isObject()) {
        QJsonObject obj = message.object();
        if (obj.contains("method")) {
            QString method = obj["method"].toString();

            if (method == ACP::NOTIFICATION_SESSION_UPDATE) {
                // Handle session update
                if (obj.contains("params")) {
                    QJsonObject params = obj["params"].toObject();
                    if (params.contains("message")) {
                        QString msg = params["message"].toString();
                        if (!msg.isEmpty()) {
                            Q_EMIT m_plugin->showMessage(KTextEditor::Message::Info, i18n("ACP: %1", msg));
                        }
                    }
                }
            }
        }
    }
}
