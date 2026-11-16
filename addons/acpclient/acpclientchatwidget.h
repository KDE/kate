/*
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>
    SPDX-FileCopyrightText: 2026 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: MIT
*/

#pragma once

#include <QHash>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMap>
#include <QPair>
#include <QPointer>
#include <QPushButton>
#include <QSet>
#include <QStringListModel>
#include <QVBoxLayout>
#include <QWidget>

#include <KTextEditor/MainWindow>

#include "acpclientprotocol.h"

class QComboBox;
class QCompleter;
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
     * @param requestId Permission request ID (can be integer or string)
     * @param optionId Selected option ID
     */
    void permissionResponse(const QJsonValue &requestId, const QString &optionId);

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
    void onPermissionRequested(const QJsonValue &requestId, const QJsonObject &toolCall, const QJsonArray &options);

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

    /**
     * @brief Handle text changed in message input
     * Enables/disables send button based on whether there's text
     */
    void onInputTextChanged();

    /**
     * @brief Ask the agent to stop working on the current prompt
     *
     * Nothing is rolled back, the agent just stops where it is. The turn ends
     * when the agent answers the prompt request it is still holding.
     */
    void cancelPrompt();

private:
    // ========================================================================
    // SESSION SELECTORS
    // ========================================================================

    /**
     * @brief A dropdown that changes one setting of the session
     *
     * Agents expose their settings as configuration options, which this client
     * shows as one dropdown each. Agents that only implement the legacy API
     * get a dropdown for their modes and one for their models instead, which
     * have to be applied with the respective legacy request.
     */
    struct SessionSelector {
        /** @brief Request used to apply a value picked by the user */
        enum class Kind {
            ConfigOption, ///< session/set_config_option
            LegacyMode, ///< session/set_mode
            LegacyModel ///< session/set_model
        };

        Kind kind = Kind::ConfigOption;
        QString configId; ///< Configuration option to set, for Kind::ConfigOption
        QString category; ///< Semantic category of the option, may be empty
        QLabel *label = nullptr; ///< Name shown in front of the dropdown
        QComboBox *combo = nullptr; ///< The dropdown itself
    };

    // ========================================================================
    // INTERNAL STATE MANAGEMENT
    // ========================================================================

    /** @brief Update UI based on session state */
    void updateSessionState();

    /** @brief Update action enablement based on server initialization state */
    void updateActionStates();

    /**
     * @brief Show whether the agent is working and offer to stop it
     *
     * Agents usually reject a prompt while they are still working on the
     * previous one, so the state has to be visible next to the input.
     */
    void updatePromptState();

    /**
     * @brief Rebuild the session selectors from the state of the session
     *
     * Creates one dropdown per configuration option the agent exposes, or one
     * for the modes and one for the models when the agent only supports the
     * legacy API.
     */
    void rebuildSessionSelectors();

    /**
     * @brief Add one dropdown to the row of session selectors
     * @param name Label shown in front of the dropdown
     * @param description Tooltip of the dropdown, may be empty
     * @param values Selectable values, as display name and value pairs
     * @param currentValue Value to preselect
     * @param selector Identification used when the user picks a value
     */
    void addSessionSelector(const QString &name,
                            const QString &description,
                            const QList<QPair<QString, QString>> &values,
                            const QString &currentValue,
                            const SessionSelector &selector);

    /** @brief Remove all session selectors and forget the session state */
    void clearSessionConfigUi();

    /** @brief The dropdown of the mode selector, or nullptr if there is none */
    QComboBox *modeSelectorCombo() const;

    /** @brief Update the slash command menu from the agent's command list */
    void updateAvailableCommands(const QList<ACP::AvailableCommand> &commands);

    /** @brief Clear the slash command menu */
    void clearAvailableCommands();

    /** @brief Show the slash command menu above the input field */
    void showCommandsMenu();

    /** @brief Show slash command suggestions while typing a command */
    void updateCommandCompletion();

    /** @brief Replace the leading command token with the given command */
    void insertCommand(const QString &name);

    /** @brief Add a message widget to the display */
    void addMessageWidget(ACPChatMessageWidget *widget);

    /** @brief Remove a single message widget from the display */
    void removeMessageWidget(ACPChatMessageWidget *widget);

    /**
     * @brief Show the placeholder that stands for the answer being worked on
     *
     * Kept as the last message, so that it moves down as the answer arrives
     * piece by piece, and removed when the prompt turn ends.
     */
    void showPendingMessage();

    /** @brief Remove the placeholder of the answer being worked on */
    void removePendingMessage();

    /**
     * @brief Remember a message to send once the agent is free again
     * @param message Text the user wants to send
     *
     * Agents reject a prompt while they are working on the previous one, so
     * the message is shown at the bottom of the chat and sent at the next
     * opportunity, see noteTurnBoundary().
     */
    void queueMessage(const QString &message);

    /**
     * @brief Send everything that is waiting, as one prompt
     *
     * Does nothing while the agent is still working. The queued messages are
     * sent together rather than one per turn, since they were written as one
     * train of thought while waiting.
     */
    void flushQueuedMessages();

    /**
     * @brief Drop a queued message without sending it
     * @param widget Bubble of the message to drop
     *
     * The bubble stays in the conversation, marked as cancelled, so that it
     * is clear what was dropped. It is not remembered beyond the session.
     */
    void cancelQueuedMessage(ACPChatMessageWidget *widget);

    /**
     * @brief Note that the agent reached a point where it can be interrupted
     *
     * Called when the agent finished a part of its answer, that is a message,
     * a reasoning block, a plan or a tool call. If messages are waiting, the
     * running prompt turn is cancelled here, which lets them be sent at the
     * next of those points instead of only when the agent is done.
     *
     * Does nothing while a tool call is running, so that work already under
     * way is not abandoned.
     */
    void noteTurnBoundary();

    /**
     * @brief Whether a tool call status means the call is done
     * @param status Status reported by the agent
     */
    static bool isToolCallFinished(const QString &status);

    /**
     * @brief Send a message and show it in the chat
     * @param message Text to send
     */
    void deliverMessage(const QString &message);

    /**
     * @brief Remove the placeholder and the queued messages from the chat
     *
     * They are kept at the bottom, so they have to make room whenever a
     * message arrives. The queued texts themselves are not forgotten.
     */
    void takeTrailingMessages();

    /** @brief Put the placeholder and the queued messages back at the bottom */
    void restoreTrailingMessages();

    /**
     * @brief Rebuild the bottom of the chat
     *
     * Used when the placeholder or the queued messages changed, which has to
     * keep them in order: the answer being worked on first, then the messages
     * waiting for it.
     */
    void rebuildTrailingMessages();

    /** @brief Forget the queued messages and remove them from the chat */
    void clearQueuedMessages();

    /**
     * @brief The newest real message, ignoring the pending placeholder
     * @return Last message widget, or nullptr if there is none
     *
     * Streamed chunks are appended to the message they belong to, which is
     * usually the last one. The placeholder must not be mistaken for it.
     */
    ACPChatMessageWidget *lastContentMessage() const;

    /** @brief Clear all message widgets */
    void clearMessages();

    /** @brief Update the size hint for a widget's list item */
    void updateWidgetSizeHint(ACPChatMessageWidget *widget);

    /** @brief Update all item size hints based on current viewport width */
    void updateAllItemSizeHints();

    /** @brief Update the size hint for a specific list item */
    void updateItemSizeHint(QListWidgetItem *item, QWidget *container);

    /** @brief Scroll the chat to the bottom to show the last message */
    void scrollToBottom();

    /**
     * @brief Scroll the chat to the newest message and follow it again
     *
     * Unlike scrollToBottom(), this ignores whether the user had scrolled up,
     * because it is only called when the user asked for it.
     */
    void jumpToBottom();

    /** @brief Get all chat text for copying */
    QString getAllChatText() const;

    // ========================================================================
    // MESSAGE TYPE HANDLERS
    // ========================================================================

    /** @brief Handle agent_message_chunk session update */
    void handleAgentMessageChunk(const QJsonObject &update);

    /**
     * @brief Handle agent_thought_chunk session update
     *
     * The reasoning of the agent is displayed in messages of its own, which
     * are collapsed unless the user asked for them to be expanded.
     */
    void handleAgentThoughtChunk(const QJsonObject &update);

    /**
     * @brief Apply the reasoning setting of the plugin to this chat
     *
     * Expands or collapses the reasoning of the messages on screen and
     * decides how the ones arriving afterwards start out.
     */
    void applyThoughtVisibility();

    /**
     * @brief Handle user_message_chunk session update
     *
     * Agents replay these when a session is loaded, and may echo the prompt
     * that was just sent. The echo is recognized and dropped, because the
     * message is displayed already when it is sent.
     */
    void handleUserMessageChunk(const QJsonObject &update);

    /** @brief Handle plan session update */
    void handlePlanUpdate(const QJsonObject &update);

    /** @brief Handle tool_call session update */
    void handleToolCallUpdate(const QJsonObject &update);

    /** @brief Handle tool_call_update session update */
    void handleToolCallStatusUpdate(const QJsonObject &update);

    /** @brief Handle usage_update session update */
    void handleUsageUpdate(const QJsonObject &update);

    /** @brief Handle current_mode_update session update */
    void handleCurrentModeUpdate(const QJsonObject &update);

    /** @brief Handle config_option_update session update */
    void handleConfigOptionUpdate(const QJsonObject &update);

    /** @brief Handle available_commands_update session update */
    void handleAvailableCommandsUpdate(const QJsonObject &update);

    // ========================================================================
    // REFERENCES
    // ========================================================================

    ACPClientPlugin *m_plugin; ///< Parent plugin instance
    KTextEditor::MainWindow *m_mainWindow; ///< Kate main window
    ACPClientServerManager *m_serverManager = nullptr; ///< Server manager (shared)
    ACPClientServer *m_server = nullptr; ///< Active server for this chat
    QString m_sessionId; ///< Current session ID
    bool m_promptRunning = false; ///< True while the agent works on a prompt
    bool m_expandThoughts = false; ///< Whether reasoning starts out expanded
    bool m_cancelRequested = false; ///< True once the agent was asked to stop the running prompt

    // ========================================================================
    // SESSION CONFIGURATION
    // ========================================================================

    bool m_hasConfigOptions = false; ///< True once config options have been received
    QList<ACP::SessionConfigOption> m_configOptions; ///< Configuration options of the session
    ACP::SessionModeState m_legacyModes; ///< Modes of the session, legacy API
    ACP::SessionModelState m_legacyModels; ///< Models of the session, legacy API
    QList<SessionSelector> m_sessionSelectors; ///< Dropdowns currently shown for the session

    // ========================================================================
    // SLASH COMMANDS
    // ========================================================================

    // ========================================================================
    // USER MESSAGE ECHO TRACKING
    // ========================================================================

    QPointer<ACPChatMessageWidget> m_pendingMessage; ///< Placeholder for the answer being worked on
    QStringList m_queuedMessages; ///< Messages waiting for the agent to become free
    QList<QPointer<ACPChatMessageWidget>> m_queuedMessageWidgets; ///< Bubbles showing the queued messages
    bool m_updatingTrailingMessages = false; ///< Guards against recursing while the bottom is rebuilt
    bool m_cancellingForQueue = false; ///< True while the running turn is cancelled to send queued messages
    QSet<QString> m_runningToolCalls; ///< Tool calls that were started and have not reported a result yet

    QPointer<ACPChatMessageWidget> m_locallyShownUserMessage; ///< Message displayed when sending, awaiting its echo
    QSet<QString> m_echoedUserMessageIds; ///< IDs of user messages the agent echoed back

    QList<ACP::AvailableCommand> m_availableCommands; ///< Commands advertised by the agent
    QStringListModel *m_commandCompletionModel = nullptr; ///< Command names for input completion
    QCompleter *m_commandCompleter = nullptr; ///< Completion popup for slash commands

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
    QStringListModel *m_historyModel = nullptr; ///< Model for message history

    // ========================================================================
    // MESSAGE DISPLAY
    // ========================================================================

    QListWidget *m_chatListWidget = nullptr; ///< List widget for messages
    bool m_followChatBottom = true; ///< Keep scrolling to new messages while the view is at the bottom
    QHash<ACPChatMessageWidget *, QWidget *> m_widgetToContainerMap; ///< Map from widgets to container widgets
    QHash<QWidget *, QListWidgetItem *> m_containerToItemMap; ///< Map from container widgets to list items
    QList<ACPChatMessageWidget *> m_messageWidgets; ///< All displayed message widgets

    // ========================================================================
    // MESSAGE TRACKING
    // ========================================================================

    QString m_lastMessageId; ///< Last message ID for streaming chunks

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

    /**
     * @brief Handle key press events for the chat widget
     */
    void keyPressEvent(QKeyEvent *event) override;
};
