/*
    SPDX-FileCopyrightText: 2022 Héctor Mesa Jiménez <wmj.py@gmx.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include <KMessageBox>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QRegularExpression>

#include <KLocalizedString>

#include <ktexteditor_utils.h>

#include "dapbackend.h"
#include "json_placeholders.h"

static QString newLine(const QString &text)
{
    return QStringLiteral("\n") + text;
}

static QString printEvent(const QString &text)
{
    return QStringLiteral("\n--> %1").arg(text);
}

using Utils::formatUrl;

DapBackend::DapBackend(QObject *parent)
    : BackendInterface(parent)
{
    m_requestThreadsTimer.setInterval(100);
    m_requestThreadsTimer.setSingleShot(true);
    m_requestThreadsTimer.callOnTimeout(this, [this] {
        if (m_client) {
            m_client->requestThreads();
        }
    });
}

DapBackend::~DapBackend()
{
    if (m_state != None) {
        shutdownUntil(None);
        slotKill();
    }
}

void DapBackend::unsetClient()
{
    m_requestThreadsTimer.stop();
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
    m_modules.clear();
}

void DapBackend::resetState(State state)
{
    m_requests = 0;
    m_runToCursor = std::nullopt;
    if (state != Running) {
        m_currentThread = std::nullopt;
    }
    m_currentFrame = std::nullopt;
    m_commandQueue.clear();
    m_restart = false;
    m_frames.clear();
    m_task = Idle;
    setState(state);
}

void DapBackend::setState(State state)
{
    if (state == m_state) {
        return;
    }

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
    case State::None:
        shutdownUntil();
        if (m_restart) {
            m_restart = false;
            start();
        } else {
            Q_EMIT gdbEnded();
        }
        break;
    default:
        break;
    }
}

void DapBackend::setTaskState(Task state)
{
    if (state == m_task) {
        return;
    }
    m_task = state;
    Q_EMIT readyForInput(debuggerRunning() && (m_task != Busy));
    if ((m_task == Idle) && !m_commandQueue.isEmpty()) {
        issueCommand(m_commandQueue.takeFirst());
    }
}

void DapBackend::pushRequest()
{
    ++m_requests;
    setTaskState(Busy);
}

void DapBackend::popRequest()
{
    if (m_requests > 0) {
        --m_requests;
    }
    setTaskState(m_requests > 0 ? Busy : Idle);
}

dap::settings::ClientSettings &DapBackend::target2dap(const DAPTargetConf &target)
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
    m_pathMap = target.dapSettings->pathMap;
    return *m_settings;
}

void DapBackend::start()
{
    if (m_state != None) {
        KMessageBox::error(nullptr, i18n("A debugging session is on course. Please, use re-run or stop the current session."));
        return;
    }
    unsetClient();

    m_client = new dap::Client(*m_settings, m_pathMap, this);

    Q_EMIT debuggerCapabilitiesChanged();

    // connect
    connect(m_client->bus(), &dap::Bus::error, this, &DapBackend::onError);

    connect(m_client, &dap::Client::finished, this, &DapBackend::onServerFinished);
    connect(m_client, &dap::Client::failed, [this] {
        onError(i18n("DAP backend '%1' failed", m_debuggerName));
        unsetClient();
    });

    connect(m_client, &dap::Client::serverDisconnected, this, &DapBackend::onServerDisconnected);
    connect(m_client, &dap::Client::debuggeeExited, this, &DapBackend::onProgramEnded);
    connect(m_client, &dap::Client::debuggeeTerminated, this, &DapBackend::onTerminated);
    connect(m_client, &dap::Client::debuggeeStopped, this, &DapBackend::onStopped);
    connect(m_client, &dap::Client::capabilitiesReceived, this, &DapBackend::onCapabilitiesReceived);
    connect(m_client, &dap::Client::debuggeeRunning, this, &DapBackend::onRunning);
    connect(m_client, &dap::Client::debuggeeContinued, this, &DapBackend::onContinuedEvent);
    connect(m_client, &dap::Client::debuggingProcess, this, &DapBackend::onDebuggingProcess);
    connect(m_client, &dap::Client::debuggeeRequiresTerminal, this, &DapBackend::debuggeeRequiresTerminal);

    connect(m_client, &dap::Client::threads, this, &DapBackend::onThreads);
    connect(m_client, &dap::Client::stackTrace, this, &DapBackend::onStackTrace);
    connect(m_client, &dap::Client::initialized, this, &DapBackend::onInitialized);
    connect(m_client, &dap::Client::errorResponse, this, &DapBackend::onErrorResponse);
    connect(m_client, &dap::Client::outputProduced, this, &DapBackend::onOutputProduced);

    connect(m_client, &dap::Client::threadChanged, this, &DapBackend::onThreadEvent);
    connect(m_client, &dap::Client::moduleChanged, this, &DapBackend::onModuleEvent);
    connect(m_client, &dap::Client::scopes, this, &DapBackend::onScopes);
    connect(m_client, &dap::Client::variables, this, &DapBackend::onVariables);
    connect(m_client, &dap::Client::modules, this, &DapBackend::onModules);
    connect(m_client, &dap::Client::sourceBreakpoints, this, &DapBackend::onSourceBreakpoints);
    connect(m_client, &dap::Client::breakpointChanged, this, &DapBackend::onBreakpointEvent);
    connect(m_client, &dap::Client::expressionEvaluated, this, &DapBackend::onExpressionEvaluated);
    connect(m_client, &dap::Client::gotoTargets, this, &DapBackend::onGotoTargets);

    m_client->bus()->start(m_settings->busSettings);
}

void DapBackend::runDebugger(const DAPTargetConf &conf)
{
    m_targetName = conf.targetName;
    m_debuggerName = conf.debugger;

    target2dap(conf);

    start();
}

void DapBackend::onTerminated(bool success)
{
    if (!isConnectedState()) {
        return;
    }

    if (success) {
        Q_EMIT outputText(printEvent(i18n("program terminated")));
        if (m_state < Terminated) {
            setState(Terminated);
        }
    } else {
        Q_EMIT outputError(i18n("Failed to terminate. Forcing shutdown..."));
        setState(Terminated);
    }
}

bool DapBackend::tryDisconnect()
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

void DapBackend::cmdShutdown()
{
    if (m_state == None) {
        return;
    }

    Q_EMIT outputError(newLine(i18n("requesting shutdown")));
    if (m_client) {
        m_client->bus()->close();
    } else {
        setState(None);
    }
}

bool DapBackend::tryTerminate()
{
    if (!isRunningState()) {
        return false;
    }

    if (!m_client->supportsTerminate()) {
        setState(Terminated);
        return false;
    }

    m_client->requestTerminate();
    return true;
}

void DapBackend::onError(const QString &message)
{
    Q_EMIT outputError(newLine(i18n("DAP backend: %1", message)));
    setState(None);
}

void DapBackend::onStopped(const dap::StoppedEvent &info)
{
    setState(Stopped);
    m_currentThread = info.threadId;

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
        Q_EMIT threadUpdated(dap::Thread(*m_currentThread), ThreadState::Stopped, true);
        pushRequest();
        m_client->requestStackTrace(*m_currentThread);
    }

    // request threads
    pushRequest();
    m_client->requestThreads();
}

void DapBackend::onContinuedEvent(const dap::ContinuedEvent &info)
{
    resetState();

    Q_EMIT threadUpdated(dap::Thread(info.threadId), ThreadState::Running, info.threadId == m_currentThread.value_or(-1));

    Q_EMIT outputText(printEvent(i18n("(continued) thread %1", QString::number(info.threadId))));
    if (info.allThreadsContinued) {
        Q_EMIT outputText(QStringLiteral(" (%1)").arg(i18n("all threads continued")));
    }
}

void DapBackend::onRunning()
{
    setState(State::Running);
    Q_EMIT outputText(printEvent(i18n("(running)")));
    // if there is not thread, request in case pause is called
    if (!m_currentThread) {
        pushRequest();
        m_client->requestThreads();
    }
}

void DapBackend::onThreads(const QList<dap::Thread> &threads, bool isError)
{
    if (!isError) {
        Q_EMIT this->threads(threads);
    }
    popRequest();
}

void DapBackend::informStackFrame()
{
    Q_EMIT stackFrameInfo(m_frames);
}

void DapBackend::onStackTrace(const int /* threadId */, const dap::StackTraceInfo &info)
{
    m_currentFrame = std::nullopt;
    m_frames = info.stackFrames;
    informStackFrame();

    if (!m_frames.isEmpty()) {
        changeStackFrame(0);
    }
    popRequest();
}

void DapBackend::onServerDisconnected()
{
    if (!isConnectedState()) {
        return;
    }

    if (!m_restart) {
        m_breakpoints.clear();
        m_wantedBreakpoints.clear();
    }

    setState(Disconnected);
}

void DapBackend::onServerFinished()
{
    Q_EMIT outputError(newLine(i18n("*** connection with server closed ***")));

    setState(None);
}

void DapBackend::onProgramEnded(int exitCode)
{
    Q_EMIT outputText(printEvent(i18n("program exited with code %1", exitCode)));
}

void DapBackend::onInitialized()
{
    if (!m_wantedBreakpoints.empty()) {
        for (const auto &[url, breakpoints] : m_wantedBreakpoints) {
            m_breakpoints[url].clear();
            pushRequest();
            m_client->requestSetBreakpoints(url, breakpoints, true);
        }
    }

    // Send ConfigurationDone request. This is the last request in init sequence
    // It indicates we have initialized fully
    m_client->requestConfigurationDone();

    shutdownUntil(None);
    Q_EMIT outputText(newLine(i18n("*** waiting for user actions ***")));
}

void DapBackend::onErrorResponse(const QString &summary, const std::optional<dap::Message> &message)
{
    Q_EMIT outputError(newLine(i18n("error on response: %1", summary)));
    if (message) {
        Q_EMIT outputError(QStringLiteral(" {code %1: %2}").arg(message->id).arg(message->format));
    }
}

void DapBackend::onOutputProduced(const dap::Output &output)
{
    if (output.output.isEmpty()) {
        return;
    }

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

void DapBackend::onDebuggingProcess(const dap::ProcessInfo &info)
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

void DapBackend::onThreadEvent(const dap::ThreadEvent &info)
{
    ThreadState state = ThreadState::Unknown;
    if (info.reason == QLatin1String("started")) {
        state = ThreadState::Started;
    } else if (info.reason == QLatin1String("exited")) {
        state = ThreadState::Exited;
    }
    Q_EMIT threadUpdated(dap::Thread(info.threadId), state, m_currentThread.value_or(-1) == info.threadId);

    Q_EMIT outputText(printEvent(QStringLiteral("(%1) %2").arg(info.reason).arg(i18n("thread %1", QString::number(info.threadId)))));
    // Request threads again with a debounce, some clients (flutter) send 0 threads the first time. Also, this keeps threads up to date
    m_requestThreadsTimer.start();
}

static QString printModule(const dap::Module &module)
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

static QString printBreakpoint(const QUrl &sourceId, const dap::SourceBreakpoint &def, const std::optional<dap::Breakpoint> &bp, const int bId)
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

    QStringList out = {QStringLiteral("[%1] %2: %3").arg(txtId).arg(formatUrl(sourceId)).arg(def.line)};
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

void DapBackend::onModuleEvent(const dap::ModuleEvent &info)
{
    if (info.reason == QStringLiteral("new")) {
        m_modules << info.module;
    } else if (info.reason == QStringLiteral("changed") || info.reason == QStringLiteral("removed")) {
        auto it = std::find_if(m_modules.begin(), m_modules.end(), [info](const dap::Module &m) {
            if (info.module.id_int.has_value() && m.id_int.has_value()) {
                return info.module.id_int.value() == m.id_int.value();
            } else if (info.module.id_str.has_value() && m.id_str.has_value()) {
                return info.module.id_str.value() == m.id_str.value();
            }
            return false;
        });
        if (it != m_modules.end()) {
            if (info.reason == u"removed") {
                m_modules.erase(it);
            } else {
                *it = info.module;
            }
        }
    }

    Q_EMIT outputText(printEvent(QStringLiteral("(%1) %2").arg(info.reason).arg(printModule(info.module))));
}

void DapBackend::changeScope(int scopeId)
{
    if (!m_client) {
        return;
    }

    if (m_currentScope && (*m_currentScope == scopeId)) {
        return;
    }

    m_currentScope = scopeId;

    // discard pending requests
    m_pendingVariableRequest.clear();
    m_pendingVariableRequest.push_back(scopeId);

    pushRequest();
    m_client->requestVariables(scopeId);
}

void DapBackend::requestVariable(int variablesReference)
{
    m_pendingVariableRequest.push_back(variablesReference);
    pushRequest();
    m_client->requestVariables(variablesReference);
}

void DapBackend::onScopes(const int /*frameId*/, const QList<dap::Scope> &scopes)
{
    m_currentScope = std::nullopt; // unset current scope as scopes have changed
    Q_EMIT scopesInfo(scopes);
    popRequest();
}

void DapBackend::onVariables(const int variablesReference, const QList<dap::Variable> &variables)
{
    auto it = std::find(m_pendingVariableRequest.begin(), m_pendingVariableRequest.end(), variablesReference);
    if (it == m_pendingVariableRequest.end()) {
        // discard responses to outdated requests
        return;
    }

    Q_EMIT variablesInfo(variablesReference, variables);

    popRequest();
}

void DapBackend::onModules(const dap::ModulesInfo &modules)
{
    m_modules = modules.modules;
    for (const auto &mod : modules.modules) {
        Q_EMIT outputText(newLine(printModule(mod)));
    }
    popRequest();
}

void DapBackend::informBreakpointAdded(const QUrl &path, const dap::Breakpoint &bpoint)
{
    if (bpoint.line) {
        Q_EMIT outputText(QStringLiteral("\n%1 %2:%3\n").arg(i18n("breakpoint set")).arg(formatUrl(path)).arg(bpoint.line.value()));
        // zero based line expected
        // Q_EMIT breakPointSet(path, bpoint.line.value());
    }
}

void DapBackend::informBreakpointRemoved(const QUrl &path, int line)
{
    Q_EMIT outputText(QStringLiteral("\n%1 %2:%3\n").arg(i18n("breakpoint cleared")).arg(formatUrl(path)).arg(line));
    // zero based line expected
    // Q_EMIT breakPointCleared(path, line);
}

void DapBackend::onSourceBreakpoints(const QUrl &path, int reference, const std::optional<QList<dap::Breakpoint>> &breakpoints)
{
    if (!breakpoints) {
        Q_EMIT outputText(QStringLiteral("Failed to set breakpoints"));
        popRequest();
        return;
    }

    const auto id = dap::Source::getUnifiedId(path, reference);
    if (id.isEmpty()) {
        popRequest();
        return;
    }

    if (m_breakpoints.find(id) == m_breakpoints.end()) {
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

    if (m_runToCursor) {
        // TODO
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
    } else {
        Q_EMIT breakPointsSet(path, breakpoints.value());
    }

    popRequest();

    if (mustContinue) {
        slotContinue();
    }
}

void DapBackend::onBreakpointEvent(const dap::BreakpointEvent &info)
{
    if (info.reason == QStringLiteral("new")) {
        Q_EMIT breakpointEvent(info.breakpoint, BreakpointEventKind::New);
    } else if (info.reason == QStringLiteral("changed")) {
        Q_EMIT breakpointEvent(info.breakpoint, BreakpointEventKind::Changed);
    } else if (info.reason == QStringLiteral("removed")) {
        Q_EMIT breakpointEvent(info.breakpoint, BreakpointEventKind::Removed);
    } else {
        QStringList parts = {i18n("(%1) breakpoint", info.reason)};
        if (info.breakpoint.source) {
            parts << QStringLiteral(" ") << formatUrl(info.breakpoint.source->unifiedId());
        }
        if (info.breakpoint.line) {
            parts << QStringLiteral(":%1").arg(info.breakpoint.line.value());
        }

        Q_EMIT outputText(printEvent(parts.join(QString())));
    }
}

void DapBackend::onExpressionEvaluated(const QString &expression, const std::optional<dap::EvaluateInfo> &info)
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

void DapBackend::onGotoTargets(const dap::Source &source, const int, const QList<dap::GotoTarget> &targets)
{
    if (!targets.isEmpty() && m_currentThread) {
        Q_EMIT outputError(newLine(QStringLiteral("jump target %1:%2 (%3)").arg(formatUrl(source.unifiedId())).arg(targets[0].line).arg(targets[0].label)));
        m_client->requestGoto(*m_currentThread, targets[0].id);
    }
    popRequest();
}

void DapBackend::onCapabilitiesReceived(const dap::Capabilities &capabilities)
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

bool DapBackend::supportsMovePC() const
{
    return isRunningState() && m_client && m_client->adapterCapabilities().supportsGotoTargetsRequest;
}

bool DapBackend::supportsRunToCursor() const
{
    return isAttachedState() && m_client && m_client->adapterCapabilities().supportsHitConditionalBreakpoints;
}

bool DapBackend::isConnectedState() const
{
    return m_client && (m_state != None) && (m_state != Disconnected);
}

bool DapBackend::isAttachedState() const
{
    return isConnectedState() && (m_state != Terminated);
}

bool DapBackend::isRunningState() const
{
    return (m_state == Running) || (m_state == Stopped);
}

bool DapBackend::canSetBreakpoints() const
{
    return isAttachedState();
}

bool DapBackend::canMove() const
{
    return isRunningState();
}

bool DapBackend::canContinue() const
{
    return (m_state == Initializing) || (m_state == Stopped);
}

bool DapBackend::canHotReload() const
{
    return m_debuggerName == QStringLiteral("flutter") && debuggerRunning();
}

bool DapBackend::canHotRestart() const
{
    return m_debuggerName == QStringLiteral("flutter") && debuggerRunning();
}

bool DapBackend::debuggerRunning() const
{
    return m_client && (m_state != None);
}

bool DapBackend::debuggerBusy() const
{
    return debuggerRunning() && (m_task == Busy);
}

std::optional<int> DapBackend::findBreakpoint(const QUrl &path, int line) const
{
    if (m_breakpoints.find(path) == m_breakpoints.end()) {
        return std::nullopt;
    }

    const auto &bpoints = m_breakpoints.at(path);
    int index = 0;
    for (const auto &bp : bpoints) {
        if (bp && bp->line && (line == bp->line)) {
            return index;
        }
        ++index;
    }
    return std::nullopt;
}

std::optional<int> DapBackend::findBreakpointIntent(const QUrl &path, int line) const
{
    if ((m_wantedBreakpoints.find(path) == m_wantedBreakpoints.end()) || (m_breakpoints.find(path) == m_breakpoints.end())) {
        return std::nullopt;
    }

    const auto &bintents = m_wantedBreakpoints.at(path);
    const auto &bpoints = m_breakpoints.at(path);
    int index = 0;
    for (const auto &bi : bintents) {
        if ((bi.line == line)) {
            const auto &bp = bpoints[index];
            // If the breakpoint is resolved to another line, ignore
            if (!bp || (bp->line.value_or(line) == line)) {
                return index;
            }
        }
        ++index;
    }
    return std::nullopt;
}

void DapBackend::setBreakpoints(const QUrl &url, const QList<dap::SourceBreakpoint> &breakpoints)
{
    if (m_task != Idle) {
        // ??
        return;
    }

    const auto path = resolveOrWarn(url);

    m_wantedBreakpoints[path] = breakpoints;

    pushRequest();
    m_client->requestSetBreakpoints(path, m_wantedBreakpoints[path], true);
}

bool DapBackend::removeBreakpoint(const QUrl &path, int line)
{
    bool informed = false;
    // clear all breakpoints in the same line (there can be more than one)
    auto index = findBreakpoint(path, line);
    while (index) {
        m_wantedBreakpoints[path].removeAt(*index);
        m_breakpoints[path].removeAt(*index);
        if (!informed) {
            informBreakpointRemoved(path, line);
            informed = true;
        }
        index = findBreakpoint(path, line);
    }
    // clear all breakpoint intents in the same line
    index = findBreakpointIntent(path, line);
    while (index) {
        m_wantedBreakpoints[path].removeAt(*index);
        m_breakpoints[path].removeAt(*index);
        if (!informed) {
            informBreakpointRemoved(path, line);
            informed = true;
        }
        index = findBreakpointIntent(path, line);
    }

    if (!informed) {
        return false;
    }

    // update breakpoint table for this file
    pushRequest();
    m_client->requestSetBreakpoints(path, m_wantedBreakpoints[path], true);

    return true;
}

void DapBackend::insertBreakpoint(const QUrl &path, int line)
{
    if (m_wantedBreakpoints.find(path) == m_wantedBreakpoints.end()) {
        m_wantedBreakpoints[path] = {dap::SourceBreakpoint(line)};
        m_breakpoints[path] = {std::nullopt};
    } else {
        m_wantedBreakpoints[path] << dap::SourceBreakpoint(line);
        m_breakpoints[path] << std::nullopt;
    }

    pushRequest();
    m_client->requestSetBreakpoints(path, m_wantedBreakpoints[path], true);
}

void DapBackend::movePC(QUrl const &url, int line)
{
    if (!m_client) {
        return;
    }

    if (m_state != State::Stopped) {
        return;
    }

    if (!m_currentThread) {
        return;
    }

    if (!m_client->adapterCapabilities().supportsGotoTargetsRequest) {
        return;
    }

    const auto path = resolveOrWarn(url);

    pushRequest();
    m_client->requestGotoTargets(path, line);
}

void DapBackend::runToCursor(QUrl const &url, int line)
{
    if (!m_client) {
        return;
    }

    if (!m_client->adapterCapabilities().supportsHitConditionalBreakpoints) {
        return;
    }

    const auto path = resolveOrWarn(url);

    dap::SourceBreakpoint bp(line);
    bp.hitCondition = QStringLiteral("<=1");

    if (m_wantedBreakpoints.find(path) == m_wantedBreakpoints.end()) {
        m_wantedBreakpoints[path] = {std::move(bp)};
        m_breakpoints[path] = {std::nullopt};
    } else {
        m_wantedBreakpoints[path] << std::move(bp);
        m_breakpoints[path] << std::nullopt;
    }

    m_runToCursor = Cursor{.line = line, .path = path};
    pushRequest();
    m_client->requestSetBreakpoints(path, m_wantedBreakpoints[path], true);
}

void DapBackend::cmdEval(const QString &cmd)
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
    if (m_currentFrame) {
        frameId = m_frames[*m_currentFrame].id;
    }

    pushRequest();
    m_client->requestWatch(expression, frameId);
}

void DapBackend::cmdJump(const QString &cmd)
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
    auto url = QUrl::fromLocalFile(path);
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
        url = frame.source->unifiedId();
    }

    this->movePC(url, line);
}

void DapBackend::cmdRunToCursor(const QString &cmd)
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
    auto url = QUrl::fromLocalFile(path);
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
        url = frame.source->unifiedId();
    }

    this->runToCursor(url, line);
}

void DapBackend::cmdPause(const QString &cmd)
{
    if (!m_client) {
        return;
    }

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

void DapBackend::cmdContinue(const QString &cmd)
{
    if (!m_client) {
        return;
    }

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

void DapBackend::cmdStepIn(const QString &cmd)
{
    if (!m_client) {
        return;
    }

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

void DapBackend::cmdStepOut(const QString &cmd)
{
    if (!m_client) {
        return;
    }

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

void DapBackend::cmdNext(const QString &cmd)
{
    if (!m_client) {
        return;
    }

    const static QRegularExpression rx_next(QStringLiteral(R"--(^n[a-z]*(?:\s+(?P<ONLY>only))?(?:\s+(?P<ID>\d+))?\s*$)--"));

    const auto match = rx_next.match(cmd);
    if (!match.hasMatch()) {
        Q_EMIT outputError(newLine(i18n("syntax error: %1", cmd)));
        return;
    }

    const auto &txtThread = match.captured(QStringLiteral("ID"));

    int threadId{};

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

void DapBackend::cmdHelp(const QString & /*cmd*/)
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

void DapBackend::cmdListModules(const QString &)
{
    if (!m_client) {
        return;
    }

    if (!m_client->adapterCapabilities().supportsModulesRequest) {
        return;
    }

    pushRequest();
    m_modules.clear();
    m_client->requestModules();
}

void DapBackend::cmdListBreakpoints(const QString &)
{
    int bId = 0;
    for (const auto &[url, breakpoints] : m_breakpoints) {
        const auto &sourceId = url;
        const auto &defs = m_wantedBreakpoints[sourceId];
        int pointIdx = 0;
        for (const auto &b : breakpoints) {
            const auto &def = defs[pointIdx];
            Q_EMIT outputText(newLine(printBreakpoint(sourceId, def, b, bId)));
            ++pointIdx;
            ++bId;
        }
    }
}

void DapBackend::cmdBreakpointOn(const QString &cmd)
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
    QString cpath = match.captured(QStringLiteral("SOURCE"));
    QUrl path;
    if (cpath.isNull()) {
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
        path = resolveOrWarn(QUrl::fromLocalFile(cpath));
    }

    if (findBreakpoint(path, bp.line) || findBreakpointIntent(path, bp.line)) {
        Q_EMIT outputError(newLine(i18n("line %1 already has a breakpoint", txtLine)));
        return;
    }

    m_wantedBreakpoints[path] << std::move(bp);
    m_breakpoints[path] << std::nullopt;

    pushRequest();
    m_client->requestSetBreakpoints(path, m_wantedBreakpoints[path], true);
}

void DapBackend::cmdBreakpointOff(const QString &cmd)
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

    QString cpath = match.captured(2);
    QUrl path = QUrl::fromLocalFile(cpath);
    if (cpath.isNull()) {
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

    if (!removeBreakpoint(path, line)) {
        Q_EMIT outputError(newLine(i18n("breakpoint not found (%1:%2)", formatUrl(path), line)));
    }
}

void DapBackend::cmdWhereami(const QString &)
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
    default:
        parts << i18n("none");
        break;
    }

    Q_EMIT outputText(parts.join(QString()));
}

void DapBackend::issueCommand(QString const &command)
{
    if (!m_client) {
        return;
    }

    if (m_task == Busy) {
        m_commandQueue << command;
        return;
    }

    QString cmd = command.trimmed();

    if (cmd.isEmpty()) {
        return;
    }

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

QString DapBackend::targetName() const
{
    return m_targetName;
}

void DapBackend::setFileSearchPaths(const QStringList & /*paths*/)
{
    // TODO
}

void DapBackend::setPendingBreakpoints(std::map<QUrl, QList<dap::SourceBreakpoint>> breakpoints)
{
    // these are set during initialization
    Q_ASSERT(m_wantedBreakpoints.empty());
    m_wantedBreakpoints = std::move(breakpoints);
}

QList<dap::Module> DapBackend::modules()
{
    return m_modules;
}

void DapBackend::slotInterrupt()
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

void DapBackend::slotStepInto()
{
    if (!m_client) {
        return;
    }

    if (m_state != State::Stopped) {
        return;
    }

    if (!m_currentThread) {
        return;
    }

    m_client->requestStepIn(*m_currentThread);
}

void DapBackend::slotStepOut()
{
    if (!m_client) {
        return;
    }

    if (m_state != State::Stopped) {
        return;
    }

    if (!m_currentThread) {
        return;
    }

    m_client->requestStepOut(*m_currentThread);
}

void DapBackend::slotStepOver()
{
    if (!m_client) {
        return;
    }

    if (m_state != State::Stopped) {
        return;
    }

    if (!m_currentThread) {
        return;
    }

    m_client->requestNext(*m_currentThread);
}

void DapBackend::slotContinue()
{
    if (!isAttachedState()) {
        return;
    }

    if (m_currentThread) {
        m_client->requestContinue(*m_currentThread);
    }
}

void DapBackend::shutdownUntil(std::optional<State> state)
{
    if (!state) {
        m_shutdown.target = std::nullopt;
    } else if (!m_shutdown.target || (*state > m_shutdown.target)) {
        // propagate until the deepest state
        m_shutdown.target = state;
    }
}

bool DapBackend::continueShutdown() const
{
    return m_restart || (m_shutdown.target && (m_shutdown.target > m_state || m_shutdown.target == None));
}

void DapBackend::slotKill()
{
    if (!isConnectedState()) {
        setState(None);
        Q_EMIT readyForInput(false);
        return;
    }

    if (isRunningState()) {
        shutdownUntil(None);
        tryTerminate();
    } else {
        shutdownUntil(None);
        tryDisconnect();
    }
}

void DapBackend::slotReRun()
{
    if (!m_client && m_settings) {
        start();
        return;
    }

    m_restart = true;
    slotKill();
}

void DapBackend::slotQueryLocals(bool)
{
}

QString DapBackend::slotPrintVariable(const QString &variable)
{
    const auto cmd = QStringLiteral("print %1").arg(variable);
    issueCommand(cmd);
    return cmd;
}

void DapBackend::slotHotReload()
{
    m_client->requestHotReload();
}

void DapBackend::slotHotRestart()
{
    m_client->requestHotRestart();
}

void DapBackend::changeStackFrame(int index)
{
    if (!debuggerRunning()) {
        return;
    }

    if ((m_frames.size() < index) || (index < 0)) {
        return;
    }

    if (m_currentFrame && (*m_currentFrame == index)) {
        return;
    }

    m_currentFrame = index;

    const auto &frame = m_frames[index];
    if (frame.source) {
        const auto id = frame.source->unifiedId();
        Q_EMIT outputText(QStringLiteral("\n")
                          + i18n("Current frame [%3]: %1:%2 (%4)", formatUrl(id), QString::number(frame.line), QString::number(index), frame.name));
        // zero-based line
        Q_EMIT debugLocationChanged(resolveOrWarn(id), frame.line);
    }

    Q_EMIT stackFrameChanged(index);

    m_client->requestScopes(m_frames[*m_currentFrame].id);
}

void DapBackend::changeThread(int index)
{
    if (!debuggerRunning()) {
        return;
    }

    if (m_currentThread && (*m_currentThread == index)) {
        return;
    }

    m_currentThread = index;

    pushRequest();
    m_client->requestStackTrace(index);
}

std::optional<QUrl> DapBackend::resolveFilename(const QUrl &file, bool fallback) const
{
    // consider absolute, as below
    if (!file.isLocalFile()) {
        return file;
    }

    auto filename = file.path();
    QFileInfo fInfo = QFileInfo(filename);
    if (fInfo.exists() && fInfo.isDir()) {
        return QUrl::fromLocalFile(fInfo.absoluteFilePath());
    }

    if (fInfo.isAbsolute()) {
        return file;
    }

    // working path
    if (!m_workDir.isEmpty()) {
        const auto base = QDir(m_workDir);
        fInfo = QFileInfo(base.absoluteFilePath(filename));
        if (fInfo.exists() && !fInfo.isDir()) {
            return QUrl::fromLocalFile(fInfo.absoluteFilePath());
        }
    }

    // executable path
    if (!m_file.isEmpty()) {
        const auto base = QDir(QFileInfo(m_file).absolutePath());
        fInfo = QFileInfo(base.absoluteFilePath(filename));
        if (fInfo.exists() && !fInfo.isDir()) {
            return QUrl::fromLocalFile(fInfo.absoluteFilePath());
        }
    }

    if (fallback) {
        return file;
    }

    return std::nullopt;
}

QUrl DapBackend::resolveOrWarn(const QUrl &filename)
{
    const auto path = resolveFilename(filename, false);

    if (path) {
        return *path;
    }

    Q_EMIT sourceFileNotFound(filename.path());

    return filename;
}

#include "moc_dapbackend.cpp"
