/*
    SPDX-FileCopyrightText: 2026

    SPDX-License-Identifier: MIT
*/

#pragma once

#include <QObject>
#include <QWidget>

#include <KTextEditor/MainWindow>
#include <KXMLGUIClient>

#include <memory>

class ACPClientPlugin;
class ACPClientServerManager;
class QAction;
class QToolBar;
class QMenu;

class ACPClientPluginView : public QObject, public KXMLGUIClient
{
    Q_OBJECT

public:
    explicit ACPClientPluginView(ACPClientPlugin *plugin, KTextEditor::MainWindow *mainWindow, const std::shared_ptr<ACPClientServerManager> &serverManager);
    ~ACPClientPluginView() override;

    // Create a session from the current document context
    void createSessionFromDocument();

    // Send the current selection as a prompt
    void sendSelectionAsPrompt();

    // Show session management dialog
    void showSessionManager();

    // Show server configuration dialog
    void showServerConfig();

    // Show tool palette
    void showToolPalette();

Q_SIGNALS:
    void sessionRequested(const QString &prompt);
    void toolCallRequested(const QString &toolId, const QJsonObject &arguments);

private Q_SLOTS:
    void onNewSession();
    void onSendPrompt();
    void onListSessions();
    void onManageServers();
    void onShowTools();
    void onServerConnected();
    void onServerDisconnected();
    void onMessageReceived(const QJsonDocument &message);

private:
    void setupActions();
    void setupUI();

    ACPClientPlugin *m_plugin;
    KTextEditor::MainWindow *m_mainWindow;
    std::shared_ptr<ACPClientServerManager> m_serverManager;

    // Actions
    QAction *m_newSessionAction;
    QAction *m_sendPromptAction;
    QAction *m_listSessionsAction;
    QAction *m_manageServersAction;
    QAction *m_showToolsAction;

    // UI elements
    QWidget *m_toolWidget;
};