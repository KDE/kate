/*
    SPDX-FileCopyrightText: 2026 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: MIT
*/

#include "acpsessionlistwidget.h"

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
            QString name = session[u"name"].toString();

            // Use sessionId as name if name is not available
            if (name.isEmpty()) {
                name = sessionId;
            }

            // Add session info if available
            QString createdAt = session[u"createdAt"].toString();
            QString updatedAt = session[u"updatedAt"].toString();

            QString displayText = name;
            if (!createdAt.isEmpty()) {
                displayText += QStringLiteral(" (Created: ") + createdAt + QStringLiteral(")");
            }

            QListWidgetItem *item = new QListWidgetItem(displayText, m_sessionList);
            item->setData(Qt::UserRole, sessionId);
            item->setToolTip(QStringLiteral("Session ID: ") + sessionId);
        }
    }
}

#include "moc_acpsessionlistwidget.cpp"
