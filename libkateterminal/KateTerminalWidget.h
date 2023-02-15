/*
    SPDX-FileCopyrightText: 2023 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include <QTabWidget>

#include "kateterm_export.h"

/**
 * A wrapper over QTermWidget which
 * tries to provide the same interace as
 * Konsole::TerminalInterface
 */
class KATE_TERMINAL_EXPORT KateTerminalWidget : public QTabWidget
{
    Q_OBJECT
public:
    KateTerminalWidget(QWidget *parent = nullptr);
    ~KateTerminalWidget() override;

    void showShellInDir(const QString &dir);

    void sendInput(const QString &text);

    // Will return empty string
    QString foregroundProcessName() const;

    static bool isAvailable();

    QString currentWorkingDirectory() const;

public Q_SLOTS:
    void createSession(const QString &profile, const QString &workingDir);

private:
    void tabRemoved(int) override;
    void newTab(const QString &workingDir);

Q_SIGNALS:
    void overrideShortcutCheck(QKeyEvent *keyEvent, bool &overridee);
};
