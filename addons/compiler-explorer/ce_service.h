#ifndef KATE_CE_SVC_H
#define KATE_CE_SVC_H

#include "ce_endpoints.h"
#include <QObject>
#include <QtNetwork/QNetworkAccessManager>

class CompilerExplorerSvc : public QObject
{
    Q_OBJECT
public:
    static CompilerExplorerSvc *instance();
    void sendRequest(CompilerExplorer::Endpoints endpoint, const QString &additional = QString());
    void compileRequest(const QString &endpoint, const QByteArray &obj);
    QNetworkReply *tooltipRequest(const QString &asmWord);

    ~CompilerExplorerSvc() override;

    static QJsonDocument getCompilationOptions(const QString &source,
                                               const QString &userArgs,
                                               bool isIntel,
                                               bool demangle,
                                               bool stripUnusedLabels,
                                               bool stripComments,
                                               bool stripLibFuncs);

    void changeUrl(const QString &newUrl);

private Q_SLOTS:
    void slotNetworkReply(QNetworkReply *reply);

Q_SIGNALS:
    void languages(const QByteArray &data);
    void compilers(const QByteArray &data);
    void asmResult(const QByteArray &data);

private:
    CompilerExplorerSvc(QObject *parent = nullptr);
    QNetworkAccessManager *mgr;
    QString url;
};

#endif // KATE_CE_SVC_H
