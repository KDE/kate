/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "EslintPlugin.h"

#include <KLocalizedString>
#include <KPluginFactory>
#include <KSharedConfig>
#include <KTextEditor/Application>
#include <KTextEditor/Editor>
#include <KTextEditor/View>
#include <KXMLGUIFactory>

#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <hostprocess.h>
#include <ktexteditor_utils.h>

K_PLUGIN_FACTORY_WITH_JSON(ESLintPluginFactory, "EslintPlugin.json", registerPlugin<ESLintPlugin>();)

ESLintPlugin::ESLintPlugin(QObject *parent)
    : KTextEditor::Plugin(parent)
{
}

QObject *ESLintPlugin::createView(KTextEditor::MainWindow *mainWindow)
{
    return new ESLintPluginView(mainWindow);
}

ESLintPluginView::ESLintPluginView(KTextEditor::MainWindow *mainWin)
    : QObject(mainWin)
    , m_mainWindow(mainWin)
    , m_provider(mainWin, this)
{
    m_provider.setObjectName(QStringLiteral("ESLintDiagnosticProvider"));
    m_provider.name = i18n("ESLint");

    connect(mainWin, &KTextEditor::MainWindow::viewChanged, this, &ESLintPluginView::onActiveViewChanged);
    connect(&m_eslintProcess, &QProcess::readyReadStandardOutput, this, &ESLintPluginView::onReadyRead);
    connect(&m_eslintProcess, &QProcess::readyReadStandardError, this, &ESLintPluginView::onError);
    connect(&m_provider, &DiagnosticsProvider::requestFixes, this, &ESLintPluginView::onFixesRequested);

    m_mainWindow->guiFactory()->addClient(this);
}

ESLintPluginView::~ESLintPluginView()
{
    disconnect(&m_eslintProcess, &QProcess::readyReadStandardOutput, this, &ESLintPluginView::onReadyRead);
    disconnect(&m_eslintProcess, &QProcess::readyReadStandardError, this, &ESLintPluginView::onError);
    if (m_eslintProcess.state() == QProcess::Running) {
        m_eslintProcess.kill();
        m_eslintProcess.waitForFinished();
    }
    disconnect(m_mainWindow, &KTextEditor::MainWindow::viewChanged, this, &ESLintPluginView::onActiveViewChanged);
    m_mainWindow->guiFactory()->removeClient(this);
}

void ESLintPluginView::onActiveViewChanged(KTextEditor::View *v)
{
    if (v && v->document() == m_activeDoc) {
        return;
    }

    if (m_activeDoc) {
        disconnect(m_activeDoc, &KTextEditor::Document::documentSavedOrUploaded, this, &ESLintPluginView::onSaved);
    }

    m_activeDoc = v ? v->document() : nullptr;

    if (m_activeDoc) {
        connect(m_activeDoc, &KTextEditor::Document::documentSavedOrUploaded, this, &ESLintPluginView::onSaved, Qt::QueuedConnection);
    }
}

static bool canLintDoc(KTextEditor::Document *d)
{
    if (!d || !d->url().isLocalFile()) {
        return false;
    }
    const auto mode = d->highlightingMode().toLower();
    return (mode == QLatin1String("javascript") || mode == QLatin1String("typescript") || mode == QLatin1String("typescript react (tsx)")
            || mode == QLatin1String("javascript react (jsx)"));
}

void ESLintPluginView::onSaved(KTextEditor::Document *)
{
    m_diagsWithFix.clear();
    if (!canLintDoc(m_activeDoc)) {
        return;
    }

    if (m_eslintProcess.state() == QProcess::Running) {
        return;
    }

    const auto name = safeExecutableName(QStringLiteral("npx"));
    if (name.isEmpty()) {
        // TODO: error
        return;
    }

    const auto docPath = m_activeDoc->url().toLocalFile();
    const auto workingDir = QFileInfo(docPath).absolutePath();
    m_eslintProcess.setWorkingDirectory(workingDir);

    const QStringList args{QStringLiteral("eslint"), QStringLiteral("-f"), QStringLiteral("json"), {m_activeDoc->url().toLocalFile()}};
    startHostProcess(m_eslintProcess, name, args);
}

static FileDiagnostics parseLine(const QString &line, std::vector<DiagnosticWithFix> &diagWithFix)
{
    QJsonParseError jsonErr;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(line.toUtf8(), &jsonErr);
    if (jsonErr.error != QJsonParseError::NoError) {
        return {};
    }

    const auto arr = jsonDoc.array();
    if (arr.empty()) {
        return {};
    }
    auto obj = arr.at(0).toObject();
    QUrl uri = QUrl::fromLocalFile(obj.value(QStringLiteral("filePath")).toString());
    if (!uri.isValid()) {
        return {};
    }

    FileDiagnostics fd;
    fd.uri = uri;
    const auto messages = obj.value(QStringLiteral("messages")).toArray();
    if (messages.empty()) {
        // No errors in this file
        return fd;
    }

    QList<Diagnostic> diags;
    diags.reserve(messages.size());
    for (const auto &m : messages) {
        const auto msg = m.toObject();
        if (msg.isEmpty()) {
            continue;
        }

        const int startLine = msg.value(QStringLiteral("line")).toInt() - 1;
        const int startColumn = msg.value(QStringLiteral("column")).toInt() - 1;
        const int endLine = msg.value(QStringLiteral("endLine")).toInt() - 1;
        const int endColumn = msg.value(QStringLiteral("endColumn")).toInt() - 1;
        Diagnostic d;
        d.range = {startLine, startColumn, endLine, endColumn};
        if (!d.range.isValid()) {
            continue;
        }
        d.code = msg.value(QStringLiteral("ruleId")).toString();
        d.message = msg.value(QStringLiteral("message")).toString();
        d.source = QStringLiteral("eslint");
        const int severity = msg.value(QStringLiteral("severity")).toInt();
        if (severity == 1) {
            d.severity = DiagnosticSeverity::Warning;
        } else if (severity == 2) {
            d.severity = DiagnosticSeverity::Error;
        } else {
            // fallback, even though there is no other severity
            d.severity = DiagnosticSeverity::Information;
        }

        auto fixObject = msg.value(QStringLiteral("fix")).toObject();
        if (!fixObject.isEmpty()) {
            const auto rangeArray = fixObject.value(QStringLiteral("range")).toArray();
            int s{};
            int e{};
            if (rangeArray.size() == 2) {
                s = rangeArray[0].toInt(-1);
                e = rangeArray[1].toInt(-1);
                auto v = fixObject.value(QStringLiteral("text"));
                if (!v.isUndefined()) {
                    DiagnosticWithFix df;
                    d.message += QStringLiteral(" (fix available)");
                    df.diag = d;
                    df.fix = {.rangeStart = s, .rangeEnd = e, .text = v.toString()};
                    diagWithFix.push_back(df);
                }
            }
        }

        diags << d;
    }
    return {.uri = uri, .diagnostics = diags};
}

void ESLintPluginView::onReadyRead()
{
    /**
     * get results of analysis
     */
    QHash<QUrl, QList<Diagnostic>> fileDiagnostics;
    const auto out = m_eslintProcess.readAllStandardOutput().split('\n');
    for (const auto &rawLine : out) {
        /**
         * get one line, split it, skip it, if too few elements
         */
        QString line = QString::fromLocal8Bit(rawLine);
        FileDiagnostics fd = parseLine(line, m_diagsWithFix);
        if (!fd.uri.isValid()) {
            continue;
        }
        fileDiagnostics[fd.uri] << fd.diagnostics;
    }

    for (auto it = fileDiagnostics.cbegin(); it != fileDiagnostics.cend(); ++it) {
        m_provider.diagnosticsAdded(FileDiagnostics{.uri = it.key(), .diagnostics = it.value()});
    }
}

void ESLintPluginView::onError()
{
    const QString err = QString::fromUtf8(m_eslintProcess.readAllStandardError());
    if (!err.isEmpty()) {
        const QString message = i18n("Eslint failed, error: %1", err);
        Utils::showMessage(message, {}, i18n("ESLint"), MessageType::Warning, m_mainWindow);
    }
}

void ESLintPluginView::onFixesRequested(const QUrl &u, const Diagnostic &d, const QVariant &v)
{
    if (m_diagsWithFix.empty()) {
        return;
    }

    for (const auto &fd : m_diagsWithFix) {
        const auto &diag = fd.diag;
        if (diag.range == d.range && diag.code == d.code && diag.message == d.message) {
            DiagnosticFix f;
            f.fixTitle = QStringLiteral("replace with %1").arg(fd.fix.text);
            f.fixCallback = [u, fix = fd.fix, this] {
                fixDiagnostic(u, fix);
            };
            Q_EMIT m_provider.fixesAvailable({f}, v);
        }
    }
}

void ESLintPluginView::fixDiagnostic(const QUrl &url, const DiagnosticWithFix::Fix &fix)
{
    KTextEditor::Document *d = nullptr;
    if (m_activeDoc && m_activeDoc->url() == url) {
        d = m_activeDoc;
    } else {
        d = KTextEditor::Editor::instance()->application()->findUrl(url);
    }
    if (!d) {
        const QString message = i18n("Failed to find doc with url %1", url.toLocalFile());
        Utils::showMessage(message, {}, i18n("ESLint"), MessageType::Info, m_mainWindow);
        return;
    }

    KTextEditor::Cursor s = d->offsetToCursor(fix.rangeStart);
    KTextEditor::Cursor e = d->offsetToCursor(fix.rangeEnd);
    if (s.isValid() && e.isValid()) {
        d->replaceText({s, e}, fix.text);
    }
}

#include "EslintPlugin.moc"
