/*
    SPDX-FileCopyrightText: 2026 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: MIT
*/

#include "acpsessionlistwidget.h"
#include "acpclient_debug.h"

#include <KLocalizedString>

#include <QDateTime>
#include <QHeaderView>
#include <QTreeWidget>
#include <QVBoxLayout>

ACPSessionListWidget::ACPSessionListWidget(ACPClientServerManager *serverManager, QWidget *parent)
    : QWidget(parent)
    , m_serverManager(serverManager)
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);

    m_sessionTree = new QTreeWidget(this);
    m_sessionTree->setObjectName(QStringLiteral("sessionTree"));
    m_sessionTree->setHeaderHidden(false);
    m_sessionTree->setRootIsDecorated(false);
    m_sessionTree->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_sessionTree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_sessionTree->setSortingEnabled(true);

    // Set up columns
    QStringList headers;
    headers << i18n("Title") << i18n("Updated") << i18n("Working Directory");
    m_sessionTree->setColumnCount(headers.size());
    m_sessionTree->setHeaderLabels(headers);

    // Resize columns to fit content
    m_sessionTree->header()->setSectionResizeMode(QHeaderView::Stretch);

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
        item->setData(0, Qt::UserRole, sessionId);
        // Column 0: Title (with fallback to name or sessionId)
        item->setText(0, displayName);
        // Column 1: Updated
        item->setText(1, displayDate);
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
}

#include "moc_acpsessionlistwidget.cpp"
