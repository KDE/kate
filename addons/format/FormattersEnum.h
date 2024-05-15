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

    // keep at end
    COUNT,
    FIRST = ClangFormat,
};

inline Formatters formatterForName(const QString &name, Formatters defaultValue)
{
    auto eq = [&](const char *s) {
        return name.compare(QLatin1String(s), Qt::CaseInsensitive) == 0;
    };
    if (eq("clangformat") || eq("clang-format")) {
        return Formatters::ClangFormat;
    }
    if (eq("prettier")) {
        return Formatters::Prettier;
    }
    if (eq("jq")) {
        return Formatters::Jq;
    }
    if (eq("xmllint")) {
        return Formatters::XmlLint;
    }
    if (eq("autopep8")) {
        return Formatters::Autopep8;
    }
    if (eq("ruff")) {
        return Formatters::Ruff;
    }
    return defaultValue;
}
