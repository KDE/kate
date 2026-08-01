/*
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>
    SPDX-FileCopyrightText: 2026 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: MIT
*/

#pragma once

#include <QJsonDocument>
#include <QObject>
#include <QProcess>

#include "acpclientprotocol.h"

#include <map>
#include <memory>

class ACPClientPlugin;
class ACPClientServerManager;

/**
 * @class ACPClientServer
 * @brief Manages a connection to a single ACP server process
 *
 * This class handles:
 * - Starting and stopping the server process
 * - Sending and receiving JSON-RPC messages
 * - Initializing the connection with the agent
 * - Tracking request/response pairs
 * - Managing server state
 *
 * The server communicates over stdin/stdout using newline-delimited JSON-RPC 2.0 messages.
 * Each message is read line by line and parsed as a JSON document.
 *
 * @see ACPClientServerManager for managing multiple server connections
 */

/**
 * @brief Server state enumeration
 *
 * Represents the current connection state of the server process.
 */
class ACPClientServer : public QObject
{
    Q_OBJECT

public:
    /** @brief Server connection states */
    enum class ServerState {
        Disconnected, ///< No active connection
        Connecting, ///< Process is being started
        Connected, ///< Process started, not yet initialized
        Initializing, ///< Sending initialize request
        Initialized, ///< Connection fully established and ready
        Error ///< Connection error occurred
    };

    /**
     * @brief Server configuration information
     *
     * Contains all settings needed to start and identify a server.
     */
    struct ServerInfo {
        QString name; ///< Human-readable server name
        QString version; ///< Server version string
        QString command; ///< Executable command path/name
        QStringList arguments; ///< Command-line arguments
        bool autoStart = false; ///< Whether to start automatically
    };

    /**
     * @brief Construct a new server connection
     * @param info Server configuration
     * @param manager Parent server manager
     * @param parent Qt parent object
     */
    explicit ACPClientServer(const ServerInfo &info, ACPClientServerManager *manager, QObject *parent = nullptr);

    /** @brief Destructor - cleans up the process */
    ~ACPClientServer() override;

    // ========================================================================
    // CONNECTION MANAGEMENT
    // ========================================================================

    /**
     * @brief Start the server connection
     *
     * Launches the server process and begins initialization.
     * Will transition through states: Disconnected → Connecting → Connected → Initializing → Initialized
     */
    void start();

    /**
     * @brief Stop the server connection
     *
     * Terminates the server process and cleans up resources.
     * Will transition to Disconnected state.
     */
    void stop();

    /**
     * @brief Initialize the server connection
     *
     * Sends the initialize request to the server after connection is established.
     * This is called automatically after start(), but can be called manually if needed.
     */
    void initializeServer();

    // ========================================================================
    // MESSAGE HANDLING
    // ========================================================================

    /**
     * @brief Send a JSON-RPC message to the server
     * @param message JSON document to send (will be serialized with newline)
     *
     * Messages are written to the server's stdin with a trailing newline.
     * If a request ID is set in the message, it will be tracked for response matching.
     */
    void sendMessage(const QJsonDocument &message);

    // ========================================================================
    // ACCESSORS
    // ========================================================================

    /** @brief Get the current server state */
    ServerState state() const
    {
        return m_state;
    }

    /** @brief Get server configuration */
    ServerInfo info() const
    {
        return m_info;
    }

    /** @brief Get the agent's capabilities (valid after Initialized state) */
    ACP::AgentCapabilities capabilities() const
    {
        return m_capabilities;
    }

    /** @brief Get the negotiated protocol version (valid after Initialized state) */
    QString protocolVersion() const
    {
        return m_protocolVersion;
    }

Q_SIGNALS:
    /** @brief Emitted when the server state changes */
    void stateChanged(ACPClientServer::ServerState state);

    /** @brief Emitted when a message is received from the server */
    void messageReceived(const QJsonDocument &message);

    /** @brief Emitted when an error occurs */
    void errorOccurred(const QString &error);

    /** @brief Emitted when the server is fully initialized */
    void initialized(const ACP::InitializeResult &result);

    /** @brief Emitted when the server disconnects */
    void disconnected();

private Q_SLOTS:
    /** @brief Called when server has data ready to read (stdout) */
    void onProcessReadyRead();

    /** @brief Called when server writes to stderr */
    void onProcessErrorOutput();

    /** @brief Called when process encounters an error */
    void onProcessError(QProcess::ProcessError error);

    /** @brief Called when server process exits */
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    // ========================================================================
    // INTERNAL METHODS
    // ========================================================================

    /**
     * @brief Set up the QProcess connection
     *
     * Creates the QProcess and connects signals for I/O and error handling.
     */
    void setupProcessConnection();

    /**
     * @brief Read available data from the server process
     *
     * Reads line by line from stdout and parses each as JSON-RPC.
     */
    void readFromProcess();

    /**
     * @brief Handle an incoming message from the server
     * @param doc Parsed JSON document
     *
     * Parses the message, matches it to pending requests if applicable,
     * and emits appropriate signals.
     */
    void handleMessage(const QJsonDocument &doc);

    /**
     * @brief Update the server state
     * @param state New state to transition to
     *
     * Changes the state and emits stateChanged signal if the state actually changed.
     */
    void setState(ServerState state);

    // ========================================================================
    // MEMBER VARIABLES
    // ========================================================================

    ServerInfo m_info; ///< Server configuration
    ACPClientServerManager *m_manager; ///< Parent server manager
    ServerState m_state = ServerState::Disconnected; ///< Current connection state
    ACP::AgentCapabilities m_capabilities; ///< Agent capabilities (populated after init)
    QString m_protocolVersion; ///< Negotiated protocol version

    // Connection objects
    std::unique_ptr<QProcess> m_process; ///< Server process (stdin/stdout connected)

    // Request tracking
    /**
     * @brief Map of pending request IDs to their callback functions
     *
     * When a request is sent with an ID, we store a callback here.
     * When the matching response is received, the callback is invoked.
     */
    std::map<qint64, std::function<void(const QJsonDocument &)>> m_pendingRequests;
};
