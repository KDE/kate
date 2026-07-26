/*
    SPDX-FileCopyrightText: 2026 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: MIT
*/

#include "acpsessionlistwidget.h"
#include "acpclient_debug.h"

#include <KLocalizedString>

#include <QListWidget>
#include <QVBoxLayout>

ACPSessionListWidget::ACPSessionListWidget(ACPClientServerManager *serverManager, QWidget *parent)
    : QWidget(parent)
    , m_serverManager(serverManager)
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);

    m_sessionList = new QListWidget(this);
    m_sessionList->setObjectName(QStringLiteral("sessionList"));
    m_sessionList->setIconSize(QSize(16, 16));
    m_layout->addWidget(m_sessionList);

    // Connect double-click to resume session
    connect(m_sessionList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
        QString sessionId = item->data(Qt::UserRole).toString();
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
    m_sessionList->clear();

    if (sessions.isEmpty()) {
        QListWidgetItem *item = new QListWidgetItem(i18n("No sessions available"), m_sessionList);
        item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
        return;
    }

    for (const QJsonValue &value : sessions) {
        if (value.isObject()) {
            QJsonObject session = value.toObject();

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

            // Build display text with session info
            QString displayText = displayName;
            if (!updatedAt.isEmpty()) {
                displayText += QStringLiteral(" (Updated: ") + updatedAt + QStringLiteral(")");
            }

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

            QListWidgetItem *item = new QListWidgetItem(displayText, m_sessionList);
            item->setData(Qt::UserRole, sessionId);
            item->setToolTip(toolTip);
            // Ensure item is selectable and enabled for double-click
            item->setFlags(item->flags() | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        }
    }
}

#include "moc_acpsessionlistwidget.cpp"
