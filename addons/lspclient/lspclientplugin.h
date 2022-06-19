/*
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>

    SPDX-License-Identifier: MIT
*/

#ifndef LSPCLIENTPLUGIN_H
#define LSPCLIENTPLUGIN_H

#include <QUrl>
#include <QVariant>

#include <KTextEditor/Message>
#include <KTextEditor/Plugin>

#include <map>
#include <set>

class LSPClientServerManager;

class LSPClientPlugin : public KTextEditor::Plugin
{
    Q_OBJECT

public:
    explicit LSPClientPlugin(QObject *parent = nullptr, const QList<QVariant> & = QList<QVariant>());
    ~LSPClientPlugin() override;

    QObject *createView(KTextEditor::MainWindow *mainWindow) override;

    int configPages() const override;
    KTextEditor::ConfigPage *configPage(int number = 0, QWidget *parent = nullptr) override;

    void readConfig();
    void writeConfig() const;

    // path for local setting files, auto-created on load
    const QString m_settingsPath;

    // default config path
    const QUrl m_defaultConfigPath;

    // settings
    bool m_symbolDetails = false;
    bool m_symbolExpand = false;
    bool m_symbolTree = false;
    bool m_symbolSort = false;
    bool m_complDoc = false;
    bool m_refDeclaration = false;
    bool m_complParens = false;
    bool m_diagnostics = false;
    bool m_diagnosticsHighlight = false;
    bool m_diagnosticsMark = false;
    bool m_diagnosticsHover = false;
    unsigned m_diagnosticsSize = 0;
    bool m_messages = false;
    bool m_autoHover = false;
    bool m_onTypeFormatting = false;
    bool m_incrementalSync = false;
    bool m_highlightGoto = true;
    QUrl m_configPath;
    bool m_semanticHighlighting = false;
    bool m_signatureHelp = true;
    bool m_autoImport = true;

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
    // server manager to pass along
    QSharedPointer<LSPClientServerManager> m_serverManager;
};

#endif
