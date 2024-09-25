//
// gdbbackend.h
//
// Description: Manages the interaction with GDB
//
//
// SPDX-FileCopyrightText: 2008-2010 Ian Wakeling <ian.wakeling@ntlworld.com>
// SPDX-FileCopyrightText: 2010 Kåre Särs <kare.sars@iki.fi>
//
//  SPDX-License-Identifier: LGPL-2.0-only

#pragma once

#include <QObject>

#include <QList>
#include <QProcess>
#include <QUrl>
#include <optional>
#include <sys/types.h>

#include "backendinterface.h"
#include "configview.h"
#include "gdbmi/parser.h"
#include "gdbmi/records.h"
#include "gdbvariableparser.h"

namespace dap
{
struct StackFrame;
}

struct GdbCommand {
    QStringList arguments;
    enum RequestType {
        None,
        BreakpointList,
        Continue,
        Step,
        ThreadInfo,
        StackListFrames,
        StackListVariables,
        BreakInsert,
        BreakDelete,
        ListFeatures,
        DataEvaluateExpression,
        InfoGdbMiCommand,
        Exit,
        Kill,
        LldbVersion,
        RegisterNames,
        RegisterValues,
        ChangedRegisters,
    };
    RequestType type = None;
    std::optional<QJsonValue> data = std::nullopt;

    bool isMachineInterface() const;
    bool check(const QString &command) const;
    bool check(const QString &part1, const QString &part2) const;
    static GdbCommand parse(const QString &request);
};

struct BreakPoint {
    int number;
    QUrl file;
    int line;

    static BreakPoint parse(const QJsonObject &);
};

class GdbBackend : public BackendInterface
{
    Q_OBJECT
public:
    GdbBackend(QObject *parent);
    ~GdbBackend() override;

    void runDebugger(const GDBTargetConf &conf, const QStringList &ioFifos);
    bool debuggerRunning() const override;
    bool debuggerBusy() const override;
    bool hasBreakpoint(QUrl const &url, int line) const override;

    bool supportsMovePC() const override;
    bool supportsRunToCursor() const override;
    bool canSetBreakpoints() const override;
    bool canMove() const override;
    bool canContinue() const override;

    void toggleBreakpoint(QUrl const &url, int line, bool *added = nullptr) override;
    void movePC(QUrl const &url, int line) override;
    void runToCursor(QUrl const &url, int line) override;

    void issueCommand(QString const &cmd) override;

    QString targetName() const override;
    void setFileSearchPaths(const QStringList &paths) override;

public Q_SLOTS:
    void slotInterrupt() override;
    void slotStepInto() override;
    void slotStepOver() override;
    void slotStepOut() override;
    void slotContinue() override;
    void slotKill() override;
    void slotReRun() override;
    QString slotPrintVariable(const QString &variable) override;

    void slotQueryLocals(bool display) override;
    void changeStackFrame(int index) override;
    void changeThread(int index) override;
    void changeScope(int scopeId) override;

private Q_SLOTS:
    void slotError();
    void slotReadDebugStdOut();
    void slotReadDebugStdErr();
    void slotDebugFinished(int exitCode, QProcess::ExitStatus status);
    void issueNextCommand();

private:
    enum State { none, ready, executingCmd };

    enum GdbState { Disconnected, Connected, Running, Stopped };

private:
    QUrl resolveFileName(const QString &fileName, bool silent = false);

    void setState(State newState, std::optional<GdbState> newGdbState = std::nullopt);
    void setGdbState(GdbState newState);
    void resetSession();
    void clearFrames();
    void clearVariables();
    void updateInspectable(bool inspectable);

    BreakPoint parseBreakpoint(const QJsonObject &item);
    int findFirstBreakpoint(const QUrl &url, int line) const;
    QStringList findAllBreakpoints(const QUrl &url, int line) const;
    void deleteBreakpoint(const int bpNumber);
    void insertBreakpoint(const QJsonObject &item);
    QString makeCmdBreakInsert(const QUrl &url, int line, bool pending = false, bool temporal = false) const;
    QString makeFrameFlags() const;

    bool inferiorRunning() const;
    void onMIParserError(const QString &error);
    void processMIStreamOutput(const gdbmi::StreamOutput &record);
    void processMIRecord(const gdbmi::Record &record);
    void processMIExec(const gdbmi::Record &record);
    void processMINotify(const gdbmi::Record &record);
    void processMIResult(const gdbmi::Record &record);
    void processMIPrompt();
    bool responseMIBreakpointList(const gdbmi::Record &record);
    bool responseMIThreadInfo(const gdbmi::Record &record);
    bool responseMIStackListFrames(const gdbmi::Record &record);
    bool responseMIStackListVariables(const gdbmi::Record &record);
    bool responseMIBreakInsert(const gdbmi::Record &record);
    bool responseMIBreakDelete(const gdbmi::Record &record, const QStringList &args);
    void notifyMIBreakpointDeleted(const gdbmi::Record &record);
    void notifyMIBreakpointModified(const gdbmi::Record &record);
    bool responseMIListFeatures(const gdbmi::Record &record);
    bool responseMIDataEvaluateExpression(const gdbmi::Record &record, const std::optional<QJsonValue> &data);
    bool responseMIRegisterNames(const gdbmi::Record &record);
    bool responseMIRegisterValues(const gdbmi::Record &record);
    bool responseMIChangedRegisters(const gdbmi::Record &record);
    bool responseMIExit(const gdbmi::Record &record);
    bool responseMIKill(const gdbmi::Record &record);
    bool responseMIInfoGdbCommand(const gdbmi::Record &record, const QStringList &args);
    bool responseMILldbVersion(const gdbmi::Record &record);
    void responseMIScopes(const gdbmi::Record &record);
    void responseMIThisScope(const gdbmi::Record &record);
    void informStackFrame();
    dap::StackFrame parseFrame(const QJsonObject &object);

    void enqueueScopeVariables();
    void enqueueScopes();
    void enqueueProtocolHandshake();
    QStringList makeInitSequence();
    void enqueueThreadInfo();
    QStringList makeRunSequence(bool stop);
    void enqueue(const QString &command);
    void enqueue(const QString &command, const QJsonValue &data, uint8_t captureMode = CaptureMode::Default);
    void enqueue(const QStringList &commands, bool prepend = false);
    void prepend(const QString &command);
    void issueCommand(const QString &cmd, const std::optional<QJsonValue> &data, uint8_t captureMode = CaptureMode::Default);
    void issueNextCommandLater(const std::optional<State> &state);
    void updateInputReady(bool newState, bool force = false);
    void clearDebugLocation();

    void cmdKateInit();
    void cmdKateTryRun(const GdbCommand &cmd, const QJsonValue &data);

private:
    QProcess m_debugProcess;
    GDBTargetConf m_targetConf;
    QString m_ioPipeString;

    State m_state;

    struct PendingCommand {
        QString command;
        std::optional<QJsonValue> data;
        uint8_t captureMode;
    };
    QList<PendingCommand> m_nextCommands;
    QString m_lastCommand;
    bool m_debugLocationChanged;
    QHash<int, BreakPoint> m_breakpointTable;
    QByteArray m_outBuffer;
    QString m_errBuffer;
    bool m_queryLocals;

    GDBVariableParser m_variableParser;

    gdbmi::GdbmiParser *m_parser;
    QHash<int, GdbCommand> m_requests;
    int m_seq = 0;
    GdbState m_gdbState = Disconnected;
    QList<dap::StackFrame> m_stackFrames;
    QList<QString> m_registerNames;
    QSet<int> m_changedRegisters;
    bool m_lastInputReady = false;
    bool m_pointerThis = false;

    enum CaptureMode { Default = 0x0, CaptureConsole = 0x1, MuteLog = 0x2 };
    uint8_t m_captureOutput = Default;
    QStringList m_capturedOutput;
    bool m_inspectable = false;
    std::optional<int> m_currentThread;
    std::optional<int> m_currentFrame;
    std::optional<int> m_currentScope;
    std::optional<int> m_watchedScope;
    int m_errorCounter = 0;

    enum DebuggerFamily { Unknown, GDB, LLDB };
    struct {
        DebuggerFamily family;
        std::optional<bool> async;
        std::optional<bool> execRunStart;
        std::optional<bool> threadInfo;
        std::optional<bool> breakList;
        std::optional<bool> pendingBreakpoints;
        std::optional<bool> execJump;
        std::optional<bool> changedRegisters;
    } m_capabilities;
};
