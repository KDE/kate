/*  This file is part of the Kate project.
 *
 *  SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */
#pragma once

#include <kateprojectcodeanalysistool.h>

#include "kateproject.h"

#include <KLocalizedString>

#include <QDir>
#include <QRegularExpression>

class KateProjectCodeAnalysisToolClazy : public KateProjectCodeAnalysisTool
{
public:
    using KateProjectCodeAnalysisTool::KateProjectCodeAnalysisTool;

    QString name() const override
    {
        return i18n("Clazy (Qt/C++)");
    }

    QString description() const override
    {
        return i18n("Clazy is a static analysis tool for Qt/C++ code");
    }

    QString fileExtensions() const override
    {
        return QStringLiteral("cpp|cxx|cc|c++|tpp|txx");
    }

    QStringList filter(const QStringList &files) const override
    {
        // c++ files
        return files.filter(
            QRegularExpression(QStringLiteral("\\.(") + fileExtensions().replace(QStringLiteral("+"), QStringLiteral("\\+")) + QStringLiteral(")$")));
    }

    QString path() const override
    {
        return QStringLiteral("clazy-standalone");
    }

    QStringList arguments() override
    {
        if (!m_project) {
            return {};
        }

        QString compileCommandsDir = compileCommandsDirectory();

        QStringList args;
        if (!compileCommandsDir.isEmpty()) {
            args = QStringList{QStringLiteral("-p"), compileCommandsDir};
        }

        const QStringList fileList = filter(m_project->files());
        setActualFilesCount(fileList.size());

        return args << fileList;
    }

    QString notInstalledMessage() const override
    {
        return i18n("Please install 'clazy'.");
    }

    FileDiagnostics parseLine(const QString &line) const override
    {
        //"/path/kate/kate/kateapp.cpp:529:10: warning: Missing reference in range-for with non trivial type (QJsonValue) [-Wclazy-range-loop]"
        int idxColon = line.indexOf(QLatin1Char(':'));
        if (idxColon < 0) {
            return {};
        }
        // File
        const QString file = line.mid(0, idxColon);
        idxColon++;

        // Line
        int nextColon = line.indexOf(QLatin1Char(':'), idxColon);
        if (nextColon < 0) {
            return {};
        }
        const QString lineNo = line.mid(idxColon, nextColon - idxColon);
        bool ok = true;
        lineNo.toInt(&ok);
        if (!ok) {
            return {};
        }
        idxColon = nextColon + 1;

        // Column
        nextColon = line.indexOf(QLatin1Char(':'), idxColon);
        const QString columnNo = line.mid(idxColon, nextColon - idxColon);
        idxColon = nextColon + 1;

        int spaceIdx = line.indexOf(QLatin1Char(' '), nextColon);
        if (spaceIdx < 0) {
            return {};
        }

        idxColon = line.indexOf(QLatin1Char(':'), spaceIdx);
        if (idxColon < 0) {
            return {};
        }

        const QString severity = line.mid(spaceIdx + 1, idxColon - (spaceIdx + 1));

        idxColon++;
        QString msg = line.mid(idxColon);

        // Code e.g [-Wclazy-range-loop]
        QString code;
        {
            int bracketOpen = msg.lastIndexOf(QLatin1Char('['));
            int bracketClose = msg.lastIndexOf(QLatin1Char(']'));
            if (bracketOpen > 0 && bracketClose > 0) {
                code = msg.mid(bracketOpen + 1, bracketClose - bracketOpen);
                // remove code from msg
                msg.remove(bracketOpen, (bracketClose - bracketOpen) + 1);
            }
        }

        const auto url = QUrl::fromLocalFile(file);
        Diagnostic d;
        d.message = msg;
        d.severity = DiagnosticSeverity::Warning;
        d.code = code;
        int ln = lineNo.toInt() - 1;
        int col = columnNo.toInt() - 1;
        col = col < 0 ? 0 : col;
        d.range = KTextEditor::Range(ln, col, ln, col);
        return {.uri = url, .diagnostics = {d}};
    }

    QString stdinMessages() override
    {
        return {};
    }

    static QString buildDirectory(const QVariantMap &projectMap)
    {
        const QVariantMap buildMap = projectMap[QStringLiteral("build")].toMap();
        const QString buildDir = buildMap[QStringLiteral("directory")].toString();
        return buildDir;
    }

    QString compileCommandsDirectory() const
    {
        QString buildDir = buildDirectory(m_project->projectMap());
        const QString compCommandsFile = QStringLiteral("compile_commands.json");

        if (buildDir.startsWith(QLatin1String("./"))) {
            buildDir = buildDir.mid(2);
        }

        /**
         * list of absolute paths to check compile commands
         */
        const QString possiblePaths[4] = {
            /** Absolute build path in .kateproject e.g from cmake */
            buildDir,
            /** Relative path in .kateproject e.g */
            m_project->baseDir() + (buildDir.startsWith(QLatin1Char('/')) ? buildDir : QLatin1Char('/') + buildDir),
            /** Check for the commonly existing "build/" directory */
            m_project->baseDir() + QStringLiteral("/build"),
            /** Project base, maybe it has a symlink to compile_commands.json file */
            m_project->baseDir(),
        };

        /**
         * Check all paths one by one for compile_commands.json and exit when found
         */
        QString compileCommandsDir;
        for (const QString &path : possiblePaths) {
            if (path.isEmpty()) {
                continue;
            }
            const QString guessedPath = QDir(path).filePath(compCommandsFile);
            const bool dirHasCompileComds = QFile::exists(guessedPath);
            if (dirHasCompileComds) {
                compileCommandsDir = guessedPath;
                break;
            }
        }

        return compileCommandsDir;
    }
};
