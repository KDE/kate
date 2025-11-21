/*
 *    SPDX-FileCopyrightText: 2025 Waqar Ahmed <waqar.17a@gmail.com>
 *    SPDX-FileCopyrightText: 2025 Kåre Särs <kare.sars@iki.fi>
 *    SPDX-License-Identifier: LGPL-2.0-or-later
 */
#pragma once

#include "backend.h"
#include "dap/entities.h"
#include <QWidget>

class QComboBox;
class QTreeView;

namespace KTextEditor
{
class MainWindow;
}

class StackView : public QWidget
{
public:
    explicit StackView(Backend *backend, KTextEditor::MainWindow *mainWindow, QWidget *parent = nullptr);

    void clear();

    void insertStackFrame(const QList<dap::StackFrame> &frames);
    void stackFrameChanged(int level);
    void stackFrameSelected();

    void onThreads(const QList<dap::Thread> &threads);
    void updateThread(const dap::Thread &thread, Backend::ThreadState, bool isActive);
    void threadSelected(int thread);

    void onStackTreeContextMenuRequest(QPoint pos);

private:
    Backend *const m_backend;
    KTextEditor::MainWindow *const m_mainWindow;
    QComboBox *const m_threadCombo;
    QTreeView *const m_stackTree;
    int m_activeThread = -1;
};
