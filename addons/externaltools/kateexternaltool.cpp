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
#include <QStandardPaths>

bool KateExternalTool::checkExec() const
{
    return !QStandardPaths::findExecutable(executable).isEmpty();
}

bool KateExternalTool::matchesMimetype(const QString& mt) const
{
    return mimetypes.isEmpty() || mimetypes.contains(mt);
}

void KateExternalTool::load(const KConfigGroup& cg)
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
    saveMode = static_cast<KateExternalTool::SaveMode>(cg.readEntry("save", 0));
    includeStderr = cg.readEntry("includeStderr", false);

    hasexec = checkExec();
}

void KateExternalTool::save(KConfigGroup& cg) const
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
    cg.writeEntry("save", static_cast<int>(saveMode));
    cg.writeEntry("includeStderr", includeStderr);
}

bool operator==(const KateExternalTool & lhs, const KateExternalTool & rhs)
{
    return lhs.category == rhs.category
        && lhs.name == rhs.name
        && lhs.icon == rhs.icon
        && lhs.executable == rhs.executable
        && lhs.arguments == rhs.arguments
        && lhs.input == rhs.input
        && lhs.workingDir == rhs.workingDir
        && lhs.mimetypes == rhs.mimetypes
        && lhs.actionName == rhs.actionName
        && lhs.cmdname == rhs.cmdname
        && lhs.saveMode == rhs.saveMode
        && lhs.includeStderr == rhs.includeStderr;
}

// kate: space-indent on; indent-width 4; replace-tabs on;
