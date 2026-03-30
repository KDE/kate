/*
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>
    SPDX-FileCopyrightText: 2023 Akseli Lahtinen <akselmo@akselmo.dev>

    SPDX-License-Identifier: MIT
*/

#include "debugconfigpage.h"
#include "jsonsettings.h"
#include "plugin_kategdb.h"

#include <KLocalizedString>

#include <KSyntaxHighlighting/Definition>
#include <KSyntaxHighlighting/Repository>
#include <KSyntaxHighlighting/SyntaxHighlighter>
#include <KSyntaxHighlighting/Theme>

#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/View>

#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QMenu>
#include <QPalette>
#include <QTabWidget>

#include <ktexteditor_utils.h>

DebugConfigPage::DebugConfigPage(QWidget *parent, KatePluginGDB *plugin)
    : KTextEditor::ConfigPage(parent)
    , m_plugin(plugin)
{
    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins({});
    auto *tabWidget = new QTabWidget(this);
    tabWidget->setDocumentMode(true);
    tabWidget->setContentsMargins({});
    layout->addWidget(tabWidget);

    m_jsonSettings = new JSONSettings(this, tabWidget, QStringLiteral(":/debugger/dap.json"), m_plugin->m_defaultConfigPath);

    connect(m_jsonSettings, &JSONSettings::configUrlChanged, this, &DebugConfigPage::changed);

    connect(m_jsonSettings, &JSONSettings::configChanged, this, &DebugConfigPage::changed);
    connect(m_jsonSettings, &JSONSettings::configSaved, m_plugin, &KatePluginGDB::update);

    reset();
}

QString DebugConfigPage::name() const
{
    return QString(i18n("Debugger"));
}

QString DebugConfigPage::fullName() const
{
    return QString(i18n("Debugger"));
}

QIcon DebugConfigPage::icon() const
{
    return QIcon::fromTheme(QStringLiteral("debug-run"));
}

void DebugConfigPage::apply()
{
    m_plugin->m_configPath = m_jsonSettings->userConfigPath();

    m_jsonSettings->saveUserConfig();

    m_plugin->writeConfig();
}

void DebugConfigPage::reset()
{
    m_jsonSettings->setConfigUrl(m_plugin->m_configPath);
    m_jsonSettings->readUserConfig();
}

void DebugConfigPage::defaults()
{
    reset();
}
