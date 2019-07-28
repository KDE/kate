/*  SPDX-License-Identifier: MIT

    Copyright (C) 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>

    Permission is hereby granted, free of charge, to any person obtaining
    a copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to use, copy, modify, merge, publish,
    distribute, sublicense, and/or sell copies of the Software, and to
    permit persons to whom the Software is furnished to do so, subject to
    the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef LSPCLIENTPROTOCOL_H
#define LSPCLIENTPROTOCOL_H

#include <QString>
#include <QUrl>
#include <QList>
#include <QVector>
#include <QHash>
#include <QJsonArray>

#include <KTextEditor/Cursor>
#include <KTextEditor/Range>

// Following types roughly follow the types/interfaces as defined in LSP protocol spec
// although some deviation may arise where it has been deemed useful
// Moreover, to avoid introducing a custom 'optional' type, absence of an optional
// part/member is usually signalled by some 'invalid' marker (empty, negative).

enum class LSPErrorCode
{
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

enum class LSPDocumentSyncKind
{
    None = 0,
    Full = 1,
    Incremental = 2
};

struct LSPCompletionOptions
{
    bool provider = false;
    bool resolveProvider = false;
    QVector<QChar> triggerCharacters;
};

struct LSPSignatureHelpOptions
{
    bool provider = false;
    QVector<QChar> triggerCharacters;
};

struct LSPServerCapabilities
{
    LSPDocumentSyncKind textDocumentSync = LSPDocumentSyncKind::None;
    bool hoverProvider = false;
    LSPCompletionOptions completionProvider;
    LSPSignatureHelpOptions signatureHelpProvider;
    bool definitionProvider = false;
    // FIXME ? clangd unofficial extension
    bool declarationProvider = false;
    bool referencesProvider = false;
    bool documentSymbolProvider = false;
    bool documentHighlightProvider = false;
    bool documentFormattingProvider = false;
    bool documentRangeFormattingProvider = false;
    bool renameProvider = false;
    // CodeActionOptions not useful/considered at present
    bool codeActionProvider = false;
};

enum class LSPMarkupKind
{
    None = 0,
    PlainText = 1,
    MarkDown = 2
};

struct LSPMarkupContent
{
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

struct LSPLocation
{
    QUrl uri;
    LSPRange range;
};

enum class LSPDocumentHighlightKind
{
    Text = 1,
    Read = 2,
    Write = 3
};

struct LSPDocumentHighlight
{
    LSPRange range;
    LSPDocumentHighlightKind kind;
};

struct LSPHover
{
    LSPMarkupContent contents;
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
};

struct LSPSymbolInformation
{
    LSPSymbolInformation(const QString & _name, LSPSymbolKind _kind,
                         LSPRange _range, const QString & _detail)
        : name(_name), detail(_detail), kind(_kind), range(_range)
    {}
    QString name;
    QString detail;
    LSPSymbolKind kind;
    LSPRange range;
    QList<LSPSymbolInformation> children;
};

enum class LSPCompletionItemKind
{
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

struct LSPCompletionItem
{
    QString label;
    LSPCompletionItemKind kind;
    QString detail;
    LSPMarkupContent documentation;
    QString sortText;
    QString insertText;
};

struct LSPParameterInformation
{
    // offsets into overall signature label
    // (-1 if invalid)
    int start;
    int end;
};

struct LSPSignatureInformation
{
    QString label;
    LSPMarkupContent documentation;
    QList<LSPParameterInformation> parameters;
};

struct LSPSignatureHelp
{
    QList<LSPSignatureInformation> signatures;
    int activeSignature;
    int activeParameter;
};

struct LSPTextEdit
{
    LSPRange range;
    QString newText;
};

enum class LSPDiagnosticSeverity
{
    Unknown = 0,
    Error = 1,
    Warning = 2,
    Information = 3,
    Hint = 4,
};

struct LSPDiagnosticRelatedInformation
{
    // empty url / invalid range when absent
    LSPLocation location;
    QString message;
};

struct LSPDiagnostic
{
    LSPRange range;
    LSPDiagnosticSeverity severity;
    QString code;
    QString source;
    QString message;
    LSPDiagnosticRelatedInformation relatedInformation;
};

struct LSPPublishDiagnosticsParams
{
    QUrl uri;
    QList<LSPDiagnostic> diagnostics;
};

struct LSPCommand
{
    QString title;
    QString command;
    // pretty opaque
    QJsonArray arguments;
};

struct LSPWorkspaceEdit
{
    // supported part for now
    QHash<QUrl, QList<LSPTextEdit>> changes;
};

struct LSPCodeAction
{
    QString title;
    QString kind;
    QList<LSPDiagnostic> diagnostics;
    LSPWorkspaceEdit edit;
    LSPCommand command;
};

struct LSPApplyWorkspaceEditParams
{
    QString label;
    LSPWorkspaceEdit edit;
};

struct LSPApplyWorkspaceEditResponse
{
    bool applied;
    QString failureReason;
};

#endif
