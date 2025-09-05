/*
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>

    SPDX-License-Identifier: MIT
*/

#pragma once

#include <QUrl>
#include <QVariant>

#include <KTextEditor/Message>
#include <KTextEditor/Plugin>

#include <map>
#include <set>

class LSPClientServerManager;

/**
 * This struct aggregates the options that are reset to their
 * default values when the user clicks the Restore Defaults button.
 *
 * @see LSPClientConfigPage::defaults()
 */
struct LSPClientPluginOptions {
    bool m_symbolDetails = false;
    bool m_symbolTree = true;
    bool m_symbolExpand = true;
    bool m_symbolSort = false;

    bool m_showCompl = true;
    bool m_complDoc = true;
    bool m_refDeclaration = true;
    bool m_complParens = true;

    bool m_autoHover = true;
    bool m_onTypeFormatting = false;
    bool m_incrementalSync = false;
    bool m_highlightGoto = true;
    bool m_semanticHighlighting = true;
    bool m_signatureHelp = true;
    bool m_autoImport = true;
    bool m_fmtOnSave = false;
    bool m_inlayHints = false;
    bool m_highLightSymbol = true;

    bool m_diagnostics = true;
    bool m_messages = true;
};

class LSPClientPlugin : public KTextEditor::Plugin, public LSPClientPluginOptions
{
    Q_OBJECT

public:
    /**
     * The list of language IDs to disable the LSP support for.
     *
     * @note This property allows applications that have better native support for some
     *       programming languages to disable the LSP plugin for them. Set by KDevelop.
     */
    Q_PROPERTY(QStringList disabledLanguages MEMBER m_alwaysDisabledLanguages)

    explicit LSPClientPlugin(QObject *parent);
    ~LSPClientPlugin() override;

    QObject *createView(KTextEditor::MainWindow *mainWindow) override;

    int configPages() const override;
    KTextEditor::ConfigPage *configPage(int number = 0, QWidget *parent = nullptr) override;

    void writeConfig() const;

    // path for local setting files, auto-created on load
    const QString m_settingsPath;

    // default config path
    const QUrl m_defaultConfigPath;

    QStringList m_alwaysDisabledLanguages;

    QUrl m_configPath;

    // debug mode?
    const bool m_debugMode;

    // hash of allowed and blacklisted server command lines
    std::map<QString, bool> m_serverCommandLineToAllowedState;

    // current active dialogs to ask for permission of some command line
    std::set<QString> m_currentActiveCommandLineDialogs;

    // get current config path
    QUrl configPath() const
    {
        return m_configPath.isEmpty() ? m_defaultConfigPath : m_configPath;
    }

    /**
     * Check if given command line is allowed to be executed.
     * Might ask the user for permission.
     * @param cmdline full command line including program to check
     * @return execution allowed?
     */
    bool isCommandLineAllowed(const QStringList &cmdline);

Q_SIGNALS:
    // signal settings update
    void update() const;

    void showMessage(KTextEditor::Message::MessageType level, const QString &msg);

private Q_SLOTS:
    /**
     * Ask the user via dialog if the given command line shall be allowed.
     * Will store the result internally and trigger LSP server restart after config change.
     * Will ensure we just ask once, even if multiple requests queue up.
     * @param fullCommandLineString full command line string to get permission for
     */
    void askForCommandLinePermission(const QString &fullCommandLineString);

private:
    /**
     * Read from config and assign to data members.
     *
     * This function may be called only from the constructor because it assumes
     * that the LSPClientPluginOptions base of this object is default-initialized.
     */
    void readConfig();

    // server manager to pass along
    std::shared_ptr<LSPClientServerManager> m_serverManager;
};
