/*
    SPDX-FileCopyrightText: 2026

    SPDX-License-Identifier: MIT
*/

#pragma once

#include <KTextEditor/ConfigPage>
#include <QWidget>

#include "acpclientserver.h"

#include <memory>

class ACPClientPlugin;
class QVBoxLayout;
class QPushButton;

namespace Ui
{
class ACPConfigWidget;
}

class ACPServerDialog;

class ACPClientConfigPage : public KTextEditor::ConfigPage
{
    Q_OBJECT

public:
    explicit ACPClientConfigPage(ACPClientPlugin *plugin, QWidget *parent = nullptr);
    ~ACPClientConfigPage() override;

    QString name() const override;
    QString fullName() const override;
    QIcon icon() const override;

    void apply() override;
    void defaults() override;
    void reset() override;

private Q_SLOTS:
    void addServer();
    void removeServer();
    void editServer();
    void serverSelected();
    void loadDefaultServers();
    void saveServersConfig();

private:
    void loadConfig();
    void saveConfig();
    void updateServerList();

    ACPClientPlugin *m_plugin;
    Ui::ACPConfigWidget *m_ui;
    QList<ACPClientServer::ServerInfo> m_servers;
};
