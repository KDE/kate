#pragma once

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
