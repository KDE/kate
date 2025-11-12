/*  This file is part of the Kate project.
 *
 *  SPDX-FileCopyrightText: 2017 Héctor Mesa Jiménez <hector@lcc.uma.es>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "../kateprojectcodeanalysistool.h"
#include "kateproject.h"

#include <KLocalizedString>
#include <QRegularExpression>

/**
 * Information provider for flake8
 */
class KateProjectCodeAnalysisToolFlake8 : public KateProjectCodeAnalysisTool
{
public:
    using KateProjectCodeAnalysisTool::KateProjectCodeAnalysisTool;

    QString name() const override
    {
        return i18n("Flake8 (Python)");
    }

    QString description() const override
    {
        return i18n("Flake8: Your Tool For Style Guide Enforcement for Python");
    }

    QString fileExtensions() const override
    {
        return QStringLiteral("py");
    }

    QStringList filter(const QStringList &files) const override
    {
        // for now we expect files with extension
        return files.filter(QRegularExpression(QStringLiteral("\\.(") + fileExtensions() + QStringLiteral(")$")));
    }

    QString path() const override
    {
        /*
         * for now, only the executable in the path can be called,
         * but it would be great to be able to specify a version
         * installed in a virtual environment
         */
        return QStringLiteral("flake8");
    }

    QStringList arguments() override
    {
        QStringList _args;

        _args << QStringLiteral("--exit-zero")
              /*
               * translating a flake8 code to a severity level is subjective,
               * so the code is provided as a severity level.
               */
              << QStringLiteral("--format=%(path)s////%(row)d////%(code)s////%(text)s");

        if (m_project) {
            auto &&fileList = filter(m_project->files());
            setActualFilesCount(fileList.size());
            _args.append(fileList);
        }

        return _args;
    }

    QString notInstalledMessage() const override
    {
        return i18n("Please install 'flake8'.");
    }

    FileDiagnostics parseLine(const QString &line) const override
    {
        const QStringList elements = line.split(QLatin1String("////"), Qt::SkipEmptyParts);
        const auto url = QUrl::fromLocalFile(elements[0]);
        Diagnostic d;
        d.message = elements[3];
        d.severity = DiagnosticSeverity::Warning;
        int ln = elements[1].toInt() - 1;
        d.range = KTextEditor::Range(ln, 0, ln, -1);
        return {.uri = url, .diagnostics = {d}};
    }

    QString stdinMessages() override
    {
        return {};
    }
};
