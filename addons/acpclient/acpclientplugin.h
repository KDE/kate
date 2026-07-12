/*
    SPDX-FileCopyrightText: 2026

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

class ACPClientServerManager;
class ACPClientPluginView;

/**
 * Options for the ACP client plugin.
 */
struct ACPClientPluginOptions {
    bool m_autoStartSession = false;
    bool m_showNotifications = true;
    bool m_showToolCalls = true;
    bool m_showProgress = true;
    bool m_debugMode = false;
};

class ACPClientPlugin : public KTextEditor::Plugin, public ACPClientPluginOptions
{
    Q_OBJECT

    friend class ACPClientConfigPage;

public:
    /**
     * Path for local setting files, auto-created on load.
     */
    const QString m_settingsPath;

    /**
     * Default config path.
     */
    const QUrl m_defaultConfigPath;

    explicit ACPClientPlugin(QObject *parent);
    ~ACPClientPlugin() override;

    QObject *createView(KTextEditor::MainWindow *mainWindow) override;

    int configPages() const override;
    KTextEditor::ConfigPage *configPage(int number = 0, QWidget *parent = nullptr) override;

    void writeConfig() const;

    // Get current config path
    QUrl configPath() const
    {
        return m_configPath.isEmpty() ? m_defaultConfigPath : m_configPath;
    }

    // Get the server manager
    ACPClientServerManager *serverManager() const
    {
        return m_serverManager ? m_serverManager.get() : nullptr;
    }

    // Hash of allowed and blacklisted server command lines
    std::map<QString, bool> m_serverCommandLineToAllowedState;

    // Current active dialogs to ask for permission of some command line
    std::set<QString> m_currentActiveCommandLineDialogs;

    // Debug mode?
    bool m_debugMode;

    QUrl m_configPath;

    /**
     * Check if given command line is allowed to be executed.
     * Might ask the user for permission.
     * @param cmdline full command line including program to check
     * @return execution allowed?
     */
    bool isCommandLineAllowed(const QStringList &cmdline);

Q_SIGNALS:
    // Signal settings update
    void update() const;

    void showMessage(KTextEditor::Message::MessageType level, const QString &msg);

private Q_SLOTS:
    /**
     * Ask the user via dialog if the given command line shall be allowed.
     * Will store the result internally and trigger ACP server restart after config change.
     * Will ensure we just ask once, even if multiple requests queue up.
     * @param fullCommandLineString full command line string to get permission for
     */
    void askForCommandLinePermission(const QString &fullCommandLineString);

private:
    /**
     * Read from config and assign to data members.
     * This function may be called only from the constructor because it assumes
     * that the ACPClientPluginOptions base of this object is default-initialized.
     */
    void readConfig();

    // Server manager to pass along
    std::shared_ptr<ACPClientServerManager> m_serverManager = nullptr;

    // Current views
    QList<ACPClientPluginView *> m_views;
};
