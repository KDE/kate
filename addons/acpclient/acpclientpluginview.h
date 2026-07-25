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
class ACPClientChatWidget;
class QAction;
class QToolBar;
class QMenu;

/**
 * @class ACPClientPluginView
 * @brief Kate plugin view for ACP Client integration
 *
 * This class implements KXMLGUIClient to integrate with Kate's GUI system.
 * It:
 * - Creates actions for menus and toolbars
 * - Manages the chat tool view
 * - Handles action triggers
 * - Connects UI actions to server manager operations
 *
 * Each Kate main window gets its own ACPClientPluginView instance.
 *
 * @see ACPClientPlugin for the main plugin class
 * @see ACPClientChatWidget for the chat interface
 */

class ACPClientPluginView : public QObject, public KXMLGUIClient
{
    Q_OBJECT

public:
    /**
     * @brief Construct the plugin view
     * @param plugin Parent plugin instance
     * @param mainWindow Kate main window
     * @param serverManager Shared server manager
     */
    explicit ACPClientPluginView(ACPClientPlugin *plugin, KTextEditor::MainWindow *mainWindow, const std::shared_ptr<ACPClientServerManager> &serverManager);

    /** @brief Destructor */
    ~ACPClientPluginView() override;

    // ========================================================================
    // DOCUMENT-ACTION METHODS
    // ========================================================================

    /**
     * @brief Create a session using the entire document as context
     *
     * Reads the current document text and sends it as a prompt to start a session.
     */
    void createSessionFromDocument();

    /**
     * @brief Send the current text selection as a prompt
     *
     * Uses the selected text in the active view as the prompt.
     */
    void sendSelectionAsPrompt();

    // ========================================================================
    // UI METHODS
    // ========================================================================

    /**
     * @brief Show the session management dialog
     * @todo Implement full session manager dialog
     */
    void showSessionManager();

    /**
     * @brief Show the server configuration dialog
     * @todo Implement server configuration dialog
     */
    void showServerConfig();

    /**
     * @brief Show the tool palette
     * @todo Implement tool palette
     */
    void showToolPalette();

    /**
     * @brief Show the chat tool view
     *
     * Creates the tool view in Kate's side panel and initializes the chat widget.
     */
    void showChatToolView();

Q_SIGNALS:
    /** @brief Emitted when a new session should be created with a prompt */
    void sessionRequested(const QString &prompt);

    /** @brief Emitted when a tool should be called */
    void toolCallRequested(const QString &toolId, const QJsonObject &arguments);

private Q_SLOTS:
    // ========================================================================
    // ACTION HANDLERS
    // ========================================================================

    /** @brief Handle "New Session" action */
    void onNewSession();

    /** @brief Handle "Send Prompt" action */
    void onSendPrompt();

    /** @brief Handle "List Sessions" action */
    void onListSessions();

    /** @brief Handle "Manage Servers" action */
    void onManageServers();

    /** @brief Handle "Show Tools" action */
    void onShowTools();

    /** @brief Handle server connected event */
    void onServerConnected();

    /** @brief Handle server disconnected event */
    void onServerDisconnected();

    /**
     * @brief Handle incoming messages from server
     * @param message Received JSON document
     *
     * Displays message information in Kate's message bar.
     */
    void onMessageReceived(const QJsonDocument &message);

private:
    // ========================================================================
    // SETUP METHODS
    // ========================================================================

    /** @brief Create and register all actions */
    void setupActions();

    /** @brief Set up the user interface */
    void setupUI();

    // ========================================================================
    // MEMBER VARIABLES
    // ========================================================================

    ACPClientPlugin *m_plugin; ///< Parent plugin instance
    KTextEditor::MainWindow *m_mainWindow; ///< Kate main window
    std::shared_ptr<ACPClientServerManager> m_serverManager; ///< Shared server manager

    // ========================================================================
    // ACTIONS
    // ========================================================================

    QAction *m_newSessionAction; ///< Action: Create new session
    QAction *m_sendPromptAction; ///< Action: Send document/selection as prompt
    QAction *m_listSessionsAction; ///< Action: List all sessions
    QAction *m_manageServersAction; ///< Action: Configure servers
    QAction *m_showToolsAction; ///< Action: Show available tools

    // ========================================================================
    // UI ELEMENTS
    // ========================================================================

    QWidget *m_toolWidget; ///< Tool widget (currently unused)
    ACPClientChatWidget *m_chatWidget = nullptr; ///< Main chat widget
    QWidget *m_chatToolView = nullptr; ///< Kate tool view container
};