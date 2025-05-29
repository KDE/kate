/*
 *  SPDX-FileCopyrightText: 2025 Christoph Cullmann <cullmann@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "assistantcommand.h"
#include "assistant.h"

#include <KLocalizedString>
#include <KTextEditor/Document>
#include <KTextEditor/View>

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
bool AssistantCommand::exec(KTextEditor::View *view, const QString &cmd, QString &msg, const KTextEditor::Range &)
{
    if (cmd == u"ai"_s) {
        // we either use the selection or the current line
        // we need to remember where to add the result
        QString prompt = view->selectionText().trimmed();
        int lineToInsertResult = view->selectionRange().end().line() + 1;
        if (prompt.isEmpty()) {
            // try current line
            prompt = view->document()->line(view->cursorPosition().line()).trimmed();
            lineToInsertResult = view->cursorPosition().line() + 1;
        }
        if (prompt.isEmpty()) {
            msg = i18n("Selection and current line contain no text, no assistance possible.");
            return false;
        }

        // send prompt, insert result in line below the request line
        m_assistant->sendPrompt(prompt, view, [view, lineToInsertResult](const QString &answer) {
            view->document()->insertText(KTextEditor::Cursor(lineToInsertResult, 0), answer);
        });
        return true;
    }

    // unknown command
    return false;
}

// output help for a command
bool AssistantCommand::help(KTextEditor::View *, const QString &cmd, QString &msg)
{
    if (cmd == u"ai"_s) {
        msg = i18n(
            "<p>ai</p>"
            "<p>Send the selected text or current line as prompt to the completion model.</p>");
        return true;
    }

    // unknown command
    return false;
}
