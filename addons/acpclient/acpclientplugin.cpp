/*
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>
    SPDX-FileCopyrightText: 2026 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: MIT
*/

#include "acpclientplugin.h"
#include "acpclient_debug.h"
#include "acpclientconfigpage.h"
#include "acpclientpluginview.h"
#include "acpclientservermanager.h"

#include <KConfigGroup>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KSharedConfig>

#include <QApplication>
#include <QDir>
#include <QJsonObject>
#include <QMessageBox>
#include <QStandardPaths>
#include <QTimer>
#include <memory>

QString acpClientConfigGroup()
{
    return QStringLiteral("acpclient");
}

static constexpr char CONFIG_SERVER_CONFIG[] = "ServerConfiguration";
static constexpr char CONFIG_TOOL_CALL_PERMISSION[] = "ToolCallPermission";

K_PLUGIN_FACTORY_WITH_JSON(ACPClientPluginFactory, "acpclientplugin.json", registerPlugin<ACPClientPlugin>();)

static const bool debug = (qEnvironmentVariableIntValue("ACPCLIENT_DEBUG") == 1);
static QLoggingCategory::CategoryFilter oldCategoryFilter = nullptr;
static void myCategoryFilter(QLoggingCategory *category)
{
    // Deactivate info and debug if not debug mode
    if (qstrcmp(category->categoryName(), "kateacpclientplugin") == 0) {
        category->setEnabled(QtInfoMsg, debug);
        category->setEnabled(QtDebugMsg, debug);
    } else if (oldCategoryFilter) {
        oldCategoryFilter(category);
    }
}

ACPClientPlugin::ACPClientPlugin(QObject *parent)
    : KTextEditor::Plugin(parent)
    , m_settingsPath(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + QStringLiteral("/acpclient"))
    , m_defaultConfigPath(QUrl::fromLocalFile(m_settingsPath + QStringLiteral("/settings.json")))
{
    qCDebug(ACPCLIENT) << "ACPClientPlugin created";

    // Ensure settings path exists
    QDir().mkpath(m_settingsPath);

    // Ensure we don't spam the user with debug messages per default
    if (!oldCategoryFilter) {
        oldCategoryFilter = QLoggingCategory::installFilter(myCategoryFilter);
    }

    // Apply our config
    readConfig();
}

ACPClientPlugin::~ACPClientPlugin() = default;

QObject *ACPClientPlugin::createView(KTextEditor::MainWindow *mainWindow)
{
    qCDebug(ACPCLIENT) << "Creating view for main window";

    if (!m_serverManager) {
        m_serverManager = std::shared_ptr<ACPClientServerManager>(ACPClientServerManager::new_(this, this));
    }

    auto view = new ACPClientPluginView(this, mainWindow, m_serverManager);
    m_views.append(view);

    connect(this, &ACPClientPlugin::showMessage, mainWindow, [](KTextEditor::Message::MessageType level, const QString &msg) {
        KTextEditor::Message *message = new KTextEditor::Message(msg, level);
        message->setPosition(KTextEditor::Message::BottomInView);
        message->setWordWrap(true);
        message->setAutoHide(5000);
        // message->show(); FIXME
    });

    return view;
}

int ACPClientPlugin::configPages() const
{
    return 1; // We have one config page
}

KTextEditor::ConfigPage *ACPClientPlugin::configPage(int number, QWidget *parent)
{
    if (number != 0) {
        return nullptr;
    }
    return new ACPClientConfigPage(this, parent);
}

void ACPClientPlugin::writeConfig() const
{
    KConfigGroup config(KSharedConfig::openConfig(), acpClientConfigGroup());
    config.writeEntry(CONFIG_SERVER_CONFIG, m_configPath);
    config.writeEntry(CONFIG_TOOL_CALL_PERMISSION, static_cast<int>(m_toolCallPermission));
}

void ACPClientPlugin::readConfig()
{
    KConfigGroup config(KSharedConfig::openConfig(), acpClientConfigGroup());
    m_configPath = config.readEntry(CONFIG_SERVER_CONFIG, QUrl());
    m_toolCallPermission = static_cast<ToolCallPermission>(config.readEntry(CONFIG_TOOL_CALL_PERMISSION, static_cast<int>(AskEachTime)));
}

#include "acpclientplugin.moc"
