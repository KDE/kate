/* This file is part of the KDE project
 *
 *  Copyright 2019 Dominik Haumann <dhaumann@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "katetoolrunner.h"

#include "kateexternaltool.h"

KateToolRunner::KateToolRunner(KateExternalTool * tool)
    : m_tool(tool)
    , m_process(new QProcess())
{
}

KateToolRunner::~KateToolRunner()
{
    delete m_process;
    m_process = nullptr;
}

void KateToolRunner::run()
{
    m_process->setProcessChannelMode(QProcess::MergedChannels);

    QObject::connect(m_process, &QProcess::readyRead, this, &KateToolRunner::slotReadyRead);
    QObject::connect(m_process, static_cast<void(QProcess::*)(int,QProcess::ExitStatus)>(&QProcess::finished), this, &KateToolRunner::toolFinished);

    m_process->start(m_tool->executable, { m_tool->arguments });
}

void KateToolRunner::waitForFinished()
{
    m_process->waitForFinished();
}


QString KateToolRunner::outputData() const
{
    return QString::fromLocal8Bit(m_output);
}

void KateToolRunner::slotReadyRead()
{
    m_output += m_process->readAll();
}

void KateToolRunner::toolFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitCode != 0) {
        // FIXME: somehow tell user
        return;
    }

    if (exitStatus != QProcess::NormalExit) {
        // FIXME: somehow tell user
        return;
    }

    // FIXME: process m_output depending on the tool's outputMode
}

// kate: space-indent on; indent-width 4; replace-tabs on;
