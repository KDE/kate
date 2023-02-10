#pragma once

#include <QTabWidget>

/**
 * A wrapper over QTermWidget which
 * tries to provide the same interace as
 * Konsole::TerminalInterface
 */
class KateTerminalWidget : public QTabWidget
{
    Q_OBJECT
public:
    KateTerminalWidget(QWidget *parent = nullptr);
    ~KateTerminalWidget() override;

    void showShellInDir(const QString &dir);

    void sendInput(const QString &text);

    // Will return empty string
    QString foregroundProcessName() const;

public Q_SLOTS:
    void createSession(const QString &profile, const QString &workingDir);

private:
    void newTab(const QString &workingDir);
};
