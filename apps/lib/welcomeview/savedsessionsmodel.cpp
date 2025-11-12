/*
 *  SPDX-FileCopyrightText: 2022 Eugene Popov <popov895@ukr.net>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "savedsessionsmodel.h"

#include <KConfig>

#include <QIcon>

SavedSessionsModel::SavedSessionsModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

QVariant SavedSessionsModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid()) {
        const size_t row = index.row();
        if (row < m_sessions.size()) {
            const SessionInfo &session = m_sessions.at(row);
            switch (role) {
            case Qt::DisplayRole:
                return session.name;
            case Qt::DecorationRole:
                return QIcon::fromTheme(QStringLiteral("document-save"));
            case Qt::ToolTipRole:
                return session.lastOpened.toString();
            default:
                break;
            }
        }
    }

    return {};
}

int SavedSessionsModel::rowCount(const QModelIndex &) const
{
    return int(m_sessions.size());
}

void SavedSessionsModel::refresh(const KateSessionList &sessionList)
{
    std::vector<SessionInfo> sessions;
    sessions.reserve(sessionList.count());

    for (const KateSession::Ptr &session : sessionList) {
        sessions.push_back({session->timestamp(), session->name()});
    }

    std::sort(sessions.begin(), sessions.end());

    beginResetModel();
    m_sessions = std::move(sessions);
    endResetModel();
}
