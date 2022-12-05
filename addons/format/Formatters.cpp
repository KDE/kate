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
#include <QJsonDocument>
#include <QJsonObject>

#include <KTextEditor/Editor>

void AbstractFormatter::run(KTextEditor::Document *doc)
{
    // QElapsedTimer t;
    // t.start();
    const auto args = this->args(doc);
    const auto name = safeExecutableName(this->name());
    if (name.isEmpty()) {
        showError(i18n("%1 is not installed, please install it to be able to format this document!", this->name()));
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
        if (onlyStdin() || doc->url().toLocalFile().isEmpty()) {
            const auto stdinText = textForStdin();
            if (!stdinText.isEmpty()) {
                p->write(stdinText);
                p->closeWriteChannel();
            }
        }
    }
}

void AbstractFormatter::onResultReady(const RunOutput &o)
{
    if (!o.err.isEmpty()) {
        showError(QString::fromUtf8(o.err));
    }
    if (o.out.isEmpty()) {
        return;
    }

    Q_EMIT textFormatted(this, m_doc, o.out);
}

QStringList ClangFormat::args(KTextEditor::Document *doc) const
{
    const auto file = doc->url().toLocalFile();
    if (file.isEmpty()) {
        return {};
    }

    const auto lines = getModifiedLines(file);
    QStringList args;
    if (lines.has_value()) {
        for (auto ll : *lines) {
            args.push_back(QStringLiteral("--lines=%1:%2").arg(ll.startLine).arg(ll.endline));
        }
        args.push_back(file);
        return args;
    } else {
        return {file};
    }
}

void DartFormat::onResultReady(const RunOutput &out)
{
    if (out.exitCode == 0) {
        // No change
        return;
    } else if (out.exitCode > 1) {
        if (!out.err.isEmpty()) {
            showError(QString::fromUtf8(out.err));
        }
    } else if (out.exitCode == 1) {
        Q_EMIT textFormatted(this, m_doc, out.out);
    }
}

void RustFormat::onResultReady(const RunOutput &out)
{
    if (!out.err.isEmpty()) {
        showError(QString::fromUtf8(out.err));
        return;
    }
    if (out.out.isEmpty()) {
        return;
    }

    auto iface = qobject_cast<KTextEditor::MovingInterface *>(m_doc);
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
                p.pos = iface->newMovingCursor(KTextEditor::Cursor(srcline, 0));
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
    if (s_nodeProcess) {
        return;
    }

    const auto node = safeExecutableName(QStringLiteral("node"));
    if (node.isEmpty()) {
        showError(i18n("Please install node and prettier"));
        return;
    }

    delete m_tempFile;
    m_tempFile = new QTemporaryFile(KTextEditor::Editor::instance());
    if (!m_tempFile->open()) {
        showError(i18n("PrettierFormat: Failed to create temporary file"));
        return;
    }
    QFile prettierServer(QStringLiteral(":/formatting/prettier_script.js"));
    bool opened = prettierServer.open(QFile::ReadOnly);
    Q_ASSERT(opened);
    m_tempFile->write(prettierServer.readAll());
    m_tempFile->close();

    // Static node process
    s_nodeProcess = new QProcess(KTextEditor::Editor::instance());
    connect(KTextEditor::Editor::instance(), &KTextEditor::Editor::destroyed, s_nodeProcess, [] {
        s_nodeProcess->kill();
        s_nodeProcess->waitForFinished();
    });

    s_nodeProcess->setProgram(node);
    s_nodeProcess->setArguments({m_tempFile->fileName()});

    startHostProcess(*s_nodeProcess, QProcess::ReadWrite);
    s_nodeProcess->waitForStarted();
}

void PrettierFormat::onReadyReadOut()
{
    m_runOutput.out += s_nodeProcess->readAllStandardOutput();
    if (m_runOutput.out.endsWith("[[{END_PRETTIER_SCRIPT}]]")) {
        m_runOutput.out.truncate(m_runOutput.out.size() - (sizeof("[[{END_PRETTIER_SCRIPT}]]") - 1));
        m_runOutput.exitCode = 0;
        onResultReady(m_runOutput);
        m_runOutput.out.clear();
    }
}

void PrettierFormat::onReadyReadErr()
{
    RunOutput o;
    o.exitCode = 1;
    o.err = s_nodeProcess->readAllStandardError();
    onResultReady(o);
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
    o[QStringLiteral("source")] = originalText;
    s_nodeProcess->write(QJsonDocument(o).toJson(QJsonDocument::Compact) + '\0');
}

void GoFormat::onResultReady(const RunOutput &out)
{
    if (out.exitCode == 0) {
        auto iface = qobject_cast<KTextEditor::MovingInterface *>(m_doc);
        const auto parsed = parseDiff(iface, QString::fromUtf8(out.out));
        Q_EMIT textFormattedPatch(m_doc, parsed);
    } else if (!out.err.isEmpty()) {
        showError(QString::fromUtf8(out.err));
    }
}
