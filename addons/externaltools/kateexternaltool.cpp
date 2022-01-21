/* This file is part of the KDE project
 *
 *  SPDX-FileCopyrightText: 2019 Dominik Haumann <dhaumann@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
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
    Q_ASSERT(false); // you forgot a case above
    return QStringLiteral("None");
}

KateExternalTool::SaveMode toSaveMode(const QString &mode)
{
    if (mode == QStringLiteral("None")) {
        return KateExternalTool::SaveMode::None;
    }
    if (mode == QStringLiteral("CurrentDocument")) {
        return KateExternalTool::SaveMode::CurrentDocument;
    }
    if (mode == QStringLiteral("AllDocuments")) {
        return KateExternalTool::SaveMode::AllDocuments;
    }
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
    Q_ASSERT(false); // you forgot a case above
    return QStringLiteral("Ignore");
}

KateExternalTool::OutputMode toOutputMode(const QString &mode)
{
    if (mode == QStringLiteral("Ignore")) {
        return KateExternalTool::OutputMode::Ignore;
    }
    if (mode == QStringLiteral("InsertAtCursor")) {
        return KateExternalTool::OutputMode::InsertAtCursor;
    }
    if (mode == QStringLiteral("ReplaceSelectedText")) {
        return KateExternalTool::OutputMode::ReplaceSelectedText;
    }
    if (mode == QStringLiteral("ReplaceCurrentDocument")) {
        return KateExternalTool::OutputMode::ReplaceCurrentDocument;
    }
    if (mode == QStringLiteral("AppendToCurrentDocument")) {
        return KateExternalTool::OutputMode::AppendToCurrentDocument;
    }
    if (mode == QStringLiteral("InsertInNewDocument")) {
        return KateExternalTool::OutputMode::InsertInNewDocument;
    }
    if (mode == QStringLiteral("CopyToClipboard")) {
        return KateExternalTool::OutputMode::CopyToClipboard;
    }
    if (mode == QStringLiteral("DisplayInPane")) {
        return KateExternalTool::OutputMode::DisplayInPane;
    }
    return KateExternalTool::OutputMode::Ignore;
}

KateExternalTool::Trigger toTrigger(const QString &trigger)
{
    if (trigger == QStringLiteral("None")) {
        return KateExternalTool::Trigger::None;
    }
    if (trigger == QStringLiteral("BeforeSave")) {
        return KateExternalTool::Trigger::BeforeSave;
    }
    if (trigger == QStringLiteral("AfterSave")) {
        return KateExternalTool::Trigger::AfterSave;
    }
    return KateExternalTool::Trigger::None;
}

QString toString(KateExternalTool::Trigger trigger)
{
    if (trigger == KateExternalTool::Trigger::None) {
        return QStringLiteral("None");
    }
    if (trigger == KateExternalTool::Trigger::BeforeSave) {
        return QStringLiteral("BeforeSave");
    }
    if (trigger == KateExternalTool::Trigger::AfterSave) {
        return QStringLiteral("AfterSave");
    }
    return QStringLiteral("None");
}
}

bool KateExternalTool::checkExec() const
{
    return !QStandardPaths::findExecutable(executable).isEmpty();
}

bool KateExternalTool::matchesMimetype(const QString &mt) const
{
    return mimetypes.contains(mt);
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
    trigger = toTrigger(cg.readEntry("trigger", "None"));

    hasexec = checkExec();
}

template < class Value >
static inline void writeEntryMaybe(KConfigGroup &cg, const char *key, const Value &value)
{
    if (value.isEmpty()) {
        cg.deleteEntry(key);
    }
    else {
        cg.writeEntry(key, value);
    }
}

void KateExternalTool::save(KConfigGroup &cg) const
{
    writeEntryMaybe(cg, "category", category);
    writeEntryMaybe(cg, "name", name);
    writeEntryMaybe(cg, "icon", icon);
    writeEntryMaybe(cg, "executable", executable);
    writeEntryMaybe(cg, "arguments", arguments);
    writeEntryMaybe(cg, "input", input);
    writeEntryMaybe(cg, "workingDir", workingDir);
    writeEntryMaybe(cg, "mimetypes", mimetypes);
    writeEntryMaybe(cg, "actionName", actionName);
    writeEntryMaybe(cg, "cmdname", cmdname);
    writeEntryMaybe(cg, "save", toString(saveMode));
    writeEntryMaybe(cg, "output", toString(outputMode));
    writeEntryMaybe(cg, "trigger", toString(trigger));

    // a logical value is never empty
    cg.writeEntry("reload", reload);
}

QString KateExternalTool::translatedName() const
{
    return name.isEmpty() ? QString() : i18nc("External tool name", name.toUtf8().data());
}

QString KateExternalTool::translatedCategory() const
{
    return category.isEmpty() ? QString() : i18nc("External tool category", category.toUtf8().data());
}

bool operator==(const KateExternalTool &lhs, const KateExternalTool &rhs)
{
    return lhs.category == rhs.category && lhs.name == rhs.name && lhs.icon == rhs.icon && lhs.executable == rhs.executable && lhs.arguments == rhs.arguments
        && lhs.input == rhs.input && lhs.workingDir == rhs.workingDir && lhs.mimetypes == rhs.mimetypes && lhs.actionName == rhs.actionName
        && lhs.cmdname == rhs.cmdname && lhs.saveMode == rhs.saveMode && lhs.reload == rhs.reload && lhs.outputMode == rhs.outputMode
        && lhs.trigger == rhs.trigger;
}

// kate: space-indent on; indent-width 4; replace-tabs on;
