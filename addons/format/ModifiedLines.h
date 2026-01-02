/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include "hostprocess.h"
#include <gitprocess.h>

#include <QFileInfo>
#include <QProcess>
#include <QString>
#include <QtCore/qlogging.h>

#include <charconv>
#include <optional>
#include <vector>

struct ModifiedLines {
    int startLine;
    int endline;
};

namespace __internal
{
static inline auto parseRange(std::string_view sv) -> std::optional<ModifiedLines>
{
    if (sv.empty()) {
        return {};
    }
    if (sv[0] == '+') {
        sv.remove_prefix(1);
    }

    const std::size_t pos = sv.find(',');
    if (pos == std::string_view::npos) {
        // if comma not found, means line count == 1
        int s = 0;
        auto res = std::from_chars(sv.data(), sv.data() + sv.size(), s);
        if (res.ptr != (sv.data() + sv.size())) {
            return {};
        }
        return ModifiedLines(s, s);
    }

    int s = 0;
    auto res = std::from_chars(sv.data(), sv.data() + pos, s);
    if (res.ptr != (sv.data() + pos)) {
        return {};
    }
    int c = 0;
    res = std::from_chars(sv.data() + pos + 1, sv.data() + sv.size(), c);
    if (res.ptr != (sv.data() + sv.size())) {
        return {};
    }
    return ModifiedLines(s, s + c);
}

static inline auto modifedLinesFromGitDiff(const QByteArray &out) -> std::optional<std::vector<ModifiedLines>>
{
    int start = out.indexOf("@@ ");
    if (start == -1) {
        return std::nullopt;
    }
    start += 3; // skip @@
    int next = 0;
    std::vector<ModifiedLines> ret;
    while (start > -1) {
        next = out.indexOf(" @@", start);
        if (next == -1) {
            return std::nullopt;
        }
        int space = out.indexOf(' ', start);
        if (space == -1) {
            return std::nullopt;
        }
        space++;
        std::string_view sv(out.constData() + space, next - space);
        const auto res = parseRange(sv);
        if (!res.has_value()) {
            return std::nullopt;
        }
        const auto [startLine, endLine] = res.value();
        ret.push_back({startLine, endLine});

        start = next + 3;
        start = out.indexOf("@@ ", start);
    }
    return ret;
}
}

static inline std::optional<std::vector<ModifiedLines>> getModifiedLines(const QString &filePath)
{
    if (!QFile::exists(filePath)) {
        qWarning("Doc doesn't exist, shouldn't happen!");
        return std::nullopt;
    }

    QProcess git;
    setupGitProcess(git, QFileInfo(filePath).absolutePath(), {QStringLiteral("diff"), QStringLiteral("--no-color"), QStringLiteral("-U0"), filePath});
    startHostProcess(git, QProcess::ReadOnly);
    if (!git.waitForStarted() || !git.waitForFinished()) {
        return std::nullopt;
    }
    if (git.exitCode() != 0 || git.exitStatus() != QProcess::NormalExit) {
        return std::nullopt;
    }
    return __internal::modifedLinesFromGitDiff(git.readAllStandardOutput());
}
