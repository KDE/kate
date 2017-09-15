//
// ioview.h
//
// Description: Widget that interacts with the debugged application
//
//
// Copyright (c) 2010 Kåre Särs <kare.sars@iki.fi>
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Library General Public
//  License version 2 as published by the Free Software Foundation.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Library General Public License for more details.
//
//  You should have received a copy of the GNU Library General Public License
//  along with this library; see the file COPYING.LIB.  If not, write to
//  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
//  Boston, MA 02110-1301, USA.

#ifndef IOVIEW_H
#define IOVIEW_H

#include <QWidget>
#include <QFile>

class QTextEdit;
class QLineEdit;
class QSocketNotifier;

class IOView : public QWidget
{
Q_OBJECT
public:
    IOView(QWidget *parent = nullptr);
    ~IOView() override;

    const QString stdinFifo();
    const QString stdoutFifo();
    const QString stderrFifo();

    void enableInput(bool enable);

    void clearOutput();

public Q_SLOTS:
    void addStdOutText(const QString &text);
    void addStdErrText(const QString &text);

private Q_SLOTS:
    void returnPressed();
    void readOutput();
    void readErrors();

Q_SIGNALS:
    void stdOutText(const QString &text);
    void stdErrText(const QString &text);

private:
    void createFifos();
    QString createFifo(const QString &prefix);

    QTextEdit       *m_output;
    QLineEdit       *m_input;

    QString          m_stdinFifo;
    QString          m_stdoutFifo;
    QString          m_stderrFifo;

    QFile            m_stdin;
    QFile            m_stdout;
    QFile            m_stderr;

    QFile            m_stdoutD;
    QFile            m_stderrD;

    int              m_stdoutFD;
    int              m_stderrFD;

    QSocketNotifier *m_stdoutNotifier;
    QSocketNotifier *m_stderrNotifier;
};

#endif
