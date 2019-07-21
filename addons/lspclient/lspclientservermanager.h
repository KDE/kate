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

#ifndef LSPCLIENTSERVERMANAGER_H
#define LSPCLIENTSERVERMANAGER_H

#include "lspclientserver.h"
#include "lspclientplugin.h"

#include <QSharedPointer>

namespace KTextEditor {
    class MainWindow;
    class Document;
    class View;
    class MovingInterface;
}

class LSPClientRevisionSnapshot;

/*
 * A helper class that manages LSP servers in relation to a KTextDocument.
 * That is, spin up a server for a document when so requested, and then
 * monitor when the server is up (or goes down) and the document (for changes).
 * Those changes may then be synced to the server when so requested (e.g. prior
 * to another component performing an LSP request for a document).
 * So, other than managing servers, it also manages the document-server
 * relationship (and document), what's in a name ...
 */
class LSPClientServerManager : public QObject
{
    Q_OBJECT

public:
    // factory method; private implementation by interface
    static QSharedPointer<LSPClientServerManager>
    new_(LSPClientPlugin *plugin, KTextEditor::MainWindow *mainWin);

    virtual QSharedPointer<LSPClientServer>
    findServer(KTextEditor::Document *document, bool updatedoc = true) = 0;

    virtual QSharedPointer<LSPClientServer>
    findServer(KTextEditor::View *view, bool updatedoc = true) = 0;

    virtual void update(KTextEditor::Document *doc, bool force) = 0;

    virtual void restart(LSPClientServer *server) = 0;

    // lock all relevant documents' current revision and sync that to server
    // locks are released when returned snapshot is delete'd
    virtual LSPClientRevisionSnapshot* snapshot(LSPClientServer *server) = 0;

public:
Q_SIGNALS:
    void serverChanged();
};

class LSPClientRevisionSnapshot : public QObject
{
    Q_OBJECT

public:
    // find a locked revision for url in snapshot
    virtual void find(const QUrl & url, KTextEditor::MovingInterface* & miface, qint64 & revision) const = 0;
};

#endif
