/*
    SPDX-FileCopyrightText: 2026 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

// A fake formatter for the tests, no dependencies to run on all platforms.
// It copies its input to its output, optionally only the first lines, then exits.
//
// Usage: fake_formatter [--lines N] [--exit CODE | --crash]
//   --lines N    only copy the first N lines
//   --exit CODE  exit with CODE
//   --crash      crash (abort) at the end

#include <cstdlib>
#include <iostream>
#include <string>

#ifdef _MSC_VER
#include <stdlib.h>
#endif

int main(int argc, char **argv)
{
    long lines = -1;
    int exitCode = 0;
    bool crash = false;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--lines" && i + 1 < argc) {
            lines = std::strtol(argv[++i], nullptr, 10);
        } else if (arg == "--exit" && i + 1 < argc) {
            exitCode = int(std::strtol(argv[++i], nullptr, 10));
        } else if (arg == "--crash") {
            crash = true;
        }
    }

    std::string line;
    for (long n = 0; (lines < 0 || n < lines) && std::getline(std::cin, line); ++n) {
        std::cout << line << '\n';
    }
    std::cout.flush();

    if (crash) {
#ifdef _MSC_VER
        // no "abort() has been called" dialog in debug builds
        _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif
        std::abort();
    }
    return exitCode;
}
