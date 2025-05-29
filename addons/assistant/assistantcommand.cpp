/*
 *  SPDX-FileCopyrightText: 2025 Christoph Cullmann <cullmann@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "assistantcommand.h"
#include "assistant.h"

#include <KLocalizedString>

using namespace Qt::Literals::StringLiterals;

AssistantCommand::AssistantCommand(Assistant *plugin)
    : KTextEditor::Command({u"ai"_s}, plugin)
    , m_assistant(plugin)
{
}

AssistantCommand::~AssistantCommand()
{
}

// execute a command
bool AssistantCommand::exec(KTextEditor::View *view, const QString &cmd, QString &msg, const KTextEditor::Range &range)
{
    return false;
}

// output help for a command
bool AssistantCommand::help(KTextEditor::View *view, const QString &cmd, QString &msg)
{
    if (cmd.startsWith(QLatin1String("ai"))) {
        msg = i18n("Usage: ai [additional commands added to the selection/current line]");
    }
    return true;
}
