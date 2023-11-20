//
// debugview.cpp
//
// Description: Manages the interaction with GDB
//
//
// SPDX-FileCopyrightText: 2008-2010 Ian Wakeling <ian.wakeling@ntlworld.com>
// SPDX-FileCopyrightText: 2011 Kåre Särs <kare.sars@iki.fi>
//
//  SPDX-License-Identifier: LGPL-2.0-only

#include "debugview.h"
#include "gdbmi/tokens.h"
#include "hostprocess.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTimer>

#include <KLocalizedString>
#include <KMessageBox>

#include <algorithm>
#include <signal.h>
#include <stdlib.h>

// #define DEBUG_GDBMI

static const dap::Scope LocalScope(0, i18n("Locals"));
static const dap::Scope ThisScope(1, QStringLiteral("*this"));
static const dap::Scope RegistersScope(2, i18n("CPU registers"));

static constexpr int DATA_EVAL_THIS_CHECK = 0;
static constexpr int DATA_EVAL_THIS_DEREF = 1;

static constexpr int MAX_RESPONSE_ERRORS = 5;

static QString printEvent(const QString &text)
{
    return QStringLiteral("--> %1\n").arg(text);
}

bool GdbCommand::isMachineInterface() const
{
    return !arguments.isEmpty() && arguments.first().startsWith(QLatin1Char('-'));
}

GdbCommand GdbCommand::parse(const QString &request)
{
    GdbCommand cmd;

    cmd.arguments = QProcess::splitCommand(request);
    if (!cmd.arguments.isEmpty()) {
        const auto parts = gdbmi::GdbmiParser::splitCommand(cmd.arguments.at(0));
        if (parts.size() > 1) {
            cmd.arguments.replace(0, parts.last());
        }
    }

    return cmd;
}

bool GdbCommand::check(const QString &command) const
{
    return !arguments.isEmpty() && (arguments.first() == command);
}

bool GdbCommand::check(const QString &part1, const QString &part2) const
{
    return (arguments.size() >= 2) && (arguments.first() == part1) && (arguments.at(1) == part2);
}

void DebugView::enqueue(const QString &command)
{
    m_nextCommands << PendingCommand{command, std::nullopt, Default};
}

void DebugView::enqueue(const QString &command, const QJsonValue &data, uint8_t captureMode)
{
    m_nextCommands << PendingCommand{command, data, captureMode};
}

void DebugView::prepend(const QString &command)
{
    m_nextCommands.prepend({command, std::nullopt, Default});
}

DebugView::DebugView(QObject *parent)
    : BackendInterface(parent)
    , m_debugProcess(nullptr)
    , m_state(none)
    , m_debugLocationChanged(true)
    , m_queryLocals(false)
    , m_parser(new gdbmi::GdbmiParser(this))
{
    // variable parser
    connect(&m_variableParser, &GDBVariableParser::variable, this, &BackendInterface::variableInfo);

    connect(m_parser, &gdbmi::GdbmiParser::outputProduced, this, &DebugView::processMIStreamOutput);
    connect(m_parser, &gdbmi::GdbmiParser::recordProduced, this, &DebugView::processMIRecord);
    connect(m_parser, &gdbmi::GdbmiParser::parserError, this, &DebugView::onMIParserError);
}

DebugView::~DebugView()
{
    if (m_debugProcess.state() != QProcess::NotRunning) {
        m_debugProcess.kill();
        m_debugProcess.blockSignals(true);
        m_debugProcess.waitForFinished();
    }
    disconnect(m_parser);
    m_parser->deleteLater();
}

bool DebugView::supportsMovePC() const
{
    return m_capabilities.execJump.value_or(false) && canMove();
}

bool DebugView::supportsRunToCursor() const
{
    return canMove();
}

bool DebugView::canSetBreakpoints() const
{
    return m_gdbState != Disconnected;
}

bool DebugView::canMove() const
{
    return (m_gdbState == Connected) || (m_gdbState == Stopped);
}

bool DebugView::canContinue() const
{
    return canMove();
}

void DebugView::resetSession()
{
    m_nextCommands.clear();
    m_currentThread.reset();
    m_stackFrames.clear();
    m_registerNames.clear();
}

void DebugView::runDebugger(const GDBTargetConf &conf, const QStringList &ioFifos)
{
    // TODO correct remote flow (connected, interrupt, etc.)
    if (conf.executable.isEmpty()) {
        const QString msg = i18n("Please set the executable in the 'Settings' tab in the 'Debug' panel.");
        Q_EMIT backendError(msg, KTextEditor::Message::Error);
        return;
    }

    m_targetConf = conf;

    // no chance if no debugger configured
    if (m_targetConf.gdbCmd.isEmpty()) {
        const QString msg = i18n("No debugger selected. Please select one in the 'Settings' tab in the 'Debug' panel.");
        Q_EMIT backendError(msg, KTextEditor::Message::Error);
        return;
    }

    // only run debugger from PATH or the absolute executable path we specified
    const auto fullExecutable = QFileInfo(m_targetConf.gdbCmd).isAbsolute() ? m_targetConf.gdbCmd : safeExecutableName(m_targetConf.gdbCmd);
    if (fullExecutable.isEmpty()) {
        const QString msg =
            i18n("Debugger not found. Please make sure you have it installed in your system. The selected debugger is '%1'", m_targetConf.gdbCmd);
        Q_EMIT backendError(msg, KTextEditor::Message::Error);
        return;
    }

    if (ioFifos.size() == 3) {
        // XXX only supported by GDB, not LLDB
        m_ioPipeString = QStringLiteral("< %1 1> %2 2> %3").arg(ioFifos[0], ioFifos[1], ioFifos[2]);
    }

    if (m_state == none) {
        m_seq = 0;
        m_errorCounter = 0;
        resetSession();
        updateInspectable(false);
        m_outBuffer.clear();
        m_errBuffer.clear();
        setGdbState(Disconnected);

        // create a process to control GDB
        m_debugProcess.setWorkingDirectory(m_targetConf.workDir);

        connect(&m_debugProcess, static_cast<void (QProcess::*)(QProcess::ProcessError)>(&QProcess::errorOccurred), this, &DebugView::slotError);

        connect(&m_debugProcess, &QProcess::readyReadStandardError, this, &DebugView::slotReadDebugStdErr);

        connect(&m_debugProcess, &QProcess::readyReadStandardOutput, this, &DebugView::slotReadDebugStdOut);

        connect(&m_debugProcess, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this, &DebugView::slotDebugFinished);

        startHostProcess(m_debugProcess, fullExecutable, {QLatin1String("--interpreter=mi3")});

        enqueue(QStringLiteral("-gdb-set pagination off"));
        // for the sake of simplicity we try to restrict to synchronous mode
        enqueue(QStringLiteral("-gdb-set mi-async off"));
        enqueueProtocolHandshake();
        setState(ready, Connected);
    } else {
        enqueue(makeInitSequence());
    }
    issueNextCommandLater(std::nullopt);
}

void DebugView::issueNextCommandLater(const std::optional<State> &state)
{
    if (state) {
        setState(*state);
    }

    // On startup the gdb prompt will trigger the "nextCommands",
    // here we have to trigger it manually.
    QTimer::singleShot(0, this, &DebugView::issueNextCommand);
}

void DebugView::enqueueProtocolHandshake()
{
    m_capabilities.family = Unknown;
    m_capabilities.execRunStart.reset();
    m_capabilities.threadInfo.reset();
    m_capabilities.breakList.reset();
    m_capabilities.pendingBreakpoints.reset();
    m_capabilities.execJump.reset();
    // "version" only exists in lldb
    // data added to recognise this request from anything entered by the user
    enqueue(QStringLiteral("version"), QJsonValue(true), CaptureConsole | MuteLog);
    enqueue(QStringLiteral("-list-features"));
    enqueue(QStringLiteral("-info-gdb-mi-command thread-info"));
    enqueue(QStringLiteral("-info-gdb-mi-command break-list"));
    enqueue(QStringLiteral("-info-gdb-mi-command exec-jump"));
    enqueue(QStringLiteral("-info-gdb-mi-command data-list-changed-registers"));
    enqueue(QStringLiteral("-kate-init"), QJsonValue(1));
}

void DebugView::enqueue(const QStringList &commands, bool prepend)
{
    if (commands.isEmpty()) {
        return;
    }
    if (prepend) {
        for (int n = commands.size() - 1; n >= 0; --n) {
            m_nextCommands.prepend({commands[n], std::nullopt, Default});
        }
    } else {
        for (const auto &cmd : commands) {
            enqueue(cmd);
        }
    }
}

QStringList DebugView::makeInitSequence()
{
    m_requests.clear();
    QStringList sequence;
    // TODO adapt sequence for remote
    sequence << QStringLiteral("-file-exec-and-symbols \"%1\"").arg(m_targetConf.executable);
    if (m_capabilities.family != LLDB) {
        sequence << QStringLiteral("-exec-arguments %1 %2").arg(m_targetConf.arguments, m_ioPipeString);
    } else {
        sequence << QStringLiteral("-exec-arguments %1").arg(m_targetConf.arguments);
    }
    sequence << QStringLiteral("-inferior-tty-set /dev/null");
    for (const auto &initPart : m_targetConf.customInit) {
        sequence << initPart;
    }
    if (m_capabilities.breakList.value_or(false)) {
        sequence << QStringLiteral("-break-list");
    }
    return sequence;
}

void DebugView::enqueueThreadInfo()
{
    if (!m_inspectable) {
        return;
    }
    if (m_capabilities.threadInfo.value_or(true)) {
        enqueue(QStringLiteral("-thread-info"));
    } else {
        enqueue(QStringLiteral("-thread-list-ids"));
    }
}

QStringList DebugView::makeRunSequence(bool stop)
{
    QStringList sequence;
    if (stop) {
        if (m_capabilities.execRunStart.value_or(false)) {
            sequence << QStringLiteral("-exec-run --start");
        } else {
            sequence << QStringLiteral("-break-insert -t main");
            sequence << QStringLiteral("-exec-run");
        }
    } else {
        sequence << QStringLiteral("-exec-run");
    }
    if (m_capabilities.family == GDB) {
        sequence << QStringLiteral("-data-evaluate-expression \"setvbuf(stdout, 0, %1, 1024)\"").arg(_IOLBF);
    }
    return sequence;
}

bool DebugView::debuggerRunning() const
{
    return (m_state != none);
}

bool DebugView::debuggerBusy() const
{
    return (m_state == executingCmd) || !m_nextCommands.isEmpty();
}

int DebugView::findFirstBreakpoint(const QUrl &url, int line) const
{
    for (auto it = m_breakpointTable.constBegin(); it != m_breakpointTable.constEnd(); ++it) {
        if ((url == it.value().file) && (line == it.value().line)) {
            return it.key();
        }
    }
    return -1;
}

QStringList DebugView::findAllBreakpoints(const QUrl &url, int line) const
{
    QStringList out;
    for (auto it = m_breakpointTable.constBegin(); it != m_breakpointTable.constEnd(); ++it) {
        if ((url == it.value().file) && (line == it.value().line)) {
            out << QString::number(it.key());
        }
    }
    return out;
}

bool DebugView::hasBreakpoint(const QUrl &url, int line) const
{
    return findFirstBreakpoint(url, line) >= 0;
}

QString DebugView::makeCmdBreakInsert(const QUrl &url, int line, bool pending, bool temporal) const
{
    QString flags = temporal ? QLatin1String("-t") : QString();
    if (pending && m_capabilities.pendingBreakpoints.value_or(false)) {
        flags += QLatin1String(" -f");
    }

    return QStringLiteral("-break-insert %1 %2:%3").arg(flags).arg(url.path()).arg(line);
}

void DebugView::toggleBreakpoint(QUrl const &url, int line)
{
    if (m_state != ready) {
        return;
    }

    QString cmd;
    const auto bpNumbers = findAllBreakpoints(url, line);
    if (bpNumbers.empty()) {
        cmd = makeCmdBreakInsert(url, line, true);
    } else {
        // delete all bpoints in that line
        cmd = QStringLiteral("-break-delete %1").arg(bpNumbers.join(QLatin1Char(' ')));
    }
    issueCommand(cmd);
}

void DebugView::slotError()
{
    Q_EMIT backendError(i18n("Could not start debugger process"), KTextEditor::Message::Error);
}

void DebugView::slotReadDebugStdOut()
{
    m_outBuffer += m_debugProcess.readAllStandardOutput();

#ifdef DEBUG_GDBMI
    if (!m_outBuffer.isEmpty()) {
        Q_EMIT outputText(QStringLiteral("\n***(<gdbmi)\n%1\n***\n").arg(QString::fromLatin1(m_outBuffer)));
    }
#endif
    do {
        int end = gdbmi::GdbmiParser::splitLines(m_outBuffer, false);

        if (end < 0) {
            break;
        }
        ++end;
        const auto head = m_parser->parseResponse(m_outBuffer.mid(0, end));
        if (!head.error) {
            m_outBuffer.remove(0, head.last);
        } else {
            m_outBuffer.remove(head.last, end);
        }
#ifdef DEBUG_GDBMI
        if (!m_outBuffer.isEmpty()) {
            Q_EMIT outputText(QStringLiteral("\n***(<gdbmi)\n%1\n***\n").arg(QString::fromLatin1(m_outBuffer)));
        }
#endif
    } while (!m_outBuffer.isEmpty());
}

void DebugView::slotReadDebugStdErr()
{
    m_errBuffer += QString::fromLocal8Bit(m_debugProcess.readAllStandardError().data());
    int end = 0;
    // add whole lines at a time to the error list
    do {
        end = m_errBuffer.indexOf(QLatin1Char('\n'));
        if (end < 0) {
            break;
        }
        m_errBuffer.remove(0, end + 1);
    } while (1);

    Q_EMIT outputError(m_errBuffer + QLatin1String("\n"));
}

void DebugView::clearDebugLocation()
{
    m_debugLocationChanged = true;
    Q_EMIT debugLocationChanged(QUrl(), -1);
}

void DebugView::slotDebugFinished(int /*exitCode*/, QProcess::ExitStatus status)
{
    if (status != QProcess::NormalExit) {
        Q_EMIT outputText(i18n("*** gdb exited normally ***") + QLatin1Char('\n'));
        clearDebugLocation();
    }

    setState(none, Disconnected);

    // remove all old breakpoints
    for (auto it = m_breakpointTable.constBegin(); it != m_breakpointTable.constEnd(); ++it) {
        Q_EMIT breakPointCleared(it.value().file, it.value().line - 1);
    }
    m_breakpointTable.clear();

    Q_EMIT gdbEnded();
}

void DebugView::movePC(QUrl const &url, int line)
{
    if ((m_state == ready) && m_capabilities.execJump.value_or(false)) {
        // jump if inferior is running, or run inferrior and stop at start
        enqueue(QStringLiteral("-kate-try-run 1"), QJsonValue());
        enqueue(QStringLiteral("-exec-jump %1:%2").arg(url.path()).arg(line));
        issueCommand(makeCmdBreakInsert(url, line, false, true));
    }
}

void DebugView::runToCursor(QUrl const &url, int line)
{
    if (m_state == ready) {
        // continue if inferior running, or run inferior
        enqueue(QStringLiteral("-kate-try-run 0"), QJsonValue(QStringLiteral("-exec-continue")));
        issueCommand(makeCmdBreakInsert(url, line, true, true));
    }
}

void DebugView::slotInterrupt()
{
    if (m_state == executingCmd) {
        m_debugLocationChanged = true;
    }
    if (!m_capabilities.async.value_or(false)) {
        const auto pid = m_debugProcess.processId();
        if (pid != 0) {
            ::kill(pid, SIGINT);
        }
    } else {
        issueCommand(QStringLiteral("-exec-interrupt"));
    }
}

void DebugView::slotKill()
{
    if (inferiorRunning() && (m_state != ready)) {
        slotInterrupt();
        setState(ready);
    }
    // FIXME couldn't find a replacement for "kill", only -gdb-exit
    if (inferiorRunning()) {
        issueCommand(QStringLiteral("kill"));
    } else if (m_gdbState == Connected) {
        issueCommand(QStringLiteral("-gdb-exit"));
    }
}

void DebugView::slotReRun()
{
    resetSession();
    if (inferiorRunning()) {
        slotKill();
    }
    enqueue(makeRunSequence(false));
    issueNextCommandLater(std::nullopt);
}

void DebugView::slotStepInto()
{
    issueCommand(QStringLiteral("-kate-try-run 1"), QJsonValue(QStringLiteral("-exec-step")));
}

void DebugView::slotStepOver()
{
    issueCommand(QStringLiteral("-kate-try-run 1"), QJsonValue(QStringLiteral("-exec-next")));
}

void DebugView::slotStepOut()
{
    issueCommand(QStringLiteral("-kate-try-run 1"), QJsonValue(QStringLiteral("-exec-finish")));
}

void DebugView::slotContinue()
{
    issueCommand(QStringLiteral("-kate-try-run 0"), QJsonValue(QStringLiteral("-exec-continue")));
}

void DebugView::processMIRecord(const gdbmi::Record &record)
{
    m_errorCounter = 0;
    switch (record.category) {
    case gdbmi::Record::Status:
        break;
    case gdbmi::Record::Exec:
        processMIExec(record);
        break;
    case gdbmi::Record::Notify:
        processMINotify(record);
        break;
    case gdbmi::Record::Result:
        processMIResult(record);
        break;
    case gdbmi::Record::Prompt:
        processMIPrompt();
        break;
    }
}

void DebugView::processMIPrompt()
{
    if ((m_state != ready) && (m_state != none)) {
        return;
    }
    if (m_captureOutput != Default) {
        // the last response has completely been processed at this point
        m_captureOutput = Default;
        m_capturedOutput.clear();
    }
    // we get here after initialization
    issueNextCommandLater(ready);
}

static QString formatRecordMessage(const gdbmi::Record &record)
{
    return record.value[QLatin1String("msg")].toString() + QLatin1Char('\n');
}

static QString stoppedThreadsToString(const QJsonValue &value)
{
    if (value.isString()) {
        return value.toString();
    } else if (value.isArray()) {
        QStringList parts;
        for (const auto &item : value.toArray()) {
            parts << item.toString();
        }
        return parts.join(QLatin1String(", "));
    }
    return QString();
}

static QString getFilename(const QJsonObject &item)
{
    QString file = item[QLatin1String("fullname")].toString();

    // lldb returns "??" and "??/???" when it is not resolved
    if (file.isEmpty() || file.startsWith(QLatin1Char('?'))) {
        file = item[QLatin1String("filename")].toString();
    }

    if (file.isEmpty() || file.startsWith(QLatin1Char('?'))) {
        file = item[QLatin1String("file")].toString();
    }

    if (file.startsWith(QLatin1Char('?'))) {
        file.clear();
    }

    return file;
}

dap::StackFrame DebugView::parseFrame(const QJsonObject &object)
{
    dap::StackFrame frame;
    frame.id = object[QLatin1String("level")].toString().toInt();
    frame.instructionPointerReference = object[QLatin1String("addr")].toString();
    frame.line = object[QLatin1String("line")].toString().toInt();
    frame.column = 0;

    auto file = getFilename(object);
    if (file.isEmpty()) {
        // not really an editable file, we mark with <> in order to avoid an opening attempt
        file = QLatin1String("<%1>").arg(object[QLatin1String("from")].toString());
    } else if (!file.contains(QLatin1Char('?'))) {
        const auto resolvedFile = resolveFileName(file, true).toLocalFile();
        if (!resolvedFile.isEmpty()) {
            file = resolvedFile;
        }
    }
    frame.source = dap::Source(file);
    const auto func = object[QLatin1String("func")].toString();

    frame.name = QStringLiteral("%1 at %2:%3").arg(!func.isEmpty() ? func : frame.instructionPointerReference.value()).arg(frame.source->path).arg(frame.line);

    return frame;
}

bool DebugView::inferiorRunning() const
{
    return (m_gdbState == Running) || (m_gdbState == Stopped);
}

void DebugView::processMIExec(const gdbmi::Record &record)
{
    const auto threadId = stoppedThreadsToString(record.value.value(QLatin1String("thread-id")));
    if (record.resultClass == QLatin1String("running")) {
        updateInspectable(false);
        // running
        setGdbState(Running);
        if (threadId == QLatin1String("all")) {
            Q_EMIT outputText(printEvent(i18n("all threads running")));
        } else {
            Q_EMIT outputText(printEvent(i18n("thread(s) running: %1", threadId)));
        }
    } else if (record.resultClass == QLatin1String("stopped")) {
        const auto stoppedThreads = record.value.value(QLatin1String("stopped-threads")).toString();
        const auto reason = record.value.value(QLatin1String("reason")).toString();
        QStringList text = {i18n("stopped (%1).", reason)};
        if (!threadId.isEmpty()) {
            text << QLatin1String(" ");
            if (stoppedThreads == QLatin1String("all")) {
                text << i18n("Active thread: %1 (all threads stopped).", threadId);
            } else {
                text << i18n("Active thread: %1.", threadId);
            }
        }

        if (reason.startsWith(QLatin1String("exited"))) {
            // stopped by exit
            clearDebugLocation();
            updateInspectable(false);
            m_nextCommands.clear();
            setGdbState(Connected);
            Q_EMIT programEnded();
        } else {
            // stopped by another reason
            updateInspectable(true);
            setGdbState(Stopped);

            const auto frame = parseFrame(record.value[QLatin1String("frame")].toObject());

            if (frame.source) {
                text << QLatin1String(" ") << i18n("Current frame: %1:%2", frame.source->path, QString::number(frame.line));
            }
            m_debugLocationChanged = true;
            Q_EMIT debugLocationChanged(resolveFileName(frame.source->path), frame.line - 1);
        }

        Q_EMIT outputText(printEvent(text.join(QString())));
    }
}

void DebugView::processMINotify(const gdbmi::Record &record)
{
    if (record.resultClass == QLatin1String("breakpoint-created")) {
        responseMIBreakInsert(record);
    } else if (record.resultClass == QLatin1String("breakpoint-deleted")) {
        notifyMIBreakpointDeleted(record);
    } else if (record.resultClass == QLatin1String("breakpoint-modified")) {
        notifyMIBreakpointModified(record);
    } else {
        QString data;
        if (record.resultClass.startsWith(QLatin1String("library-"))) {
            const auto target = record.value[QLatin1String("target-name")].toString();
            const auto host = record.value[QLatin1String("host-name")].toString();

            if (target == host) {
                data = target;
            } else {
                data = i18n("Host: %1. Target: %1", host, target);
            }
        } else {
            data = QString::fromLocal8Bit(QJsonDocument(record.value).toJson(QJsonDocument::Compact));
        }

        const auto msg = QStringLiteral("(%1) %2").arg(record.resultClass).arg(data);
        Q_EMIT outputText(printEvent(msg));
    }
}

void DebugView::processMIResult(const gdbmi::Record &record)
{
    auto reqType = GdbCommand::None;
    bool isMI = true;
    QStringList args;
    std::optional<QJsonValue> commandData = std::nullopt;
    if (record.token && m_requests.contains(record.token.value())) {
        const auto command = m_requests.take(record.token.value());
        reqType = command.type;
        isMI = command.isMachineInterface();
        args = command.arguments;
        commandData = command.data;
    }
    if (isMI && (record.resultClass == QLatin1String("error")) && !(m_captureOutput & MuteLog)) {
        Q_EMIT outputError(m_lastCommand + QLatin1String("\n"));
        Q_EMIT outputError(formatRecordMessage(record));
    }

    bool isReady = true;

    switch (reqType) {
    case GdbCommand::BreakpointList:
        isReady = responseMIBreakpointList(record);
        break;
    case GdbCommand::ThreadInfo:
        isReady = responseMIThreadInfo(record);
        break;
    case GdbCommand::StackListFrames:
        isReady = responseMIStackListFrames(record);
        break;
    case GdbCommand::StackListVariables:
        isReady = responseMIStackListVariables(record);
        break;
    case GdbCommand::BreakInsert:
        isReady = responseMIBreakInsert(record);
        break;
    case GdbCommand::BreakDelete:
        isReady = responseMIBreakDelete(record, args);
        break;
    case GdbCommand::ListFeatures:
        isReady = responseMIListFeatures(record);
        break;
    case GdbCommand::DataEvaluateExpression:
        isReady = responseMIDataEvaluateExpression(record, commandData);
        break;
    case GdbCommand::Exit:
        isReady = responseMIExit(record);
        break;
    case GdbCommand::Kill:
        isReady = responseMIKill(record);
        break;
    case GdbCommand::InfoGdbMiCommand:
        isReady = responseMIInfoGdbCommand(record, args);
        break;
    case GdbCommand::LldbVersion:
        isReady = responseMILldbVersion(record);
        break;
    case GdbCommand::RegisterNames:
        isReady = responseMIRegisterNames(record);
        break;
    case GdbCommand::RegisterValues:
        isReady = responseMIRegisterValues(record);
        break;
    case GdbCommand::ChangedRegisters:
        isReady = responseMIChangedRegisters(record);
        break;
    case GdbCommand::Continue:
    case GdbCommand::Step:
    default:
        break;
    }
    if (isReady) {
        issueNextCommandLater(ready);
    } else {
        issueNextCommandLater(std::nullopt);
    }
}

void DebugView::clearFrames()
{
    // clear cached frames
    m_stackFrames.clear();
    if (m_queryLocals) {
        // request empty panel
        Q_EMIT stackFrameInfo(-1, QString());
    }

    clearVariables();
}

void DebugView::clearVariables()
{
    if (m_queryLocals) {
        Q_EMIT scopesInfo(QList<dap::Scope>{}, std::nullopt);
        Q_EMIT variableScopeOpened();
        Q_EMIT variableScopeClosed();
    }
}

bool DebugView::responseMIThreadInfo(const gdbmi::Record &record)
{
    if (record.resultClass == QLatin1String("error")) {
        if (!m_capabilities.threadInfo) {
            // now we know threadInfo is not supported
            m_capabilities.threadInfo = false;
            // try again
            enqueueThreadInfo();
        }
        return true;
    }

    // clear table
    Q_EMIT threadInfo(dap::Thread(-1), false);

    int n_threads = 0;

    bool ok = false;
    const int currentThread = record.value[QLatin1String("current-thread-id")].toString().toInt(&ok);
    if (ok) {
        // list loaded, but not selected yet
        m_currentThread = -1;
    } else {
        // unexpected, abort
        updateInspectable(false);
        return true;
    }

    QString fCollection = QLatin1String("threads");
    QString fId = QLatin1String("id");
    bool hasName = true;
    if (!record.value.contains(fCollection)) {
        fCollection = QLatin1String("thread-ids");
        fId = QLatin1String("thread-id");
        hasName = false;
    }

    for (const auto &item : record.value[fCollection].toArray()) {
        const auto thread = item.toObject();
        dap::Thread info(thread[fId].toString().toInt());
        if (hasName) {
            info.name = thread[QLatin1String("name")].toString(thread[QLatin1String("target-id")].toString());
        }

        Q_EMIT threadInfo(info, currentThread == info.id);
        ++n_threads;
    }

    if (m_queryLocals) {
        if (n_threads > 0) {
            changeThread(currentThread);
        } else {
            updateInspectable(false);
        }
    }

    return true;
}

bool DebugView::responseMIExit(const gdbmi::Record &record)
{
    if (record.resultClass != QLatin1String("exit")) {
        return true;
    }
    setState(none, Disconnected);

    // not ready
    return false;
}

bool DebugView::responseMIKill(const gdbmi::Record &record)
{
    if (record.resultClass != QLatin1String("done")) {
        return true;
    }
    clearDebugLocation();
    setState(none, Connected);
    Q_EMIT programEnded();

    return false;
}

bool DebugView::responseMIInfoGdbCommand(const gdbmi::Record &record, const QStringList &args)
{
    if (record.resultClass != QLatin1String("done")) {
        return true;
    }

    if (args.size() < 2) {
        return true;
    }

    const auto &command = args[1];
    const bool exists = record.value[QLatin1String("command")].toObject()[QLatin1String("exists")].toString() == QLatin1String("true");

    if (command == QLatin1String("thread-info")) {
        m_capabilities.threadInfo = exists;
    } else if (command == QLatin1String("break-list")) {
        m_capabilities.breakList = exists;
    } else if (command == QLatin1String("exec-jump")) {
        m_capabilities.execJump = exists;
    } else if (command == QLatin1String("data-list-changed-registers")) {
        m_capabilities.changedRegisters = exists;
    }

    return true;
}

bool DebugView::responseMILldbVersion(const gdbmi::Record &record)
{
    bool isLLDB = false;
    if (record.resultClass == QLatin1String("done")) {
        isLLDB = std::any_of(m_capturedOutput.cbegin(), m_capturedOutput.cend(), [](const QString &line) {
            return line.toLower().contains(QLatin1String("lldb"));
        });
    }
    m_capabilities.family = isLLDB ? LLDB : GDB;
    // lldb-mi inherently uses the asynchronous mode
    m_capabilities.async = m_capabilities.family == LLDB;

    return true;
}

bool DebugView::responseMIStackListFrames(const gdbmi::Record &record)
{
    if (record.resultClass == QLatin1String("error")) {
        return true;
    }

    // clear table
    clearFrames();

    for (const auto &item : record.value[QLatin1String("stack")].toArray()) {
        m_stackFrames << parseFrame(item.toObject()[QLatin1String("frame")].toObject());
    }

    m_currentFrame = -1;
    m_currentScope.reset();
    informStackFrame();

    if (!m_stackFrames.isEmpty()) {
        changeStackFrame(0);
    }

    return true;
}

bool DebugView::responseMIRegisterNames(const gdbmi::Record &record)
{
    if (record.resultClass != QLatin1String("done")) {
        return true;
    }

    const auto names = record.value[QLatin1String("register-names")].toArray();
    m_registerNames.clear();
    m_registerNames.reserve(names.size());
    for (const auto &name : names) {
        m_registerNames << name.toString().trimmed();
    }

    return true;
}

bool DebugView::responseMIRegisterValues(const gdbmi::Record &record)
{
    if (record.resultClass != QLatin1String("done")) {
        return true;
    }

    Q_EMIT variableScopeOpened();
    for (const auto &item : record.value[QLatin1String("register-values")].toArray()) {
        const auto var = item.toObject();
        bool ok = false;
        const int regIndex = var[QLatin1String("number")].toString().toInt(&ok);
        if (!ok || (regIndex < 0) || (regIndex >= m_registerNames.size())) {
            continue;
        }
        const auto &name = m_registerNames[regIndex];
        if (name.isEmpty()) {
            continue;
        }
        m_variableParser.insertVariable(m_registerNames[regIndex], var[QLatin1String("value")].toString(), QString(), m_changedRegisters.contains(regIndex));
    }
    Q_EMIT variableScopeClosed();
    return true;
}

bool DebugView::responseMIChangedRegisters(const gdbmi::Record &record)
{
    if (record.resultClass != QLatin1String("done")) {
        return true;
    }
    for (const auto &item : record.value[QLatin1String("changed-registers")].toArray()) {
        bool ok = false;
        const int regIndex = item.toString().toInt(&ok);
        if (!ok) {
            continue;
        }
        m_changedRegisters.insert(regIndex);
    }
    return true;
}

bool DebugView::responseMIStackListVariables(const gdbmi::Record &record)
{
    if (record.resultClass == QLatin1String("error")) {
        return true;
    }

    Q_EMIT variableScopeOpened();
    for (const auto &item : record.value[QLatin1String("variables")].toArray()) {
        const auto var = item.toObject();
        m_variableParser.insertVariable(var[QLatin1String("name")].toString(), var[QLatin1String("value")].toString(), var[QLatin1String("type")].toString());
    }
    Q_EMIT variableScopeClosed();

    return true;
}

bool DebugView::responseMIListFeatures(const gdbmi::Record &record)
{
    if (record.resultClass != QLatin1String("done")) {
        return true;
    }

    for (const auto &item : record.value[QLatin1String("features")].toArray()) {
        const auto capability = item.toString();
        if (capability == QLatin1String("exec-run-start-option")) {
            // exec-run --start is not reliable in lldb
            m_capabilities.execRunStart = m_capabilities.family != LLDB;
        } else if (capability == QLatin1String("thread-info")) {
            m_capabilities.threadInfo = true;
        } else if (capability == QLatin1String("pending-breakpoints")) {
            m_capabilities.pendingBreakpoints = true;
        }
    }

    return true;
}

BreakPoint BreakPoint::parse(const QJsonObject &item)
{
    const QString f_line = QLatin1String("line");

    BreakPoint breakPoint;
    breakPoint.number = item[QLatin1String("number")].toString(QLatin1String("1")).toInt();
    breakPoint.line = item[f_line].toString(QLatin1String("-1")).toInt();

    // file
    auto file = getFilename(item);

    if ((breakPoint.line < 0) || file.isEmpty()) {
        // try original-location
        QString pending = item[QLatin1String("original-location")].toString();

        // try pending
        const auto f_pending = QLatin1String("pending");
        if (pending.isEmpty()) {
            const auto &v_pending = item[f_pending];
            if (v_pending.isArray()) {
                const auto values = v_pending.toArray();
                if (!values.isEmpty()) {
                    pending = values.first().toString();
                }
            } else {
                pending = v_pending.toString();
            }
        }
        int sep = pending.lastIndexOf(QLatin1Char(':'));
        if (sep > 0) {
            if (breakPoint.line < 0) {
                breakPoint.line = pending.mid(sep + 1).toInt();
            }
            if (file.isEmpty()) {
                file = pending.left(sep);
                if (file.startsWith(QLatin1Char('?'))) {
                    file.clear();
                }
            }
        }
    }

    if ((breakPoint.line < 0) || file.isEmpty()) {
        // try locations
        const auto f_locations = QLatin1String("locations");
        if (item.contains(f_locations)) {
            for (const auto item_loc : item[f_locations].toArray()) {
                const auto loc = item_loc.toObject();
                if (breakPoint.line < 0) {
                    breakPoint.line = loc[f_line].toString(QLatin1String("-1")).toInt();
                }
                if (file.isEmpty()) {
                    file = getFilename(loc);
                    if (file.startsWith(QLatin1Char('?'))) {
                        file.clear();
                    }
                }
                if ((breakPoint.line > -1) && !file.isEmpty()) {
                    break;
                }
            }
        }
    }

    if (!file.isEmpty()) {
        breakPoint.file = QUrl::fromLocalFile(file);
    }

    return breakPoint;
}

BreakPoint DebugView::parseBreakpoint(const QJsonObject &item)
{
    // XXX in a breakpoint with multiple locations, only the first one is considered
    BreakPoint breakPoint = BreakPoint::parse(item);
    breakPoint.file = resolveFileName(breakPoint.file.toLocalFile());

    return breakPoint;
}

void DebugView::insertBreakpoint(const QJsonObject &item)
{
    const BreakPoint breakPoint = parseBreakpoint(item);
    Q_EMIT breakPointSet(breakPoint.file, breakPoint.line - 1);
    m_breakpointTable[breakPoint.number] = std::move(breakPoint);
}

bool DebugView::responseMIBreakpointList(const gdbmi::Record &record)
{
    if (record.resultClass != QLatin1String("done")) {
        return true;
    }

    Q_EMIT clearBreakpointMarks();
    m_breakpointTable.clear();

    for (const auto item : record.value[QLatin1String("body")].toArray()) {
        insertBreakpoint(item.toObject());
    }

    return true;
}

bool DebugView::responseMIBreakInsert(const gdbmi::Record &record)
{
    if (record.resultClass == QLatin1String("error")) {
        // cancel pending commands
        m_nextCommands.clear();
        return true;
    }

    const auto bkpt = record.value[QLatin1String("bkpt")].toObject();
    if (bkpt.isEmpty()) {
        return true;
    }

    insertBreakpoint(bkpt);

    return true;
}

void DebugView::notifyMIBreakpointModified(const gdbmi::Record &record)
{
    const auto bkpt = record.value[QLatin1String("bkpt")].toObject();
    if (bkpt.isEmpty()) {
        return;
    }

    const BreakPoint newBp = parseBreakpoint(bkpt);
    if (!m_breakpointTable.contains(newBp.number)) {
        // may be a pending breakpoint
        responseMIBreakInsert(record);
        return;
    }

    const auto &oldBp = m_breakpointTable[newBp.number];

    if ((oldBp.line != newBp.line) || (oldBp.file != newBp.file)) {
        const QUrl oldFile = oldBp.file;
        const int oldLine = oldBp.line;
        m_breakpointTable[newBp.number] = newBp;
        if (findFirstBreakpoint(oldFile, oldLine) < 0) {
            // this is the last bpoint in this line
            Q_EMIT breakPointCleared(oldFile, oldLine - 1);
        }
        Q_EMIT breakPointSet(newBp.file, newBp.line - 1);
    }
}

void DebugView::deleteBreakpoint(const int bpNumber)
{
    if (!m_breakpointTable.contains(bpNumber)) {
        return;
    }
    const auto bp = m_breakpointTable.take(bpNumber);
    if (findFirstBreakpoint(bp.file, bp.line) < 0) {
        // this is the last bpoint in this line
        Q_EMIT breakPointCleared(bp.file, bp.line - 1);
    }
}

void DebugView::notifyMIBreakpointDeleted(const gdbmi::Record &record)
{
    bool ok = false;
    const int bpNumber = record.value[QLatin1String("id")].toString().toInt(&ok);
    if (ok) {
        deleteBreakpoint(bpNumber);
    }
}

bool DebugView::responseMIBreakDelete(const gdbmi::Record &record, const QStringList &args)
{
    if (record.resultClass != QLatin1String("done")) {
        return true;
    }

    // get breakpoints ids from arguments
    if (args.size() < 2) {
        return true;
    }

    for (int idx = 1; idx < args.size(); ++idx) {
        bool ok = false;
        const int bpNumber = args[idx].toInt(&ok);
        if (!ok) {
            continue;
        }
        deleteBreakpoint(bpNumber);
    }

    return true;
}

QString DebugView::makeFrameFlags() const
{
    if (!m_currentThread || !m_currentFrame) {
        return QString();
    }

    if ((*m_currentFrame >= m_stackFrames.size()) || (*m_currentFrame < 0)) {
        return QString();
    }

    const int frameId = m_stackFrames[*m_currentFrame].id;

    return QLatin1String("--thread %1 --frame %2").arg(QString::number(*m_currentThread)).arg(frameId);
}

void DebugView::enqueueScopes()
{
    if (!m_currentFrame || !m_currentThread) {
        return;
    }
    enqueue(QLatin1String("-data-evaluate-expression %1 \"this\"").arg(makeFrameFlags()), QJsonValue(DATA_EVAL_THIS_CHECK), MuteLog);
}

void DebugView::enqueueScopeVariables()
{
    if (!m_currentFrame || !m_currentThread) {
        return;
    }
    if (m_pointerThis && (m_currentScope == ThisScope.variablesReference)) {
        // request *this
        enqueue(QLatin1String("-data-evaluate-expression %1 \"*this\"").arg(makeFrameFlags()), QJsonValue(DATA_EVAL_THIS_DEREF));
    } else if (m_currentScope == RegistersScope.variablesReference) {
        if (m_registerNames.isEmpty()) {
            enqueue(QLatin1String("-data-list-register-names"));
        }
        if (m_capabilities.changedRegisters.value_or(false)) {
            m_changedRegisters.clear();
            enqueue(QLatin1String("-data-list-changed-registers"));
        }
        enqueue(QLatin1String("-data-list-register-values --skip-unavailable r"));
    } else {
        // request locals
        enqueue(QLatin1String("-stack-list-variables %1 --all-values").arg(makeFrameFlags()));
    }
}

void DebugView::responseMIScopes(const gdbmi::Record &record)
{
    m_pointerThis = record.resultClass != QLatin1String("error");
    if (!m_inspectable || !m_queryLocals) {
        return;
    }

    QList<dap::Scope> scopes = {LocalScope};
    if (m_pointerThis) {
        scopes << ThisScope;
    }
    scopes << RegistersScope;

    const auto activeScope = std::find_if(scopes.cbegin(), scopes.cend(), [this](const auto &scope) {
        return !m_watchedScope || (*m_watchedScope == scope.variablesReference);
    });
    if (activeScope != scopes.cend()) {
        m_watchedScope = activeScope->variablesReference;
    } else {
        m_watchedScope = scopes[0].variablesReference;
    }

    m_currentScope.reset();

    if (m_queryLocals) {
        // preload variables
        Q_EMIT scopesInfo(scopes, m_watchedScope);
        slotQueryLocals(true);
    }
}

void DebugView::responseMIThisScope(const gdbmi::Record &record)
{
    if (record.resultClass == QLatin1String("error")) {
        return;
    }

    const auto value = record.value[QStringLiteral("value")].toString();
    const auto parent = dap::Variable(QString(), value, 0);
    Q_EMIT variableScopeOpened();
    m_variableParser.insertVariable(QStringLiteral("*this"), value, QString());
    Q_EMIT variableScopeClosed();
}

bool DebugView::responseMIDataEvaluateExpression(const gdbmi::Record &record, const std::optional<QJsonValue> &data)
{
    if (data) {
        const int mode = data->toInt(-1);
        switch (mode) {
        case DATA_EVAL_THIS_CHECK:
            responseMIScopes(record);
            return true;
            break;
        case DATA_EVAL_THIS_DEREF:
            responseMIThisScope(record);
            return true;
            break;
        }
    }

    if (record.resultClass != QLatin1String("done")) {
        return true;
    }

    QString key;
    if (data) {
        key = data->toString(QLatin1String("$1"));
    } else {
        key = QLatin1String("$1");
    }
    Q_EMIT outputText(QStringLiteral("%1 = %2\n").arg(key).arg(record.value[QLatin1String("value")].toString()));

    return true;
}

void DebugView::onMIParserError(const QString &errorMessage)
{
    QString message;
    ++m_errorCounter;
    const bool overflow = m_errorCounter > MAX_RESPONSE_ERRORS;
    if (overflow) {
        message = i18n("gdb-mi: Could not parse last response: %1. Too many consecutive errors. Shutting down.", errorMessage);
    } else {
        message = i18n("gdb-mi: Could not parse last response: %1", errorMessage);
    }
    m_nextCommands.clear();
    Q_EMIT backendError(message, KTextEditor::Message::Error);

    if (overflow) {
        m_debugProcess.kill();
    }
}

QString DebugView::slotPrintVariable(const QString &variable)
{
    const QString cmd = QStringLiteral("-data-evaluate-expression \"%1\"").arg(gdbmi::quotedString(variable));
    issueCommand(cmd, QJsonValue(variable));
    return cmd;
}

void DebugView::issueCommand(QString const &cmd)
{
    issueCommand(cmd, std::nullopt);
}

void DebugView::cmdKateInit()
{
    // enqueue full init sequence
    updateInputReady(!debuggerBusy() && canMove(), true);
    enqueue(makeInitSequence(), true);
    issueNextCommandLater(std::nullopt);
}

void DebugView::cmdKateTryRun(const GdbCommand &command, const QJsonValue &data)
{
    // enqueue command if running, or run inferior
    // 0 - run & continue
    // 1 - run & stop
    // command - data[str]
    if (!inferiorRunning()) {
        bool stop = false;
        if (command.arguments.size() > 1) {
            bool ok = false;
            const int val = command.arguments[1].toInt(&ok);
            if (ok) {
                stop = val > 0;
            }
        }
        enqueue(makeRunSequence(stop), true);
    } else {
        const auto altCmd = data.toString();
        if (!altCmd.isEmpty()) {
            prepend(data.toString());
        }
    }
    issueNextCommandLater(std::nullopt);
}

void DebugView::issueCommand(const QString &cmd, const std::optional<QJsonValue> &data, uint8_t captureMode)
{
    auto command = GdbCommand::parse(cmd);
    // macro command
    if (data) {
        if (command.check(QLatin1String("-kate-init"))) {
            cmdKateInit();
            return;
        } else if (command.check(QLatin1String("-kate-try-run"))) {
            cmdKateTryRun(command, *data);
            return;
        }
    }
    // real command
    if (m_state == ready) {
        setState(executingCmd);

        if (data) {
            command.data = data;
        }
        m_captureOutput = captureMode;
        QString newCmd;

        const bool isMI = command.isMachineInterface();

        if (isMI) {
            newCmd = cmd;
        } else {
            newCmd = QLatin1String("-interpreter-exec console \"%1\"").arg(gdbmi::quotedString(cmd));
        }

        if (command.check(QLatin1String("-break-list"))) {
            command.type = GdbCommand::BreakpointList;
        } else if (command.check(QLatin1String("-exec-continue")) || command.check(QLatin1String("continue")) || command.check(QLatin1String("c"))
                   || command.check(QLatin1String("fg"))) {
            // continue family
            command.type = GdbCommand::Continue;
        } else if (command.check(QLatin1String("-exec-step")) || command.check(QLatin1String("step")) || command.check(QLatin1String("s"))
                   || command.check(QLatin1String("-exec-finish")) || command.check(QLatin1String("finish")) || command.check(QLatin1String("fin"))
                   || command.check(QLatin1String("-exec-next")) || command.check(QLatin1String("next")) || command.check(QLatin1String("n"))) {
            command.type = GdbCommand::Step;
        } else if (command.check(QLatin1String("-thread-info")) || command.check(QLatin1String("-thread-list-ids"))) {
            command.type = GdbCommand::ThreadInfo;
        } else if (command.check(QLatin1String("-stack-list-frames"))) {
            command.type = GdbCommand::StackListFrames;
        } else if (command.check(QLatin1String("-stack-list-variables"))) {
            command.type = GdbCommand::StackListVariables;
        } else if (command.check(QLatin1String("-break-insert"))) {
            command.type = GdbCommand::BreakInsert;
        } else if (command.check(QLatin1String("-break-delete"))) {
            command.type = GdbCommand::BreakDelete;
        } else if (command.check(QLatin1String("-list-features"))) {
            command.type = GdbCommand::ListFeatures;
        } else if (command.check(QLatin1String("-data-evaluate-expression"))) {
            command.type = GdbCommand::DataEvaluateExpression;
        } else if (command.check(QLatin1String("-data-list-register-names"))) {
            command.type = GdbCommand::RegisterNames;
        } else if (command.check(QLatin1String("-data-list-register-values"))) {
            command.type = GdbCommand::RegisterValues;
        } else if (command.check(QLatin1String("-data-list-changed-registers"))) {
            command.type = GdbCommand::ChangedRegisters;
        } else if (command.check(QLatin1String("-gdb-exit"))) {
            command.type = GdbCommand::Exit;
        } else if (command.check(QLatin1String("-info-gdb-mi-command"))) {
            command.type = GdbCommand::InfoGdbMiCommand;
        } else if (command.check(QLatin1String("kill"))) {
            command.type = GdbCommand::Kill;
        } else if (command.check(QLatin1String("version")) && data) {
            command.type = GdbCommand::LldbVersion;
        }

        // register the response parsing type
        newCmd = QStringLiteral("%1%2").arg(m_seq).arg(newCmd);
        m_requests[m_seq++] = command;

        m_lastCommand = cmd;

        if (!isMI && !(m_captureOutput & MuteLog)) {
            Q_EMIT outputText(QStringLiteral("(gdb) %1\n").arg(cmd));
        }
#ifdef DEBUG_GDBMI
        Q_EMIT outputText(QStringLiteral("\n***(gdbmi>)\n%1\n***\n").arg(newCmd));
#endif
        m_debugProcess.write(qPrintable(newCmd));
        m_debugProcess.write("\n");
    }
}

void DebugView::updateInputReady(bool newState, bool force)
{
    // refresh only when the state changed
    if (force || (m_lastInputReady != newState)) {
        m_lastInputReady = newState;
        Q_EMIT readyForInput(newState);
    }
}

void DebugView::setState(State newState, std::optional<GdbState> newGdbState)
{
    m_state = newState;
    if (newGdbState) {
        m_gdbState = *newGdbState;
    }

    updateInputReady(!debuggerBusy() && canMove(), true);
}

void DebugView::setGdbState(GdbState newState)
{
    m_gdbState = newState;

    updateInputReady(!debuggerBusy() && canMove(), true);
}

void DebugView::issueNextCommand()
{
    if (m_state == ready) {
        if (!m_nextCommands.empty()) {
            const auto cmd = m_nextCommands.takeFirst();
            issueCommand(cmd.command, cmd.data, cmd.captureMode);
        } else {
            if (m_debugLocationChanged) {
                m_debugLocationChanged = false;
                if (m_queryLocals) {
                    slotQueryLocals(true);
                    issueNextCommand();
                    return;
                }
            }
            updateInputReady(!debuggerBusy() && canMove());
        }
    }
}

QUrl DebugView::resolveFileName(const QString &fileName, bool silent)
{
    QFileInfo fInfo = QFileInfo(fileName);
    // did we end up with an absolute path or a relative one?
    if (fInfo.exists()) {
        return QUrl::fromUserInput(fInfo.absoluteFilePath());
    }

    if (fInfo.isAbsolute()) {
        // we can not do anything just return the fileName
        return QUrl::fromUserInput(fileName);
    }

    // Now try to add the working path
    fInfo = QFileInfo(m_targetConf.workDir + fileName);
    if (fInfo.exists()) {
        return QUrl::fromUserInput(fInfo.absoluteFilePath());
    }

    // now try the executable path
    fInfo = QFileInfo(QFileInfo(m_targetConf.executable).absolutePath() + fileName);
    if (fInfo.exists()) {
        return QUrl::fromUserInput(fInfo.absoluteFilePath());
    }

    for (const QString &srcPath : qAsConst(m_targetConf.srcPaths)) {
        fInfo = QFileInfo(srcPath + QDir::separator() + fileName);
        if (fInfo.exists()) {
            return QUrl::fromUserInput(fInfo.absoluteFilePath());
        }
    }

    // we can not do anything just return the fileName
    if (!silent) {
        Q_EMIT sourceFileNotFound(fileName);
    }
    return QUrl::fromUserInput(fileName);
}

void DebugView::processMIStreamOutput(const gdbmi::StreamOutput &output)
{
    switch (output.channel) {
    case gdbmi::StreamOutput::Console:
        if (m_captureOutput & CaptureConsole) {
            m_capturedOutput << output.message;
        }
        Q_EMIT outputText(output.message);
        break;
    case gdbmi::StreamOutput::Log:
        if (!(m_captureOutput & ~MuteLog)) {
            Q_EMIT outputError(output.message);
        }
        break;
    case gdbmi::StreamOutput::Output:
        Q_EMIT debuggeeOutput(dap::Output(output.message, dap::Output::Category::Stdout));
        break;
    }
}

void DebugView::informStackFrame()
{
    if (!m_queryLocals) {
        return;
    }

    int level = 0;

    for (const auto &frame : m_stackFrames) {
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

void DebugView::slotQueryLocals(bool query)
{
    if (!debuggerRunning()) {
        return;
    }
    m_queryLocals = query;

#ifdef DEBUG_GDBMI
    Q_EMIT outputText(QStringLiteral("\n***(gdbmi^)\nSLOT QUERY LOCALS INIT %1\n***\n").arg(query));
#endif

    if (!query) {
        return;
    }

    // !current thread -> no threads; current thread -> threads
    if (!m_currentThread) {
// get threads
#ifdef DEBUG_GDBMI
        Q_EMIT outputText(QStringLiteral("\n***(gdbmi^)\nSLOT QUERY LOCALS THREAD INFO %1\n***\n").arg(query));
#endif
        enqueueThreadInfo();
        issueNextCommandLater(std::nullopt);
    } else if (!m_currentFrame) {
// get frames
#ifdef DEBUG_GDBMI
        Q_EMIT outputText(QStringLiteral("\n***(gdbmi^)\nSLOT QUERY LOCALS CHANGE THREAD %1\n***\n").arg(query));
#endif
        changeThread(*m_currentThread);
    } else if (!m_watchedScope) {
// get scopes
#ifdef DEBUG_GDBMI
        Q_EMIT outputText(QStringLiteral("\n***(gdbmi^)\nSLOT QUERY LOCALS CHANGE STACK FRAME %1\n***\n").arg(query));
#endif
        changeStackFrame(*m_currentFrame);
    } else {
// get variables
#ifdef DEBUG_GDBMI
        Q_EMIT outputText(QStringLiteral("\n***(gdbmi^)\nSLOT QUERY LOCALS CHANGE SCOPE %1\n***\n").arg(query));
#endif
        changeScope(*m_watchedScope);
    }
}

QString DebugView::targetName() const
{
    return m_targetConf.targetName;
}

void DebugView::setFileSearchPaths(const QStringList &paths)
{
    m_targetConf.srcPaths = paths;
}

void DebugView::updateInspectable(bool inspectable)
{
    m_inspectable = inspectable;
    m_currentThread.reset();
    m_currentFrame.reset();
    m_currentScope.reset();
    clearFrames();
    Q_EMIT scopesInfo(QList<dap::Scope>{}, std::nullopt);
}

void DebugView::changeStackFrame(int index)
{
#ifdef DEBUG_GDBMI
    Q_EMIT outputText(QStringLiteral("\n***(gdbmi^)\nCHANGE FRAME %1\n***\n").arg(index));
#endif
    if (!debuggerRunning() || !m_inspectable) {
        return;
    }
    if (!m_currentThread) {
        // unexpected state
        updateInspectable(false);
        return;
    }
    if ((m_stackFrames.size() < index) || (index < 0)) {
        // frame not found in stack
        return;
    }

    if (m_currentFrame && (*m_currentFrame == index)) {
        // already loaded
        return;
    }

    m_currentFrame = index;

    const auto &frame = m_stackFrames[index];
    if (frame.source) {
        Q_EMIT debugLocationChanged(resolveFileName(frame.source->path), frame.line - 1);
    }

    Q_EMIT stackFrameChanged(index);

    m_currentScope.reset();
    enqueueScopes();
    issueNextCommandLater(std::nullopt);
}

void DebugView::changeThread(int index)
{
#ifdef DEBUG_GDBMI
    Q_EMIT outputText(QStringLiteral("\n***(gdbmi^)\nCHANGE THREAD %1\n***\n").arg(index));
#endif
    if (!debuggerRunning() || !m_inspectable) {
        return;
    }
    if (!m_queryLocals) {
        return;
    }
    if (*m_currentThread && (*m_currentThread == index)) {
        // already loaded
        return;
    }
    m_currentThread = index;

    enqueue(QStringLiteral("-stack-list-frames --thread %1").arg(index));
    issueNextCommandLater(std::nullopt);
}

void DebugView::changeScope(int scopeId)
{
#ifdef DEBUG_GDBMI
    Q_EMIT outputText(QStringLiteral("\n***(gdbmi^)\nCHANGE SCOPE %1\n***\n").arg(scopeId));
#endif
    if (!debuggerRunning() || !m_inspectable) {
        return;
    }

    m_watchedScope = scopeId;

    if (m_currentScope && (*m_currentScope == scopeId)) {
        // already loaded
        return;
    }

    m_currentScope = m_watchedScope;

    if (m_queryLocals) {
        enqueueScopeVariables();
        issueNextCommandLater(std::nullopt);
    }
}

#include "moc_debugview.cpp"
