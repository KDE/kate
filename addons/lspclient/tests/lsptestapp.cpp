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

#include "../lspclientserver.h"
#include <iostream>

#include <QCoreApplication>
#include <QEventLoop>
#include <QFile>
#include <QJsonObject>
#include <QTextStream>

int main(int argc, char **argv)
{
    if (argc < 5)
        return -1;

    LSPClientServer lsp(QString::fromLatin1(argv[1]).split(QLatin1Char(' ')), QUrl(QString::fromLatin1(argv[2])));

    QCoreApplication app(argc, argv);
    QEventLoop q;

    auto state_h = [&lsp, &q]() {
        if (lsp.state() == LSPClientServer::State::Running)
            q.quit();
    };
    auto conn = QObject::connect(&lsp, &LSPClientServer::stateChanged, state_h);
    lsp.start();
    q.exec();
    QObject::disconnect(conn);

    auto diagnostics_h = [](const LSPPublishDiagnosticsParams &diag) { std::cout << "diagnostics  " << diag.uri.path().toUtf8().toStdString() << " count: " << diag.diagnostics.length(); };

    QObject::connect(&lsp, &LSPClientServer::publishDiagnostics, diagnostics_h);

    auto document = QUrl(QString::fromLatin1(argv[3]));

    QFile file(document.path());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return -1;
    QTextStream in(&file);
    QString content = in.readAll();
    lsp.didOpen(document, 0, QString(), content);

    auto ds_h = [&q](const QList<LSPSymbolInformation> &syms) {
        std::cout << "symbol count: " << syms.length() << std::endl;
        q.quit();
    };
    lsp.documentSymbols(document, &app, ds_h);
    q.exec();

    auto position = QString::fromLatin1(argv[4]).split(QLatin1Char(' '));
    auto def_h = [&q](const QList<LSPLocation> &defs) {
        std::cout << "definition count: " << defs.length() << std::endl;
        q.quit();
    };
    lsp.documentDefinition(document, {position[0].toInt(), position[1].toInt()}, &app, def_h);
    q.exec();

    auto comp_h = [&q](const QList<LSPCompletionItem> &completions) {
        std::cout << "completion count: " << completions.length() << std::endl;
        q.quit();
    };
    lsp.documentCompletion(document, {position[0].toInt(), position[1].toInt()}, &app, comp_h);
    q.exec();

    auto sig_h = [&q](const LSPSignatureHelp &help) {
        std::cout << "signature help count: " << help.signatures.length() << std::endl;
        q.quit();
    };
    lsp.signatureHelp(document, {position[0].toInt(), position[1].toInt()}, &app, sig_h);
    q.exec();

    auto hover_h = [&q](const LSPHover &hover) {
        for (auto &element : hover.contents) {
            std::cout << "hover: " << element.value.toStdString() << std::endl;
        }
        q.quit();
    };
    lsp.documentHover(document, {position[0].toInt(), position[1].toInt()}, &app, hover_h);
    q.exec();

    auto ref_h = [&q](const QList<LSPLocation> &refs) {
        std::cout << "refs: " << refs.length() << std::endl;
        q.quit();
    };
    lsp.documentReferences(document, {position[0].toInt(), position[1].toInt()}, true, &app, ref_h);
    q.exec();

    auto hl_h = [&q](const QList<LSPDocumentHighlight> &hls) {
        std::cout << "highlights: " << hls.length() << std::endl;
        q.quit();
    };
    lsp.documentHighlight(document, {position[0].toInt(), position[1].toInt()}, &app, hl_h);
    q.exec();

    auto fmt_h = [&q](const QList<LSPTextEdit> &edits) {
        std::cout << "edits: " << edits.length() << std::endl;
        q.quit();
    };
    lsp.documentFormatting(document, {2, true, QJsonObject()}, &app, fmt_h);
    q.exec();

    // lsp.didOpen(document, 0, QStringLiteral("blah"));
    lsp.didChange(document, 1, QStringLiteral("foo"));
    lsp.didClose(document);
}
