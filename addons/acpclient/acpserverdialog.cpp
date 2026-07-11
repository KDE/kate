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

    // Load from resources
    QStringList presetFiles = {QStringLiteral(":/kateacpclient/agents/vibe-acp.json"),
                               QStringLiteral(":/kateacpclient/agents/claude-code.json"),
                               QStringLiteral(":/kateacpclient/agents/cursor.json")};

    for (const QString &presetFile : presetFiles) {
        QFile file(presetFile);
        if (file.exists() && file.open(QIODevice::ReadOnly)) {
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);

            if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
                ACPClientServer::ServerInfo info;
                QJsonObject obj = doc.object();

                info.name = obj[u"name"].toString();
                info.command = obj[u"command"].toString();
                info.autoStart = obj[u"auto_start"].toBool(false);

                if (obj.contains(u"arguments") && obj[u"arguments"].isArray()) {
                    QJsonArray args = obj[u"arguments"].toArray();
                    for (const QJsonValue &arg : args) {
                        info.arguments.append(arg.toString());
                    }
                }

                if (obj.contains(u"metadata") && obj[u"metadata"].isObject()) {
                    info.metadata = obj[u"metadata"].toObject();
                }

                m_presets.append(info);
                m_ui->presetComboBox->addItem(info.name);
            }
            file.close();
        }
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
