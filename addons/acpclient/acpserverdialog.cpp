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
    connect(m_ui->connectionTypeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ACPServerDialog::updatePortVisibility);
    connect(m_ui->applyPresetButton, &QPushButton::clicked, this, &ACPServerDialog::applyPreset);

    // Update port visibility based on connection type
    updatePortVisibility(m_ui->connectionTypeComboBox->currentIndex());
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
    info.arguments = m_ui->argumentsLineEdit->text().split(QChar(' '), Qt::SkipEmptyParts);
    info.port = m_ui->portSpinBox->value();
    info.autoStart = m_ui->autoStartCheckBox->isChecked();

    // Set connection type
    switch (m_ui->connectionTypeComboBox->currentIndex()) {
    default:
    case 0:
        info.connectionType = ACPClientServer::ConnectionType::StdIO;
        break;
    case 1:
        info.connectionType = ACPClientServer::ConnectionType::WebSocket;
        break;
    case 2:
        info.connectionType = ACPClientServer::ConnectionType::TcpSocket;
        break;
    }

    // For network connections, use host from command field
    if (info.connectionType != ACPClientServer::ConnectionType::StdIO) {
        info.host = m_ui->commandLineEdit->text();
    }

    return info;
}

void ACPServerDialog::setServerInfo(const ACPClientServer::ServerInfo &info)
{
    m_ui->nameLineEdit->setText(info.name);

    if (info.connectionType == ACPClientServer::ConnectionType::StdIO) {
        m_ui->connectionTypeComboBox->setCurrentIndex(0);
        m_ui->commandLineEdit->setText(info.command);
        m_ui->argumentsLineEdit->setText(info.arguments.join(QChar(' ')));
    } else if (info.connectionType == ACPClientServer::ConnectionType::WebSocket) {
        m_ui->connectionTypeComboBox->setCurrentIndex(1);
        m_ui->commandLineEdit->setText(info.host);
        m_ui->portSpinBox->setValue(info.port > 0 ? info.port : 8080);
    } else {
        m_ui->connectionTypeComboBox->setCurrentIndex(2);
        m_ui->commandLineEdit->setText(info.host);
        m_ui->portSpinBox->setValue(info.port > 0 ? info.port : 8080);
    }

    m_ui->autoStartCheckBox->setChecked(info.autoStart);

    // Select custom preset
    m_ui->presetComboBox->setCurrentIndex(0);

    updatePortVisibility(m_ui->connectionTypeComboBox->currentIndex());
}

void ACPServerDialog::updatePortVisibility(int index)
{
    bool showPort = (index == 1 || index == 2); // WebSocket or TCP
    m_ui->portSpinBox->setEnabled(showPort);
    m_ui->label_5->setEnabled(showPort);

    // Update label text
    if (index == 0) {
        m_ui->label_3->setText(i18n("Command:"));
        m_ui->commandLineEdit->setPlaceholderText(i18n("/path/to/acp-agent"));
    } else {
        m_ui->label_3->setText(i18n("Hostname/IP:"));
        m_ui->commandLineEdit->setPlaceholderText(i18n("localhost or 127.0.0.1"));
    }
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

                info.name = obj["name"].toString();
                info.command = obj["command"].toString();
                info.autoStart = obj["auto_start"].toBool(false);

                if (obj.contains("arguments") && obj["arguments"].isArray()) {
                    QJsonArray args = obj["arguments"].toArray();
                    for (const QJsonValue &arg : args) {
                        info.arguments.append(arg.toString());
                    }
                }

                QString connectionType = obj["connection_type"].toString();
                if (connectionType == "websocket") {
                    info.connectionType = ACPClientServer::ConnectionType::WebSocket;
                } else if (connectionType == "tcp") {
                    info.connectionType = ACPClientServer::ConnectionType::TcpSocket;
                } else {
                    info.connectionType = ACPClientServer::ConnectionType::StdIO;
                }

                info.host = obj["host"].toString();
                info.port = obj["port"].toInt();

                if (obj.contains("metadata") && obj["metadata"].isObject()) {
                    info.metadata = obj["metadata"].toObject();
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
        QMessageBox::warning(this, i18n("Invalid Configuration"), i18n("Please enter a command or hostname."));
        return;
    }

    QDialog::accept();
}

#include "moc_acpserverdialog.cpp"