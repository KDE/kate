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

#ifndef LSPCLIENTSERVER_H
#define LSPCLIENTSERVER_H

#include "lspclientprotocol.h"

#include <QObject>
#include <QString>
#include <QUrl>
#include <QList>
#include <QVector>
#include <QPointer>
#include <QJsonValue>

#include <functional>

namespace utils
{

// template helper
// function bind helpers
template<typename R, typename T, typename Tp, typename... Args>
inline std::function<R(Args ...)>
mem_fun(R (T::*pm)(Args ...), Tp object)
{
  return [object, pm](Args... args) {
    return (object->*pm)(std::forward<Args>(args)...);
  };
}

template<typename R, typename T, typename Tp, typename... Args>
inline std::function<R(Args...)>
mem_fun(R (T::*pm)(Args ...) const, Tp object)
{
  return [object, pm](Args... args) {
    return (object->*pm)(std::forward<Args>(args)...);
  };
}

// prevent argument deduction
template<typename T> struct identity { typedef T type; };

} // namespace utils


static const int TIMEOUT_SHUTDOWN = 200;

template<typename T>
using ReplyHandler = std::function<void(const T &)>;

using DocumentSymbolsReplyHandler = ReplyHandler<QList<LSPSymbolInformation>>;
using DocumentDefinitionReplyHandler = ReplyHandler<QList<LSPLocation>>;
using DocumentHighlightReplyHandler = ReplyHandler<QList<LSPDocumentHighlight>>;
using DocumentHoverReplyHandler = ReplyHandler<LSPHover>;
using DocumentCompletionReplyHandler = ReplyHandler<QList<LSPCompletionItem>>;
using SignatureHelpReplyHandler = ReplyHandler<LSPSignatureHelp>;
using FormattingReplyHandler = ReplyHandler<QList<LSPTextEdit>>;
using CodeActionReplyHandler = ReplyHandler<QList<LSPCodeAction>>;

class LSPClientServer : public QObject
{
    Q_OBJECT

public:
    enum class State
    {
        None,
        Started,
        Running,
        Shutdown
    };

    class LSPClientServerPrivate;
    class RequestHandle
    {
        friend class LSPClientServerPrivate;
        QPointer<LSPClientServer> m_server;
        int m_id = -1;
    public:
        RequestHandle& cancel()
        {
            if (m_server)
                m_server->cancel(m_id);
            return *this;
        }
    };

    LSPClientServer(const QStringList & server, const QUrl & root,
                    const QJsonValue & init = QJsonValue());
    ~LSPClientServer();

    // server management
    // request start
    bool start();
    // request shutdown/stop
    // if to_xxx >= 0 -> send signal if not exit'ed after timeout
    void stop(int to_term_ms, int to_kill_ms);
    int cancel(int id);

    // properties
    const QStringList& cmdline() const;
    State state() const;
    Q_SIGNAL void stateChanged(LSPClientServer * server);

    const LSPServerCapabilities& capabilities() const;

    // language
    RequestHandle documentSymbols(const QUrl & document, const QObject *context,
        const DocumentSymbolsReplyHandler & h);
    RequestHandle documentDefinition(const QUrl & document, const LSPPosition & pos,
        const QObject *context, const DocumentDefinitionReplyHandler & h);
    RequestHandle documentDeclaration(const QUrl & document, const LSPPosition & pos,
        const QObject *context, const DocumentDefinitionReplyHandler & h);
    RequestHandle documentHighlight(const QUrl & document, const LSPPosition & pos,
        const QObject *context, const DocumentHighlightReplyHandler & h);
    RequestHandle documentHover(const QUrl & document, const LSPPosition & pos,
        const QObject *context, const DocumentHoverReplyHandler & h);
    RequestHandle documentReferences(const QUrl & document, const LSPPosition & pos, bool decl,
        const QObject *context, const DocumentDefinitionReplyHandler & h);
    RequestHandle documentCompletion(const QUrl & document, const LSPPosition & pos,
        const QObject *context, const DocumentCompletionReplyHandler & h);
    RequestHandle signatureHelp(const QUrl & document, const LSPPosition & pos,
        const QObject *context, const SignatureHelpReplyHandler & h);

    RequestHandle documentFormatting(const QUrl & document, int tabSize, bool insertSpaces,
        const QJsonObject & options, const QObject *context, const FormattingReplyHandler & h);
    RequestHandle documentRangeFormatting(const QUrl & document, const LSPRange & range,
        int tabSize, bool insertSpaces, const QJsonObject & options,
        const QObject *context, const FormattingReplyHandler & h);

    RequestHandle documentCodeAction(const QUrl & document, const LSPRange & range,
        const QList<QString> & kinds, QList<LSPDiagnostic> diagnostics,
        const QObject *context, const CodeActionReplyHandler & h);

    // sync
    void didOpen(const QUrl & document, int version, const QString & text);
    void didChange(const QUrl & document, int version, const QString & text);
    void didSave(const QUrl & document, const QString & text);
    void didClose(const QUrl & document);

    // notifcation = signal
Q_SIGNALS:
    void publishDiagnostics(const LSPPublishDiagnosticsParams & );

private:
    // pimpl data holder
    LSPClientServerPrivate * const d;
};

#endif
