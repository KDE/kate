/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef KATE_SIG_WATCHER_H
#define KATE_SIG_WATCHER_H

#include <QObject>

class SignalWatcher : public QObject
{
    Q_OBJECT

#if defined(Q_OS_UNIX)
public:
    explicit SignalWatcher(QObject *parent = nullptr);
    ~SignalWatcher() = default;

    void watchForSignal(int signal);

    enum Signal { Signal_Interrupt = 0, Signal_Terminate };

Q_SIGNALS:
    void unixSignal(Signal signal);

private Q_SLOTS:
    void handleSigInt();
    void handleSigTerm();

private:
    class QSocketNotifier *m_sigIntSocketNotifier;
    class QSocketNotifier *m_sigTermSocketNotifier;
#endif // defined(Q_OS_UNIX)
};

#endif
