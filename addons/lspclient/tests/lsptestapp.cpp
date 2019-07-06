/* This file is part of the KDE project
 *
 *  Copyright (C) 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "../lspclientserver.h"
#include <iostream>

#include <QCoreApplication>
#include <QEventLoop>
#include <QFile>
#include <QTextStream>

int main(int argc, char ** argv)
{
    if (argc < 5)
        return -1;

    LSPClientServer lsp(QString::fromLatin1(argv[1]).split(QStringLiteral(" ")),
            QUrl(QString::fromLatin1(argv[2])));

    QCoreApplication app(argc, argv);
    QEventLoop q;

    auto state_h = [&lsp, &q] () {
        if (lsp.state() == LSPClientServer::State::Running)
            q.quit();
    };
    auto conn = QObject::connect(&lsp, &LSPClientServer::stateChanged, state_h);
    lsp.start();
    q.exec();
    QObject::disconnect(conn);

    auto document = QUrl(QString::fromLatin1(argv[3]));

    QFile file(document.path());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return -1;
    QTextStream in(&file);
    QString content = in.readAll();
    lsp.didOpen(document, 0, content);

    auto ds_h = [&q] (const QList<LSPSymbolInformation> & syms) {
        std::cout << "symbol count: " << syms.length() << std::endl;
        q.quit();
    };
    lsp.documentSymbols(document, &app, ds_h);
    q.exec();

    auto position = QString::fromLatin1(argv[4]).split(QStringLiteral(" "));
    auto def_h = [&q] (const QList<LSPLocation> & defs) {
        std::cout << "definition count: " << defs.length() << std::endl;
        q.quit();
    };
    lsp.documentDefinition(document, {position[0].toInt(), position[1].toInt()}, &app, def_h);
    q.exec();

    auto comp_h = [&q] (const QList<LSPCompletionItem> & completions) {
        std::cout << "completion count: " << completions.length() << std::endl;
        q.quit();
    };
    lsp.documentCompletion(document, {position[0].toInt(), position[1].toInt()}, &app, comp_h);
    q.exec();

    auto sig_h = [&q] (const LSPSignatureHelp & help) {
        std::cout << "signature help count: " << help.signatures.length() << std::endl;
        q.quit();
    };
    lsp.signatureHelp(document, {position[0].toInt(), position[1].toInt()}, &app, sig_h);
    q.exec();

    auto hover_h = [&q] (const LSPHover & hover) {
        std::cout << "hover: " << hover.contents.value.toStdString() << std::endl;
        q.quit();
    };
    lsp.documentHover(document, {position[0].toInt(), position[1].toInt()}, &app, hover_h);
    q.exec();

    auto ref_h = [&q] (const QList<LSPLocation> & refs) {
        std::cout << "refs: " << refs.length() << std::endl;
        q.quit();
    };
    lsp.documentReferences(document, {position[0].toInt(), position[1].toInt()}, true, &app, ref_h);
    q.exec();

    // lsp.didOpen(document, 0, QStringLiteral("blah"));
    lsp.didChange(document, 1, QStringLiteral("foo"));
    lsp.didClose(document);
}
