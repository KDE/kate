/**
 * Description: Common interface for GDB and DAP clients
 *
 *  SPDX-FileCopyrightText: 2022 Héctor Mesa Jiménez <wmj.py@gmx.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */
#ifndef DEBUGVIEW_IFACE_H
#define DEBUGVIEW_IFACE_H

#include <QObject>
#include <QUrl>
#include <optional>

#include "configview.h"
#include "dap/entities.h"

class DebugViewInterface : public QObject
{
    Q_OBJECT
public:
    DebugViewInterface(QObject *parent);
    ~DebugViewInterface() override = default;

    /**
     * true if debugger is running
     */
    virtual bool debuggerRunning() const = 0;
    /**
     * true if debugger is running and busy executing commands
     */
    virtual bool debuggerBusy() const = 0;
    /**
     * true if a breakpoint exists at url:line
     */
    virtual bool hasBreakpoint(QUrl const &url, int line) const = 0;
    /**
     * true if debugger supports move program counter
     */
    virtual bool supportsMovePC() const = 0;
    /**
     * true if debugger supports run to cursor
     */
    virtual bool supportsRunToCursor() const = 0;
    /**
     * true if breakpoints can be set/unset
     */
    virtual bool canSetBreakpoints() const = 0;
    /**
     * true if execution can continue
     */
    virtual bool canContinue() const = 0;
    /**
     * true if basic movement actions can be used
     */
    virtual bool canMove() const = 0;
    /**
     * toggle breakpoint at url:line
     */
    virtual void toggleBreakpoint(QUrl const &url, int line) = 0;
    /**
     * move PC to url:line
     */
    virtual void movePC(QUrl const &url, int line) = 0;
    /**
     * run to url:line
     */
    virtual void runToCursor(QUrl const &url, int line) = 0;

    virtual void issueCommand(QString const &cmd) = 0;

    virtual QString targetName() const = 0;
    virtual void setFileSearchPaths(const QStringList &paths) = 0;

public Q_SLOTS:
    /**
     * interrupt debugger process
     */
    virtual void slotInterrupt() = 0;
    /**
     * step into command
     */
    virtual void slotStepInto() = 0;
    /**
     * step over command
     */
    virtual void slotStepOver() = 0;
    /**
     * step out command
     */
    virtual void slotStepOut() = 0;
    /**
     * continue command
     */
    virtual void slotContinue() = 0;
    /**
     * kill debuggee command
     */
    virtual void slotKill() = 0;
    /**
     * re-run program
     */
    virtual void slotReRun() = 0;

    virtual QString slotPrintVariable(const QString &variable) = 0;

    /**
     * query information
     *
     * stack
     * frame
     * arguments
     * this
     * locals
     * thread
     */
    virtual void slotQueryLocals(bool display) = 0;

    virtual void changeStackFrame(int index) = 0;
    virtual void changeThread(int thread) = 0;
    virtual void changeScope(int scopeId) = 0;

Q_SIGNALS:
    void debugLocationChanged(const QUrl &file, int lineNum);
    void breakPointSet(const QUrl &file, int lineNum);
    void breakPointCleared(const QUrl &file, int lineNum);
    void clearBreakpointMarks();
    void stackFrameInfo(int level, QString const &info);
    void stackFrameChanged(int level);
    void threadInfo(const dap::Thread &thread, bool active);

    void variableInfo(int parentId, const dap::Variable &variable);
    void variableScopeOpened();
    void variableScopeClosed();
    void scopesInfo(const QList<dap::Scope> &scopes, std::optional<int> activeId);

    void outputText(const QString &text);
    void outputError(const QString &text);
    void debuggeeOutput(const dap::Output &output);
    void readyForInput(bool ready);
    void programEnded();
    void gdbEnded();
    void sourceFileNotFound(const QString &fileName);

    void debuggerCapabilitiesChanged();
};

#endif
