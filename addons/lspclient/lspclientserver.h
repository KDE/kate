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

#include <QObject>
#include <QString>
#include <QUrl>
#include <QList>
#include <QVector>
#include <QPointer>

#include <functional>

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
};

struct LSPPosition
{
    // both are 0-based
    int line;
    int column;
};

struct LSPDocumentPosition
{
    QUrl uri;
    LSPPosition position;
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
                         LSPPosition _start, LSPPosition _end,
                         const QString & _detail = QString())
        : name(_name), detail(_detail), kind(_kind), start(_start), end(_end)
    {}
    QString name;
    QString detail;
    LSPSymbolKind kind;
    LSPPosition start;
    LSPPosition end;
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
    QString documentation;
    QString sortText;
    QString insertText;
};

template<typename T>
using ReplyHandler = std::function<void(const T &)>;

using DocumentSymbolsReplyHandler = ReplyHandler<QList<LSPSymbolInformation>>;
using DocumentDefinitionReplyHandler = ReplyHandler<QList<LSPDocumentPosition>>;
using DocumentCompletionReplyHandler = ReplyHandler<QList<LSPCompletionItem>>;

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

    LSPClientServer(const QStringList & server, const QUrl & root);
    ~LSPClientServer();

    // server management
    // request start
    bool start();
    // request shutdown/stop
    // if to_xxx >= 0 -> send signal if not exit'ed after timeout
    void stop(int to_term_ms, int to_kill_ms);
    int cancel(int id);

    // properties
    State state() const;
    Q_SIGNAL void stateChanged();

    const LSPServerCapabilities& capabilities() const;

    // language
    RequestHandle documentSymbols(const QUrl & document, const QObject *context,
        const DocumentSymbolsReplyHandler & h);
    RequestHandle documentDefinition(const QUrl & document, const LSPPosition & pos,
        const QObject *context, const DocumentDefinitionReplyHandler & h);
    RequestHandle documentCompletion(const QUrl & document, const LSPPosition & pos,
        const QObject *context, const DocumentCompletionReplyHandler & h);

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
