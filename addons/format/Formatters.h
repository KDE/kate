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
    FormatterRunner(Formatter fmt, const QJsonObject &obj, KTextEditor::Document *parent)
        : QObject(parent)
        , originalText(parent->text())
        , m_doc(parent)
        , m_globalConfig(obj)
        , m_fmt(std::move(fmt))
    {
    }

    virtual ~FormatterRunner()
    {
        if (m_procHandle && m_procHandle->state() != QProcess::NotRunning) {
            m_procHandle->disconnect(this);
            m_procHandle->kill();
            m_procHandle->waitForFinished();
        }
    }

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

class PrettierFormat : public FormatterRunner
{
public:
    using FormatterRunner::FormatterRunner;

    void run(KTextEditor::Document *) override;

    QString workingDir() const override
    {
        return Utils::projectBaseDirForDocument(m_doc);
    }

private:
    void onReadyReadOut();
    void onReadyReadErr();

    void setupNode();
    static inline QPointer<QTemporaryFile> s_tempFile = nullptr;
    static inline QPointer<QProcess> s_nodeProcess = nullptr;
    RunOutput m_runOutput;
};

class XmlLintFormat : public FormatterRunner
{
public:
    using FormatterRunner::FormatterRunner;

    QProcessEnvironment env() override
    {
        auto environment = FormatterRunner::env();
        auto ciface = m_doc;

        // Reuse doc's indent
        bool ok = false;
        int width = ciface->configValue(QStringLiteral("indent-width")).toInt(&ok);
        if (!ok) {
            return environment;
        }
        bool spaces = ciface->configValue(QStringLiteral("replace-tabs")).toBool();
        QString indent;
        if (spaces) {
            indent = QString(width, QLatin1Char(' '));
        } else {
            indent = QStringLiteral("\t");
        }

        environment.insert(QStringLiteral("XMLLINT_INDENT"), indent);
        return environment;
    }
};

FormatterRunner *formatterForDoc(KTextEditor::Document *doc, const QJsonObject &config);
