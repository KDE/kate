/*
    SPDX-FileCopyrightText: 2023 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <QFileInfo>
#include <QRegularExpression>

enum OpenLinkType {
    HttpLink,
    FileLink,
};

struct OpenLinkRange {
    int start = 0;
    int end = 0;
    OpenLinkType type;
};

static const QRegularExpression &httplinkRE()
{
    static const QRegularExpression re(
        QStringLiteral(R"re((https?:\/\/(www\.)?[-a-zA-Z0-9@:%._\+~#=]{1,256}\.[a-zA-Z0-9()]{1,6}\b([-a-zA-Z0-9()@:%_\+.~#?&//=]*)))re"));
    return re;
}

static void adjustMDLink(const QString &line, int capturedStart, int &capturedEnd)
{
    if (capturedStart > 1) { // at least two chars before
        int i = capturedStart - 1;
        // for markdown [asd](google.com) style urls, make sure to strip last `)`
        bool isMD = line.at(i - 1) == QLatin1Char(']') && line.at(i) == QLatin1Char('(');
        if (isMD) {
            int f = line.lastIndexOf(QLatin1Char(')'), capturedEnd >= line.size() ? line.size() - 1 : capturedEnd);
            capturedEnd = f != -1 ? f : capturedEnd;
        }
    }
}

static std::pair<int, int> matchesFilePath(const QString &line, int from)
{
    int s = line.indexOf(QLatin1Char('/'), from);
    if (s != -1) {
        const bool matchNextQuote = s > 0 && line[s - 1] == u'"'; // last char is quote?
        int e = -1;
        if (!matchNextQuote) {
            e = line.indexOf(QLatin1String(" "), s);
            e = e == -1 ? line.size() : e;
        } else {
            e = line.indexOf(u'"', s);
        }
        if (e != -1 && QFileInfo(line.mid(s, e - s)).isFile()) {
            return {s, e};
        }
    }
    return {-1, -1};
}

[[maybe_unused]] static void matchLine(const QString &line, std::vector<OpenLinkRange> *outColumnRanges)
{
    outColumnRanges->clear();
    if (line.contains(QLatin1String("http://")) || line.contains(QLatin1String("https://"))) {
        QRegularExpressionMatchIterator it = httplinkRE().globalMatch(line);
        while (it.hasNext()) {
            auto match = it.next();
            if (match.hasMatch()) {
                int capturedEnd = match.capturedEnd();
                adjustMDLink(line, match.capturedStart(), capturedEnd);
                outColumnRanges->push_back({.start = (int)match.capturedStart(), .end = capturedEnd, .type = HttpLink});
            }
        }
    }

    if (auto [s, e] = matchesFilePath(line, 0); s >= 0 && e >= 0) {
        outColumnRanges->push_back({.start = s, .end = e, .type = FileLink});
        while (true) {
            auto [nextstart, nextend] = matchesFilePath(line, e + 1);
            if (nextstart >= 0 && nextend >= 0) {
                outColumnRanges->push_back({.start = nextstart, .end = nextend, .type = FileLink});
                e = nextend;
            } else {
                break;
            }
        }
    }
}
