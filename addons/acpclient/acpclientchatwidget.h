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

private Q_SLOTS:
    void sendMessage();
    void onInputReturnPressed();
    void onServerMessageReceived(const QJsonDocument &message);

private:
    void setupUI();
    void loadHistory();
    void saveHistory();
    QString formatMessage(const QString &sender, const QString &message, bool isUser) const;
    void updateSessionState();

    ACPClientPlugin *m_plugin;
    KTextEditor::MainWindow *m_mainWindow;
    ACPClientServerManager *m_serverManager = nullptr;
    ACPClientServer *m_server = nullptr;
    QString m_sessionId;

    Ui::ACPChatWidget *m_ui;
    QList<QString> m_messageHistory;
    int m_historyIndex = 0;
};