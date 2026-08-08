/*
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>
    SPDX-FileCopyrightText: 2026 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: MIT
*/

#pragma once

#include <QJsonArray>
#include <QJsonDocument>
#include <QList>
#include <QObject>

#include <KTextEditor/Message>

#include "acpclientserver.h"

#include <map>
#include <memory>
#include <qjsondocument.h>

class ACPClientPlugin;

/**
 * @class ACPClientServerManager
 * @brief Manages multiple ACP server connections and coordinates session management
 *
 * This is the central hub for all server-related operations. It:
 * - Creates and manages multiple ACPClientServer instances
 * - Handles session lifecycle (create, load, resume, close, delete)
 * - Routes messages between servers and connected clients (UI)
 * - Manages permission requests from servers
 * - Tracks active and available servers
 *
 * The manager is typically created once per plugin instance and shared across views.
 *
 * @see ACPClientServer for individual server connections
 * @see ACPClientPlugin for the main plugin class
 */

class ACPClientServerManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Construct a new server manager
     * @param plugin Parent plugin instance
     * @param parent Qt parent object
     *
     * Automatically loads default servers from configuration.
     */
    explicit ACPClientServerManager(ACPClientPlugin *plugin, QObject *parent = nullptr);

    /** @brief Destructor - cleans up all managed servers */
    ~ACPClientServerManager() override;

    // ========================================================================
    // SERVER MANAGEMENT
    // ========================================================================

    /**
     * @brief Create a new server connection
     * @param info Server configuration
     * @return Pointer to the new server (managed by this manager)
     *
     * The server will be added to the internal list.
     * The serverAdded signal will be emitted.
     */
    ACPClientServer *createServer(const ACPClientServer::ServerInfo &info);

    /**
     * @brief Load servers from default configuration
     *
     * Reads server configurations from the shipped settings.json resource
     * and creates server instances for them.
     */
    void loadDefaultServers();

    /** @brief Get all managed servers */
    QList<ACPClientServer *> servers() const;

    /**
     * @brief Get a server by its name
     * @param name Server name to find
     * @return Pointer to the server, or nullptr if not found
     */
    ACPClientServer *server(const QString &name) const;

    /**
     * @brief Remove and delete a server
     * @param server Server to remove
     *
     * Stops the server, removes it from the list, and deletes it.
     * The serverRemoved signal will be emitted.
     */
    void removeServer(ACPClientServer *server);

    // ========================================================================
    // ACTIVE SERVER MANAGEMENT
    // ========================================================================

    /**
     * @brief Get the currently active server
     * @return First initialized server, or nullptr if none
     *
     * The active server is the one used for session operations.
     * Currently returns the first initialized server found.
     */
    ACPClientServer *activeServer() const;

    /**
     * @brief Set the active server by name
     * @param name Name of the server to make active
     *
     * The server must already exist (have been created).
     * The activeServerChanged signal will be emitted.
     */
    void setActiveServer(const QString &name);

    /**
     * @brief Check if the active server supports session loading
     * @return true if the agent supports loadSession capability
     */
    bool supportsLoadSession() const;

    /**
     * @brief Check if the active server supports session resuming
     * @return true if the agent supports sessionCapabilities.resume
     */
    bool supportsResumeSession() const;

    // ========================================================================
    // SESSION MANAGEMENT
    // ========================================================================

    /**
     * @brief Create a new session on the active server
     * @return Session ID (may be empty if creation failed)
     *
     * Emits sessionCreated signal when the session is created.
     * The actual session ID is received asynchronously via the signal.
     */
    QString createSession();

    /**
     * @brief Load an existing session
     * @param sessionId ID of the session to load
     * @return Session ID (may be empty if load failed)
     *
     * Emits sessionLoaded signal when complete.
     */
    QString loadSession(const QString &sessionId);

    /**
     * @brief Resume a paused session
     * @param sessionId ID of the session to resume
     * @return Session ID (may be empty if resume failed)
     *
     * Emits sessionResumed signal when complete.
     */
    QString resumeSession(const QString &sessionId);

    /**
     * @brief Close a session
     * @param sessionId ID of the session to close
     *
     * Emits sessionClosed signal when complete.
     */
    void closeSession(const QString &sessionId);

    /**
     * @brief Send a prompt to a session
     * @param sessionId Target session ID
     * @param message Prompt text to send
     */
    void sendPrompt(const QString &sessionId, const QString &message);

    /** @brief Request list of all sessions from the active server */
    void listSessions();

    /**
     * @brief Delete a session
     * @param sessionId ID of the session to delete
     *
     * Emits sessionDeleted signal when complete.
     */
    void deleteSession(const QString &sessionId);

    // ========================================================================
    // TOOL MANAGEMENT
    // ========================================================================

    /** @brief Request list of available tools from the active server */
    void listTools();

    /**
     * @brief Call a tool on the active server
     * @param toolId Tool identifier
     * @param arguments Tool arguments as JSON object
     *
     * Emits toolCallCompleted signal when the tool call finishes.
     */
    void callTool(const QString &toolId, const QJsonObject &arguments);

Q_SIGNALS:
    // ========================================================================
    // SERVER SIGNALS
    // ========================================================================

    /** @brief Emitted when a new server is added */
    void serverAdded(ACPClientServer *server);

    /** @brief Emitted when a server is removed */
    void serverRemoved(ACPClientServer *server);

    /** @brief Emitted when the active server changes */
    void activeServerChanged(ACPClientServer *server);

    // ========================================================================
    // SESSION SIGNALS
    // ========================================================================

    /** @brief Emitted when a session is created */
    void sessionCreated(const QString &sessionId);

    /** @brief Emitted when a session is loaded */
    void sessionLoaded(const QString &sessionId);

    /** @brief Emitted when a session is resumed */
    void sessionResumed(const QString &sessionId);

    /** @brief Emitted when a session is closed */
    void sessionClosed(const QString &sessionId);

    /** @brief Emitted when session list is received */
    void sessionListReceived(const QJsonArray &sessions);

    /** @brief Emitted when a session is deleted */
    void sessionDeleted(const QString &sessionId);

    // ========================================================================
    // TOOL SIGNALS
    // ========================================================================

    /** @brief Emitted when tool list is received */
    void toolsReceived(const QJsonArray &tools);

    /** @brief Emitted when a tool call completes */
    void toolCallCompleted(qint64 callId, const QJsonObject &result);

    // ========================================================================
    // MESSAGE AND ERROR SIGNALS
    // ========================================================================

    /** @brief Emitted when a message is received from any server */
    void messageReceived(const QJsonDocument &message);

    /** @brief Emitted when an error occurs */
    void errorOccurred(const QString &error);

    // ========================================================================
    // PERMISSION AND SESSION UPDATE SIGNALS
    // ========================================================================

    /**
     * @brief Emitted when a permission request is received from the server
     * @param requestId Permission request ID to respond to
     * @param toolCall Tool call details
     * @param options Available permission options
     *
     * The receiver should display the permission request to the user
     * and call createPermissionResponse() with the user's choice.
     */
    void permissionRequested(qint64 requestId, const QJsonObject &toolCall, const QJsonArray &options);

    /**
     * @brief Emitted when a session update notification is received
     * @param sessionId The session being updated
     * @param update The update object
     *
     * This is a lower-level signal for session updates. For most use cases,
     * use the messageReceived signal and parse the update type.
     */
    void sessionUpdateReceived(const QString &sessionId, const QJsonObject &update);

private Q_SLOTS:
    /** @brief Handle server initialized signal */
    void onServerInitialized(const ACP::InitializeResult &result);

    /** @brief Handle server disconnected signal */
    void onServerDisconnected();

    /** @brief Handle server error signal */
    void onServerError(const QString &error);

    /**
     * @brief Handle incoming messages from servers
     * @param message Received JSON document
     *
     * Routes messages based on their type (request, response, notification).
     */
    void onServerMessageReceived(const QJsonDocument &message);

private:
    // ========================================================================
    // INTERNAL MESSAGE HANDLERS
    // ========================================================================

    /**
     * @brief Handle a session/update notification
     * @param doc JSON document with session update
     */
    void handleSessionUpdate(const QJsonDocument &doc);

    /**
     * @brief Handle a progress notification
     * @param doc JSON document with progress update
     */
    void handleProgressNotification(const QJsonDocument &doc);

    /**
     * @brief Handle a permission request from the server
     * @param doc JSON document with permission request
     */
    void handlePermissionRequest(const QJsonDocument &doc);

    void showMessage(const QString &msg, KTextEditor::Message::MessageType level);

    // ========================================================================
    // MEMBER VARIABLES
    // ========================================================================

    ACPClientPlugin *m_plugin; ///< Parent plugin instance

    // current json config for the servers
    QJsonObject m_serverConfig;

    /**
     * @brief List of all managed servers
     *
     * Uses unique_ptr for automatic cleanup when servers are removed.
     */
    std::vector<std::unique_ptr<ACPClientServer>> m_servers;

    ACPClientServer *m_activeServer = nullptr; ///< Currently active server
    QString m_activeServerName; ///< Name of the active server

    /**
     * @brief Track pending session creation requests
     *
     * Maps request IDs to... (note: current implementation stores requestId twice)
     * @todo Clean up this mapping - currently stores requestId as both key and value
     */
    std::map<qint64, qint64> m_pendingSessionRequests;

    /**
     * @brief Request types for tracking pending requests
     */
    enum class PendingRequestType {
        SessionNew,
        SessionLoad,
        SessionResume,
        SessionClose,
        SessionList,
        SessionDelete,
        ToolsList,
        ToolsCall
    };

    /**
     * @brief Track all pending requests by type
     *
     * Maps request IDs to their request type for proper response handling.
     */
    std::map<qint64, PendingRequestType> m_pendingRequests;

public:
    // ========================================================================
    // FACTORY METHOD
    // ========================================================================

    /**
     * @brief Factory method to create or get the singleton instance
     * @param plugin Parent plugin instance
     * @param parent Qt parent object
     * @return Server manager instance
     *
     * @note This maintains a singleton pattern - the first call creates the instance,
     * subsequent calls return the same instance.
     */
    static ACPClientServerManager *new_(ACPClientPlugin *plugin, QObject *parent = nullptr);
};
