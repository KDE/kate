#ifndef KATE_CE_ENDPPOINTS_H
#define KATE_CE_ENDPPOINTS_H

#include <QHash>

namespace CompilerExplorer
{
enum Endpoints {
    Languages,
    Compilers,
    CompilerCompile,
};

static const QHash<Endpoints, QString> endpointsToString = {
    {Endpoints::Languages, QStringLiteral("languages")},
    {Endpoints::Compilers, QStringLiteral("compilers")},
    {Endpoints::CompilerCompile, QStringLiteral("compiler")},
};

static const QHash<QString, Endpoints> stringToEndpoint = {
    {QStringLiteral("languages"), Endpoints::Languages},
    {QStringLiteral("compilers"), Endpoints::Compilers},
    {QStringLiteral("compiler"), Endpoints::CompilerCompile},
};

}

#endif // KATE_CE_ENDPPOINTS_H
