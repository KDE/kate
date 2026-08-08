/*
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>
    SPDX-FileCopyrightText: 2026 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: MIT
*/

#pragma once

#include <QObject>
#include <QUrl>

#include <KTextEditor/Message>
#include <KTextEditor/Plugin>

#include <map>
#include <memory>
#include <set>

#include "acpclientservermanager.h"

class ACPClientPluginView;

/**
 * @namespace ACPClientPluginOptions
 * @brief Configuration options for the ACP Client plugin
 *
 * Contains all user-configurable settings for the plugin.
 */

/**
 * @struct ACPClientPluginOptions
 * @brief Configuration options for the ACP Client plugin
 *
 * All options are stored in KConfig and can be configured via the
 * Kate Settings dialog (Plugins → ACP Client).
 */
struct ACPClientPluginOptions {
    // Tool call permission settings
    /** @brief Permission modes for tool calls */
    enum ToolCallPermission {
        AllowAll, ///< Automatically allow all tool calls without prompting
        DenyAll, ///< Automatically deny all tool calls
        AskEachTime ///< Ask user for permission each time a tool is called
    };
    ToolCallPermission m_toolCallPermission = AskEachTime; ///< Default: ask user each time
};

/**
 * @class ACPClientPlugin
 * @brief Main plugin class for the ACP Client
 *
 * This class is the KTextEditor::Plugin implementation that:
 * - Manages plugin lifecycle
 * - Stores and retrieves configuration
 * - Creates plugin views for each Kate main window
 * - Provides access to the server manager
 * - Handles command line permission management
 *
 * The plugin is a singleton per Kate instance. Each Kate main window gets
 * its own ACPClientPluginView but shares the same plugin instance.
 *
 * @see ACPClientPluginView for the per-window view integration
 * @see ACPClientServerManager for server connection management
 */
class ACPClientPlugin : public KTextEditor::Plugin, public ACPClientPluginOptions
{
    Q_OBJECT

    friend class ACPClientConfigPage; ///< Config page needs access to private members

public:
    /**
     * @brief Path for local setting files
     *
     * Directory is auto-created on plugin load.
     * Defaults to ~/.config/kate/acpclient/
     */
    const QString m_settingsPath;

    /**
     * @brief Default configuration file path
     *
     * Points to settings.json in the settings directory.
     */
    const QUrl m_defaultConfigPath;

    /**
     * @brief Constructor
     * @param parent Qt parent object
     *
     * Initializes settings paths and loads configuration.
     */
    explicit ACPClientPlugin(QObject *parent);

    /** @brief Destructor */
    ~ACPClientPlugin() override;

    // ========================================================================
    // KTEXteditor::PLUGIN INTERFACE
    // ========================================================================

    /**
     * @brief Create a view for a Kate main window
     * @param mainWindow The main window to create a view for
     * @return The created view object
     *
     * Called once per main window. Creates the ACPClientPluginView and
     * connects it to the server manager.
     */
    QObject *createView(KTextEditor::MainWindow *mainWindow) override;

    // ========================================================================
    // CONFIGURATION INTERFACE
    // ========================================================================

    /** @brief Get the number of configuration pages */
    int configPages() const override;

    /**
     * @brief Get a configuration page by index
     * @param number Page index (0 = main config page)
     * @param parent Parent widget for the page
     * @return Configuration page widget
     */
    KTextEditor::ConfigPage *configPage(int number = 0, QWidget *parent = nullptr) override;

    /**
     * @brief Write current configuration to KConfig
     *
     * Called when configuration changes are applied.
     */
    void writeConfig() const;

    // ========================================================================
    // ACCESSORS
    // ========================================================================

    /** @brief Get the current configuration file path */
    QUrl configPath() const
    {
        return m_configPath.isEmpty() ? m_defaultConfigPath : m_configPath;
    }

    /** @brief Get the server manager instance */
    ACPClientServerManager *serverManager()
    {
        return &m_serverManager;
    }

    /** @brief Get the tool call permission mode */
    ToolCallPermission toolCallPermission() const
    {
        return m_toolCallPermission;
    }

    QUrl m_configPath; ///< Current configuration file path (may be overridden)

Q_SIGNALS:
    // ========================================================================
    // SIGNALS
    // ========================================================================

    /**
     * @brief Emitted when settings change
     *
     * Views should update their state when this signal is received.
     */
    void update() const;

    /**
     * @brief Request to show a message in the Kate UI
     * @param level Message severity level
     * @param msg Message text
     *
     * Connected to by views to display messages in Kate's message bar.
     */
    void showMessage(KTextEditor::Message::MessageType level, const QString &msg);

private:
    // ========================================================================
    // INTERNAL METHODS
    // ========================================================================

    /**
     * @brief Load configuration from KConfig
     *
     * Reads all plugin options from the KConfig store.
     * This function may be called only from the constructor because it assumes
     * that the ACPClientPluginOptions base of this object is default-initialized.
     */
    void readConfig();

    // ========================================================================
    // MEMBER VARIABLES
    // ========================================================================

    /**
     * @brief Shared server manager instance
     *
     * Created on first view creation and shared across all views.
     */
    ACPClientServerManager m_serverManager;
};
