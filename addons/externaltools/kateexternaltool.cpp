/*
   This file is part of the Kate text editor of the KDE project.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.

   ---
   Copyright (C) 2019 Dominik Haumann <dhaumann@kde.org>
*/
#include "kateexternaltool.h"

#include <KConfigGroup>
#include <QStandardPaths>

KateExternalTool::KateExternalTool(const QString& name, const QString& command, const QString& icon,
                                   const QString& executable, const QStringList& mimetypes, const QString& actionName,
                                   const QString& cmdname, SaveMode saveMode)
    : name(name)
    , icon(icon)
    , executable(executable)
    , command(command)
    , mimetypes(mimetypes)
    , actionName(actionName)
    , cmdname(cmdname)
    , saveMode(saveMode)
{
    // if ( ! executable.isEmpty() )
    hasexec = checkExec();
}

bool KateExternalTool::checkExec()
{
    m_exec = QStandardPaths::findExecutable(executable);
    return !m_exec.isEmpty();
}

bool KateExternalTool::valid(const QString& mt) const
{
    return mimetypes.isEmpty() || mimetypes.contains(mt);
}

void KateExternalTool::load(const KConfigGroup & cg)
{
    name = cg.readEntry("name", "");
    command = cg.readEntry("command", "");
    icon = cg.readEntry("icon", "");
    executable = cg.readEntry("executable", "");
    mimetypes = cg.readEntry("mimetypes", QStringList());
    actionName = cg.readEntry("actionName");
    cmdname = cg.readEntry("cmdname");
    saveMode = static_cast<KateExternalTool::SaveMode>(cg.readEntry("save", 0));
    includeStderr = cg.readEntry("includeStderr", false);

    hasexec = checkExec();
}

void KateExternalTool::save(KConfigGroup & cg)
{
    cg.writeEntry("name", name);
    cg.writeEntry("command", command);
    cg.writeEntry("icon", icon);
    cg.writeEntry("executable", executable);
    cg.writeEntry("mimetypes", mimetypes);
    cg.writeEntry("actionName", actionName);
    cg.writeEntry("cmdname", cmdname);
    cg.writeEntry("save", static_cast<int>(saveMode));
    cg.writeEntry("includeStderr", includeStderr);
}

// kate: space-indent on; indent-width 4; replace-tabs on;
