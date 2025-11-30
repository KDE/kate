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
    friend class BreakpointViewTest;

public:
    explicit BreakpointView(KTextEditor::MainWindow *mainWindow, BackendInterface *backend, QWidget *parent);

public:
    /**
     * Returns a list of all breakpoints by file
     */
    std::map<QUrl, QList<dap::SourceBreakpoint>> allBreakpoints() const;

    /**
     * Toggle breakpoint at the location of currently active view's cursor position
     */
    void toggleBreakpoint();

    void clearLineBreakpoints();

    // This function is called when the user sets breakpoint by clicking View border
    void updateBreakpoints(const KTextEditor::Document *document, const KTextEditor::Mark mark);

private:
    void slotBreakpointsSet(const QUrl &file, const QList<dap::Breakpoint> &breakpoints);

    /**
     * \p file the file where breakpoint should be toggled
     * \p line where to toggle breakpoint
     * \p enabledStateChange whether enabled state of a breakpoint changed i.e., it was checked/unchecked
     */
    void setBreakpoint(const QUrl &file, int line, std::optional<bool> enabledStateChange);
    void onBreakpointEvent(const dap::Breakpoint &bp, BackendInterface::BreakpointEventKind);

private:
    KTextEditor::MainWindow *const m_mainWindow;
    BackendInterface *const m_backend;
    QTreeView *const m_treeview;
    BreakpointModel *const m_breakpointModel;
};
