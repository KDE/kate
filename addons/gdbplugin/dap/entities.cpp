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
    , format(body[QLatin1String("format")].toString())
    , variables(parseOptionalMap<QString>(body[QLatin1String("variables")], value_as_string))
    , sendTelemetry(parseOptionalBool(body[QLatin1String("sendTelemetry")]))
    , showUser(parseOptionalBool(body[QLatin1String("showUser")]))
    , url(parseOptionalString(body[QLatin1String("url")]))
    , urlLabel(parseOptionalString(body[QLatin1String("urlLabel")]))
{
}

Response::Response(const QJsonObject &msg)
    : request_seq(msg[DAP_REQUEST_SEQ].toInt(-1))
    , success(msg[DAP_SUCCESS].toBool(false))
    , command(msg[DAP_COMMAND].toString())
    , message(msg[QLatin1String("message")].toString())
    , body(msg[DAP_BODY])
    , errorBody(success ? std::nullopt : parseOptionalObject<Message>(body.toObject()[QLatin1String("error")]))
{
}

bool Response::isCancelled() const
{
    return message == QLatin1String("cancelled");
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
        } else if (QLatin1String("startCollapsed") == value) {
            group = Group::StartCollapsed;
        } else if (QLatin1String("end") == value) {
            group = Group::End;
        }
    }
    if (body.contains(DAP_CATEGORY)) {
        const auto value = body[DAP_CATEGORY].toString();
        if (QLatin1String("console") == value) {
            category = Category::Console;
        } else if (QLatin1String("important") == value) {
            category = Category::Important;
        } else if (QLatin1String("stdout") == value) {
            category = Category::Stdout;
        } else if (QLatin1String("stderr") == value) {
            category = Category::Stderr;
        } else if (QLatin1String("telemetry") == value) {
            category = Category::Telemetry;
        }
    }
}

Output::Output(const QString &output_, const Output::Category &category_)
    : category(category_)
    , output(output_)
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

Source::Source(const QUrl &url)
    : path(url)
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

ExceptionBreakpointsFilter::ExceptionBreakpointsFilter(const QJsonObject &body)
    : filter(body[QLatin1String("filter")].toString())
    , label(body[QLatin1String("label")].toString())
    , description(parseOptionalString(body[QLatin1String("description")]))
    , defaultValue(parseOptionalBool(body[QLatin1String("defaultValue")]))
    , supportsCondition(parseOptionalBool(body[QLatin1String("supportsCondition")]))
    , conditionDescription(parseOptionalString(body[QLatin1String("conditionDescription")]))
{
}

QString ExceptionBreakpointsFilter::toString() const
{
    QString ret;
    ret += QStringLiteral("filter: %1, ").arg(filter);
    ret += QStringLiteral("label: %1").arg(label);
    if (description) {
        ret += QStringLiteral(", description: %1").arg(description.value());
    }
    if (defaultValue) {
        ret += QStringLiteral(", defaultValue: %1").arg(defaultValue.value());
    }
    if (supportsCondition) {
        ret += QStringLiteral(", supportsCondition: %1").arg(supportsCondition.value());
    }
    if (conditionDescription) {
        ret += QStringLiteral(", conditionDescription: %1").arg(conditionDescription.value());
    }
    ret += QStringLiteral("\n");
    return ret;
}

Capabilities::Capabilities(const QJsonObject &body)
    : supportsConfigurationDoneRequest(body[QLatin1String("supportsConfigurationDoneRequest")].toBool())
    , supportsFunctionBreakpoints(body[QLatin1String("supportsFunctionBreakpoints")].toBool())
    , supportsConditionalBreakpoints(body[QLatin1String("supportsConditionalBreakpoints")].toBool())
    , supportsHitConditionalBreakpoints(body[QLatin1String("supportsHitConditionalBreakpoints")].toBool())
    , supportsLogPoints(body[QLatin1String("supportsLogPoints")].toBool())
    , supportsModulesRequest(body[QLatin1String("supportsModulesRequest")].toBool())
    , supportsTerminateRequest(body[QLatin1String("supportsTerminateRequest")].toBool())
    , supportTerminateDebuggee(body[QLatin1String("supportTerminateDebuggee")].toBool())
    , supportsGotoTargetsRequest(body[QLatin1String("supportsGotoTargetsRequest")].toBool())
    , exceptionBreakpointFilters(parseObjectList<ExceptionBreakpointsFilter>(body[QLatin1String("exceptionBreakpointFilters")].toArray()))
{
}

ThreadEvent::ThreadEvent(const QJsonObject &body)
    : reason(body[DAP_REASON].toString())
    , threadId(body[DAP_THREAD_ID].toInt())
{
}

StoppedEvent::StoppedEvent(const QJsonObject &body)
    : reason(body[DAP_REASON].toString())
    , description(parseOptionalString(body[QLatin1String("description")]))
    , threadId(body[DAP_THREAD_ID].toInt())
    , preserveFocusHint(parseOptionalBool(body[QLatin1String("preserveFocusHint")]))
    , text(parseOptionalString(body[QLatin1String("text")]))
    , allThreadsStopped(parseOptionalBool(body[QLatin1String("allThreadsStopped")]))
    , hitBreakpointIds(parseOptionalIntList(body[QLatin1String("hitBreakpointIds")]))
{
}

Thread::Thread(const QJsonObject &body)
    : id(body[DAP_ID].toInt())
    , name(body[DAP_NAME].toString())
{
}

Thread::Thread(const int tid)
    : id(tid)
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
    , endLine(parseOptionalInt(body[QLatin1String("endLine")]))
    , canRestart(parseOptionalBool((body[QLatin1String("canRestart")])))
    , instructionPointerReference(parseOptionalString(body[QLatin1String("instructionPointerReference")]))
    , moduleId_int(parseOptionalInt(body[DAP_MODULE_ID]))
    , moduleId_str(parseOptionalString(body[DAP_MODULE_ID]))
    , presentationHint(parseOptionalString(body[DAP_PRESENTATION_HINT]))
{
}

StackTraceInfo::StackTraceInfo(const QJsonObject &body, MessageContext &ctx)
    : stackFrames(parseObjectList<StackFrame>(body[QLatin1String("stackFrames")].toArray(), ctx))
    , totalFrames(parseOptionalInt(body[QLatin1String("totalFrames")]))
{
}

Module::Module(const QJsonObject &body)
    : id_int(parseOptionalInt(body[DAP_ID]))
    , id_str(parseOptionalString(body[DAP_ID]))
    , name(body[DAP_NAME].toString())
    , path(parseOptionalString(body[DAP_PATH]))
    , isOptimized(parseOptionalBool(body[QLatin1String("isOptimized")]))
    , isUserCode(parseOptionalBool(body[QLatin1String("isUserCode")]))
    , version(parseOptionalString(body[QLatin1String("version")]))
    , symbolStatus(parseOptionalString(body[QLatin1String("symbolStatus")]))
    , symbolFilePath(parseOptionalString(body[QLatin1String("symbolFilePath")]))
    , dateTimeStamp(parseOptionalString(body[QLatin1String("dateTimeStamp")]))
    , addressRange(parseOptionalString(body[QLatin1String("addressRange")]))
{
}

ModuleEvent::ModuleEvent(const QJsonObject &body)
    : reason(body[DAP_REASON].toString())
    , module(Module(body[QLatin1String("module")].toObject()))
{
}

Scope::Scope(const QJsonObject &body, MessageContext &ctx)
    : name(body[DAP_NAME].toString())
    , presentationHint(parseOptionalString(body[DAP_PRESENTATION_HINT]))
    , variablesReference(body[DAP_VARIABLES_REFERENCE].toInt())
    , namedVariables(parseOptionalInt(body[QLatin1String("namedVariables")]))
    , indexedVariables(parseOptionalInt(body[QLatin1String("indexedVariables")]))
    , expensive(parseOptionalBool(body[QLatin1String("expensive")]))
    , source(parseOptionalObject<Source>(body[QLatin1String("source")], ctx))
    , line(parseOptionalInt(body[QLatin1String("line")]))
    , column(parseOptionalInt(body[QLatin1String("column")]))
    , endLine(parseOptionalInt(body[QLatin1String("endLine")]))
    , endColumn(parseOptionalInt(body[QLatin1String("endColumn")]))
{
}

Scope::Scope(int varRef, QString name_)
    : name(std::move(name_))
    , variablesReference(varRef)
{
}

QList<Scope> Scope::parseList(const QJsonArray &scopes, MessageContext &ctx)
{
    return parseObjectList<Scope>(scopes, ctx);
}

Variable::Variable(const QJsonObject &body)
    : name(body[DAP_NAME].toString())
    , value(body[QLatin1String("value")].toString())
    , type(parseOptionalString(body[DAP_TYPE].toString()))
    , evaluateName(parseOptionalString(body[QLatin1String("evaluateName")].toString()))
    , variablesReference(body[DAP_VARIABLES_REFERENCE].toInt())
    , namedVariables(parseOptionalInt(body[QLatin1String("namedVariables")]))
    , indexedVariables(parseOptionalInt(body[QLatin1String("indexedVariables")]))
    , memoryReference(parseOptionalString(body[QLatin1String("memoryReference")]))
{
}

Variable::Variable(const QString &name_, const QString &val, const int reference)
    : name(name_)
    , value(val)
    , variablesReference(reference)
{
}

QList<Variable> Variable::parseList(const QJsonArray &variables)
{
    return parseObjectList<Variable>(variables);
}

ModulesInfo::ModulesInfo(const QJsonObject &body)
    : modules(parseObjectList<Module>(body[DAP_MODULES].toArray()))
    , totalModules(parseOptionalInt(body[QLatin1String("totalModules")]))
{
}

ContinuedEvent::ContinuedEvent(const QJsonObject &body)
    : threadId(body[DAP_THREAD_ID].toInt())
    , allThreadsContinued(parseOptionalBool(body[DAP_ALL_THREADS_CONTINUED]))
{
}

ContinuedEvent::ContinuedEvent(int threadId_, bool allThreadsContinued_)
    : threadId(threadId_)
    , allThreadsContinued(allThreadsContinued_)
{
}

SourceContent::SourceContent(const QJsonObject &body)
    : content(body[QLatin1String("content")].toString())
    , mimeType(parseOptionalString(body[QLatin1String("mimeType")]))
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
    , logMessage(parseOptionalString(body[QLatin1String("logMessage")]))
{
}

SourceBreakpoint::SourceBreakpoint(const int bpLine)
    : line(bpLine)
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
    , verified(body[QLatin1String("verified")].toBool())
    , message(parseOptionalString(body[QLatin1String("message")]))
    , source(parseOptionalObject<Source>(body[DAP_SOURCE], ctx))
    , line(parseOptionalInt(body[DAP_LINE]))
    , column(parseOptionalInt(body[DAP_COLUMN]))
    , endLine(parseOptionalInt(body[DAP_END_LINE]))
    , endColumn(parseOptionalInt(body[DAP_END_COLUMN]))
    , instructionReference(parseOptionalString(body[QLatin1String("instructionReference")]))
    , offset(parseOptionalInt(body[QLatin1String("offset")]))
{
}

Breakpoint::Breakpoint(const int line_)
    : line(line_)
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
    , namedVariables(parseOptionalInt(body[QLatin1String("namedVariables")]))
    , indexedVariables(parseOptionalInt(body[QLatin1String("indexedVariables")]))
    , memoryReference(parseOptionalString(body[QLatin1String("memoryReference")]))
{
}

GotoTarget::GotoTarget(const QJsonObject &body)
    : id(body[DAP_ID].toInt())
    , label(body[QLatin1String("label")].toString())
    , line(body[DAP_LINE].toInt())
    , column(parseOptionalInt(body[DAP_COLUMN]))
    , endLine(parseOptionalInt(body[DAP_END_LINE]))
    , endColumn(parseOptionalInt(body[DAP_END_COLUMN]))
    , instructionPointerReference(parseOptionalString(body[QLatin1String("instructionPointerReference")]))
{
}

QList<GotoTarget> GotoTarget::parseList(const QJsonArray &variables)
{
    return parseObjectList<GotoTarget>(variables);
}

RunInTerminalRequestArguments::RunInTerminalRequestArguments(const QJsonObject &body)
    : title(parseOptionalString(body[QLatin1String("title")]))
    , cwd(body[QLatin1String("cwd")].toString())
    , args(parseObjectList<QString>(body[QLatin1String("args")].toArray()))
    , env(parseOptionalMap<std::optional<QString>>(body[QLatin1String("env")].toObject(), value_as_optstring))
{
}
}

#include "moc_entities.cpp"
