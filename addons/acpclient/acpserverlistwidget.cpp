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

    // Set up columns - Actions, Status, Name, Command
    QStringList headers;
    headers << i18n("Actions") << i18n("Status") << i18n("Name") << i18n("Command");
    m_serverTree->setColumnCount(headers.size());
    m_serverTree->setHeaderLabels(headers);

    // Resize columns to fit content
    m_serverTree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_serverTree->header()->setStretchLastSection(false);

    // Ensure the actions column has enough space for buttons
    m_serverTree->setColumnWidth(0, 200); // Actions column is first

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
        item->setText(0, QString()); // Column 0 is Actions (empty for this message)
        item->setText(1, QString()); // Column 1 is Status (empty for this message)
        item->setText(2, i18n("No servers available"));
        item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
        // Span across all columns
        for (int col = 3; col < m_serverTree->columnCount(); ++col) {
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

        // Column 0: Actions (buttons)
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
        m_serverTree->setItemWidget(item, 0, actionsWidget);

        // Column 1: Status
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
        item->setText(1, statusText);

        // Column 2: Name
        item->setText(2, serverName);

        // Column 3: Command & Arguments
        QString commandText = info.command;
        if (!info.arguments.isEmpty()) {
            commandText += QStringLiteral(" ") + info.arguments.join(QStringLiteral(" "));
        }
        item->setText(3, commandText);

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

        // Build tooltip
        QString toolTip = QStringLiteral("Name: ") + serverName;
        if (!commandText.isEmpty()) {
            toolTip += QStringLiteral("\nCommand: ") + commandText;
        }
        item->setToolTip(2, toolTip);
    }

    // Resize columns to fit content after loading
    // Skip column 0 (Actions) as it has a fixed width
    for (int col = 1; col < m_serverTree->columnCount(); ++col) {
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
