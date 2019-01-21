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
#include "externaltools.h"
#include "externaltoolsplugin.h"

KateExternalToolsCommand::KateExternalToolsCommand(KateExternalToolsPlugin* plugin)
    : KTextEditor::Command(plugin->commands())
    , m_plugin(plugin)
{
}

bool KateExternalToolsCommand::exec(KTextEditor::View* view, const QString& cmd, QString& msg,
                                    const KTextEditor::Range& range)
{
    Q_UNUSED(msg)
    Q_UNUSED(range)

    const QString command = cmd.trimmed();
    const auto tool = m_plugin->toolForCommand(command);
    if (tool) {
        m_plugin->runTool(*tool, view);
        return true;
    }
    return false;
}

bool KateExternalToolsCommand::help(KTextEditor::View*, const QString&, QString&)
{
    return false;
}

// kate: space-indent on; indent-width 4; replace-tabs on;
