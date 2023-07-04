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

#include <KTextEditor/Editor>

// TODO: Move to KTextEditor::Document
static QString cursorToOffset(KTextEditor::Document *doc, KTextEditor::Cursor c)
{
    if (doc) {
        int o = 0;
        for (int i = 0; i < c.line(); ++i) {
            o += doc->lineLength(i) + 1; // + 1 for \n
        }
        o += c.column();
        return QString::number(o);
    }
    return {};
}

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
    auto is_or_contains = [m](const char *s) {
        return m.compare(QLatin1String(s), Qt::CaseInsensitive) == 0 || m.contains(QLatin1String(s));
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

    if (is_or_contains("c++")) {
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

    if (!workingDir().isEmpty()) {
        m_procHandle->setWorkingDirectory(workingDir());
    } else {
        m_procHandle->setWorkingDirectory(QFileInfo(doc->url().toDisplayString(QUrl::PreferLocalFile)).absolutePath());
    }
    m_procHandle->setProcessEnvironment(env());

    startHostProcess(*p, name, args);

    if (supportsStdin()) {
        const auto stdinText = textForStdin();
        if (!stdinText.isEmpty()) {
            p->write(stdinText);
            p->closeWriteChannel();
        }
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
    const QString offset = cursorToOffset(m_doc, m_pos);
    QStringList args;
    if (!offset.isEmpty()) {
        const_cast<ClangFormat *>(this)->m_withCursor = true;
        args << QStringLiteral("--cursor=%1").arg(offset);
    }

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

void ClangFormat::onResultReady(const RunOutput &o)
{
    if (!o.err.isEmpty()) {
        Q_EMIT error(QString::fromUtf8(o.err));
        return;
    }
    if (!o.out.isEmpty()) {
        if (m_withCursor) {
            int p = o.out.indexOf('\n');
            if (p >= 0) {
                QJsonParseError e;
                auto jd = QJsonDocument::fromJson(o.out.mid(0, p), &e);
                if (e.error == QJsonParseError::NoError && jd.isObject()) {
                    auto v = jd.object()[QLatin1String("Cursor")].toInt(-1);
                    Q_EMIT textFormatted(this, m_doc, o.out.mid(p + 1), v);
                }
            }
        } else {
            Q_EMIT textFormatted(this, m_doc, o.out);
        }
    }
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

void RustFormat::onResultReady(const RunOutput &out)
{
    if (!out.err.isEmpty()) {
        Q_EMIT error(QString::fromUtf8(out.err));
        return;
    }
    if (out.out.isEmpty()) {
        return;
    }

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    auto iface = qobject_cast<KTextEditor::MovingInterface *>(m_doc);
#endif
    std::vector<PatchLine> patch;
    const auto lines = out.out.split('\n');
    for (int i = 0; i < lines.size(); ++i) {
        if (!lines[i].startsWith("Diff in")) {
            continue;
        }

        int x = lines[i].indexOf("at line ");
        if (x < 0) {
            qWarning() << "Invalid rustfmt diff 1?";
            continue;
        }
        x += sizeof("at line ") - 1;
        int e = lines[i].indexOf(':', x);
        if (e < 0) {
            qWarning() << "Invalid rustfmt diff 2?";
            continue;
        }
        bool ok = false;
        auto lineNo = lines[i].mid(x, e - x).toInt(&ok);
        if (!ok) {
            qWarning() << "Invalid rustfmt diff 3?";
            continue;
        }

        // unroll into the hunk
        int srcline = lineNo - 1;
        int tgtline = lineNo - 1;
        for (int j = i + 1; j < lines.size(); ++j) {
            const auto &hl = lines.at(j);
            if (hl.startsWith((' '))) {
                srcline++;
                tgtline++;
            } else if (hl.startsWith(('+'))) {
                PatchLine p;
                p.type = PatchLine::Add;
                p.text = QString::fromUtf8(hl.mid(1));
                p.inPos = KTextEditor::Cursor(tgtline, 0);
                patch.push_back(p);
                tgtline++;
            } else if (hl.startsWith(('-'))) {
                PatchLine p;
                p.type = PatchLine::Remove;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                p.pos = m_doc->newMovingCursor(KTextEditor::Cursor(srcline, 0));
#else
                p.pos = iface->newMovingCursor(KTextEditor::Cursor(srcline, 0));
#endif
                patch.push_back(p);
                srcline++;
            } else if (hl.startsWith("Diff in")) {
                i = j - 1; // advance i to next hunk
                break;
            }
        }
    }

    textFormattedPatch(m_doc, patch);
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
    s_nodeProcess->waitForStarted();
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
    o[QStringLiteral("cursorOffset")] = cursorToOffset(doc, m_pos);
    s_nodeProcess->write(QJsonDocument(o).toJson(QJsonDocument::Compact) + '\0');
}

void GoFormat::onResultReady(const RunOutput &out)
{
    if (out.exitCode == 0) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        const auto parsed = parseDiff(m_doc, QString::fromUtf8(out.out));
#else
        auto iface = qobject_cast<KTextEditor::MovingInterface *>(m_doc);
        const auto parsed = parseDiff(iface, QString::fromUtf8(out.out));
#endif
        Q_EMIT textFormattedPatch(m_doc, parsed);
    } else if (!out.err.isEmpty()) {
        Q_EMIT error(QString::fromUtf8(out.err));
    }
}

#include "moc_Formatters.cpp"
