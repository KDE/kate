/*
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>
    SPDX-FileCopyrightText: 2026 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: MIT
*/

#pragma once

#include <QWidget>

#include "acpclientserver.h"

class QTreeWidget;
class QTreeWidgetItem;
class QVBoxLayout;
class QPushButton;
class QHBoxLayout;
class ACPClientServerManager;

/**
 * @class ACPServerListWidget
 * @brief Widget for displaying and managing the list of ACP servers
 *
 * This widget shows configured ACP servers and allows the user to:
 * - View all configured servers
 * - See their current state (connected/disconnected)
 * - Start and stop servers manually
 *
 * It connects to the server manager to receive server state updates.
 */
class ACPServerListWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Construct the server list widget
     * @param serverManager Server manager to query for servers
     * @param parent Qt parent widget
     */
    explicit ACPServerListWidget(ACPClientServerManager *serverManager, QWidget *parent = nullptr);

    /** @brief Destructor */
    ~ACPServerListWidget() override;

    /**
     * @brief Update the server list
     * @param servers List of server pointers
     */
    void updateServerList(const QList<ACPClientServer *> &servers);

Q_SIGNALS:
    /**
     * @brief Emitted when a server should be activated
     * @param serverName Name of the server to activate
     */
    void serverActivated(const QString &serverName);

private:
    /** @brief Update button states based on server state */
    void updateButtonStates(const QString &serverName, ACPClientServer::ServerState state);

    ACPClientServerManager *m_serverManager = nullptr;
    QTreeWidget *m_serverTree = nullptr;
    QVBoxLayout *m_layout = nullptr;
    QMap<QString, QPair<QPushButton *, QPushButton *>> m_serverButtons; ///< server name -> (startBtn, stopBtn)
};
