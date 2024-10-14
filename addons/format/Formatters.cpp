/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "Formatters.h"
#include "FormattersEnum.h"

#include <QDebug>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>

#include <KLocalizedString>
#include <KTextEditor/Editor>

Q_LOGGING_CATEGORY(FORMATTING, "kate.formatting", QtWarningMsg)

static QStringList readCommandFromJson(const QJsonObject &o)
{
    const auto arr = o.value(QLatin1String("command")).toArray();
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

void FormatterRunner::run(KTextEditor::Document *doc)
{
    // QElapsedTimer t;
    // t.start();
    m_config = m_globalConfig.value(m_fmt.name).toObject();
    // Arguments supplied by the user are prepended
    auto command = readCommandFromJson(m_config);
    if (command.isEmpty()) {
        Q_EMIT error(i18n("%1: Unexpected empty command!", m_fmt.name));
        return;
    }
    QString path = command.takeFirst();
    const auto args = command + m_fmt.args;
    const auto name = safeExecutableName(!path.isEmpty() ? path : m_fmt.name);
    if (name.isEmpty()) {
        Q_EMIT error(i18n("%1 is not installed, please install it to be able to format this document!", m_fmt.name));
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

    if (m_fmt.supportsStdin) {
        connect(p, &QProcess::started, this, [this, p] {
            const auto stdinText = textForStdin();
            if (!stdinText.isEmpty()) {
                p->write(stdinText);
                p->closeWriteChannel();
            }
        });
    }

    qCDebug(FORMATTING) << "executing" << name << args;
    startHostProcess(*p, name, args);
}

void FormatterRunner::onResultReady(const RunOutput &o)
{
    if (!o.err.isEmpty()) {
        Q_EMIT error(QString::fromUtf8(o.err));
        return;
    } else if (!o.out.isEmpty()) {
        Q_EMIT textFormatted(this, m_doc, o.out);
    }
}

void PrettierFormat::setupNode()
{
    if (s_nodeProcess && s_nodeProcess->state() == QProcess::Running) {
        return;
    }

    m_config = m_globalConfig.value(m_fmt.name).toObject();
    const QStringList cmd = readCommandFromJson(m_config);
    if (cmd.isEmpty()) {
        return;
    }
    const auto node = safeExecutableName(cmd.first());
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
        Q_EMIT error(i18n("PrettierFormat: Failed to start 'node': %1", s_nodeProcess->errorString()));
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

static Formatter newStdinFmt(const char *name, const QStringList &args)
{
    return {.name = QString::fromUtf8(name), .args = args, .workingDir = {}, .supportsStdin = true};
}

inline Formatter jqFmt(KTextEditor::Document *doc)
{
    // Reuse doc's indent
    bool ok = false;
    int width = doc->configValue(QStringLiteral("indent-width")).toInt(&ok);
    width = ok ? width : 4;
    const QStringList args{QStringLiteral("."), QStringLiteral("--indent"), QString::number(width), QStringLiteral("-M")}; // -M => no color
    return newStdinFmt("jq", args);
}

#define S(s) QStringLiteral(s)
static Formatter prettier()
{
    return Formatter{S("prettier"), {}, {}, true};
}

static std::optional<Formatter> makeFormatter(KTextEditor::Document *doc, const QJsonObject &config)
{
    const auto mode = doc->highlightingMode().toLower();
    auto is = [mode](const char *s) {
        return mode == QLatin1String(s);
    };
    auto is_or_contains = [mode](const char *s) {
        return mode == QLatin1String(s) || mode.contains(QLatin1String(s));
    };

    // NOTE: When adding a new formatter ensure that it is documented in plugins.docbook

    if (is_or_contains("c++") || is("c") || is("objective-c") || is("objective-c++") || is("protobuf")) {
        return newStdinFmt("clang-format", {S("--assume-filename=%1").arg(filenameFromMode(doc))});
    } else if (is("dart")) {
        return newStdinFmt("dart",
                           {S("format"), S("--output=show"), S("--summary=none"), S("--stdin-name"), doc->url().toDisplayString(QUrl::PreferLocalFile)});
    } else if (is("html")) {
        return prettier();
    } else if (is("javascript") || is("typescript") || is("typescript react (tsx)") || is("javascript react (jsx)") || is("css")) {
        return prettier();
    } else if (is("json")) {
        const auto configValue = config.value(QLatin1String("formatterForJson")).toString();
        Formatters f = formatterForName(configValue, Formatters::Prettier);
        if (f == Formatters::Prettier) {
            return prettier();
        } else if (f == Formatters::ClangFormat) {
            return newStdinFmt("clang-format", {S("--assume-filename=%1").arg(filenameFromMode(doc))});
        } else if (f == Formatters::Jq) {
            return jqFmt(doc);
        }
        Utils::showMessage(i18n("Unknown formatterForJson: %1", configValue), {}, i18n("Format"), MessageType::Error);
        return jqFmt(doc);
    } else if (is("rust")) {
        return newStdinFmt("rustfmt", {S("--color=never"), S("--emit=stdout")});
    } else if (is("xml")) {
        return newStdinFmt("xmllint", {S("--format"), S("-")});
    } else if (is("go")) {
        return newStdinFmt("gofmt", {});
    } else if (is("zig")) {
        return newStdinFmt("zig", {S("fmt"), S("--stdin")});
    } else if (is("cmake")) {
        return newStdinFmt("cmake-format", {S("-")});
    } else if (is("python")) {
        const auto configValue = config.value(QLatin1String("formatterForPython")).toString();
        Formatters f = formatterForName(configValue, Formatters::Ruff);
        if (f == Formatters::Ruff) {
            return newStdinFmt("ruff", {S("format"), S("-q"), S("--stdin-filename"), S("a.py")});
        } else if (f == Formatters::Autopep8) {
            return newStdinFmt("autopep8", {S("-")});
        }
        Utils::showMessage(i18n("Unknown formatterForPython: %1", configValue), {}, i18n("Format"), MessageType::Error);
        return newStdinFmt("ruff", {S("format"), S("-q"), S("--stdin-filename"), S("a.py")});
    } else if (is("d")) {
        return newStdinFmt("dfmt", {});
    } else if (is("fish")) {
        return newStdinFmt("fish_indent", {});
    } else if (is("bash")) {
        int width = doc->configValue(S("indent-width")).toInt();
        width = width == 0 ? 4 : width;
        bool spaces = doc->configValue(S("replace-tabs")).toBool();
        return newStdinFmt("shfmt", {S("--indent"), QString::number(spaces ? width : 0)});
    } else if (is("nixfmt")) {
        return newStdinFmt("nixfmt", {});
    } else if (is("qml")) {
        return Formatter{.name = S("qmlformat"), .args = {doc->url().toLocalFile()}, .workingDir = {}, .supportsStdin = false};
    } else if (is("yaml")) {
        const auto configValue = config.value(QLatin1String("formatterForYaml")).toString();
        Formatters f = formatterForName(configValue, Formatters::YamlFmt);
        if (f == Formatters::YamlFmt) {
            return newStdinFmt("yamlfmt", {});
        } else if (f == Formatters::Prettier) {
            return prettier();
        }
        Utils::showMessage(i18n("Unknown formatterForYaml: %1, falling back to yamlfmt", configValue), {}, i18n("Format"), MessageType::Error);
        return newStdinFmt("yamlfmt", {});
    } else if (is("opsi-script")) {
        return newStdinFmt("opsi-script-beautifier", {});
    } else if (is("odin")) {
        return newStdinFmt("odinfmt", {S("--stdin")});
    }
    return std::nullopt;
#undef S
}

FormatterRunner *formatterForDoc(KTextEditor::Document *doc, const QJsonObject &config)
{
    if (!doc) {
        qWarning() << "Unexpected null doc";
        return nullptr;
    }

    const std::optional<Formatter> fmtOpt = makeFormatter(doc, config);
    if (!fmtOpt.has_value()) {
        static QList<QString> alreadyWarned;
        const QString mode = doc->highlightingMode();
        if (!alreadyWarned.contains(mode)) {
            alreadyWarned.push_back(mode);
            Utils::showMessage(i18n("Failed to run formatter. Unsupported language %1", mode), {}, i18n("Format"), MessageType::Info);
        }
        return nullptr;
    }
    const Formatter fmt = fmtOpt.value();
    if (fmt.name == QLatin1String("prettier")) {
        return new PrettierFormat(fmt, config, doc);
    }
    if (fmt.name == QLatin1String("xmllint")) {
        return new XmlLintFormat(fmt, config, doc);
    }
    return new FormatterRunner(fmt, config, doc);
}

#include "moc_Formatters.cpp"
