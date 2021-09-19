#include "ce_service.h"

#include <QtNetwork/QNetworkReply>

#include <KConfigGroup>
#include <KSharedConfig>
#include <QJsonDocument>
#include <QJsonObject>

// static const char url[] = "http://localhost:10240/api/";

CompilerExplorerSvc *CompilerExplorerSvc::instance()
{
    static CompilerExplorerSvc s_instance;
    return &s_instance;
}

void CompilerExplorerSvc::changeUrl(const QString &newUrl)
{
    bool initializing = url.isEmpty();
    url = newUrl;

    if (url.endsWith(QLatin1Char('/'))) {
        url.chop(1);
    }

    if (!url.endsWith(QLatin1String("/api"))) {
        url.append(QLatin1String("/api/"));
    }

    // Since the url has changed, we must refresh compilers/languages
    if (!initializing) {
        sendRequest(CompilerExplorer::Languages);
        sendRequest(CompilerExplorer::Compilers);
    }
}

void CompilerExplorerSvc::sendRequest(CompilerExplorer::Endpoints endpoint, const QString &additional)
{
    QString endp = CompilerExplorer::endpointsToString.value(endpoint);
    QString requestUrl = url + endp + additional;
    QUrl url{requestUrl};
    QNetworkRequest req{url};
    req.setRawHeader("ACCEPT", "application/json");
    req.setRawHeader("Content-Type", "application/json");

    mgr->get(req);
}

void CompilerExplorerSvc::compileRequest(const QString &endpoint, const QByteArray &obj)
{
    QString requestUrl = url + endpoint;
    QNetworkRequest req{QUrl{requestUrl}};
    req.setRawHeader("ACCEPT", "application/json");
    req.setRawHeader("Content-Type", "application/json");
    mgr->post(req, obj);
}

QNetworkReply *CompilerExplorerSvc::tooltipRequest(const QString &asmWord)
{
    QNetworkRequest request;
    QString urlString = url;
    urlString += QStringLiteral("asm/") + asmWord;
    request.setRawHeader("ACCEPT", "application/json");
    request.setRawHeader("Content-Type", "application/json");
    request.setUrl(QUrl(urlString));
    return mgr->get(request);
}

CompilerExplorerSvc::~CompilerExplorerSvc()
{
    delete mgr;
}

CompilerExplorerSvc::CompilerExplorerSvc(QObject *parent)
    : QObject(parent)
{
    mgr = new QNetworkAccessManager(this);
    connect(mgr, &QNetworkAccessManager::finished, this, &CompilerExplorerSvc::slotNetworkReply);

    KConfigGroup cg(KSharedConfig::openConfig(), "kate_compilerexplorer");
    changeUrl(cg.readEntry("kate_compilerexplorer_url", QStringLiteral("http://localhost:10240")));
}

void CompilerExplorerSvc::slotNetworkReply(QNetworkReply *reply)
{
    const QString path = reply->url().path().split(QLatin1Char('/')).at(2);
    CompilerExplorer::Endpoints endpoint;
    if (path.startsWith(QLatin1String("compilers"))) {
        endpoint = CompilerExplorer::stringToEndpoint.value(QStringLiteral("compilers"));
    } else if (path.startsWith(QLatin1String("compiler"))) {
        endpoint = CompilerExplorer::stringToEndpoint.value(QStringLiteral("compiler"));
    } else if (path.startsWith(QStringLiteral("asm"))) {
        return;
    } else {
        endpoint = CompilerExplorer::stringToEndpoint.value(path);
    }
    const QByteArray data = reply->readAll();

    switch (endpoint) {
    case CompilerExplorer::Languages: {
        Q_EMIT languages(data);
        break;
    }
    case CompilerExplorer::Compilers:
        Q_EMIT compilers(data);
        break;
    case CompilerExplorer::CompilerCompile:
        Q_EMIT asmResult(data);
        break;
    }
}

QJsonDocument CompilerExplorerSvc::getCompilationOptions(const QString &source,
                                                         const QString &userArgs,
                                                         bool isIntel,
                                                         bool demangle,
                                                         bool stripUnusedLabels,
                                                         bool stripComments,
                                                         bool stripLibFuncs)
{
    // opt obj
    QJsonObject optObj;
    optObj[QStringLiteral("userArguments")] = userArgs;

    // compiler options obj
    QJsonObject compilerObj;
    compilerObj[QStringLiteral("skipAsm")] = false;
    compilerObj[QStringLiteral("executorRequest")] = false;

    // add compileropts to opt obj
    optObj[QStringLiteral("compilerOptions")] = compilerObj;

    // filters
    QJsonObject filterObj;
    filterObj[QStringLiteral("binary")] = false;
    filterObj[QStringLiteral("commentOnly")] = stripComments;
    filterObj[QStringLiteral("demangle")] = demangle;
    filterObj[QStringLiteral("directives")] = true;
    filterObj[QStringLiteral("intel")] = isIntel;
    filterObj[QStringLiteral("labels")] = stripUnusedLabels;
    filterObj[QStringLiteral("execute")] = false;
    filterObj[QStringLiteral("libraryCode")] = stripLibFuncs;

    optObj[QStringLiteral("filters")] = filterObj;

    QJsonObject main;
    //    main["source"] = "int sum(){ return 2 + 2; }";
    main[QStringLiteral("source")] = source;
    main[QStringLiteral("options")] = optObj;

    QJsonDocument doc{main};
    return doc;
}
