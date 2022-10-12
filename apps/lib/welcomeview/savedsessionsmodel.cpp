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
{}

QVariant SavedSessionsModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid()) {
        const int row = index.row();
        if (row >= 0 && row < m_sessions.count()) {
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

    return QVariant();
}

int SavedSessionsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return m_sessions.count();
}

void SavedSessionsModel::refresh(const KateSessionList &sessionList)
{
    QVector<SessionInfo> sessions;
    sessions.reserve(sessionList.count());

    for (const KateSession::Ptr &session : sessionList) {
        sessions.append({ session->timestamp(), session->name() });
    }

    std::sort(sessions.begin(), sessions.end());

    beginResetModel();
    m_sessions = std::move(sessions);
    endResetModel();
}
