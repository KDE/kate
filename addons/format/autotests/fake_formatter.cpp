/*
    SPDX-FileCopyrightText: 2026 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

// A fake formatter for the tests, a binary to run on all platforms without a shell.
// It copies its input to its output, optionally only the first lines, then exits.
//
// Usage: fake_formatter [--lines N] [--exit CODE | --crash]
//   --lines N    only copy the first N lines
//   --exit CODE  exit with CODE
//   --crash      crash (abort) at the end

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFile>

#include <cstdlib>

#ifdef _MSC_VER
#include <stdlib.h>
#endif

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QCommandLineParser parser;
    const QCommandLineOption linesOption(QStringLiteral("lines"), QString(), QStringLiteral("N"));
    const QCommandLineOption exitOption(QStringLiteral("exit"), QString(), QStringLiteral("CODE"));
    const QCommandLineOption crashOption(QStringLiteral("crash"));
    parser.addOptions({linesOption, exitOption, crashOption});
    parser.process(app);

    QFile in;
    QFile out;
    if (!in.open(stdin, QIODevice::ReadOnly) || !out.open(stdout, QIODevice::WriteOnly)) {
        return 1;
    }

    const int lines = parser.isSet(linesOption) ? parser.value(linesOption).toInt() : -1;
    for (int n = 0; lines < 0 || n < lines; ++n) {
        const QByteArray line = in.readLine();
        if (line.isEmpty()) {
            break;
        }
        out.write(line);
    }
    out.flush();

    if (parser.isSet(crashOption)) {
#ifdef _MSC_VER
        // no "abort() has been called" dialog in debug builds
        _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif
        std::abort();
    }
    return parser.value(exitOption).toInt();
}
