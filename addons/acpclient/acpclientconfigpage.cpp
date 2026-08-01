/*
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>
    SPDX-FileCopyrightText: 2026 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: MIT
*/

#include "acpclientconfigpage.h"
#include "acpclientplugin.h"
#include "jsonsettings.h"
#include "ui_acpconfigwidget.h"

#include <KLocalizedString>

#include <QVBoxLayout>

ACPClientConfigPage::ACPClientConfigPage(ACPClientPlugin *plugin, QWidget *parent)
    : KTextEditor::ConfigPage(parent)
    , m_plugin(plugin)
{
    ui = new Ui::ACPConfigWidget();
    ui->setupUi(this);
    ui->tabWidget->setDocumentMode(true);
    ui->tabWidget->tabBar()->setExpanding(true);

    m_jsonSettings = new JSONSettings(this, ui->tabWidget, QStringLiteral(":/kateacpclient/settings.json"), m_plugin->m_defaultConfigPath);

    connect(m_jsonSettings, &JSONSettings::configUrlChanged, this, &ACPClientConfigPage::changed);
    connect(m_jsonSettings, &JSONSettings::configChanged, this, &ACPClientConfigPage::changed);
    connect(m_jsonSettings, &JSONSettings::configSaved, m_plugin, &ACPClientPlugin::update);

    ui->tabWidget->setTabText(1, i18n("User Server Settings"));
    ui->tabWidget->setTabText(2, i18n("Default Server Settings"));

    reset();

    for (const auto &cb : {
             ui->autoStartCheckBox,
             ui->showNotificationsCheckBox,
             ui->showToolCallsCheckBox,
             ui->showProgressCheckBox,
             ui->debugModeCheckBox,
         }) {
        connect(cb, &QCheckBox::toggled, this, &ACPClientConfigPage::changed);
    }

    connect(ui->toolCallPermissionComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ACPClientConfigPage::changed);
}

ACPClientConfigPage::~ACPClientConfigPage()
{
    delete ui;
}

QString ACPClientConfigPage::name() const
{
    return i18n("ACP Client");
}

QString ACPClientConfigPage::fullName() const
{
    return i18n("Agent Client Protocol Client Configuration");
}

QIcon ACPClientConfigPage::icon() const
{
    return QIcon::fromTheme(QStringLiteral("utilities-terminal"));
}

void ACPClientConfigPage::apply()
{
    m_plugin->m_autoStartSession = ui->autoStartCheckBox->isChecked();
    m_plugin->m_showNotifications = ui->showNotificationsCheckBox->isChecked();
    m_plugin->m_showToolCalls = ui->showToolCallsCheckBox->isChecked();
    m_plugin->m_showProgress = ui->showProgressCheckBox->isChecked();
    m_plugin->m_debugMode = ui->debugModeCheckBox->isChecked();

    int permissionIndex = ui->toolCallPermissionComboBox->currentIndex();
    switch (permissionIndex) {
    case 1:
        m_plugin->m_toolCallPermission = ACPClientPluginOptions::AllowAll;
        break;
    case 2:
        m_plugin->m_toolCallPermission = ACPClientPluginOptions::DenyAll;
        break;
    case 0:
    default:
        m_plugin->m_toolCallPermission = ACPClientPluginOptions::AskEachTime;
        break;
    }

    m_plugin->m_configPath = m_jsonSettings->userConfigPath();

    m_jsonSettings->saveUserConfig();

    m_plugin->writeConfig();
}

void ACPClientConfigPage::resetUiTo()
{
    ui->autoStartCheckBox->setChecked(m_plugin->m_autoStartSession);
    ui->showNotificationsCheckBox->setChecked(m_plugin->m_showNotifications);
    ui->showToolCallsCheckBox->setChecked(m_plugin->m_showToolCalls);
    ui->showProgressCheckBox->setChecked(m_plugin->m_showProgress);
    ui->debugModeCheckBox->setChecked(m_plugin->m_debugMode);

    int permissionIndex = 0;
    switch (m_plugin->m_toolCallPermission) {
    case ACPClientPluginOptions::AllowAll:
        permissionIndex = 1;
        break;
    case ACPClientPluginOptions::DenyAll:
        permissionIndex = 2;
        break;
    case ACPClientPluginOptions::AskEachTime:
    default:
        permissionIndex = 0;
        break;
    }
    ui->toolCallPermissionComboBox->setCurrentIndex(permissionIndex);
}

void ACPClientConfigPage::reset()
{
    resetUiTo();

    m_jsonSettings->setConfigUrl(m_plugin->m_configPath);
    m_jsonSettings->readUserConfig();
}

void ACPClientConfigPage::defaults()
{
    resetUiTo();
}
