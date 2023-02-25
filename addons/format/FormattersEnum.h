/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include <QString>

enum class Formatters {
    Invalid = -1,
    ClangFormat = 0,
    DartFmt,
    Prettier,
    Jq,
    RustFmt,
    XmlLint,
    GoFmt,
    ZigFmt,
};

inline Formatters formatterForName(const QString &name)
{
    auto eq = [&](const char *s) {
        return name.compare(QLatin1String(s), Qt::CaseInsensitive) == 0;
    };
    if (eq("clangformat") || eq("clang-format")) {
        return Formatters::ClangFormat;
    }
    if (eq("dart") || eq("dartfmt")) {
        return Formatters::DartFmt;
    }
    if (eq("prettier")) {
        return Formatters::Prettier;
    }
    if (eq("jq")) {
        return Formatters::Jq;
    }
    if (eq("rustfmt")) {
        return Formatters::RustFmt;
    }
    if (eq("xmllint")) {
        return Formatters::XmlLint;
    }
    if (eq("gofmt")) {
        return Formatters::GoFmt;
    }
    if (eq("zig") || eq("zigfmt")) {
        return Formatters::ZigFmt;
    }
    return Formatters::Invalid;
}
