/* This file is part of the KDE project
 *
 *  Copyright 2019 Dominik Haumann <dhaumann@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */
#include "kateexternaltool.h"

#include <KConfigGroup>
#include <KLocalizedString>
#include <QStandardPaths>

namespace
{
QString toString(KateExternalTool::SaveMode saveMode)
{
    switch (saveMode) {
    case KateExternalTool::SaveMode::None:
        return QStringLiteral("None");
    case KateExternalTool::SaveMode::CurrentDocument:
        return QStringLiteral("CurrentDocument");
    case KateExternalTool::SaveMode::AllDocuments:
        return QStringLiteral("AllDocuments");
    }
    Q_ASSERT(false); // yout forgot a case above
    return QStringLiteral("None");
}

KateExternalTool::SaveMode toSaveMode(const QString &mode)
{
    if (mode == QStringLiteral("None"))
        return KateExternalTool::SaveMode::None;
    if (mode == QStringLiteral("CurrentDocument"))
        return KateExternalTool::SaveMode::CurrentDocument;
    if (mode == QStringLiteral("AllDocuments"))
        return KateExternalTool::SaveMode::AllDocuments;
    return KateExternalTool::SaveMode::None;
}

QString toString(KateExternalTool::OutputMode outputMode)
{
    switch (outputMode) {
    case KateExternalTool::OutputMode::Ignore:
        return QStringLiteral("Ignore");
    case KateExternalTool::OutputMode::InsertAtCursor:
        return QStringLiteral("InsertAtCursor");
    case KateExternalTool::OutputMode::ReplaceSelectedText:
        return QStringLiteral("ReplaceSelectedText");
    case KateExternalTool::OutputMode::ReplaceCurrentDocument:
        return QStringLiteral("ReplaceCurrentDocument");
    case KateExternalTool::OutputMode::AppendToCurrentDocument:
        return QStringLiteral("AppendToCurrentDocument");
    case KateExternalTool::OutputMode::InsertInNewDocument:
        return QStringLiteral("InsertInNewDocument");
    case KateExternalTool::OutputMode::CopyToClipboard:
        return QStringLiteral("CopyToClipboard");
    case KateExternalTool::OutputMode::DisplayInPane:
        return QStringLiteral("DisplayInPane");
    }
    Q_ASSERT(false); // yout forgot a case above
    return QStringLiteral("Ignore");
}

KateExternalTool::OutputMode toOutputMode(const QString &mode)
{
    if (mode == QStringLiteral("Ignore"))
        return KateExternalTool::OutputMode::Ignore;
    if (mode == QStringLiteral("InsertAtCursor"))
        return KateExternalTool::OutputMode::InsertAtCursor;
    if (mode == QStringLiteral("ReplaceSelectedText"))
        return KateExternalTool::OutputMode::ReplaceSelectedText;
    if (mode == QStringLiteral("ReplaceCurrentDocument"))
        return KateExternalTool::OutputMode::ReplaceCurrentDocument;
    if (mode == QStringLiteral("AppendToCurrentDocument"))
        return KateExternalTool::OutputMode::AppendToCurrentDocument;
    if (mode == QStringLiteral("InsertInNewDocument"))
        return KateExternalTool::OutputMode::InsertInNewDocument;
    if (mode == QStringLiteral("CopyToClipboard"))
        return KateExternalTool::OutputMode::CopyToClipboard;
    if (mode == QStringLiteral("DisplayInPane"))
        return KateExternalTool::OutputMode::DisplayInPane;
    return KateExternalTool::OutputMode::Ignore;
}
}

bool KateExternalTool::checkExec() const
{
    return !QStandardPaths::findExecutable(executable).isEmpty();
}

bool KateExternalTool::matchesMimetype(const QString &mt) const
{
    return mimetypes.isEmpty() || mimetypes.contains(mt);
}

void KateExternalTool::load(const KConfigGroup &cg)
{
    category = cg.readEntry("category", "");
    name = cg.readEntry("name", "");
    icon = cg.readEntry("icon", "");
    executable = cg.readEntry("executable", "");
    arguments = cg.readEntry("arguments", "");
    input = cg.readEntry("input", "");
    workingDir = cg.readEntry("workingDir", "");
    mimetypes = cg.readEntry("mimetypes", QStringList());
    actionName = cg.readEntry("actionName");
    cmdname = cg.readEntry("cmdname");
    saveMode = toSaveMode(cg.readEntry("save", "None"));
    reload = cg.readEntry("reload", false);
    outputMode = toOutputMode(cg.readEntry("output", "Ignore"));

    hasexec = checkExec();
}

void KateExternalTool::save(KConfigGroup &cg) const
{
    cg.writeEntry("category", category);
    cg.writeEntry("name", name);
    cg.writeEntry("icon", icon);
    cg.writeEntry("executable", executable);
    cg.writeEntry("arguments", arguments);
    cg.writeEntry("input", input);
    cg.writeEntry("workingDir", workingDir);
    cg.writeEntry("mimetypes", mimetypes);
    cg.writeEntry("actionName", actionName);
    cg.writeEntry("cmdname", cmdname);
    cg.writeEntry("save", toString(saveMode));
    cg.writeEntry("reload", reload);
    cg.writeEntry("output", toString(outputMode));
}

QString KateExternalTool::translatedName() const
{
    return name.isEmpty() ? QString() : i18n(name.toUtf8().data());
}

QString KateExternalTool::translatedCategory() const
{
    return category.isEmpty() ? QString() : i18n(category.toUtf8().data());
}

bool operator==(const KateExternalTool &lhs, const KateExternalTool &rhs)
{
    return lhs.category == rhs.category && lhs.name == rhs.name && lhs.icon == rhs.icon && lhs.executable == rhs.executable && lhs.arguments == rhs.arguments && lhs.input == rhs.input && lhs.workingDir == rhs.workingDir &&
        lhs.mimetypes == rhs.mimetypes && lhs.actionName == rhs.actionName && lhs.cmdname == rhs.cmdname && lhs.saveMode == rhs.saveMode && lhs.reload == rhs.reload && lhs.outputMode == rhs.outputMode;
}

// kate: space-indent on; indent-width 4; replace-tabs on;
