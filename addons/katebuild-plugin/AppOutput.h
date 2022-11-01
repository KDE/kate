// SPDX-FileCopyrightText: 2022 Kåre Särs <kare.sars@iki.fi>
//
//  SPDX-License-Identifier: LGPL-2.0-only

#ifndef AppOutput_H
#define AppOutput_H

#include <QString>
#include <QWidget>

/** This widget provides terminal (where konsolepart is available and
 * plain buffered stdout as a fallback.
 */
class AppOutput : public QWidget
{
    Q_OBJECT

public:
    AppOutput(QWidget *parent = nullptr);
    ~AppOutput(); // This one is needed for the std::unique_ptr

    void setWorkingDir(const QString &path);
    void runCommand(const QString &cmd);
    QString runningProcess();

Q_SIGNALS:
    void runningChanhged();

private:
    struct Private;
    friend struct Private;
    std::unique_ptr<Private> d;
};

#endif
