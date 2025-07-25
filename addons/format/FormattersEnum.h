/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include <QString>

enum class Formatters {
    // Not all formatters that we support need to be specified here.
    // Only those that languages that have multiple formatters e.g., json
    // should be specified
    ClangFormat = 0,
    Prettier,
    Jq,
    XmlLint,
    Autopep8,
    Ruff,
    YamlFmt,

    // keep at end
    COUNT,
    FIRST = ClangFormat,
};

inline Formatters formatterForName(const QString &name, Formatters defaultValue)
{
    struct NameToFormatter {
        const char *name;
        Formatters formatter;
    };
    static const NameToFormatter strToFmt[] = {
        {"clang-format", Formatters::ClangFormat},
        {"clangformat", Formatters::ClangFormat},
        {"prettier", Formatters::Prettier},
        {"jq", Formatters::Jq},
        {"xmllint", Formatters::XmlLint},
        {"autopep8", Formatters::Autopep8},
        {"ruff", Formatters::Ruff},
        {"yamlfmt", Formatters::YamlFmt},
    };

    for (const auto &[fmtName, enumValue] : strToFmt) {
        if (name.compare(QLatin1String(fmtName), Qt::CaseInsensitive) == 0) {
            return enumValue;
        }
    }
    return defaultValue;
}
