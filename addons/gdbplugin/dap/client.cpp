/*
    SPDX-FileCopyrightText: 2022 Héctor Mesa Jiménez <wmj.py@gmx.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "client.h"
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <limits>

#include "dap/bus_selector.h"
#include "dapclient_debug.h"

#include "messages.h"

#include <exec_utils.h>

namespace dap
{
constexpr int MAX_HEADER_SIZE = 1 << 16;

struct ClientMessageContext : public MessageContext {
    Utils::PathMappingPtr pathMap;

    ClientMessageContext(Utils::PathMappingPtr pm)
        : pathMap(pm)
    {
    }

    QString toRemote(const QUrl &url) override
    {
        if (!pathMap)
            return url.toLocalFile();

        auto result = Utils::mapPath(*pathMap, url, true).toLocalFile();
        // if not able to map (e.g. src not present in remote env), then pass as-is
        // e.g. gdb might be able to find some heuristic match in symbol info
        return result.isEmpty() ? url.toLocalFile() : result;
    }

    virtual QUrl toLocal(const QString &path) override
    {
        auto url = QUrl::fromLocalFile(path);
        // pass along relative paths; can not be reasonably mapped
        // is a matter of search paths to resolve those
        if (!pathMap || QFileInfo(path).isRelative())
            return url;

        return Utils::mapPath(*pathMap, url, false);
    }
};

class Client::MessageParser
{
public:
    QByteArray m_buffer;

    void push(QByteArray d)
    {
        m_buffer.append(d);
    }
    QByteArray read();
    std::optional<Client::HeaderInfo> readHeader();
};

Client::Client(const settings::ProtocolSettings &protocolSettings, Bus *bus, Utils::PathMappingPtr pm, QObject *parent)
    : QObject(parent)
    , m_bus(bus)
    , m_managedBus(false)
    , m_protocol(protocolSettings)
    , m_launchCommand(extractCommand(protocolSettings.launchRequest))
    , m_msgParser(new MessageParser())
{
    m_msgContext = std::make_unique<ClientMessageContext>(pm);
    bind();
}

Client::Client(const settings::ClientSettings &clientSettings, Utils::PathMappingPtr pm, QObject *parent)
    : QObject(parent)
    , m_managedBus(true)
    , m_protocol(clientSettings.protocolSettings)
    , m_launchCommand(extractCommand(clientSettings.protocolSettings.launchRequest))
    , m_msgParser(new MessageParser())
    , m_outputMsgParser(new MessageParser())
{
    m_msgContext = std::make_unique<ClientMessageContext>(pm);
    m_checkExtraData = pm.get();
    m_bus = createBus(clientSettings.busSettings);
    m_bus->setParent(this);
    bind();
}

Client::~Client()
{
    detach();
    delete m_msgParser;
    delete m_outputMsgParser;
}

void Client::bind()
{
    connect(m_bus, &Bus::readyRead, this, &Client::read);
    connect(m_bus, &Bus::running, this, &Client::start);
    connect(m_bus, &Bus::closed, this, &Client::finished);
    if (m_protocol.redirectStderr)
        connect(m_bus, &Bus::serverOutput, this, &Client::onServerOutput);
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
    } else if (DAP_REQUEST == type) {
        processReverseRequest(msg);
    } else {
        qCWarning(DAPCLIENT, "unknown, empty or unexpected ProtocolMessage::%ls (%ls)", qUtf16Printable(DAP_TYPE), qUtf16Printable(type));
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
        qCWarning(DAPCLIENT, "unexpected requested seq in response");
        return;
    }

    const auto request = m_requests.take(response.request_seq);

    // check response
    if (response.command != request.command) {
        qCWarning(DAPCLIENT, "unexpected command in response: %ls (expected: %ls)", qUtf16Printable(response.command), qUtf16Printable(request.command));
    }
    if (response.isCancelled()) {
        qCWarning(DAPCLIENT, "request cancelled: %ls", qUtf16Printable(response.command));
    }

    if (!response.success) {
        Q_EMIT errorResponse(response.message, response.errorBody);
    }
    (this->*request.callback)(response, request.arguments);
}

void Client::processReverseRequest(const QJsonObject &msg)
{
    if (!msg.contains(DAP_SEQ) || msg[DAP_COMMAND].toString() != DAP_RUN_IN_TERMINAL) {
        // error response
        write(makeResponse(msg, false));
        return;
    }
    processRequestRunInTerminal(msg);
}

void Client::processResponseInitialize(const Response &response, const QJsonValue &)
{
    if (m_state != State::Initializing) {
        qCWarning(DAPCLIENT, "unexpected initialize response");
        setState(State::None);
        return;
    }

    if (!response.success && response.isCancelled()) {
        qCWarning(DAPCLIENT, "InitializeResponse error: %ls", qUtf16Printable(response.message));
        if (response.errorBody) {
            qCWarning(DAPCLIENT, "error %d %ls", response.errorBody->id, qUtf16Printable(response.errorBody->format));
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
        if ((m_state != State::Initializing)) {
            qCWarning(DAPCLIENT, "unexpected initialized event");
            return;
        }
        setState(State::Initialized);
    } else if (QStringLiteral("terminated") == event) {
        Q_EMIT debuggeeTerminated(true);
    } else if (QStringLiteral("exited") == event) {
        const int exitCode = body[QStringLiteral("exitCode")].toInt(-1);
        Q_EMIT debuggeeExited(exitCode);
    } else if (DAP_OUTPUT == event) {
        Q_EMIT outputProduced(Output(body, *m_msgContext));
    } else if (QStringLiteral("process") == event) {
        Q_EMIT debuggingProcess(ProcessInfo(body));
    } else if (QStringLiteral("thread") == event) {
        Q_EMIT threadChanged(ThreadEvent(body));
    } else if (QStringLiteral("stopped") == event) {
        Q_EMIT debuggeeStopped(StoppedEvent(body));
    } else if (QStringLiteral("module") == event) {
        Q_EMIT moduleChanged(ModuleEvent(body));
    } else if (QStringLiteral("continued") == event) {
        Q_EMIT debuggeeContinued(ContinuedEvent(body));
    } else if (DAP_BREAKPOINT == event) {
        Q_EMIT breakpointChanged(BreakpointEvent(body, *m_msgContext));
    } else {
        qCWarning(DAPCLIENT, "unsupported event: %ls", qUtf16Printable(event));
    }
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
        Q_EMIT threads(Thread::parseList(response.body.toObject()[DAP_THREADS].toArray()), false, {});
    } else {
        Q_EMIT threads({}, true, response.message);
    }
}

void Client::processResponseStackTrace(const Response &response, const QJsonValue &request)
{
    const int threadId = request.toObject()[DAP_THREAD_ID].toInt();
    if (response.success) {
        Q_EMIT stackTrace(threadId, StackTraceInfo(response.body.toObject(), *m_msgContext));
    } else {
        Q_EMIT stackTrace(threadId, StackTraceInfo());
    }
}

void Client::processResponseScopes(const Response &response, const QJsonValue &request)
{
    const int frameId = request.toObject()[DAP_FRAME_ID].toInt();
    if (response.success) {
        Q_EMIT scopes(frameId, Scope::parseList(response.body.toObject()[DAP_SCOPES].toArray(), *m_msgContext));
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

void Client::processResponseTerminate(const Response &r, const QJsonValue &)
{
    Q_EMIT debuggeeTerminated(r.success);
}

void Client::processResponseHotReload(const Response &, const QJsonValue &)
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
    const auto path = m_msgContext->toLocal(req[DAP_SOURCE].toObject()[DAP_PATH].toString());
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
    const auto source = Source(request.toObject()[DAP_SOURCE].toObject(), *m_msgContext);
    if (response.success) {
        const auto resp = response.body.toObject();
        if (resp.contains(DAP_BREAKPOINTS)) {
            QList<Breakpoint> breakpoints;
            for (const auto &item : resp[DAP_BREAKPOINTS].toArray()) {
                breakpoints.append(Breakpoint(item.toObject(), *m_msgContext));
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
    const auto source = Source(req[DAP_SOURCE].toObject(), *m_msgContext);
    const int line = req[DAP_LINE].toInt();
    if (response.success) {
        Q_EMIT gotoTargets(source, line, GotoTarget::parseList(response.body.toObject()[QStringLiteral("targets")].toArray()));
    } else {
        Q_EMIT gotoTargets(source, line, QList<GotoTarget>());
    }
}

void Client::processRequestRunInTerminal(const QJsonObject &msg)
{
    Q_EMIT debuggeeRequiresTerminal(RunInTerminalRequestArguments(msg[DAP_ARGUMENTS].toObject()),
                                    [this, msg](bool success, const std::optional<int> &processId, const std::optional<int> &terminalId) {
                                        auto response = makeResponse(msg, success);
                                        // Response
                                        if (success) {
                                            QJsonObject message;
                                            if (processId) {
                                                message[QStringLiteral("processId")] = *processId;
                                            }
                                            if (terminalId) {
                                                message[QStringLiteral("shellProcessId")] = *terminalId;
                                            }
                                            response[DAP_BODY] = message;
                                        }
                                        this->write(response);
                                    });
}

void Client::setState(const State &state)
{
    if (state != m_state) {
        m_state = state;

        switch (m_state) {
        case State::Initialized:
            Q_EMIT initialized();
            checkRunning();
            break;
        case State::Running:
            Q_EMIT debuggeeRunning();
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

    qCDebug(DAPCLIENT, " --> %s", payload.data());

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
QJsonObject Client::makeRequest(const QString &command, const QJsonValue &arguments, ResponseHandler handler)
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
    m_requests[seq] = Request{command, arguments, handler};

    return message;
}

QJsonObject Client::makeResponse(const QJsonObject &request, bool success)
{
    return QJsonObject{{DAP_SEQ, sequenceNumber()},
                       {DAP_TYPE, DAP_RESPONSE},
                       {DAP_REQUEST_SEQ, request[DAP_SEQ].toInt(-1)},
                       {DAP_COMMAND, request[DAP_COMMAND]},
                       {DAP_SUCCESS, success}};
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
                                   {DAP_SUPPORTS_RUN_IN_TERMINAL_REQUEST, m_protocol.runInTerminal},
                                   {DAP_SUPPORTS_MEMORY_REFERENCES, false},
                                   {DAP_SUPPORTS_PROGRESS_REPORTING, false},
                                   {DAP_SUPPORTS_INVALIDATED_EVENT, false},
                                   {DAP_SUPPORTS_MEMORY_EVENT, false}};

    setState(State::Initializing);
    this->write(makeRequest(DAP_INITIALIZE, capabilities, &Client::processResponseInitialize));
}

void Client::requestConfigurationDone()
{
    if (m_state != State::Initialized) {
        qCWarning(DAPCLIENT, "trying to configure in an unexpected status");
        return;
    }

    if (!m_adapterCapabilities.supportsConfigurationDoneRequest) {
        Q_EMIT configured();
        return;
    }

    this->write(makeRequest(QStringLiteral("configurationDone"), QJsonObject{}, &Client::processResponseConfigurationDone));
}

void Client::requestThreads()
{
    this->write(makeRequest(DAP_THREADS, QJsonObject{}, &Client::processResponseThreads));
}

void Client::requestStackTrace(int threadId, int startFrame, int levels)
{
    const QJsonObject arguments{{DAP_THREAD_ID, threadId}, {QStringLiteral("startFrame"), startFrame}, {QStringLiteral("levels"), levels}};

    this->write(makeRequest(QStringLiteral("stackTrace"), arguments, &Client::processResponseStackTrace));
}

void Client::requestScopes(int frameId)
{
    const QJsonObject arguments{{DAP_FRAME_ID, frameId}};

    this->write(makeRequest(DAP_SCOPES, arguments, &Client::processResponseScopes));
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

    this->write(makeRequest(DAP_VARIABLES, arguments, &Client::processResponseVariables));
}

void Client::requestModules(int start, int count)
{
    this->write(makeRequest(DAP_MODULES, QJsonObject{{DAP_START, start}, {DAP_COUNT, count}}, &Client::processResponseModules));
}

void Client::requestNext(int threadId, bool singleThread)
{
    QJsonObject arguments{{DAP_THREAD_ID, threadId}};
    if (singleThread) {
        arguments[DAP_SINGLE_THREAD] = true;
    }
    this->write(makeRequest(QStringLiteral("next"), arguments, &Client::processResponseNext));
}

void Client::requestStepIn(int threadId, bool singleThread)
{
    QJsonObject arguments{{DAP_THREAD_ID, threadId}};
    if (singleThread) {
        arguments[DAP_SINGLE_THREAD] = true;
    }
    this->write(makeRequest(QStringLiteral("stepIn"), arguments, &Client::processResponseNext));
}

void Client::requestStepOut(int threadId, bool singleThread)
{
    QJsonObject arguments{{DAP_THREAD_ID, threadId}};
    if (singleThread) {
        arguments[DAP_SINGLE_THREAD] = true;
    }
    this->write(makeRequest(QStringLiteral("stepOut"), arguments, &Client::processResponseNext));
}

void Client::requestGoto(int threadId, int targetId)
{
    const QJsonObject arguments{{DAP_THREAD_ID, threadId}, {DAP_TARGET_ID, targetId}};

    this->write(makeRequest(QStringLiteral("goto"), arguments, &Client::processResponseNext));
}

void Client::requestContinue(int threadId, bool singleThread)
{
    QJsonObject arguments{{DAP_THREAD_ID, threadId}};
    if (singleThread) {
        arguments[DAP_SINGLE_THREAD] = true;
    }
    this->write(makeRequest(QStringLiteral("continue"), arguments, &Client::processResponseContinue));
}

void Client::requestPause(int threadId)
{
    const QJsonObject arguments{{DAP_THREAD_ID, threadId}};

    this->write(makeRequest(QStringLiteral("pause"), arguments, &Client::processResponsePause));
}

void Client::requestTerminate(bool restart)
{
    QJsonObject arguments;
    if (restart) {
        arguments[QStringLiteral("restart")] = true;
    }

    this->write(makeRequest(QStringLiteral("terminate"), arguments, &Client::processResponseTerminate));
}

void Client::requestDisconnect(bool restart)
{
    QJsonObject arguments;
    if (restart) {
        arguments[QStringLiteral("restart")] = true;
    }

    this->write(makeRequest(QStringLiteral("disconnect"), arguments, &Client::processResponseDisconnect));
}

void Client::requestSource(const Source &source)
{
    auto reference = source.sourceReference.value_or(0);
    if (reference == 0 && source.path.scheme() == Source::referenceScheme())
        reference = source.path.path().toInt();
    QJsonObject arguments{{DAP_SOURCE_REFERENCE, reference}};
    const QJsonObject sourceArg{{DAP_SOURCE_REFERENCE, reference}, {DAP_PATH, m_msgContext->toRemote(source.path)}};

    arguments[DAP_SOURCE] = sourceArg;

    this->write(makeRequest(DAP_SOURCE, arguments, &Client::processResponseSource));
}

void Client::requestSetBreakpoints(const QUrl &path, const QList<SourceBreakpoint> &breakpoints, bool sourceModified)
{
    requestSetBreakpoints(Source(path), breakpoints, sourceModified);
}

void Client::requestSetBreakpoints(const Source &source, const QList<SourceBreakpoint> &breakpoints, bool sourceModified)
{
    QJsonArray bpoints;
    for (const auto &item : breakpoints) {
        bpoints.append(item.toJson());
    }
    QJsonObject arguments{{DAP_SOURCE, source.toJson(*m_msgContext)}, {DAP_BREAKPOINTS, bpoints}, {QStringLiteral("sourceModified"), sourceModified}};

    this->write(makeRequest(QStringLiteral("setBreakpoints"), arguments, &Client::processResponseSetBreakpoints));
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

    this->write(makeRequest(QStringLiteral("evaluate"), arguments, &Client::processResponseEvaluate));
}

void Client::requestWatch(const QString &expression, std::optional<int> frameId)
{
    requestEvaluate(expression, QStringLiteral("watch"), frameId);
}

void Client::requestGotoTargets(const QUrl &path, const int line, const std::optional<int> column)
{
    requestGotoTargets(Source(path), line, column);
}

void Client::requestGotoTargets(const Source &source, const int line, const std::optional<int> column)
{
    QJsonObject arguments{{DAP_SOURCE, source.toJson(*m_msgContext)}, {DAP_LINE, line}};
    if (column) {
        arguments[DAP_COLUMN] = *column;
    }

    this->write(makeRequest(QStringLiteral("gotoTargets"), arguments, &Client::processResponseGotoTargets));
}

void Client::requestLaunchCommand()
{
    if (m_state != State::Initializing) {
        qCWarning(DAPCLIENT, "trying to launch in an unexpected state");
        return;
    }
    if (m_launchCommand.isNull() || m_launchCommand.isEmpty())
        return;

    this->write(makeRequest(m_launchCommand, m_protocol.launchRequest, &Client::processResponseLaunch));
}

void Client::requestHotReload()
{
    this->write(makeRequest(QStringLiteral("hotReload"), {}, &Client::processResponseHotReload));
}

void Client::requestHotRestart()
{
    this->write(makeRequest(QStringLiteral("hotRestart"), {}, &Client::processResponseHotReload));
}

void Client::checkRunning()
{
    if (m_launched && m_configured && (m_state == State::Initialized)) {
        setState(State::Running);
    }
}

void Client::onServerOutput(const QByteArray &message)
{
    Q_EMIT outputProduced(dap::Output(QString::fromLocal8Bit(message), dap::Output::Category::Console));
}

void Client::onProcessOutput(const QByteArray &_message)
{
    auto message = _message;
    if (m_checkExtraData) {
        if (m_outputMsgParser->m_buffer.isEmpty() && !message.startsWith(DAP_CONTENT_LENGTH)) {
            m_checkExtraData = false;
        } else {
            m_outputMsgParser->push(message);
            auto extra = m_outputMsgParser->read();
            if (!extra.size()) {
                // need more
                return;
            }
            // only 1 attempt, so no more
            m_checkExtraData = false;
            processExtraData(extra);
            // consider remainder
            message = std::move(m_outputMsgParser->m_buffer);
            if (!message.size())
                return;
        }
    }
    if (m_protocol.redirectStdout)
        Q_EMIT outputProduced(dap::Output(QString::fromLocal8Bit(message), dap::Output::Category::Stdout));
}

QString Client::extractCommand(const QJsonObject &launchRequest)
{
    const auto &command = launchRequest[DAP_COMMAND].toString();
    if ((command != DAP_LAUNCH) && (command != DAP_ATTACH)) {
        qCWarning(DAPCLIENT, "unsupported request command: %ls", qUtf16Printable(command));
        return QString();
    }
    return command;
}

void Client::processExtraData(const QByteArray &data)
{
    auto ctx = static_cast<ClientMessageContext *>(m_msgContext.get());
    auto mapping = ctx->pathMap;
    Q_ASSERT(mapping);
    qCInfo(DAPCLIENT) << "process extra data" << data;
    bool ok = Utils::updateMapping(*mapping, data);
    qCInfo(DAPCLIENT) << "map updated" << ok << "now;\n" << *mapping;
}

void Client::read()
{
    m_msgParser->push(m_bus->read());
    while (true) {
        auto data = m_msgParser->read();
        if (!data.size())
            break;

        // check oob data
        if (m_checkExtraData && data.front() != '{' && !isblank(data.front())) {
            /* this does not make for valid json, so treat as extra */
            processExtraData(data);
            m_checkExtraData = false;
            continue;
        }

        // parse payload
        QJsonParseError jsonError;
        const auto message = QJsonDocument::fromJson(data, &jsonError);
        if ((jsonError.error != QJsonParseError::NoError) || message.isNull() || !message.isObject()) {
            qCWarning(DAPCLIENT, "JSON bad format: %ls", qUtf16Printable(jsonError.errorString()));
            continue;
        }

        qDebug(DAPCLIENT, "<-- %s", message.toJson().constData());

        // process message
        processProtocolMessage(message.object());
    }
}

QByteArray Client::MessageParser::read()
{
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

        return data;
    }

    return {};
}

std::optional<Client::HeaderInfo> Client::MessageParser::readHeader()
{
    int length = -1;
    int start = 0;
    int end = -1;

    auto discardExploredBuffer = [this, &length, &start, &end]() {
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
                qCWarning(DAPCLIENT, "unexpected end of header block");
                discardExploredBuffer();
                continue;
            }
            break; // END HEADER (length>0, end>0)
        }

        // parse field
        const int sep = header.indexOf(":");
        if (sep < 0) {
            qCWarning(DAPCLIENT, "cannot parse header field: %s", header.constData());
            discardExploredBuffer();
            continue; // CONTINUE HEADER
        }

        // parse content-length
        if (header.left(sep) == DAP_CONTENT_LENGTH) {
            bool ok = false;
            length = header.mid(sep + 1, header.size() - sep).toInt(&ok);
            if (!ok) {
                qCWarning(DAPCLIENT, "invalid value: %s", header.constData());
                discardExploredBuffer();
                continue; // CONTINUE HEADER
            }
        }
        start = end;
    }

    if (length < 0) {
        return std::nullopt;
    }

    return HeaderInfo{.payloadStart = end, .payloadLength = length};
}

void Client::start()
{
    m_launched = false;
    m_configured = false;
    if (m_state != State::None) {
        qCWarning(DAPCLIENT, "trying to re-start has no effect");
        return;
    }
    requestInitialize();
}

}

#include "moc_client.cpp"
