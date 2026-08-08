/*
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>
    SPDX-FileCopyrightText: 2026 Christoph Cullmann <cullmann@kde.org>

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
class ACPSessionListWidget;
class ACPServerListWidget;
class QTabWidget;

/**
 * @class ACPClientPluginView
 * @brief Kate plugin view for ACP Client integration
 *
 * This class implements KXMLGUIClient to integrate with Kate's GUI system.
 * It manages the chat tool view with servers, sessions, and chat tabs.
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
    explicit ACPClientPluginView(ACPClientPlugin *plugin, KTextEditor::MainWindow *mainWindow, ACPClientServerManager *serverManager);

    /** @brief Destructor */
    ~ACPClientPluginView() override;

    // ========================================================================
    // UI METHODS
    // ========================================================================

    /**
     * @brief Show the chat tool view
     *
     * Creates the tool view in Kate's side panel and initializes the chat widget.
     */
    void showChatToolView();

private Q_SLOTS:
    // ========================================================================
    // SLOTS
    // ========================================================================

    /** @brief Handle session list received from server */
    void onSessionListReceived(const QJsonArray &sessions);

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

    /**
     * @brief Update the enabled state of tabs based on server availability
     *
     * Enables Sessions and Chat tabs only when an active server is available.
     */
    void updateTabEnabledState();

private:
    // ========================================================================
    // SETUP METHODS
    // ========================================================================

    /** @brief Set up the user interface */
    void setupUI();

    // ========================================================================
    // MEMBER VARIABLES
    // ========================================================================

    ACPClientPlugin *m_plugin; ///< Parent plugin instance
    KTextEditor::MainWindow *m_mainWindow; ///< Kate main window
    ACPClientServerManager *m_serverManager; ///< Shared server manager

    // ========================================================================
    // UI ELEMENTS
    // ========================================================================

    QTabWidget *m_tabWidget = nullptr; ///< Tab widget for servers, sessions, and chat
    ACPClientChatWidget *m_chatWidget = nullptr; ///< Main chat widget
    ACPSessionListWidget *m_sessionListWidget = nullptr; ///< Session list widget
    ACPServerListWidget *m_serverListWidget = nullptr; ///< Server list widget
    QWidget *m_chatToolView = nullptr; ///< Kate tool view container

    // Tab indices for enabling/disabling
    int m_serversTabIndex = -1; ///< Index of the Servers tab
    int m_sessionsTabIndex = -1; ///< Index of the Sessions tab
    int m_chatTabIndex = -1; ///< Index of the Chat tab
};
