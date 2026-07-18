/*
    SPDX-FileCopyrightText: 2026

    SPDX-License-Identifier: MIT
*/

#include "acpserverdialog.h"
#include "acpclient_debug.h"

#include "ui_acpserverdialog.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>

ACPServerDialog::ACPServerDialog(QWidget *parent, const ACPClientServer::ServerInfo &info)
    : QDialog(parent)
    , m_ui(new Ui::ACPServerDialog)
{
    qCDebug(ACPCLIENT) << "ACPServerDialog created";

    m_ui->setupUi(this);

    // Load presets
    loadPresets();

    // Set initial info
    setServerInfo(info);

    // Connect signals
    connect(m_ui->applyPresetButton, &QPushButton::clicked, this, &ACPServerDialog::applyPreset);
}

ACPServerDialog::~ACPServerDialog()
{
    delete m_ui;
}

ACPClientServer::ServerInfo ACPServerDialog::serverInfo() const
{
    ACPClientServer::ServerInfo info;

    info.name = m_ui->nameLineEdit->text();
    info.command = m_ui->commandLineEdit->text();
    info.arguments = m_ui->argumentsLineEdit->text().split(QLatin1Char(' '), Qt::SkipEmptyParts);
    info.autoStart = m_ui->autoStartCheckBox->isChecked();

    return info;
}

void ACPServerDialog::setServerInfo(const ACPClientServer::ServerInfo &info)
{
    m_ui->nameLineEdit->setText(info.name);
    m_ui->commandLineEdit->setText(info.command);
    m_ui->argumentsLineEdit->setText(info.arguments.join(QLatin1Char(' ')));
    m_ui->autoStartCheckBox->setChecked(info.autoStart);

    // Select custom preset
    m_ui->presetComboBox->setCurrentIndex(0);
}

void ACPServerDialog::loadPresets()
{
    qCDebug(ACPCLIENT) << "Loading ACP server presets";

    // Load from settings.json resource
    QFile settingsFile(QStringLiteral(":/kateacpclient/settings.json"));
    if (settingsFile.exists() && settingsFile.open(QIODevice::ReadOnly)) {
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(settingsFile.readAll(), &parseError);

        if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
            QJsonObject root = doc.object();

            if (root.contains(u"servers") && root[u"servers"].isArray()) {
                QJsonArray servers = root[u"servers"].toArray();

                for (const QJsonValue &serverValue : servers) {
                    if (serverValue.isObject()) {
                        QJsonObject serverObj = serverValue.toObject();
                        ACPClientServer::ServerInfo info;

                        info.name = serverObj[u"name"].toString();
                        info.command = serverObj[u"command"].toString();
                        info.autoStart = serverObj[u"auto_start"].toBool(false);

                        if (serverObj.contains(u"arguments") && serverObj[u"arguments"].isArray()) {
                            QJsonArray args = serverObj[u"arguments"].toArray();
                            for (const QJsonValue &arg : args) {
                                info.arguments.append(arg.toString());
                            }
                        }

                        if (serverObj.contains(u"metadata") && serverObj[u"metadata"].isObject()) {
                            info.metadata = serverObj[u"metadata"].toObject();
                        }

                        m_presets.append(info);
                        m_ui->presetComboBox->addItem(info.name);
                    }
                }
            }
        }
        settingsFile.close();
    }
}

void ACPServerDialog::loadPreset(const QString &presetName)
{
    qCDebug(ACPCLIENT) << "Loading preset:" << presetName;

    for (const ACPClientServer::ServerInfo &preset : m_presets) {
        if (preset.name == presetName) {
            setServerInfo(preset);
            return;
        }
    }
}

void ACPServerDialog::applyPreset()
{
    qCDebug(ACPCLIENT) << "Applying preset";
    loadPreset(m_ui->presetComboBox->currentText());
}

void ACPServerDialog::accept()
{
    qCDebug(ACPCLIENT) << "Dialog accepted";

    // Validate inputs
    if (m_ui->nameLineEdit->text().isEmpty()) {
        QMessageBox::warning(this, i18n("Invalid Configuration"), i18n("Please enter a server name."));
        return;
    }

    if (m_ui->commandLineEdit->text().isEmpty()) {
        QMessageBox::warning(this, i18n("Invalid Configuration"), i18n("Please enter a command."));
        return;
    }

    QDialog::accept();
}
