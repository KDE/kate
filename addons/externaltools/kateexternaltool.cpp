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

#include <KConfig>
#include <KConfigGroup>
#include <QStandardPaths>

KateExternalTool::KateExternalTool(const QString& name, const QString& command, const QString& icon,
                                   const QString& executable, const QStringList& mimetypes, const QString& acname,
                                   const QString& cmdname, int save)
    : name(name)
    , icon(icon)
    , executable(executable)
    , command(command)
    , mimetypes(mimetypes)
    , acname(acname)
    , cmdname(cmdname)
    , save(save)
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

// kate: space-indent on; indent-width 4; replace-tabs on;
