/*
    SPDX-FileCopyrightText: 2022 Héctor Mesa Jiménez <wmj.py@gmx.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef DAP_CLIENT_H
#define DAP_CLIENT_H

#include <QByteArray>
#include <QHash>
#include <QJsonObject>
#include <QObject>
#include <functional>
#include <memory>
#include <optional>
#include <utility>

#include "bus.h"
#include "entities.h"
#include "settings.h"

class QJsonObject;

namespace dap
{
class Client : public QObject
{
    Q_OBJECT
public:
    enum class State { None, Initializing, Initialized, Running, Terminated, Failed };
    Q_ENUM(State)

    Client(const settings::ProtocolSettings &protocolSettings, Bus *bus, QObject *parent = nullptr);

    Client(const settings::ClientSettings &clientSettings, QObject *parent = nullptr);

    ~Client() override;

    Bus *bus() const
    {
        return m_bus;
    }

    void start();
    State state() const
    {
        return m_state;
    }
    settings::ProtocolSettings protocol() const
    {
        return m_protocol;
    }
    Capabilities adapterCapabilities() const
    {
        return m_adapterCapabilities;
    }
    bool isServerConnected() const;

    bool supportsTerminate() const;

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
    void requestSetBreakpoints(const QString &path, const QList<dap::SourceBreakpoint> breakpoints, bool sourceModified = false);
    void requestSetBreakpoints(const dap::Source &source, const QList<dap::SourceBreakpoint> breakpoints, bool sourceModified = false);
    void requestEvaluate(const QString &expression, const QString &context, std::optional<int> frameId = std::nullopt);
    void requestWatch(const QString &expression, std::optional<int> frameId = std::nullopt);
    void requestGotoTargets(const QString &path, const int line, const std::optional<int> column = std::nullopt);
    void requestGotoTargets(const dap::Source &source, const int line, const std::optional<int> column = std::nullopt);

    void detach();

    static QString extractCommand(const QJsonObject &launchRequest);

    typedef std::function<void(const Response &, const QJsonValue &)> ResponseHandler;

Q_SIGNALS:
    void finished();
    void stateChanged(State state);
    void initialized();
    void launched();
    void configured();
    void failed();

    void capabilitiesReceived(const Capabilities &capabilities);
    void debuggeeRunning();
    void debuggeeTerminated();
    void debuggeeExited(int exitCode);
    void debuggeeStopped(const StoppedEvent &);
    void debuggeeContinued(const ContinuedEvent &);
    void outputProduced(const Output &);
    void debuggingProcess(const ProcessInfo &);
    void errorResponse(const QString &summary, const std::optional<Message> &message);
    void threadChanged(const ThreadEvent &);
    void moduleChanged(const ModuleEvent &);
    void threads(const QList<Thread> &);
    void stackTrace(const int threadId, const StackTraceInfo &);
    void scopes(const int frameId, const QList<Scope> &);
    void variables(const int variablesReference, const QList<Variable> &);
    void modules(const ModulesInfo &);
    void serverDisconnected();
    void sourceContent(const QString &path, int reference, const SourceContent &content);
    void sourceBreakpoints(const QString &path, int reference, const std::optional<QList<Breakpoint>> &breakpoints);
    void breakpointChanged(const BreakpointEvent &);
    void expressionEvaluated(const QString &expression, const std::optional<EvaluateInfo> &);
    void gotoTargets(const Source &source, const int line, const QList<GotoTarget> &targets);

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
    void processResponseEvaluate(const Response &response, const QJsonValue &);
    void processResponseGotoTargets(const Response &response, const QJsonValue &);
    void processResponsePause(const Response &response, const QJsonValue &);

    /*
     * events
     */
    void processEvent(const QJsonObject &msg);
    void processEventInitialized();
    void processEventTerminated();
    void processEventExited(const QJsonObject &body);
    void processEventOutput(const QJsonObject &body);
    void processEventProcess(const QJsonObject &body);
    void processEventThread(const QJsonObject &body);
    void processEventStopped(const QJsonObject &body);
    void processEventModule(const QJsonObject &body);
    void processEventContinued(const QJsonObject &body);
    void processEventBreakpoint(const QJsonObject &body);

    int sequenceNumber();

    void write(const QJsonObject &msg);

    /*
     * requests
     */
    QJsonObject makeRequest(const QString &command, const QJsonValue &arguments, const ResponseHandler &handler);
    void requestInitialize();
    void requestLaunchCommand();

    void checkRunning();
    void onServerOutput(const QString &message);
    void onProcessOutput(const QString &message);

    /*
     * server capabilities
     */
    Capabilities m_adapterCapabilities;

    Bus *m_bus = nullptr;
    bool m_managedBus;
    QByteArray m_buffer;
    int m_seq = 0;
    QHash<int, std::tuple<QString, QJsonValue, ResponseHandler>> m_requests;

    State m_state = State::None;
    bool m_launched = false;
    bool m_configured = false;

    settings::ProtocolSettings m_protocol;
    QString m_launchCommand;
};

}
#endif // DAP_CLIENT_H
