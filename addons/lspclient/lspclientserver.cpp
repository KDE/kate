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

#include "lspclientserver.h"

#include "lspclient_debug.h"

#include <QScopedPointer>
#include <QProcess>
#include <QVariantMap>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCoreApplication>
#include <QTime>

static QString MEMBER_ID = QStringLiteral("id");
static QString MEMBER_METHOD = QStringLiteral("method");
static QString MEMBER_PARAMS = QStringLiteral("params");
static QString MEMBER_RESULT = QStringLiteral("result");
static QString MEMBER_URI = QStringLiteral("uri");
static QString MEMBER_VERSION = QStringLiteral("version");
static QString MEMBER_START = QStringLiteral("start");
static QString MEMBER_END = QStringLiteral("end");
static QString MEMBER_POSITION = QStringLiteral("position");
static QString MEMBER_LOCATION = QStringLiteral("location");
static QString MEMBER_RANGE = QStringLiteral("range");
static QString MEMBER_LINE = QStringLiteral("line");
static QString MEMBER_CHARACTER = QStringLiteral("character");
static QString MEMBER_TEXT = QStringLiteral("text");
static QString MEMBER_LANGID = QStringLiteral("languageId");

static const int TIMEOUT_SHUTDOWN = 200;

// message construction helpers
static QJsonObject
versionedTextDocumentIdentifier(const QUrl & document, int version = -1)
{
    QJsonObject map { { MEMBER_URI, document.toString() } };
    if (version >= 0)
        map[MEMBER_VERSION] = version;
    return map;
}

static QJsonObject
textDocumentItem(const QUrl & document, const QString & lang,
                 const QString & text, int version)
{
    auto map = versionedTextDocumentIdentifier(document, version);
    map[MEMBER_TEXT] = text;
    // TODO ?? server does not mind
    map[MEMBER_LANGID] = lang;
    return map;
}

static QJsonObject
textDocumentParams(const QJsonObject & m)
{
    return QJsonObject {
        { QStringLiteral("textDocument"), m}
    };
}

static QJsonObject
textDocumentParams(const QUrl & document, int version = -1)
{ return textDocumentParams(versionedTextDocumentIdentifier(document, version)); }

static QJsonObject
textDocumentPositionParams(const QUrl & document, LSPPosition pos)
{
    auto params = textDocumentParams(document);
    params[MEMBER_POSITION] = QJsonObject {
        { MEMBER_LINE, pos.line },
        { MEMBER_CHARACTER, pos.column }
    };
    return params;
}

static void
from_json(QVector<QChar> & trigger, const QJsonValue & json)
{
    for (const auto & t : json.toArray()) {
        auto st = t.toString();
        if (st.length())
            trigger.push_back(st.at(0));
    }
}

static void
from_json(LSPCompletionOptions & options, const QJsonValue & json)
{
    if (json.isObject()) {
        auto ob = json.toObject();
        options.provider = true;
        options.resolveProvider = ob.value(QStringLiteral("resolveProvider")).toBool();
        from_json(options.triggerCharacters, ob.value(QStringLiteral("triggerCharacters")));
    }
}

static void
from_json(LSPSignatureHelpOptions & options, const QJsonValue & json)
{
    if (json.isObject()) {
        auto ob = json.toObject();
        options.provider = true;
        from_json(options.triggerCharacters, ob.value(QStringLiteral("triggerCharacters")));
    }
}

static void
from_json(LSPServerCapabilities & caps, const QJsonObject & json)
{
    auto sync = json.value(QStringLiteral("textDocumentSync"));
    caps.textDocumentSync = (LSPDocumentSyncKind)
            (sync.isObject() ? sync.toObject().value(QStringLiteral("change")) : sync).toInt((int)LSPDocumentSyncKind::None);
    caps.hoverProvider = json.value(QStringLiteral("hoverProvider")).toBool();
    from_json(caps.completionProvider, json.value(QStringLiteral("completionProvider")));
    from_json(caps.signatureHelpProvider, json.value(QStringLiteral("signatureHelpProvider")));
    caps.definitionProvider = json.value(QStringLiteral("definitionProvider")).toBool();
    caps.declarationProvider = json.value(QStringLiteral("declarationProvider")).toBool();
    caps.referencesProvider = json.value(QStringLiteral("referencesProvider")).toBool();
    caps.documentSymbolProvider = json.value(QStringLiteral("documentSymbolProvider")).toBool();
}

using GenericReplyType = QJsonValue;
using GenericReplyHandler = ReplyHandler<GenericReplyType>;

class LSPClientServer::LSPClientServerPrivate
{
    typedef LSPClientServerPrivate self_type;

    LSPClientServer *q;
    // server cmd line
    QStringList m_server;
    // workspace root to pass along
    QUrl m_root;
    // server process
    QProcess m_sproc;
    // server declared capabilites
    LSPServerCapabilities m_capabilities;
    // server state
    State m_state = State::None;
    // last msg id
    int m_id = 0;
    // receive buffer
    QByteArray m_receive;
    // registered reply handlers
    QHash<int, GenericReplyHandler> m_handlers;

public:
    LSPClientServerPrivate(LSPClientServer * _q, const QStringList & server, const QUrl & root)
        : q(_q), m_server(server), m_root(root)
    {
        // setup async reading
        QObject::connect(&m_sproc, &QProcess::readyRead, mem_fun(&self_type::read, this));
        QObject::connect(&m_sproc, &QProcess::stateChanged, mem_fun(&self_type::onStateChanged, this));
    }

    ~LSPClientServerPrivate()
    {
        stop(TIMEOUT_SHUTDOWN, TIMEOUT_SHUTDOWN);
    }

    State state()
    {
        return m_state;
    }

    const LSPServerCapabilities&
    capabilities()
    {
        return m_capabilities;
    }

    int cancel(int reqid)
    {
        // TODO also send cancel to server
        m_handlers.remove(reqid);
        return -1;
    }

private:
    void setState(State s)
    {
        m_state = s;
        emit q->stateChanged();
    }

    RequestHandle write(const QJsonObject & msg, const GenericReplyHandler & h = nullptr)
    {
        RequestHandle ret;
        ret.m_server = q;

        if (!running())
            return ret;

        auto ob = msg;
        ob.insert(QStringLiteral("jsonrpc"), QStringLiteral("2.0"));
        // notification == no handler
        if (h) {
            ob.insert(MEMBER_ID, ++m_id);
            ret.m_id = m_id;
            m_handlers[m_id] = h;
        }

        QJsonDocument json(ob);
        auto sjson = json.toJson();

        qCInfo(LSPCLIENT) << "calling" << msg[MEMBER_METHOD].toString();
        qCDebug(LSPCLIENT) << "sending message:\n" << QString::fromUtf8(sjson);
        // some simple parsers expect length header first
        auto hdr = QStringLiteral("Content-Length: %1\r\n").arg(sjson.length());
        // write is async, so no blocking wait occurs here
        m_sproc.write(hdr.toLatin1());
        m_sproc.write("Content-Type: application/vscode-jsonrpc; charset=utf-8\r\n\r\n");
        m_sproc.write(sjson);

        return ret;
    }

    RequestHandle
    send(const QJsonObject & msg, const GenericReplyHandler & h = nullptr)
    {
        Q_ASSERT (m_state == State::Running);
        if (m_state == State::Running)
            return write(msg, h);
        return RequestHandle();
    }

    void read()
    {
        // accumulate in buffer
        m_receive.append(m_sproc.readAllStandardOutput());

        // try to get one (or more) message
        QByteArray &buffer = m_receive;

        while (true) {
            qCDebug(LSPCLIENT) << "reply buffer size" << buffer.length();
            // TODO constant
            auto header = QByteArray("Content-Length:");
            int index = buffer.indexOf(header);
            if (index < 0 )
                break;
            index += header.length();
            int endindex = buffer.indexOf("\r\n", index);
            auto msgstart = buffer.indexOf("\r\n\r\n", index);
            if (endindex < 0 || msgstart < 0)
                break;
            msgstart += 4;
            bool ok = false;
            auto length = buffer.mid(index, endindex - index).toInt(&ok, 10);
            // TODO more error handling (shutdown)
            if (!ok) {
                qCWarning(LSPCLIENT) << "invalid Content-Length";
                break;
            }
            if (msgstart + length > buffer.length())
                break;
            // now onto payload
            auto payload = buffer.mid(msgstart, length);
            buffer.remove(0, msgstart + length);
            qCInfo(LSPCLIENT) << "got message payload size " << length;
            qCDebug(LSPCLIENT) << "message payload:\n" << payload;
            QJsonParseError error{};
            auto msg = QJsonDocument::fromJson(payload, &error);
            if (error.error != QJsonParseError::NoError || !msg.isObject()) {
                qCWarning(LSPCLIENT) << "invalid response payload";
                continue;
            }
            auto result = msg.object();
            // check if it is the expected result
            int msgid = -1;
            if (result.contains(MEMBER_ID)) {
                msgid = result[MEMBER_ID].toInt();
            } else {
                // notification; never mind those for now
                qCWarning(LSPCLIENT) << "discarding notification"
                                     << msg[MEMBER_METHOD].toString();
                continue;
            }
            // a valid reply; what to do with it now
            auto it = m_handlers.find(msgid);
            if (it != m_handlers.end()) {
                (*it)(result.value(MEMBER_RESULT));
                m_handlers.erase(it);
            } else {
                // could have been canceled
                qCDebug(LSPCLIENT) << "unexpected reply id";
            }
        }
    }

    static QJsonObject
    init_request(const QString & method, const QJsonObject & params = QJsonObject())
    {
        return QJsonObject {
            { MEMBER_METHOD, method },
            { MEMBER_PARAMS, params }
        };
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

    void onInitializeReply(const QJsonValue & value)
    {
        // only parse parts that we use later on
        from_json(m_capabilities, value.toObject().value(QStringLiteral("capabilities")).toObject());
        // finish init
        initialized();
    }

    void initialize()
    {
        QJsonObject capabilities {
            { QStringLiteral("textDocument"),
                QJsonObject {
                    { QStringLiteral("documentSymbol"),
                                QJsonObject { { QStringLiteral("hierarchicalDocumentSymbolSupport"), true } }
                    }
                }
            }
        };
        // NOTE a typical server does not use root all that much,
        // other than for some corner case (in) requests
        QJsonObject params {
            { QStringLiteral("processId"), QCoreApplication::applicationPid() },
            { QStringLiteral("rootPath"), m_root.path() },
            { QStringLiteral("rootUri"), m_root.toString() },
            { QStringLiteral("capabilities"), capabilities }
        };
        //
        write(init_request(QStringLiteral("initialize"), params),
             mem_fun(&self_type::onInitializeReply, this));
    }

    void initialized()
    {
        write(init_request(QStringLiteral("initialized")));
        setState(State::Running);
    }

public:
    bool start()
    {
        if (m_state != State::None)
            return true;

        auto program = m_server.front();
        auto args = m_server;
        args.pop_front();
        qCInfo(LSPCLIENT) << "starting" << m_server;
        // at least we see some errors somewhere then
        m_sproc.setProcessChannelMode(QProcess::ForwardedErrorChannel);
        m_sproc.setReadChannel(QProcess::QProcess::StandardOutput);
        m_sproc.start(program, args);
        bool result = m_sproc.waitForStarted();
        if (!result) {
            qCWarning(LSPCLIENT) << m_sproc.error();
        } else {
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
            if ((to_term >= 0) && !m_sproc.waitForFinished(to_term))
                m_sproc.terminate();
            if ((to_kill >= 0) && !m_sproc.waitForFinished(to_kill))
                m_sproc.kill();
        }
    }

    RequestHandle documentSymbols(const QUrl & document, const GenericReplyHandler & h)
    {
        auto params = textDocumentParams(document);
        return send(init_request(QStringLiteral("textDocument/documentSymbol"), params), h);
    }

    RequestHandle documentDefinition(const QUrl & document, const LSPPosition & pos,
        const GenericReplyHandler & h)
    {
        auto params = textDocumentPositionParams(document, pos);
        return send(init_request(QStringLiteral("textDocument/definition"), params), h);
    }

    RequestHandle documentCompletion(const QUrl & document, const LSPPosition & pos,
        const GenericReplyHandler & h)
    {
        auto params = textDocumentPositionParams(document, pos);
        return send(init_request(QStringLiteral("textDocument/completion"), params), h);
    }

    void didOpen(const QUrl & document, int version, const QString & text)
    {
        auto params = textDocumentParams(textDocumentItem(document, QString(), text, version));
        send(init_request(QStringLiteral("textDocument/didOpen"), params));
    }

    void didChange(const QUrl & document, int version, const QString & text)
    {
        auto params = textDocumentParams(document, version);
        params[QStringLiteral("contentChanges")] = QJsonArray {
            QJsonObject {{MEMBER_TEXT, text}}
        };
        send(init_request(QStringLiteral("textDocument/didChange"), params));
    }

    void didSave(const QUrl & document, const QString & text)
    {
        auto params = textDocumentParams(document);
        params[QStringLiteral("text")] = text;
        send(init_request(QStringLiteral("textDocument/didSave"), params));
    }

    void didClose(const QUrl & document)
    {
        auto params = textDocumentParams(document);
        send(init_request(QStringLiteral("textDocument/didClose"), params));
    }
};

static LSPPosition
parsePosition(const QJsonObject & m)
{
    auto vline = m.value(MEMBER_LINE);
    auto vcharacter = m.value(MEMBER_CHARACTER);
    auto line = vline.toInt(-1);
    auto column = vcharacter.toInt(-1);
    return {line, column};
}

static bool
isPositionValid(const LSPPosition & pos)
{ return pos.line >= 0 && pos.column >=0; }

static QList<LSPSymbolInformation>
parseDocumentSymbols(const QJsonValue & result)
{
    // the reply could be old SymbolInformation[] or new (hierarchical) DocumentSymbol[]
    // try to parse it adaptively in any case
    // if new style, hierarchy is specified clearly in reply
    // if old style, it is assumed the values enter linearly, that is;
    // * a parent/container is listed before its children
    // * if a name is defined/declared several times and then used as a parent,
    //   then it is the last instance that is used as a parent

    QList<LSPSymbolInformation> ret;
    QMap<QString, LSPSymbolInformation*> index;

    std::function<void(const QJsonObject & symbol, LSPSymbolInformation *parent)> parseSymbol =
        [&] (const QJsonObject & symbol, LSPSymbolInformation *parent) {
        // if flat list, try to find parent by name
        if (!parent) {
            auto container = symbol.value(QStringLiteral("containerName")).toString();
            parent = index.value(container, nullptr);
        }
        auto list = parent ? &parent->children : &ret;
        auto name = symbol.value(QStringLiteral("name")).toString();
        auto kind = (LSPSymbolKind) symbol.value(QStringLiteral("kind")).toInt();
        const auto& location = symbol.value(MEMBER_LOCATION).toObject();
        const auto& range = symbol.contains(MEMBER_RANGE) ?
                    symbol.value(MEMBER_RANGE).toObject() : location.value(MEMBER_RANGE).toObject();
        auto startpos = parsePosition(range.value(MEMBER_START).toObject());
        auto endpos = parsePosition(range.value(MEMBER_END).toObject());
        if (isPositionValid(startpos) && isPositionValid(endpos)) {
            list->push_back({name, kind, startpos, endpos});
            index[name] = &list->back();
            // proceed recursively
            for (const auto &child : symbol.value(QStringLiteral("children")).toArray())
                parseSymbol(child.toObject(), &list->back());
        }
    };

    for (const auto& info : result.toArray()) {
        parseSymbol(info.toObject(), nullptr);
    }
    return ret;
}


QList<LSPDocumentPosition>
parseDocumentDefinition(const QJsonValue & result)
{
    QList<LSPDocumentPosition> ret;
    for (const auto & def : result.toArray()) {
        const auto & mdef = def.toObject();
        auto uri = mdef.value(MEMBER_URI).toString();
        const auto& range = mdef.value(MEMBER_RANGE).toObject();
        const auto& start = range.value(MEMBER_START).toObject();
        auto startpos = parsePosition(start);
        if (uri.length() > 0 && isPositionValid(startpos))
            ret.push_back({QUrl(uri), startpos.line, startpos.column});
    }
    return ret;
}


QList<LSPCompletionItem>
parseDocumentCompletion(const QJsonValue & result)
{
    QList<LSPCompletionItem> ret;
    QJsonArray items = result.toArray();
    // might be CompletionList
    if (items.size() == 0) {
        items = result.toObject().value(QStringLiteral("items")).toArray();
    }
    for (const auto & vitem : items) {
        const auto & item = vitem.toObject();
        auto label = item.value(QStringLiteral("label")).toString();
        auto detail = item.value(QStringLiteral("detail")).toString();
        auto doc = item.value(QStringLiteral("documentation")).toString();
        auto sortText = item.value(QStringLiteral("sortText")).toString();
        auto insertText = item.value(QStringLiteral("insertText")).toString();
        auto kind = (LSPCompletionItemKind) item.value(QStringLiteral("kind")).toInt();
        ret.push_back({label, kind, detail, doc, sortText, insertText});
    }
    return ret;
}

// prevent argument deduction
template<typename T> struct identity { typedef T type; };

// generic convert handler
// sprinkle some connection-like context safety
// not so likely relevant/needed due to typical sequence of events,
// but in case the latter would be changed in surprising ways ...
template<typename ReplyType>
static GenericReplyHandler make_handler(const ReplyHandler<ReplyType> & h,
    const QObject *context,
    typename identity<std::function<ReplyType(const GenericReplyType&)>>::type c)
{
    QPointer<const QObject> ctx(context);
    return [ctx, h, c] (const GenericReplyType & m) { if (ctx) h(c(m)); };
}


LSPClientServer::LSPClientServer(const QStringList & server, const QUrl & root)
    : d(new LSPClientServerPrivate(this, server, root))
{}

LSPClientServer::~LSPClientServer()
{ delete d; }

LSPClientServer::State LSPClientServer::state() const
{ return d->state(); }

const LSPServerCapabilities&
LSPClientServer::capabilities() const
{ return d->capabilities(); }

bool LSPClientServer::start()
{ return d->start(); }

void LSPClientServer::stop(int to_t, int to_k)
{ return d->stop(to_t, to_k); }

int LSPClientServer::cancel(int reqid)
{ return d->cancel(reqid); }

LSPClientServer::RequestHandle
LSPClientServer::documentSymbols(const QUrl & document, const QObject *context,
    const DocumentSymbolsReplyHandler & h)
{ return d->documentSymbols(document, make_handler(h, context, parseDocumentSymbols)); }

LSPClientServer::RequestHandle
LSPClientServer::documentDefinition(const QUrl & document, const LSPPosition & pos,
    const QObject *context, const DocumentDefinitionReplyHandler & h)
{ return d->documentDefinition(document, pos, make_handler(h, context, parseDocumentDefinition)); }

LSPClientServer::RequestHandle
LSPClientServer::documentCompletion(const QUrl & document, const LSPPosition & pos,
    const QObject *context, const DocumentCompletionReplyHandler & h)
{ return d->documentCompletion(document, pos, make_handler(h, context, parseDocumentCompletion)); }

void LSPClientServer::didOpen(const QUrl & document, int version, const QString & text)
{ return d->didOpen(document, version, text); }

void LSPClientServer::didChange(const QUrl & document, int version, const QString & text)
{ return d->didChange(document, version, text); }

void LSPClientServer::didSave(const QUrl & document, const QString & text)
{ return d->didSave(document, text); }

void LSPClientServer::didClose(const QUrl & document)
{ return d->didClose(document); }
