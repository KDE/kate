/**
 * Description: Common interface for GDB and DAP clients
 *
 *  SPDX-FileCopyrightText: 2022 Héctor Mesa Jiménez <wmj.py@gmx.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "backend.h"
#include "debugview.h"
#include "debugview_dap.h"
#include <KMessageBox>

Backend::Backend(QObject *parent)
    : DebugViewInterface(parent)
    , m_debugger(nullptr)
{
}

void Backend::runDebugger(const GDBTargetConf &conf, const QStringList &ioFifos)
{
    if (m_debugger && m_debugger->debuggerRunning()) {
        KMessageBox::error(nullptr, i18n("A debugging session is on course. Please, use re-run or stop the current session."));
        return;
    }

    DebugView *gdb;

    if (m_mode != GDB) {
        unbind();
        m_debugger = gdb = new DebugView(this);
        m_mode = GDB;
        bind();
    } else {
        gdb = dynamic_cast<DebugView *>(m_debugger);
    }

    gdb->runDebugger(conf, ioFifos);

    if (m_displayQueryLocals) {
        gdb->slotQueryLocals(*m_displayQueryLocals);
    }
}

void Backend::runDebugger(const DAPTargetConf &conf)
{
    if (m_debugger && m_debugger->debuggerRunning()) {
        KMessageBox::error(nullptr, i18n("A debugging session is on course. Please, use re-run or stop the current session."));
        return;
    }

    DapDebugView *dap;

    unbind();
    m_debugger = dap = new DapDebugView(this);
    m_mode = DAP;
    bind();

    dap->runDebugger(conf);

    if (m_displayQueryLocals) {
        dap->slotQueryLocals(*m_displayQueryLocals);
    }
}

void Backend::bind()
{
    connect(m_debugger, &DebugViewInterface::debugLocationChanged, this, &DebugViewInterface::debugLocationChanged);
    connect(m_debugger, &DebugViewInterface::breakPointSet, this, &DebugViewInterface::breakPointSet);
    connect(m_debugger, &DebugViewInterface::breakPointCleared, this, &DebugViewInterface::breakPointCleared);
    connect(m_debugger, &DebugViewInterface::clearBreakpointMarks, this, &DebugViewInterface::clearBreakpointMarks);
    connect(m_debugger, &DebugViewInterface::stackFrameInfo, this, &DebugViewInterface::stackFrameInfo);
    connect(m_debugger, &DebugViewInterface::stackFrameChanged, this, &DebugViewInterface::stackFrameChanged);
    connect(m_debugger, &DebugViewInterface::threadInfo, this, &DebugViewInterface::threadInfo);

    connect(m_debugger, &DebugViewInterface::variableInfo, this, &DebugViewInterface::variableInfo);
    connect(m_debugger, &DebugViewInterface::variableScopeOpened, this, &DebugViewInterface::variableScopeOpened);
    connect(m_debugger, &DebugViewInterface::variableScopeClosed, this, &DebugViewInterface::variableScopeClosed);

    connect(m_debugger, &DebugViewInterface::outputText, this, &DebugViewInterface::outputText);
    connect(m_debugger, &DebugViewInterface::outputError, this, &DebugViewInterface::outputError);
    connect(m_debugger, &DebugViewInterface::readyForInput, this, &DebugViewInterface::readyForInput);
    connect(m_debugger, &DebugViewInterface::programEnded, this, &DebugViewInterface::programEnded);
    connect(m_debugger, &DebugViewInterface::gdbEnded, this, &DebugViewInterface::gdbEnded);
    connect(m_debugger, &DebugViewInterface::sourceFileNotFound, this, &DebugViewInterface::sourceFileNotFound);
    connect(m_debugger, &DebugViewInterface::scopesInfo, this, &DebugViewInterface::scopesInfo);

    connect(m_debugger, &DebugViewInterface::debuggerCapabilitiesChanged, this, &DebugViewInterface::debuggerCapabilitiesChanged);

    connect(m_debugger, &DebugViewInterface::debuggeeOutput, this, &DebugViewInterface::debuggeeOutput);
}

void Backend::unbind()
{
    if (!m_debugger)
        return;
    disconnect(m_debugger, nullptr, this, nullptr);
    delete m_debugger;
}

bool Backend::debuggerRunning() const
{
    return m_debugger && m_debugger->debuggerRunning();
}

bool Backend::debuggerBusy() const
{
    return !m_debugger || m_debugger->debuggerBusy();
}

bool Backend::hasBreakpoint(QUrl const &url, int line) const
{
    return m_debugger && m_debugger->hasBreakpoint(url, line);
}

bool Backend::supportsMovePC() const
{
    return m_debugger && m_debugger->supportsMovePC();
}

bool Backend::supportsRunToCursor() const
{
    return m_debugger && m_debugger->supportsRunToCursor();
}

bool Backend::canSetBreakpoints() const
{
    return m_debugger && m_debugger->canSetBreakpoints();
}

bool Backend::canMove() const
{
    return m_debugger && m_debugger->canMove();
}

bool Backend::canContinue() const
{
    return m_debugger && m_debugger->canContinue();
}

void Backend::toggleBreakpoint(QUrl const &url, int line)
{
    if (m_debugger)
        m_debugger->toggleBreakpoint(url, line);
}

void Backend::movePC(QUrl const &url, int line)
{
    if (m_debugger)
        m_debugger->movePC(url, line);
}

void Backend::runToCursor(QUrl const &url, int line)
{
    if (m_debugger)
        m_debugger->runToCursor(url, line);
}

void Backend::issueCommand(QString const &cmd)
{
    if (m_debugger)
        m_debugger->issueCommand(cmd);
}

QString Backend::targetName() const
{
    if (m_debugger)
        return m_debugger->targetName();
    return QString();
}

void Backend::setFileSearchPaths(const QStringList &paths)
{
    if (m_debugger)
        m_debugger->setFileSearchPaths(paths);
}

void Backend::slotInterrupt()
{
    if (m_debugger)
        m_debugger->slotInterrupt();
}

void Backend::slotStepInto()
{
    if (m_debugger)
        m_debugger->slotStepInto();
}

void Backend::slotStepOver()
{
    if (m_debugger)
        m_debugger->slotStepOver();
}

void Backend::slotStepOut()
{
    if (m_debugger)
        m_debugger->slotStepOut();
}

void Backend::slotContinue()
{
    if (m_debugger)
        m_debugger->slotContinue();
}

void Backend::slotKill()
{
    if (m_debugger)
        m_debugger->slotKill();
}

void Backend::slotReRun()
{
    if (m_debugger)
        m_debugger->slotReRun();
}

QString Backend::slotPrintVariable(const QString &variable)
{
    if (m_debugger)
        return m_debugger->slotPrintVariable(variable);
    return QString();
}

void Backend::slotQueryLocals(bool display)
{
    if (m_debugger) {
        m_debugger->slotQueryLocals(display);
        m_displayQueryLocals = std::nullopt;
    } else {
        m_displayQueryLocals = display;
    }
}

void Backend::changeStackFrame(int index)
{
    if (m_debugger)
        m_debugger->changeStackFrame(index);
}

void Backend::changeThread(int thread)
{
    if (m_debugger)
        m_debugger->changeThread(thread);
}

void Backend::changeScope(int scopeId)
{
    if (m_debugger)
        m_debugger->changeScope(scopeId);
}
