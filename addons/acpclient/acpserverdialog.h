/*
    SPDX-FileCopyrightText: 2026 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: MIT
*/

#pragma once

#include "acpclientserver.h"
#include <QDialog>

namespace Ui
{
class ACPServerDialog;
}

class ACPServerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ACPServerDialog(QWidget *parent = nullptr, const ACPClientServer::ServerInfo &info = ACPClientServer::ServerInfo());
    ~ACPServerDialog() override;

    ACPClientServer::ServerInfo serverInfo() const;
    void setServerInfo(const ACPClientServer::ServerInfo &info);

private Q_SLOTS:
    void applyPreset();
    void accept() override;

private:
    void loadPresets();
    void loadPreset(const QString &presetName);

    Ui::ACPServerDialog *m_ui;
    QList<ACPClientServer::ServerInfo> m_presets;
};