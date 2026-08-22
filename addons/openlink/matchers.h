/*
    SPDX-FileCopyrightText: 2023 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <QFileInfo>
#include <QRegularExpression>

#include <KTextEditor/Cursor>

enum OpenLinkType {
    HttpLink,
    FileLink,
};

struct OpenLinkRange {
    int start = 0;
    int end = 0;
    QString link = {};
    KTextEditor::Cursor startPos = KTextEditor::Cursor::invalid();
    OpenLinkType type;
};

static const QRegularExpression &httplinkRE()
{
    static const QRegularExpression re(
        QStringLiteral(R"re((https?:\/\/(www\.)?[-a-zA-Z0-9@:%._\+~#=]{1,256}\.[a-zA-Z0-9()]{1,6}\b([-a-zA-Z0-9()@:%_\+.~#?&//=]*)))re"));
    return re;
}

static void adjustLink(QString &link)
{
    // fixup links like:
    // [ccc](https://cullmann.dev)
    // Visit 'https://cullmann.dev'
    // Visit "https://cullmann.dev"
    // The web site https://cullmann.dev.
    // <https://cullmann.dev>
    // (for [#3695](https://github.com/pbek/QOwnNotes/issues/3695))
    while (!link.isEmpty()) {
        const QChar last = link.back();
        if (last == u')') {
            // Only strip ')' if the parentheses are unbalanced so that
            // links with balanced parens like
            // https://en.wikipedia.org/wiki/Link_(film) stay intact
            if (link.count(u')') > link.count(u'(')) {
                link.chop(1);
                continue;
            }
            break;
        }
        if (last == u'\'' || last == u'"' || last == u'.' || last == u'>') {
            link.chop(1);
            continue;
        }
        break;
    }
}

static KTextEditor::Cursor parseLineCol(QStringView &link)
{
    int line = -1;
    int col = -1;
    if (link.last() == u':' || link.last().isDigit()) {
        // strip last colon
        if (link.last() == u':') {
            link = link.mid(0, link.size() - 1);
        }
        // find the last colon
        if (int colon = link.lastIndexOf(u':'); colon != -1) {
            int num1 = -1;
            bool num1_Ok = false;
            int num2 = -1;
            bool num2_Ok = false;

            // get the number
            num1 = link.mid(colon + 1).toInt(&num1_Ok);

            // is it ok?
            if (num1_Ok) {
                // adjust the link
                link = link.mid(0, colon);
                // try to find another colon
                colon = link.lastIndexOf(u':');
                // try to get the second number
                if (colon != -1) {
                    num2 = link.mid(colon + 1).toInt(&num2_Ok);
                    if (num2_Ok) {
                        link = link.mid(0, colon);
                    }
                }
            }

            if (num1_Ok && num2_Ok) {
                line = num2;
                col = num1;
            } else if (num1_Ok) {
                line = num1;
                col = 0;
            }
        }
    }
    return KTextEditor::Cursor(line, col);
}

static void pushLink(int s, int e, QStringView line, std::vector<OpenLinkRange> *outColumnRanges)
{
    QStringView linkView(QStringView(line).mid(s, e - s));
    KTextEditor::Cursor c = parseLineCol(linkView);
    QString link = linkView.toString();
    if (QFileInfo(link).isFile()) {
        outColumnRanges->push_back({.start = s, .end = e, .link = link, .startPos = c, .type = FileLink});
    }
}

static void matchFilePaths(const QString &line, std::vector<OpenLinkRange> *outColumnRanges)
{
    const auto isPrevCharAcceptable = [](QChar c) {
        return c == u' ' || c == u'"' || c == u'(' || c == u')' || c == u'=';
    };

#ifdef Q_OS_WIN
    const auto isValidDriveLetter = [](QChar letter) {
        return (letter.isLetter() && letter.toUpper() >= u'A' && letter.toUpper() <= u'Z');
    };

    int s = 0;
    while (true) {
        const int from = s;
        s = line.indexOf(u'\\', from);
        s = s != -1 ? s : line.indexOf(u'/', from);
        if (s < 0) {
            break;
        }

        const bool isAbsoloutePath = s >= 2 && line[s - 1] == u':' && isValidDriveLetter(line[s - 2]);
        const bool isRelativePath = s > 1 && line[s - 1] == u'.';
        if (isAbsoloutePath || isRelativePath) {
            const bool isDotDotRelativePath = isRelativePath && s > 2 && line[s - 2] == u'.';
            // move s back to actual start position
            const int orignalS = s;
            s = isAbsoloutePath ? s - 2 : s - 1;
            s = isDotDotRelativePath ? s - 1 : s;

            // must be preceded by a space or a symbol
            if (s != 0 && !isPrevCharAcceptable(line[s - 1])) {
                s = orignalS + 1;
                continue;
            }

            const bool matchNextQuote = s > 0 && line[s - 1] == u'"'; // last char is quote?
            int e = -1;
            if (!matchNextQuote) {
                e = line.indexOf(QLatin1String(" "), s);
                e = e == -1 ? line.size() : e;

                // Strip trailing punctuation
                while (e > s && (line[e - 1] == u',' || line[e - 1] == u'.')) {
                    e--;
                }
            } else {
                e = line.indexOf(u'"', s);
                if (e == -1) {
                    break;
                }
            }

            if (isRelativePath) {
                // TODO support relative paths
                s = e;
                continue;
            }

            if (e != -1) {
                pushLink(s, e, line, outColumnRanges);
            }
            s = e;
        } else {
            s++;
        }
        // try find relative path
    }
#else
    int s = 0;
    while (true) {
        s = line.indexOf(u'/', s);
        if (s == -1) {
            break;
        }
        // must be preceded by a space or a symbol
        if (s != 0 && !isPrevCharAcceptable(line[s - 1])) {
            s++;
            continue;
        }
        const bool matchNextQuote = s > 0 && line[s - 1] == u'"'; // last char is quote?
        int e = -1;
        if (!matchNextQuote) {
            e = line.indexOf(QLatin1String(" "), s);
            e = e == -1 ? line.size() : e;

            // Strip trailing punctuation
            while (e > s && (line[e - 1] == u',' || line[e - 1] == u'.')) {
                e--;
            }
        } else {
            e = line.indexOf(u'"', s);
            if (e == -1) {
                break;
            }
        }

        if (e != -1) {
            pushLink(s, e, line, outColumnRanges);
        }
        s = e;
    }
#endif
}

[[maybe_unused]] static void matchLine(const QString &line, std::vector<OpenLinkRange> *outColumnRanges)
{
    outColumnRanges->clear();
    if (line.contains(QLatin1String("http://")) || line.contains(QLatin1String("https://"))) {
        QRegularExpressionMatchIterator it = httplinkRE().globalMatch(line);
        while (it.hasNext()) {
            auto match = it.next();
            if (match.hasMatch()) {
                QString link = match.captured();
                adjustLink(link);
                if (!link.isEmpty()) {
                    outColumnRanges->push_back(
                        {.start = int(match.capturedStart()), .end = int(match.capturedStart() + link.size()), .link = link, .type = HttpLink});
                }
            }
        }
    }

    matchFilePaths(line, outColumnRanges);
}
