// SPDX-FileCopyrightText: 2022 Kåre Särs <kare.sars@iki.fi>
//
//  SPDX-License-Identifier: LGPL-2.0-only

#include "AppOutput.h"

#include "hostprocess.h"

#include <QDebug>
#include <QFontDatabase>
#include <QScrollBar>
#include <QTextCursor>
#include <QTextEdit>
#include <QVBoxLayout>

#include <KColorScheme>
#include <KPluginFactory>
#include <KProcess>

#include <KParts/ReadOnlyPart>
#include <kde_terminal_interface.h>

struct AppOutput::Private {
    Private(AppOutput *pub)
        : q(pub)
    {
    }
    KParts::ReadOnlyPart *part = nullptr;
    KProcess process;
    QTextEdit *outputArea = nullptr;
    QString terminalProcess;
    AppOutput *q = nullptr;

    void addOutputText(QString const &text)
    {
        qDebug() << text;
        if (!outputArea) {
            qWarning() << "Can't output text to nullptr";
            return;
        }

        QScrollBar *scrollb = outputArea->verticalScrollBar();
        if (!scrollb) {
            return;
        }
        bool atEnd = (scrollb->value() == scrollb->maximum());

        QTextCursor cursor = outputArea->textCursor();
        if (!cursor.atEnd()) {
            cursor.movePosition(QTextCursor::End);
        }
        cursor.insertText(text);

        if (atEnd) {
            scrollb->setValue(scrollb->maximum());
        }
    }

    void updateTerminalProcessInfo()
    {
        TerminalInterface *t = qobject_cast<TerminalInterface *>(part);
        if (!t) {
            return;
        }

        if (terminalProcess != t->foregroundProcessName()) {
            terminalProcess = t->foregroundProcessName();
            Q_EMIT q->runningChanhged();
        }
    }
};

AppOutput::AppOutput(QWidget *parent)
    : QWidget(parent)
    , d(std::make_unique<AppOutput::Private>(this))
{
    KPluginFactory *factory = KPluginFactory::loadFactory(QStringLiteral("konsolepart")).plugin;
    if (!factory) {
        qDebug() << "could not load the konsolepart factory";
    } else {
        d->part = factory->create<KParts::ReadOnlyPart>(this);
    }

    TerminalInterface *t = nullptr;
    if (!d->part) {
        qDebug() << "could not create a konsole part";
    } else {
        t = qobject_cast<TerminalInterface *>(d->part);
    }
    if (!t) {
        qDebug() << "Failed to cast the TerminalInterface";
    }

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    if (d->part) {
        layout->addWidget(d->part->widget());
        connect(d->part->widget(), &QObject::destroyed, this, &AppOutput::deleteLater);
        setFocusProxy(d->part->widget());

        connect(d->part, &KParts::ReadOnlyPart::setWindowCaption, this, [this]() {
            d->updateTerminalProcessInfo();
        });

    } else {
        d->outputArea = new QTextEdit(this);
        layout->addWidget(d->outputArea);
        d->outputArea->setAcceptRichText(false);
        d->outputArea->setReadOnly(true);
        d->outputArea->setUndoRedoEnabled(false);
        // fixed wide font, like konsole
        d->outputArea->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
        // alternate color scheme, like konsole
        KColorScheme schemeView(QPalette::Active, KColorScheme::View);
        d->outputArea->setTextBackgroundColor(schemeView.foreground().color());
        d->outputArea->setTextColor(schemeView.background().color());
        QPalette p = d->outputArea->palette();
        p.setColor(QPalette::Base, schemeView.foreground().color());
        d->outputArea->setPalette(p);

        d->process.setOutputChannelMode(KProcess::SeparateChannels);
        connect(&d->process, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this, &AppOutput::runningChanhged);
        connect(&d->process, &KProcess::readyReadStandardError, this, [this]() {
            d->addOutputText(QString::fromUtf8(d->process.readAllStandardError()));
        });
        connect(&d->process, &KProcess::readyReadStandardOutput, this, [this]() {
            d->addOutputText(QString::fromUtf8(d->process.readAllStandardOutput()));
        });
    }
}

AppOutput::~AppOutput()
{
    d->process.kill();
}

void AppOutput::setWorkingDir(const QString &path)
{
    TerminalInterface *t = qobject_cast<TerminalInterface *>(d->part);
    if (t) {
        t->showShellInDir(path);
    } else {
        d->process.setWorkingDirectory(path);
    }
}

void AppOutput::runCommand(const QString &cmd)
{
    TerminalInterface *t = qobject_cast<TerminalInterface *>(d->part);
    if (t) {
        t->sendInput(cmd + QLatin1Char('\n'));
        d->terminalProcess = cmd;
    } else {
        d->outputArea->clear();
        d->process.setShellCommand(cmd);
        startHostProcess(d->process);
        d->process.waitForStarted(300);
    }
    Q_EMIT runningChanhged();
}

QString AppOutput::runningProcess()
{
    TerminalInterface *t = qobject_cast<TerminalInterface *>(d->part);
    if (t) {
        return d->terminalProcess;
    }

    QString program = d->process.program().isEmpty() ? QString() : d->process.program().first();
    return d->process.state() == QProcess::NotRunning ? QString() : program;
}
