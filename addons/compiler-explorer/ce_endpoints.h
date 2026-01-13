#pragma once

#include <QHash>

namespace CompilerExplorer
{
enum Endpoints {
    Languages,
    Compilers,
    CompilerCompile,
};

inline const QHash<Endpoints, QString> &endpointsToString()
{
    static const QHash<Endpoints, QString> endpointsToStr = {
        {Endpoints::Languages, QStringLiteral("languages")},
        {Endpoints::Compilers, QStringLiteral("compilers")},
        {Endpoints::CompilerCompile, QStringLiteral("compiler")},
    };
    return endpointsToStr;
}

inline const QHash<QString, Endpoints> &stringToEndpoint()
{
    static const QHash<QString, Endpoints> strToEndpoint = {
        {QStringLiteral("languages"), Endpoints::Languages},
        {QStringLiteral("compilers"), Endpoints::Compilers},
        {QStringLiteral("compiler"), Endpoints::CompilerCompile},
    };
    return strToEndpoint;
}
}
