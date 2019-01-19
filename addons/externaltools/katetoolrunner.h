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

#ifndef KTEXTEDITOR_EXTERNALTOOLRUNNER_H
#define KTEXTEDITOR_EXTERNALTOOLRUNNER_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QProcess>

class KateExternalTool;
class QProcess;

/**
 * Helper class to run a KateExternalTool.
 */
class KateToolRunner : public QObject
{
public:
    KateToolRunner(KateExternalTool * tool);
    KateToolRunner(const KateToolRunner &) = delete;
    void operator=(const KateToolRunner &) = delete;

    ~KateToolRunner();

    void run();
    void waitForFinished();
    QString outputData() const;

private Q_SLOTS:
    /**
     * More tool output is available
     */
    void slotReadyRead();

    /**
     * Analysis finished
     * @param exitCode analyzer process exit code
     * @param exitStatus analyzer process exit status
     */
    void toolFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    KateExternalTool * m_tool;
    QProcess * m_process = nullptr;
    QByteArray m_output;
};

#endif // KTEXTEDITOR_EXTERNALTOOLRUNNER_H

// kate: space-indent on; indent-width 4; replace-tabs on;
