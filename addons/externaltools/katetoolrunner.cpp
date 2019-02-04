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

#include <KShell>
#include <KTextEditor/View>
#include <KLocalizedString>

KateToolRunner::KateToolRunner(std::unique_ptr<KateExternalTool> tool, KTextEditor::View * view, QObject* parent)
    : QObject(parent)
    , m_view(view)
    , m_tool(std::move(tool))
    , m_process(new QProcess())
{
}

KateToolRunner::~KateToolRunner()
{
}

KTextEditor::View* KateToolRunner::view() const
{
    return m_view;
}

KateExternalTool* KateToolRunner::tool() const
{
    return m_tool.get();
}

void KateToolRunner::run()
{
    if (m_tool->includeStderr) {
        m_process->setProcessChannelMode(QProcess::MergedChannels);
    }

    if (!m_tool->workingDir.isEmpty()) {
        m_process->setWorkingDirectory(m_tool->workingDir);
    } else {
        // if nothing is set, use the current document's directory
        const QString path = m_view->document()->url().toString(QUrl::RemoveScheme | QUrl::RemoveFilename);
        m_process->setWorkingDirectory(path);
    }

    QObject::connect(m_process.get(), &QProcess::readyRead, this, &KateToolRunner::slotReadyRead);
    QObject::connect(m_process.get(), static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this,
                     &KateToolRunner::handleToolFinished);

    // Write stdin to process, if applicable, then close write channel
    QObject::connect(m_process.get(), &QProcess::started, [this]() {
        if (!m_tool->input.isEmpty()) {
            m_process->write(m_tool->input.toLocal8Bit());
        }
        m_process->closeWriteChannel();
    });

    const QStringList args = KShell::splitArgs(m_tool->arguments);
    m_process->start(m_tool->executable, args);
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

void KateToolRunner::handleToolFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_EMIT toolFinished(this, exitCode, exitStatus == QProcess::CrashExit);
}

// kate: space-indent on; indent-width 4; replace-tabs on;
