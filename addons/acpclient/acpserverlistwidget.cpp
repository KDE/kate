/*
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>
    SPDX-FileCopyrightText: 2026 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: MIT
*/

#include "acpserverlistwidget.h"
#include "acpclient_debug.h"
#include "acpclientserver.h"
#include "acpclientservermanager.h"

#include <KLocalizedString>

#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

ACPServerListWidget::ACPServerListWidget(ACPClientServerManager *serverManager, QWidget *parent)
    : QWidget(parent)
    , m_serverManager(serverManager)
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(4);

    m_serverTree = new QTreeWidget(this);
    m_serverTree->setObjectName(QStringLiteral("serverTree"));
    m_serverTree->setHeaderHidden(false);
    m_serverTree->setRootIsDecorated(false);
    m_serverTree->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_serverTree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_serverTree->setSortingEnabled(true);

    // Set up columns
    QStringList headers;
    headers << i18n("Name") << i18n("Version") << i18n("Command") << i18n("Status") << i18n("Actions");
    m_serverTree->setColumnCount(headers.size());
    m_serverTree->setHeaderLabels(headers);

    // Resize columns to fit content
    m_serverTree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_serverTree->header()->setStretchLastSection(false);

    // Ensure the actions column has enough space for buttons
    m_serverTree->setColumnWidth(4, 200); // Actions column

    m_layout->addWidget(m_serverTree);

    // Connect to server manager for server list updates
    connect(m_serverManager, &ACPClientServerManager::serverAdded, this, [this]() {
        updateServerList(m_serverManager->servers());
    });
    connect(m_serverManager, &ACPClientServerManager::serverRemoved, this, [this]() {
        updateServerList(m_serverManager->servers());
    });

    // Initial update
    updateServerList(m_serverManager->servers());
}

ACPServerListWidget::~ACPServerListWidget()
{
    // Clean up button connections
    for (auto &entry : m_serverButtons) {
        delete entry.first; // Start button
        delete entry.second; // Stop button
    }
    m_serverButtons.clear();
}

void ACPServerListWidget::updateServerList(const QList<ACPClientServer *> &servers)
{
    m_serverTree->clear();
    m_serverButtons.clear();

    if (servers.isEmpty()) {
        QTreeWidgetItem *item = new QTreeWidgetItem(m_serverTree);
        item->setText(0, i18n("No servers available"));
        item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
        // Span across all columns
        for (int col = 1; col < m_serverTree->columnCount(); ++col) {
            item->setText(col, QString());
        }
        return;
    }

    for (ACPClientServer *server : servers) {
        if (!server) {
            continue;
        }

        const ACPClientServer::ServerInfo &info = server->info();
        QString serverName = info.name;
        if (serverName.isEmpty()) {
            serverName = info.command;
        }

        QTreeWidgetItem *item = new QTreeWidgetItem(m_serverTree);
        // Column 0: Name
        item->setText(0, serverName);
        // Column 1: Version
        item->setText(1, info.version);
        // Column 2: Command
        item->setText(2, info.command);

        // Column 3: Status
        QString statusText;
        ACPClientServer::ServerState state = server->state();
        switch (state) {
        case ACPClientServer::ServerState::Disconnected:
            statusText = i18n("Disconnected");
            break;
        case ACPClientServer::ServerState::Connecting:
            statusText = i18n("Connecting...");
            break;
        case ACPClientServer::ServerState::Initializing:
            statusText = i18n("Initializing...");
            break;
        case ACPClientServer::ServerState::Initialized:
            statusText = i18n("Connected");
            break;
        case ACPClientServer::ServerState::Error:
            statusText = i18n("Error");
            break;
        default:
            statusText = i18n("Unknown");
            break;
        }
        item->setText(3, statusText);

        // Column 4: Actions (buttons)
        QWidget *actionsWidget = new QWidget(m_serverTree);
        QHBoxLayout *actionsLayout = new QHBoxLayout(actionsWidget);
        actionsLayout->setContentsMargins(2, 2, 2, 2);
        actionsLayout->setSpacing(4);

        QPushButton *startButton = new QPushButton(i18n("Start"), actionsWidget);
        startButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        QPushButton *stopButton = new QPushButton(i18n("Stop"), actionsWidget);
        stopButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

        actionsLayout->addWidget(startButton);
        actionsLayout->addWidget(stopButton);
        actionsLayout->addStretch();

        // Store buttons for state updates
        m_serverButtons[serverName] = qMakePair(startButton, stopButton);

        // Update button states based on server state
        updateButtonStates(serverName, state);

        // Connect button signals
        connect(startButton, &QPushButton::clicked, this, [this, server, serverName]() {
            qCDebug(ACPCLIENT) << "Start button clicked for server:" << serverName;
            if (server->state() == ACPClientServer::ServerState::Disconnected) {
                server->start();
            }
            Q_EMIT serverActivated(serverName);
        });

        connect(stopButton, &QPushButton::clicked, this, [this, server, serverName]() {
            qCDebug(ACPCLIENT) << "Stop button clicked for server:" << serverName;
            if (server->state() != ACPClientServer::ServerState::Disconnected) {
                server->stop();
            }
        });

        // Connect to server state changes
        connect(server, &ACPClientServer::stateChanged, this, [this, serverName]() {
            ACPClientServer *s = m_serverManager->server(serverName);
            if (s) {
                updateButtonStates(serverName, s->state());
            }
            updateServerList(m_serverManager->servers());
        });

        m_serverTree->setItemWidget(item, 4, actionsWidget);

        // Build tooltip
        QString toolTip = QStringLiteral("Name: ") + serverName;
        if (!info.version.isEmpty()) {
            toolTip += QStringLiteral("\nVersion: ") + info.version;
        }
        if (!info.command.isEmpty()) {
            toolTip += QStringLiteral("\nCommand: ") + info.command;
        }
        item->setToolTip(0, toolTip);
    }

    // Resize columns to fit content after loading
    for (int col = 0; col < m_serverTree->columnCount() - 1; ++col) {
        m_serverTree->resizeColumnToContents(col);
    }
}

void ACPServerListWidget::updateButtonStates(const QString &serverName, ACPClientServer::ServerState state)
{
    if (!m_serverButtons.contains(serverName)) {
        return;
    }

    QPair<QPushButton *, QPushButton *> &buttons = m_serverButtons[serverName];
    QPushButton *startButton = buttons.first;
    QPushButton *stopButton = buttons.second;

    if (!startButton || !stopButton) {
        return;
    }

    switch (state) {
    case ACPClientServer::ServerState::Disconnected:
        startButton->setEnabled(true);
        stopButton->setEnabled(false);
        break;
    case ACPClientServer::ServerState::Connecting:
        startButton->setEnabled(false);
        stopButton->setEnabled(true);
        break;
    case ACPClientServer::ServerState::Initializing:
        startButton->setEnabled(false);
        stopButton->setEnabled(true);
        break;
    case ACPClientServer::ServerState::Initialized:
        startButton->setEnabled(false);
        stopButton->setEnabled(true);
        break;
    case ACPClientServer::ServerState::Error:
        startButton->setEnabled(true);
        stopButton->setEnabled(true);
        break;
    default:
        startButton->setEnabled(true);
        stopButton->setEnabled(false);
        break;
    }
}

#include "moc_acpserverlistwidget.cpp"
