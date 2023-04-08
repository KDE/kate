/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <QJsonObject>
#include <QPointer>
#include <QProcess>

#include <KLocalizedString>
#include <KTextEditor/ConfigInterface>
#include <KTextEditor/Document>

#include "FormatApply.h"
#include <ktexteditor_utils.h>

struct PatchLine;

class AbstractFormatter : public QObject
{
    Q_OBJECT
public:
    AbstractFormatter(const QJsonObject &obj, KTextEditor::Document *parent)
        : QObject(parent)
        , originalText(parent->text())
        , m_doc(parent)
        , m_globalConfig(obj)
    {
    }

    virtual ~AbstractFormatter()
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
        return m_globalConfig.value(name()).toObject().value(QLatin1String("formatOnSave")).toBool(defaultValue);
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

    virtual bool supportsStdin() const
    {
        return false;
    }

    virtual QStringList args(KTextEditor::Document *doc) const = 0;
    virtual QString name() const = 0;
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

private:
    QByteArray textForStdin() const
    {
        return originalText.toUtf8();
    }

    const QJsonObject m_globalConfig;

Q_SIGNALS:
    void textFormatted(AbstractFormatter *formatter, KTextEditor::Document *doc, const QByteArray &text, int offset = -1);
    void textFormattedPatch(KTextEditor::Document *doc, const std::vector<PatchLine> &);
    void error(const QString &error);
};

class ClangFormat : public AbstractFormatter
{
public:
    using AbstractFormatter::AbstractFormatter;
    QString name() const override
    {
        return QStringLiteral("clang-format");
    }

    QStringList args(KTextEditor::Document *doc) const override;
    void onResultReady(const RunOutput &o) override;

    bool supportsStdin() const override
    {
        return true;
    }

private:
    bool m_withCursor = false;
};

class DartFormat : public AbstractFormatter
{
public:
    using AbstractFormatter::AbstractFormatter;
    QString name() const override
    {
        return QStringLiteral("dart");
    }

    QStringList args(KTextEditor::Document *doc) const override
    {
        return {QStringLiteral("format"),
                QStringLiteral("--output=show"),
                QStringLiteral("--summary=none"),
                QStringLiteral("--set-exit-if-changed"),
                doc->url().toDisplayString(QUrl::PreferLocalFile)};
    }

private:
    void onResultReady(const RunOutput &out) override;
};

class JsonJqFormat : public AbstractFormatter
{
public:
    using AbstractFormatter::AbstractFormatter;
    QString name() const override
    {
        return QStringLiteral("jq");
    }

    QStringList args(KTextEditor::Document *doc) const override
    {
        auto ciface = qobject_cast<KTextEditor::ConfigInterface *>(doc);
        // Reuse doc's indent
        bool ok = false;
        int width = ciface->configValue(QStringLiteral("indent-width")).toInt(&ok);
        if (!ok) {
            width = 4;
        }
        return {
            QStringLiteral("."),
            QStringLiteral("--indent"),
            QString::number(width),
            QStringLiteral("-M"), // no color
        };
    }

private:
    bool supportsStdin() const override
    {
        return true;
    }
};

class PrettierFormat : public AbstractFormatter
{
public:
    using AbstractFormatter::AbstractFormatter;

    QString name() const override
    {
        return QStringLiteral("prettier");
    }

    void run(KTextEditor::Document *) override;

    QStringList args(KTextEditor::Document *doc) const override
    {
        return {QStringLiteral("--no-color"), doc->url().toDisplayString(QUrl::PreferLocalFile)};
    }

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

class RustFormat : public AbstractFormatter
{
public:
    using AbstractFormatter::AbstractFormatter;
    QString name() const override
    {
        return QStringLiteral("rustfmt");
    }

    QStringList args(KTextEditor::Document *) const override
    {
        return {QStringLiteral("--color=never"), QStringLiteral("--check")};
    }

private:
    bool supportsStdin() const override
    {
        return true;
    }

    void onResultReady(const RunOutput &out) override;
};

class XmlLintFormat : public AbstractFormatter
{
public:
    using AbstractFormatter::AbstractFormatter;
    QString name() const override
    {
        return QStringLiteral("xmllint");
    }

    QStringList args(KTextEditor::Document *) const override
    {
        return {QStringLiteral("--format"), QStringLiteral("-")};
    }

    QProcessEnvironment env() override
    {
        auto environment = AbstractFormatter::env();
        auto ciface = qobject_cast<KTextEditor::ConfigInterface *>(m_doc);

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

    bool supportsStdin() const override
    {
        return true;
    }
};

class GoFormat : public AbstractFormatter
{
public:
    using AbstractFormatter::AbstractFormatter;
    QString name() const override
    {
        return QStringLiteral("gofmt");
    }

    QStringList args(KTextEditor::Document *) const override
    {
        return {QStringLiteral("-d")};
    }

private:
    bool supportsStdin() const override
    {
        return true;
    }

    void onResultReady(const RunOutput &out) override;
};

class ZigFormat : public AbstractFormatter
{
public:
    using AbstractFormatter::AbstractFormatter;
    QString name() const override
    {
        return QStringLiteral("zig");
    }

    QStringList args(KTextEditor::Document *) const override
    {
        return {QStringLiteral("fmt"), QStringLiteral("--stdin")};
    }

private:
    bool supportsStdin() const override
    {
        return true;
    }
};

class CMakeFormat : public AbstractFormatter
{
public:
    using AbstractFormatter::AbstractFormatter;
    QString name() const override
    {
        return QStringLiteral("cmake-format");
    }

    QStringList args(KTextEditor::Document *) const override
    {
        return {m_doc->url().toLocalFile()};
    }
};

class AutoPep8Format : public AbstractFormatter
{
public:
    using AbstractFormatter::AbstractFormatter;
    QString name() const override
    {
        return QStringLiteral("autopep8");
    }

    QStringList args(KTextEditor::Document *) const override
    {
        return {m_doc->url().toLocalFile()};
    }
};
