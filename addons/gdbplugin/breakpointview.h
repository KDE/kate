/*
 *    SPDX-FileCopyrightText: 2025 Waqar Ahmed <waqar.17a@gmail.com>
 *    SPDX-License-Identifier: LGPL-2.0-or-later
 */
#pragma once

#include "backendinterface.h"
#include "dap/entities.h"

#include <KTextEditor/Document>

#include <QWidget>

class QTreeView;
class Backend;
class BreakpointModel;

namespace KTextEditor
{
class MainWindow;
}

class BreakpointView : public QWidget
{
public:
    explicit BreakpointView(KTextEditor::MainWindow *mainWindow, Backend *backend, QWidget *parent);

public:
    /**
     * Returns a list of all breakpoints by file
     */
    std::map<QUrl, QList<dap::SourceBreakpoint>> allBreakpoints() const;

    /**
     * Toggle breakpoint at the location of currently active view's cursor position
     */
    void toggleBreakpoint();

    void clearMarks();

    // This function is called when the user sets breakpoint by clicking View border
    void updateBreakpoints(const KTextEditor::Document *document, const KTextEditor::Mark mark);

private:
    void slotBreakpointsSet(const QUrl &file, const QList<dap::Breakpoint> &breakpoints);
    void setBreakpoint(const QUrl &file, int line);
    void onBreakpointEvent(const dap::Breakpoint &bp, BackendInterface::BreakpointEventKind);

private:
    KTextEditor::MainWindow *const m_mainWindow;
    Backend *const m_backend;
    QTreeView *const m_treeview;
    BreakpointModel *const m_breakpointModel;
};
