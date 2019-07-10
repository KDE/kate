/***************************************************************************
 *   Copyright (C) 2019 by Mark Nauwelaerts <mark.nauwelaerts@gmail.com>   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

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

    // sync
    void didOpen(const QUrl & document, int version, const QString & text);
    void didChange(const QUrl & document, int version, const QString & text);
    void didSave(const QUrl & document, const QString & text);
    void didClose(const QUrl & document);

private:
    // pimpl data holder
    LSPClientServerPrivate * const d;
};

#endif
