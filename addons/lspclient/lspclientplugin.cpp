/*
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>

    SPDX-License-Identifier: MIT
*/

#include "lspclientplugin.h"
#include "lspclientconfigpage.h"
#include "lspclientpluginview.h"
#include "lspclientservermanager.h"

#include "lspclient_debug.h"

#include <KConfigGroup>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KSharedConfig>

#include <QDir>
#include <QMessageBox>
#include <QStandardPaths>
#include <QTimer>

static const QString CONFIG_LSPCLIENT{QStringLiteral("lspclient")};
static const QString CONFIG_SYMBOL_DETAILS{QStringLiteral("SymbolDetails")};
static const QString CONFIG_SYMBOL_TREE{QStringLiteral("SymbolTree")};
static const QString CONFIG_SYMBOL_EXPAND{QStringLiteral("SymbolExpand")};
static const QString CONFIG_SYMBOL_SORT{QStringLiteral("SymbolSort")};
static const QString CONFIG_COMPLETION_DOC{QStringLiteral("CompletionDocumentation")};
static const QString CONFIG_REFERENCES_DECLARATION{QStringLiteral("ReferencesDeclaration")};
static const QString CONFIG_COMPLETION_PARENS{QStringLiteral("CompletionParens")};
static const QString CONFIG_AUTO_HOVER{QStringLiteral("AutoHover")};
static const QString CONFIG_TYPE_FORMATTING{QStringLiteral("TypeFormatting")};
static const QString CONFIG_INCREMENTAL_SYNC{QStringLiteral("IncrementalSync")};
static const QString CONFIG_HIGHLIGHT_GOTO{QStringLiteral("HighlightGoto")};
static const QString CONFIG_DIAGNOSTICS{QStringLiteral("Diagnostics")};
static const QString CONFIG_DIAGNOSTICS_HIGHLIGHT{QStringLiteral("DiagnosticsHighlight")};
static const QString CONFIG_DIAGNOSTICS_MARK{QStringLiteral("DiagnosticsMark")};
static const QString CONFIG_DIAGNOSTICS_HOVER{QStringLiteral("DiagnosticsHover")};
static const QString CONFIG_DIAGNOSTICS_SIZE{QStringLiteral("DiagnosticsSize")};
static const QString CONFIG_MESSAGES{QStringLiteral("Messages")};
static const QString CONFIG_SERVER_CONFIG{QStringLiteral("ServerConfiguration")};
static const QString CONFIG_SEMANTIC_HIGHLIGHTING{QStringLiteral("SemanticHighlighting")};
static const QString CONFIG_SIGNATURE_HELP{QStringLiteral("SignatureHelp")};
static const QString CONFIG_AUTO_IMPORT{QStringLiteral("AutoImport")};
static const QString CONFIG_ALLOWED_COMMANDS{QStringLiteral("AllowedServerCommandLines")};
static const QString CONFIG_BLOCKED_COMMANDS{QStringLiteral("BlockedServerCommandLines")};

K_PLUGIN_FACTORY_WITH_JSON(LSPClientPluginFactory, "lspclientplugin.json", registerPlugin<LSPClientPlugin>();)

/**
 * ensure we don't spam the user with debug output per-default
 */
static const bool debug = (qgetenv("LSPCLIENT_DEBUG") == QByteArray("1"));
static QLoggingCategory::CategoryFilter oldCategoryFilter = nullptr;
void myCategoryFilter(QLoggingCategory *category)
{
    // deactivate info and debug if not debug mode
    if (qstrcmp(category->categoryName(), "katelspclientplugin") == 0) {
        category->setEnabled(QtInfoMsg, debug);
        category->setEnabled(QtDebugMsg, debug);
    } else if (oldCategoryFilter) {
        oldCategoryFilter(category);
    }
}

LSPClientPlugin::LSPClientPlugin(QObject *parent, const QList<QVariant> &)
    : KTextEditor::Plugin(parent)
    , m_settingsPath(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + QStringLiteral("/lspclient"))
    , m_defaultConfigPath(QUrl::fromLocalFile(m_settingsPath + QStringLiteral("/settings.json")))
    , m_debugMode(debug)
{
    // ensure settings path exist, for e.g. local settings.json
    QDir().mkpath(m_settingsPath);

    // ensure we don't spam the user with debug messages per default, do this just once
    if (!oldCategoryFilter) {
        oldCategoryFilter = QLoggingCategory::installFilter(myCategoryFilter);
    }

    // apply our config
    readConfig();
}

LSPClientPlugin::~LSPClientPlugin()
{
}

QObject *LSPClientPlugin::createView(KTextEditor::MainWindow *mainWindow)
{
    if (!m_serverManager) {
        m_serverManager = LSPClientServerManager::new_(this);
    }
    return LSPClientPluginView::new_(this, mainWindow, m_serverManager);
}

int LSPClientPlugin::configPages() const
{
    return 1;
}

KTextEditor::ConfigPage *LSPClientPlugin::configPage(int number, QWidget *parent)
{
    if (number != 0) {
        return nullptr;
    }

    return new LSPClientConfigPage(parent, this);
}

void LSPClientPlugin::readConfig()
{
    KConfigGroup config(KSharedConfig::openConfig(), CONFIG_LSPCLIENT);
    m_symbolDetails = config.readEntry(CONFIG_SYMBOL_DETAILS, false);
    m_symbolTree = config.readEntry(CONFIG_SYMBOL_TREE, true);
    m_symbolExpand = config.readEntry(CONFIG_SYMBOL_EXPAND, true);
    m_symbolSort = config.readEntry(CONFIG_SYMBOL_SORT, false);
    m_complDoc = config.readEntry(CONFIG_COMPLETION_DOC, true);
    m_refDeclaration = config.readEntry(CONFIG_REFERENCES_DECLARATION, true);
    m_complParens = config.readEntry(CONFIG_COMPLETION_PARENS, true);
    m_autoHover = config.readEntry(CONFIG_AUTO_HOVER, true);
    m_onTypeFormatting = config.readEntry(CONFIG_TYPE_FORMATTING, false);
    m_incrementalSync = config.readEntry(CONFIG_INCREMENTAL_SYNC, false);
    m_highlightGoto = config.readEntry(CONFIG_HIGHLIGHT_GOTO, true);
    m_diagnostics = config.readEntry(CONFIG_DIAGNOSTICS, true);
    m_diagnosticsHighlight = config.readEntry(CONFIG_DIAGNOSTICS_HIGHLIGHT, true);
    m_diagnosticsMark = config.readEntry(CONFIG_DIAGNOSTICS_MARK, true);
    m_diagnosticsHover = config.readEntry(CONFIG_DIAGNOSTICS_HOVER, true);
    m_diagnosticsSize = config.readEntry(CONFIG_DIAGNOSTICS_SIZE, 1024);
    m_messages = config.readEntry(CONFIG_MESSAGES, true);
    m_configPath = config.readEntry(CONFIG_SERVER_CONFIG, QUrl());
    m_semanticHighlighting = config.readEntry(CONFIG_SEMANTIC_HIGHLIGHTING, true);
    m_signatureHelp = config.readEntry(CONFIG_SIGNATURE_HELP, true);
    m_autoImport = config.readEntry(CONFIG_AUTO_IMPORT, true);

    // read allow + block lists as two separate keys, let block always win
    const auto allowed = config.readEntry(CONFIG_ALLOWED_COMMANDS, QStringList());
    const auto blocked = config.readEntry(CONFIG_BLOCKED_COMMANDS, QStringList());
    m_serverCommandLineToAllowedState.clear();
    for (const auto &cmd : allowed) {
        m_serverCommandLineToAllowedState[cmd] = true;
    }
    for (const auto &cmd : blocked) {
        m_serverCommandLineToAllowedState[cmd] = false;
    }

    Q_EMIT update();
}

void LSPClientPlugin::writeConfig() const
{
    KConfigGroup config(KSharedConfig::openConfig(), CONFIG_LSPCLIENT);
    config.writeEntry(CONFIG_SYMBOL_DETAILS, m_symbolDetails);
    config.writeEntry(CONFIG_SYMBOL_TREE, m_symbolTree);
    config.writeEntry(CONFIG_SYMBOL_EXPAND, m_symbolExpand);
    config.writeEntry(CONFIG_SYMBOL_SORT, m_symbolSort);
    config.writeEntry(CONFIG_COMPLETION_DOC, m_complDoc);
    config.writeEntry(CONFIG_REFERENCES_DECLARATION, m_refDeclaration);
    config.writeEntry(CONFIG_COMPLETION_PARENS, m_complParens);
    config.writeEntry(CONFIG_AUTO_HOVER, m_autoHover);
    config.writeEntry(CONFIG_TYPE_FORMATTING, m_onTypeFormatting);
    config.writeEntry(CONFIG_INCREMENTAL_SYNC, m_incrementalSync);
    config.writeEntry(CONFIG_HIGHLIGHT_GOTO, m_highlightGoto);
    config.writeEntry(CONFIG_DIAGNOSTICS, m_diagnostics);
    config.writeEntry(CONFIG_DIAGNOSTICS_HIGHLIGHT, m_diagnosticsHighlight);
    config.writeEntry(CONFIG_DIAGNOSTICS_MARK, m_diagnosticsMark);
    config.writeEntry(CONFIG_DIAGNOSTICS_HOVER, m_diagnosticsHover);
    config.writeEntry(CONFIG_DIAGNOSTICS_SIZE, m_diagnosticsSize);
    config.writeEntry(CONFIG_MESSAGES, m_messages);
    config.writeEntry(CONFIG_SERVER_CONFIG, m_configPath);
    config.writeEntry(CONFIG_SEMANTIC_HIGHLIGHTING, m_semanticHighlighting);
    config.writeEntry(CONFIG_SIGNATURE_HELP, m_signatureHelp);
    config.writeEntry(CONFIG_AUTO_IMPORT, m_autoImport);

    // write allow + block lists as two separate keys
    QStringList allowed, blocked;
    for (const auto &it : m_serverCommandLineToAllowedState) {
        if (it.second) {
            allowed.push_back(it.first);
        } else {
            blocked.push_back(it.first);
        }
    }
    config.writeEntry(CONFIG_ALLOWED_COMMANDS, allowed);
    config.writeEntry(CONFIG_BLOCKED_COMMANDS, blocked);

    Q_EMIT update();
}

bool LSPClientPlugin::isCommandLineAllowed(const QStringList &cmdline)
{
    // check our allow list
    // if we already have stored some value, perfect, just use that one
    const QString fullCommandLineString = cmdline.join(QStringLiteral(" "));
    if (const auto it = m_serverCommandLineToAllowedState.find(fullCommandLineString); it != m_serverCommandLineToAllowedState.end()) {
        return it->second;
    }

    // else: queue asking the user async, will ensure we don't have duplicate dialogs
    // tell the launching it is forbidden ATM
    QTimer::singleShot(0, this, [this, fullCommandLineString]() {
        askForCommandLinePermission(fullCommandLineString);
    });
    return false;
}

void LSPClientPlugin::askForCommandLinePermission(const QString &fullCommandLineString)
{
    // did we already store a new result for the given command line? just use that
    if (const auto it = m_serverCommandLineToAllowedState.find(fullCommandLineString); it != m_serverCommandLineToAllowedState.end()) {
        // we need to trigger config update if the command is not blocked any longer
        if (it->second) {
            Q_EMIT update();
        }
        return;
    }

    // is this command line request already open? => skip the new request, dialog is already there
    if (!m_currentActiveCommandLineDialogs.insert(fullCommandLineString).second) {
        return;
    }

    // ask user if the start should be allowed
    QPointer<QMessageBox> msgBox(new QMessageBox());
    msgBox->setWindowTitle(i18n("LSP server start requested"));
    msgBox->setTextFormat(Qt::RichText);
    msgBox->setText(
        i18n("Do you want the LSP server to be started?<br><br>The full command line is:<br><br><b>%1</b><br><br>The choice can be altered via the config page "
             "of the plugin.",
             fullCommandLineString.toHtmlEscaped()));
    msgBox->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox->setDefaultButton(QMessageBox::Yes);
    const bool allowed = (msgBox->exec() == QMessageBox::Yes);

    // store new configured value
    m_serverCommandLineToAllowedState.emplace(fullCommandLineString, allowed);

    // inform the user if it was forbidden! do this here to just emit this once
    if (!allowed) {
        Q_EMIT showMessage(KTextEditor::Message::Information,
                           i18n("User permanently blocked start of: '%1'.\nUse the config page of the plugin to undo this block.", fullCommandLineString));
    }

    // purge dialog entry
    m_currentActiveCommandLineDialogs.erase(fullCommandLineString);

    // flush config, will trigger update()
    writeConfig();
}

#include "lspclientplugin.moc"
