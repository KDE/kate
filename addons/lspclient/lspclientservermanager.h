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

#ifndef LSPCLIENTSERVERMANAGER_H
#define LSPCLIENTSERVERMANAGER_H

#include "lspclientserver.h"
#include "lspclientplugin.h"

#include <QSharedPointer>

namespace KTextEditor {
    class MainWindow;
    class Document;
    class View;
}

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

    virtual void update(KTextEditor::Document *doc) = 0;

    virtual void restart(LSPClientServer *server) = 0;

public:
Q_SIGNALS:
    void serverChanged();
};

#endif
