#ifndef PUSHPULLDIALOG_H
#define PUSHPULLDIALOG_H

#include "quickdialog.h"

class PushPullDialog : public QuickDialog
{
    Q_OBJECT
public:
    PushPullDialog(QWidget *mainWindow, const QString &repo);

    enum Mode { Push, Pull };
    void openDialog(Mode m);

    Q_SIGNAL void runGitCommand(const QStringList &args);

private:
    QString buildPushString();
    QString buildPullString();

    QString m_repo;

protected Q_SLOTS:
    void slotReturnPressed() override;
};

#endif // PUSHPULLDIALOG_H
