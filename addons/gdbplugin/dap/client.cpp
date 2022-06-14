/*
    SPDX-FileCopyrightText: 2022 Héctor Mesa Jiménez <wmj.py@gmx.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "client.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <limits>

#include "dap/bus_selector.h"
#include "dap/settings.h"
#include "dapclient_debug.h"

#include "messages.h"

namespace dap
{
constexpr int MAX_HEADER_SIZE = 1 << 16;

template<typename T>
inline Client::ResponseHandler make_response_handler(void (T::*member)(const Response &response, const QJsonValue &request), T *object)
{
    return [object, member](const Response &response, const QJsonValue &request) {
        return (object->*member)(response, request);
    };
}

Client::Client(const settings::ProtocolSettings &protocolSettings, Bus *bus, QObject *parent)
    : QObject(parent)
    , m_bus(bus)
    , m_managedBus(false)
    , m_protocol(protocolSettings)
    , m_launchCommand(extractCommand(protocolSettings.launchRequest))
{
    bind();
}

Client::Client(const settings::ClientSettings &clientSettings, QObject *parent)
    : QObject(parent)
    , m_managedBus(true)
    , m_protocol(clientSettings.protocolSettings)
    , m_launchCommand(extractCommand(clientSettings.protocolSettings.launchRequest))
{
    m_bus = createBus(clientSettings.busSettings);
    m_bus->setParent(this);
    bind();
}

Client::~Client()
{
    detach();
}

void Client::bind()
{
    connect(m_bus, &Bus::readyRead, this, &Client::read);
    connect(m_bus, &Bus::running, this, &Client::start);
    connect(m_bus, &Bus::closed, this, &Client::finished);
    if (m_protocol.redirectStderr)
        connect(m_bus, &Bus::serverOutput, this, &Client::onServerOutput);
    if (m_protocol.redirectStdout)
        connect(m_bus, &Bus::processOutput, this, &Client::onProcessOutput);
}

void Client::detach()
{
    if (!m_bus)
        return;

    disconnect(m_bus);
    if (m_managedBus) {
        m_bus->close();
        m_bus->deleteLater();
        m_bus = nullptr;
    }
}

bool Client::isServerConnected() const
{
    return (m_state != State::None) && (m_state != State::Failed) && (m_bus->state() == Bus::State::Running);
}

bool Client::supportsTerminate() const
{
    return m_adapterCapabilities.supportsTerminateRequest && (m_protocol.launchRequest[DAP_COMMAND].toString() == DAP_LAUNCH);
}

/*
 * fields:
 *  seq: number;
 *  type: 'request' | 'response' | 'event' | string;
 *
 * extensions:
 *  Request
 *  Response
 *  Event
 */
void Client::processProtocolMessage(const QJsonObject &msg)
{
    const auto type = msg[DAP_TYPE].toString();

    if (DAP_RESPONSE == type) {
        processResponse(msg);
    } else if (DAP_EVENT == type) {
        processEvent(msg);
    } else {
        qCWarning(DAPCLIENT) << "unknown, empty or unexpected ProtocolMessage::" << DAP_TYPE << " (" << type << ")";
    }
}

/*
 * extends ProtocolMessage
 *
 * fields:
 *  request_seq: number;
 *  success: boolean;
 *  command: string
 *  message?: 'cancelled' | string;
 *  body?: any;
 *
 * extensions:
 *  ErrorResponse
 */
void Client::processResponse(const QJsonObject &msg)
{
    const Response response(msg);
    // check sequence
    if ((response.request_seq < 0) || !m_requests.contains(response.request_seq)) {
        qCWarning(DAPCLIENT) << "unexpected requested seq in response";
        return;
    }

    const auto request = m_requests.take(response.request_seq);

    // check response
    if (response.command != std::get<0>(request)) {
        qCWarning(DAPCLIENT) << "unexpected command in response: " << response.command << " (expected: " << std::get<0>(request) << ")";
    }
    if (response.isCancelled()) {
        qCWarning(DAPCLIENT) << "request cancelled: " << response.command;
    }

    if (!response.success) {
        Q_EMIT errorResponse(response.message, response.errorBody);
    }
    std::get<2>(request)(response, std::get<1>(request));
}

void Client::processResponseInitialize(const Response &response, const QJsonValue &)
{
    if (m_state != State::Initializing) {
        qCWarning(DAPCLIENT) << "unexpected initialize response";
        setState(State::None);
        return;
    }

    if (!response.success && response.isCancelled()) {
        qCWarning(DAPCLIENT) << "InitializeResponse error: " << response.message;
        if (response.errorBody) {
            qCWarning(DAPCLIENT) << "error" << response.errorBody->id << response.errorBody->format;
        }
        setState(State::None);
        return;
    }

    // get server capabilities
    m_adapterCapabilities = Capabilities(response.body.toObject());
    Q_EMIT capabilitiesReceived(m_adapterCapabilities);

    requestLaunchCommand();
}

void Client::processEvent(const QJsonObject &msg)
{
    const QString event = msg[DAP_EVENT].toString();
    const auto body = msg[DAP_BODY].toObject();

    if (QStringLiteral("initialized") == event) {
        processEventInitialized();
    } else if (QStringLiteral("terminated") == event) {
        processEventTerminated();
    } else if (QStringLiteral("exited") == event) {
        processEventExited(body);
    } else if (DAP_OUTPUT == event) {
        processEventOutput(body);
    } else if (QStringLiteral("process") == event) {
        processEventProcess(body);
    } else if (QStringLiteral("thread") == event) {
        processEventThread(body);
    } else if (QStringLiteral("stopped") == event) {
        processEventStopped(body);
    } else if (QStringLiteral("module") == event) {
        processEventModule(body);
    } else if (QStringLiteral("continued") == event) {
        processEventContinued(body);
    } else if (DAP_BREAKPOINT == event) {
        processEventBreakpoint(body);
    } else {
        qCWarning(DAPCLIENT) << "unsupported event: " << event;
    }
}

void Client::processEventInitialized()
{
    if ((m_state != State::Initializing)) {
        qCWarning(DAPCLIENT) << "unexpected initialized event";
        return;
    }
    setState(State::Initialized);
}

void Client::processEventTerminated()
{
    setState(State::Terminated);
}

void Client::processEventExited(const QJsonObject &body)
{
    const int exitCode = body[QStringLiteral("exitCode")].toInt(-1);
    Q_EMIT debuggeeExited(exitCode);
}

void Client::processEventOutput(const QJsonObject &body)
{
    Q_EMIT outputProduced(Output(body));
}

void Client::processEventProcess(const QJsonObject &body)
{
    Q_EMIT debuggingProcess(ProcessInfo(body));
}

void Client::processEventThread(const QJsonObject &body)
{
    Q_EMIT threadChanged(ThreadEvent(body));
}

void Client::processEventStopped(const QJsonObject &body)
{
    Q_EMIT debuggeeStopped(StoppedEvent(body));
}

void Client::processEventModule(const QJsonObject &body)
{
    Q_EMIT moduleChanged(ModuleEvent(body));
}

void Client::processEventContinued(const QJsonObject &body)
{
    Q_EMIT debuggeeContinued(ContinuedEvent(body));
}

void Client::processEventBreakpoint(const QJsonObject &body)
{
    Q_EMIT breakpointChanged(BreakpointEvent(body));
}

void Client::processResponseConfigurationDone(const Response &response, const QJsonValue &)
{
    if (response.success) {
        m_configured = true;
        Q_EMIT configured();
        checkRunning();
    }
}

void Client::processResponseLaunch(const Response &response, const QJsonValue &)
{
    if (response.success) {
        m_launched = true;
        Q_EMIT launched();
        checkRunning();
    } else {
        setState(State::Failed);
    }
}

void Client::processResponseThreads(const Response &response, const QJsonValue &)
{
    if (response.success) {
        Q_EMIT threads(Thread::parseList(response.body.toObject()[DAP_THREADS].toArray()));
    } else {
        Q_EMIT threads(QList<Thread>());
    }
}

void Client::processResponseStackTrace(const Response &response, const QJsonValue &request)
{
    const int threadId = request.toObject()[DAP_THREAD_ID].toInt();
    if (response.success) {
        Q_EMIT stackTrace(threadId, StackTraceInfo(response.body.toObject()));
    } else {
        Q_EMIT stackTrace(threadId, StackTraceInfo());
    }
}

void Client::processResponseScopes(const Response &response, const QJsonValue &request)
{
    const int frameId = request.toObject()[DAP_FRAME_ID].toInt();
    if (response.success) {
        Q_EMIT scopes(frameId, Scope::parseList(response.body.toObject()[DAP_SCOPES].toArray()));
    } else {
        Q_EMIT scopes(frameId, QList<Scope>());
    }
}

void Client::processResponseVariables(const Response &response, const QJsonValue &request)
{
    const int variablesReference = request.toObject()[DAP_VARIABLES_REFERENCE].toInt();
    if (response.success) {
        Q_EMIT variables(variablesReference, Variable::parseList(response.body.toObject()[DAP_VARIABLES].toArray()));
    } else {
        Q_EMIT variables(variablesReference, QList<Variable>());
    }
}

void Client::processResponseModules(const Response &response, const QJsonValue &)
{
    if (response.success) {
        Q_EMIT modules(ModulesInfo(response.body.toObject()));
    } else {
        Q_EMIT modules(ModulesInfo());
    }
}

void Client::processResponseNext(const Response &response, const QJsonValue &request)
{
    if (response.success) {
        Q_EMIT debuggeeContinued(ContinuedEvent(request.toObject()[DAP_THREAD_ID].toInt(), !response.body.toObject()[DAP_SINGLE_THREAD].toBool(false)));
    }
}

void Client::processResponseContinue(const Response &response, const QJsonValue &request)
{
    if (response.success) {
        Q_EMIT debuggeeContinued(ContinuedEvent(request.toObject()[DAP_THREAD_ID].toInt(), response.body.toObject()[DAP_ALL_THREADS_CONTINUED].toBool(true)));
    }
}

void Client::processResponsePause(const Response &, const QJsonValue &)
{
}

void Client::processResponseTerminate(const Response &, const QJsonValue &)
{
}

void Client::processResponseDisconnect(const Response &response, const QJsonValue &)
{
    if (response.success) {
        Q_EMIT serverDisconnected();
    }
}

void Client::processResponseSource(const Response &response, const QJsonValue &request)
{
    const auto req = request.toObject();
    const auto path = req[DAP_SOURCE].toObject()[DAP_PATH].toString();
    const auto reference = req[DAP_SOURCE_REFERENCE].toInt(0);
    if (response.success) {
        Q_EMIT sourceContent(path, reference, SourceContent(response.body.toObject()));
    } else {
        // empty
        Q_EMIT sourceContent(path, reference, SourceContent());
    }
}

void Client::processResponseSetBreakpoints(const Response &response, const QJsonValue &request)
{
    const auto source = Source(request.toObject()[DAP_SOURCE].toObject());
    if (response.success) {
        const auto resp = response.body.toObject();
        if (resp.contains(DAP_BREAKPOINTS)) {
            QList<Breakpoint> breakpoints;
            for (const auto &item : resp[DAP_BREAKPOINTS].toArray()) {
                breakpoints.append(Breakpoint(item.toObject()));
            }
            Q_EMIT sourceBreakpoints(source.path, source.sourceReference.value_or(0), breakpoints);
        } else {
            QList<Breakpoint> breakpoints;
            for (const auto &item : resp[DAP_LINES].toArray()) {
                breakpoints.append(Breakpoint(item.toInt()));
            }
            Q_EMIT sourceBreakpoints(source.path, source.sourceReference.value_or(0), breakpoints);
        }
    } else {
        Q_EMIT sourceBreakpoints(source.path, source.sourceReference.value_or(0), std::nullopt);
    }
}

void Client::processResponseEvaluate(const Response &response, const QJsonValue &request)
{
    const auto &expression = request.toObject()[DAP_EXPRESSION].toString();
    if (response.success) {
        Q_EMIT expressionEvaluated(expression, EvaluateInfo(response.body.toObject()));
    } else {
        Q_EMIT expressionEvaluated(expression, std::nullopt);
    }
}

void Client::processResponseGotoTargets(const Response &response, const QJsonValue &request)
{
    const auto &req = request.toObject();
    const auto source = Source(req[DAP_SOURCE].toObject());
    const int line = req[DAP_LINE].toInt();
    if (response.success) {
        Q_EMIT gotoTargets(source, line, GotoTarget::parseList(response.body.toObject()[QStringLiteral("targets")].toArray()));
    } else {
        Q_EMIT gotoTargets(source, line, QList<GotoTarget>());
    }
}

void Client::setState(const State &state)
{
    if (state != m_state) {
        m_state = state;
        Q_EMIT stateChanged(m_state);

        switch (m_state) {
        case State::Initialized:
            Q_EMIT initialized();
            checkRunning();
            break;
        case State::Running:
            Q_EMIT debuggeeRunning();
            break;
        case State::Terminated:
            Q_EMIT debuggeeTerminated();
            break;
        case State::Failed:
            Q_EMIT failed();
            break;
        default:;
        }
    }
}

int Client::sequenceNumber()
{
    const int seq = m_seq;
    if (m_seq == std::numeric_limits<int>::max()) {
        m_seq = 0;
    } else {
        ++m_seq;
    }
    return seq;
}

void Client::write(const QJsonObject &msg)
{
    const auto payload = QJsonDocument(msg).toJson();

    qCDebug(DAPCLIENT) << "--> " << msg;

    // write header
    m_bus->write(DAP_TPL_HEADER_FIELD.arg(QLatin1String(DAP_CONTENT_LENGTH)).arg(payload.size()).toLatin1());
    m_bus->write(DAP_SEP);
    // write payload
    m_bus->write(payload);
}

/*
 * command: str
 * arguments?: any
 */
QJsonObject Client::makeRequest(const QString &command, const QJsonValue &arguments, const ResponseHandler &handler)
{
    const int seq = sequenceNumber();
    QJsonObject message;
    // ProtocolMessage
    message[DAP_SEQ] = seq;
    message[DAP_TYPE] = DAP_REQUEST;
    // Request
    message[DAP_COMMAND] = command;
    if (!arguments.isUndefined()) {
        message[DAP_ARGUMENTS] = arguments;
    }
    m_requests[seq] = std::make_tuple(command, arguments, handler);

    return message;
}

void Client::requestInitialize()
{
    const QJsonObject capabilities{// TODO clientID?: string
                                   // TODO clientName?: string
                                   {QStringLiteral("locale"), m_protocol.locale},
                                   {DAP_ADAPTER_ID, QStringLiteral("qdap")},
                                   {DAP_LINES_START_AT1, m_protocol.linesStartAt1},
                                   {DAP_COLUMNS_START_AT2, m_protocol.columnsStartAt1},
                                   {DAP_PATH, (m_protocol.pathFormatURI ? DAP_URI : DAP_PATH)},
                                   {DAP_SUPPORTS_VARIABLE_TYPE, true},
                                   {DAP_SUPPORTS_VARIABLE_PAGING, false},
                                   {DAP_SUPPORTS_RUN_IN_TERMINAL_REQUEST, false},
                                   {DAP_SUPPORTS_MEMORY_REFERENCES, false},
                                   {DAP_SUPPORTS_PROGRESS_REPORTING, false},
                                   {DAP_SUPPORTS_INVALIDATED_EVENT, false},
                                   {DAP_SUPPORTS_MEMORY_EVENT, false}};

    setState(State::Initializing);
    this->write(makeRequest(DAP_INITIALIZE, capabilities, make_response_handler(&Client::processResponseInitialize, this)));
}

void Client::requestConfigurationDone()
{
    if (m_state != State::Initialized) {
        qCWarning(DAPCLIENT) << "trying to configure in an unexpected status";
        return;
    }

    if (!m_adapterCapabilities.supportsConfigurationDoneRequest) {
        Q_EMIT configured();
        return;
    }

    this->write(makeRequest(QStringLiteral("configurationDone"), QJsonValue(), make_response_handler(&Client::processResponseConfigurationDone, this)));
}

void Client::requestThreads()
{
    this->write(makeRequest(DAP_THREADS, QJsonValue(), make_response_handler(&Client::processResponseThreads, this)));
}

void Client::requestStackTrace(int threadId, int startFrame, int levels)
{
    const QJsonObject arguments{{DAP_THREAD_ID, threadId}, {QStringLiteral("startFrame"), startFrame}, {QStringLiteral("levels"), levels}};

    this->write(makeRequest(QStringLiteral("stackTrace"), arguments, make_response_handler(&Client::processResponseStackTrace, this)));
}

void Client::requestScopes(int frameId)
{
    const QJsonObject arguments{{DAP_FRAME_ID, frameId}};

    this->write(makeRequest(DAP_SCOPES, arguments, make_response_handler(&Client::processResponseScopes, this)));
}

void Client::requestVariables(int variablesReference, Variable::Type filter, int start, int count)
{
    QJsonObject arguments{
        {DAP_VARIABLES_REFERENCE, variablesReference},
        {DAP_START, start},
        {DAP_COUNT, count},
    };

    switch (filter) {
    case Variable::Type::Indexed:
        arguments[DAP_FILTER] = QStringLiteral("indexed");
        break;
    case Variable::Type::Named:
        arguments[DAP_FILTER] = QStringLiteral("named");
        break;
    default:;
    }

    this->write(makeRequest(DAP_VARIABLES, arguments, make_response_handler(&Client::processResponseVariables, this)));
}

void Client::requestModules(int start, int count)
{
    this->write(makeRequest(DAP_MODULES, QJsonObject{{DAP_START, start}, {DAP_COUNT, count}}, make_response_handler(&Client::processResponseModules, this)));
}

void Client::requestNext(int threadId, bool singleThread)
{
    QJsonObject arguments{{DAP_THREAD_ID, threadId}};
    if (singleThread) {
        arguments[DAP_SINGLE_THREAD] = true;
    }
    this->write(makeRequest(QStringLiteral("next"), arguments, make_response_handler(&Client::processResponseNext, this)));
}

void Client::requestStepIn(int threadId, bool singleThread)
{
    QJsonObject arguments{{DAP_THREAD_ID, threadId}};
    if (singleThread) {
        arguments[DAP_SINGLE_THREAD] = true;
    }
    this->write(makeRequest(QStringLiteral("stepIn"), arguments, make_response_handler(&Client::processResponseNext, this)));
}

void Client::requestStepOut(int threadId, bool singleThread)
{
    QJsonObject arguments{{DAP_THREAD_ID, threadId}};
    if (singleThread) {
        arguments[DAP_SINGLE_THREAD] = true;
    }
    this->write(makeRequest(QStringLiteral("stepOut"), arguments, make_response_handler(&Client::processResponseNext, this)));
}

void Client::requestGoto(int threadId, int targetId)
{
    const QJsonObject arguments{{DAP_THREAD_ID, threadId}, {DAP_TARGET_ID, targetId}};

    this->write(makeRequest(QStringLiteral("goto"), arguments, make_response_handler(&Client::processResponseNext, this)));
}

void Client::requestContinue(int threadId, bool singleThread)
{
    QJsonObject arguments{{DAP_THREAD_ID, threadId}};
    if (singleThread) {
        arguments[DAP_SINGLE_THREAD] = true;
    }
    this->write(makeRequest(QStringLiteral("continue"), arguments, make_response_handler(&Client::processResponseContinue, this)));
}

void Client::requestPause(int threadId)
{
    const QJsonObject arguments{{DAP_THREAD_ID, threadId}};

    this->write(makeRequest(QStringLiteral("pause"), arguments, make_response_handler(&Client::processResponsePause, this)));
}

void Client::requestTerminate(bool restart)
{
    QJsonObject arguments;
    if (restart) {
        arguments[QStringLiteral("restart")] = true;
    }

    this->write(makeRequest(QStringLiteral("terminate"), arguments, make_response_handler(&Client::processResponseTerminate, this)));
}

void Client::requestDisconnect(bool restart)
{
    QJsonObject arguments;
    if (restart) {
        arguments[QStringLiteral("restart")] = true;
    }

    this->write(makeRequest(QStringLiteral("disconnect"), arguments, make_response_handler(&Client::processResponseDisconnect, this)));
}

void Client::requestSource(const Source &source)
{
    QJsonObject arguments{{DAP_SOURCE_REFERENCE, source.sourceReference.value_or(0)}};
    const QJsonObject sourceArg{{DAP_SOURCE_REFERENCE, source.sourceReference.value_or(0)}, {DAP_PATH, source.path}};

    arguments[DAP_SOURCE] = sourceArg;

    this->write(makeRequest(DAP_SOURCE, arguments, make_response_handler(&Client::processResponseSource, this)));
}

void Client::requestSetBreakpoints(const QString &path, const QList<SourceBreakpoint> breakpoints, bool sourceModified)
{
    requestSetBreakpoints(Source(path), breakpoints, sourceModified);
}

void Client::requestSetBreakpoints(const Source &source, const QList<SourceBreakpoint> breakpoints, bool sourceModified)
{
    QJsonArray bpoints;
    for (const auto &item : breakpoints) {
        bpoints.append(item.toJson());
    }
    QJsonObject arguments{{DAP_SOURCE, source.toJson()}, {DAP_BREAKPOINTS, bpoints}, {QStringLiteral("sourceModified"), sourceModified}};

    this->write(makeRequest(QStringLiteral("setBreakpoints"), arguments, make_response_handler(&Client::processResponseSetBreakpoints, this)));
}

void Client::requestEvaluate(const QString &expression, const QString &context, std::optional<int> frameId)
{
    QJsonObject arguments{{DAP_EXPRESSION, expression}};
    if (!context.isEmpty()) {
        arguments[DAP_CONTEXT] = context;
    }
    if (frameId) {
        arguments[DAP_FRAME_ID] = *frameId;
    }

    this->write(makeRequest(QStringLiteral("evaluate"), arguments, make_response_handler(&Client::processResponseEvaluate, this)));
}

void Client::requestWatch(const QString &expression, std::optional<int> frameId)
{
    requestEvaluate(expression, QStringLiteral("watch"), frameId);
}

void Client::requestGotoTargets(const QString &path, const int line, const std::optional<int> column)
{
    requestGotoTargets(Source(path), line, column);
}

void Client::requestGotoTargets(const Source &source, const int line, const std::optional<int> column)
{
    QJsonObject arguments{{DAP_SOURCE, source.toJson()}, {DAP_LINE, line}};
    if (column) {
        arguments[DAP_COLUMN] = *column;
    }

    this->write(makeRequest(QStringLiteral("gotoTargets"), arguments, make_response_handler(&Client::processResponseGotoTargets, this)));
}

void Client::requestLaunchCommand()
{
    if (m_state != State::Initializing) {
        qCWarning(DAPCLIENT) << "trying to launch in an unexpected state";
        return;
    }
    if (m_launchCommand.isNull() || m_launchCommand.isEmpty())
        return;

    this->write(makeRequest(m_launchCommand, m_protocol.launchRequest, make_response_handler(&Client::processResponseLaunch, this)));
}

void Client::checkRunning()
{
    if (m_launched && m_configured && (m_state == State::Initialized)) {
        setState(State::Running);
    }
}

void Client::onServerOutput(const QString &message)
{
    Q_EMIT outputProduced(dap::Output(message, dap::Output::Category::Console));
}

void Client::onProcessOutput(const QString &message)
{
    Q_EMIT outputProduced(dap::Output(message, dap::Output::Category::Stdout));
}

QString Client::extractCommand(const QJsonObject &launchRequest)
{
    const auto &command = launchRequest[DAP_COMMAND].toString();
    if ((command != DAP_LAUNCH) && (command != DAP_ATTACH)) {
        qCWarning(DAPCLIENT) << "unsupported request command: " << command;
        return QString();
    }
    return command;
}

void Client::read()
{
    m_buffer.append(m_bus->read());

    while (true) {
        // read headers
        const auto info = readHeader();
        if (!info) {
            // incomplete header -> abort
            break; // PENDING
        }
        // read payload
        const auto data = m_buffer.mid(info->payloadStart, info->payloadLength);
        if (data.size() < info->payloadLength) {
            break; // PENDING
        }
        m_buffer.remove(0, info->payloadStart + info->payloadLength);

        // parse payload
        QJsonParseError jsonError;
        const auto message = QJsonDocument::fromJson(data, &jsonError);
        if ((jsonError.error != QJsonParseError::NoError) || message.isNull() || !message.isObject()) {
            qCWarning(DAPCLIENT) << "JSON bad format: " << jsonError.errorString();
            continue;
        }

        qDebug(DAPCLIENT) << "<-- " << message;

        // process message
        processProtocolMessage(message.object());
    }
}

std::optional<Client::HeaderInfo> Client::readHeader()
{
    int length = -1;
    int start = 0;
    int end = -1;

    auto discardExploredBuffer = [=]() mutable {
        m_buffer.remove(0, end);
        length = end = -1;
        start = 0;
    };

    while (true) {
        end = m_buffer.indexOf(DAP_SEP, start);
        if (end < 0) {
            if (m_buffer.size() > MAX_HEADER_SIZE) {
                m_buffer.clear();
            }
            length = -1;
            break; // PENDING
        }

        const auto header = m_buffer.mid(start, end - start);
        end += DAP_SEP_SIZE;

        // header block separator
        if (header.size() == 0) {
            if (length < 0) {
                // unexpected end of header
                qCWarning(DAPCLIENT) << "unexpected end of header block";
                discardExploredBuffer();
                continue;
            }
            break; // END HEADER (length>0, end>0)
        }

        // parse field
        const int sep = header.indexOf(":");
        if (sep < 0) {
            qCWarning(DAPCLIENT) << "cannot parse header field: " << header;
            discardExploredBuffer();
            continue; // CONTINUE HEADER
        }

        // parse content-length
        if (header.left(sep) == DAP_CONTENT_LENGTH) {
            bool ok = false;
            length = header.mid(sep + 1, header.size() - sep).toInt(&ok);
            if (!ok) {
                qCWarning(DAPCLIENT) << "invalid value: " << header;
                discardExploredBuffer();
                continue; // CONTINUE HEADER
            }
        }
        start = end;
    }

    if (length < 0) {
        return std::nullopt;
    }

    return HeaderInfo{end, length};
}

void Client::start()
{
    m_launched = false;
    m_configured = false;
    if (m_state != State::None) {
        qCWarning(DAPCLIENT) << "trying to re-start has no effect";
        return;
    }
    requestInitialize();
}

}
