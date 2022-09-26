/*
    SPDX-FileCopyrightText: 2022 Héctor Mesa Jiménez <wmj.py@gmx.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef DAP_ENTITIES_H
#define DAP_ENTITIES_H

#include <QHash>
#include <QJsonValue>
#include <QList>
#include <QString>
#include <optional>
#include <utility>

class QJsonObject;

namespace dap
{
struct Message {
    /**
     * @brief id unique identifier for the message
     */
    int id;
    /**
     * @brief format A format string for the message.
     *
     * Embedded variables have the form '{name}'. If variable
     * name starts with an underscore character, the variable
     * does not contain user data (PII) and can be safely used for
     * telemetry purposes.
     */
    QString format;
    /**
     * @brief variables An object used as a dictionary for looking
     * up the variables in the format string.
     */
    std::optional<QHash<QString, QString>> variables;
    /**
     * @brief sendTelemetry If true send telemetry
     */
    std::optional<bool> sendTelemetry;
    /**
     * @brief showUser If true show user
     */
    std::optional<bool> showUser;
    /**
     * @brief showUser An optional url where additional information
     * about this message can be found.
     */
    std::optional<QString> url;
    /**
     * @brief urlLabel An optional label that is presented to the user as the UI
     * for opening the url.
     */
    std::optional<QString> urlLabel;

    Message() = default;
    Message(const QJsonObject &body);
};

struct Response {
    int request_seq;
    bool success;
    QString command;
    QString message;
    QJsonValue body;
    std::optional<Message> errorBody;

    Response() = default;
    Response(const QJsonObject &msg);

    bool isCancelled() const;
};

struct ProcessInfo {
    /**
     * @brief name the logical name of the process
     *
     * This is usually the full path to process's executable file.
     */
    QString name;
    /**
     * @brief systemProcessId the system process id of the debugged process
     *
     * This property will be missing for non-system processes.
     */
    std::optional<int> systemProcessId;
    /**
     * @brief isLocalProcess if true, the process is running on the same computer as DA
     */
    std::optional<bool> isLocalProcess;
    /**
     * @brief startMethod describes how the debug engine started debugging this process.
     */
    std::optional<QString> startMethod;
    /**
     * @brief pointerSize the sized of a pointer or address for this process, in bits.
     *
     * This value may be used by clients when formatting addresses for display.
     */
    std::optional<int> pointerSize;

    ProcessInfo() = default;
    ProcessInfo(const QJsonObject &body);
};

struct Checksum {
    QString checksum;
    QString algorithm;

    Checksum() = default;
    Checksum(const QJsonObject &body);

    QJsonObject toJson() const;
};

struct Source {
    QString name;
    QString path;
    std::optional<int> sourceReference;
    std::optional<QString> presentationHint;
    QString origin;
    QList<Source> sources;
    QJsonValue adapterData;
    QList<Checksum> checksums;

    QString unifiedId() const;
    static QString getUnifiedId(const QString &path, std::optional<int> sourceReference);

    Source() = default;
    Source(const QJsonObject &body);
    Source(const QString &path);

    QJsonObject toJson() const;
};

struct SourceContent {
    QString content;
    std::optional<QString> mimeType;

    SourceContent() = default;
    SourceContent(const QJsonObject &body);
    SourceContent(const QString &path);
};

struct SourceBreakpoint {
    int line;
    std::optional<int> column;
    /**
     * An optional expression for conditional breakpoints.
     * It is only honored by a debug adapter if the capability
     * 'supportsConditionalBreakpoints' is true.
     */
    std::optional<QString> condition;
    /**
     * An optional expression that controls how many hits of the breakpoint are
     * ignored.
     * The backend is expected to interpret the expression as needed.
     * The attribute is only honored by a debug adapter if the capability
     * 'supportsHitConditionalBreakpoints' is true.
     */
    std::optional<QString> hitCondition;
    /**
     * If this attribute exists and is non-empty, the backend must not 'break'
     * (stop)
     * but log the message instead. Expressions within {} are interpolated.
     * The attribute is only honored by a debug adapter if the capability
     * 'supportsLogPoints' is true.
     */
    std::optional<QString> logMessage;

    SourceBreakpoint() = default;
    SourceBreakpoint(const QJsonObject &body);
    SourceBreakpoint(const int line);

    QJsonObject toJson() const;
};

struct Breakpoint {
    /**
     * An optional identifier for the breakpoint. It is needed if breakpoint
     * events are used to update or remove breakpoints.
     */
    std::optional<int> id;
    /**
     * If true breakpoint could be set (but not necessarily at the desired
     * location).
     */
    bool verified;
    /**
     * An optional message about the state of the breakpoint.
     * This is shown to the user and can be used to explain why a breakpoint could
     * not be verified.
     */
    std::optional<QString> message;
    std::optional<Source> source;
    /**
     * The start line of the actual range covered by the breakpoint.
     */
    std::optional<int> line;
    std::optional<int> column;
    std::optional<int> endLine;
    std::optional<int> endColumn;
    /**
     * An optional memory reference to where the breakpoint is set.
     */
    std::optional<QString> instructionReference;
    /**
     * An optional offset from the instruction reference.
     * This can be negative.
     */
    std::optional<int> offset;

    Breakpoint() = default;
    Breakpoint(const QJsonObject &body);
    Breakpoint(const int line);
};

class Output
{
    Q_GADGET
public:
    enum class Category { Console, Important, Stdout, Stderr, Telemetry, Unknown };

    Q_ENUM(Category)

    enum class Group {
        Start,
        StartCollapsed,
        End,
    };

    Category category;
    QString output;
    std::optional<Group> group;
    std::optional<int> variablesReference;
    std::optional<Source> source;
    std::optional<int> line;
    std::optional<int> column;
    QJsonValue data;

    Output() = default;
    Output(const QJsonObject &body);
    Output(const QString &output, const Category &category);

    bool isSpecialOutput() const;
};

struct Capabilities {
    bool supportsConfigurationDoneRequest;
    bool supportsFunctionBreakpoints;
    bool supportsConditionalBreakpoints;
    bool supportsHitConditionalBreakpoints;
    bool supportsLogPoints;
    bool supportsModulesRequest;
    bool supportsTerminateRequest;
    bool supportTerminateDebuggee;
    bool supportsGotoTargetsRequest;

    Capabilities() = default;
    Capabilities(const QJsonObject &body);
};

struct ThreadEvent {
    QString reason;
    int threadId;

    ThreadEvent() = default;
    ThreadEvent(const QJsonObject &body);
};

struct Module {
    std::optional<int> id_int;
    std::optional<QString> id_str;
    QString name;
    std::optional<QString> path;
    std::optional<bool> isOptimized;
    std::optional<bool> isUserCode;
    std::optional<QString> version;
    std::optional<QString> symbolStatus;
    std::optional<QString> symbolFilePath;
    std::optional<QString> dateTimeStamp;
    std::optional<QString> addressRange;

    Module() = default;
    Module(const QJsonObject &body);
};

struct ModulesInfo {
    QList<Module> modules;
    std::optional<int> totalModules;

    ModulesInfo() = default;
    ModulesInfo(const QJsonObject &body);
};

struct ModuleEvent {
    QString reason;
    Module module;

    ModuleEvent() = default;
    ModuleEvent(const QJsonObject &body);
};

struct StoppedEvent {
    QString reason;
    std::optional<QString> description;
    /**
     * @brief threadId The thread which was stopped.
     */
    std::optional<int> threadId;
    /**
     * @brief preserverFocusHint A value of true hints to the frontend that
     * this event should not change the focus
     */
    std::optional<bool> preserveFocusHint;
    /**
     * @brief text Additional information.
     */
    std::optional<QString> text;
    /**
     * @brief allThreadsStopped if true, a DA can announce
     * that all threads have stopped.
     * - The client should use this information to enable that all threads can be
     *  expanded to access their stacktraces.
     * - If the attribute is missing or false, only the thread with the given threadId
     *   can be expanded.
     */
    std::optional<bool> allThreadsStopped;
    /**
     * @brief hitBreakpointsIds ids of the breakpoints that triggered the event
     */
    std::optional<QList<int>> hitBreakpointsIds;

    StoppedEvent() = default;
    StoppedEvent(const QJsonObject &body);
};

struct ContinuedEvent {
    int threadId;
    /**
     * If 'allThreadsContinued' is true, a debug adapter can announce that all
     * threads have continued.
     */
    std::optional<bool> allThreadsContinued;

    ContinuedEvent() = default;
    ContinuedEvent(int threadId, bool allThreadsContinued);
    ContinuedEvent(const QJsonObject &body);
};

struct BreakpointEvent {
    QString reason;
    Breakpoint breakpoint;

    BreakpointEvent() = default;
    BreakpointEvent(const QJsonObject &body);
};

struct Thread {
    int id;
    QString name;

    Thread() = default;
    Thread(const QJsonObject &body);
    explicit Thread(const int id);

    static QList<Thread> parseList(const QJsonArray &threads);
};

struct StackFrame {
    int id;
    QString name;
    std::optional<Source> source;
    int line;
    int column;
    std::optional<int> endLine;
    std::optional<int> endColumn;
    std::optional<bool> canRestart;
    std::optional<QString> instructionPointerReference;
    std::optional<int> moduleId_int;
    std::optional<QString> moduleId_str;
    std::optional<QString> presentationHint;

    StackFrame() = default;
    StackFrame(const QJsonObject &body);
};

struct StackTraceInfo {
    QList<StackFrame> stackFrames;
    std::optional<int> totalFrames;

    StackTraceInfo() = default;
    StackTraceInfo(const QJsonObject &body);
};

struct Scope {
    QString name;
    std::optional<QString> presentationHint;
    int variablesReference;
    std::optional<int> namedVariables;
    std::optional<int> indexedVariables;
    std::optional<bool> expensive;
    std::optional<Source> source;
    std::optional<int> line;
    std::optional<int> column;
    std::optional<int> endLine;
    std::optional<int> endColumn;

    Scope() = default;
    Scope(const QJsonObject &body);
    Scope(int variablesReference, QString name);

    static QList<Scope> parseList(const QJsonArray &scopes);
};

struct Variable {
    enum Type { Indexed = 1, Named = 2, Both = 3 };

    QString name;
    QString value;
    std::optional<QString> type;
    /**
     * @brief evaluateName Optional evaluatable name of tihs variable which can be
     * passed to the EvaluateRequest to fetch the variable's value
     */
    std::optional<QString> evaluateName;
    /**
     * @brief variablesReference if > 0, its children can be retrieved by VariablesRequest
     */
    int variablesReference;
    /**
     * @brief namedVariables number of named child variables
     */
    std::optional<int> namedVariables;
    /**
     * @brief indexedVariables number of indexed child variables
     */
    std::optional<int> indexedVariables;
    /**
     * @brief memoryReference optional memory reference for the variable if the
     * variable represents executable code, such as a function pointer.
     * Requires 'supportsMemoryReferences'
     */
    std::optional<QString> memoryReference;

    Variable() = default;
    Variable(const QJsonObject &body);
    Variable(const QString &name, const QString &value, const int reference = 0);

    static QList<Variable> parseList(const QJsonArray &variables);
};

struct EvaluateInfo {
    QString result;
    std::optional<QString> type;
    int variablesReference;
    std::optional<int> namedVariables;
    std::optional<int> indexedVariables;
    std::optional<QString> memoryReference;

    EvaluateInfo();
    EvaluateInfo(const QJsonObject &body);
};

struct GotoTarget {
    int id;
    QString label;
    int line;
    std::optional<int> column;
    std::optional<int> endLine;
    std::optional<int> endColumn;
    std::optional<QString> instructionPointerReference;

    GotoTarget() = default;
    GotoTarget(const QJsonObject &body);

    static QList<GotoTarget> parseList(const QJsonArray &variables);
};

}

#endif // DAP_ENTITIES_H
