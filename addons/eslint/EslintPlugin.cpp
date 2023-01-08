/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "EslintPlugin.h"

#include <KLocalizedString>
#include <KPluginFactory>
#include <KSharedConfig>
#include <KTextEditor/View>
#include <KXMLGUIFactory>

#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>

#include <hostprocess.h>
#include <ktexteditor_utils.h>


K_PLUGIN_FACTORY_WITH_JSON(ESLintPluginFactory, "EslintPlugin.json", registerPlugin<ESLintPlugin>();)

ESLintPlugin::ESLintPlugin(QObject *parent, const QVariantList &)
    : KTextEditor::Plugin(parent)
{
}

QObject *ESLintPlugin::createView(KTextEditor::MainWindow *mainWindow)
{
    return new ESLintPluginView(this, mainWindow);
}

ESLintPluginView::ESLintPluginView(ESLintPlugin *plugin, KTextEditor::MainWindow *mainWin)
    : QObject(plugin)
    , m_plugin(plugin)
    , m_mainWindow(mainWin)
{
    Utils::registerDiagnosticsProvider(&m_provider, m_mainWindow);

    connect(mainWin, &KTextEditor::MainWindow::viewChanged, this, &ESLintPluginView::onActiveViewChanged);
    connect(&m_eslintProcess, &QProcess::readyReadStandardOutput, this, &ESLintPluginView::onReadyRead);
    connect(&m_eslintProcess, &QProcess::readyReadStandardError, this, &ESLintPluginView::onError);

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
    Utils::unregisterDiagnosticsProvider(&m_provider, m_mainWindow);
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
    return (mode == QStringLiteral("javascript") || mode == QStringLiteral("typescript") || mode == QStringLiteral("typescript react (tsx)")
            || mode == QStringLiteral("javascript react (jsx)"));
}

void ESLintPluginView::onSaved(KTextEditor::Document *d)
{
    if (!canLintDoc(d)) {
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
    const QStringList args{QStringLiteral("eslint"), QStringLiteral("-f"), QStringLiteral("json"), {d->url().toLocalFile()}};
    startHostProcess(m_eslintProcess, name, args);
}

static FileDiagnostics parseLine(const QString &line)
{
    QJsonParseError e;
    QJsonDocument d = QJsonDocument::fromJson(line.toUtf8(), &e);
    if (e.error != QJsonParseError::NoError) {
        return {};
    }

    const auto arr = d.array();
    if (arr.empty()) {
        return {};
    }
    auto obj = arr.at(0).toObject();
    QUrl uri = QUrl::fromLocalFile(obj.value(QStringLiteral("filePath")).toString());
    if (!uri.isValid()) {
        return {};
    }
    const auto messages = obj.value(QStringLiteral("messages")).toArray();
    if (messages.empty()) {
        return {};
    }

    QVector<Diagnostic> diags;
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
        diags << d;
    }
    return {uri, diags};
}

void ESLintPluginView::onReadyRead()
{
    /**
     * get results of analysis
     */
    QHash<QUrl, QVector<Diagnostic>> fileDiagnostics;
    const auto out = m_eslintProcess.readAllStandardOutput().split('\n');
    for (const auto &rawLine : out) {
        /**
         * get one line, split it, skip it, if too few elements
         */
        QString line = QString::fromLocal8Bit(rawLine);
        FileDiagnostics fd = parseLine(line);
        if (!fd.uri.isValid()) {
            continue;
        }
        fileDiagnostics[fd.uri] << fd.diagnostics;
    }

    for (auto it = fileDiagnostics.cbegin(); it != fileDiagnostics.cend(); ++it) {
        m_provider.diagnosticsAdded(FileDiagnostics{it.key(), it.value()});
    }
}

void ESLintPluginView::onError()
{
    const QString err = QString::fromUtf8(m_eslintProcess.readAllStandardError());
    if (!err.isEmpty()) {
        const QString message = i18n("Eslint failed, error: %1", err);
        Utils::showMessage(message, {}, i18n("ESLint"), QStringLiteral("Error"), m_mainWindow);
    }
}

#include "EslintPlugin.moc"
