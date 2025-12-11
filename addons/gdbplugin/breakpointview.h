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
    [[nodiscard]] std::map<QUrl, QList<dap::SourceBreakpoint>> allBreakpoints() const;

    /**
     * Returns a list of all function breakpoints
     */
    [[nodiscard]] QList<dap::FunctionBreakpoint> allFunctionBreakpoints() const;

    /**
     * Toggle breakpoint at the location of currently active view's cursor position
     */
    void toggleBreakpoint();

    void clearLineBreakpoints();

    void runToCursor();

    void onStoppedAtLine(const QUrl &url, int line);

private:
    // This function is called when the user sets breakpoint by clicking View border
    void updateBreakpoints(const KTextEditor::Document *document, const KTextEditor::Mark mark);

    void slotBreakpointsSet(const QUrl &file, const QList<dap::Breakpoint> &breakpoints);

    /**
     * \p file the file where breakpoint should be toggled
     * \p line where to toggle breakpoint
     * \p enabledStateChange whether enabled state of a breakpoint changed i.e., it was checked/unchecked
     */
    void setBreakpoint(const QUrl &file, int line, std::optional<bool> enabledStateChange, bool isOneShot = false);
    void onBreakpointEvent(const dap::Breakpoint &bp, BackendInterface::BreakpointEventKind);
    void enableBreakpointMarks(KTextEditor::Document *doc);

    void onRemoveBreakpointRequested(const QUrl &url, int line);
    void onAddBreakpointRequested(const QUrl &url, const dap::SourceBreakpoint &breakpoint);
    void onListBreakpointsRequested();
    void runToPosition(const QUrl &url, int line);
    void addOrRemoveDocumentBreakpointMark(const QUrl &url, int line, bool add) const;
    void onContextMenuRequested(QPoint pos);
    void buildContextMenu(const QModelIndex &index, QMenu *menu);
    void onAddFunctionBreakpoint();
    void addFunctionBreakpoint(const QString &function);

private:
    KTextEditor::MainWindow *const m_mainWindow;
    BackendInterface *const m_backend;
    QTreeView *const m_treeview;
    BreakpointModel *const m_breakpointModel;
};
