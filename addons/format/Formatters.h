/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include <QJsonObject>
#include <QPointer>
#include <QProcess>

#include <KTextEditor/Document>

#include <ktexteditor_utils.h>

class QTemporaryFile;

struct Formatter {
    QString name;
    QStringList args;
    bool supportsStdin = false;
};

class FormatterRunner : public QObject
{
    Q_OBJECT
public:
    FormatterRunner(Formatter fmt, const QJsonObject &obj, KTextEditor::Document *parent);

    ~FormatterRunner() override;

    virtual void run(KTextEditor::Document *doc);

    void setCursorPosition(KTextEditor::Cursor c)
    {
        m_pos = c;
    }

    bool formatOnSaveEnabled(bool defaultValue) const
    {
        return m_globalConfig.value(m_fmt.name).toObject().value(QLatin1String("formatOnSave")).toBool(defaultValue);
    }

    QString cmdline() const
    {
        if (m_procHandle) {
            return m_procHandle->program() + QLatin1String(" ") + m_procHandle->arguments().join(QLatin1String(" "));
        }
        return {};
    }

    const QString originalText;

protected:
    struct RunOutput {
        int exitCode = -1;
        QByteArray out;
        QByteArray err;
    };

    virtual void onResultReady(const RunOutput &o);

    virtual QString workingDir() const
    {
        return {};
    }

    virtual QProcessEnvironment env()
    {
        return QProcessEnvironment::systemEnvironment();
    }

    QPointer<KTextEditor::Document> m_doc;
    QJsonObject m_config;
    QPointer<QProcess> m_procHandle;
    KTextEditor::Cursor m_pos;
    const QJsonObject m_globalConfig;
    const Formatter m_fmt;

private:
    QByteArray textForStdin() const
    {
        return originalText.toUtf8();
    }

Q_SIGNALS:
    void textFormatted(FormatterRunner *, KTextEditor::Document *, const QByteArray &text, int offset = -1);
    void message(const QString &error, MessageType = MessageType::Error);
};

class XmlLintFormat : public FormatterRunner
{
public:
    using FormatterRunner::FormatterRunner;

    QProcessEnvironment env() override;
};

FormatterRunner *formatterForDoc(KTextEditor::Document *doc, const QJsonObject &config);
