/*
 *  SPDX-FileCopyrightText: 2025 Christoph Cullmann <cullmann@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <KTextEditor/Command>

class Assistant;

class AssistantCommand : public KTextEditor::Command
{
public:
    explicit AssistantCommand(Assistant *plugin);
    ~AssistantCommand();

    // execute a command
    bool exec(KTextEditor::View *view, const QString &cmd, QString &msg, const KTextEditor::Range &range = KTextEditor::Range::invalid()) override;

    // output help for a command
    bool help(KTextEditor::View *view, const QString &cmd, QString &msg) override;

private:
    Assistant *const m_assistant;
};
