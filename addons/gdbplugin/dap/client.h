/*
    SPDX-FileCopyrightText: 2022 Héctor Mesa Jiménez <wmj.py@gmx.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include <QByteArray>
#include <QHash>
#include <QJsonObject>
#include <QObject>
#include <functional>
#include <optional>

#include "bus.h"
#include "entities.h"
#include "settings.h"

#include <exec_utils.h>

namespace dap
{
const int SEQ_START = 1;

class Client : public QObject
{
    Q_OBJECT
public:
    enum class State {
        None,
        Initializing,
        Initialized,
        Running,
        Failed
    };
    Q_ENUM(State)

    Client(const settings::ProtocolSettings &protocolSettings, Bus *bus, Utils::PathMappingPtr, QObject *parent = nullptr);

    Client(const settings::ClientSettings &clientSettings, Utils::PathMappingPtr, QObject *parent = nullptr);

    ~Client() override;

    [[nodiscard]] Bus *bus() const
    {
        return m_bus;
    }

    void start();
    [[nodiscard]] settings::ProtocolSettings protocol() const
    {
        return m_protocol;
    }
    [[nodiscard]] Capabilities adapterCapabilities() const
    {
        return m_adapterCapabilities;
    }
    [[nodiscard]] bool isServerConnected() const;

    [[nodiscard]] bool supportsTerminate() const;

    /*
     * requests
     */
    void requestConfigurationDone();
    void requestThreads();
    void requestStackTrace(int threadId, int startFrame = 0, int levels = 0);
    void requestScopes(int frameId);
    void requestVariables(int variablesReference, Variable::Type filter = Variable::Type::Both, int start = 0, int count = 0);
    void requestModules(int start = 0, int count = 0);
    void requestNext(int threadId, bool singleThread = false);
    void requestStepIn(int threadId, bool singleThread = false);
    void requestStepOut(int threadId, bool singleThread = false);
    void requestGoto(int threadId, int targetId);
    void requestContinue(int threadId, bool singleThread = false);
    void requestPause(int threadId);
    void requestTerminate(bool restart = false);
    void requestDisconnect(bool restart = false);
    void requestSource(const Source &source);
    void requestSetBreakpoints(const QUrl &path, const QList<dap::SourceBreakpoint> &breakpoints, bool sourceModified = false);
    void requestSetBreakpoints(const dap::Source &source, const QList<dap::SourceBreakpoint> &breakpoints, bool sourceModified = false);
    void requestSetFunctionBreakpoints(const QList<dap::FunctionBreakpoint> &breakpoints);
    void requestSetExceptionBreakpoints(const QStringList &filters);
    void requestEvaluate(const QString &expression, const QString &context, std::optional<int> frameId = std::nullopt);
    void requestWatch(const QString &expression, std::optional<int> frameId = std::nullopt);
    void requestGotoTargets(const QUrl &path, int line, std::optional<int> column = std::nullopt);
    void requestGotoTargets(const dap::Source &source, int line, std::optional<int> column = std::nullopt);
    void requestHotReload();
    void requestHotRestart();

    void detach();

    static QString extractCommand(const QJsonObject &launchRequest);

    using ResponseHandler = void (Client::*)(const Response &response, const QJsonValue &request);
    /*
     * - success
     * - foreground process id
     * - shell process id
     */
    typedef std::function<void(bool, const std::optional<int> &, const std::optional<int> &)> ProcessInTerminal;

Q_SIGNALS:
    void finished();
    void initialized();
    void launched();
    void configured();
    void failed();

    void capabilitiesReceived(const Capabilities &capabilities);
    void debuggeeRunning();
    void debuggeeTerminated(bool success);
    void debuggeeExited(int exitCode);
    void debuggeeStopped(const StoppedEvent &);
    void debuggeeContinued(const ContinuedEvent &);
    void outputProduced(const Output &);
    void debuggingProcess(const ProcessInfo &);
    void errorResponse(const QString &summary, const std::optional<Message> &message);
    void threadChanged(const ThreadEvent &);
    void moduleChanged(const ModuleEvent &);
    void threads(const QList<Thread> &, bool isError, const QString &errorString);
    void stackTrace(int threadId, const StackTraceInfo &);
    void scopes(int frameId, const QList<Scope> &);
    void variables(int variablesReference, const QList<Variable> &);
    void modules(const ModulesInfo &);
    void serverDisconnected();
    void sourceContent(const QUrl &path, int reference, const SourceContent &content);
    void sourceBreakpoints(const QUrl &path, int reference, const std::optional<QList<Breakpoint>> &breakpoints);
    void breakpointChanged(const BreakpointEvent &);
    void expressionEvaluated(const QString &expression, const std::optional<EvaluateInfo> &);
    void gotoTargets(const Source &source, int line, const QList<GotoTarget> &targets);
    void debuggeeRequiresTerminal(const RunInTerminalRequestArguments &args, const ProcessInTerminal &processCreated);
    void functionBreakpointsSet(const QList<dap::FunctionBreakpoint> &requestedBreakpoints, const QList<dap::Breakpoint> &response);

private:
    void setState(const State &state);
    void bind();
    void read();

    struct HeaderInfo {
        int payloadStart;
        int payloadLength;
    };
    /**
     * @brief readHeader
     * @param headerEnd position of the header's end
     * @return info extracted from header or nullopt if incomplete
     */
    std::optional<HeaderInfo> readHeader();

    void processProtocolMessage(const QJsonObject &msg);

    /*
     * responses
     */
    void processResponse(const QJsonObject &msg);
    void processResponseInitialize(const Response &response, const QJsonValue &);
    void processResponseConfigurationDone(const Response &response, const QJsonValue &);
    void processResponseLaunch(const Response &response, const QJsonValue &);
    void processResponseThreads(const Response &response, const QJsonValue &);
    void processResponseStackTrace(const Response &response, const QJsonValue &request);
    void processResponseScopes(const Response &response, const QJsonValue &request);
    void processResponseVariables(const Response &response, const QJsonValue &request);
    void processResponseModules(const Response &response, const QJsonValue &);
    void processResponseNext(const Response &response, const QJsonValue &);
    void processResponseContinue(const Response &response, const QJsonValue &);
    void processResponseTerminate(const Response &response, const QJsonValue &);
    void processResponseDisconnect(const Response &response, const QJsonValue &);
    void processResponseSource(const Response &response, const QJsonValue &);
    void processResponseSetBreakpoints(const Response &response, const QJsonValue &);
    void processResponseSetFunctionBreakpoints(const Response &response, const QJsonValue &);
    void processResponseSetExceptionBreakpoints(const Response &response, const QJsonValue &);
    void processResponseEvaluate(const Response &response, const QJsonValue &);
    void processResponseGotoTargets(const Response &response, const QJsonValue &);
    void processResponsePause(const Response &response, const QJsonValue &);
    void processResponseHotReload(const Response &response, const QJsonValue &);
    void processRequestRunInTerminal(const QJsonObject &msg);
    // reverse requests
    void processReverseRequest(const QJsonObject &msg);

    /*
     * events
     */
    void processEvent(const QJsonObject &msg);

    int sequenceNumber();

    void write(const QJsonObject &msg);

    /*
     * requests
     */
    QJsonObject makeRequest(const QString &command, const QJsonValue &arguments, ResponseHandler handler);
    QJsonObject makeResponse(const QJsonObject &response, bool success = false);
    void requestInitialize();
    void requestLaunchCommand();

    void checkRunning();
    void onServerOutput(const QByteArray &message);
    void onProcessOutput(const QByteArray &message);

    void processExtraData(const QByteArray &data);

    /*
     * server capabilities
     */
    Capabilities m_adapterCapabilities{};

    Bus *m_bus = nullptr;
    bool m_managedBus;
    int m_seq = SEQ_START;

    struct Request {
        QString command;
        QJsonValue arguments;
        ResponseHandler callback = nullptr;
    };
    QHash<int, Request> m_requests;

    State m_state = State::None;
    bool m_launched = false;
    bool m_configured = false;

    settings::ProtocolSettings m_protocol;
    QString m_launchCommand;
    std::unique_ptr<MessageContext> m_msgContext;

    class MessageParser;
    // unique_ptr does not work well with undefined type on some platforms
    MessageParser *const m_msgParser = nullptr;
    bool m_checkExtraData = false;
    MessageParser *const m_outputMsgParser = nullptr;
};

}
