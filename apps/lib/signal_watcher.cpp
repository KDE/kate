/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "signal_watcher.h"

#if defined(Q_OS_UNIX)

#include <QSocketNotifier>

#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

static int sigIntFd[2] = {0, 0};
static int sigTermFd[2] = {0, 0};

static void intSignalHandler(int)
{
    char a = 1;
    ::write(sigIntFd[0], &a, sizeof(a));
}

static void termSignalHandler(int)
{
    char a = 1;
    ::write(sigTermFd[0], &a, sizeof(a));
}

static bool setup_unix_signal_handlers()
{
    struct sigaction sigInt;
    struct sigaction term;

    sigInt.sa_handler = intSignalHandler;
    sigemptyset(&sigInt.sa_mask);
    sigInt.sa_flags = 0;
    sigInt.sa_flags |= SA_RESTART;

    if (sigaction(SIGINT, &sigInt, nullptr) != 0)
        return false;

    term.sa_handler = termSignalHandler;
    sigemptyset(&term.sa_mask);
    term.sa_flags = 0;
    term.sa_flags |= SA_RESTART;

    if (sigaction(SIGTERM, &term, nullptr) != 0)
        return false;

    return true;
}

SignalWatcher::SignalWatcher(QObject *parent)
    : QObject(parent)
{
    if (setup_unix_signal_handlers()) {
        if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sigIntFd))
            qWarning("Couldn't create INT socketpair");

        if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sigTermFd))
            qWarning("Couldn't create TERM socketpair");

        m_sigIntSocketNotifier = new QSocketNotifier(sigIntFd[1], QSocketNotifier::Read, this);
        connect(m_sigIntSocketNotifier, &QSocketNotifier::activated, this, &SignalWatcher::handleSigInt);
        m_sigIntSocketNotifier->setEnabled(true);

        m_sigTermSocketNotifier = new QSocketNotifier(sigTermFd[1], QSocketNotifier::Read, this);
        connect(m_sigTermSocketNotifier, &QSocketNotifier::activated, this, &SignalWatcher::handleSigTerm);
        m_sigTermSocketNotifier->setEnabled(true);
    } else {
        printf("Failed to setup signal handler\n");
    }
}

void SignalWatcher::handleSigInt()
{
    m_sigIntSocketNotifier->setEnabled(false);
    char tmp{};
    ::read(sigIntFd[1], &tmp, sizeof(tmp));

    Q_EMIT unixSignal(Signal_Interrupt);

    m_sigIntSocketNotifier->setEnabled(true);
}

void SignalWatcher::handleSigTerm()
{
    m_sigTermSocketNotifier->setEnabled(false);
    char tmp{};
    ::read(sigTermFd[1], &tmp, sizeof(tmp));

    Q_EMIT unixSignal(Signal_Terminate);

    m_sigTermSocketNotifier->setEnabled(true);
}

#endif
