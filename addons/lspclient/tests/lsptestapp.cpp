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
    lsp.documentSymbols(document, ds_h);
    q.exec();

    auto position = QString::fromLatin1(argv[4]).split(QStringLiteral(" "));
    auto def_h = [&q] (const QList<LSPDocumentPosition> & defs) {
        std::cout << "definition count: " << defs.length() << std::endl;
        q.quit();
    };
    lsp.documentDefinition(document, {position[0].toInt(), position[1].toInt()}, def_h);
    q.exec();

    auto comp_h = [&q] (const QList<LSPCompletionItem> & completions) {
        std::cout << "completion count: " << completions.length() << std::endl;
        q.quit();
    };
    lsp.documentCompletion(document, {position[0].toInt(), position[1].toInt()}, comp_h);
    q.exec();

    // lsp.didOpen(document, 0, QStringLiteral("blah"));
    lsp.didChange(document, 1, QStringLiteral("foo"));
    lsp.didClose(document);
}
