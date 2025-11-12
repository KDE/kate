/*
 *    SPDX-FileCopyrightText: 2025 Waqar Ahmed <waqar.17a@gmail.com>
 *    SPDX-License-Identifier: LGPL-2.0-or-later
 */
#include "stackframe_model.h"

#include <KLocalizedString>

#include <ktexteditor_utils.h>

#include <QIcon>

int StackFrameModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return Num_Columns;
}

int StackFrameModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_frames.size();
}

QVariant StackFrameModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= m_frames.size()) {
        return {};
    }

    if (role == StackFrameRole) {
        return QVariant::fromValue(m_frames.at(index.row()));
    }

    if (role != Qt::DisplayRole && role != Qt::DecorationRole) {
        return {};
    }

    const dap::StackFrame &frame = m_frames.at(index.row());
    const Column column = static_cast<Column>(index.column());
    switch (column) {
    case Number:
        if (role == Qt::DisplayRole) {
            if (index.row() == m_activeFrame) {
                return QStringLiteral("> %1").arg(index.row());
            }
            return QString::number(index.row());
        }
        break;
    case Func:
        if (role == Qt::DisplayRole) {
            return frame.name;
        }
        break;
    case Path:
        if (role == Qt::DisplayRole) {
            if (frame.source.has_value()) {
                return QStringLiteral("%2:%3").arg(Utils::formatUrl(frame.source->path)).arg(frame.line);
            }
        }
        break;

    case Num_Columns:
        break;
    }

    return {};
}

QVariant StackFrameModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return {};
    }

    const Column column = static_cast<Column>(section);
    switch (column) {
    case Number:
        return i18nc("Column label (frame number)", "Nr");
    case Func:
        return i18nc("Column label", "Function");
    case Path:
        return i18nc("Column label", "Location");
    case Num_Columns:
        break;
    }
    return {};
}

void StackFrameModel::setActiveFrame(int level)
{
    const int oldActiveRow = m_activeFrame;
    m_activeFrame = level;

    if (oldActiveRow >= 0 && oldActiveRow < m_frames.size()) {
        const auto oldActive = index(oldActiveRow, 0, {});
        Q_EMIT dataChanged(oldActive, oldActive, QList<int>{Qt::DisplayRole});
    }

    if (m_activeFrame >= 0 && m_activeFrame < m_frames.size()) {
        const auto newActive = index(m_activeFrame, 0, {});
        Q_EMIT dataChanged(newActive, newActive, QList<int>{Qt::DisplayRole});
    }
}

void StackFrameModel::setFrames(const QList<dap::StackFrame> &frames)
{
    beginResetModel();
    m_frames = frames;
    endResetModel();
}
