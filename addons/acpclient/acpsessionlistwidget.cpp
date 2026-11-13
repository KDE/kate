/*
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>
    SPDX-FileCopyrightText: 2026 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: MIT
*/

#include "acpsessionlistwidget.h"
#include "acpclient_debug.h"

#include <KLocalizedString>

#include <QDateTime>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonObject>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

ACPSessionListWidget::ACPSessionListWidget(ACPClientServerManager *serverManager, QWidget *parent)
    : QWidget(parent)
    , m_serverManager(serverManager)
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);

    // Create header with reload button
    QWidget *headerWidget = new QWidget(this);
    QHBoxLayout *headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(4, 4, 4, 4);
    headerLayout->setSpacing(4);

    QLabel *headerLabel = new QLabel(i18n("Sessions"), headerWidget);
    QFont boldFont = headerLabel->font();
    boldFont.setBold(true);
    headerLabel->setFont(boldFont);
    headerLayout->addWidget(headerLabel);
    headerLayout->addStretch();

    m_newSessionButton = new QPushButton(headerWidget);
    m_newSessionButton->setIcon(QIcon::fromTheme(QStringLiteral("document-new")));
    m_newSessionButton->setToolTip(i18n("New Session"));
    m_newSessionButton->setFlat(true);
    headerLayout->addWidget(m_newSessionButton);

    m_reloadButton = new QPushButton(headerWidget);
    m_reloadButton->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh")));
    m_reloadButton->setToolTip(i18n("Reload session list"));
    m_reloadButton->setFlat(true);
    headerLayout->addWidget(m_reloadButton);

    m_layout->addWidget(headerWidget);

    m_sessionTree = new QTreeWidget(this);
    m_sessionTree->setObjectName(QStringLiteral("sessionTree"));
    m_sessionTree->setHeaderHidden(false);
    m_sessionTree->setRootIsDecorated(false);
    m_sessionTree->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_sessionTree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_sessionTree->setSortingEnabled(true);
    m_sessionTree->setContextMenuPolicy(Qt::CustomContextMenu);

    // Set up columns
    QStringList headers;
    headers << i18n("Updated") << i18n("Title") << i18n("Working Directory");
    m_sessionTree->setColumnCount(headers.size());
    m_sessionTree->setHeaderLabels(headers);

    // Resize columns to fit content
    m_sessionTree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

    m_layout->addWidget(m_sessionTree);

    // Connect double-click to resume session
    connect(m_sessionTree, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem *item, int column) {
        Q_UNUSED(column);
        QString sessionId = item->data(0, Qt::UserRole).toString();
        qCDebug(ACPCLIENT) << "Session double-clicked:" << sessionId;
        if (!sessionId.isEmpty()) {
            Q_EMIT sessionResumed(sessionId);
        }
    });

    // Connect custom context menu request
    connect(m_sessionTree, &QTreeWidget::customContextMenuRequested, this, &ACPSessionListWidget::showContextMenu);

    // Connect reload button
    connect(m_reloadButton, &QPushButton::clicked, this, [this]() {
        Q_EMIT reloadRequested();
    });

    // Connect new session button
    connect(m_newSessionButton, &QPushButton::clicked, this, [this]() {
        Q_EMIT newSessionRequested();
    });
}

ACPSessionListWidget::~ACPSessionListWidget()
{
}

void ACPSessionListWidget::updateSessionList(const QJsonArray &sessions)
{
    m_sessionTree->clear();

    if (sessions.isEmpty()) {
        QTreeWidgetItem *item = new QTreeWidgetItem(m_sessionTree);
        item->setText(0, i18n("No sessions available"));
        item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
        // Span across all columns
        for (int col = 1; col < m_sessionTree->columnCount(); ++col) {
            item->setText(col, QString());
        }
        return;
    }

    // Sort sessions by updatedAt date (newest first) if available
    QList<QJsonObject> sessionList;
    for (const QJsonValue &value : sessions) {
        if (value.isObject()) {
            sessionList.append(value.toObject());
        }
    }

    // Sort by updatedAt descending (newest first)
    std::sort(sessionList.begin(), sessionList.end(), [](const QJsonObject &a, const QJsonObject &b) {
        QString aDate = a[u"updatedAt"].toString();
        QString bDate = b[u"updatedAt"].toString();
        if (!aDate.isEmpty() && !bDate.isEmpty()) {
            return QDateTime::fromString(aDate, Qt::ISODate) > QDateTime::fromString(bDate, Qt::ISODate);
        }
        return false;
    });

    for (const QJsonObject &session : sessionList) {
        QString sessionId = session[u"id"].toString();
        // Try alternative key names that vibe-acp might use
        if (sessionId.isEmpty()) {
            sessionId = session[u"sessionId"].toString();
        }
        if (sessionId.isEmpty()) {
            sessionId = session[u"name"].toString();
        }
        QString title = session[u"title"].toString();
        QString name = session[u"name"].toString();
        QString updatedAt = session[u"updatedAt"].toString();
        QString cwd = session[u"cwd"].toString();

        // Use title if available, otherwise name, otherwise sessionId
        QString displayName = title;
        if (displayName.isEmpty()) {
            displayName = name;
        }
        if (displayName.isEmpty()) {
            displayName = sessionId;
        }

        // Format the updated date for display (using sortable ISO-like format)
        QString displayDate = updatedAt;
        if (!updatedAt.isEmpty()) {
            QDateTime dt = QDateTime::fromString(updatedAt, Qt::ISODate);
            if (dt.isValid()) {
                // Use a format that's both readable and sortable: YYYY-MM-DD HH:MM
                displayDate = dt.toLocalTime().toString(QStringLiteral("yyyy-MM-dd HH:mm"));
            }
        }

        // Truncate cwd if too long
        QString displayCwd = cwd;
        if (cwd.length() > 40) {
            displayCwd = QStringLiteral("...") + cwd.right(37);
        }

        QTreeWidgetItem *item = new QTreeWidgetItem(m_sessionTree);
        // Store sessionId in column 0's UserRole for double-click handler
        item->setData(0, Qt::UserRole, sessionId);
        // Column 0: Updated
        item->setText(0, displayDate);
        // Column 1: Title (with fallback to name or sessionId)
        item->setText(1, displayName);
        // Column 2: Working Directory
        item->setText(2, displayCwd);

        // Build tooltip with all details
        QString toolTip = QStringLiteral("Session ID: ") + sessionId;
        if (!title.isEmpty()) {
            toolTip += QStringLiteral("\nTitle: ") + title;
        }
        if (!updatedAt.isEmpty()) {
            toolTip += QStringLiteral("\nUpdated: ") + updatedAt;
        }
        if (!cwd.isEmpty()) {
            toolTip += QStringLiteral("\nWorking Directory: ") + cwd;
        }
        item->setToolTip(0, toolTip);

        // Ensure item is selectable and enabled for double-click
        item->setFlags(item->flags() | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    }

    // Resize columns to fit content after loading
    m_sessionTree->header()->resizeSections(QHeaderView::ResizeToContents);
}

void ACPSessionListWidget::showContextMenu(const QPoint &pos)
{
    QTreeWidgetItem *item = m_sessionTree->itemAt(pos);
    if (!item) {
        return;
    }

    QString sessionId = item->data(0, Qt::UserRole).toString();
    if (sessionId.isEmpty()) {
        return;
    }

    QMenu menu(this);

    QAction *resumeAction = menu.addAction(QIcon::fromTheme(QStringLiteral("media-playback-start")), i18n("Resume Session"));
    connect(resumeAction, &QAction::triggered, this, [this, sessionId]() {
        Q_EMIT sessionResumed(sessionId);
    });

    QAction *deleteAction = menu.addAction(QIcon::fromTheme(QStringLiteral("edit-delete")), i18n("Delete Session"));
    connect(deleteAction, &QAction::triggered, this, [this, sessionId]() {
        Q_EMIT sessionDeleted(sessionId);
    });

    menu.exec(m_sessionTree->viewport()->mapToGlobal(pos));
}

void ACPSessionListWidget::deleteSelectedSession()
{
    QTreeWidgetItem *item = m_sessionTree->currentItem();
    if (!item) {
        return;
    }

    QString sessionId = item->data(0, Qt::UserRole).toString();
    if (!sessionId.isEmpty()) {
        Q_EMIT sessionDeleted(sessionId);
    }
}

#include "moc_acpsessionlistwidget.cpp"
