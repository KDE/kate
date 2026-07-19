/*
    SPDX-FileCopyrightText: 2026

    SPDX-License-Identifier: MIT
*/

#pragma once

#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QWidget>

#include <KTextEditor/MainWindow>

class ACPClientPlugin;
class ACPClientServer;
class ACPClientServerManager;

namespace Ui
{
class ACPChatWidget;
}

class ACPClientChatWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ACPClientChatWidget(ACPClientPlugin *plugin, KTextEditor::MainWindow *mainWindow, QWidget *parent = nullptr);
    ~ACPClientChatWidget() override;

    // Start a new chat session
    void startNewSession();

    // Set the active session ID
    void setSessionId(const QString &sessionId);

    // Get the current session ID
    QString sessionId() const;

    // Append a message to the chat
    void appendMessage(const QString &sender, const QString &message, bool isUser = false);

    // Clear the chat
    void clearChat();

    // Set the server
    void setServer(ACPClientServer *server);

Q_SIGNALS:
    void messageSent(const QString &sessionId, const QString &message);
    void sessionRequested();
    void sessionEnded();

    // Permission response signal
    void permissionResponse(qint64 requestId, const QString &optionId);

private Q_SLOTS:
    void sendMessage();
    void onInputReturnPressed();
    void onServerMessageReceived(const QJsonDocument &message);
    void onPermissionRequested(qint64 requestId, const QJsonObject &toolCall, const QJsonArray &options);

private:
    void setupUI();
    void loadHistory();
    void saveHistory();
    QString formatMessage(const QString &sender, const QString &message, bool isUser) const;
    void updateSessionState();
    void appendHtml(const QString &html);

    // Message handlers
    void handleAgentMessageChunk(const QJsonObject &update);
    void handlePlanUpdate(const QJsonObject &update);
    void handleToolCallUpdate(const QJsonObject &update);
    void handleToolCallStatusUpdate(const QJsonObject &update);
    void handleUsageUpdate(const QJsonObject &update);
    QString formatAgentTextMessage(const QString &text, const QString &messageId);

    ACPClientPlugin *m_plugin;
    KTextEditor::MainWindow *m_mainWindow;
    ACPClientServerManager *m_serverManager = nullptr;
    ACPClientServer *m_server = nullptr;
    QString m_sessionId;

    Ui::ACPChatWidget *m_ui;
    QList<QString> m_messageHistory;
    int m_historyIndex = 0;
};