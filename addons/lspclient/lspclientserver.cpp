/*
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>

    SPDX-License-Identifier: MIT
*/

#include "lspclientserver.h"

#include "hostprocess.h"
#include "lspclient_debug.h"

#include "ktexteditor_utils.h"
#include "lspclientprotocol.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>

#include <utility>

#include <qcompilerdetection.h>

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

// good/bad old school; allows easier concatenate
#define CONTENT_LENGTH "Content-Length"

static constexpr char MEMBER_ID[] = "id";
static constexpr char MEMBER_METHOD[] = "method";
static constexpr char MEMBER_ERROR[] = "error";
static constexpr char MEMBER_CODE[] = "code";
static constexpr char MEMBER_MESSAGE[] = "message";
static constexpr char MEMBER_PARAMS[] = "params";
static constexpr char MEMBER_RESULT[] = "result";
static constexpr char MEMBER_URI[] = "uri";
static constexpr char MEMBER_VERSION[] = "version";
static constexpr char MEMBER_START[] = "start";
static constexpr char MEMBER_END[] = "end";
static constexpr char MEMBER_POSITION[] = "position";
static constexpr char MEMBER_POSITIONS[] = "positions";
static constexpr char MEMBER_LOCATION[] = "location";
static constexpr char MEMBER_RANGE[] = "range";
static constexpr char MEMBER_LINE[] = "line";
static constexpr char MEMBER_CHARACTER[] = "character";
static constexpr char MEMBER_KIND[] = "kind";
static constexpr char MEMBER_TEXT[] = "text";
static constexpr char MEMBER_LANGID[] = "languageId";
static constexpr char MEMBER_LABEL[] = "label";
static constexpr char MEMBER_DETAIL[] = "detail";
static constexpr char MEMBER_COMMAND[] = "command";
static constexpr char MEMBER_ARGUMENTS[] = "arguments";
static constexpr char MEMBER_DIAGNOSTICS[] = "diagnostics";
static constexpr char MEMBER_PREVIOUS_RESULT_ID[] = "previousResultId";
static constexpr char MEMBER_QUERY[] = "query";
static constexpr char MEMBER_TARGET_URI[] = "targetUri";
static constexpr char MEMBER_TARGET_SELECTION_RANGE[] = "";
static constexpr char MEMBER_TARGET_RANGE[] = "targetRange";
static constexpr char MEMBER_DOCUMENTATION[] = "documentation";
static constexpr char MEMBER_TITLE[] = "title";
static constexpr char MEMBER_EDIT[] = "edit";
static constexpr char MEMBER_ACTIONS[] = "actions";

static QByteArray rapidJsonStringify(const rapidjson::Value &v)
{
    rapidjson::StringBuffer buf;
    rapidjson::Writer w(buf);
    v.Accept(w);
    return QByteArray(buf.GetString(), buf.GetSize());
}

static const rapidjson::Value &GetJsonValueForKey(const rapidjson::Value &v, std::string_view key)
{
    if (v.IsObject()) {
        rapidjson::Value keyRef(rapidjson::StringRef(key.data(), key.size()));
        auto it = v.FindMember(keyRef);
        if (it != v.MemberEnd()) {
            return it->value;
        }
    }
    static const rapidjson::Value nullvalue = rapidjson::Value(rapidjson::kNullType);
    return nullvalue;
}

static QString GetStringValue(const rapidjson::Value &v, std::string_view key)
{
    const auto &value = GetJsonValueForKey(v, key);
    if (value.IsString()) {
        return QString::fromUtf8(value.GetString(), value.GetStringLength());
    }
    return {};
}

static int GetIntValue(const rapidjson::Value &v, std::string_view key, int defaultValue = -1)
{
    const auto &value = GetJsonValueForKey(v, key);
    if (value.IsInt()) {
        return value.GetInt();
    }
    return defaultValue;
}

static bool GetBoolValue(const rapidjson::Value &v, std::string_view key)
{
    const auto &value = GetJsonValueForKey(v, key);
    if (value.IsBool()) {
        return value.GetBool();
    }
    return false;
}

static const rapidjson::Value &GetJsonObjectForKey(const rapidjson::Value &v, std::string_view key)
{
    const auto &value = GetJsonValueForKey(v, key);
    if (value.IsObject()) {
        return value;
    }
    static const rapidjson::Value dummy = rapidjson::Value(rapidjson::kObjectType);
    return dummy;
}

static const rapidjson::Value &GetJsonArrayForKey(const rapidjson::Value &v, std::string_view key)
{
    const auto &value = GetJsonValueForKey(v, key);
    if (value.IsArray()) {
        return value;
    }
    static const rapidjson::Value dummy = rapidjson::Value(rapidjson::kArrayType);
    return dummy;
}

static QJsonValue encodeUrl(const QUrl &url)
{
    return QJsonValue(QLatin1String(url.toEncoded()));
}

// message construction helpers
static QJsonObject to_json(const LSPPosition &pos)
{
    return QJsonObject{{QLatin1String(MEMBER_LINE), pos.line()}, {QLatin1String(MEMBER_CHARACTER), pos.column()}};
}

static QJsonObject to_json(const LSPRange &range)
{
    return QJsonObject{{QLatin1String(MEMBER_START), to_json(range.start())}, {QLatin1String(MEMBER_END), to_json(range.end())}};
}

static QJsonValue to_json(const LSPLocation &location)
{
    if (location.uri.isValid()) {
        return QJsonObject{{QLatin1String(MEMBER_URI), encodeUrl(location.uri)}, {QLatin1String(MEMBER_RANGE), to_json(location.range)}};
    }
    return QJsonValue();
}

static QJsonValue to_json(const LSPDiagnosticRelatedInformation &related)
{
    auto loc = to_json(related.location);
    if (loc.isObject()) {
        return QJsonObject{{QLatin1String(MEMBER_LOCATION), to_json(related.location)}, {QLatin1String(MEMBER_MESSAGE), related.message}};
    }
    return QJsonValue();
}

static QJsonObject to_json(const LSPDiagnostic &diagnostic)
{
    // required
    auto result = QJsonObject();
    result[QLatin1String(MEMBER_RANGE)] = to_json(diagnostic.range);
    result[QLatin1String(MEMBER_MESSAGE)] = diagnostic.message;
    // optional
    if (!diagnostic.code.isEmpty()) {
        result[QStringLiteral("code")] = diagnostic.code;
    }
    if (diagnostic.severity != LSPDiagnosticSeverity::Unknown) {
        result[QStringLiteral("severity")] = static_cast<int>(diagnostic.severity);
    }
    if (!diagnostic.source.isEmpty()) {
        result[QStringLiteral("source")] = diagnostic.source;
    }
    QJsonArray relatedInfo;
    for (const auto &vrelated : diagnostic.relatedInformation) {
        auto related = to_json(vrelated);
        if (related.isObject()) {
            relatedInfo.push_back(related);
        }
    }
    result[QStringLiteral("relatedInformation")] = relatedInfo;
    return result;
}

static QJsonArray to_json(const QList<LSPTextDocumentContentChangeEvent> &changes)
{
    QJsonArray result;
    for (const auto &change : changes) {
        result.push_back(QJsonObject{{QLatin1String(MEMBER_RANGE), to_json(change.range)}, {QLatin1String(MEMBER_TEXT), change.text}});
    }
    return result;
}

static QJsonArray to_json(const QList<LSPPosition> &positions)
{
    QJsonArray result;
    for (const auto &position : positions) {
        result.push_back(to_json(position));
    }
    return result;
}

static QJsonObject versionedTextDocumentIdentifier(const QUrl &document, int version = -1)
{
    QJsonObject map{{QLatin1String(MEMBER_URI), encodeUrl(document)}};
    if (version >= 0) {
        map[QLatin1String(MEMBER_VERSION)] = version;
    }
    return map;
}

static QJsonObject textDocumentItem(const QUrl &document, const QString &lang, const QString &text, int version)
{
    auto map = versionedTextDocumentIdentifier(document, version);
    map[QLatin1String(MEMBER_TEXT)] = text;
    map[QLatin1String(MEMBER_LANGID)] = lang;
    return map;
}

static QJsonObject textDocumentParams(const QJsonObject &m)
{
    return QJsonObject{{QStringLiteral("textDocument"), m}};
}

static QJsonObject textDocumentParams(const QUrl &document, int version = -1)
{
    return textDocumentParams(versionedTextDocumentIdentifier(document, version));
}

static QJsonObject textDocumentPositionParams(const QUrl &document, LSPPosition pos)
{
    auto params = textDocumentParams(document);
    params[QLatin1String(MEMBER_POSITION)] = to_json(pos);
    return params;
}

static QJsonObject textDocumentPositionsParams(const QUrl &document, const QList<LSPPosition> &positions)
{
    auto params = textDocumentParams(document);
    params[QLatin1String(MEMBER_POSITIONS)] = to_json(positions);
    return params;
}

static QJsonObject referenceParams(const QUrl &document, LSPPosition pos, bool decl)
{
    auto params = textDocumentPositionParams(document, pos);
    params[QStringLiteral("context")] = QJsonObject{{QStringLiteral("includeDeclaration"), decl}};
    return params;
}

static QJsonObject formattingOptions(const LSPFormattingOptions &_options)
{
    auto options = _options.extra;
    options[QStringLiteral("tabSize")] = _options.tabSize;
    options[QStringLiteral("insertSpaces")] = _options.insertSpaces;
    return options;
}

static QJsonObject documentRangeFormattingParams(const QUrl &document, const LSPRange *range, const LSPFormattingOptions &_options)
{
    auto params = textDocumentParams(document);
    if (range) {
        params[QLatin1String(MEMBER_RANGE)] = to_json(*range);
    }
    params[QStringLiteral("options")] = formattingOptions(_options);
    return params;
}

static QJsonObject documentOnTypeFormattingParams(const QUrl &document, const LSPPosition &pos, const QChar &lastChar, const LSPFormattingOptions &_options)
{
    auto params = textDocumentPositionParams(document, pos);
    params[QStringLiteral("ch")] = QString(lastChar);
    params[QStringLiteral("options")] = formattingOptions(_options);
    return params;
}

static QJsonObject renameParams(const QUrl &document, const LSPPosition &pos, const QString &newName)
{
    auto params = textDocumentPositionParams(document, pos);
    params[QStringLiteral("newName")] = newName;
    return params;
}

static QJsonObject codeActionParams(const QUrl &document, const LSPRange &range, const QList<QString> &kinds, const QList<LSPDiagnostic> &diagnostics)
{
    auto params = textDocumentParams(document);
    params[QLatin1String(MEMBER_RANGE)] = to_json(range);
    QJsonObject context;
    QJsonArray diags;
    for (const auto &diagnostic : diagnostics) {
        diags.push_back(to_json(diagnostic));
    }
    context[QLatin1String(MEMBER_DIAGNOSTICS)] = diags;
    if (!kinds.empty()) {
        context[QStringLiteral("only")] = QJsonArray::fromStringList(kinds);
    }
    params[QStringLiteral("context")] = context;
    return params;
}

static QJsonObject executeCommandParams(const LSPCommand &command)
{
    const auto doc = QJsonDocument::fromJson(command.arguments);
    QJsonValue args;
    if (doc.isArray()) {
        args = doc.array();
    } else {
        args = doc.object();
    }
    return QJsonObject{{QLatin1String(MEMBER_COMMAND), command.command}, {QLatin1String(MEMBER_ARGUMENTS), args}};
}

static QJsonObject applyWorkspaceEditResponse(const LSPApplyWorkspaceEditResponse &response)
{
    return QJsonObject{{QStringLiteral("applied"), response.applied}, {QStringLiteral("failureReason"), response.failureReason}};
}

static QJsonObject workspaceFolder(const LSPWorkspaceFolder &response)
{
    return QJsonObject{{QLatin1String(MEMBER_URI), encodeUrl(response.uri)}, {QStringLiteral("name"), response.name}};
}

static QJsonObject changeConfigurationParams(const QJsonValue &settings)
{
    return QJsonObject{{QStringLiteral("settings"), settings}};
}

static QJsonArray to_json(const QList<LSPWorkspaceFolder> &l)
{
    QJsonArray result;
    for (const auto &e : l) {
        result.push_back(workspaceFolder(e));
    }
    return result;
}

static QJsonObject changeWorkspaceFoldersParams(const QList<LSPWorkspaceFolder> &added, const QList<LSPWorkspaceFolder> &removed)
{
    QJsonObject event;
    event[QStringLiteral("added")] = to_json(added);
    event[QStringLiteral("removed")] = to_json(removed);
    return QJsonObject{{QStringLiteral("event"), event}};
}

static void from_json(QList<QChar> &trigger, const rapidjson::Value &json)
{
    if (json.IsArray()) {
        const auto triggersArray = json.GetArray();
        trigger.reserve(triggersArray.Size());
        for (const auto &t : triggersArray) {
            if (t.IsString() && t.GetStringLength() > 0) {
                trigger << QChar::fromLatin1(t.GetString()[0]);
            }
        }
    }
}

static void from_json(LSPCompletionOptions &options, const rapidjson::Value &json)
{
    if (json.IsObject()) {
        options.provider = true;
        options.resolveProvider = GetBoolValue(json, "resolveProvider");
        from_json(options.triggerCharacters, GetJsonArrayForKey(json, "triggerCharacters"));
    }
}

static void from_json(LSPSignatureHelpOptions &options, const rapidjson::Value &json)
{
    if (json.IsObject()) {
        options.provider = true;
        from_json(options.triggerCharacters, GetJsonArrayForKey(json, "triggerCharacters"));
    }
}

static void from_json(LSPDocumentOnTypeFormattingOptions &options, const rapidjson::Value &json)
{
    if (json.IsObject()) {
        options.provider = true;
        from_json(options.triggerCharacters, GetJsonArrayForKey(json, "moreTriggerCharacter"));
        const QString trigger = GetStringValue(json, "firstTriggerCharacter");
        if (!trigger.isEmpty()) {
            options.triggerCharacters.insert(0, trigger.at(0));
        }
    }
}

static void from_json(LSPWorkspaceFoldersServerCapabilities &options, const rapidjson::Value &json)
{
    if (json.IsObject()) {
        options.supported = GetBoolValue(json, "supported");
        auto it = json.FindMember("changeNotifications");
        if (it != json.MemberEnd()) {
            if (it->value.IsString()) {
                options.changeNotifications = it->value.GetStringLength() > 0;
            } else if (it->value.IsTrue()) {
                options.changeNotifications = true;
            }
        }
    }
}

static void from_json(LSPSemanticTokensOptions &options, const rapidjson::Value &json)
{
    if (!json.IsObject()) {
        return;
    }

    auto it = json.FindMember("full");
    if (it != json.MemberEnd()) {
        if (it->value.IsObject()) {
            options.fullDelta = GetBoolValue(it->value, "delta");
        } else {
            options.full = it->value.IsTrue();
        }
    }

    options.range = GetBoolValue(json, "range");

    it = json.FindMember("legend");
    if (it != json.MemberEnd()) {
        const auto &tokenTypes = GetJsonArrayForKey(it->value, "tokenTypes");
        const auto tokenTypesArray = tokenTypes.GetArray();
        std::vector<QString> types;
        types.reserve(tokenTypesArray.Size());
        for (const auto &tokenType : tokenTypesArray) {
            if (tokenType.IsString()) {
                types.push_back(QString::fromUtf8(tokenType.GetString()));
            }
        }
        options.legend.initialize(types);
    }
    // options.types = QList<QString>(types.begin(), types.end());
    // Disabled
    //     const auto tokenMods = legend.value(QStringLiteral("tokenModifiers")).toArray();
    //     std::vector<QString> modifiers;
    //     modifiers.reserve(tokenMods.size());
    //     std::transform(tokenMods.cbegin(), tokenMods.cend(), std::back_inserter(modifiers), [](const QJsonValue &jv) {
    //         return jv.toString();
    //     });
}

static void from_json(LSPServerCapabilities &caps, const rapidjson::Value &json)
{
    const auto &sync = GetJsonValueForKey(json, "textDocumentSync");
    if (sync.IsObject()) {
        caps.textDocumentSync.change = (LSPDocumentSyncKind)GetIntValue(sync, "change", (int)LSPDocumentSyncKind::None);
        auto it = sync.FindMember("save");
        if (it != sync.MemberEnd()) {
            caps.textDocumentSync.save = {GetBoolValue(it->value, "includeText")};
        }
    } else if (sync.IsInt()) {
        caps.textDocumentSync.change = LSPDocumentSyncKind(sync.GetInt());
    } else {
        caps.textDocumentSync.change = LSPDocumentSyncKind::None;
    }

    // in older protocol versions a support option is simply a boolean
    // in newer version it may be an object instead;
    // it should not be sent unless such support is announced, but let's handle it anyway
    // so consider an object there as a (good?) sign that the server is suitably capable
    // HasMember will thus just check the existence of a given key

    caps.hoverProvider = json.HasMember("hoverProvider");
    from_json(caps.completionProvider, GetJsonObjectForKey(json, "completionProvider"));
    from_json(caps.signatureHelpProvider, GetJsonObjectForKey(json, "signatureHelpProvider"));
    caps.definitionProvider = json.HasMember("definitionProvider");
    caps.declarationProvider = json.HasMember("declarationProvider");
    caps.typeDefinitionProvider = json.HasMember("typeDefinitionProvider");
    caps.referencesProvider = json.HasMember("referencesProvider");
    caps.implementationProvider = json.HasMember("implementationProvider");
    caps.documentSymbolProvider = json.HasMember("documentSymbolProvider");
    caps.documentHighlightProvider = json.HasMember("documentHighlightProvider");
    caps.documentFormattingProvider = json.HasMember("documentFormattingProvider");
    caps.documentRangeFormattingProvider = json.HasMember("documentRangeFormattingProvider");
    from_json(caps.documentOnTypeFormattingProvider, GetJsonObjectForKey(json, "documentOnTypeFormattingProvider"));
    caps.renameProvider = json.HasMember("renameProvider");
    caps.codeActionProvider = json.HasMember("codeActionProvider");
    from_json(caps.semanticTokenProvider, GetJsonObjectForKey(json, "semanticTokensProvider"));
    const auto &workspace = GetJsonObjectForKey(json, "workspace");
    from_json(caps.workspaceFolders, GetJsonObjectForKey(workspace, "workspaceFolders"));
    caps.selectionRangeProvider = json.HasMember("selectionRangeProvider");
    caps.inlayHintProvider = json.HasMember("inlayHintProvider");
}

static void from_json(LSPVersionedTextDocumentIdentifier &id, const rapidjson::Value &json)
{
    if (json.IsObject()) {
        id.uri = Utils::normalizeUrl(QUrl(GetStringValue(json, MEMBER_URI)));
        id.version = GetIntValue(json, MEMBER_VERSION, -1);
    }
}

static LSPResponseError parseResponseError(const rapidjson::Value &v)
{
    LSPResponseError ret;
    if (v.IsObject()) {
        ret.code = LSPErrorCode(GetIntValue(v, MEMBER_CODE));
        ret.message = GetStringValue(v, MEMBER_MESSAGE);
        auto it = v.FindMember("data");
        if (it != v.MemberEnd()) {
            ret.data = rapidJsonStringify(it->value);
        }
    }
    return ret;
}

static LSPMarkupContent parseMarkupContent(const rapidjson::Value &v)
{
    LSPMarkupContent ret;
    if (v.IsObject()) {
        ret.value = GetStringValue(v, "value");
        auto kind = GetStringValue(v, MEMBER_KIND);
        if (kind == QLatin1String("plaintext")) {
            ret.kind = LSPMarkupKind::PlainText;
        } else if (kind == QLatin1String("markdown")) {
            ret.kind = LSPMarkupKind::MarkDown;
        }
    } else if (v.IsString()) {
        ret.kind = LSPMarkupKind::PlainText;
        ret.value = QString::fromUtf8(v.GetString(), v.GetStringLength());
    }
    return ret;
}

static bool isPositionValid(const LSPPosition &pos)
{
    return pos.isValid();
}

static LSPPosition parsePosition(const rapidjson::Value &m)
{
    auto line = GetIntValue(m, MEMBER_LINE);
    auto column = GetIntValue(m, MEMBER_CHARACTER);
    return {line, column};
}

static LSPRange parseRange(const rapidjson::Value &range)
{
    auto start = parsePosition(GetJsonObjectForKey(range, MEMBER_START));
    auto end = parsePosition(GetJsonObjectForKey(range, MEMBER_END));
    return {start, end};
}

static std::shared_ptr<LSPSelectionRange> parseSelectionRange(const rapidjson::Value &selectionRange)
{
    auto current = std::make_shared<LSPSelectionRange>(LSPSelectionRange{});
    std::shared_ptr<LSPSelectionRange> ret = current;
    const rapidjson::Value *selRange = &selectionRange;
    while (selRange->IsObject()) {
        current->range = parseRange(GetJsonObjectForKey(*selRange, MEMBER_RANGE));
        auto it = selRange->FindMember("parent");
        if (it == selRange->MemberEnd() || !it->value.IsObject()) {
            current->parent = nullptr;
            break;
        }

        selRange = &(it->value);
        current->parent = std::make_shared<LSPSelectionRange>(LSPSelectionRange{});
        current = current->parent;
    }

    return ret;
}

static QList<std::shared_ptr<LSPSelectionRange>> parseSelectionRanges(const rapidjson::Value &result)
{
    QList<std::shared_ptr<LSPSelectionRange>> ret;
    if (!result.IsArray()) {
        return ret;
    }
    ret.reserve(result.Size());
    for (const auto &selectionRange : result.GetArray()) {
        ret.push_back(parseSelectionRange(selectionRange));
    }
    return ret;
}

static LSPLocation parseLocation(const rapidjson::Value &loc)
{
    auto uri = Utils::normalizeUrl(QUrl(GetStringValue(loc, MEMBER_URI)));
    KTextEditor::Range range;
    if (auto it = loc.FindMember(MEMBER_RANGE); it != loc.MemberEnd()) {
        range = parseRange(it->value);
    }
    return {QUrl(uri), range};
}

static LSPLocation parseLocationLink(const rapidjson::Value &loc)
{
    auto urlString = GetStringValue(loc, MEMBER_TARGET_URI);
    auto uri = Utils::normalizeUrl(QUrl(urlString));
    // both should be present, selection contained by the other
    // so let's preferentially pick the smallest one
    KTextEditor::Range range;
    if (auto it = loc.FindMember(MEMBER_TARGET_SELECTION_RANGE); it != loc.MemberEnd()) {
        range = parseRange(it->value);
    } else if (auto it = loc.FindMember(MEMBER_TARGET_RANGE); it != loc.MemberEnd()) {
        range = parseRange(it->value);
    }
    return {QUrl(uri), range};
}

static QList<LSPTextEdit> parseTextEdit(const rapidjson::Value &result)
{
    QList<LSPTextEdit> ret;
    if (!result.IsArray()) {
        return ret;
    }
    ret.reserve(result.Size());
    for (const auto &edit : result.GetArray()) {
        auto text = GetStringValue(edit, "newText");
        auto range = parseRange(GetJsonObjectForKey(edit, MEMBER_RANGE));
        ret.push_back({.range = range, .newText = std::move(text)});
    }
    return ret;
}

static LSPDocumentHighlight parseDocumentHighlight(const rapidjson::Value &result)
{
    auto range = parseRange(GetJsonObjectForKey(result, MEMBER_RANGE));
    // default is DocumentHighlightKind.Text
    auto kind = (LSPDocumentHighlightKind)GetIntValue(result, MEMBER_KIND, (int)LSPDocumentHighlightKind::Text);
    return {.range = range, .kind = kind};
}

static QList<LSPDocumentHighlight> parseDocumentHighlightList(const rapidjson::Value &result)
{
    QList<LSPDocumentHighlight> ret;
    // could be array
    if (result.IsArray()) {
        const auto defs = result.GetArray();
        for (const auto &def : defs) {
            ret.push_back(parseDocumentHighlight(def));
        }
    } else if (result.IsObject()) {
        // or a single value
        ret.push_back(parseDocumentHighlight(result));
    }
    return ret;
}

static LSPMarkupContent parseHoverContentElement(const rapidjson::Value &contents)
{
    return parseMarkupContent(contents);
}

static LSPHover parseHover(const rapidjson::Value &hover)
{
    LSPHover ret;
    if (!hover.IsObject()) {
        return ret;
    }

    // normalize content which can be of many forms
    // NOTE: might be invalid
    ret.range = parseRange(GetJsonObjectForKey(hover, MEMBER_RANGE));

    auto it = hover.FindMember("contents");

    // support the deprecated MarkedString[] variant, used by e.g. Rust rls
    if (it != hover.MemberEnd() && it->value.IsArray()) {
        const auto elements = it->value.GetArray();
        for (const auto &c : elements) {
            ret.contents.push_back(parseHoverContentElement(c));
        }
    } else if (it != hover.MemberEnd()) { // String | Object
        ret.contents.push_back(parseHoverContentElement(it->value));
    }
    return ret;
}

static std::list<LSPSymbolInformation> parseDocumentSymbols(const rapidjson::Value &result)
{
    // the reply could be old SymbolInformation[] or new (hierarchical) DocumentSymbol[]
    // try to parse it adaptively in any case
    // if new style, hierarchy is specified clearly in reply
    // if old style, it is assumed the values enter linearly, that is;
    // * a parent/container is listed before its children
    // * if a name is defined/declared several times and then used as a parent,
    //   then we try to find such a parent whose range contains current range
    //   (otherwise fall back to using the last instance as a parent)

    std::list<LSPSymbolInformation> ret;
    if (!result.IsArray()) {
        return ret;
    }
    // std::list provides stable references/iterators, so index by direct pointer is safe
    QMultiMap<QString, LSPSymbolInformation *> index;

    std::function<void(const rapidjson::Value &symbol, LSPSymbolInformation *parent)> parseSymbol = [&](const rapidjson::Value &symbol,
                                                                                                        LSPSymbolInformation *parent) {
        const auto &location = GetJsonObjectForKey(symbol, MEMBER_LOCATION);
        LSPRange range;
        if (symbol.HasMember(MEMBER_RANGE)) {
            range = parseRange(symbol[MEMBER_RANGE]);
        } else {
            range = parseRange(GetJsonObjectForKey(location, MEMBER_RANGE));
        }

        // if flat list, try to find parent by name
        if (!parent) {
            QString container = GetStringValue(symbol, "containerName");
            auto it = index.find(container);
            // default to last inserted
            if (it != index.end()) {
                parent = it.value();
            }
            // but prefer a containing range
            while (it != index.end() && it.key() == container) {
                if (it.value()->range.contains(range)) {
                    parent = it.value();
                    break;
                }
                ++it;
            }
        }
        auto list = parent ? &parent->children : &ret;
        if (isPositionValid(range.start()) && isPositionValid(range.end())) {
            QString name = GetStringValue(symbol, "name");
            auto kind = (LSPSymbolKind)GetIntValue(symbol, MEMBER_KIND);
            QString detail = GetStringValue(symbol, MEMBER_DETAIL);

            list->push_back({name, kind, range, detail});
            index.insert(name, &list->back());
            // proceed recursively
            const auto &children = GetJsonArrayForKey(symbol, "children");
            for (const auto &child : children.GetArray()) {
                parseSymbol(child, &list->back());
            }
        }
    };

    const auto symInfos = result.GetArray();
    for (const auto &info : symInfos) {
        parseSymbol(info, nullptr);
    }
    return ret;
}

static QList<LSPLocation> parseDocumentLocation(const rapidjson::Value &result)
{
    QList<LSPLocation> ret;
    // could be array
    if (result.IsArray()) {
        const auto locs = result.GetArray();
        ret.reserve(locs.Size());
        for (const auto &def : locs) {
            ret << parseLocation(def);

            // bogus server might have sent LocationLink[] instead
            // let's try to handle it, but not announce in capabilities
            if (ret.back().uri.isEmpty()) {
                ret.back() = parseLocationLink(def);
            }
        }
    } else if (result.IsObject()) {
        // or a single value
        ret.push_back(parseLocation(result));
    }
    return ret;
}

static LSPCompletionItem parseCompletionItem(const rapidjson::Value &item)
{
    auto label = GetStringValue(item, MEMBER_LABEL);
    auto detail = GetStringValue(item, MEMBER_DETAIL);
    LSPMarkupContent doc;
    auto it = item.FindMember(MEMBER_DOCUMENTATION);
    if (it != item.MemberEnd()) {
        doc = parseMarkupContent(it->value);
    }

    auto sortText = GetStringValue(item, "sortText");
    if (sortText.isEmpty()) {
        sortText = label;
    }
    auto insertText = GetStringValue(item, "insertText");
    LSPTextEdit lspTextEdit;
    const auto &textEdit = GetJsonObjectForKey(item, "textEdit");
    if (textEdit.IsObject()) {
        // Not a proper implementation of textEdit, but a workaround for KDE bug #445085
        auto newText = GetStringValue(textEdit, "newText");
        // Only override insertText with newText if insertText is empty. This avoids issues with
        // servers such typescript-language-server which will provide a different value in newText
        // which makes sense only if its used in combination with range. E.g.,
        // string.length is expected
        // but user gets => string..length because newText contains ".length"
        insertText = insertText.isEmpty() ? newText : insertText;
        lspTextEdit.newText = newText;
        lspTextEdit.range = parseRange(GetJsonObjectForKey(textEdit, "range"));
    }
    if (insertText.isEmpty()) {
        // if this happens, the server is broken but lets try the label anyways
        insertText = label;
    }
    auto kind = static_cast<LSPCompletionItemKind>(GetIntValue(item, MEMBER_KIND, 1));
    const auto additionalTextEdits = parseTextEdit(GetJsonArrayForKey(item, "additionalTextEdits"));

    auto dataIt = item.FindMember("data");
    QByteArray data;
    if (dataIt != item.MemberEnd()) {
        data = rapidJsonStringify(dataIt->value);
    }

    return {.label = label,
            .originalLabel = label,
            .kind = kind,
            .detail = detail,
            .documentation = doc,
            .sortText = sortText,
            .insertText = insertText,
            .additionalTextEdits = additionalTextEdits,
            .textEdit = lspTextEdit,
            .data = data};
}

static QList<LSPCompletionItem> parseDocumentCompletion(const rapidjson::Value &result)
{
    QList<LSPCompletionItem> ret;
    const rapidjson::Value *items = &result;

    // might be CompletionList
    auto &subItems = GetJsonArrayForKey(result, "items");
    if (!result.IsArray()) {
        items = &subItems;
    }

    if (!items->IsArray()) {
        qCWarning(LSPCLIENT) << "Unexpected, completion items is not an array";
        return ret;
    }

    const auto array = items->GetArray();
    for (const auto &item : array) {
        ret.push_back(parseCompletionItem(item));
    }
    return ret;
}

static LSPCompletionItem parseDocumentCompletionResolve(const rapidjson::Value &result)
{
    LSPCompletionItem ret;
    if (!result.IsObject()) {
        return ret;
    }
    return parseCompletionItem(result);
}

static LSPSignatureInformation parseSignatureInformation(const rapidjson::Value &json)
{
    LSPSignatureInformation info;

    info.label = GetStringValue(json, MEMBER_LABEL);
    auto it = json.FindMember(MEMBER_DOCUMENTATION);
    if (it != json.MemberEnd()) {
        info.documentation = parseMarkupContent(it->value);
    }
    const auto &params = GetJsonArrayForKey(json, "parameters");
    for (const auto &par : params.GetArray()) {
        auto label = par.FindMember(MEMBER_LABEL);
        int begin = -1, end = -1;
        if (label->value.IsArray()) {
            auto range = label->value.GetArray();
            if (range.Size() == 2) {
                begin = range[0].GetInt();
                end = range[1].GetInt();
                if (begin > info.label.length()) {
                    begin = -1;
                }
                if (end > info.label.length()) {
                    end = -1;
                }
            }
        } else if (label->value.IsString()) {
            auto str = label->value.GetString();
            QString sub = QString::fromUtf8(str, label->value.GetStringLength());
            if (sub.size()) {
                begin = info.label.indexOf(sub);
                if (begin >= 0) {
                    end = begin + sub.length();
                }
            }
        }
        info.parameters.push_back({.start = begin, .end = end});
    }
    return info;
}

static LSPSignatureHelp parseSignatureHelp(const rapidjson::Value &result)
{
    LSPSignatureHelp ret;
    if (!result.IsObject()) {
        return ret;
    }
    const auto sigInfos = GetJsonArrayForKey(result, "signatures").GetArray();
    for (const auto &info : sigInfos) {
        ret.signatures.push_back(parseSignatureInformation(info));
    }
    ret.activeSignature = GetIntValue(result, "activeSignature", 0);
    ret.activeParameter = GetIntValue(result, "activeParameter", 0);
    ret.activeSignature = qMin(qMax(ret.activeSignature, 0), ret.signatures.size());
    ret.activeParameter = qMax(ret.activeParameter, 0);
    if (!ret.signatures.isEmpty()) {
        ret.activeParameter = qMin(ret.activeParameter, ret.signatures.at(ret.activeSignature).parameters.size());
    }
    return ret;
}

static QString parseClangdSwitchSourceHeader(const rapidjson::Value &result)
{
    return result.IsString() ? QString::fromUtf8(result.GetString(), result.GetStringLength()) : QString();
}

static LSPExpandedMacro parseExpandedMacro(const rapidjson::Value &result)
{
    LSPExpandedMacro ret;
    ret.name = GetStringValue(result, "name");
    ret.expansion = GetStringValue(result, "expansion");
    return ret;
}

static LSPTextDocumentEdit parseTextDocumentEdit(const rapidjson::Value &result)
{
    LSPTextDocumentEdit ret;

    from_json(ret.textDocument, GetJsonObjectForKey(result, "textDocument"));
    const auto &edits = GetJsonArrayForKey(result, "edits");
    ret.edits = parseTextEdit(edits.GetArray());
    return ret;
}

static LSPWorkspaceEdit parseWorkSpaceEdit(const rapidjson::Value &result)
{
    LSPWorkspaceEdit ret;
    if (!result.IsObject()) {
        return ret;
    }

    const auto &changes = GetJsonObjectForKey(result, "changes");
    for (const auto &change : changes.GetObject()) {
        auto url = QString::fromUtf8(change.name.GetString());
        ret.changes.insert(Utils::normalizeUrl(QUrl(url)), parseTextEdit(change.value.GetArray()));
    }

    const auto &documentChanges = GetJsonArrayForKey(result, "documentChanges");
    // resourceOperations not supported for now
    for (const auto &edit : documentChanges.GetArray()) {
        ret.documentChanges.push_back(parseTextDocumentEdit(edit));
    }
    return ret;
}

static LSPCommand parseCommand(const rapidjson::Value &result)
{
    auto title = GetStringValue(result, MEMBER_TITLE);
    auto command = GetStringValue(result, MEMBER_COMMAND);
    auto args = rapidJsonStringify(GetJsonArrayForKey(result, MEMBER_ARGUMENTS));
    return {.title = title, .command = command, .arguments = args};
}

static QList<LSPDiagnostic> parseDiagnosticsArray(const rapidjson::Value &result)
{
    QList<LSPDiagnostic> ret;
    if (!result.IsArray()) {
        return ret;
    }
    const auto diags = result.GetArray();
    ret.reserve(diags.Size());
    for (const auto &vdiag : diags) {
        auto diag = vdiag.GetObject();

        auto it = diag.FindMember(MEMBER_RANGE);
        if (it == diag.end()) {
            continue;
        }
        auto range = parseRange(it->value);
        auto severity = static_cast<LSPDiagnosticSeverity>(GetIntValue(diag, "severity"));

        const auto &codeValue = GetJsonValueForKey(diag, "code");
        QString code;
        // code can be string or an integer
        if (codeValue.IsString()) {
            code = QString::fromUtf8(codeValue.GetString(), codeValue.GetStringLength());
        } else if (codeValue.IsInt()) {
            code = QString::number(codeValue.GetInt());
        }
        auto source = GetStringValue(diag, "source");
        auto message = GetStringValue(diag, MEMBER_MESSAGE);

        QList<LSPDiagnosticRelatedInformation> relatedInfoList;
        const auto &relInfoJson = GetJsonArrayForKey(diag, "relatedInformation");
        for (const auto &related : relInfoJson.GetArray()) {
            if (!related.IsObject()) {
                continue;
            }
            LSPLocation relLocation = parseLocation(GetJsonObjectForKey(related, MEMBER_LOCATION));
            auto relMessage = GetStringValue(related, MEMBER_MESSAGE);
            relatedInfoList.push_back({.location = relLocation, .message = relMessage});
        }

        ret.push_back({.range = range, .severity = severity, .code = code, .source = source, .message = message, .relatedInformation = relatedInfoList});
    }
    return ret;
}

static QList<LSPCodeAction> parseCodeAction(const rapidjson::Value &result)
{
    QList<LSPCodeAction> ret;
    if (!result.IsArray()) {
        return ret;
    }

    const auto codeActions = result.GetArray();
    for (const auto &action : codeActions) {
        // entry could be Command or CodeAction
        auto it = action.FindMember(MEMBER_COMMAND);
        const bool isCommand = it != action.MemberEnd() && it->value.IsString();
        if (!isCommand) {
            // CodeAction
            auto title = GetStringValue(action, MEMBER_TITLE);
            auto kind = GetStringValue(action, MEMBER_KIND);

            auto &commandJson = GetJsonObjectForKey(action, MEMBER_COMMAND);
            auto command = parseCommand(commandJson);
            auto edit = parseWorkSpaceEdit(GetJsonObjectForKey(action, MEMBER_EDIT));

            auto diagnostics = parseDiagnosticsArray(GetJsonArrayForKey(action, MEMBER_DIAGNOSTICS));
            ret.push_back({.title = title, .kind = kind, .diagnostics = diagnostics, .edit = edit, .command = command});
        } else {
            // Command
            auto command = parseCommand(action);
            ret.push_back({.title = command.title, .kind = QString(), .diagnostics = {}, .edit = {}, .command = command});
        }
    }
    return ret;
}

static QJsonArray supportedSemanticTokenTypes()
{
    return QJsonArray({QStringLiteral("namespace"), QStringLiteral("type"),     QStringLiteral("class"),         QStringLiteral("enum"),
                       QStringLiteral("interface"), QStringLiteral("struct"),   QStringLiteral("typeParameter"), QStringLiteral("parameter"),
                       QStringLiteral("variable"),  QStringLiteral("property"), QStringLiteral("enumMember"),    QStringLiteral("event"),
                       QStringLiteral("function"),  QStringLiteral("method"),   QStringLiteral("macro"),         QStringLiteral("keyword"),
                       QStringLiteral("modifier"),  QStringLiteral("comment"),  QStringLiteral("string"),        QStringLiteral("number"),
                       QStringLiteral("regexp"),    QStringLiteral("operator")});
}

/**
 * Used for both delta and full
 */
static LSPSemanticTokensDelta parseSemanticTokensDelta(const rapidjson::Value &result)
{
    LSPSemanticTokensDelta ret;
    if (!result.IsObject()) {
        return ret;
    }

    ret.resultId = GetStringValue(result, "resultId");

    const auto &edits = GetJsonArrayForKey(result, "edits");
    for (const auto &edit : edits.GetArray()) {
        if (!edit.IsObject()) {
            continue;
        }

        LSPSemanticTokensEdit e;
        e.start = GetIntValue(edit, "start");
        e.deleteCount = GetIntValue(edit, "deleteCount");

        const auto &data = GetJsonArrayForKey(edit, "data");
        const auto dataArray = data.GetArray();
        e.data.reserve(dataArray.Size());
        for (const auto &v : dataArray) {
            if (v.IsInt()) {
                e.data.push_back(v.GetInt());
            }
        }
        ret.edits.push_back(e);
    }

    auto data = GetJsonArrayForKey(result, "data").GetArray();
    ret.data.reserve(data.Size());
    for (const auto &v : data) {
        if (v.IsInt()) {
            ret.data.push_back(v.GetInt());
        }
    }

    return ret;
}

static std::vector<LSPInlayHint> parseInlayHints(const rapidjson::Value &result)
{
    std::vector<LSPInlayHint> ret;
    if (!result.IsArray()) {
        return ret;
    }

    const auto hints = result.GetArray();
    for (const auto &hint : hints) {
        LSPInlayHint h;
        auto labelIt = hint.FindMember("label");
        if (labelIt->value.IsArray()) {
            for (const auto &part : labelIt->value.GetArray()) {
                h.label += GetStringValue(part, "value");
            }
        } else if (labelIt->value.IsString()) {
            h.label = QString::fromUtf8(labelIt->value.GetString());
        }
        // skip if empty
        if (h.label.isEmpty()) {
            continue;
        }

        h.position = parsePosition(GetJsonObjectForKey(hint, "position"));
        h.paddingLeft = GetBoolValue(hint, "paddingLeft");
        h.paddingRight = GetBoolValue(hint, "paddingRight");
        // if the last position and current one is same, merge the labels
        if (!ret.empty() && ret.back().position == h.position) {
            ret.back().label += h.label;
        } else {
            ret.push_back(h);
        }
    }
    auto comp = [](const LSPInlayHint &l, const LSPInlayHint &r) {
        return l.position < r.position;
    };

    // it is likely to be already sorted
    if (!std::is_sorted(ret.begin(), ret.end(), comp)) {
        std::sort(ret.begin(), ret.end(), comp);
    }

    // printf("%s\n", QJsonDocument(result.toArray()).toJson().constData());
    return ret;
}

static LSPPublishDiagnosticsParams parseDiagnostics(const rapidjson::Value &result)
{
    LSPPublishDiagnosticsParams ret;

    auto it = result.FindMember(MEMBER_URI);
    if (it != result.MemberEnd()) {
        ret.uri = QUrl(QString::fromUtf8(it->value.GetString(), it->value.GetStringLength()));
    }

    it = result.FindMember(MEMBER_DIAGNOSTICS);
    if (it != result.MemberEnd()) {
        ret.diagnostics = parseDiagnosticsArray(it->value);
    }

    return ret;
}

static LSPApplyWorkspaceEditParams parseApplyWorkspaceEditParams(const rapidjson::Value &result)
{
    LSPApplyWorkspaceEditParams ret;
    ret.label = GetStringValue(result, MEMBER_LABEL);
    ret.edit = parseWorkSpaceEdit(GetJsonObjectForKey(result, MEMBER_EDIT));
    return ret;
}

static LSPShowMessageParams parseMessage(const rapidjson::Value &result)
{
    LSPShowMessageParams ret;
    ret.type = static_cast<LSPMessageType>(GetIntValue(result, "type", static_cast<int>(LSPMessageType::Log)));
    ret.message = GetStringValue(result, MEMBER_MESSAGE);
    return ret;
}

static void from_json(LSPWorkDoneProgressValue &value, const rapidjson::Value &json)
{
    if (!json.IsObject()) {
        return;
    }
    auto kind = GetStringValue(json, "kind");
    if (kind == QStringLiteral("begin")) {
        value.kind = LSPWorkDoneProgressKind::Begin;
    } else if (kind == QStringLiteral("report")) {
        value.kind = LSPWorkDoneProgressKind::Report;
    } else if (kind == QStringLiteral("end")) {
        value.kind = LSPWorkDoneProgressKind::End;
    }

    value.title = GetStringValue(json, "title");
    value.message = GetStringValue(json, "message");
    value.cancellable = GetBoolValue(json, "cancellable");
    int percentage = GetIntValue(json, "percentage", -1);
    if (percentage >= 0) {
        if (percentage > 100) {
            percentage = 100;
        }
        // force it to 100 if its not
        if (value.kind == LSPWorkDoneProgressKind::End && percentage != 100) {
            percentage = 100;
        }
        value.percentage = percentage;
    }
}

template<typename T>
static LSPProgressParams<T> parseProgress(const rapidjson::Value &json)
{
    LSPProgressParams<T> ret;

    ret.token = GetStringValue(json, "token");
    auto it = json.FindMember("value");
    if (it != json.MemberEnd()) {
        from_json(ret.value, it->value);
    }
    return ret;
}

static LSPWorkDoneProgressParams parseWorkDone(const rapidjson::Value &json)
{
    return parseProgress<LSPWorkDoneProgressValue>(json);
}

static std::vector<LSPSymbolInformation> parseWorkspaceSymbols(const rapidjson::Value &result)
{
    std::vector<LSPSymbolInformation> symbols;
    if (!result.IsArray()) {
        return symbols;
    }

    auto res = result.GetArray();

    symbols.reserve(res.Size());

    std::transform(res.begin(), res.end(), std::back_inserter(symbols), [](const rapidjson::Value &jv) {
        LSPSymbolInformation symInfo;
        if (!jv.IsObject()) {
            return symInfo;
        }
        auto symbol = jv.GetObject();

        auto location = parseLocation(GetJsonObjectForKey(symbol, MEMBER_LOCATION));
        if (symbol.HasMember(MEMBER_RANGE)) {
            location.range = parseRange(GetJsonObjectForKey(symbol, MEMBER_RANGE));
        }

        auto containerName = GetStringValue(symbol, "containerName");
        if (!containerName.isEmpty()) {
            containerName.append(QStringLiteral("::"));
        }
        symInfo.name = containerName + GetStringValue(symbol, "name");
        symInfo.kind = (LSPSymbolKind)GetIntValue(symbol, MEMBER_KIND);
        symInfo.range = location.range;
        symInfo.url = location.uri;
        auto scoreIt = symbol.FindMember("score");
        if (scoreIt != symbol.MemberEnd()) {
            symInfo.score = scoreIt->value.GetDouble();
        }
        symInfo.tags = (LSPSymbolTag)GetIntValue(symbol, "tags");
        return symInfo;
    });

    std::sort(symbols.begin(), symbols.end(), [](const LSPSymbolInformation &l, const LSPSymbolInformation &r) {
        return l.score > r.score;
    });

    return symbols;
}

using GenericReplyType = rapidjson::Value;
using GenericReplyHandler = ReplyHandler<GenericReplyType>;

class LSPClientServer::LSPClientServerPrivate
{
    typedef LSPClientServerPrivate self_type;

    LSPClientServer *q;
    // server cmd line
    QStringList m_server;
    // workspace root to pass along
    QUrl m_root;
    // language id
    QString m_langId;
    // user provided init
    QJsonValue m_init;
    // additional tweaks
    ExtraServerConfig m_config;
    // server process
    QProcess m_sproc;
    // server declared capabilities
    LSPServerCapabilities m_capabilities;
    // server state
    State m_state = State::None;
    // last msg id
    int m_id = 0;
    // receive buffer
    QByteArray m_receive;
    // registered reply handlers
    // (result handler, error result handler)
    QHash<int, std::pair<GenericReplyHandler, GenericReplyHandler>> m_handlers;
    // pending request responses
    static constexpr int MAX_REQUESTS = 5;
    QVariantList m_requests{MAX_REQUESTS + 1};

    // currently accumulated stderr output, used to output to the message view on line level
    QString m_currentStderrOutput;

public:
    LSPClientServerPrivate(LSPClientServer *_q,
                           const QStringList &server,
                           const QUrl &root,
                           const QString &langId,
                           const QJsonValue &init,
                           ExtraServerConfig config)
        : q(_q)
        , m_server(server)
        , m_root(root)
        , m_langId(langId)
        , m_init(init)
        , m_config(std::move(config))
    {
        // setup async reading
        QObject::connect(&m_sproc, &QProcess::readyReadStandardOutput, utils::mem_fun(&self_type::readStandardOutput, this));
        QObject::connect(&m_sproc, &QProcess::readyReadStandardError, utils::mem_fun(&self_type::readStandardError, this));
        QObject::connect(&m_sproc, &QProcess::stateChanged, utils::mem_fun(&self_type::onStateChanged, this));
    }

    ~LSPClientServerPrivate()
    {
        stop(TIMEOUT_SHUTDOWN, TIMEOUT_SHUTDOWN);
    }

    const QStringList &cmdline() const
    {
        return m_server;
    }

    const QUrl &root() const
    {
        return m_root;
    }

    const QString &langId() const
    {
        return m_langId;
    }

    State state()
    {
        return m_state;
    }

    const LSPServerCapabilities &capabilities()
    {
        return m_capabilities;
    }

    int cancel(int reqid)
    {
        if (m_handlers.remove(reqid)) {
            auto params = QJsonObject{{QLatin1String(MEMBER_ID), reqid}};
            write(init_request(QStringLiteral("$/cancelRequest"), params));
        }
        return -1;
    }

private:
    void setState(State s)
    {
        if (m_state != s) {
            m_state = s;
            Q_EMIT q->stateChanged(q);
        }
    }

    RequestHandle write(const QJsonObject &msg, const GenericReplyHandler &h = nullptr, const GenericReplyHandler &eh = nullptr, const QVariant &id = {})
    {
        RequestHandle ret;
        ret.m_server = q;

        if (!running()) {
            return ret;
        }

        auto ob = msg;
        ob.insert(QStringLiteral("jsonrpc"), QStringLiteral("2.0"));
        // notification == no handler
        if (h) {
            ob.insert(QLatin1String(MEMBER_ID), ++m_id);
            ret.m_id = m_id;
            m_handlers[m_id] = {h, eh};
        } else if (!id.isNull()) {
            ob.insert(QLatin1String(MEMBER_ID), QJsonValue::fromVariant(id));
        }

        QJsonDocument json(ob);
        auto sjson = json.toJson();

        qCInfo(LSPCLIENT) << "calling" << msg[QLatin1String(MEMBER_METHOD)].toString();
        qCDebug(LSPCLIENT) << "sending message:\n" << QString::fromUtf8(sjson);
        // some simple parsers expect length header first
        auto hdr = QStringLiteral(CONTENT_LENGTH ": %1\r\n").arg(sjson.length());
        // write is async, so no blocking wait occurs here
        m_sproc.write(hdr.toLatin1());
        m_sproc.write("\r\n");
        m_sproc.write(sjson);

        return ret;
    }

    RequestHandle send(const QJsonObject &msg, const GenericReplyHandler &h = nullptr, const GenericReplyHandler &eh = nullptr)
    {
        if (m_state == State::Running) {
            return write(msg, h, eh);
        } else {
            qCWarning(LSPCLIENT) << "send for non-running server";
        }
        return RequestHandle();
    }

    void readStandardOutput()
    {
        // accumulate in buffer
        m_receive.append(m_sproc.readAllStandardOutput());

        // try to get one (or more) message
        QByteArray &buffer = m_receive;

        while (true) {
            qCDebug(LSPCLIENT) << "buffer size" << buffer.length();
            auto header = QByteArray(CONTENT_LENGTH ":");
            int index = buffer.indexOf(header);
            if (index < 0) {
                // avoid collecting junk
                if (buffer.length() > 1 << 20) {
                    buffer.clear();
                }
                break;
            }
            index += header.length();
            int endindex = buffer.indexOf("\r\n", index);
            auto msgstart = buffer.indexOf("\r\n\r\n", index);
            if (endindex < 0 || msgstart < 0) {
                break;
            }
            msgstart += 4;
            bool ok = false;
            auto length = buffer.mid(index, endindex - index).toInt(&ok, 10);
            // FIXME perhaps detect if no reply for some time
            // then again possibly better left to user to restart in such case
            if (!ok) {
                qCWarning(LSPCLIENT) << "invalid " CONTENT_LENGTH;
                // flush and try to carry on to some next header
                buffer.remove(0, msgstart);
                continue;
            }
            // sanity check to avoid extensive buffering
            if (length > 1 << 29) {
                qCWarning(LSPCLIENT) << "excessive size";
                buffer.clear();
                continue;
            }
            if (msgstart + length > buffer.length()) {
                break;
            }
            // now onto payload
            auto payload = buffer.mid(msgstart, length);
            buffer.remove(0, msgstart + length);
            qCInfo(LSPCLIENT) << "got message payload size " << length;
            qCDebug(LSPCLIENT) << "message payload:\n" << payload;

            rapidjson::Document doc;
            doc.ParseInsitu(payload.data());
            if (doc.HasParseError()) {
                qWarning(LSPCLIENT) << "invalid response payload" << doc.GetParseError() << doc.GetErrorOffset();
                continue;
            }

            rapidjson::GenericObject result = doc.GetObject();
            auto memIdIt = result.FindMember(MEMBER_ID);
            int msgid = -1;
            if (memIdIt != result.MemberEnd()) {
                // According to the spec, the ID can be `integer | string | null`
                if (memIdIt->value.IsString()) {
                    msgid = QByteArray(memIdIt->value.GetString()).toInt();
                } else if (memIdIt->value.IsInt()) {
                    msgid = memIdIt->value.GetInt();
                }

            } else {
                processNotification(result);
                continue;
            }

            // could be request
            if (result.HasMember(MEMBER_METHOD)) {
                processRequest(result);
                continue;
            }

            // a valid reply; what to do with it now
            auto it = m_handlers.find(msgid);
            if (it != m_handlers.end()) {
                // copy handler to local storage
                const auto handler = *it;

                // remove handler from our set, do this pre handler execution to avoid races
                m_handlers.erase(it);

                // run handler, might e.g. trigger some new LSP actions for this server
                // process and provide error if caller interested,
                // otherwise reply will resolve to 'empty' response
                auto &h = handler.first;
                auto &eh = handler.second;
                if (auto it = result.FindMember(MEMBER_ERROR); it != result.MemberEnd() && eh) {
                    eh(it->value);
                } else {
                    // result can be object or array so just extract value
                    h(GetJsonValueForKey(result, MEMBER_RESULT));
                }
            } else {
                // could have been canceled
                qCDebug(LSPCLIENT) << "unexpected reply id" << msgid;
            }
        }
    }

    void readStandardError()
    {
        // append new stuff to our buffer, we assume UTF-8 output
        m_currentStderrOutput.append(QString::fromUtf8(m_sproc.readAllStandardError()));

        // now, cut out all full lines
        LSPShowMessageParams msg;
        if (const int lastNewLineIndex = m_currentStderrOutput.lastIndexOf(QLatin1Char('\n')); lastNewLineIndex >= 0) {
            msg.message = m_currentStderrOutput.left(lastNewLineIndex);
            m_currentStderrOutput.remove(0, lastNewLineIndex + 1);
        }

        // emit the output lines if non-empty
        // this might strip empty lines in error output, but that is better then a spammed output view
        if (!msg.message.isEmpty()) {
            msg.type = LSPMessageType::Log;
            Q_EMIT q->logMessage(msg);
        }
    }

    static QJsonObject init_error(const LSPErrorCode code, const QString &msg)
    {
        return QJsonObject{
            {QLatin1String(MEMBER_ERROR), QJsonObject{{QLatin1String(MEMBER_CODE), static_cast<int>(code)}, {QLatin1String(MEMBER_MESSAGE), msg}}}};
    }

    static QJsonObject init_request(const QString &method, const QJsonObject &params = QJsonObject())
    {
        return QJsonObject{{QLatin1String(MEMBER_METHOD), method}, {QLatin1String(MEMBER_PARAMS), params}};
    }

    static QJsonObject init_response(const QJsonValue &result = QJsonValue())
    {
        return QJsonObject{{QLatin1String(MEMBER_RESULT), result}};
    }

    bool running()
    {
        return m_sproc.state() == QProcess::Running;
    }

    void onStateChanged(QProcess::ProcessState nstate)
    {
        if (nstate == QProcess::NotRunning) {
            setState(State::None);
        }
    }

    void shutdown()
    {
        if (m_state == State::Running) {
            qCInfo(LSPCLIENT) << "shutting down" << m_server;
            // cancel all pending
            m_handlers.clear();
            // shutdown sequence
            send(init_request(QStringLiteral("shutdown")));
            // maybe we will get/see reply on the above, maybe not
            // but not important or useful either way
            send(init_request(QStringLiteral("exit")));
            // no longer fit for regular use
            setState(State::Shutdown);
        }
    }

    void applyTriggerOverride(QList<QChar> &characters, const TriggerCharactersOverride &adjust)
    {
        // these are expected 'small' sets, so the simple way should do
        for (const auto &c : adjust.exclude) {
            characters.removeAll(c);
        }
        characters.append(adjust.include);
    }

    void onInitializeReply(const rapidjson::Value &value)
    {
        // only parse parts that we use later on
        from_json(m_capabilities, GetJsonObjectForKey(value, "capabilities"));
        // tweak triggers as specified
        applyTriggerOverride(m_capabilities.completionProvider.triggerCharacters, m_config.completion);
        applyTriggerOverride(m_capabilities.signatureHelpProvider.triggerCharacters, m_config.signature);
        // finish init
        initialized();
    }

    void initialize()
    {
        // clang-format off
        QJsonObject codeAction{{QStringLiteral("codeActionLiteralSupport"),
                                    QJsonObject{{
                                        QStringLiteral("codeActionKind"), QJsonObject{{
                                            QStringLiteral("valueSet"), QJsonArray({
                                                QStringLiteral("quickfix"),
                                                QStringLiteral("refactor"),
                                                QStringLiteral("source")
                                            })
                                        }}
                                    }}
                              }};

        QJsonObject semanticTokens{{QStringLiteral("requests"),
                                        QJsonObject{
                                            {QStringLiteral("range"), true},
                                            {QStringLiteral("full"), QJsonObject{{QStringLiteral("delta"), true}}}
                                       }
                                  },
                                  {QStringLiteral("tokenTypes"), supportedSemanticTokenTypes()},
                                  {QStringLiteral("tokenModifiers"), QJsonArray()},
                                  {QStringLiteral("formats"), QJsonArray({QStringLiteral("relative")})},
        };
        QJsonObject capabilities{{QStringLiteral("textDocument"),
                                        QJsonObject{
                                            {QStringLiteral("documentSymbol"), QJsonObject{{QStringLiteral("hierarchicalDocumentSymbolSupport"), true}} },
                                            {QStringLiteral("publishDiagnostics"), QJsonObject{{QStringLiteral("relatedInformation"), true}}},
                                            {QStringLiteral("codeAction"), codeAction},
                                            {QStringLiteral("semanticTokens"), semanticTokens},
                                            {QStringLiteral("synchronization"), QJsonObject{{QStringLiteral("didSave"), true}}},
                                            {QStringLiteral("selectionRange"), QJsonObject{{QStringLiteral("dynamicRegistration"), false}}},
                                            {QStringLiteral("hover"), QJsonObject{
                                                {QStringLiteral("contentFormat"), QJsonArray{
                                                    QStringLiteral("markdown"),
                                                    QStringLiteral("plaintext")
                                                }}
                                            }},
                                            {QStringLiteral("completion"), QJsonObject{
                                                {QStringLiteral("completionItem"), QJsonObject{
                                                    {QStringLiteral("snippetSupport"), m_config.caps.snippetSupport},
                                                    {QStringLiteral("resolveSupport"), QJsonObject{
                                                        {QStringLiteral("properties"), QJsonArray{
                                                            QStringLiteral("additionalTextEdits"),
                                                            QStringLiteral("documentation")
                                                        }}
                                                    }}
                                                }}
                                            }},
                                            {QStringLiteral("inlayHint"), QJsonObject{
                                                {QStringLiteral("dynamicRegistration"), false}
                                            }}
                                        },
                                  },
                                  {QStringLiteral("window"),
                                        QJsonObject{
                                            {QStringLiteral("workDoneProgress"), true},
                                            {QStringLiteral("showMessage"), QJsonObject{
                                                {QStringLiteral("messageActionItem"), QJsonObject{
                                                    {QStringLiteral("additionalPropertiesSupport"), true}
                                                }}
                                            }}
                                        }
                                  }
                                };
        // only declare workspace support if folders so specified
        const auto &folders = m_config.folders;
        if (folders) {
            capabilities[QStringLiteral("workspace")] = QJsonObject{{QStringLiteral("workspaceFolders"), true}};
        }
        // NOTE a typical server does not use root all that much,
        // other than for some corner case (in) requests
        QJsonObject params{{QStringLiteral("processId"), QCoreApplication::applicationPid()},
                           {QStringLiteral("rootPath"), m_root.isValid() ? m_root.toLocalFile() : QJsonValue()},
                           {QStringLiteral("rootUri"), m_root.isValid() ? m_root.toString() : QJsonValue()},
                           {QStringLiteral("capabilities"), capabilities},
                           {QStringLiteral("initializationOptions"), m_init}};
        // only add new style workspaces init if so specified
        if (folders) {
            params[QStringLiteral("workspaceFolders")] = to_json(*folders);
        }
        //
        write(init_request(QStringLiteral("initialize"), params), utils::mem_fun(&self_type::onInitializeReply, this));
        // clang-format on
    }

    void initialized()
    {
        write(init_request(QStringLiteral("initialized")));
        setState(State::Running);
    }

public:
    bool start(bool forwardStdError)
    {
        if (m_state != State::None) {
            return true;
        }

        auto program = m_server.front();
        auto args = m_server;
        args.pop_front();
        qCInfo(LSPCLIENT) << "starting" << m_server << "with root" << m_root;

        // start LSP server in project root
        m_sproc.setWorkingDirectory(m_root.toLocalFile());

        // we handle stdout/stderr internally, important stuff via stdout
        m_sproc.setProcessChannelMode(forwardStdError ? QProcess::ForwardedErrorChannel : QProcess::SeparateChannels);
        m_sproc.setReadChannel(QProcess::QProcess::StandardOutput);
        startHostProcess(m_sproc, program, args);
        const bool result = m_sproc.waitForStarted();
        if (result) {
            setState(State::Started);
            // perform initial handshake
            initialize();
        }
        return result;
    }

    void stop(int to_term, int to_kill)
    {
        if (running()) {
            shutdown();
            if ((to_term >= 0) && !m_sproc.waitForFinished(to_term)) {
                m_sproc.terminate();
            }
            if ((to_kill >= 0) && !m_sproc.waitForFinished(to_kill)) {
                m_sproc.kill();
            }
        }
    }

    RequestHandle documentSymbols(const QUrl &document, const GenericReplyHandler &h, const GenericReplyHandler &eh)
    {
        auto params = textDocumentParams(document);
        return send(init_request(QStringLiteral("textDocument/documentSymbol"), params), h, eh);
    }

    RequestHandle documentDefinition(const QUrl &document, const LSPPosition &pos, const GenericReplyHandler &h)
    {
        auto params = textDocumentPositionParams(document, pos);
        return send(init_request(QStringLiteral("textDocument/definition"), params), h);
    }

    RequestHandle documentDeclaration(const QUrl &document, const LSPPosition &pos, const GenericReplyHandler &h)
    {
        auto params = textDocumentPositionParams(document, pos);
        return send(init_request(QStringLiteral("textDocument/declaration"), params), h);
    }

    RequestHandle documentTypeDefinition(const QUrl &document, const LSPPosition &pos, const GenericReplyHandler &h)
    {
        auto params = textDocumentPositionParams(document, pos);
        return send(init_request(QStringLiteral("textDocument/typeDefinition"), params), h);
    }

    RequestHandle documentImplementation(const QUrl &document, const LSPPosition &pos, const GenericReplyHandler &h)
    {
        auto params = textDocumentPositionParams(document, pos);
        return send(init_request(QStringLiteral("textDocument/implementation"), params), h);
    }

    RequestHandle documentHover(const QUrl &document, const LSPPosition &pos, const GenericReplyHandler &h)
    {
        auto params = textDocumentPositionParams(document, pos);
        return send(init_request(QStringLiteral("textDocument/hover"), params), h);
    }

    RequestHandle documentHighlight(const QUrl &document, const LSPPosition &pos, const GenericReplyHandler &h)
    {
        auto params = textDocumentPositionParams(document, pos);
        return send(init_request(QStringLiteral("textDocument/documentHighlight"), params), h);
    }

    RequestHandle documentReferences(const QUrl &document, const LSPPosition &pos, bool decl, const GenericReplyHandler &h)
    {
        auto params = referenceParams(document, pos, decl);
        return send(init_request(QStringLiteral("textDocument/references"), params), h);
    }

    RequestHandle documentCompletion(const QUrl &document, const LSPPosition &pos, const GenericReplyHandler &h)
    {
        auto params = textDocumentPositionParams(document, pos);
        return send(init_request(QStringLiteral("textDocument/completion"), params), h);
    }

    RequestHandle documentCompletionResolve(const LSPCompletionItem &c, const GenericReplyHandler &h)
    {
        QJsonObject params;
        auto dataDoc = QJsonDocument::fromJson(c.data);
        if (dataDoc.isObject()) {
            params[QStringLiteral("data")] = dataDoc.object();
        } else {
            params[QStringLiteral("data")] = dataDoc.array();
        }
        params[QLatin1String(MEMBER_DETAIL)] = c.detail;
        params[QStringLiteral("insertText")] = c.insertText;
        params[QStringLiteral("sortText")] = c.sortText;
        params[QStringLiteral("textEdit")] = QJsonObject{{QStringLiteral("newText"), c.textEdit.newText}, {QStringLiteral("range"), to_json(c.textEdit.range)}};
        params[QLatin1String(MEMBER_LABEL)] = c.originalLabel;
        params[QLatin1String(MEMBER_KIND)] = (int)c.kind;
        return send(init_request(QStringLiteral("completionItem/resolve"), params), h);
    }

    RequestHandle signatureHelp(const QUrl &document, const LSPPosition &pos, const GenericReplyHandler &h)
    {
        auto params = textDocumentPositionParams(document, pos);
        return send(init_request(QStringLiteral("textDocument/signatureHelp"), params), h);
    }

    RequestHandle selectionRange(const QUrl &document, const QList<LSPPosition> &positions, const GenericReplyHandler &h)
    {
        auto params = textDocumentPositionsParams(document, positions);
        return send(init_request(QStringLiteral("textDocument/selectionRange"), params), h);
    }

    RequestHandle clangdSwitchSourceHeader(const QUrl &document, const GenericReplyHandler &h)
    {
        auto params = QJsonObject{{QLatin1String(MEMBER_URI), encodeUrl(document)}};
        return send(init_request(QStringLiteral("textDocument/switchSourceHeader"), params), h);
    }

    RequestHandle clangdMemoryUsage(const GenericReplyHandler &h)
    {
        return send(init_request(QStringLiteral("$/memoryUsage"), QJsonObject()), h);
    }

    RequestHandle rustAnalyzerExpandMacro(const QUrl &document, const LSPPosition &pos, const GenericReplyHandler &h)
    {
        auto params = textDocumentPositionParams(document, pos);
        return send(init_request(QStringLiteral("rust-analyzer/expandMacro"), params), h);
    }

    RequestHandle documentFormatting(const QUrl &document, const LSPFormattingOptions &options, const GenericReplyHandler &h)
    {
        auto params = documentRangeFormattingParams(document, nullptr, options);
        return send(init_request(QStringLiteral("textDocument/formatting"), params), h);
    }

    RequestHandle documentRangeFormatting(const QUrl &document, const LSPRange &range, const LSPFormattingOptions &options, const GenericReplyHandler &h)
    {
        auto params = documentRangeFormattingParams(document, &range, options);
        return send(init_request(QStringLiteral("textDocument/rangeFormatting"), params), h);
    }

    RequestHandle
    documentOnTypeFormatting(const QUrl &document, const LSPPosition &pos, QChar lastChar, const LSPFormattingOptions &options, const GenericReplyHandler &h)
    {
        auto params = documentOnTypeFormattingParams(document, pos, lastChar, options);
        return send(init_request(QStringLiteral("textDocument/onTypeFormatting"), params), h);
    }

    RequestHandle documentRename(const QUrl &document, const LSPPosition &pos, const QString &newName, const GenericReplyHandler &h)
    {
        auto params = renameParams(document, pos, newName);
        return send(init_request(QStringLiteral("textDocument/rename"), params), h);
    }

    RequestHandle documentCodeAction(const QUrl &document,
                                     const LSPRange &range,
                                     const QList<QString> &kinds,
                                     const QList<LSPDiagnostic> &diagnostics,
                                     const GenericReplyHandler &h)
    {
        auto params = codeActionParams(document, range, kinds, diagnostics);
        return send(init_request(QStringLiteral("textDocument/codeAction"), params), h);
    }

    RequestHandle documentSemanticTokensFull(const QUrl &document, bool delta, const QString &requestId, const LSPRange &range, const GenericReplyHandler &h)
    {
        auto params = textDocumentParams(document);
        // Delta
        if (delta && !requestId.isEmpty()) {
            params[QLatin1String(MEMBER_PREVIOUS_RESULT_ID)] = requestId;
            return send(init_request(QStringLiteral("textDocument/semanticTokens/full/delta"), params), h);
        }
        // Range
        if (range.isValid()) {
            params[QLatin1String(MEMBER_RANGE)] = to_json(range);
            return send(init_request(QStringLiteral("textDocument/semanticTokens/range"), params), h);
        }

        return send(init_request(QStringLiteral("textDocument/semanticTokens/full"), params), h);
    }

    RequestHandle documentInlayHint(const QUrl &document, const LSPRange &range, const GenericReplyHandler &h)
    {
        auto params = textDocumentParams(document);
        params[QLatin1String(MEMBER_RANGE)] = to_json(range);
        return send(init_request(QStringLiteral("textDocument/inlayHint"), params), h);
    }

    void executeCommand(const LSPCommand &command)
    {
        auto params = executeCommandParams(command);
        // Pass an empty lambda as reply handler because executeCommand is a Request, but we ignore the result
        send(init_request(QStringLiteral("workspace/executeCommand"), params), [](const auto &) { });
    }

    void didOpen(const QUrl &document, int version, const QString &langId, const QString &text)
    {
        auto params = textDocumentParams(textDocumentItem(document, langId, text, version));
        send(init_request(QStringLiteral("textDocument/didOpen"), params));
    }

    void didChange(const QUrl &document, int version, const QString &text, const QList<LSPTextDocumentContentChangeEvent> &changes)
    {
        Q_ASSERT(text.isEmpty() || changes.empty());
        auto params = textDocumentParams(document, version);
        params[QStringLiteral("contentChanges")] = text.size() ? QJsonArray{QJsonObject{{QLatin1String(MEMBER_TEXT), text}}} : to_json(changes);
        send(init_request(QStringLiteral("textDocument/didChange"), params));
    }

    void didSave(const QUrl &document, const QString &text)
    {
        auto params = textDocumentParams(document);
        if (!text.isNull()) {
            params[QStringLiteral("text")] = text;
        }
        send(init_request(QStringLiteral("textDocument/didSave"), params));
    }

    void didClose(const QUrl &document)
    {
        auto params = textDocumentParams(document);
        send(init_request(QStringLiteral("textDocument/didClose"), params));
    }

    void didChangeConfiguration(const QJsonValue &settings)
    {
        auto params = changeConfigurationParams(settings);
        send(init_request(QStringLiteral("workspace/didChangeConfiguration"), params));
    }

    void didChangeWorkspaceFolders(const QList<LSPWorkspaceFolder> &added, const QList<LSPWorkspaceFolder> &removed)
    {
        auto params = changeWorkspaceFoldersParams(added, removed);
        send(init_request(QStringLiteral("workspace/didChangeWorkspaceFolders"), params));
    }

    void workspaceSymbol(const QString &symbol, const GenericReplyHandler &h)
    {
        auto params = QJsonObject{{QLatin1String(MEMBER_QUERY), symbol}};
        send(init_request(QStringLiteral("workspace/symbol"), params), h);
    }

    void processNotification(const rapidjson::Value &msg)
    {
        auto methodId = msg.FindMember(MEMBER_METHOD);
        if (methodId == msg.MemberEnd()) {
            return;
        }
        auto methodParamsIt = msg.FindMember(MEMBER_PARAMS);
        if (methodParamsIt == msg.MemberEnd()) {
            qWarning() << "Ignore because no 'params' member in notification" << QByteArray(methodId->value.GetString());
            return;
        }

        auto methodString = methodId->value.GetString();
        auto methodLen = methodId->value.GetStringLength();
        std::string_view method(methodString, methodLen);

        const bool isObj = methodParamsIt->value.IsObject();
        auto &obj = methodParamsIt->value;
        if (isObj && method == "textDocument/publishDiagnostics") {
            Q_EMIT q->publishDiagnostics(parseDiagnostics(obj));
        } else if (isObj && method == "window/showMessage") {
            Q_EMIT q->showMessage(parseMessage(obj));
        } else if (isObj && method == "window/logMessage") {
            Q_EMIT q->logMessage(parseMessage(obj));
        } else if (isObj && method == "$/progress") {
            Q_EMIT q->workDoneProgress(parseWorkDone(obj));
        } else {
            qCWarning(LSPCLIENT) << "discarding notification" << method.data() << ", params is object:" << isObj;
        }
    }

    ReplyHandler<QJsonValue> prepareResponse(const QVariant &msgid)
    {
        // allow limited number of outstanding requests
        auto ctx = QPointer<LSPClientServer>(q);
        m_requests.push_back(msgid);
        if (m_requests.size() > MAX_REQUESTS) {
            m_requests.pop_front();
        }

        auto h = [ctx, this, msgid](const QJsonValue &response) {
            if (!ctx) {
                return;
            }
            auto index = m_requests.indexOf(msgid);
            if (index >= 0) {
                m_requests.remove(index);
                write(init_response(response), nullptr, nullptr, msgid);
            } else {
                qCWarning(LSPCLIENT) << "discarding response" << msgid;
            }
        };
        return h;
    }

    template<typename ReplyType>
    static ReplyHandler<ReplyType> responseHandler(const ReplyHandler<QJsonValue> &h,
                                                   typename utils::identity<std::function<QJsonValue(const ReplyType &)>>::type c)
    {
        return [h, c](const ReplyType &m) {
            h(c(m));
        };
    }

    // pretty rare and limited use, but anyway
    void processRequest(const rapidjson::Value &msg)
    {
        auto method = GetStringValue(msg, MEMBER_METHOD);

        // could be number or string, let's retain as-is
        QVariant msgId;
        if (msg[MEMBER_ID].IsString()) {
            msgId = GetStringValue(msg, MEMBER_ID);
        } else {
            msgId = GetIntValue(msg, MEMBER_ID, -1);
        }

        const auto &params = GetJsonObjectForKey(msg, MEMBER_PARAMS);
        bool handled = false;
        if (method == QLatin1String("workspace/applyEdit")) {
            auto h = responseHandler<LSPApplyWorkspaceEditResponse>(prepareResponse(msgId), applyWorkspaceEditResponse);
            Q_EMIT q->applyEdit(parseApplyWorkspaceEditParams(params), h, handled);
        } else if (method == QLatin1String("workspace/workspaceFolders")) {
            // helper to convert from array to value
            auto workspaceFolders = [](const QList<LSPWorkspaceFolder> &p) -> QJsonValue {
                return to_json(p);
            };
            auto h = responseHandler<QList<LSPWorkspaceFolder>>(prepareResponse(msgId), workspaceFolders);
            Q_EMIT q->workspaceFolders(h, handled);
        } else if (method == QLatin1String("window/workDoneProgress/create") || method == QLatin1String("client/registerCapability")) {
            // void reply to accept
            // that should trigger subsequent progress notifications
            // for now; also no need to extract supplied token
            auto h = prepareResponse(msgId);
            h(QJsonValue());
        } else if (method == QLatin1String("workspace/semanticTokens/refresh")) {
            // void reply to accept, we don't handle this at the moment, but some servers send it and require some valid reply
            // e.g. typst-lsp, see https://invent.kde.org/utilities/kate/-/issues/108
            auto h = prepareResponse(msgId);
            h(QJsonValue());
        } else if (method == QLatin1String("window/showMessageRequest")) {
            auto actions = GetJsonArrayForKey(params, MEMBER_ACTIONS).GetArray();
            QList<LSPMessageRequestAction> v;
            auto responder = prepareResponse(msgId);
            for (const auto &action : actions) {
                QString title = GetStringValue(action, MEMBER_TITLE);
                QJsonObject actionToSubmit = QJsonDocument::fromJson(rapidJsonStringify(action)).object();
                v.append(LSPMessageRequestAction{.title = title, .choose = [=]() {
                                                     responder(actionToSubmit);
                                                 }});
            }
            auto nullResponse = [responder]() {
                responder(QJsonObject());
            };
            Q_EMIT q->showMessageRequest(parseMessage(params), v, nullResponse, handled);
        } else {
            write(init_error(LSPErrorCode::MethodNotFound, method), nullptr, nullptr, msgId);
            qCWarning(LSPCLIENT) << "discarding request" << method;
        }
    }
};

// generic convert handler
// sprinkle some connection-like context safety
// not so likely relevant/needed due to typical sequence of events,
// but in case the latter would be changed in surprising ways ...
template<typename ReplyType>
static GenericReplyHandler
make_handler(const ReplyHandler<ReplyType> &h, const QObject *context, typename utils::identity<std::function<ReplyType(const GenericReplyType &)>>::type c)
{
    // empty provided handler leads to empty handler
    if (!h || !c) {
        return nullptr;
    }

    QPointer<const QObject> ctx(context);
    return [ctx, h, c](const GenericReplyType &m) {
        if (ctx) {
            h(c(m));
        }
    };
}

LSPClientServer::LSPClientServer(const QStringList &server, const QUrl &root, const QString &langId, const QJsonValue &init, ExtraServerConfig config)
    : d(new LSPClientServerPrivate(this, server, root, langId, init, std::move(config)))
{
}

LSPClientServer::~LSPClientServer()
{
    delete d;
}

const QStringList &LSPClientServer::cmdline() const
{
    return d->cmdline();
}

const QUrl &LSPClientServer::root() const
{
    return d->root();
}

const QString &LSPClientServer::langId() const
{
    return d->langId();
}

LSPClientServer::State LSPClientServer::state() const
{
    return d->state();
}

const LSPServerCapabilities &LSPClientServer::capabilities() const
{
    return d->capabilities();
}

bool LSPClientServer::start(bool forwardStdError)
{
    return d->start(forwardStdError);
}

void LSPClientServer::stop(int to_t, int to_k)
{
    d->stop(to_t, to_k);
}

int LSPClientServer::cancel(int reqid)
{
    return d->cancel(reqid);
}

LSPClientServer::RequestHandle
LSPClientServer::documentSymbols(const QUrl &document, const QObject *context, const DocumentSymbolsReplyHandler &h, const ErrorReplyHandler &eh)
{
    return d->documentSymbols(document, make_handler(h, context, parseDocumentSymbols), make_handler(eh, context, parseResponseError));
}

LSPClientServer::RequestHandle
LSPClientServer::documentDefinition(const QUrl &document, const LSPPosition &pos, const QObject *context, const DocumentDefinitionReplyHandler &h)
{
    return d->documentDefinition(document, pos, make_handler(h, context, parseDocumentLocation));
}

LSPClientServer::RequestHandle
LSPClientServer::documentImplementation(const QUrl &document, const LSPPosition &pos, const QObject *context, const DocumentDefinitionReplyHandler &h)
{
    return d->documentImplementation(document, pos, make_handler(h, context, parseDocumentLocation));
}

LSPClientServer::RequestHandle
LSPClientServer::documentDeclaration(const QUrl &document, const LSPPosition &pos, const QObject *context, const DocumentDefinitionReplyHandler &h)
{
    return d->documentDeclaration(document, pos, make_handler(h, context, parseDocumentLocation));
}

LSPClientServer::RequestHandle
LSPClientServer::documentTypeDefinition(const QUrl &document, const LSPPosition &pos, const QObject *context, const DocumentDefinitionReplyHandler &h)
{
    return d->documentTypeDefinition(document, pos, make_handler(h, context, parseDocumentLocation));
}

LSPClientServer::RequestHandle
LSPClientServer::documentHover(const QUrl &document, const LSPPosition &pos, const QObject *context, const DocumentHoverReplyHandler &h)
{
    return d->documentHover(document, pos, make_handler(h, context, parseHover));
}

LSPClientServer::RequestHandle
LSPClientServer::documentHighlight(const QUrl &document, const LSPPosition &pos, const QObject *context, const DocumentHighlightReplyHandler &h)
{
    return d->documentHighlight(document, pos, make_handler(h, context, parseDocumentHighlightList));
}

LSPClientServer::RequestHandle
LSPClientServer::documentReferences(const QUrl &document, const LSPPosition &pos, bool decl, const QObject *context, const DocumentDefinitionReplyHandler &h)
{
    return d->documentReferences(document, pos, decl, make_handler(h, context, parseDocumentLocation));
}

LSPClientServer::RequestHandle
LSPClientServer::documentCompletion(const QUrl &document, const LSPPosition &pos, const QObject *context, const DocumentCompletionReplyHandler &h)
{
    return d->documentCompletion(document, pos, make_handler(h, context, parseDocumentCompletion));
}

LSPClientServer::RequestHandle
LSPClientServer::documentCompletionResolve(const LSPCompletionItem &c, const QObject *context, const DocumentCompletionResolveReplyHandler &h)
{
    return d->documentCompletionResolve(c, make_handler(h, context, parseDocumentCompletionResolve));
}

LSPClientServer::RequestHandle
LSPClientServer::signatureHelp(const QUrl &document, const LSPPosition &pos, const QObject *context, const SignatureHelpReplyHandler &h)
{
    return d->signatureHelp(document, pos, make_handler(h, context, parseSignatureHelp));
}

LSPClientServer::RequestHandle
LSPClientServer::selectionRange(const QUrl &document, const QList<LSPPosition> &positions, const QObject *context, const SelectionRangeReplyHandler &h)
{
    return d->selectionRange(document, positions, make_handler(h, context, parseSelectionRanges));
}

LSPClientServer::RequestHandle LSPClientServer::clangdSwitchSourceHeader(const QUrl &document, const QObject *context, const SwitchSourceHeaderHandler &h)
{
    return d->clangdSwitchSourceHeader(document, make_handler(h, context, parseClangdSwitchSourceHeader));
}

LSPClientServer::RequestHandle LSPClientServer::clangdMemoryUsage(const QObject *context, const MemoryUsageHandler &h)
{
    auto identity = [](const rapidjson::Value &p) -> QString {
        rapidjson::StringBuffer buf;
        rapidjson::PrettyWriter w(buf);
        p.Accept(w);
        return QString::fromUtf8(buf.GetString(), buf.GetSize());
    };
    return d->clangdMemoryUsage(make_handler(h, context, identity));
}

LSPClientServer::RequestHandle
LSPClientServer::rustAnalyzerExpandMacro(const QObject *context, const QUrl &document, const LSPPosition &pos, const ExpandMacroHandler &h)
{
    return d->rustAnalyzerExpandMacro(document, pos, make_handler(h, context, parseExpandedMacro));
}

LSPClientServer::RequestHandle
LSPClientServer::documentFormatting(const QUrl &document, const LSPFormattingOptions &options, const QObject *context, const FormattingReplyHandler &h)
{
    return d->documentFormatting(document, options, make_handler(h, context, parseTextEdit));
}

LSPClientServer::RequestHandle LSPClientServer::documentRangeFormatting(const QUrl &document,
                                                                        const LSPRange &range,
                                                                        const LSPFormattingOptions &options,
                                                                        const QObject *context,
                                                                        const FormattingReplyHandler &h)
{
    return d->documentRangeFormatting(document, range, options, make_handler(h, context, parseTextEdit));
}

LSPClientServer::RequestHandle LSPClientServer::documentOnTypeFormatting(const QUrl &document,
                                                                         const LSPPosition &pos,
                                                                         const QChar lastChar,
                                                                         const LSPFormattingOptions &options,
                                                                         const QObject *context,
                                                                         const FormattingReplyHandler &h)
{
    return d->documentOnTypeFormatting(document, pos, lastChar, options, make_handler(h, context, parseTextEdit));
}

LSPClientServer::RequestHandle LSPClientServer::documentRename(const QUrl &document,
                                                               const LSPPosition &pos,
                                                               const QString &newName,
                                                               const QObject *context,
                                                               const WorkspaceEditReplyHandler &h)
{
    return d->documentRename(document, pos, newName, make_handler(h, context, parseWorkSpaceEdit));
}

LSPClientServer::RequestHandle LSPClientServer::documentCodeAction(const QUrl &document,
                                                                   const LSPRange &range,
                                                                   const QList<QString> &kinds,
                                                                   QList<LSPDiagnostic> diagnostics,
                                                                   const QObject *context,
                                                                   const CodeActionReplyHandler &h)
{
    return d->documentCodeAction(document, range, kinds, std::move(diagnostics), make_handler(h, context, parseCodeAction));
}

LSPClientServer::RequestHandle
LSPClientServer::documentSemanticTokensFull(const QUrl &document, const QString &requestId, const QObject *context, const SemanticTokensDeltaReplyHandler &h)
{
    auto invalidRange = KTextEditor::Range::invalid();
    return d->documentSemanticTokensFull(document, /* delta = */ false, requestId, invalidRange, make_handler(h, context, parseSemanticTokensDelta));
}

LSPClientServer::RequestHandle LSPClientServer::documentSemanticTokensFullDelta(const QUrl &document,
                                                                                const QString &requestId,
                                                                                const QObject *context,
                                                                                const SemanticTokensDeltaReplyHandler &h)
{
    auto invalidRange = KTextEditor::Range::invalid();
    return d->documentSemanticTokensFull(document, /* delta = */ true, requestId, invalidRange, make_handler(h, context, parseSemanticTokensDelta));
}

LSPClientServer::RequestHandle
LSPClientServer::documentSemanticTokensRange(const QUrl &document, const LSPRange &range, const QObject *context, const SemanticTokensDeltaReplyHandler &h)
{
    return d->documentSemanticTokensFull(document, /* delta = */ false, QString(), range, make_handler(h, context, parseSemanticTokensDelta));
}

LSPClientServer::RequestHandle
LSPClientServer::documentInlayHint(const QUrl &document, const LSPRange &range, const QObject *context, const InlayHintsReplyHandler &h)
{
    return d->documentInlayHint(document, range, make_handler(h, context, parseInlayHints));
}

void LSPClientServer::executeCommand(const LSPCommand &command)
{
    d->executeCommand(command);
}

void LSPClientServer::didOpen(const QUrl &document, int version, const QString &langId, const QString &text)
{
    d->didOpen(document, version, langId, text);
}

void LSPClientServer::didChange(const QUrl &document, int version, const QString &text, const QList<LSPTextDocumentContentChangeEvent> &changes)
{
    d->didChange(document, version, text, changes);
}

void LSPClientServer::didSave(const QUrl &document, const QString &text)
{
    d->didSave(document, text);
}

void LSPClientServer::didClose(const QUrl &document)
{
    d->didClose(document);
}

void LSPClientServer::didChangeConfiguration(const QJsonValue &settings)
{
    d->didChangeConfiguration(settings);
}

void LSPClientServer::didChangeWorkspaceFolders(const QList<LSPWorkspaceFolder> &added, const QList<LSPWorkspaceFolder> &removed)
{
    d->didChangeWorkspaceFolders(added, removed);
}

void LSPClientServer::workspaceSymbol(const QString &symbol, const QObject *context, const WorkspaceSymbolsReplyHandler &h)
{
    d->workspaceSymbol(symbol, make_handler(h, context, parseWorkspaceSymbols));
}

#include "moc_lspclientserver.cpp"
