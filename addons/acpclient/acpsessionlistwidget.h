/*
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>
    SPDX-FileCopyrightText: 2026 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: MIT
*/

#pragma once

#include <QWidget>

#include <QJsonArray>

class QTreeWidget;
class QTreeWidgetItem;
class QVBoxLayout;
class ACPClientServerManager;

/**
 * @class ACPSessionListWidget
 * @brief Widget for displaying the list of ACP sessions
 *
 * This widget shows available sessions and allows the user to:
 * - View existing sessions
 * - Switch to a session
 * - Resume a session
 *
 * It connects to the server manager to receive session list updates.
 */
class ACPSessionListWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Construct the session list widget
     * @param serverManager Server manager to query for sessions
     * @param parent Qt parent widget
     */
    explicit ACPSessionListWidget(ACPClientServerManager *serverManager, QWidget *parent = nullptr);

    /** @brief Destructor */
    ~ACPSessionListWidget() override;

    /**
     * @brief Update the session list
     * @param sessions JSON array of session objects
     */
    void updateSessionList(const QJsonArray &sessions);

Q_SIGNALS:
    /**
     * @brief Emitted when a session should be resumed
     * @param sessionId ID of the session to resume
     */
    void sessionResumed(const QString &sessionId);

private:
    ACPClientServerManager *m_serverManager = nullptr;
    QTreeWidget *m_sessionTree = nullptr;
    QVBoxLayout *m_layout = nullptr;
};
