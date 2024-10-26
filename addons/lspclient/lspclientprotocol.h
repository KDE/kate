/*
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>

    SPDX-License-Identifier: MIT
*/

#pragma once

#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QUrl>

#include "lspsemantichighlighting.h"
#include "semantic_tokens_legend.h"
#include <diagnostics/diagnostic_types.h>

#include <KTextEditor/Cursor>
#include <KTextEditor/Range>

#include <memory>
#include <optional>

// Following types roughly follow the types/interfaces as defined in LSP protocol spec
// although some deviation may arise where it has been deemed useful
// Moreover, to avoid introducing a custom 'optional' type, absence of an optional
// part/member is usually signalled by some 'invalid' marker (empty, negative).

enum class LSPErrorCode {
    // Defined by JSON RPC
    ParseError = -32700,
    InvalidRequest = -32600,
    MethodNotFound = -32601,
    InvalidParams = -32602,
    InternalError = -32603,
    serverErrorStart = -32099,
    serverErrorEnd = -32000,
    ServerNotInitialized = -32002,
    UnknownErrorCode = -32001,

    // Defined by the protocol.
    RequestCancelled = -32800,
    ContentModified = -32801
};

struct LSPResponseError {
    LSPErrorCode code{};
    QString message;
    QByteArray data;
};

enum class LSPDocumentSyncKind {
    None = 0,
    Full = 1,
    Incremental = 2
};

struct LSPSaveOptions {
    bool includeText = false;
};

// only used parts for now
struct LSPTextDocumentSyncOptions {
    LSPDocumentSyncKind change = LSPDocumentSyncKind::None;
    std::optional<LSPSaveOptions> save;
};

struct LSPCompletionOptions {
    bool provider = false;
    bool resolveProvider = false;
    QList<QChar> triggerCharacters;
};

struct LSPSignatureHelpOptions {
    bool provider = false;
    QList<QChar> triggerCharacters;
};

// ensure distinct type
struct LSPDocumentOnTypeFormattingOptions : public LSPSignatureHelpOptions {
};

// Ref: https://microsoft.github.io/language-server-protocol/specification#textDocument_semanticTokens
struct LSPSemanticTokensOptions {
    bool full = false;
    bool fullDelta = false;
    bool range = false;
    SemanticTokensLegend legend;
    //     QList<QString> types;
};

struct LSPWorkspaceFoldersServerCapabilities {
    bool supported = false;
    bool changeNotifications = false;
};

struct LSPServerCapabilities {
    LSPTextDocumentSyncOptions textDocumentSync;
    bool hoverProvider = false;
    LSPCompletionOptions completionProvider;
    LSPSignatureHelpOptions signatureHelpProvider;
    bool definitionProvider = false;
    // official extension as of 3.14.0
    bool declarationProvider = false;
    bool typeDefinitionProvider = false;
    bool referencesProvider = false;
    bool implementationProvider = false;
    bool documentSymbolProvider = false;
    bool documentHighlightProvider = false;
    bool documentFormattingProvider = false;
    bool documentRangeFormattingProvider = false;
    LSPDocumentOnTypeFormattingOptions documentOnTypeFormattingProvider;
    bool renameProvider = false;
    // CodeActionOptions not useful/considered at present
    bool codeActionProvider = false;
    LSPSemanticTokensOptions semanticTokenProvider;
    // workspace caps flattened
    // (other parts not useful/considered at present)
    LSPWorkspaceFoldersServerCapabilities workspaceFolders;
    bool selectionRangeProvider = false;
    bool inlayHintProvider = false;
};

enum class LSPMarkupKind {
    None = 0,
    PlainText = 1,
    MarkDown = 2
};

struct LSPMarkupContent {
    LSPMarkupKind kind = LSPMarkupKind::None;
    QString value;
};

/**
 * Language Server Protocol Position
 * line + column, 0 based, negative for invalid
 * maps 1:1 to KTextEditor::Cursor
 */
using LSPPosition = KTextEditor::Cursor;

/**
 * Language Server Protocol Range
 * start + end tuple of LSPPosition
 * maps 1:1 to KTextEditor::Range
 */
using LSPRange = KTextEditor::Range;

using LSPLocation = SourceLocation;
// struct LSPLocation {
//     QUrl uri;
//     LSPRange range;
// };

struct LSPTextDocumentContentChangeEvent {
    LSPRange range;
    QString text;
};

enum class LSPDocumentHighlightKind {
    Text = 1,
    Read = 2,
    Write = 3
};

struct LSPDocumentHighlight {
    LSPRange range;
    LSPDocumentHighlightKind kind;
};

struct LSPHover {
    // vector for contents to support all three variants:
    // MarkedString | MarkedString[] | MarkupContent
    // vector variant is still in use e.g. by Rust rls
    QList<LSPMarkupContent> contents;
    LSPRange range;
};

enum class LSPSymbolKind {
    File = 1,
    Module = 2,
    Namespace = 3,
    Package = 4,
    Class = 5,
    Method = 6,
    Property = 7,
    Field = 8,
    Constructor = 9,
    Enum = 10,
    Interface = 11,
    Function = 12,
    Variable = 13,
    Constant = 14,
    String = 15,
    Number = 16,
    Boolean = 17,
    Array = 18,
    Object = 19,
    Key = 20,
    Null = 21,
    EnumMember = 22,
    Struct = 23,
    Event = 24,
    Operator = 25,
    TypeParameter = 26,
};

enum class LSPSymbolTag : uint8_t {
    Deprecated = 1,
};

struct LSPSymbolInformation {
    LSPSymbolInformation() = default;
    LSPSymbolInformation(const QString &_name, LSPSymbolKind _kind, LSPRange _range, const QString &_detail)
        : name(_name)
        , detail(_detail)
        , kind(_kind)
        , range(_range)
    {
    }
    QString name;
    QString detail;
    LSPSymbolKind kind;
    QUrl url;
    LSPRange range;
    double score = 0.0;
    LSPSymbolTag tags;
    std::list<LSPSymbolInformation> children;
};

struct LSPTextEdit {
    LSPRange range;
    QString newText;
};

struct LSPSelectionRange {
    LSPRange range;
    std::shared_ptr<LSPSelectionRange> parent;
};

enum class LSPCompletionItemKind {
    Text = 1,
    Method = 2,
    Function = 3,
    Constructor = 4,
    Field = 5,
    Variable = 6,
    Class = 7,
    Interface = 8,
    Module = 9,
    Property = 10,
    Unit = 11,
    Value = 12,
    Enum = 13,
    Keyword = 14,
    Snippet = 15,
    Color = 16,
    File = 17,
    Reference = 18,
    Folder = 19,
    EnumMember = 20,
    Constant = 21,
    Struct = 22,
    Event = 23,
    Operator = 24,
    TypeParameter = 25,
};

struct LSPCompletionItem {
    QString label;
    QString originalLabel; // needed for completionItem/resolve
    LSPCompletionItemKind kind = LSPCompletionItemKind::Text;
    QString detail;
    LSPMarkupContent documentation;
    QString sortText;
    QString insertText;
    QList<LSPTextEdit> additionalTextEdits;
    // textEdit is unused because doesn't work well
    // with KTE. See: https://invent.kde.org/utilities/kate/-/merge_requests/438
    // We still read it so that it can be used with completionItem/resolve
    LSPTextEdit textEdit;
    QByteArray data;
};

struct LSPParameterInformation {
    // offsets into overall signature label
    // (-1 if invalid)
    int start;
    int end;
};

struct LSPSignatureInformation {
    QString label;
    LSPMarkupContent documentation;
    QList<LSPParameterInformation> parameters;
};

struct LSPSignatureHelp {
    QList<LSPSignatureInformation> signatures;
    int activeSignature;
    int activeParameter;
};

struct LSPFormattingOptions {
    int tabSize;
    bool insertSpaces;
    // additional fields
    QJsonObject extra;
};

using LSPDiagnosticSeverity = DiagnosticSeverity;

using LSPDiagnosticRelatedInformation = DiagnosticRelatedInformation;

using LSPDiagnostic = Diagnostic;

using LSPPublishDiagnosticsParams = FileDiagnostics;

enum class LSPMessageType {
    Error = 1,
    Warning = 2,
    Info = 3,
    Log = 4
};

struct LSPShowMessageParams {
    LSPMessageType type;
    QString message;
};

using LSPLogMessageParams = LSPShowMessageParams;

enum class LSPWorkDoneProgressKind {
    Begin,
    Report,
    End
};

// combines following similar interfaces
// WorkDoneProgressBegin
// WorkDoneProgressReport
// WorkDoneProgressEnd
struct LSPWorkDoneProgressValue {
    LSPWorkDoneProgressKind kind;
    QString title;
    QString message;
    bool cancellable;
    std::optional<unsigned> percentage;
};

template<typename T>
struct LSPProgressParams {
    // number or string
    QJsonValue token;
    T value;
};

// alias convenience
using LSPWorkDoneProgressParams = LSPProgressParams<LSPWorkDoneProgressValue>;

struct LSPSemanticHighlightingToken {
    quint32 character = 0;
    quint16 length = 0;
    quint16 scope = 0;
};
Q_DECLARE_TYPEINFO(LSPSemanticHighlightingToken, Q_MOVABLE_TYPE);

struct LSPSemanticHighlightingInformation {
    int line = -1;
    QList<LSPSemanticHighlightingToken> tokens;
};

struct LSPVersionedTextDocumentIdentifier {
    QUrl uri;
    int version = -1;
};

struct LSPSemanticHighlightingParams {
    LSPVersionedTextDocumentIdentifier textDocument;
    QList<LSPSemanticHighlightingInformation> lines;
};

struct LSPCommand {
    QString title;
    QString command;
    // pretty opaque
    QByteArray arguments;
};

struct LSPTextDocumentEdit {
    LSPVersionedTextDocumentIdentifier textDocument;
    QList<LSPTextEdit> edits;
};

struct LSPWorkspaceEdit {
    // supported part for now
    QHash<QUrl, QList<LSPTextEdit>> changes;
    QList<LSPTextDocumentEdit> documentChanges;
};

struct LSPCodeAction {
    QString title;
    QString kind;
    QList<LSPDiagnostic> diagnostics;
    LSPWorkspaceEdit edit;
    LSPCommand command;
};

struct LSPApplyWorkspaceEditParams {
    QString label;
    LSPWorkspaceEdit edit;
};

struct LSPApplyWorkspaceEditResponse {
    bool applied;
    QString failureReason;
};

struct LSPWorkspaceFolder {
    QUrl uri;
    QString name;
};

struct LSPSemanticTokensEdit {
    uint32_t start = 0;
    uint32_t deleteCount = 0;
    std::vector<uint32_t> data;
};

struct LSPSemanticTokensDelta {
    QString resultId;
    std::vector<LSPSemanticTokensEdit> edits;
    std::vector<uint32_t> data;
};

struct LSPExpandedMacro {
    QString name;
    QString expansion;
};

struct LSPInlayHint {
    LSPPosition position;
    QString label;
    bool paddingLeft = false;
    bool paddingRight = false;
    // unused fields atm, not sure if we will need them
    // enum Kind { Type = 1, Parameter = 2 } kind;
    // QString tooltip;

    // kate specific
    int width = 0; ///> Used to cache width
};

struct LSPMessageRequestAction {
    QString title;
    std::function<void()> choose;
};
