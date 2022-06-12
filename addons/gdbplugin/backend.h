/**
 * Description: Debugger backend selector
 *
 *  SPDX-FileCopyrightText: 2022 Héctor Mesa Jiménez <wmj.py@gmx.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */
#ifndef BACKEND_H_
#define BACKEND_H_

#include <QObject>
#include <memory>
#include <optional>

#include "configview.h"
#include "debugview_iface.h"

class Backend : public DebugViewInterface
{
    Q_OBJECT
public:
    Backend(QObject *parent);
    ~Backend() override = default;

    void runDebugger(const GDBTargetConf &conf, const QStringList &ioFifos);
    void runDebugger(const DAPTargetConf &conf);

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
    void changeThread(int thread) override;
    void changeScope(int scopeId) override;

private:
    enum DebugMode { NONE, GDB, DAP } m_mode = NONE;

    void bind();
    void unbind();

    DebugViewInterface *m_debugger;
    std::optional<bool> m_displayQueryLocals = std::nullopt;
};

#endif
