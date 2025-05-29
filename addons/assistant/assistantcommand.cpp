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
    if (cmd == u"ai"_s) {
        msg = i18n(
            "<p>ai</p>"
            "<p>Send the selected text or current line if there is no selection as prompt to the completion model.</p>");
    }
    return true;
}

// output help for a command
bool AssistantCommand::help(KTextEditor::View *, const QString &cmd, QString &msg)
{
    if (cmd == u"ai"_s) {
        msg = i18n(
            "<p>ai</p>"
            "<p>Send the selected text or current line if there is no selection as prompt to the completion model.</p>");
    }
    return true;
}
