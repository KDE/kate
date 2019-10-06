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

#include <QByteArray>
#include <QObject>
#include <QPointer>
#include <QProcess>
#include <QString>

#include <memory>

class KateExternalTool;
class QProcess;
namespace KTextEditor
{
class View;
}

/**
 * Helper class to run a KateExternalTool.
 */
class KateToolRunner : public QObject
{
    Q_OBJECT

public:
    /**
     * Constructor that will run @p tool in the run() method.
     * The @p view can later be retrieved again with view() to process the data when the tool is finished.
     */
    KateToolRunner(std::unique_ptr<KateExternalTool> tool, KTextEditor::View *view, QObject *parent = nullptr);

    KateToolRunner(const KateToolRunner &) = delete;
    void operator=(const KateToolRunner &) = delete;

    ~KateToolRunner();

    /**
     * Returns the view that was active when running the tool.
     * @warning May be a nullptr, since the view could have been closed in the meantime.
     */
    KTextEditor::View *view() const;

    /**
     * Returns the tool that was passed in the constructor.
     */
    KateExternalTool *tool() const;

    /**
     * Starts a child process that executes the tool.
     */
    void run();

    /**
     * Blocking call that waits until the tool is finised.
     * Used internally for unit testing.
     */
    void waitForFinished();

    /**
     * Returns the data that was collected on stdout.
     */
    QString outputData() const;

    /**
     * Returns the data that was collected on stderr.
     */
    QString errorData() const;

Q_SIGNALS:
    /**
     * This signal is emitted when the tool is finished.
     */
    void toolFinished(KateToolRunner *runner, int exitCode, bool crashed);

private:
    //! Use QPointer here, since the View may be closed in the meantime.
    QPointer<KTextEditor::View> m_view;

    //! We are the owner of the tool (it was copied)
    std::unique_ptr<KateExternalTool> m_tool;

    //! Child process that runs the tool
    std::unique_ptr<QProcess> m_process;

    //! Collect stdout
    QByteArray m_stdout;

    //! Collect stderr
    QByteArray m_stderr;
};

#endif // KTEXTEDITOR_EXTERNALTOOLRUNNER_H

// kate: space-indent on; indent-width 4; replace-tabs on;
