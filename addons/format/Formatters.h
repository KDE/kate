/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include <QJsonObject>
#include <QPointer>
#include <QProcess>

#include <KLocalizedString>
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

    bool supportsStdin() const override
    {
        return true;
    }
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

    bool supportsStdin() const override
    {
        return true;
    }
};

class StdinFormatter : public AbstractFormatter
{
public:
    StdinFormatter(const QJsonObject &obj, KTextEditor::Document *parent, const QString &name, const QStringList &args)
        : AbstractFormatter(obj, parent)
        , m_name(name)
        , m_args(args)
    {
    }

    QString name() const override
    {
        return m_name;
    }
    QStringList args(KTextEditor::Document *) const override
    {
        return m_args;
    }

private:
    bool supportsStdin() const override
    {
        return true;
    }

    QString m_name;
    QStringList m_args;
};

class QMLFormat : public AbstractFormatter
{
public:
    using AbstractFormatter::AbstractFormatter;
    QString name() const override
    {
        return QStringLiteral("qmlformat");
    }

    QStringList args(KTextEditor::Document *) const override
    {
        return {m_doc->url().toLocalFile()};
    }
};

#define S(s) QStringLiteral(s)

#define DEFINE_STDIN_FORMATTER(FN_NAME, NAME, ARGS)                                                                                                            \
    inline AbstractFormatter *FN_NAME(const QJsonObject &obj, KTextEditor::Document *parent)                                                                   \
    {                                                                                                                                                          \
        return new StdinFormatter(obj, parent, QLatin1String(NAME), ARGS);                                                                                     \
    }

DEFINE_STDIN_FORMATTER(rustFormat, "rustfmt", (QStringList{S("--color=never"), S("--emit=stdout")}));
DEFINE_STDIN_FORMATTER(zigFormat, "zig", (QStringList{S("fmt"), S("--stdin")}));
DEFINE_STDIN_FORMATTER(ruffFormat, "ruff", (QStringList{S("format"), S("-q"), S("--stdin-filename"), S("a.py")}));
DEFINE_STDIN_FORMATTER(goFormat, "gofmt", (QStringList{}));
DEFINE_STDIN_FORMATTER(autoPep8Format, "autopep8", (QStringList{S("-")}));
DEFINE_STDIN_FORMATTER(cMakeFormat, "cmake-format", (QStringList{S("-")}));
DEFINE_STDIN_FORMATTER(dfmt, "dfmt", (QStringList{}));
DEFINE_STDIN_FORMATTER(fishIndent, "fish_indent", (QStringList{}));
DEFINE_STDIN_FORMATTER(nixfmt, "nixfmt", (QStringList{}));
DEFINE_STDIN_FORMATTER(yamlfmt, "yamlfmt", (QStringList{}));
DEFINE_STDIN_FORMATTER(opsiScriptBeautifier, "opsi-script-beautifier", (QStringList{S("-s")}));
DEFINE_STDIN_FORMATTER(odinFmt, "odinfmt", (QStringList{S("--stdin")}));

inline AbstractFormatter *shfmt(const QJsonObject &obj, KTextEditor::Document *parent)
{
    int width = parent->configValue(QStringLiteral("indent-width")).toInt();
    width = width == 0 ? 4 : width;
    bool spaces = parent->configValue(QStringLiteral("replace-tabs")).toBool();
    return new StdinFormatter(obj, parent, QStringLiteral("shfmt"), QStringList{QStringLiteral("--indent"), QString::number(spaces ? width : 0)});
}

inline AbstractFormatter *jqFmt(const QJsonObject &obj, KTextEditor::Document *doc)
{
    // Reuse doc's indent
    bool ok = false;
    int width = doc->configValue(QStringLiteral("indent-width")).toInt(&ok);
    width = ok ? width : 4;

    const QStringList args{QStringLiteral("."), QStringLiteral("--indent"), QString::number(width), QStringLiteral("-M")}; // -M => no color
    return new StdinFormatter(obj, doc, QStringLiteral("jq"), args);
}

#undef S
#undef DEFINE_STDIN_FORMATTER
