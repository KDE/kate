//
// debugview.h
//
// Description: Manages the interaction with GDB
//
//
// SPDX-FileCopyrightText: 2008-2010 Ian Wakeling <ian.wakeling@ntlworld.com>
// SPDX-FileCopyrightText: 2010 Kåre Särs <kare.sars@iki.fi>
//
//  SPDX-License-Identifier: LGPL-2.0-only

#ifndef DEBUGVIEW_H
#define DEBUGVIEW_H

#include <QObject>

#include <QProcess>
#include <QUrl>

#include "configview.h"
#include "debugview_iface.h"
#include "gdbvariableparser.h"

class DebugView : public DebugViewInterface
{
    Q_OBJECT
public:
    DebugView(QObject *parent);
    ~DebugView() override;

    void runDebugger(const GDBTargetConf &conf, const QStringList &ioFifos);
    bool debuggerRunning() const override;
    bool debuggerBusy() const override;
    bool hasBreakpoint(QUrl const &url, int line) const override;

    bool supportsMovePC() const override;
    bool supportsRunToCursor() const override;
    bool canSetBreakpoints() const override;
    bool canMove() const override;
    bool canContinue() const override;

    void toggleBreakpoint(QUrl const &url, int line) override;
    void movePC(QUrl const &url, int line) override;
    void runToCursor(QUrl const &url, int line) override;

    void issueCommand(QString const &cmd) override;

    QString targetName() const override;
    void setFileSearchPaths(const QStringList &paths) override;

public Q_SLOTS:
    void slotInterrupt() override;
    void slotStepInto() override;
    void slotStepOver() override;
    void slotStepOut() override;
    void slotContinue() override;
    void slotKill() override;
    void slotReRun() override;
    QString slotPrintVariable(const QString &variable) override;

    void slotQueryLocals(bool display) override;
    void changeStackFrame(int index) override;
    void changeThread(int index) override;
    void changeScope(int scopeId) override;

private Q_SLOTS:
    static void slotError();
    void slotReadDebugStdOut();
    void slotReadDebugStdErr();
    void slotDebugFinished(int exitCode, QProcess::ExitStatus status);
    void issueNextCommand();

private:
    enum State { none, ready, executingCmd, listingBreakpoints, infoStack, infoArgs, printThis, infoLocals, infoThreads };

    enum SubState { normal, stackFrameSeen, stackTraceSeen };

    struct BreakPoint {
        int number;
        QUrl file;
        int line;
    };

private:
    void processLine(QString output);
    void processErrors();
    void outputTextMaybe(const QString &text);
    QUrl resolveFileName(const QString &fileName);

private:
    QProcess m_debugProcess;
    GDBTargetConf m_targetConf;
    QString m_ioPipeString;

    State m_state;
    SubState m_subState;

    QString m_currentFile;
    QString m_newFrameFile;
    int m_newFrameLevel = 0;
    QStringList m_nextCommands;
    QString m_lastCommand;
    bool m_debugLocationChanged;
    QList<BreakPoint> m_breakPointList;
    QString m_outBuffer;
    QString m_errBuffer;
    QStringList m_errorList;
    bool m_queryLocals;

    GDBVariableParser m_variableParser;
};

#endif
