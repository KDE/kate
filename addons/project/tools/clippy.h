/**
 *  SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */
#pragma once

#include "kateproject.h"
#include <kateprojectcodeanalysistool.h>

#include <KTextEditor/Document>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>

#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QStringLiteral>

class Clippy : public KateProjectCodeAnalysisTool
{
public:
    using KateProjectCodeAnalysisTool::KateProjectCodeAnalysisTool;

    QString name() const override
    {
        return i18n("Clippy (Rust)");
    }

    QString description() const override
    {
        return i18n("Clippy is a static analysis tool for Rust code.");
    }

    QString fileExtensions() const override
    {
        return QStringLiteral("rs");
    }

    QStringList filter(const QStringList &files) const override
    {
        // js/ts files
        return files.filter(
            QRegularExpression(QStringLiteral("\\.(") + fileExtensions().replace(QStringLiteral("+"), QStringLiteral("\\+")) + QStringLiteral(")$")));
    }

    QString path() const override
    {
        return QStringLiteral("cargo");
    }

    QString getNearestManifestPath(KTextEditor::MainWindow *mainWindow)
    {
        if (auto v = mainWindow->activeView()) {
            QString path = v->document()->url().toLocalFile();
            if (!path.isEmpty()) {
                QDir d(path);
                while (d.cdUp()) {
                    if (d.exists(QStringLiteral("Cargo.toml"))) {
                        return d.absoluteFilePath(QStringLiteral("Cargo.toml"));
                    }
                }
            }
        }
        return {};
    }

    QStringList arguments() override
    {
        if (!m_project) {
            return {};
        }

        QStringList args;
        QString manifestPath = getNearestManifestPath(m_mainWindow);

        args << QStringLiteral("clippy");
        if (!manifestPath.isEmpty()) {
            args << QStringLiteral("--manifest-path");
            args << manifestPath;
        }
        args << QStringLiteral("--message-format");
        args << QStringLiteral("json");
        args << QStringLiteral("--quiet");
        args << QStringLiteral("--no-deps");
        args << QStringLiteral("--offline");

        setActualFilesCount(m_project->files().size());

        return args;
    }

    QString notInstalledMessage() const override
    {
        return i18n("Please install 'cargo'.");
    }

    struct FileRange {
        QString file;
        KTextEditor::Range range;
    };

    static FileRange sourceLocationFromSpans(const QJsonArray &spans)
    {
        int lineStart = -1;
        int lineEnd = -1;
        int colStart = -1;
        int colEnd = -1;
        QString file;
        for (const auto span : spans) {
            auto obj = span.toObject();
            lineStart = obj.value(u"line_start").toInt() - 1;
            lineEnd = obj.value(u"line_end").toInt() - 1;
            colStart = obj.value(u"column_start").toInt() - 1;
            colEnd = obj.value(u"column_end").toInt() - 1;
            file = obj.value(u"file_name").toString();
            break;
        }
        return {file, KTextEditor::Range(lineStart, colStart, lineEnd, colEnd)};
    }

    FileDiagnostics parseLine(const QString &line) const override
    {
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8(), &err);
        if (err.error != QJsonParseError::NoError) {
            return {};
        }

        auto topLevelObj = doc.object();
        auto reason = topLevelObj.value(QLatin1String("reason"));
        if (reason.isNull() || reason.isUndefined() || reason.toString() != QStringLiteral("compiler-message")) {
            return {};
        }

        QDir manifest_path = topLevelObj.value(u"manifest_path").toString();
        manifest_path.cdUp();
        if (!manifest_path.exists()) {
            qDebug("invalid uri");
            return {};
        }

        const auto message = topLevelObj.value(QLatin1String("message")).toObject();
        if (message.isEmpty()) {
            qDebug("invalid message");
            return {};
        }

        QString code = message.value(u"code").toObject().value(u"code").toString();
        QString level = message.value(u"level").toString().toLower();
        QString diagMessage = message.value(u"message").toString();
        const auto spans = message.value(u"spans").toArray();
        const auto [fileName, range] = sourceLocationFromSpans(spans);

        Diagnostic diag;
        diag.severity = [&level]() {
            if (level == u"warning") {
                return DiagnosticSeverity::Warning;
            }
            if (level == u"error") {
                return DiagnosticSeverity::Error;
            }
            return DiagnosticSeverity::Warning;
        }();
        diag.code = code;
        diag.message = diagMessage;
        diag.range = range;
        if (!diag.range.isValid()) {
            return {};
        }

        QUrl uri = QUrl::fromLocalFile(manifest_path.absoluteFilePath(fileName));
        if (!uri.isValid()) {
            return {};
        }

        const auto children = message.value(QLatin1String("children")).toArray();
        for (const auto &child : children) {
            auto obj = child.toObject();
            QString msg = obj.value(u"message").toString();
            if (msg.isEmpty()) {
                continue;
            }
            auto spans = obj.value(u"spans").toArray();
            auto [_, range] = sourceLocationFromSpans(spans);
            QUrl u = uri;
            if (!spans.isEmpty()) {
                auto rep = spans.first().toObject().value(u"suggested_replacement").toString();
                if (!rep.isEmpty()) {
                    msg += QLatin1String(": `");
                    msg += spans.first().toObject().value(u"suggested_replacement").toString();
                    msg += QLatin1String("`");
                }
            }

            if (!range.isValid()) {
                range = diag.range;
            }
            diag.relatedInformation.push_back(DiagnosticRelatedInformation{.location = {.uri = u, .range = range}, .message = msg});
        }

        // qDebug() << uri << diag.message << diag.range;
        return {.uri = uri, .diagnostics = {diag}};
    }

    QString stdinMessages() override
    {
        return QString();
    }

    bool isSuccessfulExitCode(int exitCode) const override
    {
        return 0 == exitCode;
    }
};
