/*
    SPDX-FileCopyrightText: 2022 Héctor Mesa Jiménez <wmj.py@gmx.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include <QString>
#include <QTextCodec>

#include "dapclient_debug.h"
#include "processbus.h"
#include "settings.h"

namespace dap
{
ProcessBus::ProcessBus(QObject *parent)
    : Bus(parent)
{
    // IO
    connect(&process, &QProcess::readyReadStandardOutput, this, &Bus::readyRead);
    connect(&process, &QProcess::stateChanged, this, &ProcessBus::onStateChanged);

    // state/error
    connect(&process, &QProcess::errorOccurred, this, &ProcessBus::onError);
    connect(&process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &ProcessBus::onFinished);
    connect(&process, &QProcess::readyReadStandardError, this, &ProcessBus::readError);
}

ProcessBus::~ProcessBus()
{
    blockSignals(true);
    bool finished = process.state() == QProcess::NotRunning;
    if (!finished) {
        process.terminate();
        finished = process.waitForFinished(500);
    }
    if (!finished) {
        process.kill();
        process.waitForFinished(300);
    }
}

QByteArray ProcessBus::read()
{
    return process.readAllStandardOutput();
}

quint16 ProcessBus::write(const QByteArray &data)
{
    return process.write(data);
}

bool ProcessBus::start(const settings::BusSettings &configuration)
{
    if (!configuration.hasCommand())
        return false;

    configuration.command->start(process);

    return true;
}

void ProcessBus::close()
{
    if (process.state() != QProcess::NotRunning) {
        if (m_tryClose == None) {
            m_tryClose = Terminate;
            process.terminate();
        } else if (m_tryClose == None) {
            m_tryClose = Kill;
            process.kill();
        } else {
            process.waitForFinished(500);
        }
    }
    setState(State::Closed);
}

void ProcessBus::onStateChanged(QProcess::ProcessState state)
{
    // starting
    //  running
    //    notrunning
    //  notrunning

    switch (state) {
    case QProcess::ProcessState::NotRunning:
        setState(State::Closed);
        break;
    case QProcess::ProcessState::Running:
        setState(State::Running);
        break;
    default:;
    }
}

void ProcessBus::onError(QProcess::ProcessError processError)
{
    qCWarning(DAPCLIENT) << "PROCESS ERROR: " << processError << " (" << process.errorString() << ")";
    Q_EMIT error(process.errorString());
}

void ProcessBus::onFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::ExitStatus::CrashExit) {
        qCWarning(DAPCLIENT) << "ABNORMAL PROCESS EXIT: code " << exitCode;
        Q_EMIT error(QStringLiteral("process exited with code %1").arg(exitCode));
    }
}

void ProcessBus::readError()
{
    const auto &message = process.readAllStandardError();
    // process' standard error
    qCDebug(DAPCLIENT) << "[BUS] STDERR << " << message;

    Q_EMIT serverOutput(QTextCodec::codecForLocale()->toUnicode(message));
}

}
