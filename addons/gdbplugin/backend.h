/**
 * Description: Debugger backend selector
 *
 *  SPDX-FileCopyrightText: 2022 Héctor Mesa Jiménez <wmj.py@gmx.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */
#pragma once

#include <QObject>
#include <optional>

#include "backendinterface.h"
#include "configview.h"

class Backend final : public BackendInterface
{
    Q_OBJECT
public:
    Backend(QObject *parent);
    ~Backend();

    void runDebugger(const DAPTargetConf &conf, std::map<QUrl, QList<dap::SourceBreakpoint>>);

    bool debuggerRunning() const override;
    bool debuggerBusy() const override;
    bool supportsMovePC() const override;
    bool supportsRunToCursor() const override;
    bool canSetBreakpoints() const override;
    bool canMove() const override;
    bool canContinue() const override;
    void setBreakpoints(const QUrl &url, const QList<dap::SourceBreakpoint> &breakpoints) override;
    void movePC(QUrl const &url, int line) override;
    void runToCursor(QUrl const &url, int line) override;
    void issueCommand(QString const &cmd) override;
    QString targetName() const override;
    void setFileSearchPaths(const QStringList &paths) override;
    QList<dap::Module> modules();

    bool canHotReload() const;
    bool canHotRestart() const;

public Q_SLOTS:
    void slotInterrupt() override;
    void slotStepInto() override;
    void slotStepOver() override;
    void slotStepOut() override;
    void slotContinue() override;
    void slotKill() override;
    void slotReRun() override;
    QString slotPrintVariable(const QString &variable) override;
    void slotHotReload();
    void slotHotRestart();
    void slotQueryLocals(bool display) override;
    void changeStackFrame(int index) override;
    void changeThread(int thread) override;
    void changeScope(int scopeId) override;
    void requestVariable(int variablesReference) override;

private:
    void bind();
    void unbind();

    BackendInterface *m_debugger;
    std::optional<bool> m_displayQueryLocals = std::nullopt;
};
