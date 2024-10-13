/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "Formatters.h"

#include "FormatApply.h"
#include "ModifiedLines.h"

#include <QDebug>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>

#include <KTextEditor/Editor>

Q_LOGGING_CATEGORY(FORMATTING, "kate.formatting", QtWarningMsg)

static QStringList readCommandFromJson(const QJsonObject &o)
{
    const auto arr = o.value(QStringLiteral("command")).toArray();
    QStringList args;
    args.reserve(arr.size());
    for (const auto &v : arr) {
        args.push_back(v.toString());
    }
    return args;
}

// Makes up a fake file name for doc mode
static QString filenameFromMode(KTextEditor::Document *doc)
{
    const QString m = doc->highlightingMode();
    auto is = [m](const char *s) {
        return m.compare(QLatin1String(s), Qt::CaseInsensitive) == 0;
    };

    QString path = doc->url().toLocalFile();
    bool needsStdinFileName;
    if (path.isEmpty()) {
        needsStdinFileName = true;
    }
    QFileInfo fi(path);
    const auto suffix = fi.suffix();
    const auto base = fi.baseName();
    // If the basename or suffix is missing, try to create a filename
    needsStdinFileName = fi.suffix().isEmpty() || fi.baseName().isEmpty();

    if (!needsStdinFileName) {
        return path;
    }

    QString prefix;
    if (!path.isEmpty()) {
        const auto fi = QFileInfo(path);
        prefix = fi.absolutePath();
        if (!prefix.isEmpty() && !prefix.endsWith(QLatin1Char('/'))) {
            prefix += QLatin1String("/");
        }
        const auto base = fi.baseName();
        if (!base.isEmpty()) {
            prefix += base + QStringLiteral("/");
        } else {
            prefix += QLatin1String("a");
        }
    } else {
        prefix = QStringLiteral("a");
    }

    if (is("c++") || is("iso c++")) {
        return prefix.append(QLatin1String(".cpp"));
    } else if (is("c")) {
        return prefix.append(QLatin1String(".c"));
    } else if (is("json")) {
        return prefix.append(QLatin1String(".json"));
    } else if (is("objective-c")) {
        return prefix.append(QLatin1String(".m"));
    } else if (is("objective-c++")) {
        return prefix.append(QLatin1String(".mm"));
    } else if (is("protobuf")) {
        return prefix.append(QLatin1String(".proto"));
    } else if (is("javascript")) {
        return prefix.append(QLatin1String(".js"));
    } else if (is("typescript")) {
        return prefix.append(QLatin1String(".ts"));
    } else if (is("javascript react (jsx)")) {
        return prefix.append(QLatin1String(".jsx"));
    } else if (is("typescript react (tsx)")) {
        return prefix.append(QLatin1String(".tsx"));
    } else if (is("css")) {
        return prefix.append(QLatin1String(".css"));
    } else if (is("html")) {
        return prefix.append(QLatin1String(".html"));
    } else if (is("yaml")) {
        return prefix.append(QLatin1String(".yml"));
    }
    return {};
}

void AbstractFormatter::run(KTextEditor::Document *doc)
{
    // QElapsedTimer t;
    // t.start();
    m_config = m_globalConfig.value(name()).toObject();
    // Arguments supplied by the user are prepended
    auto command = readCommandFromJson(m_config);
    if (command.isEmpty()) {
        Q_EMIT error(i18n("%1: Unexpected empty command!", this->name()));
        return;
    }
    QString path = command.takeFirst();
    const auto args = command + this->args(doc);
    const auto name = safeExecutableName(!path.isEmpty() ? path : this->name());
    if (name.isEmpty()) {
        Q_EMIT error(i18n("%1 is not installed, please install it to be able to format this document!", this->name()));
        return;
    }

    RunOutput o;

    m_procHandle = new QProcess(this);
    QProcess *p = m_procHandle;
    connect(p, &QProcess::finished, this, [this, p](int exit, QProcess::ExitStatus) {
        RunOutput o;
        o.exitCode = exit;
        o.out = p->readAllStandardOutput();
        o.err = p->readAllStandardError();
        onResultReady(o);

        p->deleteLater();
        deleteLater();
    });

    connect(p, &QProcess::errorOccurred, this, [this, p](QProcess::ProcessError e) {
        Q_EMIT error(QStringLiteral("%1: %2").arg(e).arg(p->errorString()));
        p->deleteLater();
        deleteLater();
    });

    if (!workingDir().isEmpty()) {
        m_procHandle->setWorkingDirectory(workingDir());
    } else {
        m_procHandle->setWorkingDirectory(QFileInfo(doc->url().toDisplayString(QUrl::PreferLocalFile)).absolutePath());
    }
    m_procHandle->setProcessEnvironment(env());

    qCDebug(FORMATTING) << "executing" << name << args;
    startHostProcess(*p, name, args);

    if (supportsStdin()) {
        connect(p, &QProcess::started, this, [this, p] {
            const auto stdinText = textForStdin();
            if (!stdinText.isEmpty()) {
                p->write(stdinText);
                p->closeWriteChannel();
            }
        });
    }
}

void AbstractFormatter::onResultReady(const RunOutput &o)
{
    if (!o.err.isEmpty()) {
        Q_EMIT error(QString::fromUtf8(o.err));
        return;
    } else if (!o.out.isEmpty()) {
        Q_EMIT textFormatted(this, m_doc, o.out);
    }
}

QStringList ClangFormat::args(KTextEditor::Document *doc) const
{
    QString file = doc->url().toLocalFile();
    QStringList args;

    args << QStringLiteral("--assume-filename=%1").arg(filenameFromMode(doc));

    if (file.isEmpty()) {
        return args;
    }

    if (!m_config.value(QStringLiteral("formatModifiedLinesOnly")).toBool()) {
        return args;
    }

    const auto lines = getModifiedLines(file);
    if (lines.has_value()) {
        for (auto ll : *lines) {
            args.push_back(QStringLiteral("--lines=%1:%2").arg(ll.startLine).arg(ll.endline));
        }
    }
    return args;
}

void DartFormat::onResultReady(const RunOutput &out)
{
    if (out.exitCode == 0) {
        // No change
        return;
    } else if (out.exitCode > 1) {
        if (!out.err.isEmpty()) {
            Q_EMIT error(QString::fromUtf8(out.err));
        }
    } else if (out.exitCode == 1) {
        Q_EMIT textFormatted(this, m_doc, out.out);
    }
}

void PrettierFormat::setupNode()
{
    if (s_nodeProcess && s_nodeProcess->state() == QProcess::Running) {
        return;
    }

    const QString path = m_config.value(QLatin1String("path")).toString();
    const auto node = safeExecutableName(!path.isEmpty() ? path : QStringLiteral("node"));
    if (node.isEmpty()) {
        Q_EMIT error(i18n("Please install node and prettier"));
        return;
    }

    delete s_tempFile;
    s_tempFile = new QTemporaryFile(KTextEditor::Editor::instance());
    if (!s_tempFile->open()) {
        Q_EMIT error(i18n("PrettierFormat: Failed to create temporary file"));
        return;
    }
    QFile prettierServer(QStringLiteral(":/formatting/prettier_script.js"));
    bool opened = prettierServer.open(QFile::ReadOnly);
    Q_ASSERT(opened);
    s_tempFile->write(prettierServer.readAll());
    s_tempFile->close();

    // Static node process
    s_nodeProcess = new QProcess(KTextEditor::Editor::instance());
    connect(KTextEditor::Editor::instance(), &KTextEditor::Editor::destroyed, s_nodeProcess, [] {
        s_nodeProcess->kill();
        s_nodeProcess->waitForFinished();
    });

    s_nodeProcess->setProgram(node);
    s_nodeProcess->setArguments({s_tempFile->fileName()});

    startHostProcess(*s_nodeProcess, QProcess::ReadWrite);
    if (!s_nodeProcess->waitForStarted()) {
        return;
    }
}

void PrettierFormat::onReadyReadOut()
{
    m_runOutput.out += s_nodeProcess->readAllStandardOutput();
    if (m_runOutput.out.endsWith("[[{END_PRETTIER_SCRIPT}]]")) {
        m_runOutput.out.truncate(m_runOutput.out.size() - (sizeof("[[{END_PRETTIER_SCRIPT}]]") - 1));
        QJsonParseError e;
        QJsonDocument doc = QJsonDocument::fromJson(m_runOutput.out, &e);
        m_runOutput.out = {};
        if (e.error != QJsonParseError::NoError) {
            Q_EMIT error(e.errorString());
        } else {
            const auto obj = doc.object();
            const auto formatted = obj[QStringLiteral("formatted")].toString().toUtf8();
            const auto cursor = obj[QStringLiteral("cursorOffset")].toInt(-1);
            Q_EMIT textFormatted(this, m_doc, formatted, cursor);
        }
    }
}

void PrettierFormat::onReadyReadErr()
{
    const auto err = s_nodeProcess->readAllStandardError();
    if (!err.isEmpty()) {
        Q_EMIT error(QString::fromUtf8(err));
    }
}

void PrettierFormat::run(KTextEditor::Document *doc)
{
    setupNode();
    if (!s_nodeProcess) {
        return;
    }

    const auto path = doc->url().toLocalFile();
    connect(s_nodeProcess, &QProcess::readyReadStandardOutput, this, &PrettierFormat::onReadyReadOut);
    connect(s_nodeProcess, &QProcess::readyReadStandardError, this, &PrettierFormat::onReadyReadErr);

    QJsonObject o;
    o[QStringLiteral("filePath")] = path;
    o[QStringLiteral("stdinFilePath")] = filenameFromMode(doc);
    o[QStringLiteral("source")] = originalText;
    o[QStringLiteral("cursorOffset")] = doc->cursorToOffset(m_pos);
    s_nodeProcess->write(QJsonDocument(o).toJson(QJsonDocument::Compact) + '\0');
}

#include "moc_Formatters.cpp"
