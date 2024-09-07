/*  This file is part of the Kate project.
 *
 *  SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */
#pragma once

#include "clazy.h"

#include <KLocalizedString>

#include <KTextEditor/Document>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>

class KateProjectCodeAnalysisToolClazyCurrent : public KateProjectCodeAnalysisToolClazy
{
public:
    using KateProjectCodeAnalysisToolClazy::KateProjectCodeAnalysisToolClazy;

    QString name() const override
    {
        return i18n("Clazy - Current File");
    }

    QString description() const override
    {
        return i18n("clang-tidy is a clang-based C++ “linter” tool");
    }

    QStringList arguments() override
    {
        if (!m_project || !m_mainWindow || !m_mainWindow->activeView()) {
            return {};
        }

        QString compileCommandsDir = compileCommandsDirectory();

        QStringList args;
        if (!compileCommandsDir.isEmpty()) {
            args << QStringList{QStringLiteral("-p"), compileCommandsDir};
        }
        setActualFilesCount(1);

        const QString file = m_mainWindow->activeView()->document()->url().toLocalFile();
        args.append(file);

        return args;
    }
};
