/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "FormatConfig.h"

#include "FormatPlugin.h"
#include "jsonsettings.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFile>
#include <QGroupBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>

#include <KConfigGroup>
#include <KSharedConfig>
#include <KSyntaxHighlighting/Definition>
#include <KSyntaxHighlighting/Repository>
#include <KSyntaxHighlighting/SyntaxHighlighter>
#include <KTextEditor/Editor>

FormatConfigPage::FormatConfigPage(class FormatPlugin *plugin, QWidget *parent)
    : KTextEditor::ConfigPage(parent)
    , m_plugin(plugin)
{
    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins({});
    auto *tabWidget = new QTabWidget(this);
    tabWidget->setDocumentMode(true);
    tabWidget->setContentsMargins({});
    layout->addWidget(tabWidget);

    m_jsonSettings = new JSONSettings(this, tabWidget, QStringLiteral(":/formatting/FormatterSettings.json"), m_plugin->m_defaultConfigPath);

    connect(m_jsonSettings, &JSONSettings::configUrlChanged, this, &FormatConfigPage::changed);

    connect(m_jsonSettings, &JSONSettings::configChanged, this, &FormatConfigPage::changed);
    connect(m_jsonSettings, &JSONSettings::configSaved, m_plugin, &FormatPlugin::readConfig);

    reset();
}

void FormatConfigPage::apply()
{
    m_jsonSettings->saveUserConfig();
    m_plugin->m_configPath = m_jsonSettings->userConfigPath();
    m_plugin->configChanged();
}

void FormatConfigPage::reset()
{
    m_jsonSettings->setConfigUrl(m_plugin->m_configPath);
    m_jsonSettings->readUserConfig();
}
