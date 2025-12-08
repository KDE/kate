/*
    SPDX-FileCopyrightText: 2022 Héctor Mesa Jiménez <wmj.py@gmx.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "entities.h"
#include "messages.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <functional>
#include <utility>

static std::optional<int> parseOptionalInt(const QJsonValue &value)
{
    if (value.isNull() || value.isUndefined() || !value.isDouble()) {
        return std::nullopt;
    }
    return value.toInt();
}

static std::optional<bool> parseOptionalBool(const QJsonValue &value)
{
    if (value.isNull() || value.isUndefined() || !value.isBool()) {
        return std::nullopt;
    }
    return value.toBool();
}

static std::optional<QString> parseOptionalString(const QJsonValue &value)
{
    if (value.isNull() || value.isUndefined() || !value.isString()) {
        return std::nullopt;
    }
    return value.toString();
}

template<typename T, typename... Args>
static std::optional<T> parseOptionalObject(const QJsonValue &value, Args &&...args)
{
    if (value.isNull() || value.isUndefined() || !value.isObject()) {
        return std::nullopt;
    }
    return T(value.toObject(), std::forward<Args>(args)...);
}

template<typename T>
static std::optional<QHash<QString, T>> parseOptionalMap(const QJsonValue &value, const std::function<T(const QJsonValue &)> &convert)
{
    if (value.isNull() || value.isUndefined() || !value.isObject()) {
        return std::nullopt;
    }
    const auto &dict = value.toObject();
    QHash<QString, T> map;
    for (auto it = dict.begin(); it != dict.end(); ++it) {
        map[it.key()] = convert(it.value());
    }
    return map;
}

template<typename T, typename... Args>
static QList<T> parseObjectList(const QJsonArray &array, Args &&...args)
{
    QList<T> out;
    for (const auto &item : array) {
        out << T(item.toObject(), std::forward<Args>(args)...);
    }
    return out;
}

template<>
QStringList parseObjectList(const QJsonArray &array)
{
    QStringList out;
    for (const auto &item : array) {
        out << item.toString();
    }
    return out;
}

static std::optional<QList<int>> parseOptionalIntList(const QJsonValue &value)
{
    if (value.isNull() || value.isUndefined() || !value.isArray()) {
        return std::nullopt;
    }
    QList<int> values;
    for (const auto &item : value.toArray()) {
        values.append(item.toInt());
    }
    return values;
}

template<typename T, typename... Args>
static QJsonArray toJsonArray(const QList<T> &items, Args &&...args)
{
    QJsonArray out;
    for (const auto &item : items) {
        out << item.toJson(std::forward<Args>(args)...);
    }
    return out;
}

static QString value_as_string(const QJsonValue &x)
{
    return x.toString();
}

static std::optional<QString> value_as_optstring(const QJsonValue &x)
{
    if (x.isNull()) {
        return std::nullopt;
    }
    return x.toString();
}

namespace dap
{
Message::Message(const QJsonObject &body)
    : id(body[DAP_ID].toInt())
    , format(body[QStringLiteral("format")].toString())
    , variables(parseOptionalMap<QString>(body[QStringLiteral("variables")], value_as_string))
    , sendTelemetry(parseOptionalBool(body[QStringLiteral("sendTelemetry")]))
    , showUser(parseOptionalBool(body[QStringLiteral("showUser")]))
    , url(parseOptionalString(body[QStringLiteral("url")]))
    , urlLabel(parseOptionalString(body[QStringLiteral("urlLabel")]))
{
}

Response::Response(const QJsonObject &msg)
    : request_seq(msg[DAP_REQUEST_SEQ].toInt(-1))
    , success(msg[DAP_SUCCESS].toBool(false))
    , command(msg[DAP_COMMAND].toString())
    , message(msg[QStringLiteral("message")].toString())
    , body(msg[DAP_BODY])
    , errorBody(success ? std::nullopt : parseOptionalObject<Message>(body.toObject()[QStringLiteral("error")]))
{
}

bool Response::isCancelled() const
{
    return message == QStringLiteral("cancelled");
}

ProcessInfo::ProcessInfo(const QJsonObject &body)
    : name(body[DAP_NAME].toString())
    , systemProcessId(parseOptionalInt(body[DAP_SYSTEM_PROCESS_ID]))
    , isLocalProcess(parseOptionalBool(body[DAP_IS_LOCAL_PROCESS]))
    , startMethod(parseOptionalString(body[DAP_START_METHOD]))
    , pointerSize(parseOptionalInt(body[DAP_POINTER_SIZE]))
{
}

Output::Output(const QJsonObject &body, MessageContext &ctx)
    : category(Category::Unknown)
    , output(body[DAP_OUTPUT].toString())
    , group(std::nullopt)
    , variablesReference(parseOptionalInt(body[DAP_VARIABLES_REFERENCE]))
    , source(parseOptionalObject<Source>(DAP_SOURCE, ctx))
    , line(parseOptionalInt(body[DAP_LINE]))
    , column(parseOptionalInt(body[DAP_COLUMN]))
    , data(body[DAP_DATA])
{
    if (body.contains(DAP_GROUP)) {
        const auto value = body[DAP_GROUP].toString();
        if (DAP_START == value) {
            group = Group::Start;
        } else if (QStringLiteral("startCollapsed") == value) {
            group = Group::StartCollapsed;
        } else if (QStringLiteral("end") == value) {
            group = Group::End;
        }
    }
    if (body.contains(DAP_CATEGORY)) {
        const auto value = body[DAP_CATEGORY].toString();
        if (QStringLiteral("console") == value) {
            category = Category::Console;
        } else if (QStringLiteral("important") == value) {
            category = Category::Important;
        } else if (QStringLiteral("stdout") == value) {
            category = Category::Stdout;
        } else if (QStringLiteral("stderr") == value) {
            category = Category::Stderr;
        } else if (QStringLiteral("telemetry") == value) {
            category = Category::Telemetry;
        }
    }
}

Output::Output(const QString &output, const Output::Category &category)
    : category(category)
    , output(output)
{
}

bool Output::isSpecialOutput() const
{
    return (category != Category::Stderr) && (category != Category::Stdout);
}

QString Source::referenceScheme()
{
    return QStringLiteral("reference");
}

QUrl Source::unifiedId() const
{
    return getUnifiedId(path, sourceReference);
}

QUrl Source::getUnifiedId(const QUrl &path, std::optional<int> sourceReference)
{
    if (sourceReference.value_or(0) > 0) {
        // a bit unfortunate, but slightly less so than QString
        // as it clearly specifies a distinct namespace
        // will have to convert back if/when used somewhere
        auto url = QUrl::fromLocalFile(QString::number(*sourceReference));
        url.setScheme(referenceScheme());
        return url;
    }
    return path;
}

Source::Source(const QJsonObject &body, MessageContext &ctx)
    : name(body[DAP_NAME].toString())
    , path(ctx.toLocal(body[DAP_PATH].toString()))
    , sourceReference(parseOptionalInt(body[DAP_SOURCE_REFERENCE]))
    , presentationHint(parseOptionalString(body[DAP_PRESENTATION_HINT]))
    , origin(body[DAP_ORIGIN].toString())
    , adapterData(body[DAP_ADAPTER_DATA])
{
    // sources
    if (body.contains(DAP_SOURCES)) {
        const auto values = body[DAP_SOURCES].toArray();
        for (const auto &item : values) {
            sources << Source(item.toObject(), ctx);
        }
    }

    // checksums
    if (body.contains(DAP_CHECKSUMS)) {
        const auto values = body[DAP_CHECKSUMS].toArray();
        for (const auto &item : values) {
            checksums << Checksum(item.toObject());
        }
    }
}

Source::Source(const QUrl &path)
    : path(path)
{
}

bool Source::operator==(const Source &r) const
{
    return name == r.name && path == r.path && sourceReference == r.sourceReference && presentationHint == r.presentationHint && origin == r.origin
        && sources == r.sources && adapterData == r.adapterData && checksums == r.checksums;
}

QJsonObject Source::toJson(MessageContext &ctx) const
{
    QJsonObject out;
    if (!name.isEmpty()) {
        out[DAP_NAME] = name;
    }
    if (!path.isEmpty()) {
        out[DAP_PATH] = ctx.toRemote(path);
    }
    if (sourceReference) {
        out[DAP_SOURCE_REFERENCE] = *sourceReference;
    }
    if (presentationHint) {
        out[DAP_PRESENTATION_HINT] = *presentationHint;
    }
    if (!origin.isEmpty()) {
        out[DAP_ORIGIN] = origin;
    }
    if (!adapterData.isNull() && !adapterData.isUndefined()) {
        out[DAP_ADAPTER_DATA] = adapterData;
    }
    if (!sources.isEmpty()) {
        out[DAP_SOURCES] = toJsonArray(sources, ctx);
    }
    if (!checksums.isEmpty()) {
        out[DAP_CHECKSUMS] = toJsonArray(checksums);
    }
    return out;
}

Checksum::Checksum(const QJsonObject &body)
    : checksum(body[DAP_CHECKSUM].toString())
    , algorithm(body[DAP_ALGORITHM].toString())
{
}

bool Checksum::operator==(const Checksum &r) const
{
    return checksum == r.checksum && algorithm == r.algorithm;
}

QJsonObject Checksum::toJson() const
{
    QJsonObject out;
    out[DAP_CHECKSUM] = checksum;
    out[DAP_ALGORITHM] = algorithm;
    return out;
}

Capabilities::Capabilities(const QJsonObject &body)
    : supportsConfigurationDoneRequest(body[QStringLiteral("supportsConfigurationDoneRequest")].toBool())
    , supportsFunctionBreakpoints(body[QStringLiteral("supportsFunctionBreakpoints")].toBool())
    , supportsConditionalBreakpoints(body[QStringLiteral("supportsConditionalBreakpoints")].toBool())
    , supportsHitConditionalBreakpoints(body[QStringLiteral("supportsHitConditionalBreakpoints")].toBool())
    , supportsLogPoints(body[QStringLiteral("supportsLogPoints")].toBool())
    , supportsModulesRequest(body[QStringLiteral("supportsModulesRequest")].toBool())
    , supportsTerminateRequest(body[QStringLiteral("supportsTerminateRequest")].toBool())
    , supportTerminateDebuggee(body[QStringLiteral("supportTerminateDebuggee")].toBool())
    , supportsGotoTargetsRequest(body[QStringLiteral("supportsGotoTargetsRequest")].toBool())
{
}

ThreadEvent::ThreadEvent(const QJsonObject &body)
    : reason(body[DAP_REASON].toString())
    , threadId(body[DAP_THREAD_ID].toInt())
{
}

StoppedEvent::StoppedEvent(const QJsonObject &body)
    : reason(body[DAP_REASON].toString())
    , description(parseOptionalString(body[QStringLiteral("description")]))
    , threadId(body[DAP_THREAD_ID].toInt())
    , preserveFocusHint(parseOptionalBool(body[QStringLiteral("preserveFocusHint")]))
    , text(parseOptionalString(body[QStringLiteral("text")]))
    , allThreadsStopped(parseOptionalBool(body[QStringLiteral("allThreadsStopped")]))
    , hitBreakpointsIds(parseOptionalIntList(body[QStringLiteral("hitBreakpointsIds")]))
{
}

Thread::Thread(const QJsonObject &body)
    : id(body[DAP_ID].toInt())
    , name(body[DAP_NAME].toString())
{
}

Thread::Thread(const int id)
    : id(id)
{
}

QList<Thread> Thread::parseList(const QJsonArray &threads)
{
    return parseObjectList<Thread>(threads);
}

StackFrame::StackFrame(const QJsonObject &body, MessageContext &ctx)
    : id(body[DAP_ID].toInt())
    , name(body[DAP_NAME].toString())
    , source(parseOptionalObject<Source>(body[DAP_SOURCE], ctx))
    , line(body[DAP_LINE].toInt())
    , column(body[DAP_COLUMN].toInt())
    , endLine(parseOptionalInt(body[QStringLiteral("endLine")]))
    , canRestart(parseOptionalBool((body[QStringLiteral("canRestart")])))
    , instructionPointerReference(parseOptionalString(body[QStringLiteral("instructionPointerReference")]))
    , moduleId_int(parseOptionalInt(body[DAP_MODULE_ID]))
    , moduleId_str(parseOptionalString(body[DAP_MODULE_ID]))
    , presentationHint(parseOptionalString(body[DAP_PRESENTATION_HINT]))
{
}

StackTraceInfo::StackTraceInfo(const QJsonObject &body, MessageContext &ctx)
    : stackFrames(parseObjectList<StackFrame>(body[QStringLiteral("stackFrames")].toArray(), ctx))
    , totalFrames(parseOptionalInt(body[QStringLiteral("totalFrames")]))
{
}

Module::Module(const QJsonObject &body)
    : id_int(parseOptionalInt(body[DAP_ID]))
    , id_str(parseOptionalString(body[DAP_ID]))
    , name(body[DAP_NAME].toString())
    , path(parseOptionalString(body[DAP_PATH]))
    , isOptimized(parseOptionalBool(body[QStringLiteral("isOptimized")]))
    , isUserCode(parseOptionalBool(body[QStringLiteral("isUserCode")]))
    , version(parseOptionalString(body[QStringLiteral("version")]))
    , symbolStatus(parseOptionalString(body[QStringLiteral("symbolStatus")]))
    , symbolFilePath(parseOptionalString(body[QStringLiteral("symbolFilePath")]))
    , dateTimeStamp(parseOptionalString(body[QStringLiteral("dateTimeStamp")]))
    , addressRange(parseOptionalString(body[QStringLiteral("addressRange")]))
{
}

ModuleEvent::ModuleEvent(const QJsonObject &body)
    : reason(body[DAP_REASON].toString())
    , module(Module(body[QStringLiteral("module")].toObject()))
{
}

Scope::Scope(const QJsonObject &body, MessageContext &ctx)
    : name(body[DAP_NAME].toString())
    , presentationHint(parseOptionalString(body[DAP_PRESENTATION_HINT]))
    , variablesReference(body[DAP_VARIABLES_REFERENCE].toInt())
    , namedVariables(parseOptionalInt(body[QStringLiteral("namedVariables")]))
    , indexedVariables(parseOptionalInt(body[QStringLiteral("indexedVariables")]))
    , expensive(parseOptionalBool(body[QStringLiteral("expensive")]))
    , source(parseOptionalObject<Source>(body[QStringLiteral("source")], ctx))
    , line(parseOptionalInt(body[QStringLiteral("line")]))
    , column(parseOptionalInt(body[QStringLiteral("column")]))
    , endLine(parseOptionalInt(body[QStringLiteral("endLine")]))
    , endColumn(parseOptionalInt(body[QStringLiteral("endColumn")]))
{
}

Scope::Scope(int variablesReference, QString name)
    : name(std::move(name))
    , variablesReference(variablesReference)
{
}

QList<Scope> Scope::parseList(const QJsonArray &scopes, MessageContext &ctx)
{
    return parseObjectList<Scope>(scopes, ctx);
}

Variable::Variable(const QJsonObject &body)
    : name(body[DAP_NAME].toString())
    , value(body[QStringLiteral("value")].toString())
    , type(parseOptionalString(body[DAP_TYPE].toString()))
    , evaluateName(parseOptionalString(body[QStringLiteral("evaluateName")].toString()))
    , variablesReference(body[DAP_VARIABLES_REFERENCE].toInt())
    , namedVariables(parseOptionalInt(body[QStringLiteral("namedVariables")]))
    , indexedVariables(parseOptionalInt(body[QStringLiteral("indexedVariables")]))
    , memoryReference(parseOptionalString(body[QStringLiteral("memoryReference")]))
{
}

Variable::Variable(const QString &name, const QString &value, const int reference)
    : name(name)
    , value(value)
    , variablesReference(reference)
{
}

QList<Variable> Variable::parseList(const QJsonArray &variables)
{
    return parseObjectList<Variable>(variables);
}

ModulesInfo::ModulesInfo(const QJsonObject &body)
    : modules(parseObjectList<Module>(body[DAP_MODULES].toArray()))
    , totalModules(parseOptionalInt(body[QStringLiteral("totalModules")]))
{
}

ContinuedEvent::ContinuedEvent(const QJsonObject &body)
    : threadId(body[DAP_THREAD_ID].toInt())
    , allThreadsContinued(parseOptionalBool(body[DAP_ALL_THREADS_CONTINUED]))
{
}

ContinuedEvent::ContinuedEvent(int threadId, bool allThreadsContinued)
    : threadId(threadId)
    , allThreadsContinued(allThreadsContinued)
{
}

SourceContent::SourceContent(const QJsonObject &body)
    : content(body[QStringLiteral("content")].toString())
    , mimeType(parseOptionalString(body[QStringLiteral("mimeType")]))
{
}

SourceContent::SourceContent(const QString &path)
{
    const QFileInfo file(path);
    if (file.isFile() && file.exists()) {
        QFile fd(path);
        if (fd.open(QIODevice::ReadOnly | QIODevice::Text)) {
            content = QString::fromLocal8Bit(fd.readAll());
        }
    }
}

SourceBreakpoint::SourceBreakpoint(const QJsonObject &body)
    : line(body[DAP_LINE].toInt())
    , column(parseOptionalInt(body[DAP_COLUMN]))
    , condition(parseOptionalString(body[DAP_CONDITION]))
    , hitCondition(parseOptionalString(body[DAP_HIT_CONDITION]))
    , logMessage(parseOptionalString(body[QStringLiteral("logMessage")]))
{
}

SourceBreakpoint::SourceBreakpoint(const int line)
    : line(line)
{
}

QJsonObject SourceBreakpoint::toJson() const
{
    QJsonObject out;
    out[DAP_LINE] = line;
    if (condition) {
        out[DAP_CONDITION] = *condition;
    }
    if (column) {
        out[DAP_COLUMN] = *column;
    }
    if (hitCondition) {
        out[DAP_HIT_CONDITION] = *hitCondition;
    }
    if (logMessage) {
        out[DAP_LOG_MESSAGE] = *logMessage;
    }
    return out;
}

Breakpoint::Breakpoint(const QJsonObject &body, MessageContext &ctx)
    : id(parseOptionalInt(body[DAP_ID]))
    , verified(body[QStringLiteral("verified")].toBool())
    , message(parseOptionalString(body[QStringLiteral("message")]))
    , source(parseOptionalObject<Source>(body[DAP_SOURCE], ctx))
    , line(parseOptionalInt(body[DAP_LINE]))
    , column(parseOptionalInt(body[DAP_COLUMN]))
    , endLine(parseOptionalInt(body[DAP_END_LINE]))
    , endColumn(parseOptionalInt(body[DAP_END_COLUMN]))
    , instructionReference(parseOptionalString(body[QStringLiteral("instructionReference")]))
    , offset(parseOptionalInt(body[QStringLiteral("offset")]))
{
}

Breakpoint::Breakpoint(const int line)
    : line(line)
{
}

bool Breakpoint::operator==(const dap::Breakpoint &r) const
{
    return id == r.id && verified == r.verified && message == r.message && source == r.source && line == r.line && column == r.column && endLine == r.endLine
        && endColumn == r.endColumn && instructionReference == r.instructionReference && offset == r.offset;
}

FunctionBreakpoint::FunctionBreakpoint(const QJsonObject &body)
    : function(body[DAP_NAME].toString())
    , condition(parseOptionalString(body[DAP_CONDITION]))
    , hitCondition(parseOptionalString(body[DAP_HIT_CONDITION]))
{
}

FunctionBreakpoint::FunctionBreakpoint(const QString &functionName)
    : function(functionName)
{
}

QJsonObject FunctionBreakpoint::toJson() const
{
    QJsonObject out;
    out[DAP_NAME] = function;
    if (condition) {
        out[DAP_CONDITION] = condition.value();
    }
    if (hitCondition) {
        out[DAP_HIT_CONDITION] = hitCondition.value();
    }
    return out;
}

BreakpointEvent::BreakpointEvent(const QJsonObject &body, MessageContext &ctx)
    : reason(body[DAP_REASON].toString())
    , breakpoint(Breakpoint(body[DAP_BREAKPOINT].toObject(), ctx))
{
}

EvaluateInfo::EvaluateInfo(const QJsonObject &body)
    : result(body[DAP_RESULT].toString())
    , type(parseOptionalString(body[DAP_TYPE]))
    , variablesReference(body[DAP_VARIABLES_REFERENCE].toInt())
    , namedVariables(parseOptionalInt(body[QStringLiteral("namedVariables")]))
    , indexedVariables(parseOptionalInt(body[QStringLiteral("indexedVariables")]))
    , memoryReference(parseOptionalString(body[QStringLiteral("memoryReference")]))
{
}

GotoTarget::GotoTarget(const QJsonObject &body)
    : id(body[DAP_ID].toInt())
    , label(body[QStringLiteral("label")].toString())
    , line(body[DAP_LINE].toInt())
    , column(parseOptionalInt(body[DAP_COLUMN]))
    , endLine(parseOptionalInt(body[DAP_END_LINE]))
    , endColumn(parseOptionalInt(body[DAP_END_COLUMN]))
    , instructionPointerReference(parseOptionalString(body[QStringLiteral("instructionPointerReference")]))
{
}

QList<GotoTarget> GotoTarget::parseList(const QJsonArray &variables)
{
    return parseObjectList<GotoTarget>(variables);
}

RunInTerminalRequestArguments::RunInTerminalRequestArguments(const QJsonObject &body)
    : title(parseOptionalString(body[QStringLiteral("title")]))
    , cwd(body[QStringLiteral("cwd")].toString())
    , args(parseObjectList<QString>(body[QStringLiteral("args")].toArray()))
    , env(parseOptionalMap<std::optional<QString>>(body[QStringLiteral("env")].toObject(), value_as_optstring))
{
}
}

#include "moc_entities.cpp"
