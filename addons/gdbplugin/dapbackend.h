/*
    SPDX-FileCopyrightText: 2022 Héctor Mesa Jiménez <wmj.py@gmx.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include <QObject>
#include <QTimer>

#include "backendinterface.h"
#include "configview.h"
#include "dap/client.h"
#include "dap/entities.h"
#include "dap/settings.h"
#include <optional>

class DapBackend final : public BackendInterface
{
    Q_OBJECT
public:
    DapBackend(QObject *parent);
    ~DapBackend() override;

    void runDebugger(const DAPTargetConf &conf);
    bool debuggerRunning() const override;
    bool debuggerBusy() const override;
    bool hasBreakpoint(QUrl const &url, int line) const override;

    bool supportsMovePC() const override;
    bool supportsRunToCursor() const override;
    bool canSetBreakpoints() const override;
    bool canMove() const override;
    bool canContinue() const override;
    bool canHotReload() const;
    bool canHotRestart() const;

    void toggleBreakpoint(QUrl const &url, int line, bool *) override;
    void movePC(QUrl const &url, int line) override;
    void runToCursor(QUrl const &url, int line) override;

    void issueCommand(QString const &cmd) override;

    QString targetName() const override;
    void setFileSearchPaths(const QStringList &paths) override;

    void setPendingBreakpoints(const QHash<QUrl, QList<int>> &breakpoints);

public Q_SLOTS:
    void slotInterrupt() override;
    void slotStepInto() override;
    void slotStepOver() override;
    void slotStepOut() override;
    void slotContinue() override;
    void slotKill() override;
    void slotReRun() override;
    QString slotPrintVariable(const QString &variable) override;
    void slotHotReload();
    void slotHotRestart();

    void slotQueryLocals(bool display) override;
    void changeStackFrame(int index) override;
    void changeThread(int index) override;
    void changeScope(int scopeId) override;
    void requestVariable(int variablesReference) override;

private:
    enum State {
        None,
        Initializing,
        Running,
        Stopped,
        Terminated,
        Disconnected
    };
    enum Task {
        Idle,
        Busy
    };

    void unsetClient();

    void onError(const QString &message);
    void onTerminated(bool success);
    void onStopped(const dap::StoppedEvent &info);
    void onThreads(const QList<dap::Thread> &threads, bool isError);
    void onStackTrace(int, const dap::StackTraceInfo &info);
    void onProgramEnded(int exitCode);
    void onRunning();
    void onContinuedEvent(const dap::ContinuedEvent &info);
    void onInitialized();
    void onErrorResponse(const QString &summary, const std::optional<dap::Message> &message);
    void onOutputProduced(const dap::Output &output);
    void onDebuggingProcess(const dap::ProcessInfo &info);
    void onThreadEvent(const dap::ThreadEvent &info);
    void onModuleEvent(const dap::ModuleEvent &info);
    void onScopes(int frameId, const QList<dap::Scope> &scopes);
    void onVariables(int variablesReference, const QList<dap::Variable> &variables);
    void onModules(const dap::ModulesInfo &modules);
    void onSourceBreakpoints(const QUrl &path, int reference, const std::optional<QList<dap::Breakpoint>> &breakpoints);
    void onBreakpointEvent(const dap::BreakpointEvent &info);
    void onExpressionEvaluated(const QString &expression, const std::optional<dap::EvaluateInfo> &info);
    void onGotoTargets(const dap::Source &, int, const QList<dap::GotoTarget> &targets);
    void onCapabilitiesReceived(const dap::Capabilities &capabilities);
    void onServerDisconnected();
    void onServerFinished();

    bool tryTerminate();
    bool tryDisconnect();
    void cmdShutdown();
    void cmdHelp(const QString &cmd);
    void cmdEval(const QString &cmd);
    void cmdJump(const QString &cmd);
    void cmdRunToCursor(const QString &cmd);
    void cmdListModules(const QString &cmd);
    void cmdListBreakpoints(const QString &cmd);
    void cmdBreakpointOn(const QString &cmd);
    void cmdBreakpointOff(const QString &cmd);
    void cmdPause(const QString &cmd);
    void cmdContinue(const QString &cmd);
    void cmdNext(const QString &cmd);
    void cmdStepIn(const QString &cmd);
    void cmdStepOut(const QString &cmd);
    void cmdWhereami(const QString &cmd);

    void resetState(State state = State::Running);
    void setState(State state);
    void setTaskState(Task state);

    bool isConnectedState() const;
    bool isAttachedState() const;
    bool isRunningState() const;

    void pushRequest();
    void popRequest();

    QUrl resolveOrWarn(const QUrl &filename);
    std::optional<QUrl> resolveFilename(const QUrl &filename, bool fallback = true) const;
    dap::settings::ClientSettings &target2dap(const DAPTargetConf &target);
    std::optional<int> findBreakpoint(const QUrl &path, int line) const;
    std::optional<int> findBreakpointIntent(const QUrl &path, int line) const;

    void insertBreakpoint(const QUrl &path, int line);
    // return false if nothing found
    bool removeBreakpoint(const QUrl &path, int line);
    void informBreakpointAdded(const QUrl &path, const dap::Breakpoint &bpoint);
    void informBreakpointRemoved(const QUrl &path, int line);
    void clearBreakpoints();
    void informStackFrame();

    QString m_targetName;
    QString m_debuggerName;

    void start();

    dap::Client *m_client = nullptr;
    std::optional<dap::settings::ClientSettings> m_settings;
    State m_state;
    Task m_task;

    QString m_file;
    QString m_workDir;
    std::optional<int> m_currentThread;
    std::optional<int> m_currentFrame;
    std::optional<int> m_currentScope;
    bool m_restart = false;

    struct {
        std::optional<State> target = std::nullopt;
    } m_shutdown;

    void shutdownUntil(std::optional<State> state = std::nullopt);
    bool continueShutdown() const;

    struct Cursor {
        int line;
        QUrl path;
    };
    std::optional<Cursor> m_runToCursor;

    int m_requests;

    QStringList m_commandQueue;

    std::map<QUrl, QList<std::optional<dap::Breakpoint>>> m_breakpoints;
    std::map<QUrl, QList<dap::SourceBreakpoint>> m_wantedBreakpoints;
    QList<dap::StackFrame> m_frames;
    QTimer m_requestThreadsTimer;

    std::vector<int> m_pendingVariableRequest;
};
