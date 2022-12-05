/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef KATE_FORMATTERS
#define KATE_FORMATTERS

#include <QIcon>
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
    AbstractFormatter(KTextEditor::Document *parent)
        : QObject(parent)
        , originalText(parent->text())
        , m_doc(parent)
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

    const QString originalText;

    static void showError(const QString &error)
    {
        Utils::showMessage(error, {}, i18n("Format"), i18n("Error"));
    }

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

    // Supports only stdin
    virtual bool onlyStdin() const
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
    QPointer<QProcess> m_procHandle;

private:
    QByteArray textForStdin() const
    {
        return originalText.toUtf8();
    }

Q_SIGNALS:
    void textFormatted(AbstractFormatter *formatter, KTextEditor::Document *doc, const QByteArray &text);
    void textFormattedPatch(KTextEditor::Document *doc, const std::vector<PatchLine> &);
};

class ClangFormat : public AbstractFormatter
{
    Q_OBJECT
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
    Q_OBJECT
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
    Q_OBJECT
public:
    using AbstractFormatter::AbstractFormatter;
    QString name() const override
    {
        return QStringLiteral("jq");
    }

    QStringList args(KTextEditor::Document *doc) const override
    {
        return {QStringLiteral("."),
                QStringLiteral("--indent"),
                QStringLiteral("4"),
                QStringLiteral("-M"), // no color
                doc->url().toDisplayString(QUrl::PreferLocalFile)};
    }

private:
    void onResultReady(const RunOutput &out) override
    {
        if (out.exitCode == 0) {
            Q_EMIT textFormatted(this, m_doc, out.out);
        } else if (out.exitCode != 0 && !out.err.isEmpty()) {
            showError(QString::fromUtf8(out.err));
        }
    }
};

class PrettierFormat : public AbstractFormatter
{
    Q_OBJECT
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

    void onResultReady(const RunOutput &out) override
    {
        if (out.exitCode == 0) {
            Q_EMIT textFormatted(this, m_doc, out.out);
        } else if (out.exitCode != 0 && !out.err.isEmpty()) {
            showError(QString::fromUtf8(out.err));
        }
    }

    void setupNode();
    QPointer<QTemporaryFile> m_tempFile = nullptr;
    static inline QPointer<QProcess> s_nodeProcess = nullptr;
    RunOutput m_runOutput;
};

class RustFormat : public AbstractFormatter
{
    Q_OBJECT
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

    bool onlyStdin() const override
    {
        return true;
    }

    void onResultReady(const RunOutput &out) override;
};

class XmlLintFormat : public AbstractFormatter
{
    Q_OBJECT
public:
    using AbstractFormatter::AbstractFormatter;
    QString name() const override
    {
        return QStringLiteral("xmllint");
    }

    QStringList args(KTextEditor::Document *doc) const override
    {
        return {QStringLiteral("--format"), doc->url().toDisplayString(QUrl::PreferLocalFile)};
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

private:
    void onResultReady(const RunOutput &out) override
    {
        if (out.exitCode == 0) {
            Q_EMIT textFormatted(this, m_doc, out.out);
        } else if (out.exitCode != 0 && !out.err.isEmpty()) {
            showError(QString::fromUtf8(out.err));
        }
    }
};

class GoFormat : public AbstractFormatter
{
    Q_OBJECT
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

    bool onlyStdin() const override
    {
        return true;
    }

    void onResultReady(const RunOutput &out) override;
};

#endif
