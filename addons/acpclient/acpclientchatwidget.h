/*
    SPDX-FileCopyrightText: 2026 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: MIT
*/

#pragma once

#include <QComboBox>
#include <QCompleter>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QStringListModel>
#include <QVBoxLayout>
#include <QWidget>

#include <KTextEditor/MainWindow>

class QMenu;

class ACPClientPlugin;
class ACPClientServer;
class ACPClientServerManager;
class ACPChatMessageWidget;

namespace Ui
{
class ACPChatWidget;
}

/**
 * @class ACPClientChatWidget
 * @brief Main chat widget for ACP agent interaction
 *
 * This widget provides the complete chat interface including:
 * - Message display area with scrollable history
 * - Message input field with send button
 * - Session controls (new, end, copy)
 * - Status bar for usage information
 *
 * The widget connects to the server manager to send and receive messages.
 * It handles various ACP message types (agent messages, tool calls, plans, etc.)
 * and displays them appropriately.
 *
 * @see ACPChatMessageWidget for individual message display
 * @see ACPClientPluginView for plugin view integration
 */
class ACPClientChatWidget : public QWidget
{
    Q_OBJECT

public:
    /** @brief Message status for tracking prompt turn state */
    enum class MessageStatus {
        None, ///< No status (default)
        Running, ///< Prompt is being processed
        Completed, ///< Prompt turn completed successfully
        Error, ///< Prompt turn ended with error
        Cancelled ///< Prompt turn was cancelled
    };

    /**
     * @brief Construct the chat widget
     * @param plugin Parent plugin instance
     * @param mainWindow Kate main window
     * @param parent Qt parent widget
     */
    explicit ACPClientChatWidget(ACPClientPlugin *plugin, KTextEditor::MainWindow *mainWindow, QWidget *parent = nullptr);

    /** @brief Destructor */
    ~ACPClientChatWidget() override;

    // ========================================================================
    // SESSION MANAGEMENT
    // ========================================================================

    /**
     * @brief Start a new chat session
     *
     * Creates a new session via the server manager and sets up connections
     * to receive messages for that session.
     */
    void startNewSession();

    /** @brief Set the active session ID */
    void setSessionId(const QString &sessionId);

    /** @brief Get the current session ID */
    QString sessionId() const;

    // ========================================================================
    // MESSAGE MANAGEMENT
    // ========================================================================

    /**
     * @brief Append a message to the chat display
     * @param sender Who sent the message
     * @param message Message text
     * @param isUser Whether this is a user message (affects styling)
     */
    void appendMessage(const QString &sender, const QString &message, bool isUser = false);

    /** @brief Clear all messages from the chat */
    void clearChat();

    /**
     * @brief Update the status of the last user message
     * @param status The status to set (Running, Completed, Error, Cancelled)
     *
     * Used to track prompt turn lifecycle and show appropriate icons.
     */
    void updateLastUserMessageStatus(MessageStatus status);

    // ========================================================================
    // SERVER MANAGEMENT
    // ========================================================================

    /**
     * @brief Set the active server
     * @param server Server to use for this chat
     *
     * Disconnects from the previous server and connects to the new one.
     */
    void setServer(ACPClientServer *server);

    /**
     * @brief Set up all connections for the current server and manager
     *
     * Connects to server messages, server manager signals, and permission requests.
     * Call this after setting the server and server manager.
     */
    void setupServerConnections();

    /**
     * @brief Initialize the chat widget with an existing session
     * @param sessionId The session ID to use
     *
     * Sets up the server, connections, and session ID for an existing session
     * (loaded or resumed from the session list).
     */
    void initializeWithSession(const QString &sessionId);

Q_SIGNALS:
    // ========================================================================
    // OUTGOING SIGNALS
    // ========================================================================

    /** @brief Emitted when a message is sent */
    void messageSent(const QString &sessionId, const QString &message);

    /** @brief Emitted when a new session is requested */
    void sessionRequested();

    /**
     * @brief Emitted when user responds to a permission request
     * @param requestId Permission request ID
     * @param optionId Selected option ID
     */
    void permissionResponse(qint64 requestId, const QString &optionId);

private Q_SLOTS:
    // ========================================================================
    // USER ACTION HANDLERS
    // ========================================================================

    /** @brief Handle send button click or Enter key in input */
    void sendMessage();

    /** @brief Handle Return key in input field */
    void onInputReturnPressed();

    /** @brief Handle end session button click */
    void endSession();

    // ========================================================================
    // SERVER MESSAGE HANDLERS
    // ========================================================================

    /**
     * @brief Handle incoming messages from the server
     * @param message JSON document received
     *
     * Parses the message and routes to appropriate handler based on type.
     */
    void onServerMessageReceived(const QJsonDocument &message);

    /**
     * @brief Handle permission requests from the server
     * @param requestId Permission request ID
     * @param toolCall Tool call details
     * @param options Available permission options
     *
     * Creates an inline permission request widget in the chat.
     */
    void onPermissionRequested(qint64 requestId, const QJsonObject &toolCall, const QJsonArray &options);

    /** @brief Handle copy button click */
    void copyChatText();

public Q_SLOTS:
    // ========================================================================
    // STATUS UPDATES
    // ========================================================================

    /**
     * @brief Update the status bar text
     * @param text Status text to display
     *
     * Used to show token usage, cost, connection status, etc.
     */
    void updateStatus(const QString &text);

private:
    // ========================================================================
    // INTERNAL STATE MANAGEMENT
    // ========================================================================

    /** @brief Update UI based on session state */
    void updateSessionState();

    /** @brief Update action enablement based on server initialization state */
    void updateActionStates();

    /** @brief Add a message widget to the display */
    void addMessageWidget(ACPChatMessageWidget *widget);

    /** @brief Clear all message widgets */
    void clearMessages();

    /** @brief Get all chat text for copying */
    QString getAllChatText() const;

    // ========================================================================
    // MESSAGE TYPE HANDLERS
    // ========================================================================

    /** @brief Handle agent_message_chunk session update */
    void handleAgentMessageChunk(const QJsonObject &update);

    /** @brief Handle plan session update */
    void handlePlanUpdate(const QJsonObject &update);

    /** @brief Handle tool_call session update */
    void handleToolCallUpdate(const QJsonObject &update);

    /** @brief Handle tool_call_update session update */
    void handleToolCallStatusUpdate(const QJsonObject &update);

    /** @brief Handle usage_update session update */
    void handleUsageUpdate(const QJsonObject &update);

    // ========================================================================
    // REFERENCES
    // ========================================================================

    ACPClientPlugin *m_plugin; ///< Parent plugin instance
    KTextEditor::MainWindow *m_mainWindow; ///< Kate main window
    ACPClientServerManager *m_serverManager = nullptr; ///< Server manager (shared)
    ACPClientServer *m_server = nullptr; ///< Active server for this chat
    QString m_sessionId; ///< Current session ID

    // ========================================================================
    // TOOL CALL TRACKING
    // ========================================================================

    /** @brief Store tool call information by ID for permission lookups */
    QMap<QString, QJsonObject> m_toolCalls;

    // ========================================================================
    // UI REFERENCES
    // ========================================================================

    Ui::ACPChatWidget *m_ui; ///< UI form from acpclientchat.ui

    // ========================================================================
    // MESSAGE HISTORY
    // ========================================================================

    QList<QString> m_messageHistory; ///< History of user messages
    QStringListModel *m_historyModel = nullptr; ///< Model for history completer
    QCompleter *m_completer = nullptr; ///< Completer for input combobox

    // ========================================================================
    // MESSAGE DISPLAY
    // ========================================================================

    QVBoxLayout *m_chatMessagesLayout = nullptr; ///< Layout for message widgets
    QScrollArea *m_chatScrollArea = nullptr; ///< Scrollable area for messages
    QWidget *m_chatDisplayContainer = nullptr; ///< Container widget for messages
    QList<ACPChatMessageWidget *> m_messageWidgets; ///< All displayed message widgets

    // ========================================================================
    // STATUS BAR
    // ========================================================================

    QLabel *m_statusLabel = nullptr; ///< Status bar label for usage/cost info

protected:
    // ========================================================================
    // EVENT HANDLING
    // ========================================================================

    /**
     * @brief Handle context menu and other events
     * @param watched Object being watched
     * @param event Event to handle
     * @return true if event was handled, false otherwise
     *
     * Currently handles context menu on the chat display for "Copy All" action.
     */
    bool eventFilter(QObject *watched, QEvent *event) override;
};