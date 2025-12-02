/*
 *    SPDX-FileCopyrightText: 2025 Waqar Ahmed <waqar.17a@gmail.com>
 *    SPDX-License-Identifier: LGPL-2.0-or-later
 */
#pragma once

#include <QAbstractTableModel>
#include <dap/entities.h>

class StackFrameModel : public QAbstractTableModel
{
public:
    enum Column {
        Number,
        Func,
        Path,
        Num_Columns // Keep at end
    };

    enum Role {
        StackFrameRole = Qt::UserRole + 1
    };

    explicit StackFrameModel(QObject *parent = nullptr)
        : QAbstractTableModel(parent)
    {
    }

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    int activeFrame() const
    {
        return m_activeFrame;
    }
    void setActiveFrame(int level);
    void setFrames(const QList<dap::StackFrame> &frames, const QList<dap::Module> &modules);

private:
    int m_activeFrame = -1;
    QList<dap::StackFrame> m_frames;
    QList<dap::Module> m_modules;
};
