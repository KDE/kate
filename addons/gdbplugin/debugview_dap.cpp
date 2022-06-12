/*
    SPDX-FileCopyrightText: 2022 Héctor Mesa Jiménez <wmj.py@gmx.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include <KMessageBox>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <pthread.h>
#include <qregularexpression.h>

#include "dap/bus_selector.h"
#include "debugview_dap.h"
#include "json_placeholders.h"

QString newLine(const QString &text)
{
    return QStringLiteral("\n") + text;
}

QString printEvent(const QString &text)
{
    return QStringLiteral("\n--> %1").arg(text);
}

DapDebugView::DapDebugView(QObject *parent)
    : DebugViewInterface(parent)
    , m_client(nullptr)
    , m_state(State::None)
    , m_requests(0)
{
}

void DapDebugView::unsetClient()
{
    if (m_client) {
        disconnect(m_client->bus());
        disconnect(m_client);
        m_client->detach();
        m_client->deleteLater();
        m_client = nullptr;
    }
    resetState(State::None);
    shutdownUntil();
    m_currentScope = std::nullopt;
}

void DapDebugView::resetState(State state)
{
    m_requests = 0;
    m_runToCursor = std::nullopt;
    if (state != Running)
        m_currentThread = std::nullopt;
    m_watchedThread = std::nullopt;
    m_currentFrame = std::nullopt;
    m_commandQueue.clear();
    m_restart = false;
    m_frames.clear();
    m_task = Idle;
    setState(state);
}

void DapDebugView::setState(State state)
{
    if (state == m_state)
        return;

    m_state = state;
    Q_EMIT readyForInput(debuggerRunning());

    switch (m_state) {
    case State::Terminated:
        Q_EMIT programEnded();
        if (continueShutdown()) {
            tryDisconnect();
        }
        break;
    case State::Disconnected:
        if (continueShutdown()) {
            cmdShutdown();
        }
        break;
    case State::PostMortem:
        shutdownUntil();
        if (m_restart) {
            m_restart = false;
            start();
        }
        break;
    case State::None:
        shutdownUntil();
        break;
    default:
        break;
    }
}

void DapDebugView::setTaskState(Task state)
{
    if (state == m_task)
        return;
    m_task = state;
    Q_EMIT readyForInput(debuggerRunning() && (m_task != Busy));
    if ((m_task == Idle) && !m_commandQueue.isEmpty()) {
        issueCommand(m_commandQueue.takeFirst());
    }
}

void DapDebugView::pushRequest()
{
    ++m_requests;
    setTaskState(Busy);
}

void DapDebugView::popRequest()
{
    if (m_requests > 0) {
        --m_requests;
    }
    setTaskState(m_requests > 0 ? Busy : Idle);
}

dap::settings::ClientSettings &DapDebugView::target2dap(const DAPTargetConf &target)
{
    // resolve dynamic port
    auto settings = dap::settings::resolveClientPort(target.dapSettings->settings);
    if (!settings) {
        settings = target.dapSettings->settings;
    }
    // resolve user variables
    auto varMap = dap::settings::findReferences(*settings);
    for (auto it = target.variables.constBegin(); it != target.variables.constEnd(); ++it) {
        varMap[it.key()] = it.value().toJsonValue();
        if (it.key() == QStringLiteral("file")) {
            m_file = it.value().toString();
        } else if (it.key() == QStringLiteral("workdir")) {
            m_workDir = it.value().toString();
        }
    }

    const auto out = json::resolve(*settings, varMap);

    Q_EMIT outputText(QString::fromLocal8Bit(QJsonDocument(out).toJson()) + QStringLiteral("\n"));

    m_settings = dap::settings::ClientSettings(out);
    return *m_settings;
}

void DapDebugView::start()
{
    if ((m_state != None) && (m_state != PostMortem)) {
        KMessageBox::error(nullptr, i18n("A debugging session is on course. Please, use re-run or stop the current session."));
        return;
    }
    unsetClient();

    m_client = new dap::Client(*m_settings, this);

    Q_EMIT debuggerCapabilitiesChanged();

    // connect
    connect(m_client->bus(), &dap::Bus::error, this, &DapDebugView::onError);

    connect(m_client, &dap::Client::finished, this, &DapDebugView::onServerFinished);
    connect(m_client, &dap::Client::failed, [this] {
        onError(i18n("DAP backend failed"));
    });

    connect(m_client, &dap::Client::serverDisconnected, this, &DapDebugView::onServerDisconnected);
    connect(m_client, &dap::Client::debuggeeExited, this, &DapDebugView::onProgramEnded);
    connect(m_client, &dap::Client::debuggeeTerminated, this, &DapDebugView::onTerminated);
    connect(m_client, &dap::Client::debuggeeStopped, this, &DapDebugView::onStopped);
    connect(m_client, &dap::Client::capabilitiesReceived, this, &DapDebugView::onCapabilitiesReceived);
    connect(m_client, &dap::Client::debuggeeRunning, this, &DapDebugView::onRunning);
    connect(m_client, &dap::Client::debuggeeContinued, this, &DapDebugView::onContinuedEvent);
    connect(m_client, &dap::Client::debuggingProcess, this, &DapDebugView::onDebuggingProcess);

    connect(m_client, &dap::Client::threads, this, &DapDebugView::onThreads);
    connect(m_client, &dap::Client::stackTrace, this, &DapDebugView::onStackTrace);
    connect(m_client, &dap::Client::initialized, this, &DapDebugView::onInitialized);
    connect(m_client, &dap::Client::errorResponse, this, &DapDebugView::onErrorResponse);
    connect(m_client, &dap::Client::outputProduced, this, &DapDebugView::onOutputProduced);

    connect(m_client, &dap::Client::threadChanged, this, &DapDebugView::onThreadEvent);
    connect(m_client, &dap::Client::moduleChanged, this, &DapDebugView::onModuleEvent);
    connect(m_client, &dap::Client::scopes, this, &DapDebugView::onScopes);
    connect(m_client, &dap::Client::variables, this, &DapDebugView::onVariables);
    connect(m_client, &dap::Client::modules, this, &DapDebugView::onModules);
    connect(m_client, &dap::Client::sourceBreakpoints, this, &DapDebugView::onSourceBreakpoints);
    connect(m_client, &dap::Client::breakpointChanged, this, &DapDebugView::onBreakpointEvent);
    connect(m_client, &dap::Client::expressionEvaluated, this, &DapDebugView::onExpressionEvaluated);
    connect(m_client, &dap::Client::gotoTargets, this, &DapDebugView::onGotoTargets);

    m_client->bus()->start(m_settings->busSettings);
}

void DapDebugView::runDebugger(const DAPTargetConf &conf)
{
    m_targetName = conf.targetName;

    target2dap(conf);

    start();
}

void DapDebugView::onTerminated()
{
    Q_EMIT outputText(printEvent(i18n("program terminated")));
    if (m_state < Terminated) {
        setState(Terminated);
    }
}

bool DapDebugView::tryDisconnect()
{
    if (!isConnectedState()) {
        return false;
    }

    Q_EMIT outputError(newLine(i18n("requesting disconnection")));
    if (m_client) {
        m_client->requestDisconnect();
    } else {
        setState(Disconnected);
    }
    return true;
}

void DapDebugView::cmdShutdown()
{
    if (m_state == None)
        return;

    Q_EMIT outputError(newLine(i18n("requesting shutdown")));
    if (m_client) {
        m_client->bus()->close();
    } else {
        setState(None);
    }
}

bool DapDebugView::tryTerminate()
{
    if (!isRunningState())
        return false;

    if (!m_client->supportsTerminate()) {
        setState(Terminated);
        return false;
    }

    m_client->requestTerminate();
    return true;
}

void DapDebugView::onError(const QString &message)
{
    Q_EMIT outputError(newLine(i18n("DAP backend: %1", message)));
    setState(PostMortem);
}

void DapDebugView::onStopped(const dap::StoppedEvent &info)
{
    setState(Stopped);
    m_currentThread = m_watchedThread = info.threadId;

    QStringList text = {i18n("stopped (%1).", info.reason)};
    if (info.description) {
        text << QStringLiteral(" (%1)").arg(info.description.value());
    }

    if (info.threadId) {
        text << QStringLiteral(" ");
        if (info.allThreadsStopped && info.allThreadsStopped.value()) {
            text << i18n("Active thread: %1 (all threads stopped).", info.threadId.value());
        } else {
            text << i18n("Active thread: %1.", info.threadId.value());
        }
    }

    if (info.hitBreakpointsIds) {
        text << QStringLiteral(" ") << i18n("Breakpoint(s) reached:");
        for (const int b : info.hitBreakpointsIds.value()) {
            text << QStringLiteral(" [%1] ").arg(b);
        }
    }

    Q_EMIT outputText(printEvent(text.join(QString())));

    // request stack trace
    if (m_currentThread) {
        pushRequest();
        m_client->requestStackTrace(*m_currentThread);
    }

    // request threads
    pushRequest();
    m_client->requestThreads();
}

void DapDebugView::onContinuedEvent(const dap::ContinuedEvent &info)
{
    resetState();
    Q_EMIT outputText(printEvent(i18n("(continued) thread %1", QString::number(info.threadId))));
    if (info.allThreadsContinued) {
        Q_EMIT outputText(QStringLiteral(" (%1)").arg(i18n("all threads continued")));
    }
}

void DapDebugView::onRunning()
{
    setState(State::Running);
    Q_EMIT outputText(printEvent(i18n("(running)")));
    // if there is not thread, request in case pause is called
    if (!m_currentThread) {
        pushRequest();
        m_client->requestThreads();
    }
}

void DapDebugView::onThreads(const QList<dap::Thread> &threads)
{
    if (!m_currentThread) {
        if (!threads.isEmpty()) {
            m_currentThread = threads[0].id;
        }
    } else {
        Q_EMIT threadInfo(dap::Thread(-1), false);
        for (const auto &thread : threads) {
            Q_EMIT threadInfo(thread, thread.id == m_currentThread.value_or(-1));
        }
    }
    popRequest();
}

void DapDebugView::informStackFrame()
{
    if (!m_queryLocals)
        return;

    int level = 0;

    for (const auto &frame : m_frames) {
        // emit stackFrameInfo
        // name at source:line
        QString info = frame.name;
        if (frame.source) {
            info = QStringLiteral("%1 at %2:%3").arg(info).arg(frame.source->path).arg(frame.line);
        }

        Q_EMIT stackFrameInfo(level, info);

        ++level;
    }
    Q_EMIT stackFrameInfo(-1, QString());
}

void DapDebugView::onStackTrace(const int /* threadId */, const dap::StackTraceInfo &info)
{
    m_currentFrame = std::nullopt;
    m_frames = info.stackFrames;
    informStackFrame();

    if (!m_frames.isEmpty()) {
        changeStackFrame(0);
    }
    popRequest();
}

void DapDebugView::clearBreakpoints()
{
    for (auto it = m_breakpoints.constBegin(); it != m_breakpoints.constEnd(); ++it) {
        const auto path = QUrl::fromLocalFile(it.key());
        for (const auto &bp : it.value()) {
            if (bp && bp->line) {
                Q_EMIT breakPointCleared(path, bp->line.value() - 1);
            }
        }
    }
    Q_EMIT clearBreakpointMarks();
}

void DapDebugView::onServerDisconnected()
{
    if (!isConnectedState()) {
        return;
    }

    clearBreakpoints();

    if (!m_restart) {
        m_breakpoints.clear();
        m_wantedBreakpoints.clear();
    }

    setState(Disconnected);
}

void DapDebugView::onServerFinished()
{
    Q_EMIT outputError(newLine(i18n("*** connection with server closed ***")));

    setState(PostMortem);
}

void DapDebugView::onProgramEnded(int exitCode)
{
    Q_EMIT outputText(printEvent(i18n("program exited with code %1", exitCode)));
}

void DapDebugView::onInitialized()
{
    Q_EMIT clearBreakpointMarks();
    if (!m_wantedBreakpoints.isEmpty()) {
        for (auto it = m_wantedBreakpoints.constBegin(); it != m_wantedBreakpoints.constEnd(); ++it) {
            m_breakpoints[it.key()].clear();
            pushRequest();
            m_client->requestSetBreakpoints(it.key(), it.value(), true);
        }
    }
    shutdownUntil(PostMortem);
    Q_EMIT outputText(newLine(i18n("*** waiting for user actions ***")));
}

void DapDebugView::onErrorResponse(const QString &summary, const std::optional<dap::Message> &message)
{
    Q_EMIT outputError(newLine(i18n("error on response: %1", summary)));
    if (message) {
        Q_EMIT outputError(QStringLiteral(" {code %1: %2}").arg(message->id).arg(message->format));
    }
}

void DapDebugView::onOutputProduced(const dap::Output &output)
{
    if (output.output.isEmpty())
        return;

    if (output.isSpecialOutput() && !output.output.isEmpty()) {
        QString channel;
        switch (output.category) {
        case dap::Output::Category::Important:
            channel = i18n("important");
            break;
        case dap::Output::Category::Telemetry:
            channel = i18n("telemetry");
            break;
        default:
            break;
        }
        if (channel.isEmpty()) {
            Q_EMIT(outputError(newLine(output.output)));
        } else {
            Q_EMIT(outputError(QStringLiteral("\n(%1) %2").arg(channel).arg(output.output)));
        }
    } else {
        Q_EMIT debuggeeOutput(output);
    }
}

void DapDebugView::onDebuggingProcess(const dap::ProcessInfo &info)
{
    QString out;
    if (info.systemProcessId) {
        out = i18n("debugging process [%1] %2", QString::number(info.systemProcessId.value()), info.name);
    } else {
        out = i18n("debugging process %1", info.name);
    }
    if (info.startMethod) {
        out += QStringLiteral(" (%1)").arg(i18n("Start method: %1", info.startMethod.value()));
    }
    Q_EMIT outputText(printEvent(out));
}

void DapDebugView::onThreadEvent(const dap::ThreadEvent &info)
{
    Q_EMIT outputText(printEvent(QStringLiteral("(%1) %2").arg(info.reason).arg(i18n("thread %1", QString::number(info.threadId)))));
}

QString printModule(const dap::Module &module)
{
    QString out = QStringLiteral("module %2: %1").arg(module.name);
    if (module.id_int) {
        out = out.arg(module.id_int.value());
    } else if (module.id_str) {
        out = out.arg(module.id_str.value());
    }
    if (module.isOptimized && module.isOptimized.value()) {
        out += QStringLiteral(" [optimized]");
    }
    if (module.path) {
        out += QStringLiteral(": %1").arg(module.path.value());
    }
    return out;
}

QString printBreakpoint(const QString &sourceId, const dap::SourceBreakpoint &def, const std::optional<dap::Breakpoint> &bp, const int bId)
{
    QString txtId = QStringLiteral("%1.").arg(bId);
    if (!bp) {
        txtId += QStringLiteral(" ");
    } else {
        if (bp->verified) {
            txtId += bp->id ? QString::number(bp->id.value()) : QStringLiteral("?");
        } else {
            txtId += QStringLiteral("!");
        }
    }

    QStringList out = {QStringLiteral("[%1] %2: %3").arg(txtId).arg(sourceId).arg(def.line)};
    if (def.column) {
        out << QStringLiteral(", %1").arg(def.column.value());
    }
    if (bp) {
        if (bp->line) {
            out << QStringLiteral("->%1").arg(bp->line.value());
            if (bp->endLine) {
                out << QStringLiteral("-%1").arg(bp->endLine.value());
            }
            if (bp->column) {
                out << QStringLiteral(",%1").arg(bp->column.value());
                if (bp->endColumn) {
                    out << QStringLiteral("-%1").arg(bp->endColumn.value());
                }
            }
        }
    }
    if (def.condition) {
        out << QStringLiteral(" when {%1}").arg(def.condition.value());
    }
    if (def.hitCondition) {
        out << QStringLiteral(" hitcount {%1}").arg(def.hitCondition.value());
    }
    if (bp && bp->message) {
        out << QStringLiteral(" (%1)").arg(bp->message.value());
    }

    return out.join(QString());
}

void DapDebugView::onModuleEvent(const dap::ModuleEvent &info)
{
    Q_EMIT outputText(printEvent(QStringLiteral("(%1) %2").arg(info.reason).arg(printModule(info.module))));
}

void DapDebugView::changeScope(int scopeId)
{
    if (!m_client)
        return;

    if (m_currentScope && (*m_currentScope == scopeId))
        return;

    m_currentScope = scopeId;

    if (m_queryLocals) {
        pushRequest();
        m_client->requestVariables(scopeId);
    }
}

void DapDebugView::onScopes(const int /*frameId*/, const QList<dap::Scope> &scopes)
{
    std::optional<int> activeScope = std::nullopt;

    for (const auto &scope : scopes) {
        if (!m_currentScope || (*m_currentScope == scope.variablesReference)) {
            activeScope = scope.variablesReference;
            break;
        }
    }

    if (activeScope) {
        m_currentScope = activeScope;
    } else if (!scopes.isEmpty()) {
        m_currentScope = scopes[0].variablesReference;
    } else {
        m_currentScope = std::nullopt;
    }

    if (m_queryLocals) {
        // preload variables
        pushRequest();
        m_client->requestVariables(*m_currentScope);
        Q_EMIT scopesInfo(scopes, m_currentScope);
    }
    popRequest();
}

void DapDebugView::onVariables(const int variablesReference, const QList<dap::Variable> &variables)
{
    if (!m_queryLocals) {
        popRequest();
        return;
    }

    const bool rootLevel = m_currentScope && (*m_currentScope == variablesReference);
    if (rootLevel) {
        Q_EMIT variableScopeOpened();
    }

    for (const auto &variable : variables) {
        Q_EMIT variableInfo(rootLevel ? 0 : variablesReference, variable);

        if (rootLevel && (variable.variablesReference > 0)) {
            // TODO don't retrieve expensive variables
            // retrieve inner info
            pushRequest();
            m_client->requestVariables(variable.variablesReference);
        }
    }

    if (m_requests == 0) {
        Q_EMIT variableScopeClosed();
    }

    popRequest();
}

void DapDebugView::onModules(const dap::ModulesInfo &modules)
{
    for (const auto &mod : modules.modules) {
        Q_EMIT outputText(newLine(printModule(mod)));
    }
    popRequest();
}

void DapDebugView::informBreakpointAdded(const QString &path, const dap::Breakpoint &bpoint)
{
    if (bpoint.line) {
        Q_EMIT outputText(QStringLiteral("\n%1 %2:%3\n").arg(i18n("breakpoint set")).arg(path).arg(bpoint.line.value()));
        // zero based line expected
        Q_EMIT breakPointSet(QUrl::fromLocalFile(path), bpoint.line.value() - 1);
    }
}

void DapDebugView::informBreakpointRemoved(const QString &path, const std::optional<dap::Breakpoint> &bpoint)
{
    if (bpoint && bpoint->line) {
        Q_EMIT outputText(QStringLiteral("\n%1 %2:%3\n").arg(i18n("breakpoint cleared")).arg(path).arg(bpoint->line.value()));
        // zero based line expected
        Q_EMIT breakPointCleared(QUrl::fromLocalFile(path), bpoint->line.value() - 1);
    }
}

void DapDebugView::onSourceBreakpoints(const QString &path, int reference, const std::optional<QList<dap::Breakpoint>> &breakpoints)
{
    if (!breakpoints) {
        popRequest();
        return;
    }

    const auto id = dap::Source::getUnifiedId(path, reference);
    if (id.isEmpty()) {
        popRequest();
        return;
    }

    if (!m_breakpoints.contains(id)) {
        m_breakpoints[id] = QList<std::optional<dap::Breakpoint>>();
        m_breakpoints[id].reserve(breakpoints->size());
    }

    // if runToCursor is pending, a bpoint with hit condition has been added
    const bool withRunToCursor = m_runToCursor && (m_runToCursor->path == path);
    bool mustContinue = false;
    const auto &wanted = m_wantedBreakpoints[path];

    auto &table = m_breakpoints[id];
    int pointIdx = 0;
    const int last = table.size();
    for (const auto &point : *breakpoints) {
        if (pointIdx >= last) {
            // bpoint added
            table << point;
            informBreakpointAdded(id, point);
        } else if (!table[pointIdx]) {
            // bpoint added
            table[pointIdx] = point;
            informBreakpointAdded(id, point);
        }
        if (withRunToCursor) {
            if (wanted[pointIdx].line == m_runToCursor->line) {
                mustContinue = point.line.has_value();
                m_runToCursor = std::nullopt;
            }
        }
        ++pointIdx;
    }

    popRequest();

    if (mustContinue) {
        slotContinue();
    }
}

void DapDebugView::onBreakpointEvent(const dap::BreakpointEvent &info)
{
    QStringList parts = {i18n("(%1) breakpoint", info.reason)};
    if (info.breakpoint.source) {
        parts << QStringLiteral(" ") << info.breakpoint.source->unifiedId();
    }
    if (info.breakpoint.line) {
        parts << QStringLiteral(":%1").arg(info.breakpoint.line.value());
    }

    Q_EMIT outputText(printEvent(parts.join(QString())));
}

void DapDebugView::onExpressionEvaluated(const QString &expression, const std::optional<dap::EvaluateInfo> &info)
{
    QString result;
    if (info) {
        result = info->result;
    } else {
        result = i18n("<not evaluated>");
    }

    Q_EMIT outputText(QStringLiteral("\n(%1) = %2").arg(expression).arg(result));

    popRequest();
}

void DapDebugView::onGotoTargets(const dap::Source &source, const int, const QList<dap::GotoTarget> &targets)
{
    if (!targets.isEmpty() && m_currentThread) {
        Q_EMIT outputError(newLine(QStringLiteral("jump target %1:%2 (%3)").arg(source.unifiedId()).arg(targets[0].line).arg(targets[0].label)));
        m_client->requestGoto(*m_currentThread, targets[0].id);
    }
    popRequest();
}

void DapDebugView::onCapabilitiesReceived(const dap::Capabilities &capabilities)
{
    // can set breakpoints now
    setState(State::Initializing);

    QStringList text = {QStringLiteral("\n%1:\n").arg(i18n("server capabilities"))};
    const QString tpl = QStringLiteral("* %1 \n");
    const auto format = [](const QString &field, bool value) {
        return QStringLiteral("* %1: %2\n").arg(field).arg(value ? i18n("supported") : i18n("unsupported"));
    };

    text << format(i18n("conditional breakpoints"), capabilities.supportsConditionalBreakpoints)
         << format(i18n("function breakpoints"), capabilities.supportsFunctionBreakpoints)
         << format(i18n("hit conditional breakpoints"), capabilities.supportsHitConditionalBreakpoints)
         << format(i18n("log points"), capabilities.supportsLogPoints) << format(i18n("modules request"), capabilities.supportsModulesRequest)
         << format(i18n("goto targets request"), capabilities.supportsGotoTargetsRequest)
         << format(i18n("terminate request"), capabilities.supportsTerminateRequest)
         << format(i18n("terminate debuggee"), capabilities.supportTerminateDebuggee);

    Q_EMIT outputText(text.join(QString()));
}

bool DapDebugView::supportsMovePC() const
{
    return isRunningState() && m_client && m_client->adapterCapabilities().supportsGotoTargetsRequest;
}

bool DapDebugView::supportsRunToCursor() const
{
    return isAttachedState() && m_client && m_client->adapterCapabilities().supportsHitConditionalBreakpoints;
}

bool DapDebugView::isConnectedState() const
{
    return m_client && (m_state != None) && (m_state != Disconnected) && (m_state != PostMortem);
}

bool DapDebugView::isAttachedState() const
{
    return isConnectedState() && (m_state != Terminated);
}

bool DapDebugView::isRunningState() const
{
    return (m_state == Running) || (m_state == Stopped);
}

bool DapDebugView::canSetBreakpoints() const
{
    return isAttachedState();
}

bool DapDebugView::canMove() const
{
    return isRunningState();
}

bool DapDebugView::canContinue() const
{
    return (m_state == Initializing) || (m_state == Stopped);
}

bool DapDebugView::debuggerRunning() const
{
    return m_client && (m_state != None);
}

bool DapDebugView::debuggerBusy() const
{
    return debuggerRunning() && (m_task == Busy);
}

std::optional<int> DapDebugView::findBreakpoint(const QString &path, int line) const
{
    if (!m_breakpoints.contains(path))
        return std::nullopt;

    const auto &bpoints = m_breakpoints[path];
    int index = 0;
    for (const auto &bp : bpoints) {
        if (bp && bp->line && (line == bp->line)) {
            return index;
        }
        ++index;
    }
    return std::nullopt;
}

bool DapDebugView::hasBreakpoint(QUrl const &url, int line) const
{
    return findBreakpoint(*resolveFilename(url.path()), line).has_value();
}

void DapDebugView::toggleBreakpoint(QUrl const &url, int line)
{
    const auto path = resolveOrWarn(url.path());
    const auto index = findBreakpoint(path, line);

    if (index) {
        // remove
        removeBreakpoint(path, *index);
    } else {
        // insert
        insertBreakpoint(path, line);
    }
}

void DapDebugView::removeBreakpoint(const QString &path, int index)
{
    m_wantedBreakpoints[path].removeAt(index);
    informBreakpointRemoved(path, m_breakpoints[path][index]);
    m_breakpoints[path].removeAt(index);

    pushRequest();
    m_client->requestSetBreakpoints(path, m_wantedBreakpoints[path], true);
}

void DapDebugView::insertBreakpoint(const QString &path, int line)
{
    if (!m_wantedBreakpoints.contains(path)) {
        m_wantedBreakpoints[path] = {dap::SourceBreakpoint(line)};
        m_breakpoints[path] = {std::nullopt};
    } else {
        m_wantedBreakpoints[path] << dap::SourceBreakpoint(line);
        m_breakpoints[path] << std::nullopt;
    }

    pushRequest();
    m_client->requestSetBreakpoints(path, m_wantedBreakpoints[path], true);
}

void DapDebugView::movePC(QUrl const &url, int line)
{
    if (!m_client)
        return;

    if (m_state != State::Stopped)
        return;

    if (!m_currentThread)
        return;

    if (!m_client->adapterCapabilities().supportsGotoTargetsRequest)
        return;

    const auto path = resolveOrWarn(url.path());

    pushRequest();
    m_client->requestGotoTargets(path, line);
}

void DapDebugView::runToCursor(QUrl const &url, int line)
{
    if (!m_client)
        return;

    if (!m_client->adapterCapabilities().supportsHitConditionalBreakpoints)
        return;

    const auto path = resolveOrWarn(url.path());

    dap::SourceBreakpoint bp(line);
    bp.hitCondition = QStringLiteral("<=1");

    if (!m_wantedBreakpoints.contains(path)) {
        m_wantedBreakpoints[path] = {std::move(bp)};
        m_breakpoints[path] = {std::nullopt};
    } else {
        m_wantedBreakpoints[path] << std::move(bp);
        m_breakpoints[path] << std::nullopt;
    }

    m_runToCursor = Cursor{line, path};
    pushRequest();
    m_client->requestSetBreakpoints(path, m_wantedBreakpoints[path], true);
}

void DapDebugView::cmdEval(const QString &cmd)
{
    int start = cmd.indexOf(QLatin1Char(' '));

    QString expression;
    if (start >= 0) {
        expression = cmd.mid(start).trimmed();
    }
    if (expression.isEmpty()) {
        Q_EMIT outputError(newLine(i18n("syntax error: expression not found")));
        return;
    }

    std::optional<int> frameId = std::nullopt;
    if (m_currentFrame)
        frameId = m_frames[*m_currentFrame].id;

    pushRequest();
    m_client->requestWatch(expression, frameId);
}

void DapDebugView::cmdJump(const QString &cmd)
{
    const static QRegularExpression rx_goto(QStringLiteral(R"--(^j[a-z]*\s+(\d+)(?:\s+(\S+))?$)--"));

    const auto match = rx_goto.match(cmd);
    if (!match.hasMatch()) {
        Q_EMIT outputError(newLine(i18n("syntax error: %1", cmd)));
        return;
    }

    const auto &txtLine = match.captured(1);
    bool ok = false;
    const int line = txtLine.toInt(&ok);
    if (!ok) {
        Q_EMIT outputError(newLine(i18n("invalid line: %1", txtLine)));
        return;
    }

    QString path = match.captured(2);
    if (path.isNull()) {
        if (!m_currentFrame) {
            Q_EMIT outputError(newLine(i18n("file not specified: %1", cmd)));
            return;
        }
        const auto &frame = this->m_frames[*m_currentFrame];
        if (!frame.source) {
            Q_EMIT outputError(newLine(i18n("file not specified: %1", cmd)));
            return;
        }
        path = frame.source->unifiedId();
    }

    this->movePC(QUrl::fromLocalFile(path), line);
}

void DapDebugView::cmdRunToCursor(const QString &cmd)
{
    const static QRegularExpression rx_goto(QStringLiteral(R"--(^to?\s+(\d+)(?:\s+(\S+))?$)--"));

    const auto match = rx_goto.match(cmd);
    if (!match.hasMatch()) {
        Q_EMIT outputError(newLine(i18n("syntax error: %1", cmd)));
        return;
    }

    const auto &txtLine = match.captured(1);
    bool ok = false;
    const int line = txtLine.toInt(&ok);
    if (!ok) {
        Q_EMIT outputError(newLine(i18n("invalid line: %1", txtLine)));
        return;
    }

    QString path = match.captured(2);
    if (path.isNull()) {
        if (!m_currentFrame) {
            Q_EMIT outputError(newLine(i18n("file not specified: %1", cmd)));
            return;
        }
        const auto &frame = this->m_frames[*m_currentFrame];
        if (!frame.source) {
            Q_EMIT outputError(newLine(i18n("file not specified: %1", cmd)));
            return;
        }
        path = frame.source->unifiedId();
    }

    this->runToCursor(QUrl::fromLocalFile(path), line);
}

void DapDebugView::cmdPause(const QString &cmd)
{
    if (!m_client)
        return;

    const static QRegularExpression rx_pause(QStringLiteral(R"--(^s[a-z]*(?:\s+(\d+))?\s*$)--"));

    const auto match = rx_pause.match(cmd);
    if (!match.hasMatch()) {
        Q_EMIT outputError(newLine(i18n("syntax error: %1", cmd)));
        return;
    }

    const auto &txtThread = match.captured(1);

    int threadId;

    if (!txtThread.isNull()) {
        bool ok = false;
        threadId = txtThread.toInt(&ok);
        if (!ok) {
            Q_EMIT outputError(newLine(i18n("invalid thread id: %1", txtThread)));
            return;
        }
    } else if (m_currentThread) {
        threadId = *m_currentThread;
    } else {
        Q_EMIT outputError(newLine(i18n("thread id not specified: %1", cmd)));
        return;
    }

    m_client->requestPause(threadId);
}

void DapDebugView::cmdContinue(const QString &cmd)
{
    if (!m_client)
        return;

    const static QRegularExpression rx_cont(QStringLiteral(R"--(^c[a-z]*(?:\s+(?P<ONLY>only))?(?:\s+(?P<ID>\d+))?\s*$)--"));

    const auto match = rx_cont.match(cmd);
    if (!match.hasMatch()) {
        Q_EMIT outputError(newLine(i18n("syntax error: %1", cmd)));
        return;
    }

    const auto &txtThread = match.captured(QStringLiteral("ID"));

    int threadId;

    if (!txtThread.isNull()) {
        bool ok = false;
        threadId = txtThread.toInt(&ok);
        if (!ok) {
            Q_EMIT outputError(newLine(i18n("invalid thread id: %1", txtThread)));
            return;
        }
    } else if (m_currentThread) {
        threadId = *m_currentThread;
    } else {
        Q_EMIT outputError(newLine(i18n("thread id not specified: %1", cmd)));
        return;
    }

    const auto only = match.captured(QStringLiteral("ONLY"));

    m_client->requestContinue(threadId, !only.isNull());
}

void DapDebugView::cmdStepIn(const QString &cmd)
{
    if (!m_client)
        return;

    const static QRegularExpression rx_in(QStringLiteral(R"--(^in?(?:\s+(?P<ONLY>only))?(?:\s+(?P<ID>\d+))?\s*$)--"));

    const auto match = rx_in.match(cmd);
    if (!match.hasMatch()) {
        Q_EMIT outputError(newLine(i18n("syntax error: %1", cmd)));
        return;
    }

    const auto &txtThread = match.captured(QStringLiteral("ID"));

    int threadId;

    if (!txtThread.isNull()) {
        bool ok = false;
        threadId = txtThread.toInt(&ok);
        if (!ok) {
            Q_EMIT outputError(newLine(i18n("invalid thread id: %1", txtThread)));
            return;
        }
    } else if (m_currentThread) {
        threadId = *m_currentThread;
    } else {
        Q_EMIT outputError(newLine(i18n("thread id not specified: %1", cmd)));
        return;
    }

    const auto only = match.captured(QStringLiteral("ONLY"));

    m_client->requestStepIn(threadId, !only.isNull());
}

void DapDebugView::cmdStepOut(const QString &cmd)
{
    if (!m_client)
        return;

    const static QRegularExpression rx_out(QStringLiteral(R"--(^o[a-z]*(?:\s+(?P<ONLY>only))?(?:\s+(?P<ID>\d+))?\s*$)--"));

    const auto match = rx_out.match(cmd);
    if (!match.hasMatch()) {
        Q_EMIT outputError(newLine(i18n("syntax error: %1", cmd)));
        return;
    }

    const auto &txtThread = match.captured(QStringLiteral("ID"));

    int threadId;

    if (!txtThread.isNull()) {
        bool ok = false;
        threadId = txtThread.toInt(&ok);
        if (!ok) {
            Q_EMIT outputError(newLine(i18n("invalid thread id: %1", txtThread)));
            return;
        }
    } else if (m_currentThread) {
        threadId = *m_currentThread;
    } else {
        Q_EMIT outputError(newLine(i18n("thread id not specified: %1", cmd)));
        return;
    }

    const auto only = match.captured(QStringLiteral("ONLY"));

    m_client->requestStepOut(threadId, !only.isNull());
}

void DapDebugView::cmdNext(const QString &cmd)
{
    if (!m_client)
        return;

    const static QRegularExpression rx_next(QStringLiteral(R"--(^n[a-z]*(?:\s+(?P<ONLY>only))?(?:\s+(?P<ID>\d+))?\s*$)--"));

    const auto match = rx_next.match(cmd);
    if (!match.hasMatch()) {
        Q_EMIT outputError(newLine(i18n("syntax error: %1", cmd)));
        return;
    }

    const auto &txtThread = match.captured(QStringLiteral("ID"));

    int threadId;

    if (!txtThread.isNull()) {
        bool ok = false;
        threadId = txtThread.toInt(&ok);
        if (!ok) {
            Q_EMIT outputError(newLine(i18n("invalid thread id: %1", txtThread)));
            return;
        }
    } else if (m_currentThread) {
        threadId = *m_currentThread;
    } else {
        Q_EMIT outputError(newLine(i18n("thread id not specified: %1", cmd)));
        return;
    }

    const auto only = match.captured(QStringLiteral("ONLY"));

    m_client->requestNext(threadId, !only.isNull());
}

void DapDebugView::cmdHelp(const QString & /*cmd*/)
{
    QStringList out = {QString(), i18n("Available commands:")};

    const QString tpl = QStringLiteral("* %1");

    out << tpl.arg(QStringLiteral("h[elp]")) << tpl.arg(QStringLiteral("p[rint] [expression]")) << tpl.arg(QStringLiteral("c[ontinue] [only] [threadId]"))
        << tpl.arg(QStringLiteral("n[ext] [only] [threadId]")) << tpl.arg(QStringLiteral("i[n] [only] [threadId]"))
        << tpl.arg(QStringLiteral("o[ut] [only] [threadId]")) << tpl.arg(QStringLiteral("s[top] [threadId]"));

    if (m_client->adapterCapabilities().supportsGotoTargetsRequest) {
        out << tpl.arg(QStringLiteral("j[ump] <line> [file]"));
    }
    if (m_client->adapterCapabilities().supportsHitConditionalBreakpoints) {
        out << tpl.arg(QStringLiteral("t[o] <line> [file]"));
    }

    if (m_client->adapterCapabilities().supportsModulesRequest) {
        out << tpl.arg(QStringLiteral("m[odules]"));
    }

    QString bcmd = QStringLiteral("b[reakpoint] <line>");
    if (m_client->adapterCapabilities().supportsConditionalBreakpoints) {
        bcmd += QStringLiteral(" [when {condition}]");
    }
    if (m_client->adapterCapabilities().supportsHitConditionalBreakpoints) {
        bcmd += QStringLiteral(" [hitcount {condition}]");
    }
    bcmd += QStringLiteral(" [on <file>]");

    out << tpl.arg(QStringLiteral("bl[ist]")) << tpl.arg(bcmd) << tpl.arg(QStringLiteral("boff <line> [file]"));

    out << tpl.arg(QStringLiteral("w[hereami]"));

    Q_EMIT outputText(out.join(QStringLiteral("\n")));
}

void DapDebugView::cmdListModules(const QString &)
{
    if (!m_client)
        return;

    if (!m_client->adapterCapabilities().supportsModulesRequest) {
        return;
    }

    pushRequest();
    m_client->requestModules();
}

void DapDebugView::cmdListBreakpoints(const QString &)
{
    int bId = 0;
    for (auto it = m_breakpoints.begin(); it != m_breakpoints.end(); ++it) {
        const auto &sourceId = it.key();
        const auto &defs = m_wantedBreakpoints[sourceId];
        int pointIdx = 0;
        for (const auto &b : it.value()) {
            const auto &def = defs[pointIdx];
            Q_EMIT outputText(newLine(printBreakpoint(sourceId, def, b, bId)));
            ++pointIdx;
            ++bId;
        }
    }
}

void DapDebugView::cmdBreakpointOn(const QString &cmd)
{
    static const QRegularExpression rx_bon(
        QStringLiteral(R"--(^b\s+(\d+)(?:\s+when\s+\{(?P<COND>.+)\})?(?:\s*hitcont\s+\{(?P<HIT>.+)\})?(?:\s+on\s+(?P<SOURCE>.+))?\s*$)--"));

    const auto match = rx_bon.match(cmd);
    if (!match.hasMatch()) {
        Q_EMIT outputText(newLine(i18n("syntax error: %1", cmd)));
        return;
    }
    const auto &txtLine = match.captured(1);
    bool ok = false;
    const int line = txtLine.toInt(&ok);
    if (!ok) {
        Q_EMIT outputError(newLine(i18n("invalid line: %1", txtLine)));
        return;
    }

    dap::SourceBreakpoint bp(line);

    bp.condition = match.captured(QStringLiteral("COND"));
    if (bp.condition->isNull()) {
        bp.condition = std::nullopt;
    } else if (!m_client->adapterCapabilities().supportsConditionalBreakpoints) {
        Q_EMIT outputError(newLine(i18n("conditional breakpoints are not supported by the server")));
        return;
    }

    bp.hitCondition = match.captured(QStringLiteral("HIT"));
    if (bp.hitCondition->isNull()) {
        bp.hitCondition = std::nullopt;
    } else if (!m_client->adapterCapabilities().supportsHitConditionalBreakpoints) {
        Q_EMIT outputError(newLine(i18n("hit conditional breakpoints are not supported by the server")));
        return;
    }
    QString path = match.captured(QStringLiteral("SOURCE"));
    if (path.isNull()) {
        if (!m_currentFrame) {
            Q_EMIT outputError(newLine(i18n("file not specified: %1", cmd)));
            return;
        }
        const auto &frame = this->m_frames[*m_currentFrame];
        if (!frame.source) {
            Q_EMIT outputError(newLine(i18n("file not specified: %1", cmd)));
            return;
        }
        path = resolveOrWarn(frame.source->unifiedId());
    } else {
        path = resolveOrWarn(path);
    }

    m_wantedBreakpoints[path] << std::move(bp);
    m_breakpoints[path] << std::nullopt;

    pushRequest();
    m_client->requestSetBreakpoints(path, m_wantedBreakpoints[path], true);
}

void DapDebugView::cmdBreakpointOff(const QString &cmd)
{
    const static QRegularExpression rx_boff(QStringLiteral(R"--(^bo[a-z]*?\s+(\d+)(?:\s+(\S+))?$)--"));

    const auto match = rx_boff.match(cmd);
    if (!match.hasMatch()) {
        Q_EMIT outputError(newLine(i18n("syntax error: %1", cmd)));
        return;
    }

    const auto &txtLine = match.captured(1);
    bool ok = false;
    const int line = txtLine.toInt(&ok);
    if (!ok) {
        Q_EMIT outputError(newLine(i18n("invalid line: %1", txtLine)));
        return;
    }

    QString path = match.captured(2);
    if (path.isNull()) {
        if (!m_currentFrame) {
            Q_EMIT outputError(newLine(i18n("file not specified: %1", cmd)));
            return;
        }
        const auto &frame = this->m_frames[*m_currentFrame];
        if (!frame.source) {
            Q_EMIT outputError(newLine(i18n("file not specified: %1", cmd)));
            return;
        }
        path = frame.source->unifiedId();
    }
    path = resolveOrWarn(path);

    const auto index = findBreakpoint(path, line);
    if (!index) {
        Q_EMIT outputError(newLine(i18n("breakpoint not found (%1:%2)", path, line)));
        return;
    }

    removeBreakpoint(path, *index);
}

void DapDebugView::cmdWhereami(const QString &)
{
    QStringList parts = {newLine(i18n("Current thread: "))};

    if (m_currentThread) {
        parts << QString::number(*m_currentThread);
    } else {
        parts << i18n("none");
    }

    parts << newLine(i18n("Current frame: "));
    if (m_currentFrame) {
        parts << QString::number(*m_currentFrame);
    } else {
        parts << i18n("none");
    }

    parts << newLine(i18n("Session state: "));
    switch (m_state) {
    case Initializing:
        parts << i18n("initializing");
        break;
    case Running:
        parts << i18n("running");
        break;
    case Stopped:
        parts << i18n("stopped");
        break;
    case Terminated:
        parts << i18n("terminated");
        break;
    case Disconnected:
        parts << i18n("disconnected");
        break;
    case PostMortem:
        parts << i18n("post mortem");
        break;
    default:
        parts << i18n("none");
        break;
    }

    Q_EMIT outputText(parts.join(QString()));
}

void DapDebugView::issueCommand(QString const &command)
{
    if (!m_client)
        return;

    if (m_task == Busy) {
        m_commandQueue << command;
        return;
    }

    QString cmd = command.trimmed();

    if (cmd.isEmpty())
        return;

    Q_EMIT outputText(QStringLiteral("\n(dap) %1").arg(command));

    if (cmd.startsWith(QLatin1Char('h'))) {
        cmdHelp(cmd);
    } else if (cmd.startsWith(QLatin1Char('c'))) {
        cmdContinue(cmd);
    } else if (cmd.startsWith(QLatin1Char('n'))) {
        cmdNext(cmd);
    } else if (cmd.startsWith(QLatin1Char('o'))) {
        cmdStepOut(cmd);
    } else if (cmd.startsWith(QLatin1Char('i'))) {
        cmdStepIn(cmd);
    } else if (cmd.startsWith(QLatin1Char('p'))) {
        cmdEval(cmd);
    } else if (cmd.startsWith(QLatin1Char('j'))) {
        cmdJump(cmd);
    } else if (cmd.startsWith(QLatin1Char('t'))) {
        cmdRunToCursor(cmd);
    } else if (cmd.startsWith(QLatin1Char('m'))) {
        cmdListModules(cmd);
    } else if (cmd.startsWith(QStringLiteral("bl"))) {
        cmdListBreakpoints(cmd);
    } else if (cmd.startsWith(QStringLiteral("bo"))) {
        cmdBreakpointOff(cmd);
    } else if (cmd.startsWith(QLatin1Char('b'))) {
        cmdBreakpointOn(cmd);
    } else if (cmd.startsWith(QLatin1Char('s'))) {
        cmdPause(cmd);
    } else if (cmd.startsWith(QLatin1Char('w'))) {
        cmdWhereami(cmd);
    } else {
        Q_EMIT outputError(newLine(i18n("command not found")));
    }
}

QString DapDebugView::targetName() const
{
    return m_targetName;
}

void DapDebugView::setFileSearchPaths(const QStringList & /*paths*/)
{
    // TODO
}

void DapDebugView::slotInterrupt()
{
    if (!isRunningState()) {
        return;
    }

    if (!m_currentThread) {
        Q_EMIT outputError(newLine(i18n("missing thread id")));
        return;
    }

    m_client->requestPause(*m_currentThread);
}

void DapDebugView::slotStepInto()
{
    if (!m_client)
        return;

    if (m_state != State::Stopped)
        return;

    if (!m_currentThread)
        return;

    m_client->requestStepIn(*m_currentThread);
}

void DapDebugView::slotStepOut()
{
    if (!m_client)
        return;

    if (m_state != State::Stopped)
        return;

    if (!m_currentThread)
        return;

    m_client->requestStepOut(*m_currentThread);
}

void DapDebugView::slotStepOver()
{
    if (!m_client)
        return;

    if (m_state != State::Stopped)
        return;

    if (!m_currentThread)
        return;

    m_client->requestNext(*m_currentThread);
}

void DapDebugView::slotContinue()
{
    if (!isAttachedState())
        return;

    if (m_state == State::Initializing) {
        m_client->requestConfigurationDone();
    } else if (m_currentThread) {
        m_client->requestContinue(*m_currentThread);
    }
}

void DapDebugView::shutdownUntil(std::optional<State> state)
{
    if (!state) {
        m_shutdown.target = std::nullopt;
        m_shutdown.userAction = std::nullopt;
    } else if (!m_shutdown.target || (*state > m_shutdown.target)) {
        // propagate until the deepest state
        m_shutdown.target = *state;
    }
}

bool DapDebugView::continueShutdown() const
{
    return m_restart || (m_shutdown.target && (m_shutdown.target > m_state));
}

void DapDebugView::slotKill()
{
    if (!isConnectedState()) {
        setState(None);
        Q_EMIT readyForInput(false);
        Q_EMIT gdbEnded();
        return;
    }
    // if it is running, interrupt instead of killing
    if (isRunningState() && !this->canContinue()) {
        slotInterrupt();
        return;
    }

    if (!m_shutdown.userAction) {
        if (isRunningState()) {
            shutdownUntil(PostMortem);
            tryTerminate();
        } else {
            shutdownUntil(PostMortem);
            tryDisconnect();
        }
    } else {
        switch (*m_shutdown.userAction) {
        case Polite:
            // try and shutdown server
            m_shutdown.userAction = Force;
            cmdShutdown();
            break;
        case Force:
            // kill server
            Q_EMIT outputError(newLine(i18n("killing backend")));
            unsetClient();
            break;
        }
    }
}

void DapDebugView::slotReRun()
{
    if (!m_client && m_settings) {
        start();
        return;
    }

    m_restart = true;
    slotKill();
}

void DapDebugView::slotQueryLocals(bool display)
{
    m_queryLocals = display;

    if (!display)
        return;

    if (!m_client)
        return;

    if (m_currentFrame) {
        informStackFrame();
        pushRequest();
        m_client->requestScopes(m_frames[*m_currentFrame].id);
    }
}

QString DapDebugView::slotPrintVariable(const QString &variable)
{
    const auto cmd = QStringLiteral("print %1").arg(variable);
    issueCommand(cmd);
    return cmd;
}

void DapDebugView::changeStackFrame(int index)
{
    if (!debuggerRunning())
        return;

    if ((m_frames.size() < index) || (index < 0))
        return;

    if (m_currentFrame && (*m_currentFrame == index))
        return;

    m_currentFrame = index;

    const auto &frame = m_frames[index];
    if (frame.source) {
        const auto id = frame.source->unifiedId();
        Q_EMIT outputText(QStringLiteral("\n") + i18n("Current frame [%3]: %1:%2 (%4)", id, QString::number(frame.line), QString::number(index), frame.name));
        // zero-based line
        Q_EMIT debugLocationChanged(QUrl::fromLocalFile(resolveOrWarn(id)), frame.line - 1);
    }

    Q_EMIT stackFrameChanged(index);

    slotQueryLocals(m_queryLocals);
}

void DapDebugView::changeThread(int index)
{
    if (!debuggerRunning())
        return;

    if (!m_queryLocals)
        return;

    if (m_watchedThread && (*m_watchedThread == index))
        return;

    m_watchedThread = index;

    pushRequest();
    m_client->requestStackTrace(index);
}

std::optional<QString> DapDebugView::resolveFilename(const QString &filename, bool fallback) const
{
    QFileInfo fInfo = QFileInfo(filename);
    if (fInfo.exists() && fInfo.isDir()) {
        return fInfo.absoluteFilePath();
    }

    if (fInfo.isAbsolute()) {
        return filename;
    }

    // working path
    if (!m_workDir.isEmpty()) {
        const auto base = QDir(m_workDir);
        fInfo = QFileInfo(base.absoluteFilePath(filename));
        if (fInfo.exists() && !fInfo.isDir()) {
            return fInfo.absoluteFilePath();
        }
    }

    // executable path
    if (!m_file.isEmpty()) {
        const auto base = QDir(QFileInfo(m_file).absolutePath());
        fInfo = QFileInfo(base.absoluteFilePath(filename));
        if (fInfo.exists() && !fInfo.isDir()) {
            return fInfo.absoluteFilePath();
        }
    }

    if (fallback)
        return filename;

    return std::nullopt;
}

QString DapDebugView::resolveOrWarn(const QString &filename)
{
    const auto path = resolveFilename(filename, false);

    if (path)
        return *path;

    Q_EMIT sourceFileNotFound(filename);

    return filename;
}
